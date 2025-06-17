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

    // ���ý��յ���Ϣʱ�Ļص�����
    void set_callback(MessageCallback callback);

private:
    void puller_loop(); // ��̨�̺߳���

    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::thread receiver_thread_;
    std::atomic<bool> running_;
    MessageCallback message_callback_;

    std::string address_;
    bool isBind_;
};
