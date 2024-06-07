#pragma once

#include "vapours/results/results_common.hpp"
#include "config_handler.h"
namespace syscon::usb
{
    void Initialize(syscon::config::DiscoveryMode discovery_mode);
    void Exit();
} // namespace syscon::usb