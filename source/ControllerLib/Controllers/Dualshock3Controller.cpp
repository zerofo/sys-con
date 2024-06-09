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

        bool buttons_mapping[MAX_CONTROLLER_BUTTONS]{
            false,
            buttonData->button1,
            buttonData->button2,
            buttonData->button3,
            buttonData->button4,
            buttonData->button5,
            buttonData->button6,
            buttonData->button7,
            buttonData->button8,
            buttonData->button9,
            buttonData->button10,
            buttonData->button11,
            buttonData->button12};

        *input_idx = 0;

        normalData->triggers[0] = Normalize(GetConfig().triggerDeadzonePercent[0], buttonData->trigger_left_pressure, 0, 255);
        normalData->triggers[1] = Normalize(GetConfig().triggerDeadzonePercent[1], buttonData->trigger_right_pressure, 0, 255);

        normalData->sticks[0].axis_x = Normalize(GetConfig().stickDeadzonePercent[0], buttonData->stick_left_x, 0, 255);
        normalData->sticks[0].axis_y = Normalize(GetConfig().stickDeadzonePercent[0], buttonData->stick_left_y, 0, 255);
        normalData->sticks[1].axis_x = Normalize(GetConfig().stickDeadzonePercent[1], buttonData->stick_right_x, 0, 255);
        normalData->sticks[1].axis_y = Normalize(GetConfig().stickDeadzonePercent[1], buttonData->stick_right_y, 0, 255);

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