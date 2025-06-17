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

class ThreadSafeZMQRequester
{
public:
    using MessageCallback = std::function<void(const std::vector<uint8_t>&)>;

    ThreadSafeZMQRequester(zmq::context_t& context, const std::string& address);
    ~ThreadSafeZMQRequester();

    // 发送请求，异步，响应通过回调返回
    void send_request_async(const std::vector<uint8_t>& data, MessageCallback cb);

    void set_timeout_callback(std::function<void()> callback);

private:
    std::function<void()> timeout_callback_;

    void requester_loop();

    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
    bool running_;
    std::string address_;

    struct OutgoingRequest {
        std::vector<uint8_t> content;
        MessageCallback callback;
    };

    std::queue<OutgoingRequest> request_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::thread requester_thread_;
};

