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

class ThreadSafeZMQPublisher
{
public:
    ThreadSafeZMQPublisher(zmq::context_t& context, const std::string& address, bool isBind = true);
    ~ThreadSafeZMQPublisher();

    void publish_async(const std::string& topic, const std::vector<uint8_t>& data);

private:
    void publisher_loop();

    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
    bool running_;
    std::string address_;
    bool isBind_;

    struct OutgoingMessage {
        std::string topic;
        std::vector<uint8_t> content;
    };

    std::queue<OutgoingMessage> send_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::thread publisher_thread_;
};

