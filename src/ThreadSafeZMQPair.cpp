#include "ThreadSafeZMQPair.h"
#include "LoggerManager.h"
#include <iostream>

ThreadSafeZMQPair::ThreadSafeZMQPair(zmq::context_t& context, const std::string& address, bool isBind)
    : context_(context), running_(true), address_(address), isBind_(isBind)
{
    socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PAIR);
    if (isBind) {
        socket_->bind(address);
        spdlog::info("[PAIR] Bound to: {}", address);
    }
    else {
        socket_->connect(address);
        spdlog::info("[PAIR] Connected to: {}", address);
    }

    io_thread_ = std::thread(&ThreadSafeZMQPair::io_loop, this);
}

ThreadSafeZMQPair::~ThreadSafeZMQPair()
{
    running_ = false;
    cv_.notify_all();

    if (io_thread_.joinable())
        io_thread_.join();

    if (socket_) {
        socket_->close();
        spdlog::info("[PAIR] Socket closed to {}", address_);
    }
}

void ThreadSafeZMQPair::send_async(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    send_queue_.emplace(data);
    cv_.notify_one();
}

void ThreadSafeZMQPair::set_callback(MessageCallback callback)
{
    message_callback_ = std::move(callback);
}

void ThreadSafeZMQPair::io_loop()
{
    zmq::pollitem_t items[] = {
        { static_cast<void*>(*socket_), 0, ZMQ_POLLIN | ZMQ_POLLOUT, 0 }
    };

    while (running_) {
        zmq::poll(items, 1, std::chrono::milliseconds(200));

        // === 1. Receive if data available
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t body;
            if (socket_->recv(body, zmq::recv_flags::none)) {
                if (message_callback_) {
                    std::vector<uint8_t> data(static_cast<uint8_t*>(body.data()),
                        static_cast<uint8_t*>(body.data()) + body.size());
                    message_callback_(data);
                    spdlog::info("[PAIR] Received binary size: {}", body.size());
                }
            }
        }

        // === 2. Send if socket is writable
        if (items[0].revents & ZMQ_POLLOUT) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            int send_count = 0;
            const int max_batch = 10;  // 防止无限发送占用 CPU

            while (!send_queue_.empty() && send_count++ < max_batch) {
                std::vector<uint8_t> data = std::move(send_queue_.front());
                send_queue_.pop();
                lock.unlock();

                zmq::message_t body(data.data(), data.size());
                auto result = socket_->send(body, zmq::send_flags::dontwait);
                if (!result.has_value()) {
                    spdlog::warn("[PAIR] Send failed.");
                }

                lock.lock();
            }
        }
    }

    spdlog::debug("[PAIR] Exit io_loop");
}
