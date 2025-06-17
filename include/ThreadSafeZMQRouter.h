#pragma once

#include <zmq.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <queue>
#include <vector>

class ThreadSafeZMQRouter {
public:
    using MessageCallback = std::function<void(const std::vector<uint8_t>& id, const std::vector<uint8_t>& data)>;

    ThreadSafeZMQRouter(zmq::context_t& context, const std::string& address);
    ~ThreadSafeZMQRouter();

    void set_callback(MessageCallback cb);

    void send_to(const std::vector<uint8_t>& identity, const std::vector<uint8_t>& data);

private:
    void router_loop();

    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::string address_;
    std::atomic<bool> running_;
    std::thread router_thread_;
    std::mutex send_mutex_;

    MessageCallback message_callback_;
};
