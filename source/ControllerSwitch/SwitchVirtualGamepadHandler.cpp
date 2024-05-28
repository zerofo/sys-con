#include "SwitchVirtualGamepadHandler.h"
#include "SwitchLogger.h"

SwitchVirtualGamepadHandler::SwitchVirtualGamepadHandler(std::unique_ptr<IController> &&controller, int polling_frequency_ms)
    : m_controller(std::move(controller)),
      m_polling_frequency_ms(polling_frequency_ms)
{
}

SwitchVirtualGamepadHandler::~SwitchVirtualGamepadHandler()
{
}

ams::Result SwitchVirtualGamepadHandler::Initialize()
{
    return m_controller->Initialize();
}

void SwitchVirtualGamepadHandler::Exit()
{
    m_controller->Exit();
}

void SwitchVirtualGamepadHandler::ThreadLoop(void *handler)
{
    SwitchVirtualGamepadHandler *gamepadHandler = static_cast<SwitchVirtualGamepadHandler *>(handler);

    ::syscon::logger::LogDebug("SwitchVirtualGamepadHandler InputThreadLoop running ...");

    do
    {
        gamepadHandler->UpdateInput();

        gamepadHandler->UpdateOutput();

        svcSleepThread(gamepadHandler->m_polling_frequency_ms * 1000 * 1000);
    } while (gamepadHandler->m_ThreadIsRunning);

    ::syscon::logger::LogDebug("SwitchVirtualGamepadHandler InputThreadLoop stopped !");
}

ams::Result SwitchVirtualGamepadHandler::InitThread()
{
    m_ThreadIsRunning = true;
    R_ABORT_UNLESS(threadCreate(&m_Thread, &SwitchVirtualGamepadHandler::ThreadLoop, this, thread_stack, sizeof(thread_stack), 0x30, -2));
    R_ABORT_UNLESS(threadStart(&m_Thread));
    return 0;
}

void SwitchVirtualGamepadHandler::ExitThread()
{
    m_ThreadIsRunning = false;
    svcCancelSynchronization(m_Thread.handle);
    threadWaitForExit(&m_Thread);
    threadClose(&m_Thread);
}

static_assert(JOYSTICK_MAX == 32767 && JOYSTICK_MIN == -32767,
              "JOYSTICK_MAX and/or JOYSTICK_MIN has incorrect values. Update libnx");

void SwitchVirtualGamepadHandler::ConvertAxisToSwitchAxis(float x, float y, float deadzone, s32 *x_out, s32 *y_out)
{
    (void)deadzone;
    float floatRange = 2.0f;
    // JOYSTICK_MAX is 1 above and JOYSTICK_MIN is 1 below acceptable joystick values, causing crashes on various games including Xenoblade Chronicles 2 and Resident Evil 4
    float newRange = (JOYSTICK_MAX - JOYSTICK_MIN);

    *x_out = (((x + 1.0f) * newRange) / floatRange) + JOYSTICK_MIN;
    *y_out = -((((y + 1.0f) * newRange) / floatRange) + JOYSTICK_MIN);
}