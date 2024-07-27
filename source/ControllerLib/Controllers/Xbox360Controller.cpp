#include "Controllers/Xbox360Controller.h"

// https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/

Xbox360Controller::Xbox360Controller(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger))
{
}

Xbox360Controller::~Xbox360Controller()
{
}

ControllerResult Xbox360Controller::Initialize()
{
    ControllerResult result = BaseController::Initialize();
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    SetLED(0, XBOX360LED_TOPLEFT);

    return CONTROLLER_STATUS_SUCCESS;
}

ControllerResult Xbox360Controller::ReadInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us)
{
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = sizeof(input_bytes);

    ControllerResult result = m_inPipe[0]->Read(input_bytes, &size, timeout_us);
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    Xbox360ButtonData *buttonData = reinterpret_cast<Xbox360ButtonData *>(input_bytes);

    *input_idx = 0;

    if (buttonData->type == XBOX360INPUT_BUTTON) // Button data
    {

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

        rawData->Rx = BaseController::Normalize(buttonData->Rx, 0, 255);
        rawData->Ry = BaseController::Normalize(buttonData->Ry, 0, 255);

        rawData->X = BaseController::Normalize(buttonData->X, -32768, 32767);
        rawData->Y = BaseController::Normalize(-buttonData->Y, -32768, 32767);
        rawData->Z = BaseController::Normalize(buttonData->Z, -32768, 32767);
        rawData->Rz = BaseController::Normalize(-buttonData->Rz, -32768, 32767);

        rawData->dpad_up = buttonData->dpad_up;
        rawData->dpad_right = buttonData->dpad_right;
        rawData->dpad_down = buttonData->dpad_down;
        rawData->dpad_left = buttonData->dpad_left;

        return CONTROLLER_STATUS_SUCCESS;
    }

    return CONTROLLER_STATUS_UNEXPECTED_DATA;
}

bool Xbox360Controller::Support(ControllerFeature feature)
{
    if (feature == SUPPORTS_RUMBLE)
        return true;

    return false;
}

ControllerResult Xbox360Controller::SetRumble(uint16_t input_idx, float amp_high, float amp_low)
{
    uint8_t rumbleData[]{0x00, 0x08, 0x00, (uint8_t)(amp_high * 255), (uint8_t)(amp_low * 255), 0x00, 0x00, 0x00};
    return m_outPipe[input_idx]->Write(rumbleData, sizeof(rumbleData));
}

ControllerResult Xbox360Controller::SetLED(uint16_t input_idx, Xbox360LEDValue value)
{
    uint8_t ledPacket[]{0x01, 0x03, (uint8_t)(value)};
    return m_outPipe[input_idx]->Write(ledPacket, sizeof(ledPacket));
}
