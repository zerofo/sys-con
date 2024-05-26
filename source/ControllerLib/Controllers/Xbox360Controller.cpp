#include "Controllers/Xbox360Controller.h"

// https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/

static constexpr uint8_t reconnectPacket[]{0x08, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t poweroffPacket[]{0x00, 0x00, 0x08, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t initDriverPacket[]{0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t ledPacketOn[]{0x00, 0x00, 0x08, 0x40 | XBOX360LED_TOPLEFT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

Xbox360Controller::Xbox360Controller(std::unique_ptr<IUSBDevice> &&interface, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(interface), config, std::move(logger))
{
    m_is_wireless = (m_device->GetProduct() == 0x0291) || (m_device->GetProduct() == 0x0719);
}

Xbox360Controller::~Xbox360Controller()
{
}

ams::Result Xbox360Controller::Initialize()
{
    R_TRY(BaseController::Initialize());

    // Duplicated with OnControllerConnect
    if (m_is_wireless)
    {
        R_TRY(m_outPipe->Write(reconnectPacket, sizeof(reconnectPacket)));
    }
    else
    {
        m_is_present = true;
    }

    SetLED(XBOX360LED_TOPLEFT);

    R_SUCCEED();
}

void Xbox360Controller::CloseInterfaces()
{
    // Duplicated with OnControllerDisconnect
    if (m_is_wireless && m_is_present)
        m_outPipe->Write(poweroffPacket, sizeof(poweroffPacket));

    BaseController::CloseInterfaces();
}

ams::Result Xbox360Controller::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
{
    uint8_t input_bytes[64];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe->Read(input_bytes, &size));

    Xbox360ButtonData *buttonData = reinterpret_cast<Xbox360ButtonData *>(input_bytes);

    uint8_t type = input_bytes[0];

    if (m_is_wireless)
    {
        if (type & 0x08)
        {
            bool newPresence = (input_bytes[1] & 0x80) != 0;

            if (m_is_present != newPresence)
            {
                m_is_present = newPresence;

                if (m_is_present)
                    OnControllerConnect();
                else
                    OnControllerDisconnect();
            }
        }

        if (input_bytes[1] != 0x01)
            R_RETURN(CONTROL_ERR_UNEXPECTED_DATA);

        buttonData = reinterpret_cast<Xbox360ButtonData *>(input_bytes + 4);
    }

    if (type == XBOX360INPUT_BUTTON) // Button data
    {
        *input_idx = 0;

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

        normalData->buttons[ControllerButton::DPAD_UP] = buttonData->dpad_up;
        normalData->buttons[ControllerButton::DPAD_RIGHT] = buttonData->dpad_right;
        normalData->buttons[ControllerButton::DPAD_DOWN] = buttonData->dpad_down;
        normalData->buttons[ControllerButton::DPAD_LEFT] = buttonData->dpad_left;
    }

    R_SUCCEED();
}

/*
bool Xbox360Controller::Support(ControllerFeature feature)
{
    if (feature == SUPPORTS_RUMBLE)
        return true;

    if (feature == SUPPORTS_PAIRING)
        return m_is_wireless ? true : false;

    return false;
}*/

ams::Result Xbox360Controller::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    if (m_is_wireless)
    {
        uint8_t rumbleData[]{0x00, 0x01, 0x0F, 0xC0, 0x00, strong_magnitude, weak_magnitude, 0x00, 0x00, 0x00, 0x00, 0x00};
        R_RETURN(m_outPipe->Write(rumbleData, sizeof(rumbleData)));
    }
    else
    {
        uint8_t rumbleData[]{0x00, sizeof(Xbox360RumbleData), 0x00, strong_magnitude, weak_magnitude, 0x00, 0x00, 0x00};
        R_RETURN(m_outPipe->Write(rumbleData, sizeof(rumbleData)));
    }
}

ams::Result Xbox360Controller::SetLED(Xbox360LEDValue value)
{
    if (m_is_wireless)
    {
        uint8_t customLEDPacket[]{0x00, 0x00, 0x08, static_cast<uint8_t>(value | 0x40), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        R_RETURN(m_outPipe->Write(customLEDPacket, sizeof(customLEDPacket)));
    }
    else
    {
        uint8_t ledPacket[]{0x01, 0x03, static_cast<uint8_t>(value)};
        R_RETURN(m_outPipe->Write(ledPacket, sizeof(ledPacket)));
    }
}

ams::Result Xbox360Controller::OnControllerConnect()
{
    LogPrint(LogLevelInfo, "Xbox360Controller Wireless controller connected ...");

    m_outputBuffer.push_back(OutputPacket{reconnectPacket, sizeof(reconnectPacket)});
    m_outputBuffer.push_back(OutputPacket{initDriverPacket, sizeof(initDriverPacket)});
    m_outputBuffer.push_back(OutputPacket{ledPacketOn, sizeof(ledPacketOn)}); // Duplicated with SetLED
    R_SUCCEED();
}

ams::Result Xbox360Controller::OnControllerDisconnect()
{
    LogPrint(LogLevelInfo, "Xbox360Controller Wireless controller disconnected ...");

    m_outputBuffer.push_back(OutputPacket{poweroffPacket, sizeof(poweroffPacket)});
    R_SUCCEED();
}

ams::Result Xbox360Controller::OutputBuffer()
{
    if (m_outputBuffer.empty())
        R_RETURN(CONTROL_ERR_BUFFER_EMPTY);

    auto it = m_outputBuffer.begin();
    R_TRY(m_outPipe->Write(it->packet, it->length));
    m_outputBuffer.erase(it);

    R_SUCCEED();
}