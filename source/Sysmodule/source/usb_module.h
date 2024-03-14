#pragma once

#include "vapours/results/results_common.hpp"

namespace syscon::usb
{
    void Initialize();
    void Exit();

    ams::Result Enable();
    void Disable();

    ams::Result CreateUsbEvents();
    void DestroyUsbEvents();
} // namespace syscon::usb