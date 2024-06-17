#include "SwitchUSBInterface.h"
#include "SwitchUSBEndpoint.h"
#include "SwitchUSBLock.h"
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
    SwitchUSBLock usbLock;

    ::syscon::logger::LogDebug("SwitchUSBInterface[%04x-%04x] Openning ...", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);

    R_TRY(usbHsAcquireUsbIf(&m_session, &m_interface));

    for (int i = 0; i < SWITCH_USB_MAX_ENDPOINTS; i++)
    {
        usb_endpoint_descriptor &epdesc = m_session.inf.inf.input_endpoint_descs[i];
        if (epdesc.bLength != 0)
        {
            ::syscon::logger::LogDebug("SwitchUSBInterface[%04x-%04x] Input endpoint found 0x%x (Idx: %d)", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct, epdesc.bEndpointAddress, i);
            m_inEndpoints[i] = std::make_unique<SwitchUSBEndpoint>(m_session, epdesc);
        }
        else
        {
            //::syscon::logger::LogWarning("SwitchUSBInterface[%x-%x] Input endpoint %d is null", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct, i);
        }
    }

    for (int i = 0; i < SWITCH_USB_MAX_ENDPOINTS; i++)
    {
        usb_endpoint_descriptor &epdesc = m_session.inf.inf.output_endpoint_descs[i];
        if (epdesc.bLength != 0)
        {
            ::syscon::logger::LogDebug("SwitchUSBInterface[%04x-%04x] Output endpoint found 0x%x (Idx: %d)", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct, epdesc.bEndpointAddress, i);
            m_outEndpoints[i] = std::make_unique<SwitchUSBEndpoint>(m_session, epdesc);
        }
        else
        {
            //::syscon::logger::LogWarning("SwitchUSBInterface(%x-%x) Output endpoint %d is null", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct, i);
        }
    }

    R_SUCCEED();
}

void SwitchUSBInterface::Close()
{
    ::syscon::logger::LogDebug("SwitchUSBInterface[%04x-%04x] Closing...", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);

    SwitchUSBLock usbLock;

    for (int i = 0; i < SWITCH_USB_MAX_ENDPOINTS; i++)
    {
        if (m_inEndpoints[i])
            m_inEndpoints[i]->Close();
        if (m_outEndpoints[i])
            m_outEndpoints[i]->Close();
    }

    usbHsIfClose(&m_session);
}

ams::Result SwitchUSBInterface::ControlTransferInput(u8 bmRequestType, u8 bmRequest, u16 wValue, u16 wIndex, void *buffer, u16 *wLength)
{
    SwitchUSBLock usbLock;

    ::syscon::logger::LogDebug("SwitchUSBInterface[%04x-%04x] ControlTransferInput (bmRequestType=0x%02X, bmRequest=0x%02X, wValue=0x%04X, wIndex=0x%04X, wLength=%d)...", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct, bmRequestType, bmRequest, wValue, wIndex, *wLength);

    if (!(bmRequestType & USB_ENDPOINT_IN))
    {
        ::syscon::logger::LogError("SwitchUSBInterface[%04x-%04x] ControlTransferInput: Trying to output data !", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);
        R_RETURN(CONTROL_ERR_INVALID_ARGUMENT);
    }

    u32 transferredSize = 0;

    ams::Result rc = usbHsIfCtrlXfer(&m_session, bmRequestType, bmRequest, wValue, wIndex, *wLength, m_usb_buffer, &transferredSize);
    if (R_SUCCEEDED(rc))
    {
        if (bmRequestType & USB_ENDPOINT_IN)
        {
            if (buffer != NULL && *wLength >= transferredSize)
            {
                memcpy(buffer, m_usb_buffer, transferredSize);
                *wLength = transferredSize;
            }
            else
            {
                ::syscon::logger::LogError("SwitchUSBInterface[%04x-%04x] ControlTransferInput: Invalid buffer size !", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);
                rc = CONTROL_ERR_INVALID_ARGUMENT;
            }
        }
    }

    return rc;
}

ams::Result SwitchUSBInterface::ControlTransferOutput(u8 bmRequestType, u8 bmRequest, u16 wValue, u16 wIndex, const void *buffer, u16 wLength)
{
    SwitchUSBLock usbLock;

    ::syscon::logger::LogDebug("SwitchUSBInterface[%04x-%04x] ControlTransferOutput (bmRequestType=0x%02X, bmRequest=0x%02X, wValue=0x%04X, wIndex=0x%04X, wLength=%d)...", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct, bmRequestType, bmRequest, wValue, wIndex, wLength);

    u32 transferredSize = 0;

    if (bmRequestType & USB_ENDPOINT_IN)
    {
        ::syscon::logger::LogError("SwitchUSBInterface[%04x-%04x] ControlTransferOutput Trying to read data !", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);
        R_RETURN(CONTROL_ERR_INVALID_ARGUMENT);
    }

    if (buffer != NULL && wLength > 0)
        memcpy(m_usb_buffer, buffer, wLength);

    R_TRY(usbHsIfCtrlXfer(&m_session, bmRequestType, bmRequest, wValue, wIndex, wLength, m_usb_buffer, &transferredSize));

    R_SUCCEED();
}

IUSBEndpoint *SwitchUSBInterface::GetEndpoint(IUSBEndpoint::Direction direction, uint8_t index)
{
    if (index >= SWITCH_USB_MAX_ENDPOINTS)
        return NULL;

    if (direction == IUSBEndpoint::USB_ENDPOINT_IN)
        return m_inEndpoints[index].get();
    else
        return m_outEndpoints[index].get();
}

ams::Result SwitchUSBInterface::Reset()
{
    SwitchUSBLock usbLock;

    ::syscon::logger::LogDebug("SwitchUSBInterface[%04x-%04x] Reset...", m_interface.device_desc.idVendor, m_interface.device_desc.idProduct);

    usbHsIfResetDevice(&m_session);

    R_SUCCEED();
}