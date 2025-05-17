#include "SwitchHDLHandler.h"
#include "SwitchLogger.h"
#include <cmath>

static HiddbgHdlsSessionId g_hdlsSessionId;

SwitchHDLHandler::SwitchHDLHandler(std::unique_ptr<IController> &&controller, int32_t polling_frequency_ms, int8_t thread_priority)
    : SwitchVirtualGamepadHandler(std::move(controller), polling_frequency_ms, thread_priority)
{
    for (int i = 0; i < CONTROLLER_MAX_INPUTS; i++)
        m_controllerData[i].m_is_sync = true;
}

SwitchHDLHandler::~SwitchHDLHandler()
{
    Exit();
}

Result SwitchHDLHandler::Initialize()
{
    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Initializing ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());

    Result rc = SwitchVirtualGamepadHandler::Initialize();
    if (R_FAILED(rc))
        return rc;

    rc = InitHdlState();
    if (R_FAILED(rc))
        return rc;

    rc = InitThread();
    if (R_FAILED(rc))
        return rc;

    syscon::logger::LogInfo("SwitchHDLHandler[%04x-%04x] Initialized !", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());

    return 0;
}

void SwitchHDLHandler::Exit()
{
    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Exiting ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());

    SwitchVirtualGamepadHandler::Exit();

    UninitHdlState();

    syscon::logger::LogInfo("SwitchHDLHandler[%04x-%04x] Uninitialized !", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());
}

Result SwitchHDLHandler::Attach(uint16_t input_idx)
{
    if (IsVirtualDeviceAttached(input_idx))
        return 0;

    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Attaching device for input: %d ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), input_idx);

    Result rc = hiddbgAttachHdlsVirtualDevice(&m_controllerData[input_idx].m_hdlHandle, &m_controllerData[input_idx].m_deviceInfo);
    if (R_FAILED(rc))
    {
        syscon::logger::LogError("SwitchHDLHandler[%04x-%04x] Failed to attach device for input: %d (Error: 0x%08X)!", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), input_idx, rc);
        return rc;
    }

    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Attach - Idx: %d", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), input_idx);

    return 0;
}

Result SwitchHDLHandler::Detach(uint16_t input_idx)
{
    if (!IsVirtualDeviceAttached(input_idx))
        return 0;

    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Detaching device for input: %d ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), input_idx);

    hiddbgDetachHdlsVirtualDevice(m_controllerData[input_idx].m_hdlHandle);
    m_controllerData[input_idx].m_hdlHandle.handle = 0;

    return 0;
}

u8 ControllerTypeToDeviceType(ControllerType type)
{
    if (type == ControllerType_ProWithBattery)
        return HidDeviceType_FullKey3;
    else if (type == ControllerType_Tarragon)
        return HidDeviceType_FullKey6;
    else if (type == ControllerType_Snes)
        return HidDeviceType_Lucia;
    else if (type == ControllerType_PokeballPlus)
        return HidDeviceType_Palma;
    else if (type == ControllerType_Gamecube)
        return HidDeviceType_FullKey13;
    else if (type == ControllerType_Pro)
        return HidDeviceType_FullKey15;
    else if (type == ControllerType_3rdPartyPro)
        return HidDeviceType_System19;
    else if (type == ControllerType_N64)
        return HidDeviceType_Lagon;
    else if (type == ControllerType_Sega)
        return HidDeviceType_Lager;
    else if (type == ControllerType_Nes)
        return HidDeviceType_LarkNesLeft;
    else if (type == ControllerType_Famicom)
        return HidDeviceType_LarkHvcLeft;

    return HidDeviceType_FullKey15;
}

Result SwitchHDLHandler::InitHdlState()
{
    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Initializing HDL state ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());

    for (int i = 0; i < m_controller->GetInputCount(); i++)
    {
        m_controllerData[i].reset();

        syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] Initializing HDL device idx: %d (Controller type: %d) ...", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct(), i, m_controller->GetConfig().controllerType);

        // Set the controller type to Pro-Controller, and set the npadInterfaceType.
        m_controllerData[i].m_deviceInfo.deviceType = ControllerTypeToDeviceType(m_controller->GetConfig().controllerType);
        m_controllerData[i].m_deviceInfo.npadInterfaceType = HidNpadInterfaceType_USB;

        // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
        m_controllerData[i].m_deviceInfo.singleColorBody = __builtin_bswap32(m_controller->GetConfig().bodyColor.rgbaValue);
        m_controllerData[i].m_deviceInfo.singleColorButtons = __builtin_bswap32(m_controller->GetConfig().buttonsColor.rgbaValue);
        m_controllerData[i].m_deviceInfo.colorLeftGrip = __builtin_bswap32(m_controller->GetConfig().leftGripColor.rgbaValue);
        m_controllerData[i].m_deviceInfo.colorRightGrip = __builtin_bswap32(m_controller->GetConfig().rightGripColor.rgbaValue);

        m_controllerData[i].m_hdlState.battery_level = 4; // Set battery charge to full.
        m_controllerData[i].m_hdlState.analog_stick_l.x = 0;
        m_controllerData[i].m_hdlState.analog_stick_l.y = 0;
        m_controllerData[i].m_hdlState.analog_stick_r.x = 0;
        m_controllerData[i].m_hdlState.analog_stick_r.y = 0;
    }

    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] HDL state successfully initialized !", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());
    return 0;
}

Result SwitchHDLHandler::UninitHdlState()
{
    syscon::logger::LogDebug("SwitchHDLHandler[%04x-%04x] UninitHdlState .. !", m_controller->GetDevice()->GetVendor(), m_controller->GetDevice()->GetProduct());

    for (int i = 0; i < m_controller->GetInputCount(); i++)
        Detach(i);

    return 0;
}

bool SwitchHDLHandler::IsVirtualDeviceAttached(uint16_t input_idx)
{
    return m_controllerData[input_idx].m_hdlHandle.handle != 0;
}

// Sets the state of the class's HDL controller to the state stored in class's hdl.state
Result SwitchHDLHandler::UpdateHdlState(const NormalizedButtonData &data, uint16_t input_idx)
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
            /*
                The switch will automatically detach the controller if user goes to SYNC menu or if user enter in a game with only 1 controller (i.e: tftpd)
                So, we must detach the controller if we failed to set the HDL state (In order to avoid a desync between the controller and the switch)
                This case might also happen if user plugged a controller and we were not able to attach it (i.e: if the game do no accept more controller)
            */

            // syscon::logger::LogError("SwitchHDLHandler UpdateHdlState - Failed to set HDL state for idx: %d (Ret: 0x%X) - Detaching controller ...", input_idx, rc);
            m_controllerData[input_idx].m_is_sync = false;
            Detach(input_idx);

            return rc;
        }
    }

    return 0;
}

void SwitchHDLHandler::UpdateInput(uint32_t timeout_us)
{
    uint16_t input_idx = 0;
    NormalizedButtonData buttonData = {0};

    Result read_rc = m_controller->ReadInput(&buttonData, &input_idx, timeout_us);

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
    // Vibrations are not supported with HDL
}

HiddbgHdlsSessionId &SwitchHDLHandler::GetHdlsSessionId()
{
    return g_hdlsSessionId;
}