#pragma once
#include "BaseController.h"

// References used:
// https://cs.chromium.org/chromium/src/device/gamepad/dualshock4_controller.cc

struct Dualshock4USBButtonData
{
    uint8_t type;
    uint8_t stick_left_x;
    uint8_t stick_left_y;
    uint8_t stick_right_x;
    uint8_t stick_right_y;

    uint8_t dpad : 4;
    bool square : 1;
    bool cross : 1;
    bool circle : 1;
    bool triangle : 1;

    bool l1 : 1;
    bool r1 : 1;
    bool l2 : 1;
    bool r2 : 1;
    bool share : 1;
    bool options : 1;
    bool l3 : 1;
    bool r3 : 1;

    bool psbutton : 1;
    bool touchpad_press : 1;
    uint8_t timestamp : 6;

    uint8_t l2_pressure;
    uint8_t r2_pressure;

    uint8_t counter[2];

    uint8_t battery_level;

    uint8_t unk;

    uint16_t gyroX;
    uint16_t gyroY;
    uint16_t gyroZ;

    uint16_t accelX;
    uint16_t accelY;
    uint16_t accelZ;

    uint32_t unk2;

    uint8_t headset_bitmask;

    uint16_t unk3;

    uint8_t touchpad_eventData : 4;
    uint8_t unk4 : 4;

    uint8_t touchpad_counter;
    uint8_t touchpad_finger1_counter : 7;
    bool touchpad_finger1_unpressed : 1;
    uint16_t touchpad_finger1_X;
    uint8_t touchpad_finger1_Y;

    uint8_t touchpad_finger2_counter : 7;
    bool touchpad_finger2_unpressed : 1;
    uint16_t touchpad_finger2_X;
    uint8_t touchpad_finger2_Y;

    uint8_t touchpad_finger1_prev[4];
    uint8_t touchpad_finger2_prev[4];
};

enum Dualshock4Dpad : uint8_t
{
    DS4_UP,
    DS4_UPRIGHT,
    DS4_RIGHT,
    DS4_DOWNRIGHT,
    DS4_DOWN,
    DS4_DOWNLEFT,
    DS4_LEFT,
    DS4_UPLEFT,
};

class Dualshock4Controller : public BaseController
{
public:
    Dualshock4Controller(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger);
    virtual ~Dualshock4Controller() override;

    virtual ams::Result Initialize() override;

    virtual ams::Result ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx) override;

    ams::Result SendInitBytes();
};