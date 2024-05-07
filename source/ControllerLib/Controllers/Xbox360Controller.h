#pragma once

#include "IController.h"
#include <vector>
// References used:
// https://cs.chromium.org/chromium/src/device/gamepad/xbox_controller_mac.mm

struct Xbox360ButtonData
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

    bool bumper_left : 1;
    bool bumper_right : 1;
    bool guide : 1;
    bool dummy1 : 1; // Always 0.

    bool a : 1;
    bool b : 1;
    bool x : 1;
    bool y : 1;

    uint8_t trigger_left;
    uint8_t trigger_right;

    int16_t stick_left_x;
    int16_t stick_left_y;
    int16_t stick_right_x;
    int16_t stick_right_y;

    // Always 0.
    uint32_t dummy2;
    uint16_t dummy3;
};

struct Xbox360RumbleData
{
    uint8_t command;
    uint8_t size;
    uint8_t dummy1;
    uint8_t big;
    uint8_t little;
    uint8_t dummy2[3];
};

enum Xbox360InputPacketType : uint8_t
{
    XBOX360INPUT_BUTTON = 0,
    XBOX360INPUT_LED = 1,
    XBOX360INPUT_RUMBLE = 3,
};

enum Xbox360LEDValue : uint8_t
{
    XBOX360LED_OFF,
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

class Xbox360Controller : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;
    bool m_is_wireless = false;
    bool m_is_present = false;
    std::vector<OutputPacket> m_outputBuffer;
    Xbox360ButtonData m_buttonData{};

public:
    Xbox360Controller(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger);
    virtual ~Xbox360Controller() override;

    virtual ams::Result Initialize() override;
    virtual void Exit() override;

    ams::Result OpenInterfaces();
    void CloseInterfaces();

    virtual ams::Result GetInput() override;

    virtual NormalizedButtonData GetNormalizedButtonData() override;

    virtual bool Support(ControllerFeature feature) override;

    ams::Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    ams::Result SetLED(Xbox360LEDValue value);

    ams::Result OnControllerConnect();
    ams::Result OnControllerDisconnect();

    virtual ams::Result OutputBuffer() override;

    bool IsControllerActive() override { return m_is_present; }
};