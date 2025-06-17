#pragma once

#include <zmq.hpp>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <vector>
#include <variant>
#include <functional>

class ThreadSafeZMQSubscriber
{
public:
    using MessageCallback = std::function<void(const std::string& topic, const std::vector<uint8_t>& data)>;

    ThreadSafeZMQSubscriber(zmq::context_t& context, const std::string& address, const std::string& topicFilter, bool isBind = false);
    ~ThreadSafeZMQSubscriber();

    void set_callback(MessageCallback cb);

private:
    void subscriber_loop();

    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
    bool running_;
    std::string address_;
    std::string topic_filter_;
    bool isBind_;

    MessageCallback message_callback_;
    std::thread subscriber_thread_;
};

