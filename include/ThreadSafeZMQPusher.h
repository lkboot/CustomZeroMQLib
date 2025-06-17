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

class ThreadSafeZMQPusher {
public:
    ThreadSafeZMQPusher(zmq::context_t& context, const std::string& address, bool isBind);
    ~ThreadSafeZMQPusher();

    // �첽������Ϣ���̰߳�ȫ��
    void send_async(const std::vector<uint8_t>& data);

private:
    void pusher_loop(); // ��̨�̺߳���

    zmq::context_t& context_;
    std::unique_ptr<zmq::socket_t> socket_;

    std::queue<std::vector<uint8_t>> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_;
    std::thread sender_thread_;
    
    std::string address_;
    bool isBind_;
};
