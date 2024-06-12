#include "Controllers/XboxController.h"

XboxController::XboxController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger))
{
}

XboxController::~XboxController()
{
}

ams::Result XboxController::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
{
    uint8_t input_bytes[64];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe[0]->Read(input_bytes, &size, UINT64_MAX));

    XboxButtonData *buttonData = reinterpret_cast<XboxButtonData *>(input_bytes);

    *input_idx = 0;

    bool buttons_mapping[MAX_CONTROLLER_BUTTONS]{
        false,
        buttonData->button1 > 0,
        buttonData->button2 > 0,
        buttonData->button3 > 0,
        buttonData->button4 > 0,
        buttonData->button5 > 0,
        buttonData->button6 > 0,
        buttonData->button7,
        buttonData->button8,
        buttonData->button9,
        buttonData->button10};

    normalData->triggers[0] = Normalize(GetConfig().triggerDeadzonePercent[0], buttonData->trigger_left, 0, 255);
    normalData->triggers[1] = Normalize(GetConfig().triggerDeadzonePercent[1], buttonData->trigger_right, 0, 255);

    normalData->sticks[0].axis_x = Normalize(GetConfig().stickDeadzonePercent[0], buttonData->stick_left_x, -32768, 32767);
    normalData->sticks[0].axis_y = Normalize(GetConfig().stickDeadzonePercent[0], -buttonData->stick_left_y, -32768, 32767);
    normalData->sticks[1].axis_x = Normalize(GetConfig().stickDeadzonePercent[1], buttonData->stick_right_x, -32768, 32767);
    normalData->sticks[1].axis_y = Normalize(GetConfig().stickDeadzonePercent[1], -buttonData->stick_right_y, -32768, 32767);

    normalData->buttons[ControllerButton::X] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::X]] ? true : false;
    normalData->buttons[ControllerButton::A] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::A]] ? true : false;
    normalData->buttons[ControllerButton::B] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::B]] ? true : false;
    normalData->buttons[ControllerButton::Y] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::Y]] ? true : false;
    normalData->buttons[ControllerButton::LSTICK_CLICK] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::LSTICK_CLICK]] ? true : false;
    normalData->buttons[ControllerButton::RSTICK_CLICK] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::RSTICK_CLICK]] ? true : false;
    normalData->buttons[ControllerButton::L] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::L]] ? true : false;
    normalData->buttons[ControllerButton::R] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::R]] ? true : false;

    normalData->buttons[ControllerButton::ZL] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::ZL]] ? true : false;
    normalData->buttons[ControllerButton::ZR] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::ZR]] ? true : false;

    if (GetConfig().buttons_pin[ControllerButton::ZL] == 0)
        normalData->buttons[ControllerButton::ZL] = normalData->triggers[0] > 0;
    if (GetConfig().buttons_pin[ControllerButton::ZR] == 0)
        normalData->buttons[ControllerButton::ZR] = normalData->triggers[1] > 0;

    normalData->buttons[ControllerButton::MINUS] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::MINUS]] ? true : false;
    normalData->buttons[ControllerButton::PLUS] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::PLUS]] ? true : false;
    normalData->buttons[ControllerButton::CAPTURE] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::CAPTURE]] ? true : false;
    normalData->buttons[ControllerButton::HOME] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::HOME]] ? true : false;

    normalData->buttons[ControllerButton::DPAD_UP] = buttonData->dpad_up;
    normalData->buttons[ControllerButton::DPAD_RIGHT] = buttonData->dpad_right;
    normalData->buttons[ControllerButton::DPAD_DOWN] = buttonData->dpad_down;
    normalData->buttons[ControllerButton::DPAD_LEFT] = buttonData->dpad_left;

    R_SUCCEED();
}

bool XboxController::Support(ControllerFeature feature)
{
    if (feature == SUPPORTS_RUMBLE)
        return true;

    return false;
}

ams::Result XboxController::SetRumble(uint16_t input_idx, float amp_high, float amp_low)
{
    (void)input_idx;
    uint8_t rumbleData[]{0x00, 0x06, 0x00, (uint8_t)(amp_high * 255), (uint8_t)(amp_low * 255), 0x00, 0x00, 0x00};
    R_RETURN(m_outPipe[input_idx]->Write(rumbleData, sizeof(rumbleData)));
}
