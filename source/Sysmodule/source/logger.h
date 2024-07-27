#pragma once
#include <cstdarg>
#include <string>
#include "ILogger.h"
#include "vapours/results/results_common.hpp"

#define LOG_LEVEL_TRACE   0
#define LOG_LEVEL_DEBUG   1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_ERROR   4
#define LOG_LEVEL_COUNT   5

namespace syscon::logger
{
    ams::Result Initialize(const char *logPath);
    void Exit();

    void SetLogLevel(int level);

    void LogTrace(const char *format, ...);
    void LogDebug(const char *format, ...);
    void LogInfo(const char *format, ...);
    void LogWarning(const char *format, ...);
    void LogError(const char *format, ...);

    void Log(int lvl, const char *fmt, ::std::va_list vl);
    void LogBuffer(int lvl, const uint8_t *buffer, size_t size);

    class Logger : public ILogger
    {
    public:
        void Log(LogLevel lvl, const char *format, ::std::va_list vl) override;
        void LogBuffer(LogLevel lvl, const uint8_t *buffer, size_t size) override;
    };
} // namespace syscon::logger
