#include "switch.h"
#include "log.h"
#include <sys/stat.h>
#include <stratosphere.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <vapours/util/util_format_string.hpp>

#define LOG_PATH "sdmc://" CONFIG_PATH "log.txt"

static ams::os::Mutex printMutex(false);
char gLogBuffer[1024];

ams::Result DiscardOldLogs()
{
    std::scoped_lock printLock(printMutex);
    s64 fileOffset = 0;

    {
        ams::fs::FileHandle file;
        R_TRY(ams::fs::OpenFile(std::addressof(file), LOG_PATH, ams::fs::OpenMode_Read));
        ON_SCOPE_EXIT { ams::fs::CloseFile(file); };
        R_TRY(ams::fs::GetFileSize(&fileOffset, file));
    }

    if (fileOffset >= (128 * 1024))
    {
        ams::fs::DeleteFile(LOG_PATH);
        WriteToLog("Deleted previous log file");
    }

    R_SUCCEED();
}

ams::Result WriteToLog(const char *fmt, ...)
{
    s64 fileOffset;
    ams::fs::FileHandle file;

    std::scoped_lock printLock(printMutex);

    ams::TimeSpan ts = ams::os::ConvertToTimeSpan(ams::os::GetSystemTick());

    /* Create the log file if it doesn't exist. */
    ams::fs::CreateDirectory(CONFIG_PATH);
    ams::fs::CreateFile(LOG_PATH, 0);

    /* Format log */
    ams::util::SNPrintf(gLogBuffer, sizeof(gLogBuffer), "%02li:%02li:%02li - ", ts.GetHours() % 24, ts.GetMinutes() % 60, ts.GetSeconds() % 60);

    ::std::va_list vl;
    va_start(vl, fmt);
    ams::util::VSNPrintf(&gLogBuffer[strlen(gLogBuffer)], ams::util::size(gLogBuffer) - strlen(gLogBuffer) - 1, fmt, vl); //-1 to leave space for the \n
    va_end(vl);

    gLogBuffer[strlen(gLogBuffer)] = '\n';
    gLogBuffer[strlen(gLogBuffer) + 1] = '\0';

    /* Write in the file. */
    R_TRY(ams::fs::OpenFile(std::addressof(file), LOG_PATH, ams::fs::OpenMode_Write | ams::fs::OpenMode_AllowAppend));
    ON_SCOPE_EXIT { ams::fs::CloseFile(file); };

    R_TRY(ams::fs::GetFileSize(&fileOffset, file));
    R_TRY(ams::fs::WriteFile(file, fileOffset, gLogBuffer, strlen(gLogBuffer), ams::fs::WriteOption::Flush));

    R_SUCCEED();
}
