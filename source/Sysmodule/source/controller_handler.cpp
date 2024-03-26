#include "switch.h"
#include "controller_handler.h"
#include "SwitchHDLHandler.h"
#include "SwitchAbstractedPadHandler.h"
#include <algorithm>
#include <functional>

#include "logger.h"

namespace syscon::controllers
{
    namespace
    {
        constexpr size_t MaxControllerHandlersSize = 10;
        std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>> controllerHandlers;
        bool UseAbstractedPad;
        ams::os::Mutex controllerMutex(false);
        int polling_frequency_ms = 0;
    } // namespace

    bool IsAtControllerLimit()
    {
        return controllerHandlers.size() >= MaxControllerHandlersSize;
    }

    ams::Result Insert(std::unique_ptr<IController> &&controllerPtr)
    {
        std::unique_ptr<SwitchVirtualGamepadHandler> switchHandler;
        if (UseAbstractedPad)
        {
            switchHandler = std::make_unique<SwitchAbstractedPadHandler>(std::move(controllerPtr), polling_frequency_ms);
            syscon::logger::LogInfo("Inserting controller as abstracted pad");
        }
        else
        {
            switchHandler = std::make_unique<SwitchHDLHandler>(std::move(controllerPtr), polling_frequency_ms);
            syscon::logger::LogInfo("Inserting controller as HDLs");
        }

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
    /*
    void Remove(std::function func)
    {
        std::remove_if(controllerHandlers.begin(), controllerHandlers.end(), func);
    }
    */

    void SetPollingFrequency(int _polling_frequency_ms)
    {
        std::scoped_lock scoped_lock(controllerMutex);
        polling_frequency_ms = _polling_frequency_ms;
    }

    void Initialize()
    {
        UseAbstractedPad = hosversionBetween(5, 7);
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