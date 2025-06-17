#include "ThreadSafeZMQReplier.h"
#include "LoggerManager.h"
#include <iostream>
#include <chrono>

ThreadSafeZMQReplier::ThreadSafeZMQReplier(zmq::context_t& context, const std::string& address)
    : context_(context), address_(address), running_(true)
{
    socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_REP);
    socket_->bind(address_);
    spdlog::info("[Replier] Bound to {}", address_);

    replier_thread_ = std::thread(&ThreadSafeZMQReplier::replier_loop, this);
}

ThreadSafeZMQReplier::~ThreadSafeZMQReplier()
{
    spdlog::debug("[Replier] Destruct called");

    running_ = false;
    if (replier_thread_.joinable())
        replier_thread_.join();

    if (socket_) {
        socket_->close();
        spdlog::info("[Replier] Socket closed");
    }
}

void ThreadSafeZMQReplier::set_callback(MessageCallback cb)
{
    message_callback_ = std::move(cb);
    spdlog::debug("[Replier] Callback set");
}

void ThreadSafeZMQReplier::send_reply(const std::vector<uint8_t>& reply)
{
    std::lock_guard<std::mutex> lock(send_mutex_);
    zmq::message_t msg(reply.data(), reply.size());
    socket_->send(msg, zmq::send_flags::none);
    spdlog::info("[Replier] Sent response size: {}", reply.size());
}

void ThreadSafeZMQReplier::replier_loop()
{
    zmq::pollitem_t items[] = {
        { static_cast<void*>(*socket_), 0, ZMQ_POLLIN, 0 }
    };

    while (running_) {
        zmq::poll(items, 1, std::chrono::milliseconds(200));

        if (items[0].revents & ZMQ_POLLIN) {
            spdlog::debug("[Replier] Waiting msg...");
            zmq::message_t msg;
            if (!socket_->recv(msg, zmq::recv_flags::none)) {
                spdlog::warn("[Replier] Incomplete request received");
                continue;
            }

            std::vector<uint8_t> data(static_cast<uint8_t*>(msg.data()), static_cast<uint8_t*>(msg.data()) + msg.size());
            spdlog::info("[Replier] Received data size: {}", data.size());
            if (message_callback_) {
                message_callback_(data);
            }
        }
    }

    spdlog::debug("[Replier] Replier_loop exited");
}
