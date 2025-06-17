#pragma once
#include "ZMQSocketManager.h"

#ifdef ZEROMQ_EXPORTS
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif

extern "C" {
	// 回调函数类型（供 C# 注册）
	typedef void(__stdcall* MessageCallbackFunction)(const uint8_t* data, int length);
	typedef void(__stdcall* SubMessageCallbackFunction)(const char* topic, const uint8_t* data, int length);
	typedef void(__stdcall* RouterMessageCallbackFunction)(const uint8_t* identity, int id_len, const uint8_t* data, int data_len);

	API ZMQSocketManager* __stdcall CreateChannel(ZMQMode mode, const char* send, const char* recv, const char* topic);
	API void __stdcall Send(ZMQSocketManager* channel, const uint8_t* data, int length);
	API void __stdcall RegisterCallback(ZMQSocketManager* channel, MessageCallbackFunction callback);
	API void __stdcall SendWithTopic(ZMQSocketManager* channel, const uint8_t* data, int length, const char* topic);
	API void __stdcall RegisterSubCallback(ZMQSocketManager* channel, SubMessageCallbackFunction callback);
	API void __stdcall SendReplierReply(ZMQSocketManager* channel, const uint8_t* data, int length);
	API void __stdcall RegisterRouterCallback(ZMQSocketManager* channel, RouterMessageCallbackFunction callback);
	API void __stdcall SendRouterReply(ZMQSocketManager* channel, const uint8_t* identity, int id_len, const uint8_t* data, int data_len);
	API void __stdcall DestroyChannel(ZMQSocketManager* channel);
}
