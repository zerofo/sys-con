#pragma once
#include <cstdarg>

typedef enum LogLevel
{
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
};