#include "ThreadSafeZMQRequester.h"
#include <iostream>
#include "LoggerManager.h"

ThreadSafeZMQRequester::ThreadSafeZMQRequester(zmq::context_t& context, const std::string& address)
    : context_(context), running_(true), address_(address)
{
    socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_REQ);
    socket_->connect(address_);
    socket_->set(zmq::sockopt::req_relaxed, 1);
    spdlog::info("[Requester] Connected to {}", address_);
    //socket_->set(zmq::sockopt::rcvtimeo, 3000);  // 设置合理超时，避免死锁

    requester_thread_ = std::thread(&ThreadSafeZMQRequester::requester_loop, this);
}

ThreadSafeZMQRequester::~ThreadSafeZMQRequester()
{
    spdlog::debug("[Requester] Destruct called");

    running_ = false;
    cv_.notify_all();
    if (requester_thread_.joinable())
        requester_thread_.join();

    if (socket_) {
        socket_->close();
        spdlog::info("[Requester] Socket closed");
    }
}

void ThreadSafeZMQRequester::send_request_async(const std::vector<uint8_t>& data, MessageCallback cb)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    request_queue_.push({ data, std::move(cb) });
    cv_.notify_one();
}

void ThreadSafeZMQRequester::set_timeout_callback(std::function<void()> callback) {
    timeout_callback_ = std::move(callback);
}

void ThreadSafeZMQRequester::requester_loop()
{
    constexpr int max_retries = 15; // 最多重试次数
    constexpr auto poll_timeout = std::chrono::milliseconds(200);

    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_.wait(lock, [this]() {
            return !request_queue_.empty() || !running_;
            });

        if (!running_)
            break;

        auto req = std::move(request_queue_.front());
        request_queue_.pop();
        lock.unlock();
        int retry_count = 0;
        bool success = false;
        std::vector<uint8_t> reply_data;

        while (retry_count < max_retries && running_) {
            // 发送请求
            zmq::message_t msg(req.content.data(), req.content.size());
            auto res = socket_->send(msg, zmq::send_flags::none);
            if (!res.has_value()) {
                spdlog::warn("[Requester] Send failed on retry {}", retry_count);
                ++retry_count;
                
                continue;
            }

            spdlog::info("[Requester] Sent data size: {}, retry {}", req.content.size(), retry_count);
            
            // 等待响应
            zmq::pollitem_t items[] = {
                { static_cast<void*>(*socket_), 0, ZMQ_POLLIN, 0 }
            };
            zmq::poll(items, 1, poll_timeout);
            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t reply_msg;
                if (socket_->recv(reply_msg, zmq::recv_flags::none)) {
                    reply_data.assign(
                        static_cast<uint8_t*>(reply_msg.data()),
                        static_cast<uint8_t*>(reply_msg.data()) + reply_msg.size()
                    );
                    spdlog::info("[Requester] Received response size: {}", reply_msg.size());
                    success = true;
                    
                    break;
                }
                else {
                    spdlog::warn("[Requester] recv failed after poll");
                }
            }
            else {
                spdlog::warn("[Requester] Poll timeout on retry {}", retry_count);
            }

            ++retry_count;
        }

        // 调用回调，无论成功与否
        if (req.callback) {
            if (success) {
                req.callback(reply_data);
            }
            else {
                spdlog::warn("[Requester] Max retries reached, sending timeout callback");
                if (timeout_callback_)
                    timeout_callback_();
            }
        }
    }

    spdlog::debug("[Requester] Requester_loop exited");
}
