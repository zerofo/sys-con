#pragma once
#include <cstdint>

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
    RSTICK_CLICK,
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

    COUNT
};

struct NormalizedStick
{
    float axis_x;
    float axis_y;
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

struct ControllerConfig
{
    char driver[255]{0};
    char profile[255]{0};
    uint8_t stickDeadzonePercent[MAX_JOYSTICKS]{0};
    uint8_t triggerDeadzonePercent[MAX_TRIGGERS]{0};
    uint8_t buttons_pin[MAX_CONTROLLER_BUTTONS]{0};
    float triggers[MAX_TRIGGERS]{};
    NormalizedStick sticks[MAX_JOYSTICKS]{};
    RGBAColor bodyColor{0, 0, 0, 255};
    RGBAColor buttonsColor{0, 0, 0, 255};
    RGBAColor leftGripColor{0, 0, 0, 255};
    RGBAColor rightGripColor{0, 0, 0, 255};
};