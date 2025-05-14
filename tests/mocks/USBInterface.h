#pragma once
#include "IUSBInterface.h"
#include <gmock/gmock.h>

class MockUSBInterface : public IUSBInterface
{
public:
    MockUSBInterface(std::unique_ptr<IUSBEndpoint> &&input, std::unique_ptr<IUSBEndpoint> &&output) : IUSBInterface()
    {
        m_inEndpoint = std::move(input);
        m_outEndpoint = std::move(output);
    }
    ~MockUSBInterface() override {}

    MOCK_METHOD(ControllerResult, Open, (), (override));
    MOCK_METHOD(void, Close, (), (override));
    MOCK_METHOD(ControllerResult, ControlTransferInput, (uint8_t bmRequestType, uint8_t bmRequest, uint16_t wValue, uint16_t wIndex, void *buffer, uint16_t *wLength), (override));
    MOCK_METHOD(ControllerResult, ControlTransferOutput, (uint8_t bmRequestType, uint8_t bmRequest, uint16_t wValue, uint16_t wIndex, const void *buffer, uint16_t wLength), (override));

    IUSBEndpoint *GetEndpoint(IUSBEndpoint::Direction direction, uint8_t index) override
    {
        if (direction == IUSBEndpoint::USB_ENDPOINT_IN && index == 0)
        {
            return m_inEndpoint.get();
        }
        else if (direction == IUSBEndpoint::USB_ENDPOINT_OUT && index == 0)
        {
            return m_outEndpoint.get();
        }
        return nullptr;
    }
    MOCK_METHOD(InterfaceDescriptor *, GetDescriptor, (), (override));
    MOCK_METHOD(ControllerResult, Reset, (), (override));

private:
    std::unique_ptr<IUSBEndpoint> m_inEndpoint;
    std::unique_ptr<IUSBEndpoint> m_outEndpoint;
};
