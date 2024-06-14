#include "Controllers/Xbox360Controller.h"

// https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/

static constexpr uint8_t reconnectPacket[]{0x08, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t poweroffPacket[]{0x00, 0x00, 0x08, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t initDriverPacket[]{0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

Xbox360Controller::Xbox360Controller(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger, bool is_wireless)
    : BaseController(std::move(device), config, std::move(logger))
{
    m_is_wireless = is_wireless;

    for (int i = 0; i < XBOX360_MAX_INPUTS; i++)
        m_is_connected[i] = false;
}

Xbox360Controller::~Xbox360Controller()
{
}

ams::Result Xbox360Controller::Initialize()
{
    R_TRY(BaseController::Initialize());

    // Duplicated with OnControllerConnect
    if (!m_is_wireless)
    {
        m_is_connected[0] = true;
        SetLED(0, XBOX360LED_TOPLEFT);
    }

    R_SUCCEED();
}

ams::Result Xbox360Controller::OpenInterfaces()
{
    R_TRY(BaseController::OpenInterfaces());

    if (m_inPipe.size() < GetInputCount())
    {
        LogPrint(LogLevelError, "Xbox360Controller: Not enough input endpoints (%d / %d)", m_inPipe.size(), GetInputCount());
        R_RETURN(CONTROL_ERR_INVALID_ENDPOINT);
    }

    R_SUCCEED();
}

void Xbox360Controller::CloseInterfaces()
{
    if (m_is_wireless)
    {
        for (int i = 0; i < XBOX360_MAX_INPUTS; i++)
        {
            if (m_is_connected[i])
                OnControllerDisconnect(i);
        }
    }

    BaseController::CloseInterfaces();
}

ams::Result Xbox360Controller::ReadInput(RawInputData *rawData, uint16_t *input_idx)
{
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = sizeof(input_bytes);

    uint16_t controller_idx = m_current_controller_idx;
    m_current_controller_idx = (m_current_controller_idx + 1) % GetInputCount();

    R_TRY(m_inPipe[controller_idx]->Read(input_bytes, &size, 1 /*TimoutUs*/));

    Xbox360ButtonData *buttonData = reinterpret_cast<Xbox360ButtonData *>(input_bytes);

    *input_idx = controller_idx;

    if (m_is_wireless)
    {
        // https://github.com/xboxdrv/xboxdrv/blob/stable/src/xbox360_controller.cpp
        // https://github.com/felis/USB_Host_Shield_2.0/blob/master/XBOXRECV.cpp
        if (input_bytes[0] & 0x08) // Connect/Disconnect
        {
            bool is_connected = (input_bytes[1] & 0x80) != 0;

            if (m_is_connected[controller_idx] != is_connected)
            {
                if (is_connected)
                    OnControllerConnect(controller_idx);
                else
                    OnControllerDisconnect(controller_idx);
            }

            R_RETURN(CONTROL_ERR_NOTHING_TODO);
        }
        else if (input_bytes[0] == 0x00 && input_bytes[1] == 0x0f && input_bytes[2] == 0x00 && input_bytes[3] == 0xf0)
        {
            // Initial Announc Message
            R_RETURN(CONTROL_ERR_NOTHING_TODO);
        }
        else if (input_bytes[0] == 0x00 && input_bytes[1] == 0x00 && input_bytes[2] == 0x00 && input_bytes[3] == 0x13)
        {
            // Battery status //  BatteryStatus = input_bytes[4];
            R_RETURN(CONTROL_ERR_NOTHING_TODO);
        }
        else if (input_bytes[0] == 0x00 && input_bytes[1] == 0x00 && input_bytes[2] == 0x00 && input_bytes[3] == 0xf0)
        {
            // Sent after each button pressed
            R_RETURN(CONTROL_ERR_NOTHING_TODO);
        }
        else if (input_bytes[0] == 0x00 && input_bytes[1] == 0x01 && input_bytes[2] == 0x00 && input_bytes[3] == 0xf0)
        {
            // Data
        }

        buttonData = reinterpret_cast<Xbox360ButtonData *>(input_bytes + 4);
    }

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

        rawData->Rx = Normalize(buttonData->Rx, 0, 255);
        rawData->Ry = Normalize(buttonData->Ry, 0, 255);

        rawData->X = Normalize(buttonData->X, -32768, 32767);
        rawData->Y = Normalize(-buttonData->Y, -32768, 32767);
        rawData->Z = Normalize(buttonData->Z, -32768, 32767);
        rawData->Rz = Normalize(-buttonData->Rz, -32768, 32767);

        rawData->dpad_up = buttonData->dpad_up;
        rawData->dpad_right = buttonData->dpad_right;
        rawData->dpad_down = buttonData->dpad_down;
        rawData->dpad_left = buttonData->dpad_left;

        R_SUCCEED();
    }

    R_RETURN(CONTROL_ERR_UNEXPECTED_DATA);
}

bool Xbox360Controller::Support(ControllerFeature feature)
{
    if (feature == SUPPORTS_RUMBLE)
        return true;

    return false;
}

uint16_t Xbox360Controller::GetInputCount()
{
    return m_is_wireless ? XBOX360_MAX_INPUTS : 1;
}

ams::Result Xbox360Controller::SetRumble(uint16_t input_idx, float amp_high, float amp_low)
{
    if (m_is_wireless)
    {
        uint8_t rumbleData[]{0x00, (uint8_t)(input_idx + 1), 0x0F, 0xC0, 0x00, (uint8_t)(amp_high * 255), (uint8_t)(amp_low * 255), 0x00, 0x00, 0x00, 0x00, 0x00};
        R_RETURN(m_outPipe[input_idx]->Write(rumbleData, sizeof(rumbleData)));
    }
    else
    {
        uint8_t rumbleData[]{0x00, 0x08, 0x00, (uint8_t)(amp_high * 255), (uint8_t)(amp_low * 255), 0x00, 0x00, 0x00};
        R_RETURN(m_outPipe[input_idx]->Write(rumbleData, sizeof(rumbleData)));
    }
}

bool Xbox360Controller::IsControllerConnected(uint16_t input_idx)
{
    return m_is_connected[input_idx];
}

ams::Result Xbox360Controller::SetLED(uint16_t input_idx, Xbox360LEDValue value)
{
    if (m_is_wireless)
    {
        uint8_t ledPacket[]{0x00, (uint8_t)(input_idx + 1), 0x08, (uint8_t)(value | 0x40), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        R_RETURN(m_outPipe[input_idx]->Write(ledPacket, sizeof(ledPacket)));
    }
    else
    {
        uint8_t ledPacket[]{0x01, 0x03, (uint8_t)(value)};
        R_RETURN(m_outPipe[input_idx]->Write(ledPacket, sizeof(ledPacket)));
    }
}

ams::Result Xbox360Controller::OnControllerConnect(uint16_t input_idx)
{
    LogPrint(LogLevelInfo, "Xbox360Controller Wireless controller connected (Idx: %d) ...", input_idx);

    m_outPipe[input_idx]->Write(reconnectPacket, sizeof(reconnectPacket));
    m_outPipe[input_idx]->Write(initDriverPacket, sizeof(initDriverPacket));

    SetLED(input_idx, (Xbox360LEDValue)((int)XBOX360LED_TOPLEFT + input_idx));

    m_is_connected[input_idx] = true;

    R_SUCCEED();
}

ams::Result Xbox360Controller::OnControllerDisconnect(uint16_t input_idx)
{
    LogPrint(LogLevelInfo, "Xbox360Controller Wireless controller disconnected (Idx: %d) ...", input_idx);

    m_outPipe[input_idx]->Write(poweroffPacket, sizeof(poweroffPacket));

    m_is_connected[input_idx] = false;

    R_SUCCEED();
}
