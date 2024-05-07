#include "Controllers/XboxController.h"
#include <cmath>

XboxController::XboxController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : IController(std::move(device), config, std::move(logger))
{
}

XboxController::~XboxController()
{
}

ams::Result XboxController::Initialize()
{
    R_RETURN(OpenInterfaces());
}

void XboxController::Exit()
{
    CloseInterfaces();
}

ams::Result XboxController::OpenInterfaces()
{
    R_TRY(m_device->Open());

    // This will open each interface and try to acquire Xbox controller's in and out endpoints, if it hasn't already
    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        R_TRY(interface->Open());

        if (interface->GetDescriptor()->bInterfaceProtocol != 0)
            continue;

        if (interface->GetDescriptor()->bNumEndpoints < 2)
            continue;

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

void XboxController::CloseInterfaces()
{
    // m_device->Reset();
    m_device->Close();
}

ams::Result XboxController::GetInput()
{
    uint8_t input_bytes[64];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe->Read(input_bytes, &size));

    m_buttonData = *reinterpret_cast<XboxButtonData *>(input_bytes);

    R_SUCCEED();
}

bool XboxController::Support(ControllerFeature feature)
{
    if (feature == SUPPORTS_RUMBLE)
        return true;
    return false;
}

// Pass by value should hopefully be optimized away by RVO
NormalizedButtonData XboxController::GetNormalizedButtonData()
{
    NormalizedButtonData normalData{};

    normalData.triggers[0] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[0], m_buttonData.trigger_left);
    normalData.triggers[1] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[1], m_buttonData.trigger_right);

    NormalizeAxis(m_buttonData.stick_left_x, m_buttonData.stick_left_y, GetConfig().stickDeadzonePercent[0],
                  &normalData.sticks[0].axis_x, &normalData.sticks[0].axis_y, -32768, 32767);
    NormalizeAxis(m_buttonData.stick_right_x, m_buttonData.stick_right_y, GetConfig().stickDeadzonePercent[1],
                  &normalData.sticks[1].axis_x, &normalData.sticks[1].axis_y, -32768, 32767);

    bool buttons[MAX_CONTROLLER_BUTTONS]{
        m_buttonData.y != 0,
        m_buttonData.b != 0,
        m_buttonData.a != 0,
        m_buttonData.x != 0,
        m_buttonData.stick_left_click,
        m_buttonData.stick_right_click,
        false,
        false,
        m_buttonData.trigger_left != 0,
        m_buttonData.trigger_right != 0,
        m_buttonData.back,
        m_buttonData.start,
        m_buttonData.dpad_up,
        m_buttonData.dpad_right,
        m_buttonData.dpad_down,
        m_buttonData.dpad_left,
        m_buttonData.white_button != 0,
        m_buttonData.black_buttton != 0,
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

ams::Result XboxController::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    uint8_t rumbleData[]{0x00, 0x06, 0x00, strong_magnitude, weak_magnitude, 0x00, 0x00, 0x00};
    R_RETURN(m_outPipe->Write(rumbleData, sizeof(rumbleData)));
}
