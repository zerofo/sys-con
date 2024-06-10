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

ams::Result Xbox360Controller::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
{
    uint8_t input_bytes[64];
    size_t size = sizeof(input_bytes);

    uint16_t controller_idx = m_current_controller_idx;
    m_current_controller_idx = (m_current_controller_idx + 1) % GetInputCount();

    R_TRY(m_inPipe[controller_idx]->Read(input_bytes, &size, IUSBEndpoint::USB_MODE_NON_BLOCKING));

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
            buttonData->button11};

        LogPrint(LogLevelDebug, "Xbox360Controller DATA: X=%d, Y=%d, Z=%d, Rz=%d, B1=%d, B2=%d, B3=%d, B4=%d, B5=%d, B6=%d, B7=%d, B8=%d, B9=%d, B10=%d",
                 buttonData->X, buttonData->Y, buttonData->Z, buttonData->Rz,
                 buttons_mapping[1],
                 buttons_mapping[2] ? 1 : 0,
                 buttons_mapping[3] ? 1 : 0,
                 buttons_mapping[4] ? 1 : 0,
                 buttons_mapping[5] ? 1 : 0,
                 buttons_mapping[6] ? 1 : 0,
                 buttons_mapping[7] ? 1 : 0,
                 buttons_mapping[8] ? 1 : 0,
                 buttons_mapping[9] ? 1 : 0,
                 buttons_mapping[10] ? 1 : 0);

        normalData->triggers[0] = Normalize(GetConfig().triggerDeadzonePercent[0], buttonData->Z, 0, 255);
        normalData->triggers[1] = Normalize(GetConfig().triggerDeadzonePercent[1], buttonData->Rz, 0, 255);

        normalData->sticks[0].axis_x = Normalize(GetConfig().stickDeadzonePercent[0], buttonData->X, -32768, 32767);
        normalData->sticks[0].axis_y = Normalize(GetConfig().stickDeadzonePercent[0], -buttonData->Y, -32768, 32767);
        normalData->sticks[1].axis_x = Normalize(GetConfig().stickDeadzonePercent[1], buttonData->Rx, -32768, 32767);
        normalData->sticks[1].axis_y = Normalize(GetConfig().stickDeadzonePercent[1], -buttonData->Ry, -32768, 32767);

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
