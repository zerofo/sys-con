#pragma once

#include "config_handler.h"
namespace syscon::usb
{
    Result Initialize(syscon::config::DiscoveryMode discovery_mode, std::vector<syscon::config::ControllerVidPid> &discovery_vidpid, bool auto_add_controller);
    void Exit();
}