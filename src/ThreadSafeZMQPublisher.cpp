#include "ThreadSafeZMQPublisher.h"
#include <iostream>
#include "LoggerManager.h"

ThreadSafeZMQPublisher::ThreadSafeZMQPublisher(zmq::context_t& context, const std::string& address, bool isBind)
    : context_(context), running_(true), address_(address), isBind_(isBind)
{
    socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PUB);
    if (isBind_) {
        socket_->bind(address_);
        spdlog::info("[Publisher] Bound to {}", address_);
    }
    else {
        socket_->connect(address_);
        spdlog::info("[Publisher] Connected to {}", address_);
    }

    publisher_thread_ = std::thread(&ThreadSafeZMQPublisher::publisher_loop, this);
}

ThreadSafeZMQPublisher::~ThreadSafeZMQPublisher()
{
    running_ = false;
    cv_.notify_all();
    if (publisher_thread_.joinable())
        publisher_thread_.join();

    if (socket_) {
        socket_->close();
        spdlog::info("[Publisher] Socket closed");
    }
}

void ThreadSafeZMQPublisher::publish_async(const std::string& topic, const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    send_queue_.push({ topic, data });
    cv_.notify_one();
}

void ThreadSafeZMQPublisher::publisher_loop()
{
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_.wait(lock, [this]() { return !send_queue_.empty() || !running_; });

        while (!send_queue_.empty()) {
            auto item = std::move(send_queue_.front());
            send_queue_.pop();
            lock.unlock();

            zmq::message_t topic_msg(item.topic.data(), item.topic.size());
            socket_->send(topic_msg, zmq::send_flags::sndmore);

            zmq::message_t body_msg(item.content.data(), item.content.size());
            auto res = socket_->send(body_msg, zmq::send_flags::none);

            if (!res.has_value()) {
                spdlog::warn("[Publisher] Send failed");
            }
            else {
                spdlog::info("[Publisher] Sent topic: {}, size: {}", item.topic.data(), item.content.size());
            }

            lock.lock();
        }
    }

    spdlog::debug("[Publisher] publisher_loop exited");
}
