#include "ZMQSocketManager.h"
#include "LoggerManager.h"
#include "PacketBuilder.h"
#include "HexUtils.h"

ZMQSocketManager::ZMQSocketManager(ZMQMode mode, const std::string& sendAddress, const std::string& recvAddress, const std::string& topicFilter)
    : mode_(mode)
{
    LoggerManager::Init();
    spdlog::info("[ZMQSocketManager] Construct with mode: {}, send: {}, recv: {}", static_cast<int>(mode), sendAddress, recvAddress);

    // 使用共享上下文
    zmq::context_t& context = get_shared_context();

    switch (mode) {
    case ZMQMode::Pair:
        if (!sendAddress.empty()) {
            // bind
            pair_endpoint_ = std::make_unique<ThreadSafeZMQPair>(context, sendAddress, true);
        }
        else if (!recvAddress.empty()) {
            // connect
            pair_endpoint_ = std::make_unique<ThreadSafeZMQPair>(context, recvAddress, false);
        }
        break;

    case ZMQMode::PubSub:
        if (!sendAddress.empty()) {
            publisher_ = std::make_unique<ThreadSafeZMQPublisher>(context, sendAddress, true);
        }
        if (!recvAddress.empty()) {
            subscriber_ = std::make_unique<ThreadSafeZMQSubscriber>(context, recvAddress, topicFilter, false);
        }
        break;

    case ZMQMode::ReqRep:
        if (!sendAddress.empty()) {
            requester_ = std::make_unique<ThreadSafeZMQRequester>(context, sendAddress);
        }
        if (!recvAddress.empty()) {
            replier_ = std::make_unique<ThreadSafeZMQReplier>(context, recvAddress);
        }
        break;

    case ZMQMode::PushPull:
        if (!sendAddress.empty()) {
            pusher_ = std::make_unique<ThreadSafeZMQPusher>(context, sendAddress, true);
        }
        if (!recvAddress.empty()) {
            puller_ = std::make_unique<ThreadSafeZMQPuller>(context, recvAddress, false);
        }
        break;

    case ZMQMode::DealerRouter:
        if (!sendAddress.empty()) {
            dealer_ = std::make_unique<ThreadSafeZMQDealer>(context, sendAddress);
        }
        if (!recvAddress.empty()) {
            router_ = std::make_unique<ThreadSafeZMQRouter>(context, recvAddress);
        }
        break;

    default:
        spdlog::error("[ZMQSocketManager] Unsupported ZMQ mode: {}", static_cast<int>(mode));
    }
}

ZMQSocketManager::~ZMQSocketManager() {
    spdlog::info("[ZMQSocketManager] Destruct called");
    shutdown();
}

void ZMQSocketManager::send_async(const std::vector<uint8_t>& data) {
    if (mode_ == ZMQMode::Pair && pair_endpoint_) {
        pair_endpoint_->send_async(data);
    }
    else if (mode_ == ZMQMode::ReqRep && requester_) {
        requester_->send_request_async(data, [this](const std::vector<uint8_t>& response) {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            if (response_callback_) {
                response_callback_(response);
            }
        });
    }
    else if (mode_ == ZMQMode::PushPull && pusher_) {
        pusher_->send_async(data);
    }
    else if (mode_ == ZMQMode::DealerRouter && dealer_) {
        dealer_->send_async(data);
    }
}

void ZMQSocketManager::send_sub_async(const std::vector<uint8_t>& data, const std::string& topic) {
    if (mode_ == ZMQMode::PubSub && publisher_) {
        publisher_->publish_async(topic, data);
    }
}

void ZMQSocketManager::set_callback(std::function<void(const std::vector<uint8_t>&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (mode_ == ZMQMode::Pair && pair_endpoint_) {
        pair_endpoint_->set_callback(std::move(callback));
    }
    else if (mode_ == ZMQMode::ReqRep && requester_) {
        response_callback_ = std::move(callback);
    }
    else if (mode_ == ZMQMode::ReqRep && replier_) {
        replier_->set_callback(std::move(callback));
    }
    else if (mode_ == ZMQMode::PushPull && puller_) {
        puller_->set_callback(std::move(callback));
    }
    else if (mode_ == ZMQMode::DealerRouter && dealer_) {
        dealer_->set_callback(std::move(callback));
    }
}

void ZMQSocketManager::set_sub_callback(std::function<void(const std::string& topic, const std::vector<uint8_t>& data)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (mode_ == ZMQMode::PubSub && subscriber_) {
        subscriber_->set_callback(std::move(callback));
    }
}

void ZMQSocketManager::set_router_callback(std::function<void(const std::vector<uint8_t>& id, const std::vector<uint8_t>& data)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (mode_ == ZMQMode::DealerRouter && router_) {
        router_->set_callback(std::move(callback));
    }
}

void ZMQSocketManager::send_replier_reply(const std::vector<uint8_t>& data)
{
    if (mode_ == ZMQMode::ReqRep && replier_) {
        replier_->send_reply(data);
    }
}

void ZMQSocketManager::send_router_reply(const std::vector<uint8_t>& id, const std::vector<uint8_t>& data) {
    if (mode_ == ZMQMode::DealerRouter && router_) {
        router_->send_to(id, data);
    }
}

void ZMQSocketManager::set_timeout_callback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    timeout_callback_ = std::move(callback);

    if (requester_)
    {
        requester_->set_timeout_callback(timeout_callback_);
    }
    else if (dealer_) {
        dealer_->set_timeout_callback(timeout_callback_);
    }

    // 可根据需要扩展其他模式支持
}

void ZMQSocketManager::shutdown() {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (pair_endpoint_) {
        spdlog::debug("[ZMQSocketManager] Releasing pair");
        pair_endpoint_.reset();
    }
    if (requester_) {
        spdlog::debug("[ZMQSocketManager] Releasing requester");
        requester_.reset();
    }
    if (replier_) {
        spdlog::debug("[ZMQSocketManager] Releasing replier");
        replier_.reset();
    }
    if (subscriber_) {
        spdlog::debug("[ZMQSocketManager] Releasing subscriber");
        subscriber_.reset();
    }
    if (publisher_) {
        spdlog::debug("[ZMQSocketManager] Releasing publisher");
        publisher_.reset();
    }
    if (puller_) {
        spdlog::debug("[ZMQSocketManager] Releasing receiver");
        puller_.reset();
    }
    if (pusher_) {
        spdlog::debug("[ZMQSocketManager] Releasing sender");
        pusher_.reset();
    }
    if (dealer_) {
        spdlog::debug("[ZMQSocketManager] Releasing dealer");
        dealer_.reset();
    }
    if (router_) {
        spdlog::debug("[ZMQSocketManager] Releasing router");
        router_.reset();
    }

    spdlog::debug("[ZMQSocketManager] Shutdown finish");
}
