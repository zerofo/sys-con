#pragma once

#include "IController.h"

// References used:
// https://cs.chromium.org/chromium/src/device/gamepad/dualshock4_controller.cc

struct GenericHIDButtonData
{
    // byte0
    uint8_t type;
    // byte1
    uint8_t pad0;

    // byte2
    bool back : 1;
    bool stick_left_click : 1;
    bool stick_right_click : 1;
    bool start : 1;

    bool dpad_up : 1;
    bool dpad_right : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;

    // byte3
    bool trigger_left : 1;
    bool trigger_right : 1;
    bool bumper_left : 1;
    bool bumper_right : 1;

    bool triangle : 1;
    bool circle : 1;
    bool cross : 1;
    bool square : 1;

    // byte4
    bool guide : 1;
    uint8_t pad1 : 7;
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
    uint16_t gyroscope;
} __attribute__((packed));

class GenericHIDController : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;

    GenericHIDButtonData m_buttonData{};

public:
    GenericHIDController(std::unique_ptr<IUSBDevice> &&device, std::unique_ptr<ILogger> &&logger);
    virtual ~GenericHIDController() override;

    virtual Result Initialize() override;
    virtual void Exit() override;

    Result OpenInterfaces();
    void CloseInterfaces();

    virtual Result GetInput() override;

    virtual NormalizedButtonData GetNormalizedButtonData() override;

    virtual ControllerType GetType() override { return CONTROLLER_DUALSHOCK3; }

    inline const GenericHIDButtonData &GetButtonData() { return m_buttonData; };

    float NormalizeTrigger(uint8_t deadzonePercent, uint8_t value);
    void NormalizeAxis(uint8_t x, uint8_t y, uint8_t deadzonePercent, float *x_out, float *y_out);

    Result SendInitBytes();
    Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    // static Result SendCommand(IUSBInterface *interface, Dualshock3FeatureValue feature, const void *buffer, uint16_t size);

    static void LoadConfig(const ControllerConfig *config);
    virtual ControllerConfig *GetConfig() override;
};