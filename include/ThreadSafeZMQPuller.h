#pragma once

#include <zmq.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <functional>

class ThreadSafeZMQPuller {
public:
    using MessageCallback = std::function<void(const std::vector<uint8_t>&)>;

    ThreadSafeZMQPuller(zmq::context_t& context, const std::string& address, bool isBind);
    ~ThreadSafeZMQPuller();

    // 设置接收到消息时的回调函数
    void set_callback(MessageCallback callback);

private:
    void puller_loop(); // 后台线程函数

    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::thread receiver_thread_;
    std::atomic<bool> running_;
    MessageCallback message_callback_;

    std::string address_;
    bool isBind_;
};
