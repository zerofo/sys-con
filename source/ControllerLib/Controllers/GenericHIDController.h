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

    // byte6-7
    uint8_t stick_left_x;
    uint8_t stick_left_y;

    // byte8-9
    uint8_t stick_right_x;
    uint8_t stick_right_y;

    // byte10-13
    uint8_t pad3[4];

    // byte14-17 These respond for how hard the corresponding button has been pressed. 0xFF completely, 0x00 not at all
    uint8_t dpad_up_pressure;
    uint8_t dpad_right_pressure;
    uint8_t dpad_down_pressure;
    uint8_t dpad_left_pressure;

    // byte18-19
    uint8_t trigger_left_pressure;
    uint8_t trigger_right_pressure;
    /*
                // byte20-21
                uint8_t bumper_left_pressure;
                uint8_t bumper_right_pressure;

                // byte22-25
                uint8_t triangle_pressure;
                uint8_t circle_pressure;
                uint8_t cross_pressure;
                uint8_t square_pressure;

                uint8_t pad4[15];

                // byte41-48
                uint16_t accelerometer_x;
                uint16_t accelerometer_y;
                uint16_t accelerometer_z;
                uint16_t gyroscope;*/
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

    virtual Result Initialize() override;
    virtual void Exit() override;

    Result OpenInterfaces();
    void CloseInterfaces();

    virtual uint16_t GetInputCount() override;

    virtual Result ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx) override;

    virtual bool Support(ControllerFeature feature) override;

    float NormalizeTrigger(uint8_t deadzonePercent, uint8_t value);
    void NormalizeAxis(uint8_t x, uint8_t y, uint8_t deadzonePercent, float *x_out, float *y_out);

    Result SendInitBytes();
    Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    // static Result SendCommand(IUSBInterface *interface, Dualshock3FeatureValue feature, const void *buffer, uint16_t size);

    static void LoadConfig(const ControllerConfig *config);
    virtual ControllerConfig *GetConfig() override;
};