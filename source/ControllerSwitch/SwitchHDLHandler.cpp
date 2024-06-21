#include "SwitchHDLHandler.h"
#include "SwitchLogger.h"
#include <cmath>

static HiddbgHdlsSessionId g_hdlsSessionId;

static uint32_t GetHidNpadMask()
{
    uint32_t HidNpadMask = 0;
    for (HidNpadIdType i = HidNpadIdType_No1; i <= HidNpadIdType_No8; i = (HidNpadIdType)((int)i + 1))
    {
        u32 deviceType = hidGetNpadDeviceType(i);
        if (deviceType == 0)
            continue;

        HidNpadMask |= 1 << i;
    }

    return HidNpadMask;
}

SwitchHDLHandler::SwitchHDLHandler(std::unique_ptr<IController> &&controller, int polling_frequency_ms)
    : SwitchVirtualGamepadHandler(std::move(controller), polling_frequency_ms)
{
    for (int i = 0; i < CONTROLLER_MAX_INPUTS; i++)
        m_controllerData[i].m_is_sync = true;
}

SwitchHDLHandler::~SwitchHDLHandler()
{
    Exit();
}

ams::Result SwitchHDLHandler::Initialize()
{
    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Initializing ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());

    R_TRY(SwitchVirtualGamepadHandler::Initialize());

    R_TRY(InitHdlState());

    R_TRY(InitThread());

    syscon::logger::LogInfo("SwitchHDLHandler[%04x-%04x] Initialized !", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());

    R_SUCCEED();
}

void SwitchHDLHandler::Exit()
{
    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Exiting ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());

    SwitchVirtualGamepadHandler::Exit();

    UninitHdlState();

    syscon::logger::LogInfo("SwitchHDLHandler[%04x-%04x] Uninitialized !", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());
}

ams::Result SwitchHDLHandler::Attach(uint16_t input_idx)
{
    if (IsVirtualDeviceAttached(input_idx))
        R_SUCCEED();

    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Attaching device for input: %d ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), input_idx);

    uint32_t HidNpadBefore = GetHidNpadMask();
    R_TRY(hiddbgAttachHdlsVirtualDevice(&m_controllerData[input_idx].m_hdlHandle, &m_controllerData[input_idx].m_deviceInfo));

    syscon::logger::LogTrace("SwitchHDLHandler[%04x-%04x] Searching for NpadId ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());
    // Wait until the controller is attached to a HidNpadIdType_xxx
    uint32_t HidNpadDiff = 0;
    for (int i = 0; i < 1000; i++) // Timeout after 1 second
    {
        uint32_t HidNpadAfter = GetHidNpadMask();
        HidNpadDiff = (HidNpadBefore ^ HidNpadAfter);
        if (HidNpadDiff != 0)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Detect which HidNpadIdType_xxx has been assigned to the controller
    for (HidNpadIdType i = HidNpadIdType_No1; i <= HidNpadIdType_No8; i = (HidNpadIdType)((int)i + 1))
    {
        if ((HidNpadDiff & (1 << (int)i)) != 0)
        {
            m_controllerData[input_idx].m_npadId = i;
            break;
        }
    }

    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Attach - Idx: %d [NpadId: %d]", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), input_idx, m_controllerData[input_idx].m_npadId);
    // R_TRY(hidInitializeVibrationDevices(&m_controllerData[input_idx].m_vibrationDeviceHandle, 1, m_controllerData[input_idx].m_npadId, HidNpadStyleTag_NpadFullKey));

    R_SUCCEED();
}

ams::Result SwitchHDLHandler::Detach(uint16_t input_idx)
{
    if (!IsVirtualDeviceAttached(input_idx))
        R_SUCCEED();

    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Detaching device for input: %d ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), input_idx);

    hiddbgDetachHdlsVirtualDevice(m_controllerData[input_idx].m_hdlHandle);
    m_controllerData[input_idx].m_hdlHandle.handle = 0;

    R_SUCCEED();
}

ams::Result SwitchHDLHandler::InitHdlState()
{
    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Initializing HDL state ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());

    for (int i = 0; i < m_controller->GetInputCount(); i++)
    {
        m_controllerData[i].reset();

        syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Initializing HDL device idx: %d ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), i);

        // Set the controller type to Pro-Controller, and set the npadInterfaceType.
        m_controllerData[i].m_deviceInfo.deviceType = HidDeviceType_FullKey15;
        m_controllerData[i].m_deviceInfo.npadInterfaceType = HidNpadInterfaceType_USB;

        // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
        m_controllerData[i].m_deviceInfo.singleColorBody = __builtin_bswap32(m_controller->GetConfig().bodyColor.rgbaValue);
        m_controllerData[i].m_deviceInfo.singleColorButtons = __builtin_bswap32(m_controller->GetConfig().buttonsColor.rgbaValue);
        m_controllerData[i].m_deviceInfo.colorLeftGrip = __builtin_bswap32(m_controller->GetConfig().leftGripColor.rgbaValue);
        m_controllerData[i].m_deviceInfo.colorRightGrip = __builtin_bswap32(m_controller->GetConfig().rightGripColor.rgbaValue);

        m_controllerData[i].m_hdlState.battery_level = 4; // Set battery charge to full.
        m_controllerData[i].m_hdlState.analog_stick_l.x = 0x1234;
        m_controllerData[i].m_hdlState.analog_stick_l.y = -0x1234;
        m_controllerData[i].m_hdlState.analog_stick_r.x = 0x5678;
        m_controllerData[i].m_hdlState.analog_stick_r.y = -0x5678;
    }

    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] HDL state successfully initialized !", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());
    R_SUCCEED();
}

ams::Result SwitchHDLHandler::UninitHdlState()
{
    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] UninitHdlState .. !", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());

    for (int i = 0; i < m_controller->GetInputCount(); i++)
        Detach(i);

    R_SUCCEED();
}

bool SwitchHDLHandler::IsVirtualDeviceAttached(uint16_t input_idx)
{
    return m_controllerData[input_idx].m_hdlHandle.handle != 0;
}

// Sets the state of the class's HDL controller to the state stored in class's hdl.state
ams::Result SwitchHDLHandler::UpdateHdlState(const NormalizedButtonData &data, uint16_t input_idx)
{
    HiddbgHdlsState *hdlState = &m_controllerData[input_idx].m_hdlState;

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

    ConvertAxisToSwitchAxis(data.sticks[0].axis_x, data.sticks[0].axis_y, &hdlState->analog_stick_l.x, &hdlState->analog_stick_l.y);
    ConvertAxisToSwitchAxis(data.sticks[1].axis_x, data.sticks[1].axis_y, &hdlState->analog_stick_r.x, &hdlState->analog_stick_r.y);

    if (!m_controllerData[input_idx].m_is_sync)
        m_controllerData[input_idx].m_is_sync = (hdlState->buttons & HidNpadButton_L) && (hdlState->buttons & HidNpadButton_R);

    if (m_controllerData[input_idx].m_is_connected && m_controllerData[input_idx].m_is_sync)
        Attach(input_idx);

    if (IsVirtualDeviceAttached(input_idx))
    {
        syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] UpdateHdlState - Idx: %d [Button: 0x%016X LeftX: %d LeftY: %d RightX: %d RightY: %d]", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), input_idx, hdlState->buttons, hdlState->analog_stick_l.x, hdlState->analog_stick_l.y, hdlState->analog_stick_r.x, hdlState->analog_stick_r.y);
        Result rc = hiddbgSetHdlsState(m_controllerData[input_idx].m_hdlHandle, hdlState);
        if (R_FAILED(rc))
        {
            // syscon::logger::LogError("SwitchHDLHandler UpdateHdlState - Failed to set HDL state for idx: %d (Ret: 0x%X) - Detaching controller ...", input_idx, rc);
            m_controllerData[input_idx].m_is_sync = false;
            Detach(input_idx);

            R_RETURN(rc);
        }
    }

    R_SUCCEED();
}

void SwitchHDLHandler::UpdateInput(s32 timeout_us)
{
    uint16_t input_idx = 0;
    NormalizedButtonData buttonData = {0};

    ams::Result read_rc = m_controller->ReadInput(&buttonData, &input_idx, timeout_us);

    /*
        Note: We must not return here if readInput fail, because it might have change the ControllerConnected state.
        So, we must check if the controller is connected and detach it if it's not.
        This case happen with wireless Xbox 360 controllers
    */

    if (m_controllerData[input_idx].m_is_connected != m_controller->IsControllerConnected(input_idx)) // State changed ?
    {
        m_controllerData[input_idx].m_is_connected = m_controller->IsControllerConnected(input_idx);
        m_controllerData[input_idx].m_is_sync = true; // Force sync to true in order to immediately attach the controller once re-connected

        if (!m_controllerData[input_idx].m_is_connected)
            Detach(input_idx);
    }

    if (R_FAILED(read_rc))
        return;

    // We get the button inputs from the input packet and update the state of our controller
    UpdateHdlState(buttonData, input_idx);
}

void SwitchHDLHandler::UpdateOutput()
{
    // Process rumble values if supported
    /*if (GetController()->Support(SUPPORTS_RUMBLE))
    {
        HidVibrationValue value;

        for (uint16_t input_idx = 0; input_idx < m_controller->GetInputCount(); input_idx++)
        {
            if (!IsVirtualDeviceAttached(input_idx))
                continue;

            if (R_FAILED(hidGetActualVibrationValue(m_controllerData[input_idx].m_vibrationDeviceHandle, &value)))
            {
                syscon::logger::LogError("SwitchHDLHandler[%04x-%04x] UpdateOutput - Failed to get vibration value for idx: %d", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), input_idx);
                continue;
            }

            if (value.amp_high == m_controllerData[input_idx].m_vibrationLastValue.amp_high && value.amp_low == m_controllerData[input_idx].m_vibrationLastValue.amp_low)
                continue; // Do nothing if the values are the same

            syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] UpdateOutput - Idx: %d [AmpHigh: %d%% AmpLow: %d%%]", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), input_idx, (uint8_t)(value.amp_high * 100), (uint8_t)(value.amp_low * 100));
            m_controller->SetRumble(input_idx, value.amp_high, value.amp_low);

            m_controllerData[input_idx].m_vibrationLastValue = value;
        }
    }*/
}

HiddbgHdlsSessionId &SwitchHDLHandler::GetHdlsSessionId()
{
    return g_hdlsSessionId;
}