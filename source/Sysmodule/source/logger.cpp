#include "switch.h"
#include "logger.h"
#include <algorithm>
#include <sys/stat.h>
#include <stratosphere.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <vapours/util/util_format_string.hpp>

#define LOG_FILE_SIZE_MAX (128 * 1024)

namespace syscon::logger
{
    static ams::os::Mutex printMutex(false);
    char logBuffer[1024];
    std::string logPath;
    int logLevel = LOG_LEVEL_INFO;
    char logLevelStr[LOG_LEVEL_COUNT] = {'T', 'D', 'I', 'W', 'E'};

    ams::Result Initialize(const char *log)
    {
        std::scoped_lock printLock(printMutex);
        s64 fileOffset = 0;
        ams::fs::FileHandle file;

        logPath = std::string(log);

        // Create folder if it doesn't exist.
        std::size_t delimBasePath = logPath.rfind('/');
        if (delimBasePath != std::string::npos)
        {
            std::string basePath = logPath.substr(0, delimBasePath);
            ams::fs::CreateDirectory(basePath.c_str());
        }

        // Check if the log file is too big, if so, delete it.
        if (R_SUCCEEDED(ams::fs::OpenFile(std::addressof(file), logPath.c_str(), ams::fs::OpenMode_Read)))
        {
            ams::fs::GetFileSize(&fileOffset, file);
            ams::fs::CloseFile(file);
        }
        if (fileOffset >= LOG_FILE_SIZE_MAX)
            ams::fs::DeleteFile(logPath.c_str());

        // Create the log file if it doesn't exist (Or previously deleted)
        ams::fs::CreateFile(logPath.c_str(), 0);

        R_SUCCEED();
    }

    void SetLogLevel(int level)
    {
        logLevel = level;
    }

    ams::Result LogWriteToFile(const char *logBuffer)
    {
        s64 fileOffset;
        ams::fs::FileHandle file;

        R_TRY(ams::fs::OpenFile(std::addressof(file), logPath.c_str(), ams::fs::OpenMode_Write | ams::fs::OpenMode_AllowAppend));
        ON_SCOPE_EXIT { ams::fs::CloseFile(file); };

        R_TRY(ams::fs::GetFileSize(&fileOffset, file));

        R_TRY(ams::fs::WriteFile(file, fileOffset, logBuffer, strlen(logBuffer), ams::fs::WriteOption::Flush));
        R_TRY(ams::fs::WriteFile(file, fileOffset + strlen(logBuffer), "\n", 1, ams::fs::WriteOption::Flush));

        R_SUCCEED();
    }

    void Log(int lvl, const char *fmt, ::std::va_list vl)
    {
        if (lvl < logLevel)
            return; // Don't log if the level is lower than the current log level.

        std::scoped_lock printLock(printMutex);

        ams::TimeSpan ts = ams::os::ConvertToTimeSpan(ams::os::GetSystemTick());

        /* Format log */
        ams::util::SNPrintf(logBuffer, sizeof(logBuffer), "|%c|%02li:%02li:%02li.%03li|%08X| ", logLevelStr[lvl], ts.GetHours() % 24, ts.GetMinutes() % 60, ts.GetSeconds() % 60, ts.GetMilliSeconds() % 1000, (uint32_t)((uint64_t)threadGetSelf()));
        ams::util::VSNPrintf(&logBuffer[strlen(logBuffer)], ams::util::size(logBuffer) - strlen(logBuffer), fmt, vl);

        /* Write in the file. */
        LogWriteToFile(logBuffer);
    }

    void LogBuffer(int lvl, const uint8_t *buffer, size_t size)
    {
        if (lvl < logLevel)
            return; // Don't log if the level is lower than the current log level.

        std::scoped_lock printLock(printMutex);

        ams::TimeSpan ts = ams::os::ConvertToTimeSpan(ams::os::GetSystemTick());

        ams::util::SNPrintf(logBuffer, sizeof(logBuffer), "|%c|%02li:%02li:%02li.%03li|%08X| ", logLevelStr[lvl], ts.GetHours() % 24, ts.GetMinutes() % 60, ts.GetSeconds() % 60, ts.GetMilliSeconds() % 1000, (uint32_t)((uint64_t)threadGetSelf()));

        size_t start_offset = strlen(logBuffer);

        ams::util::SNPrintf(&logBuffer[strlen(logBuffer)], ams::util::size(logBuffer) - strlen(logBuffer), "Buffer (%ld): ", size);

        LogWriteToFile(logBuffer);

        /* Format log */
        for (size_t i = 0; i < size; i += 16)
        {
            for (size_t k = 0; k < std::min((size_t)16, size - i); k++)
                ams::util::SNPrintf(&logBuffer[start_offset + (k * 3)], ams::util::size(logBuffer) - (start_offset + (k * 3)), "%02X ", buffer[i + k]);

            /* Write in the file. */
            LogWriteToFile(logBuffer);
        }
    }

    void LogTrace(const char *fmt, ...)
    {
        ::std::va_list vl;
        va_start(vl, fmt);
        Log(LOG_LEVEL_TRACE, fmt, vl);
        va_end(vl);
    }

    void LogDebug(const char *fmt, ...)
    {
        ::std::va_list vl;
        va_start(vl, fmt);
        Log(LOG_LEVEL_DEBUG, fmt, vl);
        va_end(vl);
    }

    void LogInfo(const char *fmt, ...)
    {
        ::std::va_list vl;
        va_start(vl, fmt);
        Log(LOG_LEVEL_INFO, fmt, vl);
        va_end(vl);
    }

    void LogWarning(const char *fmt, ...)
    {
        ::std::va_list vl;
        va_start(vl, fmt);
        Log(LOG_LEVEL_WARNING, fmt, vl);
        va_end(vl);
    }

    void LogError(const char *fmt, ...)
    {
        ::std::va_list vl;
        va_start(vl, fmt);
        Log(LOG_LEVEL_ERROR, fmt, vl);
        va_end(vl);
    }

    void Exit()
    {
    }

    void Logger::Print(LogLevel lvl, const char *format, ::std::va_list vl)
    {
        Log(lvl, format, vl);
    }

    void Logger::PrintBuffer(LogLevel lvl, const uint8_t *buffer, size_t size)
    {
        LogBuffer(lvl, buffer, size);
    }

} // namespace syscon::logger