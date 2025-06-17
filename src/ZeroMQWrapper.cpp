#include <iostream>
#include <string>
#include <memory>
#include <mutex>

#include "ZeroMQWrapper.h"

extern "C" {
    
    ZMQSocketManager* __stdcall CreateChannel(ZMQMode mode, const char* send, const char* recv, const char* topic) {
        return new ZMQSocketManager(mode, send, recv, topic);
    }

    void __stdcall Send(ZMQSocketManager* channel, const uint8_t* data, int length) {
        if (channel && data && length > 0) {
            std::vector<uint8_t> vec(data, data + length);
            channel->send_async(vec);
        }
    }

    void __stdcall RegisterCallback(ZMQSocketManager* channel, MessageCallbackFunction callback) {
        if (channel && callback) {
            channel->set_callback([=](const std::vector<uint8_t>& data) {
                callback(data.data(), static_cast<int>(data.size()));
                });
        }
    }

    void __stdcall SendWithTopic(ZMQSocketManager* channel, const uint8_t* data, int length, const char* topic) {
        if (channel && data && length > 0) {
            std::vector<uint8_t> vec(data, data + length);
            channel->send_sub_async(vec, topic);
        }
    }

    void __stdcall RegisterSubCallback(ZMQSocketManager* channel, SubMessageCallbackFunction callback) {
        if (channel && callback) {
            channel->set_sub_callback([=](const std::string& topic, const std::vector<uint8_t>& data) {
                callback(topic.c_str(), data.data(), static_cast<int>(data.size()));
                });
        }
    }

    void __stdcall SendReplierReply(ZMQSocketManager* channel, const uint8_t* data, int length) {
        if (channel && data && length > 0) {
            std::vector<uint8_t> vec(data, data + length);
            channel->send_replier_reply(vec);
        }
    }

    void __stdcall RegisterRouterCallback(ZMQSocketManager* channel, RouterMessageCallbackFunction callback) {
        if (channel && callback) {
            channel->set_router_callback([=](const std::vector<uint8_t>& identity, const std::vector<uint8_t>& data) {
                callback(identity.data(), static_cast<int>(identity.size()), data.data(), static_cast<int>(data.size()));
                });
        }
    }

    void __stdcall SendRouterReply(ZMQSocketManager* channel, const uint8_t* identity, int id_len, const uint8_t* data, int data_len)
    {
        if (!channel || !identity || id_len <= 0 || !data || data_len <= 0) {
            return;
        }

        std::vector<uint8_t> id_vec(identity, identity + id_len);
        std::vector<uint8_t> data_vec(data, data + data_len);

        channel->send_router_reply(id_vec, data_vec);
    }

    void __stdcall DestroyChannel(ZMQSocketManager* channel) {
        if (channel) {
            delete channel;
            channel = nullptr;
        }
    }

}
