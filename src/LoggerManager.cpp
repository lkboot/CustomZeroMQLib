#include "LoggerManager.h"
#include <mutex>
#include <iostream>

namespace {
    std::once_flag initFlag;
}

void LoggerManager::Init()
{
    std::call_once(initFlag, []() {
        try
        {
            std::cout << "[LoggerManager] Init called" << std::endl;
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs\\zeromq_log.txt", true);

            std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
            auto logger = std::make_shared<spdlog::logger>("zmq", sinks.begin(), sinks.end());

            spdlog::set_default_logger(logger);
            spdlog::set_level(spdlog::level::debug);
            spdlog::flush_on(spdlog::level::debug);
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        }
        });
}

void LoggerManager::SetLevel(spdlog::level::level_enum level)
{
    spdlog::set_level(level);
    spdlog::flush_on(level);
}
