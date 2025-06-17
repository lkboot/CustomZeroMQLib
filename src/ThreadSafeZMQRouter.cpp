#include "ThreadSafeZMQRouter.h"
#include "LoggerManager.h"
#include "HexUtils.h"

ThreadSafeZMQRouter::ThreadSafeZMQRouter(zmq::context_t& context, const std::string& address)
    : context_(context), address_(address), running_(true) {
    socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_ROUTER);
    socket_->bind(address_);
    spdlog::info("[Router] Bound to {}", address_);

    router_thread_ = std::thread(&ThreadSafeZMQRouter::router_loop, this);
}

ThreadSafeZMQRouter::~ThreadSafeZMQRouter() {
    running_ = false;

    if (router_thread_.joinable())
        router_thread_.join();

    socket_->close();
    spdlog::info("[Router] Socket closed");
}

void ThreadSafeZMQRouter::set_callback(MessageCallback cb) {
    message_callback_ = std::move(cb);
}

void ThreadSafeZMQRouter::send_to(const std::vector<uint8_t>& identity, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    zmq::message_t id_msg(identity.data(), identity.size());
    zmq::message_t empty_msg(0);
    zmq::message_t data_msg(data.data(), data.size());

    auto r1 = socket_->send(id_msg, zmq::send_flags::sndmore);
    auto r2 = socket_->send(empty_msg, zmq::send_flags::sndmore);
    auto r3 = socket_->send(data_msg, zmq::send_flags::none);

    if (!r1.has_value() || !r2.has_value() || !r3.has_value()) {
        spdlog::error("[Router] Failed to send message to {}", HexUtils::BytesToHex(identity));
    }
}

void ThreadSafeZMQRouter::router_loop() {
    zmq::pollitem_t items[] = {
        { static_cast<void*>(*socket_), 0, ZMQ_POLLIN, 0 }
    };

    while (running_) {
        zmq::poll(items, 1, std::chrono::milliseconds(200));

        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t identity;
            zmq::message_t content;

            if (socket_->recv(identity, zmq::recv_flags::none) &&
                socket_->recv(content, zmq::recv_flags::none)) {

                std::vector<uint8_t> id_vec(static_cast<uint8_t*>(identity.data()), static_cast<uint8_t*>(identity.data()) + identity.size());
                std::vector<uint8_t> data(static_cast<uint8_t*>(content.data()), static_cast<uint8_t*>(content.data()) + content.size());

                spdlog::info("[Router] Received from id: {}, size: {}", HexUtils::BytesToHex(id_vec), data.size());

                if (message_callback_) {
                    message_callback_(id_vec, data);
                }
            }
        }
    }

    spdlog::debug("[Router] Router_loop exited");
}