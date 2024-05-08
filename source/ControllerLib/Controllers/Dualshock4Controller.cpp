#include "Controllers/Dualshock4Controller.h"

Dualshock4Controller::Dualshock4Controller(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger))
{
}

Dualshock4Controller::~Dualshock4Controller()
{
}

ams::Result Dualshock4Controller::SendInitBytes()
{
    const uint8_t init_bytes[32] = {
        0x05, 0x07, 0x00, 0x00,
        0x00, 0x00,                                                             // initial strong and weak rumble
        GetConfig().ledColor.r, GetConfig().ledColor.g, GetConfig().ledColor.b, // LED color
        0x00, 0x00};

    R_RETURN(m_outPipe->Write(init_bytes, sizeof(init_bytes)));
}

ams::Result Dualshock4Controller::Initialize()
{
    R_TRY(BaseController::Initialize());

    R_TRY(SendInitBytes());

    R_SUCCEED();
}

ams::Result Dualshock4Controller::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
{
    uint8_t input_bytes[64];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe->Read(input_bytes, &size));

    if (input_bytes[0] == 0x01)
    {
        *input_idx = 0;

        Dualshock4USBButtonData *buttonData = reinterpret_cast<Dualshock4USBButtonData *>(input_bytes);

        normalData->triggers[0] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[0], buttonData->l2_pressure);
        normalData->triggers[1] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[1], buttonData->r2_pressure);

        NormalizeAxis(buttonData->stick_left_x, buttonData->stick_left_y, GetConfig().stickDeadzonePercent[0],
                      &normalData->sticks[0].axis_x, &normalData->sticks[0].axis_y, 0, 255);
        NormalizeAxis(buttonData->stick_right_x, buttonData->stick_right_y, GetConfig().stickDeadzonePercent[1],
                      &normalData->sticks[1].axis_x, &normalData->sticks[1].axis_y, 0, 255);

        bool buttons[MAX_CONTROLLER_BUTTONS] = {
            buttonData->triangle,
            buttonData->circle,
            buttonData->cross,
            buttonData->square,
            buttonData->l3,
            buttonData->r3,
            buttonData->l1,
            buttonData->r1,
            buttonData->l2,
            buttonData->r2,
            buttonData->share,
            buttonData->options,
            (buttonData->dpad == DS4_UP) || (buttonData->dpad == DS4_UPRIGHT) || (buttonData->dpad == DS4_UPLEFT),
            (buttonData->dpad == DS4_RIGHT) || (buttonData->dpad == DS4_UPRIGHT) || (buttonData->dpad == DS4_DOWNRIGHT),
            (buttonData->dpad == DS4_DOWN) || (buttonData->dpad == DS4_DOWNRIGHT) || (buttonData->dpad == DS4_DOWNLEFT),
            (buttonData->dpad == DS4_LEFT) || (buttonData->dpad == DS4_UPLEFT) || (buttonData->dpad == DS4_DOWNLEFT),
            buttonData->touchpad_press,
            buttonData->psbutton,
            false,
            buttonData->touchpad_finger1_unpressed == false,
        };

        for (int i = 0; i != MAX_CONTROLLER_BUTTONS; ++i)
        {
            ControllerButton button = GetConfig().buttons[i];
            if (button == NONE)
                continue;

            normalData->buttons[(button != DEFAULT ? button - 2 : i)] += buttons[i];
        }
    }

    R_SUCCEED();
}
