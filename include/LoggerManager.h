#pragma once

#include <memory>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class LoggerManager
{
public:
    static void Init();
    static void SetLevel(spdlog::level::level_enum level);
};
