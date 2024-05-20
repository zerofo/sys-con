#pragma once
#include <cstdarg>

typedef enum LogLevel
{
    LogLevelTrace = 0,
    LogLevelDebug,
    LogLevelInfo,
    LogLevelWarning,
    LogLevelError
} LogLevel;

class ILogger
{
public:
    virtual ~ILogger() = default;
    virtual void Print(LogLevel aLogLevel, const char *format, ::std::va_list vl) = 0;
    virtual void PrintBuffer(LogLevel aLogLevel, const uint8_t *buffer, size_t size) = 0;
};