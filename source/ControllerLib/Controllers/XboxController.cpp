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

    R_TRY(m_inPipe[0]->Read(input_bytes, &size, IUSBEndpoint::USB_MODE_BLOCKING));

    XboxButtonData *buttonData = reinterpret_cast<XboxButtonData *>(input_bytes);

    *input_idx = 0;

    normalData->triggers[0] = Normalize(GetConfig().triggerDeadzonePercent[0], buttonData->trigger_left, 0, 255);
    normalData->triggers[1] = Normalize(GetConfig().triggerDeadzonePercent[1], buttonData->trigger_right, 0, 255);

    normalData->sticks[0].axis_x = Normalize(GetConfig().stickDeadzonePercent[0], buttonData->stick_left_x, -32768, 32767);
    normalData->sticks[0].axis_y = Normalize(GetConfig().stickDeadzonePercent[0], -buttonData->stick_left_y, -32768, 32767);
    normalData->sticks[1].axis_x = Normalize(GetConfig().stickDeadzonePercent[1], buttonData->stick_right_x, -32768, 32767);
    normalData->sticks[1].axis_y = Normalize(GetConfig().stickDeadzonePercent[1], -buttonData->stick_right_y, -32768, 32767);

    normalData->buttons[ControllerButton::X] = buttonData->y > 0;
    normalData->buttons[ControllerButton::A] = buttonData->b > 0;
    normalData->buttons[ControllerButton::B] = buttonData->a > 0;
    normalData->buttons[ControllerButton::Y] = buttonData->x > 0;
    normalData->buttons[ControllerButton::LSTICK_CLICK] = buttonData->stick_left_click;
    normalData->buttons[ControllerButton::RSTICK_CLICK] = buttonData->stick_right_click;
    normalData->buttons[ControllerButton::L] = false;
    normalData->buttons[ControllerButton::R] = false;
    normalData->buttons[ControllerButton::ZL] = buttonData->trigger_left > 0;
    normalData->buttons[ControllerButton::ZR] = buttonData->trigger_right > 0;
    normalData->buttons[ControllerButton::MINUS] = buttonData->back;
    normalData->buttons[ControllerButton::PLUS] = buttonData->start;
    normalData->buttons[ControllerButton::DPAD_UP] = buttonData->dpad_up;
    normalData->buttons[ControllerButton::DPAD_RIGHT] = buttonData->dpad_right;
    normalData->buttons[ControllerButton::DPAD_DOWN] = buttonData->dpad_down;
    normalData->buttons[ControllerButton::DPAD_LEFT] = buttonData->dpad_left;
    normalData->buttons[ControllerButton::CAPTURE] = buttonData->white_button;
    normalData->buttons[ControllerButton::HOME] = buttonData->black_button;

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
