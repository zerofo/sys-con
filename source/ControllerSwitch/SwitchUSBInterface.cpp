#include "SwitchUSBInterface.h"
#include "SwitchUSBEndpoint.h"
#include "SwitchLogger.h"
#include <malloc.h>
#include <cstring>

SwitchUSBInterface::SwitchUSBInterface(UsbHsInterface &interface)
    : m_interface(interface)
{
}

SwitchUSBInterface::~SwitchUSBInterface()
{
}

ams::Result SwitchUSBInterface::Open()
{
    ::syscon::logger::LogDebug("SwitchUSBInterface Openning %x-%x...", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);

    R_TRY(usbHsAcquireUsbIf(&m_session, &m_interface));

    for (int i = 0; i < 15; i++)
    {
        usb_endpoint_descriptor &epdesc = m_session.inf.inf.input_endpoint_descs[i];
        if (epdesc.bLength != 0)
        {
            ::syscon::logger::LogDebug("SwitchUSBInterface: Input endpoint found 0x%x (Idx: %d)", epdesc.bEndpointAddress, i);
            m_inEndpoints[i] = std::make_unique<SwitchUSBEndpoint>(m_session, epdesc);
        }
        else
        {
            //::syscon::logger::LogWarning("SwitchUSBInterface: Input endpoint %d is null", i);
        }
    }

    for (int i = 0; i < 15; i++)
    {
        usb_endpoint_descriptor &epdesc = m_session.inf.inf.output_endpoint_descs[i];
        if (epdesc.bLength != 0)
        {
            ::syscon::logger::LogDebug("SwitchUSBInterface: Output endpoint found %x (Idx: %d)", epdesc.bEndpointAddress, i);
            m_outEndpoints[i] = std::make_unique<SwitchUSBEndpoint>(m_session, epdesc);
        }
        else
        {
            //::syscon::logger::LogWarning("SwitchUSBInterface: Output endpoint %d is null", i);
        }
    }

    R_SUCCEED();
}

void SwitchUSBInterface::Close()
{
    ::syscon::logger::LogDebug("SwitchUSBInterface Closing %x-%x...", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);

    for (int i = 0; i < 15; i++)
    {
        if (m_inEndpoints[i])
            m_inEndpoints[i]->Close();
        if (m_outEndpoints[i])
            m_outEndpoints[i]->Close();
    }

    usbHsIfClose(&m_session);
}

ams::Result SwitchUSBInterface::ControlTransfer(u8 bmRequestType, u8 bmRequest, u16 wValue, u16 wIndex, u16 wLength, void *buffer)
{
    ::syscon::logger::LogDebug("SwitchUSBInterface ControlTransfer %x-%x...", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);

    u32 transferredSize;

    memcpy(m_usb_buffer, buffer, wLength);

    ams::Result rc = usbHsIfCtrlXfer(&m_session, bmRequestType, bmRequest, wValue, wIndex, wLength, m_usb_buffer, &transferredSize);

    if (R_SUCCEEDED(rc))
    {
        memcpy(buffer, m_usb_buffer, transferredSize);
    }

    return rc;
}

ams::Result SwitchUSBInterface::ControlTransfer(u8 bmRequestType, u8 bmRequest, u16 wValue, u16 wIndex, u16 wLength, const void *buffer)
{
    ::syscon::logger::LogDebug("SwitchUSBInterface ControlTransfer %x-%x...", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);

    u32 transferredSize;

    memcpy(m_usb_buffer, buffer, wLength);

    R_TRY(usbHsIfCtrlXfer(&m_session, bmRequestType, bmRequest, wValue, wIndex, wLength, m_usb_buffer, &transferredSize));

    R_SUCCEED();
}

IUSBEndpoint *SwitchUSBInterface::GetEndpoint(IUSBEndpoint::Direction direction, uint8_t index)
{
    if (direction == IUSBEndpoint::USB_ENDPOINT_IN)
        return m_inEndpoints[index].get();
    else
        return m_outEndpoints[index].get();
}

ams::Result SwitchUSBInterface::Reset()
{
    ::syscon::logger::LogDebug("SwitchUSBInterface Reset %x-%x...", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);

    usbHsIfResetDevice(&m_session);

    R_SUCCEED();
}