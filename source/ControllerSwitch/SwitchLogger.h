#pragma once

namespace syscon::logger
{
    void LogDebug(const char *fmt, ...);
    void LogInfo(const char *fmt, ...);
    void LogWarning(const char *fmt, ...);
    void LogError(const char *fmt, ...);
} // namespace syscon::logger
