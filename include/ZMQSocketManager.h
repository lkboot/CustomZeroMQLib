#pragma once

#include <string>
#include <memory>
#include <functional>
#include <mutex>

#include "ThreadSafeZMQPair.h"
#include "ThreadSafeZMQPublisher.h"
#include "ThreadSafeZMQSubscriber.h"
#include "ThreadSafeZMQReplier.h"
#include "ThreadSafeZMQRequester.h"
#include "ThreadSafeZMQPusher.h"
#include "ThreadSafeZMQPuller.h"
#include "ThreadSafeZMQDealer.h"
#include "ThreadSafeZMQRouter.h"

enum class ZMQMode {
    Pair = 0,
    PubSub,
    ReqRep,
    PushPull,
    DealerRouter
};

class ZMQSocketManager {
public:
    // 构造函数，初始化发送和接收地址
    ZMQSocketManager(ZMQMode mode, const std::string& sendAddress = "", const std::string& recvAddress = "", const std::string& topicFilter = "");

    // 析构函数，自动清理资源
    ~ZMQSocketManager();

    // 异步发送消息
    void send_async(const std::vector<uint8_t>& data);
    void send_sub_async(const std::vector<uint8_t>& data, const std::string& topic = "");

    // 设置接收回调函数
    void set_callback(std::function<void(const std::vector<uint8_t>&)> callback);
    void set_sub_callback(std::function<void(const std::string& topic, const std::vector<uint8_t>& data)> callback);
    void set_router_callback(std::function<void(const std::vector<uint8_t>& id, const std::vector<uint8_t>& data)> callback);

    void send_replier_reply(const std::vector<uint8_t>& data);
    void send_router_reply(const std::vector<uint8_t>& id, const std::vector<uint8_t>& data);

    // 设置超时回调（适用于 Dealer 等异步接收类型）
    void set_timeout_callback(std::function<void()> callback);

    // 主动关闭通道
    void shutdown();

private:
    static zmq::context_t& get_shared_context() {
        static zmq::context_t context(1); // 线程安全的局部静态变量
        return context;
    }

    std::function<void(const std::vector<uint8_t>&)> response_callback_;

    std::function<void()> timeout_callback_;

    std::unique_ptr<ThreadSafeZMQPair> pair_endpoint_;

    std::unique_ptr<ThreadSafeZMQPublisher> publisher_;
    std::unique_ptr<ThreadSafeZMQSubscriber> subscriber_;

    std::unique_ptr<ThreadSafeZMQRequester> requester_;
    std::unique_ptr<ThreadSafeZMQReplier> replier_;

    std::unique_ptr<ThreadSafeZMQPusher> pusher_;
    std::unique_ptr<ThreadSafeZMQPuller> puller_;

    std::unique_ptr<ThreadSafeZMQDealer> dealer_;
    std::unique_ptr<ThreadSafeZMQRouter> router_;

    std::mutex callback_mutex_;  // 保护回调设置的互斥锁

    ZMQMode mode_;
};
