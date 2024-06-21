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

ams::Result Dualshock3Controller::ReadInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us)
{
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe[0]->Read(input_bytes, &size, timeout_us));

    *input_idx = 0;

    if (input_bytes[0] == Ds3InputPacket_Button)
    {
        Dualshock3ButtonData *buttonData = reinterpret_cast<Dualshock3ButtonData *>(input_bytes);

        rawData->buttons[1] = buttonData->button1;
        rawData->buttons[2] = buttonData->button2;
        rawData->buttons[3] = buttonData->button3;
        rawData->buttons[4] = buttonData->button4;
        rawData->buttons[5] = buttonData->button5;
        rawData->buttons[6] = buttonData->button6;
        rawData->buttons[7] = buttonData->button7;
        rawData->buttons[8] = buttonData->button8;
        rawData->buttons[9] = buttonData->button9;
        rawData->buttons[10] = buttonData->button10;
        rawData->buttons[11] = buttonData->button11;
        rawData->buttons[12] = buttonData->button12;
        rawData->buttons[13] = buttonData->button13;

        rawData->Rx = Normalize(buttonData->Rx, 0, 255);
        rawData->Ry = Normalize(buttonData->Ry, 0, 255);

        rawData->X = Normalize(buttonData->X, 0, 255);
        rawData->Y = Normalize(buttonData->Y, 0, 255);
        rawData->Z = Normalize(buttonData->Z, 0, 255);
        rawData->Rz = Normalize(buttonData->Rz, 0, 255);

        rawData->dpad_up = buttonData->dpad_up;
        rawData->dpad_right = buttonData->dpad_right;
        rawData->dpad_down = buttonData->dpad_down;
        rawData->dpad_left = buttonData->dpad_left;

        R_SUCCEED();
    }

    R_RETURN(CONTROL_ERR_UNEXPECTED_DATA);
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