#pragma once

#include "IController.h"

// References used:
// https://cs.chromium.org/chromium/src/device/gamepad/dualshock4_controller.cc

struct GenericHIDButtonData
{
    // byte0
    uint8_t type;

    // byte1
    uint8_t dpad_left_right; // right = 0xFF left = 0x00 None = 0x80

    // byte2
    uint8_t dpad_up_down; // up = 0x00 down = 0xFF None = 0x80

    // byte3
    bool button1 : 1;
    bool button2 : 1;
    bool button3 : 1;
    bool button4 : 1;
    bool button5 : 1;
    bool button6 : 1;
    bool button7 : 1;
    bool button8 : 1;

    // byte4
    bool button9 : 1;
    bool button10 : 1;
    bool button11 : 1;
    bool button12 : 1;
    bool button13 : 1;
    bool button14 : 1;
    bool button15 : 1;
    bool button16 : 1;

    // byte5
    uint8_t pad2;

} __attribute__((packed));

class GenericHIDController : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;
    bool m_features[SUPPORTS_COUNT];
    uint16_t m_inputCount = 0;

public:
    GenericHIDController(std::unique_ptr<IUSBDevice> &&device, std::unique_ptr<ILogger> &&logger);
    virtual ~GenericHIDController() override;

    virtual ams::Result Initialize() override;
    virtual void Exit() override;

    ams::Result OpenInterfaces();
    void CloseInterfaces();

    virtual uint16_t GetInputCount() override;

    virtual ams::Result ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx) override;

    virtual bool Support(ControllerFeature feature) override;

    float NormalizeTrigger(uint8_t deadzonePercent, uint8_t value);
    void NormalizeAxis(uint8_t x, uint8_t y, uint8_t deadzonePercent, float *x_out, float *y_out);

    ams::Result SendInitBytes();
    ams::Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    static void LoadConfig(const ControllerConfig *config);
    virtual ControllerConfig *GetConfig() override;
};