#include "Controllers/BaseController.h"
#include <cmath>

// https://www.usb.org/sites/default/files/documents/hid1_11.pdf  p55

BaseController::BaseController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : IController(std::move(device), config, std::move(logger))
{
    LogPrint(LogLevelDebug, "Controller[%04x-%04x] Created !", m_device->GetVendor(), m_device->GetProduct());
}

BaseController::~BaseController()
{
}

ams::Result BaseController::Initialize()
{
    LogPrint(LogLevelDebug, "Controller[%04x-%04x] Initializing ...", m_device->GetVendor(), m_device->GetProduct());

    R_TRY(OpenInterfaces());

    R_SUCCEED();
}

void BaseController::Exit()
{
    CloseInterfaces();
}

uint16_t BaseController::GetInputCount()
{
    return 1;
}

ams::Result BaseController::OpenInterfaces()
{
    int interfaceCount = 0;
    LogPrint(LogLevelDebug, "Controller[%04x-%04x] Opening interfaces ...", m_device->GetVendor(), m_device->GetProduct());

    ams::Result rc = m_device->Open();
    if (R_FAILED(rc))
    {
        LogPrint(LogLevelError, "Controller[%04x-%04x] Failed to open device !", m_device->GetVendor(), m_device->GetProduct());
        R_RETURN(rc);
    }

    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        LogPrint(LogLevelDebug, "Controller[%04x-%04x] Opening interface idx=%d ...", m_device->GetVendor(), m_device->GetProduct(), interfaceCount++);

        R_TRY(interface->Open());

        for (uint8_t idx = 0; idx < 15; idx++)
        {
            IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, idx);
            if (inEndpoint == NULL)
                continue;

            rc = inEndpoint->Open();
            if (R_FAILED(rc))
            {
                LogPrint(LogLevelError, "Controller[%04x-%04x] Failed to open input endpoint %d !", m_device->GetVendor(), m_device->GetProduct(), idx);
                R_RETURN(rc);
            }

            m_inPipe.push_back(inEndpoint);
        }

        for (uint8_t idx = 0; idx < 15; idx++)
        {
            IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, idx);
            if (outEndpoint == NULL)
                continue;

            rc = outEndpoint->Open();
            if (R_FAILED(rc))
            {
                LogPrint(LogLevelError, "Controller[%04x-%04x] Failed to open output  endpoint %d !", m_device->GetVendor(), m_device->GetProduct(), idx);
                R_RETURN(rc);
            }

            m_outPipe.push_back(outEndpoint);
        }

        m_interfaces.push_back(interface.get());
    }

    if (m_inPipe.empty())
    {
        LogPrint(LogLevelError, "Controller[%04x-%04x] Not input endpoint found !", m_device->GetVendor(), m_device->GetProduct());
        R_RETURN(CONTROL_ERR_INVALID_ENDPOINT);
    }

    LogPrint(LogLevelDebug, "Controller[%04x-%04x] successfully opened !", m_device->GetVendor(), m_device->GetProduct());
    R_SUCCEED();
}

void BaseController::CloseInterfaces()
{
    m_device->Close();
}

bool BaseController::Support(ControllerFeature feature)
{
    switch (feature)
    {
        default:
            return false;
    }
}

ams::Result BaseController::SetRumble(uint16_t input_idx, float amp_high, float amp_low)
{
    (void)input_idx;
    (void)amp_high;
    (void)amp_low;
    // Not implemented yet
    return 9;
}

ams::Result BaseController::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx, uint32_t timeout_us)
{
    RawInputData rawData;

    R_TRY(ReadInput(&rawData, input_idx, timeout_us));

    LogPrint(LogLevelDebug, "Controller[%04x-%04x] DATA: X=%d%%, Y=%d%%, Z=%d%%, Rz=%d%%, B1=%d, B2=%d, B3=%d, B4=%d, B5=%d, B6=%d, B7=%d, B8=%d, B9=%d, B10=%d",
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

    normalData->triggers[0] = GetConfig().triggerConfig[0].sign * ApplyDeadzone(GetConfig().triggerDeadzonePercent[0], bindAnalog[GetConfig().triggerConfig[0].bind]);
    normalData->triggers[1] = GetConfig().triggerConfig[1].sign * ApplyDeadzone(GetConfig().triggerDeadzonePercent[1], bindAnalog[GetConfig().triggerConfig[1].bind]);

    normalData->sticks[0].axis_x = GetConfig().stickConfig[0].X.sign * ApplyDeadzone(GetConfig().stickDeadzonePercent[0], bindAnalog[GetConfig().stickConfig[0].X.bind]);
    normalData->sticks[0].axis_y = GetConfig().stickConfig[0].Y.sign * ApplyDeadzone(GetConfig().stickDeadzonePercent[0], bindAnalog[GetConfig().stickConfig[0].Y.bind]);
    normalData->sticks[1].axis_x = GetConfig().stickConfig[1].X.sign * ApplyDeadzone(GetConfig().stickDeadzonePercent[1], bindAnalog[GetConfig().stickConfig[1].X.bind]);
    normalData->sticks[1].axis_y = GetConfig().stickConfig[1].Y.sign * ApplyDeadzone(GetConfig().stickDeadzonePercent[1], bindAnalog[GetConfig().stickConfig[1].Y.bind]);

    normalData->buttons[ControllerButton::X] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::X]] ? true : false;
    normalData->buttons[ControllerButton::A] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::A]] ? true : false;
    normalData->buttons[ControllerButton::B] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::B]] ? true : false;
    normalData->buttons[ControllerButton::Y] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::Y]] ? true : false;
    normalData->buttons[ControllerButton::LSTICK_CLICK] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::LSTICK_CLICK]] ? true : false;
    normalData->buttons[ControllerButton::RSTICK_CLICK] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::RSTICK_CLICK]] ? true : false;
    normalData->buttons[ControllerButton::L] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::L]] ? true : false;
    normalData->buttons[ControllerButton::R] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::R]] ? true : false;

    normalData->buttons[ControllerButton::ZL] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::ZL]] ? true : false;
    normalData->buttons[ControllerButton::ZR] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::ZR]] ? true : false;

    if (GetConfig().buttons_pin[ControllerButton::ZL] == 0)
        normalData->buttons[ControllerButton::ZL] = normalData->triggers[0] > 0;

    if (GetConfig().buttons_pin[ControllerButton::ZR] == 0)
        normalData->buttons[ControllerButton::ZR] = normalData->triggers[1] > 0;

    normalData->buttons[ControllerButton::MINUS] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::MINUS]] ? true : false;
    normalData->buttons[ControllerButton::PLUS] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::PLUS]] ? true : false;
    normalData->buttons[ControllerButton::CAPTURE] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::CAPTURE]] ? true : false;
    normalData->buttons[ControllerButton::HOME] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::HOME]] ? true : false;

    normalData->buttons[ControllerButton::DPAD_UP] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::DPAD_UP]];
    normalData->buttons[ControllerButton::DPAD_DOWN] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::DPAD_DOWN]];
    normalData->buttons[ControllerButton::DPAD_RIGHT] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::DPAD_RIGHT]];
    normalData->buttons[ControllerButton::DPAD_LEFT] = rawData.buttons[GetConfig().buttons_pin[ControllerButton::DPAD_LEFT]];

    if (GetConfig().buttons_pin[ControllerButton::DPAD_UP] == 0)
        normalData->buttons[ControllerButton::DPAD_UP] = rawData.dpad_up;

    if (GetConfig().buttons_pin[ControllerButton::DPAD_DOWN] == 0)
        normalData->buttons[ControllerButton::DPAD_DOWN] = rawData.dpad_down;

    if (GetConfig().buttons_pin[ControllerButton::DPAD_RIGHT] == 0)
        normalData->buttons[ControllerButton::DPAD_RIGHT] = rawData.dpad_right;

    if (GetConfig().buttons_pin[ControllerButton::DPAD_LEFT] == 0)    
        normalData->buttons[ControllerButton::DPAD_LEFT] = rawData.dpad_left;

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

    R_SUCCEED();
}

float BaseController::ApplyDeadzone(uint8_t deadzonePercent, float value)
{
    float deadzone = 1.0 * deadzonePercent / 100;

    if (std::abs(value) < deadzone)
        return 0.0f;

    if (value > 0)
        value = (value - deadzone) / (1.0 - deadzone);
    else
        value = (value + deadzone) / (1.0 - deadzone);

    return value;
}

float BaseController::Normalize(int32_t value, int32_t min, int32_t max)
{
    float range = (max - min) / 2;
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