#pragma once
#include <string>
#include <vector>
#include <functional>

class IZMQSocket
{
public:
    virtual ~IZMQSocket() = default;

    // �����ַ���
    virtual void send(const std::string& message) = 0;

    // ���Ͷ���������
    virtual void send(const std::vector<uint8_t>& data) = 0;

    // �����ַ�����Ϣ�ص������������н��չ��ܵ�socket��
    virtual void set_string_callback(std::function<void(const std::string&)> callback) = 0;

    // ���ö�������Ϣ�ص�
    virtual void set_callback(std::function<void(const std::vector<uint8_t>&)> callback) = 0;

    // �ر���Դ
    virtual void shutdown() = 0;
};

