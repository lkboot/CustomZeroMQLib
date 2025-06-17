#pragma once

#include <zmq.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <vector>
#include <string>

class ThreadSafeZMQDealer {
public:
    using MessageCallback = std::function<void(const std::vector<uint8_t>&)>;

    ThreadSafeZMQDealer(zmq::context_t& context, const std::string& address);
    ~ThreadSafeZMQDealer();

    void send_async(const std::vector<uint8_t>& data);
    void set_callback(MessageCallback cb);
    void set_timeout_callback(std::function<void()> callback);

private:
    std::function<void()> timeout_callback_;

    void dealer_loop();

    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::string address_;
    std::atomic<bool> running_;
    std::thread dealer_thread_;

    std::mutex send_mutex_;
    std::queue<std::vector<uint8_t>> send_queue_;
    std::condition_variable cv_;

    MessageCallback message_callback_;
};
