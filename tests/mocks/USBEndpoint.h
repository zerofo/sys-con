#pragma once
#include "IUSBEndpoint.h"
#include <gmock/gmock.h>

class MockUSBEndpoint : public IUSBEndpoint
{
public:
    MockUSBEndpoint(Direction direction) : IUSBEndpoint(), m_direction(direction) {}
    ~MockUSBEndpoint() override {}

    MOCK_METHOD(ControllerResult, Open, (int maxPacketSize), (override));
    MOCK_METHOD(void, Close, (), (override));
    MOCK_METHOD(ControllerResult, Write, (const uint8_t *inBuffer, size_t bufferSize), (override));
    MOCK_METHOD(ControllerResult, Read, (uint8_t *outBuffer, size_t *bufferSizeInOut, uint64_t aTimeoutUs), (override));

    Direction GetDirection() override
    {
        return m_direction;
    }

    MOCK_METHOD(EndpointDescriptor *, GetDescriptor, (), (override));

private:
    Direction m_direction;
};
