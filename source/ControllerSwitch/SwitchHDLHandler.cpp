#include "SwitchHDLHandler.h"
#include "SwitchLogger.h"
#include <cmath>

static HiddbgHdlsSessionId g_hdlsSessionId;

SwitchHDLHandler::SwitchHDLHandler(std::unique_ptr<IController> &&controller, int polling_frequency_ms)
    : SwitchVirtualGamepadHandler(std::move(controller), polling_frequency_ms)
{
}

SwitchHDLHandler::~SwitchHDLHandler()
{
    Exit();
}

ams::Result SwitchHDLHandler::Initialize()
{
    syscon::logger::LogDebug("SwitchHDLHandler Initializing ...");

    R_TRY(m_controller->Initialize());

    if (GetController()->Support(SUPPORTS_NOTHING))
        return 0;

    R_TRY(InitHdlState());

    R_TRY(InitThread());

    syscon::logger::LogDebug("SwitchHDLHandler Initialized !");

    return 0;
}

void SwitchHDLHandler::Exit()
{
    if (GetController()->Support(SUPPORTS_NOTHING))
    {
        m_controller->Exit();
        return;
    }

    ExitThread();
    m_controller->Exit();
    UninitHdlState();
}

ams::Result SwitchHDLHandler::InitHdlState()
{
    syscon::logger::LogDebug("SwitchHDLHandler Initializing HDL state ...");

    for (int i = 0; i < m_controller->GetInputCount(); i++)
    {
        m_hdlHandle[i] = {0};
        m_deviceInfo[i] = {0};
        m_hdlState[i] = {0};

        syscon::logger::LogDebug("SwitchHDLHandler Initializing HDL device idx: %d ...", i);

        // Set the controller type to Pro-Controller, and set the npadInterfaceType.
        m_deviceInfo[i].deviceType = HidDeviceType_FullKey15;
        m_deviceInfo[i].npadInterfaceType = HidNpadInterfaceType_USB;

        // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
        m_deviceInfo[i].singleColorBody = __builtin_bswap32(m_controller->GetConfig().bodyColor.rgbaValue);
        m_deviceInfo[i].singleColorButtons = __builtin_bswap32(m_controller->GetConfig().buttonsColor.rgbaValue);
        m_deviceInfo[i].colorLeftGrip = __builtin_bswap32(m_controller->GetConfig().leftGripColor.rgbaValue);
        m_deviceInfo[i].colorRightGrip = __builtin_bswap32(m_controller->GetConfig().rightGripColor.rgbaValue);

        m_hdlState[i].battery_level = 4; // Set battery charge to full.
        m_hdlState[i].analog_stick_l.x = 0x1234;
        m_hdlState[i].analog_stick_l.y = -0x1234;
        m_hdlState[i].analog_stick_r.x = 0x5678;
        m_hdlState[i].analog_stick_r.y = -0x5678;

        if (m_controller->IsControllerActive(i))
        {
            syscon::logger::LogDebug("SwitchHDLHandler hiddbgAttachHdlsVirtualDevice ...");
            R_TRY(hiddbgAttachHdlsVirtualDevice(&m_hdlHandle[i], &m_deviceInfo[i]));
        }
    }

    syscon::logger::LogDebug("SwitchHDLHandler HDL state successfully initialized !");
    R_SUCCEED();
}

ams::Result SwitchHDLHandler::UninitHdlState()
{
    syscon::logger::LogDebug("SwitchHDLHandler UninitHdlState .. !");

    for (int i = 0; i < m_controller->GetInputCount(); i++)
    {
        if (IsVirtualDeviceAttached(i))
            hiddbgDetachHdlsVirtualDevice(m_hdlHandle[i]);
    }

    R_SUCCEED();
}

bool SwitchHDLHandler::IsVirtualDeviceAttached(uint16_t input_idx)
{
    syscon::logger::LogDebug("SwitchHDLHandler handle: %d ...", m_hdlHandle[input_idx].handle);

    return m_hdlHandle[input_idx].handle != 0;
}

// Sets the state of the class's HDL controller to the state stored in class's hdl.state
ams::Result SwitchHDLHandler::UpdateHdlState(const NormalizedButtonData &data, uint16_t input_idx)
{
    HiddbgHdlsState *hdlState = &m_hdlState[input_idx];

    if (!IsVirtualDeviceAttached(input_idx))
    {
        syscon::logger::LogDebug("SwitchHDLHandler hiddbgAttachHdlsVirtualDevice ...");
        hiddbgAttachHdlsVirtualDevice(&m_hdlHandle[input_idx], &m_deviceInfo[input_idx]);
    }

    // we convert the input packet into switch-specific button states
    hdlState->buttons = 0;

    if (data.buttons[ControllerButton::X])
        hdlState->buttons |= HidNpadButton_X;
    if (data.buttons[ControllerButton::A])
        hdlState->buttons |= HidNpadButton_A;
    if (data.buttons[ControllerButton::B])
        hdlState->buttons |= HidNpadButton_B;
    if (data.buttons[ControllerButton::Y])
        hdlState->buttons |= HidNpadButton_Y;
    if (data.buttons[ControllerButton::LSTICK_CLICK])
        hdlState->buttons |= HidNpadButton_StickL;
    if (data.buttons[ControllerButton::RSTICK_CLICK])
        hdlState->buttons |= HidNpadButton_StickR;
    if (data.buttons[ControllerButton::L])
        hdlState->buttons |= HidNpadButton_L;
    if (data.buttons[ControllerButton::R])
        hdlState->buttons |= HidNpadButton_R;
    if (data.buttons[ControllerButton::ZL])
        hdlState->buttons |= HidNpadButton_ZL;
    if (data.buttons[ControllerButton::ZR])
        hdlState->buttons |= HidNpadButton_ZR;
    if (data.buttons[ControllerButton::MINUS])
        hdlState->buttons |= HidNpadButton_Minus;
    if (data.buttons[ControllerButton::PLUS])
        hdlState->buttons |= HidNpadButton_Plus;
    if (data.buttons[ControllerButton::DPAD_UP])
        hdlState->buttons |= HidNpadButton_Up;
    if (data.buttons[ControllerButton::DPAD_RIGHT])
        hdlState->buttons |= HidNpadButton_Right;
    if (data.buttons[ControllerButton::DPAD_DOWN])
        hdlState->buttons |= HidNpadButton_Down;
    if (data.buttons[ControllerButton::DPAD_LEFT])
        hdlState->buttons |= HidNpadButton_Left;
    if (data.buttons[ControllerButton::CAPTURE])
        hdlState->buttons |= HiddbgNpadButton_Capture;
    if (data.buttons[ControllerButton::HOME])
        hdlState->buttons |= HiddbgNpadButton_Home;

    ConvertAxisToSwitchAxis(data.sticks[0].axis_x, data.sticks[0].axis_y, 0, &hdlState->analog_stick_l.x, &hdlState->analog_stick_l.y);
    ConvertAxisToSwitchAxis(data.sticks[1].axis_x, data.sticks[1].axis_y, 0, &hdlState->analog_stick_r.x, &hdlState->analog_stick_r.y);

    syscon::logger::LogDebug("SwitchHDLHandler UpdateHdlState - Idx: %d [Button: 0x%016X LeftX: %d LeftY: %d RightX: %d RightY: %d]", input_idx, hdlState->buttons, hdlState->analog_stick_l.x, hdlState->analog_stick_l.y, hdlState->analog_stick_r.x, hdlState->analog_stick_r.y);

    R_TRY(hiddbgSetHdlsState(m_hdlHandle[input_idx], hdlState));

    R_SUCCEED();
}

void SwitchHDLHandler::UpdateInput()
{
    NormalizedButtonData data = {0};
    uint16_t input_idx = 0;

    // We process any input packets here. If it fails, return and try again
    if (R_FAILED(m_controller->ReadInput(&data, &input_idx)))
        return;

    // This is a check for controllers that can prompt themselves to go inactive - e.g. wireless Xbox 360 controllers
    if (!m_controller->IsControllerActive(input_idx))
    {
        if (IsVirtualDeviceAttached(input_idx))
            hiddbgDetachHdlsVirtualDevice(m_hdlHandle[input_idx]);
    }
    else
    {
        // We get the button inputs from the input packet and update the state of our controller
        if (R_FAILED(UpdateHdlState(data, input_idx)))
            return;
    }
}

void SwitchHDLHandler::UpdateOutput()
{
    // Process rumble values if supported
    /*if (GetController()->Support(SUPPORTS_RUMBLE))
    {
        HidVibrationValue value;

        for (int i = 0; i < m_controller->GetInputCount(); i++)
        {
            if (R_FAILED(hidGetActualVibrationValue(m_vibrationDeviceHandle[i], &value))
                return;

            m_controller->SetRumble(static_cast<uint8_t>(value.amp_high * 255.0f), static_cast<uint8_t>(value.amp_low * 255.0f));
        }
    }*/
}

HiddbgHdlsSessionId &SwitchHDLHandler::GetHdlsSessionId()
{
    return g_hdlsSessionId;
}