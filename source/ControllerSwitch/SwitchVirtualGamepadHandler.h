#pragma once
#include "switch.h"
#include "IController.h"
#include <stratosphere.hpp>

// This class is a base class for SwitchHDLHandler and SwitchAbstractedPaadHandler.
class SwitchVirtualGamepadHandler
{
    friend void SwitchVirtualGamepadHandlerThreadFunc(void *arg);

protected:
    std::unique_ptr<IController> m_controller;
    s32 m_polling_frequency_ms;
    s32 m_read_input_timeout_us;

    alignas(ams::os::ThreadStackAlignment) u8 thread_stack[0x1000];
    Thread m_Thread;
    bool m_ThreadIsRunning = false;

    void onRun();

public:
    SwitchVirtualGamepadHandler(std::unique_ptr<IController> &&controller, s32 polling_frequency_ms);
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
    virtual void UpdateInput(s32 timeout_us) = 0;
    // The function to call indefinitely by the output thread
    virtual void UpdateOutput() = 0;

    void ConvertAxisToSwitchAxis(float x, float y, s32 *x_out, s32 *y_out);

    // Get the raw controller pointer
    inline IController *GetController() { return m_controller.get(); }
};