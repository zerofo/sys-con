#pragma once
#include "IUSBEndpoint.h"

enum usb_request_recipient
{
    USB_RECIPIENT_DEVICE = 0x00,
    USB_RECIPIENT_INTERFACE = 0x01,
    USB_RECIPIENT_ENDPOINT = 0x02,
    USB_RECIPIENT_OTHER = 0x03,
};

class IUSBInterface
{
protected:
public:
    struct InterfaceDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;   ///< Must match USB_DT_INTERFACE.
        uint8_t bInterfaceNumber;  ///< See also USBDS_DEFAULT_InterfaceNumber.
        uint8_t bAlternateSetting; ///< Must match 0.
        uint8_t bNumEndpoints;
        uint8_t bInterfaceClass;
        uint8_t bInterfaceSubClass;
        uint8_t bInterfaceProtocol;
        uint8_t iInterface; ///< Ignored.
    };
    virtual ~IUSBInterface() = default;

    virtual ams::Result Open() = 0;
    virtual void Close() = 0;

    virtual ams::Result ControlTransferInput(uint8_t bmRequestType, uint8_t bmRequest, uint16_t wValue, uint16_t wIndex, void *buffer, uint16_t *wLength) = 0;
    virtual ams::Result ControlTransferOutput(uint8_t bmRequestType, uint8_t bmRequest, uint16_t wValue, uint16_t wIndex, const void *buffer, uint16_t wLength) = 0;

    virtual IUSBEndpoint *GetEndpoint(IUSBEndpoint::Direction direction, uint8_t index) = 0;
    virtual InterfaceDescriptor *GetDescriptor() = 0;

    virtual ams::Result Reset() = 0;
};