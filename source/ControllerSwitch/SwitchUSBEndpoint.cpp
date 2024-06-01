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

ams::Result SwitchUSBEndpoint::Open(int maxPacketSize)
{
    maxPacketSize = maxPacketSize != 0 ? maxPacketSize : m_descriptor->wMaxPacketSize;

    ::syscon::logger::LogDebug("SwitchUSBEndpoint Opening 0x%x (Pkt size: %d)...", m_descriptor->bEndpointAddress, maxPacketSize);

    R_TRY(usbHsIfOpenUsbEp(m_ifSession, &m_epSession, 1, maxPacketSize, m_descriptor));

    ::syscon::logger::LogDebug("SwitchUSBEndpoint successfully opened!");

    R_SUCCEED();
}

void SwitchUSBEndpoint::Close()
{
    usbHsEpClose(&m_epSession);
}

ams::Result SwitchUSBEndpoint::Write(const void *inBuffer, size_t bufferSize)
{
    u32 transferredSize = 0;

    memcpy(m_usb_buffer_out, inBuffer, bufferSize);

    if (GetDirection() == USB_ENDPOINT_IN)
        ::syscon::logger::LogError("SwitchUSBEndpoint::Write: Trying to write on an IN endpoint!");

    R_TRY(usbHsEpPostBuffer(&m_epSession, m_usb_buffer_out, bufferSize, &transferredSize));

    svcSleepThread(m_descriptor->bInterval * 1000000);

    R_SUCCEED();
}

ams::Result SwitchUSBEndpoint::Read(void *outBuffer, size_t *bufferSizeInOut, Mode mode)
{
    if (GetDirection() == USB_ENDPOINT_OUT)
        ::syscon::logger::LogError("SwitchUSBEndpoint::Read: Trying to read on an OUT endpoint!");

    if (mode & IUSBEndpoint::USB_MODE_BLOCKING)
    {
        u32 transferredSize;
        R_TRY(usbHsEpPostBuffer(&m_epSession, m_usb_buffer_in, *bufferSizeInOut, &transferredSize));

        memcpy(outBuffer, m_usb_buffer_in, transferredSize);
        *bufferSizeInOut = transferredSize;

        R_SUCCEED()
    }

    if (mode & IUSBEndpoint::USB_MODE_NON_BLOCKING)
    {
        u32 count = 0;
        UsbHsXferReport report;

        if (m_xferIdRead == 0)
            R_TRY(usbHsEpPostBufferAsync(&m_epSession, m_usb_buffer_in, *bufferSizeInOut, 0, &m_xferIdRead))

        R_TRY(eventWait(usbHsEpGetXferEvent(&m_epSession), 1)); // Wait 1nS (Indeed it's a blocking call, but it's the only way to get the data asynchronously.)

        eventClear(usbHsEpGetXferEvent(&m_epSession));
        m_xferIdRead = 0;

        memset(&report, 0, sizeof(report));
        R_TRY(usbHsEpGetXferReport(&m_epSession, &report, 1, &count));

        if ((count <= 0) || (m_xferIdRead != report.xferId))
            R_RETURN(CONTROL_ERR_NO_DATA_AVAILABLE);

        memcpy(outBuffer, m_usb_buffer_in, report.transferredSize);
        *bufferSizeInOut = report.transferredSize;
        return report.res;
    }

    ::syscon::logger::LogError("SwitchUSBEndpoint::Read: Invalid mode provided: 0x%02X", mode);
    R_RETURN(CONTROL_ERR_INVALID_ARGUMENT);
}

IUSBEndpoint::Direction SwitchUSBEndpoint::GetDirection()
{
    return ((m_descriptor->bEndpointAddress & USB_ENDPOINT_IN) ? USB_ENDPOINT_IN : USB_ENDPOINT_OUT);
}

IUSBEndpoint::EndpointDescriptor *SwitchUSBEndpoint::GetDescriptor()
{
    return reinterpret_cast<IUSBEndpoint::EndpointDescriptor *>(m_descriptor);
}