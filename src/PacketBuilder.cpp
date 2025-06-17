#include "PacketBuilder.h"
#include <cstring>

std::vector<uint8_t> PacketBuilder::BuildPacket(PayloadType type, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> packet(1 + 4 + payload.size());

    // 类型
    packet[0] = static_cast<uint8_t>(type);

    // 长度：小端
    uint32_t length = static_cast<uint32_t>(payload.size());
    packet[1] = length & 0xFF;
    packet[2] = (length >> 8) & 0xFF;
    packet[3] = (length >> 16) & 0xFF;
    packet[4] = (length >> 24) & 0xFF;

    // 数据体
    std::memcpy(packet.data() + 5, payload.data(), payload.size());

    return packet;
}

bool PacketBuilder::TryParsePacket(const std::vector<uint8_t>& packet, PayloadType& type, std::vector<uint8_t>& payload) {
    if (packet.size() < 5)
        return false;

    type = static_cast<PayloadType>(packet[0]);

    // 提取长度（小端）
    uint32_t length =
        (packet[1]) |
        (packet[2] << 8) |
        (packet[3] << 16) |
        (packet[4] << 24);

    if (packet.size() < 5 + length)
        return false;

    payload.assign(packet.begin() + 5, packet.begin() + 5 + length);
    return true;
}
