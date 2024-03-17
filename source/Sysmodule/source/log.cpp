#include "switch.h"
#include "log.h"
#include <sys/stat.h>
#include <stratosphere.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <vapours/util/util_format_string.hpp>

static ams::os::Mutex printMutex(false);
char gLogBuffer[1024];

// C:\dev\MissionControl\lib\Atmosphere-libs\libstratosphere\source\diag\diag_log.cpp
void DiscardOldLogs()
{
    std::scoped_lock printLock(printMutex);

    FsFileSystem *fs = fsdevGetDeviceFileSystem("sdmc");
    FsFile file;
    s64 fileSize;

    Result rc = fsFsOpenFile(fs, LOG_PATH, FsOpenMode_Read, &file);
    if (R_FAILED(rc))
        return;

    rc = fsFileGetSize(&file, &fileSize);
    fsFileClose(&file);
    if (R_FAILED(rc))
        return;

    if (fileSize >= 0x20'000)
    {
        fsFsDeleteFile(fs, LOG_PATH);
        WriteToLog("Deleted previous log file");
    }
}

ams::Result WriteToLog(const char *fmt, ...)
{
    std::scoped_lock printLock(printMutex);

    ams::TimeSpan ts = ams::os::ConvertToTimeSpan(ams::os::GetSystemTick());

    ams::fs::CreateDirectory(CONFIG_PATH);

    /* Create file if not exists */
    ams::fs::CreateFile(LOG_PATH, 0);

    /* Open the file. */
    ams::fs::FileHandle file;
    R_TRY(ams::fs::OpenFile(std::addressof(file), LOG_PATH, ams::fs::OpenMode_Write | ams::fs::OpenMode_AllowAppend));
    ON_SCOPE_EXIT { ams::fs::CloseFile(file); };

    // Get file size
    s64 fileOffset;
    R_TRY(ams::fs::GetFileSize(&fileOffset, file));

    ams::util::SNPrintf(gLogBuffer, sizeof(gLogBuffer), "%02li:%02li:%02li - ", ts.GetHours() % 24, ts.GetMinutes() % 60, ts.GetSeconds() % 60);

    ::std::va_list vl;
    va_start(vl, fmt);
    ams::util::VSNPrintf(&gLogBuffer[strlen(gLogBuffer)], ams::util::size(gLogBuffer) - strlen(gLogBuffer), fmt, vl);
    va_end(vl);

    R_TRY(ams::fs::WriteFile(file, fileOffset, gLogBuffer, strlen(gLogBuffer), ams::fs::WriteOption::Flush));

    R_SUCCEED();
}
