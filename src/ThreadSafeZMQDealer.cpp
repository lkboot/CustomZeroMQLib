#include "ThreadSafeZMQDealer.h"
#include "LoggerManager.h"

ThreadSafeZMQDealer::ThreadSafeZMQDealer(zmq::context_t& context, const std::string& address)
    : context_(context), address_(address), running_(true) {
    socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_DEALER);
    socket_->connect(address_);
    int timeout_ms = 2000; // 2秒
    socket_->set(zmq::sockopt::sndtimeo, timeout_ms);
    socket_->set(zmq::sockopt::rcvtimeo, timeout_ms);

    spdlog::info("[Dealer] Connected to {}", address_);

    dealer_thread_ = std::thread(&ThreadSafeZMQDealer::dealer_loop, this);
}

ThreadSafeZMQDealer::~ThreadSafeZMQDealer() {
    running_ = false;
    cv_.notify_all();

    if (dealer_thread_.joinable())
        dealer_thread_.join();

    socket_->close();
    spdlog::info("[Dealer] Socket closed");
}

void ThreadSafeZMQDealer::send_async(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    if (send_queue_.size() > 1000) {
        spdlog::warn("[Dealer] Send queue size too large: {}", send_queue_.size());
    }
    send_queue_.push(data);
    cv_.notify_one();
}

void ThreadSafeZMQDealer::set_callback(MessageCallback cb) {
    message_callback_ = std::move(cb);
}

void ThreadSafeZMQDealer::set_timeout_callback(std::function<void()> callback) {
    timeout_callback_ = std::move(callback);
}

void ThreadSafeZMQDealer::dealer_loop() {
    zmq::pollitem_t items[] = {
        { static_cast<void*>(*socket_), 0, ZMQ_POLLIN, 0 }
    };

    while (running_) {
        // 发送数据
        {
            std::queue<std::vector<uint8_t>> local_queue;
            {
                std::unique_lock<std::mutex> lock(send_mutex_);
                cv_.wait_for(lock, std::chrono::milliseconds(200), [this] {
                    return !send_queue_.empty();
                    });
                std::swap(send_queue_, local_queue);
            }
            // 释放锁后，逐条发送 local_queue 中的消息
            while (!local_queue.empty()) {
                auto& data = local_queue.front();
                zmq::message_t msg(data.data(), data.size());
                auto result = socket_->send(msg, zmq::send_flags::none);
                if (!result.has_value()) {
                    spdlog::error("[Dealer] Failed to send message");
                }
                else {
                    spdlog::debug("[Dealer] Sent message size: {}", result.value());
                }
                // 处理 result
                local_queue.pop();
            }
        }

        // 接收数据
        zmq::poll(items, 1, std::chrono::milliseconds(2000));
        if (items[0].revents & ZMQ_POLLIN) {
            std::vector<uint8_t> complete_data;

            while (true) {
                zmq::message_t msg;
                auto result = socket_->recv(msg, zmq::recv_flags::none);
                if (!result.has_value()) {
                    spdlog::warn("[Dealer] recv returned no message or was interrupted");
                    break;
                }

                complete_data.insert(complete_data.end(),
                    static_cast<uint8_t*>(msg.data()),
                    static_cast<uint8_t*>(msg.data()) + msg.size());

                bool more = socket_->get(zmq::sockopt::rcvmore);
                if (!more) 
                    break;
            }


            spdlog::debug("[Dealer] Received message size: {}", complete_data.size());
            if (message_callback_) 
                message_callback_(complete_data);
        } else {
            // 超时，没有消息到达
            if (timeout_callback_) 
                timeout_callback_();
        }
    }

    spdlog::debug("[Dealer] Dealer_loop exited");
}
