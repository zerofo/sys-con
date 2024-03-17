#pragma once
#include "config_handler.h"
#include "vapours/results/results_common.hpp"

#define LOG_PATH CONFIG_PATH "log.txt"

#ifdef __cplusplus
extern "C"
{
#endif
    void DiscardOldLogs();

    ams::Result WriteToLog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

    void LockedUpdateConsole();

#ifdef __cplusplus
}
#endif
