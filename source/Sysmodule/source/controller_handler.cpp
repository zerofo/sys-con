#include "switch.h"
#include "controller_handler.h"
#include "SwitchHDLHandler.h"
#include <algorithm>
#include <functional>

#include "logger.h"

namespace syscon::controllers
{
    namespace
    {
        constexpr size_t MaxControllerHandlersSize = 10;
        std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>> controllerHandlers;
        ams::os::Mutex controllerMutex(false);
        int polling_frequency_ms = 0;
    } // namespace

    bool IsAtControllerLimit()
    {
        std::scoped_lock scoped_lock(controllerMutex);
        return controllerHandlers.size() >= MaxControllerHandlersSize;
    }

    ams::Result Insert(std::unique_ptr<IController> &&controllerPtr)
    {
        std::unique_ptr<SwitchVirtualGamepadHandler> switchHandler = std::make_unique<SwitchHDLHandler>(std::move(controllerPtr), polling_frequency_ms);

        ams::Result rc = switchHandler->Initialize();
        if (R_SUCCEEDED(rc))
        {
            std::scoped_lock scoped_lock(controllerMutex);
            controllerHandlers.push_back(std::move(switchHandler));
        }
        else
        {
            syscon::logger::LogError("Failed to initialize controller: Error: 0x%X (Module: 0x%X, Desc: 0x%X)", rc.GetValue(), R_MODULE(rc.GetValue()), R_DESCRIPTION(rc.GetValue()));
        }

        return rc;
    }

    std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>> &Get()
    {
        return controllerHandlers;
    }

    ams::os::Mutex &GetScopedLock()
    {
        return controllerMutex;
    }

    void SetPollingFrequency(int _polling_frequency_ms)
    {
        polling_frequency_ms = _polling_frequency_ms;
    }

    void Initialize()
    {
        controllerHandlers.reserve(MaxControllerHandlersSize);
    }

    void Reset()
    {
        std::scoped_lock scoped_lock(controllerMutex);
        controllerHandlers.clear();
    }

    void Exit()
    {
        Reset();
    }
} // namespace syscon::controllers