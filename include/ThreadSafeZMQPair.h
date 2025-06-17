#pragma once
#include <zmq.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <variant>
#include <functional>
#include <atomic>

class ThreadSafeZMQPair {
public:
    using MessageCallback = std::function<void(const std::vector<uint8_t>&)>;

    ThreadSafeZMQPair(zmq::context_t& context, const std::string& address, bool isBind);
    ~ThreadSafeZMQPair();

    void send_async(const std::vector<uint8_t>& data);

    void set_callback(MessageCallback callback);

private:
    void io_loop();

    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::string address_;
    bool isBind_;
    std::atomic<bool> running_;

    std::queue<std::vector<uint8_t>> send_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;

    std::thread io_thread_;

    MessageCallback message_callback_;
};
