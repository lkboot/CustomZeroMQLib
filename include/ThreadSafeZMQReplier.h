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
#include <msgpack.hpp>

#include "MessagePackData.h"

class ThreadSafeZMQReplier
{
public:
    using MessageCallback = std::function<void(const std::vector<uint8_t>&)>;

    ThreadSafeZMQReplier(zmq::context_t& context, const std::string& address);
    ~ThreadSafeZMQReplier();

    void set_callback(MessageCallback cb);

    void send_reply(const std::vector<uint8_t>& reply);

private:
    void replier_loop();

    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::string address_;
    std::atomic<bool> running_;
    std::thread replier_thread_;
    std::mutex send_mutex_;

    MessageCallback message_callback_;
};

