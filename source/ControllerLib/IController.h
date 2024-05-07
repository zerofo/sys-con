#pragma once
#include "IUSBDevice.h"
#include "ILogger.h"
#include "ControllerTypes.h"
#include "ControllerConfig.h"

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
    IController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger) : m_device(std::move(device)), m_config(config), m_logger(std::move(logger))
    {
    }
    virtual ~IController() = default;

    virtual ams::Result Initialize() = 0;

    // Since Exit is used to clean up resources, no Result report should be needed
    virtual void Exit() = 0;

    virtual uint16_t GetInputCount() { return 1; }

    virtual ams::Result ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
    {
        R_TRY(GetInput());

        *normalData = GetNormalizedButtonData();
        *input_idx = 0;
        R_SUCCEED();
    }

    virtual ams::Result GetInput() { return 1; }

    virtual NormalizedButtonData GetNormalizedButtonData() { return NormalizedButtonData(); }

    inline IUSBDevice *GetDevice() { return m_device.get(); }

    virtual bool Support(ControllerFeature aFeature) = 0;

    virtual ams::Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
    {
        (void)strong_magnitude;
        (void)weak_magnitude;
        return 1;
    }
    virtual bool IsControllerActive() { return true; }

    virtual ams::Result OutputBuffer() { return 1; };

    float NormalizeTrigger(uint8_t deadzonePercent, uint8_t value)
    {
        uint8_t deadzone = (UINT8_MAX * deadzonePercent) / 100;

        if (value < deadzone)
            return 0;

        return static_cast<float>(value - deadzone) / (UINT8_MAX - deadzone);
    }

    void NormalizeAxis(uint8_t x, uint8_t y, uint8_t deadzonePercent, float *x_out, float *y_out, int32_t min, int32_t max)
    {
        //*x_out, *y_out is between -1.0 and 1.0
        float ratio = 0;
        float x_val = x;
        float y_val = y;
        int32_t range = max;

        if (min >= 0)
        {
            x_val = x - (max / 2);
            y_val = (max / 2) - y;
            range = (max / 2);
        }

        float real_magnitude = std::sqrt(x_val * x_val + y_val * y_val);
        float real_deadzone = (range * deadzonePercent) / 100;
        if (real_magnitude > real_deadzone)
        {
            float magnitude = std::min((float)range, real_magnitude);
            magnitude -= real_deadzone;
            ratio = (magnitude / (range - real_deadzone)) / real_magnitude;
        }
        else
        {
            *x_out = *y_out = 0.0f;
        }
        *x_out = x_val * ratio;
        *y_out = y_val * ratio;
    }

    virtual const ControllerConfig &GetConfig() { return m_config; }
};