#pragma once

#include "BaseController.h"

// References used:
// http://euc.jp/periphs/xbox-controller.ja.html

struct XboxButtonData
{
    uint8_t type;
    uint8_t length;

    bool dpad_up : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;
    bool dpad_right : 1;

    bool start : 1;
    bool back : 1;
    bool stick_left_click : 1;
    bool stick_right_click : 1;

    uint8_t reserved;

    // These are analog
    uint8_t a;
    uint8_t b;
    uint8_t x;
    uint8_t y;

    uint8_t black_button;
    uint8_t white_button;

    uint8_t trigger_left;
    uint8_t trigger_right;

    int16_t stick_left_x;
    int16_t stick_left_y;
    int16_t stick_right_x;
    int16_t stick_right_y;
};

struct XboxRumbleData
{
    uint8_t command;
    uint8_t size;
    uint8_t dummy1;
    uint8_t big;
    uint8_t dummy2;
    uint8_t little;
};

class XboxController : public BaseController
{
public:
    XboxController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger);
    virtual ~XboxController() override;

    virtual ams::Result ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx) override;

    bool Support(ControllerFeature feature) override;
    ams::Result SetRumble(uint16_t input_idx, uint8_t strong_magnitude, uint8_t weak_magnitude) override;
};