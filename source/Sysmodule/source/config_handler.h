#pragma once
#include "ControllerTypes.h"
#include "ControllerConfig.h"
#include <string>
#include <switch.h>

#define CONFIG_PATH     "sdmc:///config/sys-con/"
#define CONFIG_FULLPATH CONFIG_PATH "config.ini"

namespace syscon::config
{
    struct GlobalConfig
    {
        uint16_t polling_frequency_ms;
        int log_level;
    };

    Result LoadGlobalConfig(GlobalConfig *config);

    Result LoadControllerConfig(ControllerConfig *config, uint16_t vendor_id, uint16_t product_id);
}; // namespace syscon::config