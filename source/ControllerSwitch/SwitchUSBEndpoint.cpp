#include "SwitchUSBEndpoint.h"
#include "SwitchUSBLock.h"
#include "SwitchLogger.h"
#include <cstring>
#include <malloc.h>
#include <stratosphere.hpp>

SwitchUSBEndpoint::SwitchUSBEndpoint(UsbHsClientIfSession &if_session, usb_endpoint_descriptor &desc)
    : m_ifSession(&if_session),
      m_descriptor(&desc)
{
}

SwitchUSBEndpoint::~SwitchUSBEndpoint()
{
}

ControllerResult SwitchUSBEndpoint::Open(int maxPacketSize)
{
    SwitchUSBLock usbLock;

    maxPacketSize = maxPacketSize != 0 ? maxPacketSize : m_descriptor->wMaxPacketSize;

    ::syscon::logger::LogDebug("SwitchUSBEndpoint Opening 0x%x (Pkt size: %d)...", m_descriptor->bEndpointAddress, maxPacketSize);

    if (R_FAILED(usbHsIfOpenUsbEp(m_ifSession, &m_epSession, 1, maxPacketSize, m_descriptor)))
    {
        ::syscon::logger::LogError("SwitchUSBEndpoint failed to open! (0x%x)", m_descriptor->bEndpointAddress);
        return CONTROLLER_STATUS_USB_ENDPOINT_OPEN;
    }

    ::syscon::logger::LogDebug("SwitchUSBEndpoint successfully opened!");

    return CONTROLLER_STATUS_SUCCESS;
}

void SwitchUSBEndpoint::Close()
{
    SwitchUSBLock usbLock;

    usbHsEpClose(&m_epSession);
}

ControllerResult SwitchUSBEndpoint::Write(const uint8_t *inBuffer, size_t bufferSize)
{
    u32 transferredSize = 0;

    SwitchUSBLock usbLock;

    memcpy(m_usb_buffer_out, inBuffer, bufferSize);

    if (GetDirection() == USB_ENDPOINT_IN)
        ::syscon::logger::LogError("SwitchUSBEndpoint:: Trying to write an INPUT endpoint!");

    ::syscon::logger::LogTrace("SwitchUSBEndpoint: Write %d bytes", bufferSize);
    ::syscon::logger::LogBuffer(LOG_LEVEL_TRACE, m_usb_buffer_out, bufferSize);

    ams::Result rc = usbHsEpPostBuffer(&m_epSession, m_usb_buffer_out, bufferSize, &transferredSize);
    if (R_FAILED(rc))
    {
        ::syscon::logger::LogError("SwitchUSBEndpoint: Write failed: %08X", rc);
        return CONTROLLER_STATUS_WRITE_FAILED;
    }

    svcSleepThread(m_descriptor->bInterval * 1000000);

    return CONTROLLER_STATUS_SUCCESS;
}

ControllerResult SwitchUSBEndpoint::Read(uint8_t *outBuffer, size_t *bufferSizeInOut, u64 aTimeoutUs)
{
    SwitchUSBLock usbLock;

    if (GetDirection() == USB_ENDPOINT_OUT)
        ::syscon::logger::LogError("SwitchUSBEndpoint: Trying to read an OUTPUT endpoint!");

    if (aTimeoutUs == UINT64_MAX)
    {
        u32 transferredSize;

        ams::Result rc = usbHsEpPostBuffer(&m_epSession, m_usb_buffer_in, *bufferSizeInOut, &transferredSize);
        if (R_FAILED(rc))
        {
            ::syscon::logger::LogError("SwitchUSBEndpoint: Read failed: %08X", rc);
            return CONTROLLER_STATUS_READ_FAILED;
        }

        memcpy(outBuffer, m_usb_buffer_in, transferredSize);
        *bufferSizeInOut = transferredSize;

        if (transferredSize == 0)
        {
            ::syscon::logger::LogError("SwitchUSBEndpoint: Read returned no data !");
            return CONTROLLER_STATUS_NO_DATA_AVAILABLE;
        }

        ::syscon::logger::LogTrace("SwitchUSBEndpoint: Read %d bytes", *bufferSizeInOut);
        ::syscon::logger::LogBuffer(LOG_LEVEL_TRACE, outBuffer, *bufferSizeInOut);

        return CONTROLLER_STATUS_SUCCESS;
    }
    else
    {
        u32 count = 0;
        UsbHsXferReport report;
        u32 tmpXcferId = 0;
        ams::Result rc;

        if (m_xferIdRead == 0)
        {
            rc = usbHsEpPostBufferAsync(&m_epSession, m_usb_buffer_in, *bufferSizeInOut, 0, &m_xferIdRead);
            if (R_FAILED(rc))
            {
                ::syscon::logger::LogError("SwitchUSBEndpoint: ReadAsync failed: %08X", rc);
                return CONTROLLER_STATUS_READ_FAILED;
            }
        }

        if (R_FAILED(eventWait(usbHsEpGetXferEvent(&m_epSession), aTimeoutUs * 1000)))
            return CONTROLLER_STATUS_TIMEOUT;

        eventClear(usbHsEpGetXferEvent(&m_epSession));

        tmpXcferId = m_xferIdRead;
        m_xferIdRead = 0;

        memset(&report, 0, sizeof(report));
        rc = usbHsEpGetXferReport(&m_epSession, &report, 1, &count);
        if (R_FAILED(rc))
        {
            ::syscon::logger::LogError("SwitchUSBEndpoint: ReadAsync failed: %08X", rc);
            return CONTROLLER_STATUS_READ_FAILED;
        }

        if ((count <= 0) || (tmpXcferId != report.xferId))
        {
            ::syscon::logger::LogError("SwitchUSBEndpoint: ReadAsync failed (Invalid XFerId or NoData returned - Count: %d, xferId %d/%d)", count, tmpXcferId, report.xferId);
            return CONTROLLER_STATUS_NO_DATA_AVAILABLE;
        }

        memcpy(outBuffer, m_usb_buffer_in, report.transferredSize);
        *bufferSizeInOut = report.transferredSize;

        if (report.transferredSize == 0)
            return CONTROLLER_STATUS_NO_DATA_AVAILABLE;

        ::syscon::logger::LogTrace("SwitchUSBEndpoint: ReadAsync %d bytes", *bufferSizeInOut);
        ::syscon::logger::LogBuffer(LOG_LEVEL_TRACE, outBuffer, *bufferSizeInOut);

        if (R_FAILED(report.res))
        {
            ::syscon::logger::LogError("SwitchUSBEndpoint: ReadAsync failed: %08X", report.res);
            return CONTROLLER_STATUS_READ_FAILED;
        }

        return CONTROLLER_STATUS_SUCCESS;
    }

    return CONTROLLER_STATUS_INVALID_ARGUMENT;
}

IUSBEndpoint::Direction SwitchUSBEndpoint::GetDirection()
{
    return ((m_descriptor->bEndpointAddress & USB_ENDPOINT_IN) ? USB_ENDPOINT_IN : USB_ENDPOINT_OUT);
}

IUSBEndpoint::EndpointDescriptor *SwitchUSBEndpoint::GetDescriptor()
{
    return reinterpret_cast<IUSBEndpoint::EndpointDescriptor *>(m_descriptor);
}