#pragma once

#include "BaseController.h"
#include "HIDJoystick.h"
#include <vector>

// References used:
// https://cs.chromium.org/chromium/src/device/gamepad/xbox_controller_mac.mm

#define XBOX360_MAX_INPUTS 4

struct Xbox360ButtonData
{
    uint8_t type;
    uint8_t length;

    bool dpad_up : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;
    bool dpad_right : 1;

    bool button8 : 1; // start
    bool button7 : 1; // back
    bool button9 : 1;
    bool button10 : 1;

    bool button5 : 1;
    bool button6 : 1;
    bool button11 : 1;
    bool dummy1 : 1; // Always 0.

    bool button1 : 1;
    bool button2 : 1;
    bool button3 : 1;
    bool button4 : 1;

    uint8_t Z;
    uint8_t Rz;

    int16_t X;
    int16_t Y;
    int16_t Rx;
    int16_t Ry;
};

enum Xbox360InputPacketType : uint8_t
{
    XBOX360INPUT_BUTTON = 0,
    XBOX360INPUT_LED = 1,
    XBOX360INPUT_RUMBLE = 3,
};

enum Xbox360LEDValue : uint8_t
{
    XBOX360LED_OFF = 0,
    XBOX360LED_ALLBLINK,
    XBOX360LED_TOPLEFTBLINK,
    XBOX360LED_TOPRIGHTBLINK,
    XBOX360LED_BOTTOMLEFTBLINK,
    XBOX360LED_BOTTOMRIGHTBLINK,
    XBOX360LED_TOPLEFT,
    XBOX360LED_TOPRIGHT,
    XBOX360LED_BOTTOMLEFT,
    XBOX360LED_BOTTOMRIGHT,
    XBOX360LED_ROTATE,
    XBOX360LED_BLINK,
    XBOX360LED_SLOWBLINK,
    XBOX360LED_ROTATE_2,
    XBOX360LED_ALLSLOWBLINK,
    XBOX360LED_BLINKONCE,
};

struct OutputPacket
{
    const uint8_t *packet;
    uint8_t length;
};

class Xbox360Controller : public BaseController
{
private:
    bool m_is_wireless = false;
    bool m_is_connected[XBOX360_MAX_INPUTS];

    ams::Result SetLED(uint16_t input_idx, Xbox360LEDValue value);

    ams::Result OnControllerConnect(uint16_t input_idx);
    ams::Result OnControllerDisconnect(uint16_t input_idx);

    void CloseInterfaces();

public:
    Xbox360Controller(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger);
    virtual ~Xbox360Controller() override;

    ams::Result Initialize() override;

    ams::Result ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx) override;

    bool Support(ControllerFeature feature) override;

    uint16_t GetInputCount() override;

    ams::Result SetRumble(uint16_t input_idx, float amp_high, float amp_low) override;

    bool IsControllerConnected(uint16_t input_idx) override;
};