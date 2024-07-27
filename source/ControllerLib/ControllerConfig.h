#pragma once
#include <cstdint>
#include <string>

#define MAX_JOYSTICKS          2
#define MAX_TRIGGERS           2
#define MAX_CONTROLLER_BUTTONS 32

enum ControllerButton
{
    X = 0,
    A,
    B,
    Y,
    LSTICK_CLICK,
    LSTICK_LEFT,
    LSTICK_RIGHT,
    LSTICK_UP,
    LSTICK_DOWN,
    RSTICK_CLICK,
    RSTICK_LEFT,
    RSTICK_RIGHT,
    RSTICK_UP,
    RSTICK_DOWN,
    L,
    R,
    ZL,
    ZR,
    MINUS,
    PLUS,
    DPAD_UP,
    DPAD_RIGHT,
    DPAD_DOWN,
    DPAD_LEFT,
    CAPTURE,
    HOME,

    COUNT,
    NONE
};

union RGBAColor
{
    struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    uint8_t values[4];
    uint32_t rgbaValue;
};

enum ControllerAnalogBinding
{
    ControllerAnalogBinding_Unknown = 0,
    ControllerAnalogBinding_X,
    ControllerAnalogBinding_Y,
    ControllerAnalogBinding_Z,
    ControllerAnalogBinding_RZ,
    ControllerAnalogBinding_RX,
    ControllerAnalogBinding_RY,

    ControllerAnalogBinding_Count
};

enum ControllerType
{
    ControllerType_Unknown = 0,
    ControllerType_Pro,
    ControllerType_ProWithBattery,
    ControllerType_Tarragon,
    ControllerType_Snes,
    ControllerType_PokeballPlus,
    ControllerType_Gamecube,
    ControllerType_3rdPartyPro,
    ControllerType_N64,
    ControllerType_Sega,
    ControllerType_Nes,
    ControllerType_Famicom
};

struct ControllerAnalogConfig
{
    float sign{1.0};
    ControllerAnalogBinding bind{ControllerAnalogBinding::ControllerAnalogBinding_Unknown};
};

struct ControllerStickConfig
{
    ControllerAnalogConfig X;
    ControllerAnalogConfig Y;
};

class ControllerConfig
{
public:
    std::string driver;
    std::string profile;
    ControllerType controllerType{ControllerType_Pro};

    uint8_t stickDeadzonePercent[MAX_JOYSTICKS]{0};
    uint8_t triggerDeadzonePercent[MAX_TRIGGERS]{0};
    uint8_t buttons_pin[MAX_CONTROLLER_BUTTONS]{0};

    ControllerStickConfig stickConfig[MAX_JOYSTICKS];
    ControllerAnalogConfig triggerConfig[MAX_TRIGGERS];

    ControllerButton simulateHome[2]{ControllerButton::NONE};
    ControllerButton simulateCapture[2]{ControllerButton::NONE};

    RGBAColor bodyColor{0, 0, 0, 255};
    RGBAColor buttonsColor{0, 0, 0, 255};
    RGBAColor leftGripColor{0, 0, 0, 255};
    RGBAColor rightGripColor{0, 0, 0, 255};
};