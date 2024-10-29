#pragma once

#include "SwitchVirtualGamepadHandler.h"
#include <stratosphere.hpp>

namespace syscon::controllers
{
    bool IsAtControllerLimit();

    ams::Result Insert(std::unique_ptr<IController> &&controllerPtr);
    void RemoveIfNotPlugged(std::vector<s32> interfaceIDsPlugged);

    void SetPollingParameters(s32 _polling_frequency_ms, s8 _thread_priority);

    void Initialize();
    void Clear();
    void Exit();
} // namespace syscon::controllers