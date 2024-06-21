#include "Controllers/XboxController.h"

XboxController::XboxController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger))
{
}

XboxController::~XboxController()
{
}

ams::Result XboxController::ReadInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us)
{
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe[0]->Read(input_bytes, &size, timeout_us));

    XboxButtonData *buttonData = reinterpret_cast<XboxButtonData *>(input_bytes);

    *input_idx = 0;

    rawData->buttons[1] = buttonData->button1 > 0;
    rawData->buttons[2] = buttonData->button2 > 0;
    rawData->buttons[3] = buttonData->button3 > 0;
    rawData->buttons[4] = buttonData->button4 > 0;
    rawData->buttons[5] = buttonData->button5 > 0;
    rawData->buttons[6] = buttonData->button6 > 0;
    rawData->buttons[7] = buttonData->button7;
    rawData->buttons[8] = buttonData->button8;
    rawData->buttons[9] = buttonData->button9;
    rawData->buttons[10] = buttonData->button10;

    rawData->Rx = Normalize(buttonData->trigger_left, 0, 255);
    rawData->Ry = Normalize(buttonData->trigger_right, 0, 255);

    rawData->X = Normalize(buttonData->stick_left_x, -32768, 32767);
    rawData->Y = Normalize(-buttonData->stick_left_y, -32768, 32767);
    rawData->Z = Normalize(buttonData->stick_right_x, -32768, 32767);
    rawData->Rz = Normalize(-buttonData->stick_right_y, -32768, 32767);

    rawData->dpad_up = false;
    rawData->dpad_right = false;
    rawData->dpad_down = false;
    rawData->dpad_left = false;

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
