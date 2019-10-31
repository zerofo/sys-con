#include "SwitchHDLHandler.h"
#include <cmath>

SwitchHDLHandler::SwitchHDLHandler(std::unique_ptr<IController> &&controller)
    : SwitchVirtualGamepadHandler(std::move(controller))
{
}

SwitchHDLHandler::~SwitchHDLHandler()
{
    Exit();
}

Result SwitchHDLHandler::Initialize()
{
    Result rc = m_controllerHandler.Initialize();
    if (R_FAILED(rc))
        return rc;

    rc = InitHdlState();
    if (R_FAILED(rc))
        return rc;

    if (R_SUCCEEDED(hidInitializeVibrationDevices(&m_vibrationDeviceHandle, 1, CONTROLLER_PLAYER_2, static_cast<HidControllerType>(TYPE_PROCONTROLLER | TYPE_SYSTEM_EXT))))
    {
        InitOutputThread();
    }

    InitInputThread();
    return rc;
}

void SwitchHDLHandler::Exit()
{
    m_controllerHandler.Exit();
    ExitInputThread();
    ExitOutputThread();
    ExitHdlState();
}

Result SwitchHDLHandler::InitHdlState()
{
    m_hdlHandle = 0;
    m_deviceInfo = {0};
    m_hdlState = {0};

    // Set the controller type to Pro-Controller, and set the npadInterfaceType.
    m_deviceInfo.deviceType = HidDeviceType_FullKey3;
    m_deviceInfo.npadInterfaceType = NpadInterfaceType_USB;
    // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
    m_deviceInfo.singleColorBody = RGBA8_MAXALPHA(107, 107, 107);
    m_deviceInfo.singleColorButtons = RGBA8_MAXALPHA(0, 0, 0);
    m_deviceInfo.colorLeftGrip = RGBA8_MAXALPHA(23, 125, 62);
    m_deviceInfo.colorRightGrip = RGBA8_MAXALPHA(23, 125, 62);

    m_hdlState.batteryCharge = 4; // Set battery charge to full.
    m_hdlState.joysticks[JOYSTICK_LEFT].dx = 0x1234;
    m_hdlState.joysticks[JOYSTICK_LEFT].dy = -0x1234;
    m_hdlState.joysticks[JOYSTICK_RIGHT].dx = 0x5678;
    m_hdlState.joysticks[JOYSTICK_RIGHT].dy = -0x5678;

    return hiddbgAttachHdlsVirtualDevice(&m_hdlHandle, &m_deviceInfo);
}
Result SwitchHDLHandler::ExitHdlState()
{
    return hiddbgDetachHdlsVirtualDevice(m_hdlHandle);
}

//Sets the state of the class's HDL controller to the state stored in class's hdl.state
Result SwitchHDLHandler::UpdateHdlState()
{
    //Checks if the virtual device was erased, in which case re-attach the device
    bool found_flag = false;
    HiddbgHdlsNpadAssignment list;
    hiddbgDumpHdlsNpadAssignmentState(&list);
    for (int i = 0; i != list.total_entries; ++i)
    {
        if (list.entries[i].HdlsHandle == m_hdlHandle)
        {
            found_flag = true;
            break;
        }
    }
    if (!found_flag)
        hiddbgAttachHdlsVirtualDevice(&m_hdlHandle, &m_deviceInfo);

    return hiddbgSetHdlsState(m_hdlHandle, &m_hdlState);
}

void SwitchHDLHandler::FillHdlState(const NormalizedButtonData &data)
{
    m_hdlState.buttons = 0;

    m_hdlState.buttons |= (data.right_action ? KEY_A : 0);
    m_hdlState.buttons |= (data.bottom_action ? KEY_B : 0);
    m_hdlState.buttons |= (data.top_action ? KEY_X : 0);
    m_hdlState.buttons |= (data.left_action ? KEY_Y : 0);

    m_hdlState.buttons |= (data.left_stick_click ? KEY_LSTICK : 0);
    m_hdlState.buttons |= (data.right_stick_click ? KEY_RSTICK : 0);

    m_hdlState.buttons |= (data.left_bumper ? KEY_L : 0);
    m_hdlState.buttons |= (data.right_bumper ? KEY_R : 0);

    m_hdlState.buttons |= ((data.left_trigger > 0.0f) ? KEY_ZL : 0);
    m_hdlState.buttons |= ((data.right_trigger > 0.0f) ? KEY_ZR : 0);

    m_hdlState.buttons |= (data.start ? KEY_PLUS : 0);
    m_hdlState.buttons |= (data.back ? KEY_MINUS : 0);

    m_hdlState.buttons |= (data.dpad_left ? KEY_DLEFT : 0);
    m_hdlState.buttons |= (data.dpad_up ? KEY_DUP : 0);
    m_hdlState.buttons |= (data.dpad_right ? KEY_DRIGHT : 0);
    m_hdlState.buttons |= (data.dpad_down ? KEY_DDOWN : 0);

    m_hdlState.buttons |= (data.capture ? KEY_CAPTURE : 0);
    m_hdlState.buttons |= (data.home ? KEY_HOME : 0);
    m_hdlState.buttons |= (data.guide ? KEY_HOME : 0);

    m_controllerHandler.ConvertAxisToSwitchAxis(data.left_stick_x, data.left_stick_y, 0, &m_hdlState.joysticks[JOYSTICK_LEFT].dx, &m_hdlState.joysticks[JOYSTICK_LEFT].dy);
    m_controllerHandler.ConvertAxisToSwitchAxis(data.right_stick_x, data.right_stick_y, 0, &m_hdlState.joysticks[JOYSTICK_RIGHT].dx, &m_hdlState.joysticks[JOYSTICK_RIGHT].dy);
}

void SwitchHDLHandler::UpdateInput()
{
    Result rc;
    rc = GetController()->GetInput();
    if (R_FAILED(rc))
        return;

    FillHdlState(GetController()->GetNormalizedButtonData());
    rc = UpdateHdlState();
    if (R_FAILED(rc))
        return;
}

void SwitchHDLHandler::UpdateOutput()
{
    //Implement rumble here
    Result rc;
    HidVibrationValue value;
    rc = hidGetActualVibrationValue(&m_vibrationDeviceHandle, &value);
    if (R_FAILED(rc))
        return;

    rc = GetController()->SetRumble(static_cast<uint8_t>(value.amp_high * 255.0f), static_cast<uint8_t>(value.amp_low * 255.0f));

    svcSleepThread(1e+7L);
}