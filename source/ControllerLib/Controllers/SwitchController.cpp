#include "Controllers/SwitchController.h"

SwitchController::SwitchController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger))
{
}

SwitchController::~SwitchController()
{
}

ControllerResult SwitchController::Initialize()
{
    ControllerResult result = BaseController::Initialize();
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    if (m_outPipe.size() < 1)
    {
        Log(LogLevelError, "SwitchController: Initialization not complete ! No output endpoint found !");
        return CONTROLLER_STATUS_INVALID_ENDPOINT;
    }

    uint8_t initPacket1[64]{0x80, 0x02};
    m_outPipe[0]->Write(initPacket1, sizeof(initPacket1));

    // Forces the Joy-Con or Pro Controller to only talk over USB HID without any timeouts.
    uint8_t initPacket2_ForceToUSB[64]{0x80, 0x04};
    m_outPipe[0]->Write(initPacket2_ForceToUSB, sizeof(initPacket2_ForceToUSB));

    return CONTROLLER_STATUS_SUCCESS;
}

ControllerResult SwitchController::ReadRawInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us)
{
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = sizeof(input_bytes);

    static_assert(sizeof(input_bytes) == 64, "Input byte for switch as to be 64 bytes long");

    ControllerResult result = m_inPipe[0]->Read(input_bytes, &size, timeout_us);
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    SwitchButtonData *buttonData = reinterpret_cast<SwitchButtonData *>(input_bytes);

    *input_idx = 0;

    if (buttonData->report_id != 0x30)
        return CONTROLLER_STATUS_NOTHING_TODO;

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
    rawData->buttons[14] = buttonData->button14;
    rawData->buttons[15] = buttonData->button15;
    rawData->buttons[16] = buttonData->button16;
    rawData->buttons[17] = buttonData->button17;
    rawData->buttons[18] = buttonData->button18;
    rawData->buttons[19] = buttonData->button19;

    /*
    rawData->X = BaseController::Normalize(buttonData->stick_left_x, -32768, 32767);
    rawData->Y = BaseController::Normalize(-buttonData->stick_left_y, -32768, 32767);
    rawData->Z = BaseController::Normalize(buttonData->stick_right_x, -32768, 32767);
    rawData->Rz = BaseController::Normalize(-buttonData->stick_right_y, -32768, 32767);
    */
    rawData->dpad_up = buttonData->dpad_up;
    rawData->dpad_right = buttonData->dpad_right;
    rawData->dpad_down = buttonData->dpad_down;
    rawData->dpad_left = buttonData->dpad_left;

    return CONTROLLER_STATUS_SUCCESS;
}

bool SwitchController::Support(ControllerFeature feature)
{
    (void)feature;
    return false;
}
