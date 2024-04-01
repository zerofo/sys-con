#pragma once
#include <cstdarg>
#include <string>
#include "ILogger.h"
#include "vapours/results/results_common.hpp"

#define LOG_LEVEL_DEBUG   0
#define LOG_LEVEL_INFO    1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR   3
#define LOG_LEVEL_COUNT   4

namespace syscon::logger
{
    ams::Result Initialize(const char *logPath);
    void Exit();

    void SetLogLevel(int level);

    void LogDebug(const char *format, ...);
    void LogInfo(const char *format, ...);
    void LogWarning(const char *format, ...);
    void LogError(const char *format, ...);

    class Logger : public ILogger
    {
    public:
        void Print(LogLevel lvl, const char *format, ::std::va_list vl) override;
    };
} // namespace syscon::logger