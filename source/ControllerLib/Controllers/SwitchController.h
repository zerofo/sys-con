#pragma once

#include "BaseController.h"

// References used:
// https://gist.github.com/ToadKing/30d65150410df13763f26f45abbd3700
// https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/USB-HID-Notes.md
// https://github.com/shinyquagsire23/HID-Joy-Con-Whispering/blob/master/hidtest/hidtest.cpp

struct SwitchButtonData
{
    uint8_t report_id;
    uint8_t timer;
    uint8_t bat_con;

    bool button8 : 1;
    bool button7 : 1;
    bool button6 : 1;
    bool button5 : 1;
    bool button4 : 1;
    bool button3 : 1;
    bool button2 : 1;
    bool button1 : 1;

    bool dummy1 : 1;
    bool button15 : 1;
    bool button14 : 1;
    bool button13 : 1;
    bool button12 : 1;
    bool button11 : 1;
    bool button10 : 1;
    bool button9 : 1;

    uint8_t reserved : 4;
    bool dpad_up : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;
    bool dpad_right : 1;

    int16_t stick_left_x;
    int16_t stick_left_y;
    int16_t stick_right_x;
    int16_t stick_right_y;
};

class SwitchController : public BaseController
{
public:
    SwitchController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger);
    virtual ~SwitchController() override;

    ControllerResult Initialize() override;

    virtual ControllerResult ReadRawInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us) override;

    bool Support(ControllerFeature feature) override;
};