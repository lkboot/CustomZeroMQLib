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
    // ������������ + 4�ֽڳ��ȣ�С�ˣ�+ ������
    static std::vector<uint8_t> BuildPacket(PayloadType type, const std::vector<uint8_t>& payload);

    // �������ȡ���ͺ�������
    static bool TryParsePacket(const std::vector<uint8_t>& packet, PayloadType& type, std::vector<uint8_t>& payload);
};

