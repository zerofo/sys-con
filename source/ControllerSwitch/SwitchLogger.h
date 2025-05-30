#pragma once

#define LOG_LEVEL_TRACE   0
#define LOG_LEVEL_DEBUG   1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_ERROR   4
#define LOG_LEVEL_COUNT   5

namespace syscon::logger
{
    void LogTrace(const char *fmt, ...);
    void LogDebug(const char *fmt, ...);
    void LogPerf(const char *fmt, ...);
    void LogInfo(const char *fmt, ...);
    void LogWarning(const char *fmt, ...);
    void LogError(const char *fmt, ...);
    void LogBuffer(int lvl, const uint8_t *buffer, size_t size);
} // namespace syscon::logger
