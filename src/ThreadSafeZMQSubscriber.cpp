#include "ThreadSafeZMQSubscriber.h"
#include <iostream>
#include "LoggerManager.h"

ThreadSafeZMQSubscriber::ThreadSafeZMQSubscriber(zmq::context_t& context, const std::string& address, const std::string& topicFilter, bool isBind)
    : context_(context), running_(true), address_(address), topic_filter_(topicFilter), isBind_(isBind)
{
    socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_SUB);
    if (isBind_) {
        socket_->bind(address_);
        spdlog::info("[Subscriber] Bound to {}", address_);
    }
    else {
        socket_->connect(address_);
        spdlog::info("[Subscriber] Connected to {}", address_);
    }

    socket_->set(zmq::sockopt::subscribe, topic_filter_);

    subscriber_thread_ = std::thread(&ThreadSafeZMQSubscriber::subscriber_loop, this);
}

ThreadSafeZMQSubscriber::~ThreadSafeZMQSubscriber()
{
    running_ = false;
    if (subscriber_thread_.joinable())
        subscriber_thread_.join();

    if (socket_) {
        socket_->close();
        spdlog::info("[Subscriber] Socket closed");
    }
}

void ThreadSafeZMQSubscriber::set_callback(MessageCallback cb)
{
    message_callback_ = std::move(cb);
}

void ThreadSafeZMQSubscriber::subscriber_loop()
{
    zmq::pollitem_t items[] = {
        { static_cast<void*>(*socket_), 0, ZMQ_POLLIN, 0 }
    };

    while (running_) {
        // 轮询 socket，超时 100ms
        zmq::poll(items, 1, std::chrono::milliseconds(200));

        // 如果有数据可读
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t topic_msg;
            zmq::message_t body_msg;

            if (!socket_->recv(topic_msg, zmq::recv_flags::none)) {
                spdlog::warn("[Subscriber] Failed to receive topic frame");
                continue;
            }

            if (!socket_->recv(body_msg, zmq::recv_flags::none)) {
                spdlog::warn("[Subscriber] Missing data frame");
                continue;
            }

            std::string topic(static_cast<char*>(topic_msg.data()), topic_msg.size());
            std::vector<uint8_t> data(
                static_cast<uint8_t*>(body_msg.data()),
                static_cast<uint8_t*>(body_msg.data()) + body_msg.size()
            );

            if (message_callback_) {
                message_callback_(topic, data);
            }

            spdlog::info("[Subscriber] Received topic: {}, size: {}", topic, body_msg.size());
        }
    }

    spdlog::debug("[Subscriber] subscriber_loop exited");
}
