#pragma once

#include "BaseController.h"
#include "Controllers/Xbox360Controller.h"

#define XBOX360_MAX_INPUTS 4

class Xbox360WirelessController : public BaseController
{
private:
    bool m_is_connected[XBOX360_MAX_INPUTS];
    uint8_t m_current_controller_idx = 0;

    ams::Result SetLED(uint16_t input_idx, Xbox360LEDValue value);

    ams::Result OnControllerConnect(uint16_t input_idx);
    ams::Result OnControllerDisconnect(uint16_t input_idx);

public:
    Xbox360WirelessController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger);
    virtual ~Xbox360WirelessController() override;

    ams::Result OpenInterfaces() override;
    void CloseInterfaces() override;

    ams::Result ReadInput(RawInputData *rawData, uint16_t *input_idx) override;

    bool Support(ControllerFeature feature) override;

    uint16_t GetInputCount() override;

    ams::Result SetRumble(uint16_t input_idx, float amp_high, float amp_low) override;

    bool IsControllerConnected(uint16_t input_idx) override;
};