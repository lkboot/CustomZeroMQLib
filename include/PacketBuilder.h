#pragma once
#include <cstdint>
#include <vector>

enum class PayloadType : uint8_t {
    RawBytes = 0x01,
    Utf8Text = 0x02,
    JsonUtf8 = 0x03,
    MessagePack = 0x04
};

class PacketBuilder
{
public:
    // 构造封包：类型 + 4字节长度（小端）+ 数据体
    static std::vector<uint8_t> BuildPacket(PayloadType type, const std::vector<uint8_t>& payload);

    // 解包：获取类型和数据体
    static bool TryParsePacket(const std::vector<uint8_t>& packet, PayloadType& type, std::vector<uint8_t>& payload);
};

