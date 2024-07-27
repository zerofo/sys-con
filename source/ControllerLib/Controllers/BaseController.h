#pragma once

#include "IController.h"
#include <vector>

class RawInputData
{
public:
    RawInputData()
    {
        for (int i = 0; i < MAX_CONTROLLER_BUTTONS; i++)
            buttons[i] = false;
    }

    bool buttons[MAX_CONTROLLER_BUTTONS];

    float Rx = 0;
    float Ry = 0;
    float X = 0;
    float Y = 0;
    float Z = 0;
    float Rz = 0;

    bool dpad_up = false;
    bool dpad_right = false;
    bool dpad_down = false;
    bool dpad_left = false;
};

class BaseController : public IController
{
protected:
    std::vector<IUSBEndpoint *> m_inPipe;
    std::vector<IUSBEndpoint *> m_outPipe;
    std::vector<IUSBInterface *> m_interfaces;

public:
    BaseController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger);
    virtual ~BaseController() override;

    virtual ControllerResult Initialize() override;
    virtual void Exit() override;

    virtual ControllerResult OpenInterfaces();
    virtual void CloseInterfaces();

    virtual bool Support(ControllerFeature feature) override;

    virtual uint16_t GetInputCount() override;

    ControllerResult ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx, uint32_t timeout_us) override;

    virtual ControllerResult ReadInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us) = 0;

    ControllerResult SetRumble(uint16_t input_idx, float amp_high, float amp_low) override;

    // Helper functions
    static float Normalize(int32_t value, int32_t min, int32_t max);
    static float ApplyDeadzone(uint8_t deadzonePercent, float value);
};