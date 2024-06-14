#pragma once
#include "IUSBDevice.h"
#include "ILogger.h"
#include "ControllerTypes.h"
#include "ControllerConfig.h"

struct NormalizedStick
{
    float axis_x;
    float axis_y;
};

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
    ControllerConfig m_config;
    std::unique_ptr<ILogger> m_logger;

    void LogPrint(LogLevel lvl, const char *format, ...)
    {
        ::std::va_list vl;
        va_start(vl, format);
        m_logger->Print(lvl, format, vl);
        va_end(vl);
    }

    void LogBuffer(LogLevel lvl, const uint8_t *buffer, size_t size)
    {
        m_logger->PrintBuffer(lvl, buffer, size);
    }

public:
    IController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger) : m_device(std::move(device)),
                                                                                                                           m_config(config),
                                                                                                                           m_logger(std::move(logger))
    {
    }
    virtual ~IController() = default;

    virtual ams::Result Initialize() = 0;
    virtual void Exit() = 0;

    virtual uint16_t GetInputCount() = 0;
    virtual ams::Result ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx) = 0;

    virtual bool Support(ControllerFeature aFeature) = 0;

    virtual ams::Result SetRumble(uint16_t input_idx, float amp_high, float amp_low) = 0;

    virtual bool IsControllerConnected(uint16_t input_idx)
    {
        (void)input_idx;
        return true;
    }

    inline const ControllerConfig &GetConfig() const { return m_config; }

    inline IUSBDevice *GetDevice() { return m_device.get(); }
};