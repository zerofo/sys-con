#pragma once
#include <cstdarg>
#include <string>
#include "ILogger.h"
#include "vapours/results/results_common.hpp"

namespace syscon::logger
{
    ams::Result Initialize(const char *logPath);
    void Exit();

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
