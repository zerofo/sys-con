#include "switch.h"
#include "logger.h"
#include <sys/stat.h>
#include <stratosphere.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <vapours/util/util_format_string.hpp>

namespace syscon::logger
{
    static ams::os::Mutex printMutex(false);
    char logBuffer[1024];
    char logPath[255];

    ams::Result Initialize(const char *log)
    {
        std::scoped_lock printLock(printMutex);
        s64 fileOffset = 0;

        snprintf(logPath, sizeof(logPath), "sdmc://%s", log);

        /* Create the log file if it doesn't exist. */
        // ams::fs::CreateDirectory(CONFIG_PATH);

        ams::fs::CreateFile(logPath, 0);

        {
            ams::fs::FileHandle file;
            R_TRY(ams::fs::OpenFile(std::addressof(file), logPath, ams::fs::OpenMode_Read));
            ON_SCOPE_EXIT { ams::fs::CloseFile(file); };
            R_TRY(ams::fs::GetFileSize(&fileOffset, file));
        }

        if (fileOffset >= (128 * 1024))
        {
            ams::fs::DeleteFile(logPath);
            syscon::logger::LogWarning("Deleted previous log file");
        }

        R_SUCCEED();
    }

    ams::Result Log(char lvl, const char *fmt, ::std::va_list vl)
    {
        s64 fileOffset;
        ams::fs::FileHandle file;

        std::scoped_lock printLock(printMutex);

        ams::TimeSpan ts = ams::os::ConvertToTimeSpan(ams::os::GetSystemTick());

        memset(logBuffer, 0, sizeof(logBuffer));

        /* Format log */
        ams::util::SNPrintf(logBuffer, sizeof(logBuffer), "|%c|%02li:%02li:%02li| ", lvl, ts.GetHours() % 24, ts.GetMinutes() % 60, ts.GetSeconds() % 60);
        ams::util::VSNPrintf(&logBuffer[strlen(logBuffer)], ams::util::size(logBuffer) - strlen(logBuffer) - 1, fmt, vl); //-1 to leave space for the \n
        logBuffer[strlen(logBuffer)] = '\n';
        logBuffer[strlen(logBuffer) + 1] = '\0';

        /* Write in the file. */
        R_TRY(ams::fs::OpenFile(std::addressof(file), logPath, ams::fs::OpenMode_Write | ams::fs::OpenMode_AllowAppend));
        ON_SCOPE_EXIT { ams::fs::CloseFile(file); };
        R_TRY(ams::fs::GetFileSize(&fileOffset, file));
        R_TRY(ams::fs::WriteFile(file, fileOffset, logBuffer, strlen(logBuffer), ams::fs::WriteOption::Flush));

        R_SUCCEED();
    }

    void LogDebug(const char *fmt, ...)
    {
        ::std::va_list vl;
        va_start(vl, fmt);
        Log('D', fmt, vl);
        va_end(vl);
    }

    void LogInfo(const char *fmt, ...)
    {
        ::std::va_list vl;
        va_start(vl, fmt);
        Log('I', fmt, vl);
        va_end(vl);
    }

    void LogWarning(const char *fmt, ...)
    {
        ::std::va_list vl;
        va_start(vl, fmt);
        Log('W', fmt, vl);
        va_end(vl);
    }

    void LogError(const char *fmt, ...)
    {
        ::std::va_list vl;
        va_start(vl, fmt);
        Log('E', fmt, vl);
        va_end(vl);
    }

    void Exit()
    {
    }

    void Logger::Print(LogLevel lvl, const char *format, ::std::va_list vl)
    {
        switch (lvl)
        {
            case LogLevelDebug:
                LogDebug(format, vl);
                break;
            case LogLevelInfo:
                LogInfo(format, vl);
                break;
            case LogLevelWarning:
                LogWarning(format, vl);
                break;
            case LogLevelError:
                LogError(format, vl);
                break;
        }
    }
} // namespace syscon::logger