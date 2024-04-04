#include "Controllers/Xbox360Controller.h"
#include <cmath>

static constexpr uint8_t reconnectPacket[]{0x08, 0x00, 0x0F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t poweroffPacket[]{0x00, 0x00, 0x08, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t initDriverPacket[]{0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static constexpr uint8_t ledPacketOn[]{0x00, 0x00, 0x08, 0x40 | XBOX360LED_TOPLEFT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

Xbox360Controller::Xbox360Controller(std::unique_ptr<IUSBDevice> &&interface, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : IController(std::move(interface), config, std::move(logger))
{
}

Xbox360Controller::~Xbox360Controller()
{
    // Exit();
}

ams::Result Xbox360Controller::Initialize()
{
    R_TRY(OpenInterfaces());

    // Duplicated with OnControllerConnect
    if (m_is_wireless)
        R_TRY(m_outPipe->Write(reconnectPacket, sizeof(reconnectPacket)));

    SetLED(XBOX360LED_TOPLEFT);

    R_SUCCEED();
}
void Xbox360Controller::Exit()
{
    CloseInterfaces();
}

ams::Result Xbox360Controller::OpenInterfaces()
{
    R_TRY(m_device->Open());

    // This will open each interface and try to acquire Xbox One controller's in and out endpoints, if it hasn't already
    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        R_TRY(interface->Open());

        if ((interface->GetDescriptor()->bInterfaceProtocol & 0x7F) != 0x01)
            continue;

        if (interface->GetDescriptor()->bNumEndpoints < 2)
            continue;

        m_is_wireless = interface->GetDescriptor()->bInterfaceProtocol & 0x80;

        if (!m_inPipe)
        {
            for (int i = 0; i != 15; ++i)
            {
                IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, i);
                if (inEndpoint)
                {
                    R_TRY(inEndpoint->Open());

                    m_inPipe = inEndpoint;
                    break;
                }
            }
        }

        if (!m_outPipe)
        {
            for (int i = 0; i != 15; ++i)
            {
                IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, i);
                if (outEndpoint)
                {
                    R_TRY(outEndpoint->Open());

                    m_outPipe = outEndpoint;
                    break;
                }
            }
        }
    }

    if (!m_inPipe || !m_outPipe)
        R_RETURN(CONTROL_ERR_INVALID_ENDPOINT);

    R_SUCCEED();
}

void Xbox360Controller::CloseInterfaces()
{
    // Duplicated with OnControllerDisconnect
    if (m_is_wireless && m_is_present)
        m_outPipe->Write(poweroffPacket, sizeof(poweroffPacket));

    // m_device->Reset();
    m_device->Close();
}

ams::Result Xbox360Controller::GetInput()
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
        m_buttonData = *reinterpret_cast<Xbox360ButtonData *>(input_bytes);
    }

    R_SUCCEED();
}

bool Xbox360Controller::Support(ControllerFeature feature)
{
    if (feature == SUPPORTS_RUMBLE)
        return true;

    if (feature == SUPPORTS_PAIRING)
        return m_is_wireless ? true : false;

    return false;
}

float Xbox360Controller::NormalizeTrigger(uint8_t deadzonePercent, uint8_t value)
{
    uint16_t deadzone = (UINT8_MAX * deadzonePercent) / 100;
    // If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < deadzone
               ? 0
               : static_cast<float>(value - deadzone) / (UINT8_MAX - deadzone);
}

void Xbox360Controller::NormalizeAxis(int16_t x,
                                      int16_t y,
                                      uint8_t deadzonePercent,
                                      float *x_out,
                                      float *y_out)
{
    float x_val = x;
    float y_val = y;
    // Determine how far the stick is pushed.
    // This will never exceed 32767 because if the stick is
    // horizontally maxed in one direction, vertically it must be neutral(0) and vice versa
    float real_magnitude = std::sqrt(x_val * x_val + y_val * y_val);
    float real_deadzone = (32767 * deadzonePercent) / 100;
    // Check if the controller is outside a circular dead zone.
    if (real_magnitude > real_deadzone)
    {
        // Clip the magnitude at its expected maximum value.
        float magnitude = std::min(32767.0f, real_magnitude);
        // Adjust magnitude relative to the end of the dead zone.
        magnitude -= real_deadzone;
        // Normalize the magnitude with respect to its expected range giving a
        // magnitude value of 0.0 to 1.0
        // ratio = (currentValue / maxValue) / realValue
        float ratio = (magnitude / (32767 - real_deadzone)) / real_magnitude;

        *x_out = x_val * ratio;
        *y_out = y_val * ratio;
    }
    else
    {
        // If the controller is in the deadzone zero out the magnitude.
        *x_out = *y_out = 0.0f;
    }
}

// Pass by value should hopefully be optimized away by RVO
NormalizedButtonData Xbox360Controller::GetNormalizedButtonData()
{
    NormalizedButtonData normalData{};

    normalData.triggers[0] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[0], m_buttonData.trigger_left);
    normalData.triggers[1] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[1], m_buttonData.trigger_right);

    NormalizeAxis(m_buttonData.stick_left_x, m_buttonData.stick_left_y, GetConfig().stickDeadzonePercent[0],
                  &normalData.sticks[0].axis_x, &normalData.sticks[0].axis_y);
    NormalizeAxis(m_buttonData.stick_right_x, m_buttonData.stick_right_y, GetConfig().stickDeadzonePercent[1],
                  &normalData.sticks[1].axis_x, &normalData.sticks[1].axis_y);

    bool buttons[MAX_CONTROLLER_BUTTONS]{
        m_buttonData.y,
        m_buttonData.b,
        m_buttonData.a,
        m_buttonData.x,
        m_buttonData.stick_left_click,
        m_buttonData.stick_right_click,
        m_buttonData.bumper_left,
        m_buttonData.bumper_right,
        normalData.triggers[0] > 0,
        normalData.triggers[1] > 0,
        m_buttonData.back,
        m_buttonData.start,
        m_buttonData.dpad_up,
        m_buttonData.dpad_right,
        m_buttonData.dpad_down,
        m_buttonData.dpad_left,
        false,
        m_buttonData.guide,
    };

    for (int i = 0; i != MAX_CONTROLLER_BUTTONS; ++i)
    {
        ControllerButton button = GetConfig().buttons[i];
        if (button == NONE)
            continue;

        normalData.buttons[(button != DEFAULT ? button - 2 : i)] += buttons[i];
    }

    return normalData;
}

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