#include "Controllers/BaseController.h"
#include <cmath>

// https://www.usb.org/sites/default/files/documents/hid1_11.pdf  p55

BaseController::BaseController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : IController(std::move(device), config, std::move(logger))
{
    LogPrint(LogLevelDebug, "BaseController Created for %04x-%04x", m_device->GetVendor(), m_device->GetProduct());
}

BaseController::~BaseController()
{
}

ams::Result BaseController::Initialize()
{
    LogPrint(LogLevelDebug, "BaseController Initializing ...");

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
    LogPrint(LogLevelDebug, "BaseController Opening interfaces ...");

    R_TRY(m_device->Open());

    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        LogPrint(LogLevelDebug, "BaseController Opening interface idx=%d ...", interfaceCount++);

        R_TRY(interface->Open());

        for (uint8_t idx = 0; idx < 15; idx++)
        {
            IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, idx);
            if (inEndpoint == NULL)
                continue;

            R_TRY(inEndpoint->Open());
            m_inPipe.push_back(inEndpoint);
        }

        for (uint8_t idx = 0; idx < 15; idx++)
        {
            IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, idx);
            if (outEndpoint == NULL)
                continue;

            R_TRY(outEndpoint->Open());
            m_outPipe.push_back(outEndpoint);
        }

        m_interfaces.push_back(interface.get());
    }

    if (m_inPipe.empty())
        R_RETURN(CONTROL_ERR_INVALID_ENDPOINT);

    LogPrint(LogLevelDebug, "BaseController successfully opened !");

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

ams::Result BaseController::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
{
    RawInputData rawData;

    R_TRY(ReadInput(&rawData, input_idx));

    LogPrint(LogLevelDebug, "BaseController DATA: X=%d, Y=%d, Z=%d, Rz=%d, B1=%d, B2=%d, B3=%d, B4=%d, B5=%d, B6=%d, B7=%d, B8=%d, B9=%d, B10=%d",
             rawData.X, rawData.Y, rawData.Z, rawData.Rz,
             rawData.buttons[1],
             rawData.buttons[2] ? 1 : 0,
             rawData.buttons[3] ? 1 : 0,
             rawData.buttons[4] ? 1 : 0,
             rawData.buttons[5] ? 1 : 0,
             rawData.buttons[6] ? 1 : 0,
             rawData.buttons[7] ? 1 : 0,
             rawData.buttons[8] ? 1 : 0,
             rawData.buttons[9] ? 1 : 0,
             rawData.buttons[10] ? 1 : 0);

    normalData->triggers[0] = ApplyDeadzone(GetConfig().triggerDeadzonePercent[0], rawData.Rx);
    normalData->triggers[1] = ApplyDeadzone(GetConfig().triggerDeadzonePercent[1], rawData.Ry);

    normalData->sticks[0].axis_x = ApplyDeadzone(GetConfig().stickDeadzonePercent[0], rawData.X);
    normalData->sticks[0].axis_y = ApplyDeadzone(GetConfig().stickDeadzonePercent[0], rawData.Y);
    normalData->sticks[1].axis_x = ApplyDeadzone(GetConfig().stickDeadzonePercent[1], rawData.Z);
    normalData->sticks[1].axis_y = ApplyDeadzone(GetConfig().stickDeadzonePercent[1], rawData.Rz);

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

    normalData->buttons[ControllerButton::DPAD_UP] = rawData.dpad_up;
    normalData->buttons[ControllerButton::DPAD_DOWN] = rawData.dpad_down;
    normalData->buttons[ControllerButton::DPAD_RIGHT] = rawData.dpad_right;
    normalData->buttons[ControllerButton::DPAD_LEFT] = rawData.dpad_left;

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