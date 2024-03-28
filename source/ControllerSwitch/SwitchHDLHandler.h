#pragma once

#include "switch.h"
#include "IController.h"
#include "SwitchVirtualGamepadHandler.h"

// Wrapper for HDL functions for switch versions [7.0.0+]
class SwitchHDLHandler : public SwitchVirtualGamepadHandler
{
private:
    HiddbgHdlsHandle m_hdlHandle;
    HiddbgHdlsDeviceInfo m_deviceInfo;
    HiddbgHdlsState m_hdlState;

public:
    // Initialize the class with specified controller
    SwitchHDLHandler(std::unique_ptr<IController> &&controller, int polling_frequency_ms);
    ~SwitchHDLHandler();

    // Initialize controller handler, HDL state
    virtual ams::Result Initialize() override;
    virtual void Exit() override;

    // This will be called periodically by the input threads
    virtual void UpdateInput() override;
    // This will be called periodically by the output threads
    virtual void UpdateOutput() override;

    // Separately init and close the HDL state
    Result InitHdlState();
    Result ExitHdlState();

    // Fills out the HDL state with the specified button data and passes it to HID
    Result UpdateHdlState(const NormalizedButtonData &data, uint16_t input_idx);

    static HiddbgHdlsSessionId &GetHdlsSessionId();
};