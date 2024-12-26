#pragma once
#include "ILogger.h"

class MockLogger : public ILogger
{
public:
    void Log(LogLevel aLogLevel, const char *format, ::std::va_list vl) override {}
    void LogBuffer(LogLevel aLogLevel, const uint8_t *buffer, size_t size) override {}
};
