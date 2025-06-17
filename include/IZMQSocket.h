#pragma once
#include <string>
#include <vector>
#include <functional>

class IZMQSocket
{
public:
    virtual ~IZMQSocket() = default;

    // 发送字符串
    virtual void send(const std::string& message) = 0;

    // 发送二进制数据
    virtual void send(const std::vector<uint8_t>& data) = 0;

    // 设置字符串消息回调（仅适用于有接收功能的socket）
    virtual void set_string_callback(std::function<void(const std::string&)> callback) = 0;

    // 设置二进制消息回调
    virtual void set_callback(std::function<void(const std::vector<uint8_t>&)> callback) = 0;

    // 关闭资源
    virtual void shutdown() = 0;
};

