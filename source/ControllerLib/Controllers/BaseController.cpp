#include "Controllers/BaseController.h"
#include <cmath>

// https://www.usb.org/sites/default/files/documents/hid1_11.pdf  p55

BaseController::BaseController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : IController(std::move(device), config, std::move(logger))
{
    LogPrint(LogLevelInfo, "BaseController Created for %04x-%04x", m_device->GetVendor(), m_device->GetProduct());
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
    LogPrint(LogLevelDebug, "BaseController Opening interfaces ...");

    R_TRY(m_device->Open());

    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        R_TRY(interface->Open());

        if (!m_inPipe)
        {
            for (int i = 0; i < 15; i++)
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
            for (int i = 0; i < 15; i++)
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

        m_interface = interface.get();

        break; // Stop after the first interface
    }

    if (!m_inPipe)
        R_RETURN(CONTROL_ERR_INVALID_ENDPOINT);

    LogPrint(LogLevelInfo, "BaseController successfully opened !");

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

ams::Result BaseController::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    (void)strong_magnitude;
    (void)weak_magnitude;
    // Not implemented yet
    return 9;
}

float BaseController::NormalizeTrigger(uint8_t deadzonePercent, uint8_t value, int16_t min, int16_t max)
{
    float real_deadzone = (max - min) * deadzonePercent / 100;

    if (value < real_deadzone)
        return 0;

    float magnitude = std::min((float)(max - min), (float)(value - min));
    magnitude -= real_deadzone;
    return magnitude / (max - min - real_deadzone);
}

void BaseController::NormalizeAxis(uint8_t x, uint8_t y, uint8_t deadzonePercent, float *x_out, float *y_out, int32_t min, int32_t max)
{
    //*x_out, *y_out is between -1.0 and 1.0
    float ratio = 0;
    float x_val = x;
    float y_val = y;
    int32_t range = max;

    if (min >= 0)
    {
        x_val = x - (max / 2);
        y_val = (max / 2) - y;
        range = (max / 2);
    }

    float real_magnitude = std::sqrt(x_val * x_val + y_val * y_val);
    float real_deadzone = (range * deadzonePercent) / 100;
    if (real_magnitude > real_deadzone)
    {
        float magnitude = std::min((float)range, real_magnitude);
        magnitude -= real_deadzone;
        ratio = (magnitude / (range - real_deadzone)) / real_magnitude;
    }
    else
    {
        *x_out = *y_out = 0.0f;
    }
    *x_out = x_val * ratio;
    *y_out = y_val * ratio;
}
