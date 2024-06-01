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
        LogPrint(LogLevelDebug, "BaseController Opening interface %d ...", interfaceCount++);

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

ams::Result BaseController::SetRumble(uint16_t input_idx, float amp_high, float amp_low)
{
    (void)input_idx;
    (void)amp_high;
    (void)amp_low;
    // Not implemented yet
    return 9;
}

float BaseController::Normalize(uint8_t deadzonePercent, int32_t value, int32_t min, int32_t max)
{
    float range = (max - min) / 2;
    float offset = range;

    if (range == max)
        offset = 0;

    float deadzone = range * deadzonePercent / 100;

    int32_t value_tmp = value - offset;
    if (std::abs(value_tmp) < deadzone)
        return 0.0f;

    float ret = value_tmp / range;

    if (ret > 1.0f)
        ret = 1.0f;
    else if (ret < -1.0f)
        ret = -1.0f;

    return ret;
}