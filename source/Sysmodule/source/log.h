#pragma once
#include "config_handler.h"
#include "vapours/results/results_common.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
    ams::Result DiscardOldLogs();

    ams::Result WriteToLog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#ifdef __cplusplus
}
#endif
