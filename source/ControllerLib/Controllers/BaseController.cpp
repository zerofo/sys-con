#include "Controllers/BaseController.h"
#include <cmath>

// https://www.usb.org/sites/default/files/documents/hid1_11.pdf  p55

BaseController::BaseController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : IController(std::move(device), config, std::move(logger))
{
    Log(LogLevelDebug, "Controller[%04x-%04x] Created !", m_device->GetVendor(), m_device->GetProduct());
}

BaseController::~BaseController()
{
}

ControllerResult BaseController::Initialize()
{
    Log(LogLevelDebug, "Controller[%04x-%04x] Initializing ...", m_device->GetVendor(), m_device->GetProduct());

    ControllerResult result = OpenInterfaces();
    if (result != CONTROLLER_STATUS_SUCCESS)
    {
        Log(LogLevelError, "Controller[%04x-%04x] Failed to open interfaces !", m_device->GetVendor(), m_device->GetProduct());
        return result;
    }

    return CONTROLLER_STATUS_SUCCESS;
}

void BaseController::Exit()
{
    CloseInterfaces();
}

uint16_t BaseController::GetInputCount()
{
    return 1;
}

ControllerResult BaseController::OpenInterfaces()
{
    int interfaceCount = 0;
    Log(LogLevelDebug, "Controller[%04x-%04x] Opening interfaces ...", m_device->GetVendor(), m_device->GetProduct());

    ControllerResult result = m_device->Open();
    if (result != CONTROLLER_STATUS_SUCCESS)
    {
        Log(LogLevelError, "Controller[%04x-%04x] Failed to open device !", m_device->GetVendor(), m_device->GetProduct());
        return result;
    }

    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        Log(LogLevelDebug, "Controller[%04x-%04x] Opening interface idx=%d ...", m_device->GetVendor(), m_device->GetProduct(), interfaceCount++);

        ControllerResult result = interface->Open();
        if (result != CONTROLLER_STATUS_SUCCESS)
        {
            Log(LogLevelError, "Controller[%04x-%04x] Failed to open interface %d !", m_device->GetVendor(), m_device->GetProduct(), interfaceCount);
            return result;
        }

        for (uint8_t idx = 0; idx < 15; idx++)
        {
            IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, idx);
            if (inEndpoint == NULL)
                continue;

            ControllerResult result = inEndpoint->Open(GetConfig().inputMaxPacketSize);
            if (result != CONTROLLER_STATUS_SUCCESS)
            {
                Log(LogLevelError, "Controller[%04x-%04x] Failed to open input endpoint %d !", m_device->GetVendor(), m_device->GetProduct(), idx);
                return result;
            }

            m_inPipe.push_back(inEndpoint);
        }

        for (uint8_t idx = 0; idx < 15; idx++)
        {
            IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, idx);
            if (outEndpoint == NULL)
                continue;

            result = outEndpoint->Open(GetConfig().outputMaxPacketSize);
            if (result != CONTROLLER_STATUS_SUCCESS)
            {
                Log(LogLevelError, "Controller[%04x-%04x] Failed to open output  endpoint %d !", m_device->GetVendor(), m_device->GetProduct(), idx);
                return result;
            }

            m_outPipe.push_back(outEndpoint);
        }

        m_interfaces.push_back(interface.get());
    }

    if (m_inPipe.empty())
    {
        Log(LogLevelError, "Controller[%04x-%04x] Not input endpoint found !", m_device->GetVendor(), m_device->GetProduct());
        return CONTROLLER_STATUS_INVALID_ENDPOINT;
    }

    Log(LogLevelDebug, "Controller[%04x-%04x] successfully opened !", m_device->GetVendor(), m_device->GetProduct());
    return CONTROLLER_STATUS_SUCCESS;
}

void BaseController::CloseInterfaces()
{
    m_device->Close();
}

bool BaseController::Support(ControllerFeature feature)
{
    (void)feature;
    return false;
}

ControllerResult BaseController::SetRumble(uint16_t input_idx, float amp_high, float amp_low)
{
    (void)input_idx;
    (void)amp_high;
    (void)amp_low;
    // Not implemented yet
    return CONTROLLER_STATUS_NOT_IMPLEMENTED;
}

static_assert(MAX_PIN_BY_BUTTONS == 2, "You need to update IsPinPressed macro !");
#define IsPinPressed(rawData, controllerButton) \
    (rawData.buttons[GetConfig().buttons_pin[controllerButton][0]] || rawData.buttons[GetConfig().buttons_pin[controllerButton][1]]) ? true : false

ControllerResult BaseController::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx, uint32_t timeout_us)
{
    RawInputData rawData;

    ControllerResult result = ReadRawInput(&rawData, input_idx, timeout_us);
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    Log(LogLevelDebug, "Controller[%04x-%04x] DATA: X=%d%%, Y=%d%%, Z=%d%%, Rz=%d%%, B1=%d, B2=%d, B3=%d, B4=%d, B5=%d, B6=%d, B7=%d, B8=%d, B9=%d, B10=%d",
        m_device->GetVendor(), m_device->GetProduct(),
        (int)(rawData.X * 100.0), (int)(rawData.Y * 100.0), (int)(rawData.Z * 100.0), (int)(rawData.Rz * 100.0),
        rawData.buttons[1] ? 1 : 0,
        rawData.buttons[2] ? 1 : 0,
        rawData.buttons[3] ? 1 : 0,
        rawData.buttons[4] ? 1 : 0,
        rawData.buttons[5] ? 1 : 0,
        rawData.buttons[6] ? 1 : 0,
        rawData.buttons[7] ? 1 : 0,
        rawData.buttons[8] ? 1 : 0,
        rawData.buttons[9] ? 1 : 0,
        rawData.buttons[10] ? 1 : 0);

    float bindAnalog[ControllerAnalogBinding_Count] = {
        0.0,
        rawData.X,
        rawData.Y,
        rawData.Z,
        rawData.Rz,
        rawData.Rx,
        rawData.Ry};

    normalData->triggers[0] = GetConfig().triggerConfig[0].sign * BaseController::ApplyDeadzone(GetConfig().triggerDeadzonePercent[0], bindAnalog[GetConfig().triggerConfig[0].bind]);
    normalData->triggers[1] = GetConfig().triggerConfig[1].sign * BaseController::ApplyDeadzone(GetConfig().triggerDeadzonePercent[1], bindAnalog[GetConfig().triggerConfig[1].bind]);

    normalData->sticks[0].axis_x = GetConfig().stickConfig[0].X.sign * BaseController::ApplyDeadzone(GetConfig().stickDeadzonePercent[0], bindAnalog[GetConfig().stickConfig[0].X.bind]);
    normalData->sticks[0].axis_y = GetConfig().stickConfig[0].Y.sign * BaseController::ApplyDeadzone(GetConfig().stickDeadzonePercent[0], bindAnalog[GetConfig().stickConfig[0].Y.bind]);
    normalData->sticks[1].axis_x = GetConfig().stickConfig[1].X.sign * BaseController::ApplyDeadzone(GetConfig().stickDeadzonePercent[1], bindAnalog[GetConfig().stickConfig[1].X.bind]);
    normalData->sticks[1].axis_y = GetConfig().stickConfig[1].Y.sign * BaseController::ApplyDeadzone(GetConfig().stickDeadzonePercent[1], bindAnalog[GetConfig().stickConfig[1].Y.bind]);

    // Stick as button
    normalData->buttons[ControllerButton::LSTICK_LEFT] = IsPinPressed(rawData, ControllerButton::LSTICK_LEFT);
    normalData->buttons[ControllerButton::LSTICK_RIGHT] = IsPinPressed(rawData, ControllerButton::LSTICK_RIGHT);
    normalData->buttons[ControllerButton::LSTICK_UP] = IsPinPressed(rawData, ControllerButton::LSTICK_UP);
    normalData->buttons[ControllerButton::LSTICK_DOWN] = IsPinPressed(rawData, ControllerButton::LSTICK_DOWN);
    normalData->buttons[ControllerButton::RSTICK_LEFT] = IsPinPressed(rawData, ControllerButton::RSTICK_LEFT);
    normalData->buttons[ControllerButton::RSTICK_RIGHT] = IsPinPressed(rawData, ControllerButton::RSTICK_RIGHT);
    normalData->buttons[ControllerButton::RSTICK_UP] = IsPinPressed(rawData, ControllerButton::RSTICK_UP);
    normalData->buttons[ControllerButton::RSTICK_DOWN] = IsPinPressed(rawData, ControllerButton::RSTICK_DOWN);

    // Set stick from button (If any)
    if (normalData->buttons[ControllerButton::LSTICK_LEFT])
        normalData->sticks[0].axis_x = -1.0f;
    if (normalData->buttons[ControllerButton::LSTICK_RIGHT])
        normalData->sticks[0].axis_x = 1.0f;
    if (normalData->buttons[ControllerButton::LSTICK_UP])
        normalData->sticks[0].axis_y = -1.0f;
    if (normalData->buttons[ControllerButton::LSTICK_DOWN])
        normalData->sticks[0].axis_y = 1.0f;
    if (normalData->buttons[ControllerButton::RSTICK_LEFT])
        normalData->sticks[1].axis_x = -1.0f;
    if (normalData->buttons[ControllerButton::RSTICK_RIGHT])
        normalData->sticks[1].axis_x = 1.0f;
    if (normalData->buttons[ControllerButton::RSTICK_UP])
        normalData->sticks[1].axis_y = -1.0f;
    if (normalData->buttons[ControllerButton::RSTICK_DOWN])
        normalData->sticks[1].axis_y = 1.0f;

    // Set button state from stick

    float stickActivationThreshold = (GetConfig().stickActivationThreshold / 100.0f);
    if (stickActivationThreshold > 0.0f)
    {
        if (normalData->sticks[0].axis_x > stickActivationThreshold)
            normalData->buttons[ControllerButton::LSTICK_RIGHT] = true;
        if (normalData->sticks[0].axis_x < -stickActivationThreshold)
            normalData->buttons[ControllerButton::LSTICK_LEFT] = true;
        if (normalData->sticks[0].axis_y > stickActivationThreshold)
            normalData->buttons[ControllerButton::LSTICK_DOWN] = true;
        if (normalData->sticks[0].axis_y < -stickActivationThreshold)
            normalData->buttons[ControllerButton::LSTICK_UP] = true;
        if (normalData->sticks[1].axis_x > stickActivationThreshold)
            normalData->buttons[ControllerButton::RSTICK_RIGHT] = true;
        if (normalData->sticks[1].axis_x < -stickActivationThreshold)
            normalData->buttons[ControllerButton::RSTICK_LEFT] = true;
        if (normalData->sticks[1].axis_y > stickActivationThreshold)
            normalData->buttons[ControllerButton::RSTICK_DOWN] = true;
        if (normalData->sticks[1].axis_y < -stickActivationThreshold)
            normalData->buttons[ControllerButton::RSTICK_UP] = true;
    }

    // Set Buttons
    normalData->buttons[ControllerButton::X] = IsPinPressed(rawData, ControllerButton::X);
    normalData->buttons[ControllerButton::A] = IsPinPressed(rawData, ControllerButton::A);
    normalData->buttons[ControllerButton::B] = IsPinPressed(rawData, ControllerButton::B);
    normalData->buttons[ControllerButton::Y] = IsPinPressed(rawData, ControllerButton::Y);
    normalData->buttons[ControllerButton::LSTICK_CLICK] = IsPinPressed(rawData, ControllerButton::LSTICK_CLICK);
    normalData->buttons[ControllerButton::RSTICK_CLICK] = IsPinPressed(rawData, ControllerButton::RSTICK_CLICK);
    normalData->buttons[ControllerButton::L] = IsPinPressed(rawData, ControllerButton::L);
    normalData->buttons[ControllerButton::R] = IsPinPressed(rawData, ControllerButton::R);
    normalData->buttons[ControllerButton::ZL] = IsPinPressed(rawData, ControllerButton::ZL);
    normalData->buttons[ControllerButton::ZR] = IsPinPressed(rawData, ControllerButton::ZR);
    normalData->buttons[ControllerButton::MINUS] = IsPinPressed(rawData, ControllerButton::MINUS);
    normalData->buttons[ControllerButton::PLUS] = IsPinPressed(rawData, ControllerButton::PLUS);
    normalData->buttons[ControllerButton::CAPTURE] = IsPinPressed(rawData, ControllerButton::CAPTURE);
    normalData->buttons[ControllerButton::HOME] = IsPinPressed(rawData, ControllerButton::HOME);
    normalData->buttons[ControllerButton::DPAD_UP] = IsPinPressed(rawData, ControllerButton::DPAD_UP);
    normalData->buttons[ControllerButton::DPAD_DOWN] = IsPinPressed(rawData, ControllerButton::DPAD_DOWN);
    normalData->buttons[ControllerButton::DPAD_RIGHT] = IsPinPressed(rawData, ControllerButton::DPAD_RIGHT);
    normalData->buttons[ControllerButton::DPAD_LEFT] = IsPinPressed(rawData, ControllerButton::DPAD_LEFT);

    // Special fallback if button pin are not set
    if (GetConfig().buttons_pin[ControllerButton::ZL][0] == 0)
        normalData->buttons[ControllerButton::ZL] = normalData->triggers[0] > 0;
    if (GetConfig().buttons_pin[ControllerButton::ZR][0] == 0)
        normalData->buttons[ControllerButton::ZR] = normalData->triggers[1] > 0;
    if (GetConfig().buttons_pin[ControllerButton::DPAD_UP][0] == 0)
        normalData->buttons[ControllerButton::DPAD_UP] = rawData.dpad_up;
    if (GetConfig().buttons_pin[ControllerButton::DPAD_DOWN][0] == 0)
        normalData->buttons[ControllerButton::DPAD_DOWN] = rawData.dpad_down;
    if (GetConfig().buttons_pin[ControllerButton::DPAD_RIGHT][0] == 0)
        normalData->buttons[ControllerButton::DPAD_RIGHT] = rawData.dpad_right;
    if (GetConfig().buttons_pin[ControllerButton::DPAD_LEFT][0] == 0)
        normalData->buttons[ControllerButton::DPAD_LEFT] = rawData.dpad_left;

    // Handle alias
    for (int i = 0; i < MAX_CONTROLLER_BUTTONS; i++)
    {
        ControllerButton alias_button = GetConfig().buttons_alias[i];
        if (alias_button != ControllerButton::NONE && normalData->buttons[i] == false)
            normalData->buttons[i] = normalData->buttons[alias_button];
    }

    // Simulate buttons
    if (GetConfig().simulateHome[0] != ControllerButton::NONE && GetConfig().simulateHome[1] != ControllerButton::NONE)
    {
        if (normalData->buttons[GetConfig().simulateHome[0]] && normalData->buttons[GetConfig().simulateHome[1]])
        {
            normalData->buttons[ControllerButton::HOME] = true;
            normalData->buttons[GetConfig().simulateHome[0]] = false;
            normalData->buttons[GetConfig().simulateHome[1]] = false;
        }
    }

    if (GetConfig().simulateCapture[0] != ControllerButton::NONE && GetConfig().simulateCapture[1] != ControllerButton::NONE)
    {
        if (normalData->buttons[GetConfig().simulateCapture[0]] && normalData->buttons[GetConfig().simulateCapture[1]])
        {
            normalData->buttons[ControllerButton::CAPTURE] = true;
            normalData->buttons[GetConfig().simulateCapture[0]] = false;
            normalData->buttons[GetConfig().simulateCapture[1]] = false;
        }
    }

    return CONTROLLER_STATUS_SUCCESS;
}

float BaseController::ApplyDeadzone(uint8_t deadzonePercent, float value)
{
    float deadzone = 1.0f * deadzonePercent / 100.0f;

    if (std::abs(value) < deadzone)
        return 0.0f;

    if (value > 0)
        value = (value - deadzone) / (1.0f - deadzone);
    else
        value = (value + deadzone) / (1.0f - deadzone);

    return value;
}

float BaseController::Normalize(int32_t value, int32_t min, int32_t max)
{
    float range = floor((max - min) / 2.0f);
    float offset = range;

    if (range == max)
        offset = 0;

    float ret = (value - offset) / range;

    if (ret > 1.0f)
        ret = 1.0f;
    else if (ret < -1.0f)
        ret = -1.0f;

    return ret;
}