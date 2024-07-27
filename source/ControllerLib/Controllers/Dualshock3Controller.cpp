#include "Controllers/Dualshock3Controller.h"

#define LED_PERMANENT 0xff, 0x27, 0x00, 0x00, 0x32

Dualshock3Controller::Dualshock3Controller(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger))
{
}

Dualshock3Controller::~Dualshock3Controller()
{
}

ControllerResult Dualshock3Controller::Initialize()
{
    ControllerResult result = BaseController::Initialize();
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    SetLED(DS3LED_1);

    return CONTROLLER_STATUS_SUCCESS;
}

ControllerResult Dualshock3Controller::OpenInterfaces()
{
    ControllerResult result = BaseController::OpenInterfaces();
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    constexpr uint8_t initBytes[] = {0x42, 0x0C, 0x00, 0x00};
    result = SendCommand(Ds3FeatureStartDevice, initBytes, sizeof(initBytes));
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    return CONTROLLER_STATUS_SUCCESS;
}

ControllerResult Dualshock3Controller::ReadInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us)
{
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = sizeof(input_bytes);

    ControllerResult result = m_inPipe[0]->Read(input_bytes, &size, timeout_us);
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

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

        rawData->Rx = BaseController::Normalize(buttonData->Rx, 0, 255);
        rawData->Ry = BaseController::Normalize(buttonData->Ry, 0, 255);

        rawData->X = BaseController::Normalize(buttonData->X, 0, 255);
        rawData->Y = BaseController::Normalize(buttonData->Y, 0, 255);
        rawData->Z = BaseController::Normalize(buttonData->Z, 0, 255);
        rawData->Rz = BaseController::Normalize(buttonData->Rz, 0, 255);

        rawData->dpad_up = buttonData->dpad_up;
        rawData->dpad_right = buttonData->dpad_right;
        rawData->dpad_down = buttonData->dpad_down;
        rawData->dpad_left = buttonData->dpad_left;

        return CONTROLLER_STATUS_SUCCESS;
    }

    return CONTROLLER_STATUS_UNEXPECTED_DATA;
}

ControllerResult Dualshock3Controller::SendCommand(Dualshock3FeatureValue feature, const void *buffer, uint16_t size)
{
    return m_interfaces[0]->ControlTransferOutput(0x21, 0x09, static_cast<uint16_t>(feature), 0, buffer, size);
}

ControllerResult Dualshock3Controller::SetLED(Dualshock3LEDValue value)
{
    const uint8_t ledPacket[]{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        static_cast<uint8_t>(value << 1),
        LED_PERMANENT,
        LED_PERMANENT,
        LED_PERMANENT,
        LED_PERMANENT};
    return SendCommand(Ds3FeatureUnknown1, ledPacket, sizeof(ledPacket));
}