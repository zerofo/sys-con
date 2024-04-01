#pragma once
#include "switch.h"
#include "IController.h"
#include <stratosphere.hpp>

// This class is a base class for SwitchHDLHandler and SwitchAbstractedPaadHandler.
class SwitchVirtualGamepadHandler
{
protected:
    HidVibrationDeviceHandle m_vibrationDeviceHandle[CONTROLLER_MAX_INPUTS];
    std::unique_ptr<IController> m_controller;
    int m_polling_frequency_ms;

    alignas(ams::os::ThreadStackAlignment) u8 thread_stack[0x1000];

    Thread m_Thread;

    bool m_ThreadIsRunning = false;

    static void ThreadLoop(void *argument);

public:
    SwitchVirtualGamepadHandler(std::unique_ptr<IController> &&controller, int polling_frequency_ms);
    virtual ~SwitchVirtualGamepadHandler();

    // Override this if you want a custom init procedure
    virtual ams::Result Initialize();
    // Override this if you want a custom exit procedure
    virtual void Exit();

    // Separately init the input-reading thread
    ams::Result InitThread();
    // Separately close the input-reading thread
    void ExitThread();

    // The function to call indefinitely by the input thread
    virtual void UpdateInput() = 0;
    // The function to call indefinitely by the output thread
    virtual void UpdateOutput() = 0;

    void ConvertAxisToSwitchAxis(float x, float y, float deadzone, s32 *x_out, s32 *y_out);

    ams::Result SetControllerVibration(float strong_mag, float weak_mag);

    // Get the raw controller pointer
    inline IController *GetController() { return m_controller.get(); }
};