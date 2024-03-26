#include "SwitchUSBEndpoint.h"
#include "SwitchLogger.h"
#include <cstring>
#include <malloc.h>

SwitchUSBEndpoint::SwitchUSBEndpoint(UsbHsClientIfSession &if_session, usb_endpoint_descriptor &desc)
    : m_ifSession(&if_session),
      m_descriptor(&desc)
{
}

SwitchUSBEndpoint::~SwitchUSBEndpoint()
{
}

Result SwitchUSBEndpoint::Open(int maxPacketSize)
{
    maxPacketSize = maxPacketSize != 0 ? maxPacketSize : m_descriptor->wMaxPacketSize;

    ::syscon::logger::LogDebug("SwitchUSBEndpoint Opening 0x%x (Pkt size: %d)...", m_descriptor->bEndpointAddress, maxPacketSize);

    Result rc = usbHsIfOpenUsbEp(m_ifSession, &m_epSession, 1, maxPacketSize, m_descriptor);
    if (R_FAILED(rc))
        return 73011;

    ::syscon::logger::LogDebug("SwitchUSBEndpoint successfully opened!");

    return rc;
}

void SwitchUSBEndpoint::Close()
{
    usbHsEpClose(&m_epSession);
}

Result SwitchUSBEndpoint::Write(const void *inBuffer, size_t bufferSize)
{
    u32 transferredSize = 0;

    memcpy(m_usb_buffer, inBuffer, bufferSize);

    if (GetDirection() == USB_ENDPOINT_IN)
        ::syscon::logger::LogError("SwitchUSBEndpoint::Write: Trying to write on an IN endpoint!");

    Result rc = usbHsEpPostBuffer(&m_epSession, m_usb_buffer, bufferSize, &transferredSize);

    //::syscon::logger::LogDebug("SwitchUSBEndpoint::Write: bufferSize: %d transferredSize: %d error_module: %x error_desc: %x interval: %d", bufferSize, transferredSize, R_MODULE(rc), R_DESCRIPTION(rc), m_descriptor->bInterval);

    if (R_SUCCEEDED(rc))
    {
        svcSleepThread(m_descriptor->bInterval * 1000000); //*2
    }
    else
    {
        ::syscon::logger::LogError("SwitchUSBEndpoint::Write: Failed to write on endpoint %x (Error: %x)", m_descriptor->bEndpointAddress, rc);
    }
    return rc;
}

Result SwitchUSBEndpoint::Read(void *outBuffer, size_t *bufferSizeInOut)
{
    u32 transferredSize;

    if (GetDirection() == USB_ENDPOINT_OUT)
        ::syscon::logger::LogError("SwitchUSBEndpoint::Read: Trying to read on an OUT endpoint!");

    Result rc = usbHsEpPostBuffer(&m_epSession, m_usb_buffer, *bufferSizeInOut, &transferredSize);

    //::syscon::logger::LogDebug("SwitchUSBEndpoint::Read: bufferSize: %d transferredSize: %d error_module: %x error_desc: %x", bufferSize, transferredSize, R_MODULE(rc), R_DESCRIPTION(rc));

    if (R_SUCCEEDED(rc))
    {
        memcpy(outBuffer, m_usb_buffer, transferredSize);
        *bufferSizeInOut = transferredSize;
    }
    else
    {
        ::syscon::logger::LogError("SwitchUSBEndpoint::Read: Failed to Read on endpoint %x (Error: %x)", m_descriptor->bEndpointAddress, rc);
        *bufferSizeInOut = 0;
    }
    return rc;
}

IUSBEndpoint::Direction SwitchUSBEndpoint::GetDirection()
{
    return ((m_descriptor->bEndpointAddress & USB_ENDPOINT_IN) ? USB_ENDPOINT_IN : USB_ENDPOINT_OUT);
}

IUSBEndpoint::EndpointDescriptor *SwitchUSBEndpoint::GetDescriptor()
{
    return reinterpret_cast<IUSBEndpoint::EndpointDescriptor *>(m_descriptor);
}