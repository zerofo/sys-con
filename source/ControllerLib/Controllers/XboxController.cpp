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

    R_TRY(m_inPipe->Read(input_bytes, &size));

    XboxButtonData *buttonData = reinterpret_cast<XboxButtonData *>(input_bytes);

    *input_idx = 0;

    normalData->triggers[0] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[0], buttonData->trigger_left);
    normalData->triggers[1] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[1], buttonData->trigger_right);

    NormalizeAxis(buttonData->stick_left_x, buttonData->stick_left_y, GetConfig().stickDeadzonePercent[0],
                  &normalData->sticks[0].axis_x, &normalData->sticks[0].axis_y, -32768, 32767);
    NormalizeAxis(buttonData->stick_right_x, buttonData->stick_right_y, GetConfig().stickDeadzonePercent[1],
                  &normalData->sticks[1].axis_x, &normalData->sticks[1].axis_y, -32768, 32767);

    bool buttons[MAX_CONTROLLER_BUTTONS]{
        buttonData->y != 0,
        buttonData->b != 0,
        buttonData->a != 0,
        buttonData->x != 0,
        buttonData->stick_left_click,
        buttonData->stick_right_click,
        false,
        false,
        buttonData->trigger_left != 0,
        buttonData->trigger_right != 0,
        buttonData->back,
        buttonData->start,
        buttonData->dpad_up,
        buttonData->dpad_right,
        buttonData->dpad_down,
        buttonData->dpad_left,
        buttonData->white_button != 0,
        buttonData->black_buttton != 0,
    };

    for (int i = 0; i != MAX_CONTROLLER_BUTTONS; ++i)
    {
        ControllerButton button = GetConfig().buttons[i];
        if (button == NONE)
            continue;

        normalData->buttons[(button != DEFAULT ? button - 2 : i)] += buttons[i];
    }

    R_SUCCEED();
}

ams::Result XboxController::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    uint8_t rumbleData[]{0x00, 0x06, 0x00, strong_magnitude, weak_magnitude, 0x00, 0x00, 0x00};
    R_RETURN(m_outPipe->Write(rumbleData, sizeof(rumbleData)));
}
