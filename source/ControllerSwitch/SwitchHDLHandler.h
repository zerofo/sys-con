#pragma once

#include "switch.h"
#include "IController.h"
#include "SwitchVirtualGamepadHandler.h"

// HDLS stands for "HID (Human Interface Devices) Device List Setting".
//  It's a part of the Nintendo Switch's HID (Human Interface Devices) system module, which is responsible for handling input from controllers and
// other user interface devices. The HDLS structures and functions are used to manage and manipulate a list of virtual HID devices.

// Wrapper for HDL functions for switch versions [7.0.0+]

class SwitchHDLHandlerData
{
public:
    void reset()
    {
        m_npadId = HidNpadIdType_Other;
        m_hdlHandle.handle = 0;
        memset(&m_deviceInfo, 0, sizeof(m_deviceInfo));
        memset(&m_hdlState, 0, sizeof(m_hdlState));
        memset(&m_vibrationDeviceHandle, 0, sizeof(m_vibrationDeviceHandle));
    }

    HidNpadIdType m_npadId;
    HiddbgHdlsHandle m_hdlHandle;
    HiddbgHdlsDeviceInfo m_deviceInfo;
    HiddbgHdlsState m_hdlState;
    HidVibrationDeviceHandle m_vibrationDeviceHandle;
    HidVibrationValue m_vibrationLastValue;
    bool m_is_connected;
    bool m_is_sync;
};

class SwitchHDLHandler : public SwitchVirtualGamepadHandler
{
private:
    SwitchHDLHandlerData m_controllerData[CONTROLLER_MAX_INPUTS];

    ams::Result Detach(uint16_t input_idx);
    ams::Result Attach(uint16_t input_idx);

public:
    // Initialize the class with specified controller
    SwitchHDLHandler(std::unique_ptr<IController> &&controller, int polling_frequency_ms);
    ~SwitchHDLHandler();

    // Initialize controller handler, HDL state
    virtual ams::Result Initialize() override;
    virtual void Exit() override;

    // This will be called periodically by the input threads
    virtual void UpdateInput(s32 timeout_us) override;
    // This will be called periodically by the output threads
    virtual void UpdateOutput() override;

    // Separately init and close the HDL state
    ams::Result InitHdlState();
    ams::Result UninitHdlState();

    bool IsVirtualDeviceAttached(uint16_t input_idx);

    // Fills out the HDL state with the specified button data and passes it to HID
    ams::Result UpdateHdlState(const NormalizedButtonData &data, uint16_t input_idx);

    static HiddbgHdlsSessionId &GetHdlsSessionId();
};