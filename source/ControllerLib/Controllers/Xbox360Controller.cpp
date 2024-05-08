#include "Controllers/Xbox360Controller.h"
// https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/

static constexpr uint8_t reconnectPacket[]{0x08, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t poweroffPacket[]{0x00, 0x00, 0x08, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t initDriverPacket[]{0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t ledPacketOn[]{0x00, 0x00, 0x08, 0x40 | XBOX360LED_TOPLEFT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

Xbox360Controller::Xbox360Controller(std::unique_ptr<IUSBDevice> &&interface, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(interface), config, std::move(logger))
{
}

Xbox360Controller::~Xbox360Controller()
{
}

ams::Result Xbox360Controller::Initialize()
{
    R_TRY(BaseController::Initialize());

    // Duplicated with OnControllerConnect
    if (m_is_wireless)
        R_TRY(m_outPipe->Write(reconnectPacket, sizeof(reconnectPacket)));

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

        if (input_bytes[1] != 0x1)
            R_RETURN(CONTROL_ERR_UNEXPECTED_DATA);
    }

    if (type == XBOX360INPUT_BUTTON) // Button data
    {
        Xbox360ButtonData *buttonData = reinterpret_cast<Xbox360ButtonData *>(input_bytes);

        *input_idx = 0;

        normalData->triggers[0] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[0], buttonData->trigger_left);
        normalData->triggers[1] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[1], buttonData->trigger_right);

        NormalizeAxis(buttonData->stick_left_x, buttonData->stick_left_y, GetConfig().stickDeadzonePercent[0],
                      &normalData->sticks[0].axis_x, &normalData->sticks[0].axis_y, -32768, 32767);
        NormalizeAxis(buttonData->stick_right_x, buttonData->stick_right_y, GetConfig().stickDeadzonePercent[1],
                      &normalData->sticks[1].axis_x, &normalData->sticks[1].axis_y, -32768, 32767);

        bool buttons[MAX_CONTROLLER_BUTTONS]{
            buttonData->y,
            buttonData->b,
            buttonData->a,
            buttonData->x,
            buttonData->stick_left_click,
            buttonData->stick_right_click,
            buttonData->bumper_left,
            buttonData->bumper_right,
            normalData->triggers[0] > 0,
            normalData->triggers[1] > 0,
            buttonData->back,
            buttonData->start,
            buttonData->dpad_up,
            buttonData->dpad_right,
            buttonData->dpad_down,
            buttonData->dpad_left,
            false,
            buttonData->guide,
        };

        for (int i = 0; i != MAX_CONTROLLER_BUTTONS; ++i)
        {
            ControllerButton button = GetConfig().buttons[i];
            if (button == NONE)
                continue;

            normalData->buttons[(button != DEFAULT ? button - 2 : i)] += buttons[i];
        }
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
    m_outputBuffer.push_back(OutputPacket{reconnectPacket, sizeof(reconnectPacket)});
    m_outputBuffer.push_back(OutputPacket{initDriverPacket, sizeof(initDriverPacket)});
    m_outputBuffer.push_back(OutputPacket{ledPacketOn, sizeof(ledPacketOn)}); // Duplicated with SetLED
    R_SUCCEED();
}

ams::Result Xbox360Controller::OnControllerDisconnect()
{
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