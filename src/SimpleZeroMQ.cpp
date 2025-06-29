// SimpleZeroMQ.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>
#include <string>
#include <future>  // for std::promise and std::future

#include "ZMQSocketManager.h"
#include "LoggerManager.h"

// 将字节序列转为十六进制字符串（不带空格）
inline std::string to_hex_string(const std::vector<uint8_t>& data, bool with_space = false) {
    std::ostringstream oss;
    for (uint8_t byte : data) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        if (with_space) oss << ' ';
    }
    return oss.str();
}

// 用于等待回调完成
std::promise<void> done_promise;
std::future<void> done_future = done_promise.get_future();

void run_pair1() {
    ZMQSocketManager pair_a(ZMQMode::Pair, "tcp://*:8000", "");

    pair_a.set_callback([&pair_a](const std::vector<uint8_t>& msg) {
        std::string content(msg.begin(), msg.end());
        std::cout << "[Pair A] Received: " << content << std::endl;
        });

    for (int i = 1; i <= 3; ++i) {
        std::string msg = "Ping from A " + std::to_string(i);
        pair_a.send_async(std::vector<uint8_t>(msg.begin(), msg.end()));
        std::cout << "[Pair A] Sent: " << msg << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待回应
}

void run_pair2() {
    ZMQSocketManager pair_b(ZMQMode::Pair, "", "tcp://localhost:8000");

    pair_b.set_callback([&pair_b](const std::vector<uint8_t>& msg) {
        std::string content(msg.begin(), msg.end());
        std::cout << "[Pair B] Received: " << content << std::endl;
        });

    for (int i = 1; i <= 3; ++i) {
        std::string msg = "Ping from B " + std::to_string(i);
        pair_b.send_async(std::vector<uint8_t>(msg.begin(), msg.end()));
        std::cout << "[Pair B] Sent: " << msg << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待回应
}

void run_server() {
    ZMQSocketManager server(ZMQMode::ReqRep, "", "tcp://*:5555");

    server.set_callback([&server](const std::vector<uint8_t>& request) {
        std::string received(request.begin(), request.end());
        std::cout << "[Server] Received: " << received << std::endl;

        std::string response = "Ack " + received;
        server.send_replier_reply(std::vector<uint8_t>(response.begin(), response.end()));
        });

    std::cout << "Server started. Waiting for requests..." << std::endl;
    std::this_thread::sleep_for(std::chrono::minutes(1));
}

void run_client() {
    ZMQSocketManager client(ZMQMode::ReqRep, "tcp://localhost:5555", "");

    client.set_callback([](const std::vector<uint8_t>& response) {
        std::string reply(response.begin(), response.end());
        std::cout << "[Client] Received reply: " << reply << std::endl;
        done_promise.set_value();  // 通知主线程“回调完成”
        });

    client.set_timeout_callback([] {
        spdlog::warn("Recv timeout");
        });

    std::string msg = "Hello Server";
    client.send_async(std::vector<uint8_t>(msg.begin(), msg.end()));

    // 主线程等待回调执行完成
    if (done_future.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        std::cout << "[Client] Timeout waiting for reply." << std::endl;
    }
}

void run_publisher() {
    ZMQSocketManager pub(ZMQMode::PubSub, "tcp://*:6000", "");
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待连接完成，否则会丢包

    int count = 0;
    while (count++ < 10) {
        std::string msg = "topic1: Hello " + std::to_string(count);
        pub.send_sub_async(std::vector<uint8_t>(msg.begin(), msg.end()), "topic1");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void run_subscriber() {
    ZMQSocketManager sub(ZMQMode::PubSub, "", "tcp://localhost:6000", "topic1");

    sub.set_sub_callback([](const std::string& topic, const std::vector<uint8_t>& msg) {
        std::string content(msg.begin(), msg.end());
        std::cout << "[Subscriber] Topic: " << topic << ", Message: " << content << std::endl;
        });

    std::this_thread::sleep_for(std::chrono::seconds(30));
}

void run_push() {
    ZMQSocketManager pusher(ZMQMode::PushPull, "tcp://*:7000", "");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (int i = 1; i <= 10; ++i) {
        std::string msg = "Pushed: " + std::to_string(i);
        pusher.send_async(std::vector<uint8_t>(msg.begin(), msg.end()));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void run_pull() {
    ZMQSocketManager puller(ZMQMode::PushPull, "", "tcp://localhost:7000");
    puller.set_callback([](const std::vector<uint8_t>& msg) {
        std::string content(msg.begin(), msg.end());
        std::cout << "[Puller] Received: " << content << std::endl;
        });

    std::this_thread::sleep_for(std::chrono::seconds(30));
}

void run_router() {
    ZMQSocketManager router(ZMQMode::DealerRouter, "", "tcp://*:9000");

    router.set_router_callback([&router](const std::vector<uint8_t>& id, const std::vector<uint8_t>& msg) {
        std::string client_id = to_hex_string(id);
        std::string content(msg.begin(), msg.end());
        std::cout << "[Router] Received from [" << client_id << "]: " << content << std::endl;

        std::string reply = "Ack from Router to: " + content;
        router.send_router_reply(id, std::vector<uint8_t>(reply.begin(), reply.end()));
        });

    std::cout << "[Router] Listening on tcp://*:9000..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(30));
}

void run_dealer() {
    ZMQSocketManager dealer(ZMQMode::DealerRouter, "tcp://localhost:9000", "");

    dealer.set_callback([&dealer](const std::vector<uint8_t>& msg) {
        std::string content(msg.begin(), msg.end());
        std::cout << "[Dealer] Received reply: " << content << std::endl;
        });

    dealer.set_timeout_callback([] {
        spdlog::warn("Recv timeout");
        });

    for (int i = 1; i <= 3; ++i) {
        std::string message = "Ping " + std::to_string(i);
        dealer.send_async(std::vector<uint8_t>(message.begin(), message.end()));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::this_thread::sleep_for(std::chrono::seconds(10)); // 等待服务器响应
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "Usage: program.exe [server|client]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    if (mode == "pair1") {
        run_pair1();
    }
    else if (mode == "pair2") {
        run_pair2();
    }
    else if (mode == "server") {
        run_server();
    }
    else if (mode == "client") {
        run_client();
    }
    else if (mode == "dealer") {
        run_dealer();
    }
    else if (mode == "router") {
        run_router();
    }
    else if (mode == "pub") {
        run_publisher();
    }
    else if (mode == "sub") {
        run_subscriber();
    }
    else if (mode == "push") {
        run_push();
    }
    else if (mode == "pull") {
        run_pull();
    }
    else if (mode == "test") {
        
    }
    else {
        std::cout << "Unknown mode: " << mode << std::endl;
        std::cout << "Usage: program.exe [server|client|dealer|router|pub|sub|test]" << std::endl;
        return 1;
    }

    spdlog::info("main exit");

    return 0;
}
