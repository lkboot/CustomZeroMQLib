#include "ThreadSafeZMQPuller.h"
#include <iostream>
#include "LoggerManager.h"

ThreadSafeZMQPuller::ThreadSafeZMQPuller(zmq::context_t& context, const std::string& address, bool isBind)
    : context_(context), running_(true), address_(address), isBind_(isBind)
{
    if (isBind) {
        socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PUSH);
        socket_->bind(address);
        spdlog::info("[Puller] Socket bound to: {}", address);
    }
    else {
        spdlog::info("[Puller] Socket connected to: {}", address);
        socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PULL);
        socket_->connect(address);
    }
    receiver_thread_ = std::thread(&ThreadSafeZMQPuller::puller_loop, this);

    spdlog::debug("[Puller] Constructed");
}

ThreadSafeZMQPuller::~ThreadSafeZMQPuller()
{
    running_ = false;

    if (receiver_thread_.joinable())
        receiver_thread_.join();    // 等待线程退出

    if (socket_) {
        socket_->close();
        spdlog::info("[Puller] Socket {} to {} closed", isBind_ ? "bound" : "connected", address_);
    }

    spdlog::debug("[Puller] Destructed");
}

void ThreadSafeZMQPuller::set_callback(MessageCallback callback)
{
    message_callback_ = std::move(callback);
}

void ThreadSafeZMQPuller::puller_loop()
{
    while (running_) {
        zmq::pollitem_t items[] = {
            { static_cast<void*>(*socket_), 0, ZMQ_POLLIN, 0 }
        };
        zmq::poll(items, 1, std::chrono::milliseconds(200));

        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            if (socket_->recv(msg, zmq::recv_flags::dontwait)) {
                std::vector<uint8_t> data(static_cast<uint8_t*>(msg.data()),
                    static_cast<uint8_t*>(msg.data()) + msg.size());
                if (message_callback_) {
                    message_callback_(data);
                }
                spdlog::info("[Puller] Received data size: {}", msg.size());
            }
            else {
                spdlog::warn("[Puller] Receive failed.");
            }
        }
    }
    spdlog::debug("[Puller] exit receiver_loop");
}
