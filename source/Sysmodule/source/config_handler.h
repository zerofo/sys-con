#pragma once
#include "ControllerTypes.h"

#define CONFIG_PATH "/config/sys-con/"

#define GLOBALCONFIG     "sdmc://" CONFIG_PATH "config_global.ini"
#define XBOXCONFIG       "sdmc://" CONFIG_PATH "config_xboxorig.ini"
#define XBOX360CONFIG    "sdmc://" CONFIG_PATH "config_xbox360.ini"
#define XBOXONECONFIG    "sdmc://" CONFIG_PATH "config_xboxone.ini"
#define DUALSHOCK3CONFIG "sdmc://" CONFIG_PATH "config_dualshock3.ini"
#define DUALSHOCK4CONFIG "sdmc://" CONFIG_PATH "config_dualshock4.ini"
#define GENERICCONFIG    "sdmc://" CONFIG_PATH "config_generic.ini"

namespace syscon::config
{
    struct GlobalConfig
    {
        uint16_t polling_frequency_ms;
        int log_level;
    };

    inline GlobalConfig globalConfig{};

    void LoadGlobalConfig(const GlobalConfig &config);
    void LoadAllConfigs();
    bool CheckForFileChanges();

    Result Initialize();
    void Exit();

    Result Enable();
    void Disable();
}; // namespace syscon::config