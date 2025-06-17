#include "ThreadSafeZMQPusher.h"
#include <iostream>
#include "LoggerManager.h"

ThreadSafeZMQPusher::ThreadSafeZMQPusher(zmq::context_t& context, const std::string& address, bool isBind)
    : context_(context), running_(true), address_(address), isBind_(isBind)
{
    if (isBind) {
        socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PUSH);
        socket_->bind(address);
        spdlog::info("[Pusher] Socket bound to: {}", address);
    }
    else {
        socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PULL);
        socket_->connect(address);
        spdlog::info("[Pusher] Socket connected to: {}", address);
    }
    sender_thread_ = std::thread(&ThreadSafeZMQPusher::pusher_loop, this);

    spdlog::debug("[Pusher] Constructed");
}

ThreadSafeZMQPusher::~ThreadSafeZMQPusher()
{
    running_ = false;
    cv_.notify_all();

    if (sender_thread_.joinable())
        sender_thread_.join();  // 等待线程退出

    if (socket_) {
        socket_->close();
        spdlog::info("[Pusher] Socket {} to {} closed", isBind_ ? "bound" : "connected", address_);
    }

    spdlog::debug("[Pusher] Destructed");
}

void ThreadSafeZMQPusher::send_async(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    message_queue_.emplace(data);
    cv_.notify_one();
}

void ThreadSafeZMQPusher::pusher_loop()
{
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_.wait(lock, [this]() {
            return !message_queue_.empty() || !running_;
        });

        while (!message_queue_.empty()) {
            std::vector<uint8_t> data = std::move(message_queue_.front());
            message_queue_.pop();
            lock.unlock();

            zmq::pollitem_t items[] = {
                { static_cast<void*>(*socket_), 0, ZMQ_POLLOUT, 0 }
            };
            zmq::poll(items, 1, std::chrono::milliseconds(200));

            if (items[0].revents & ZMQ_POLLOUT) {
                zmq::message_t msg(data.data(), data.size());
                auto result = socket_->send(msg, zmq::send_flags::dontwait);
                if (!result.has_value()) {
                    spdlog::warn("[Pusher] Send failed.");
                }
                else {
                    spdlog::info("[Pusher] Sent data size: {}", data.size());
                }
            }
            else {
                spdlog::warn("[Pusher] Socket not ready to send.");
            }

            lock.lock();
        }

    }
    spdlog::debug("[Pusher] exit sender_loop");
}
