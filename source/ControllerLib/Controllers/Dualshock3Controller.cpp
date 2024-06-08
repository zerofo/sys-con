#include "Controllers/Dualshock3Controller.h"

#define LED_PERMANENT 0xff, 0x27, 0x00, 0x00, 0x32

Dualshock3Controller::Dualshock3Controller(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger))
{
}

Dualshock3Controller::~Dualshock3Controller()
{
}

ams::Result Dualshock3Controller::Initialize()
{
    R_TRY(BaseController::Initialize());

    SetLED(DS3LED_1);

    R_SUCCEED();
}

ams::Result Dualshock3Controller::OpenInterfaces()
{
    R_TRY(BaseController::OpenInterfaces());

    constexpr uint8_t initBytes[] = {0x42, 0x0C, 0x00, 0x00};
    R_TRY(SendCommand(Ds3FeatureStartDevice, initBytes, sizeof(initBytes)));

    R_SUCCEED();
}

ams::Result Dualshock3Controller::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
{
    uint8_t input_bytes[64];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe[0]->Read(input_bytes, &size, IUSBEndpoint::USB_MODE_BLOCKING));

    if (input_bytes[0] == Ds3InputPacket_Button)
    {
        Dualshock3ButtonData *buttonData = reinterpret_cast<Dualshock3ButtonData *>(input_bytes);

        *input_idx = 0;

        normalData->triggers[0] = Normalize(GetConfig().triggerDeadzonePercent[0], buttonData->trigger_left_pressure, 0, 255);
        normalData->triggers[1] = Normalize(GetConfig().triggerDeadzonePercent[1], buttonData->trigger_right_pressure, 0, 255);

        normalData->sticks[0].axis_x = Normalize(GetConfig().stickDeadzonePercent[0], buttonData->stick_left_x, 0, 255);
        normalData->sticks[0].axis_y = Normalize(GetConfig().stickDeadzonePercent[0], buttonData->stick_left_y, 0, 255);
        normalData->sticks[1].axis_x = Normalize(GetConfig().stickDeadzonePercent[1], buttonData->stick_right_x, 0, 255);
        normalData->sticks[1].axis_y = Normalize(GetConfig().stickDeadzonePercent[1], buttonData->stick_right_y, 0, 255);

        normalData->buttons[ControllerButton::A] = buttonData->circle;
        normalData->buttons[ControllerButton::B] = buttonData->cross;
        normalData->buttons[ControllerButton::X] = buttonData->triangle;
        normalData->buttons[ControllerButton::Y] = buttonData->square;
        normalData->buttons[ControllerButton::L] = buttonData->bumper_left;
        normalData->buttons[ControllerButton::R] = buttonData->bumper_right;
        normalData->buttons[ControllerButton::ZL] = normalData->triggers[0] > 0 ? true : false;
        normalData->buttons[ControllerButton::ZR] = normalData->triggers[1] > 0 ? true : false;
        normalData->buttons[ControllerButton::MINUS] = buttonData->back;
        normalData->buttons[ControllerButton::PLUS] = buttonData->start;
        normalData->buttons[ControllerButton::DPAD_UP] = buttonData->dpad_up;
        normalData->buttons[ControllerButton::DPAD_RIGHT] = buttonData->dpad_right;
        normalData->buttons[ControllerButton::DPAD_DOWN] = buttonData->dpad_down;
        normalData->buttons[ControllerButton::DPAD_LEFT] = buttonData->dpad_left;
        normalData->buttons[ControllerButton::CAPTURE] = false;
        normalData->buttons[ControllerButton::HOME] = buttonData->guide;
    }

    R_SUCCEED();
}

ams::Result Dualshock3Controller::SendCommand(Dualshock3FeatureValue feature, const void *buffer, uint16_t size)
{
    R_RETURN(m_interfaces[0]->ControlTransferOutput(0x21, 0x09, static_cast<uint16_t>(feature), 0, buffer, size));
}

ams::Result Dualshock3Controller::SetLED(Dualshock3LEDValue value)
{
    const uint8_t ledPacket[]{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        static_cast<uint8_t>(value << 1),
        LED_PERMANENT,
        LED_PERMANENT,
        LED_PERMANENT,
        LED_PERMANENT};
    R_RETURN(SendCommand(Ds3FeatureUnknown1, ledPacket, sizeof(ledPacket)));
}