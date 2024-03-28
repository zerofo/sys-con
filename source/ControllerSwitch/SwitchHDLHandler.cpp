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

    if (GetController()->Support(SUPPORTS_PAIRING))
    {
        R_TRY(InitOutputThread());
    }

    R_TRY(InitInputThread());

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

    ExitInputThread();
    ExitOutputThread();
    m_controller->Exit();
    ExitHdlState();
}

Result SwitchHDLHandler::InitHdlState()
{
    syscon::logger::LogDebug("Initializing HDL state ...");

    m_hdlHandle = {0};
    m_deviceInfo = {0};
    m_hdlState = {0};

    // Set the controller type to Pro-Controller, and set the npadInterfaceType.
    m_deviceInfo.deviceType = HidDeviceType_FullKey15;
    m_deviceInfo.npadInterfaceType = HidNpadInterfaceType_USB;
    // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
    ControllerConfig *config = m_controller->GetConfig();
    m_deviceInfo.singleColorBody = config->bodyColor.rgbaValue;
    m_deviceInfo.singleColorButtons = config->buttonsColor.rgbaValue;
    m_deviceInfo.colorLeftGrip = config->leftGripColor.rgbaValue;
    m_deviceInfo.colorRightGrip = config->rightGripColor.rgbaValue;

    m_hdlState.battery_level = 4; // Set battery charge to full.
    m_hdlState.analog_stick_l.x = 0x1234;
    m_hdlState.analog_stick_l.y = -0x1234;
    m_hdlState.analog_stick_r.x = 0x5678;
    m_hdlState.analog_stick_r.y = -0x5678;
    /*
    Since 12.0
    m_hdlState.six_axis_sensor_acceleration.x = 0x1234;
    m_hdlState.six_axis_sensor_acceleration.y = 0x5678;
    m_hdlState.six_axis_sensor_acceleration.z = 0x9abc;
    m_hdlState.six_axis_sensor_angle.x = 0xdef0;
    m_hdlState.six_axis_sensor_angle.y = 0x1234;
    m_hdlState.six_axis_sensor_angle.z = 0x5678;
    m_hdlState.attribute = 0x0;
    m_hdlState.indicator = 0x0;
    m_hdlState.padding[0] = 0x0;
    m_hdlState.padding[1] = 0x0;
    m_hdlState.padding[2] = 0x0;
    */

    if (m_controller->IsControllerActive())
    {
        syscon::logger::LogInfo("SwitchHDLHandler hiddbgAttachHdlsVirtualDevice ...");
        return hiddbgAttachHdlsVirtualDevice(&m_hdlHandle, &m_deviceInfo);
    }

    return 0;
}
Result SwitchHDLHandler::ExitHdlState()
{
    return hiddbgDetachHdlsVirtualDevice(m_hdlHandle);
}

// Sets the state of the class's HDL controller to the state stored in class's hdl.state
Result SwitchHDLHandler::UpdateHdlState(const NormalizedButtonData &data, uint16_t input_idx)
{
    (void)input_idx;
    // we convert the input packet into switch-specific button states
    m_hdlState.buttons = 0;

    if (data.buttons[0])
        m_hdlState.buttons |= HidNpadButton_X;
    if (data.buttons[1])
        m_hdlState.buttons |= HidNpadButton_A;
    if (data.buttons[2])
        m_hdlState.buttons |= HidNpadButton_B;
    if (data.buttons[3])
        m_hdlState.buttons |= HidNpadButton_Y;
    if (data.buttons[4])
        m_hdlState.buttons |= HidNpadButton_StickL;
    if (data.buttons[5])
        m_hdlState.buttons |= HidNpadButton_StickR;
    if (data.buttons[6])
        m_hdlState.buttons |= HidNpadButton_L;
    if (data.buttons[7])
        m_hdlState.buttons |= HidNpadButton_R;
    if (data.buttons[8])
        m_hdlState.buttons |= HidNpadButton_ZL;
    if (data.buttons[9])
        m_hdlState.buttons |= HidNpadButton_ZR;
    if (data.buttons[10])
        m_hdlState.buttons |= HidNpadButton_Minus;
    if (data.buttons[11])
        m_hdlState.buttons |= HidNpadButton_Plus;

    ControllerConfig *config = m_controller->GetConfig();

    if (config && config->swapDPADandLSTICK)
    {
        if (data.sticks[0].axis_y > 0.5f)
            m_hdlState.buttons |= HidNpadButton_Up;
        if (data.sticks[0].axis_x > 0.5f)
            m_hdlState.buttons |= HidNpadButton_Right;
        if (data.sticks[0].axis_y < -0.5f)
            m_hdlState.buttons |= HidNpadButton_Down;
        if (data.sticks[0].axis_x < -0.5f)
            m_hdlState.buttons |= HidNpadButton_Left;

        float daxis_x{}, daxis_y{};

        daxis_y += data.buttons[12] ? 1.0f : 0.0f;  // DUP
        daxis_x += data.buttons[13] ? 1.0f : 0.0f;  // DRIGHT
        daxis_y += data.buttons[14] ? -1.0f : 0.0f; // DDOWN
        daxis_x += data.buttons[15] ? -1.0f : 0.0f; // DLEFT

        // clamp lefstick values to their acceptable range of values
        float real_magnitude = std::sqrt(daxis_x * daxis_x + daxis_y * daxis_y);
        float clipped_magnitude = std::min(1.0f, real_magnitude);
        float ratio = clipped_magnitude / real_magnitude;

        daxis_x *= ratio;
        daxis_y *= ratio;

        ConvertAxisToSwitchAxis(daxis_x, daxis_y, 0, &m_hdlState.analog_stick_l.x, &m_hdlState.analog_stick_l.y);
    }
    else
    {
        if (data.buttons[12])
            m_hdlState.buttons |= HidNpadButton_Up;
        if (data.buttons[13])
            m_hdlState.buttons |= HidNpadButton_Right;
        if (data.buttons[14])
            m_hdlState.buttons |= HidNpadButton_Down;
        if (data.buttons[15])
            m_hdlState.buttons |= HidNpadButton_Left;

        ConvertAxisToSwitchAxis(data.sticks[0].axis_x, data.sticks[0].axis_y, 0, &m_hdlState.analog_stick_l.x, &m_hdlState.analog_stick_l.y);
    }

    ConvertAxisToSwitchAxis(data.sticks[1].axis_x, data.sticks[1].axis_y, 0, &m_hdlState.analog_stick_r.x, &m_hdlState.analog_stick_r.y);

    m_hdlState.buttons |= (data.buttons[16] ? HiddbgNpadButton_Capture : 0);
    m_hdlState.buttons |= (data.buttons[17] ? HiddbgNpadButton_Home : 0);

    // Checks if the virtual device was erased, in which case re-attach the device
    /*bool isAttached;

    syscon::logger::LogDebug("SwitchHDLHandler UpdateHdlState - hiddbgIsHdlsVirtualDeviceAttached ...");

    if (R_SUCCEEDED(hiddbgIsHdlsVirtualDeviceAttached(GetHdlsSessionId(), m_hdlHandle, &isAttached)))
    {
        if (!isAttached)
        {
            syscon::logger::LogDebug("SwitchHDLHandler hiddbgAttachHdlsVirtualDevice ...");
            hiddbgAttachHdlsVirtualDevice(&m_hdlHandle, &m_deviceInfo);
        }
    }
    */

    syscon::logger::LogDebug("SwitchHDLHandler UpdateHdlState - Button: 0x%016X - Idx: %d ...");
    return hiddbgSetHdlsState(m_hdlHandle, &m_hdlState);
}

void SwitchHDLHandler::UpdateInput()
{
    NormalizedButtonData data = {0};
    uint16_t input_idx;

    // We process any input packets here. If it fails, return and try again
    Result rc = m_controller->ReadInput(&data, &input_idx);
    if (R_FAILED(rc))
        return;

    // This is a check for controllers that can prompt themselves to go inactive - e.g. wireless Xbox 360 controllers
    if (!m_controller->IsControllerActive())
    {
        hiddbgDetachHdlsVirtualDevice(m_hdlHandle);
    }
    else
    {
        // We get the button inputs from the input packet and update the state of our controller
        rc = UpdateHdlState(data, input_idx);
        if (R_FAILED(rc))
            return;
    }
}

void SwitchHDLHandler::UpdateOutput()
{
    // Process a single output packet from a buffer
    if (R_SUCCEEDED(m_controller->OutputBuffer()))
        return;

    // Process rumble values if supported
    if (GetController()->Support(SUPPORTS_RUMBLE))
    {
        Result rc;
        HidVibrationValue value;
        rc = hidGetActualVibrationValue(m_vibrationDeviceHandle, &value);
        if (R_SUCCEEDED(rc))
            m_controller->SetRumble(static_cast<uint8_t>(value.amp_high * 255.0f), static_cast<uint8_t>(value.amp_low * 255.0f));
    }
}

HiddbgHdlsSessionId &SwitchHDLHandler::GetHdlsSessionId()
{
    return g_hdlsSessionId;
}