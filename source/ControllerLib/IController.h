#pragma once
#include "IUSBDevice.h"
#include "ILogger.h"
#include "ControllerTypes.h"
#include "ControllerConfig.h"
#include <memory>

struct NormalizedButtonData
{
    bool buttons[MAX_CONTROLLER_BUTTONS];
    float triggers[2];
    NormalizedStick sticks[2];
};

class IController
{
protected:
    std::unique_ptr<IUSBDevice> m_device;
    std::unique_ptr<ILogger> m_logger;

    void LogPrint(LogLevel lvl, const char *format, ...)
    {
        ::std::va_list vl;
        va_start(vl, format);
        m_logger->Print(lvl, format, vl);
        va_end(vl);
    }

public:
    IController(std::unique_ptr<IUSBDevice> &&device, std::unique_ptr<ILogger> &&logger) : m_device(std::move(device)), m_logger(std::move(logger))
    {
    }
    virtual ~IController() = default;

    virtual Result Initialize() = 0;

    // Since Exit is used to clean up resources, no Result report should be needed
    virtual void Exit() = 0;

    virtual Result GetInput() { return 1; }

    virtual NormalizedButtonData GetNormalizedButtonData() { return NormalizedButtonData(); }

    inline IUSBDevice *GetDevice() { return m_device.get(); }
    virtual ControllerType GetType() = 0;
    virtual Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
    {
        (void)strong_magnitude;
        (void)weak_magnitude;
        return 1;
    }
    virtual bool IsControllerActive() { return true; }

    virtual Result OutputBuffer() { return 1; };

    virtual ControllerConfig *GetConfig() { return nullptr; }
};