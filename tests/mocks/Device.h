#pragma once
#include "IUSBDevice.h"

class MockDevice : public IUSBDevice
{
public:
    ControllerResult Open() override { return CONTROLLER_STATUS_SUCCESS; }
    void Close() override {}
    void Reset() override {}
};
