#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

class HexUtils {
public:
    // 字节向十六进制字符串转换
    static std::string BytesToHex(const std::vector<uint8_t>& data) {
        std::ostringstream oss;
        for (uint8_t byte : data) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return oss.str();
    }

    // 十六进制字符串转换回字节（可选）
    static std::vector<uint8_t> HexToBytes(const std::string& hex) {
        std::vector<uint8_t> result;
        if (hex.length() % 2 != 0) return result;

        for (size_t i = 0; i < hex.length(); i += 2) {
            uint8_t byte = static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16));
            result.push_back(byte);
        }
        return result;
    }
};
