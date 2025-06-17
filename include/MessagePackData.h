#pragma once

#include <string>
#include <queue>
#include <vector>
#include <msgpack.hpp>

struct TestData {
    int id;
    std::string item;
    std::vector<uint8_t> data;

    MSGPACK_DEFINE(id, item, data);
};