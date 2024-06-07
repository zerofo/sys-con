#pragma once
#include "ControllerTypes.h"
#include "ControllerConfig.h"
#include <string>
#include <switch.h>
#include <stratosphere.hpp>

#define CONFIG_PATH     "sdmc:///config/sys-con/"
#define CONFIG_FULLPATH CONFIG_PATH "config.ini"

namespace syscon::config
{
    typedef enum DiscoveryMode
    {
        Everything = 0,
        OnlyKnownVIDPID = 1,
    } DiscoveryMode;

    struct GlobalConfig
    {
        uint16_t polling_frequency_ms;
        int log_level;
        DiscoveryMode discovery_mode;
    };

    class ControllerVidPid
    {
    public:
        ControllerVidPid(uint16_t vid, uint16_t pid)
            : vid(vid), pid(pid)
        {
        }

        operator std::string() const
        {
            std::stringstream ss;
            ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << vid;
            ss << "-";
            ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << pid;
            return ss.str();
        }

        bool operator==(const ControllerVidPid &other) const
        {
            return vid == other.vid && pid == other.pid;
        }

        uint16_t vid;
        uint16_t pid;
    };

    ams::Result LoadGlobalConfig(GlobalConfig *config);

    ams::Result LoadControllerConfig(ControllerConfig *config, uint16_t vendor_id, uint16_t product_id);

    ams::Result LoadControllerList(std::vector<ControllerVidPid> *vid_pid);

}; // namespace syscon::config