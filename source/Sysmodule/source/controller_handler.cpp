#include "switch.h"
#include "controller_handler.h"
#include "SwitchHDLHandler.h"
#include "SwitchUSBInterface.h"
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
            syscon::logger::LogInfo("Controller[%04x-%04x] plugged !", switchHandler->GetController()->GetDevice()->GetVendor(), switchHandler->GetController()->GetDevice()->GetProduct());

            std::scoped_lock scoped_lock(controllerMutex);
            controllerHandlers.push_back(std::move(switchHandler));
        }
        else
        {
            syscon::logger::LogError("Controller[%04x-%04x] Failed to initialize controller: Error: 0x%X (Module: 0x%X, Desc: 0x%X)", switchHandler->GetController()->GetDevice()->GetVendor(), switchHandler->GetController()->GetDevice()->GetProduct(), rc.GetValue(), R_MODULE(rc.GetValue()), R_DESCRIPTION(rc.GetValue()));
        }

        return rc;
    }

    void RemoveIfNotPlugged(std::vector<s32> interfaceIDsPlugged)
    {
        std::scoped_lock scoped_lock(controllerMutex);
        for (auto it = controllerHandlers.begin(); it != controllerHandlers.end(); it++)
        {
            bool found = false;

            for (auto &&ptr : (*it)->GetController()->GetDevice()->GetInterfaces())
            {
                for (auto &&interfaceID : interfaceIDsPlugged)
                {
                    if (static_cast<SwitchUSBInterface *>(ptr.get())->GetID() == interfaceID)
                    {
                        found = true;
                        break;
                    }
                }
            }

            // We check if a device was removed by comparing the controller's interfaces and the currently acquired interfaces
            // If we didn't find a single matching interface ID, we consider a controller removed
            if (!found)
            {
                syscon::logger::LogInfo("Controller[%04x-%04x] unplugged !", (*it)->GetController()->GetDevice()->GetVendor(), (*it)->GetController()->GetDevice()->GetProduct());
                controllerHandlers.erase(it--);
            }
        }
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
        syscon::logger::LogInfo("Controllers Reset !");
        std::scoped_lock scoped_lock(controllerMutex);
        controllerHandlers.clear();
    }

    void Exit()
    {
        Reset();
    }
} // namespace syscon::controllers