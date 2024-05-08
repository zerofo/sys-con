#include "Controllers/GenericHIDController.h"
#include "HIDReportDescriptor.h"
#include "HIDJoystick.h"
#include <string.h>

// https://www.usb.org/sites/default/files/documents/hid1_11.pdf  p55

GenericHIDController::GenericHIDController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger))
{
    LogPrint(LogLevelInfo, "GenericHIDController Created for %04x-%04x", m_device->GetVendor(), m_device->GetProduct());
}

GenericHIDController::~GenericHIDController()
{
}

ams::Result GenericHIDController::Initialize()
{
    LogPrint(LogLevelDebug, "GenericHIDController Initializing ...");

    R_TRY(BaseController::Initialize());

    uint8_t buffer[CONTROLLER_HID_REPORT_BUFFER_SIZE];
    uint16_t size = sizeof(buffer);

    // Get the HID report descriptor
    R_TRY(m_interface->ControlTransferInput((uint8_t)USB_ENDPOINT_IN | (uint8_t)USB_RECIPIENT_INTERFACE, USB_REQUEST_GET_DESCRIPTOR, (USB_DT_REPORT << 8) | m_interface->GetDescriptor()->bInterfaceNumber, 0, buffer, &size));

    LogPrint(LogLevelDebug, "GenericHIDController Got descriptor for interface %d", m_interface->GetDescriptor()->bInterfaceNumber);
    LogBuffer(LogLevelDebug, buffer, size);

    m_joystick = std::make_shared<HIDJoystick>(HIDReportDescriptor(buffer, size));

    m_joystick_count = m_joystick->getCount();

    if (!m_joystick_count == 0)
    {
        LogPrint(LogLevelError, "GenericHIDController HID report descriptor don't contain joystick");
        R_RETURN(CONTROL_ERR_HID_IS_NOT_JOYSTICK);
    }

    LogPrint(LogLevelInfo, "GenericHIDController USB joystick successfully opened (%d inputs detected) !", GetInputCount());

    R_SUCCEED();
}

uint16_t GenericHIDController::GetInputCount()
{
    return m_joystick_count;
}

ams::Result GenericHIDController::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
{
    HIDJoystickData joystick_data;
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe->Read(input_bytes, &size));

    LogPrint(LogLevelDebug, "GenericHIDController ReadInput %d bytes", size);
    LogBuffer(LogLevelDebug, input_bytes, size);

    if (!m_joystick->parseData(input_bytes, size, &joystick_data))
    {
        LogPrint(LogLevelError, "GenericHIDController Failed to parse input data");
        R_RETURN(CONTROL_ERR_UNEXPECTED_DATA);
    }

    LogPrint(LogLevelDebug, "GenericHIDController DATA: X=%d, Y=%d, Z=%d, Rz=%d, HatSwitch=%d, B1=%d, B2=%d, B3=%d, B4=%d, B5=%d, B6=%d, B7=%d, B8=%d, B9=%d, B10=%d",
             joystick_data.X, joystick_data.Y, joystick_data.Z, joystick_data.Rz, joystick_data.hat_switch,
             joystick_data.buttons[1],
             joystick_data.buttons[2] ? 1 : 0,
             joystick_data.buttons[3] ? 1 : 0,
             joystick_data.buttons[4] ? 1 : 0,
             joystick_data.buttons[5] ? 1 : 0,
             joystick_data.buttons[6] ? 1 : 0,
             joystick_data.buttons[7] ? 1 : 0,
             joystick_data.buttons[8] ? 1 : 0,
             joystick_data.buttons[9] ? 1 : 0,
             joystick_data.buttons[10] ? 1 : 0);

    *input_idx = joystick_data.index;

    bool hatswitch_up = joystick_data.hat_switch == HIDJoystickHatSwitch::UP || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_LEFT;
    bool hatswitch_right = joystick_data.hat_switch == HIDJoystickHatSwitch::RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_RIGHT;
    bool hatswitch_down = joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_LEFT;
    bool hatswitch_left = joystick_data.hat_switch == HIDJoystickHatSwitch::LEFT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_LEFT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_LEFT;

    // Button 1, 2, 3, 4 has been mapped according to remote control: Guilikit, Xbox360
    bool buttons[MAX_CONTROLLER_BUTTONS] = {
        joystick_data.buttons[4] ? true : false,  // X
        joystick_data.buttons[2] ? true : false,  // A
        joystick_data.buttons[1] ? true : false,  // B
        joystick_data.buttons[3] ? true : false,  // Y
        joystick_data.buttons[11] ? true : false, // stick_left_click,
        joystick_data.buttons[12] ? true : false, // stick_right_click,
        joystick_data.buttons[5] ? true : false,  // L
        joystick_data.buttons[6] ? true : false,  // R
        joystick_data.buttons[7] ? true : false,  // ZL
        joystick_data.buttons[8] ? true : false,  // ZR
        joystick_data.buttons[9] ? true : false,  // Minus
        joystick_data.buttons[10] ? true : false, // Plus
        hatswitch_up,                             // UP
        hatswitch_right,                          // RIGHT
        hatswitch_down,                           // DOWN
        hatswitch_left,                           // LEFT
        false,                                    // Capture
        false,                                    // Home
    };

    normalData->triggers[0] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[0], joystick_data.Z);
    normalData->triggers[1] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[1], joystick_data.Rz);

    NormalizeAxis(joystick_data.X, joystick_data.Y, GetConfig().stickDeadzonePercent[0], &normalData->sticks[0].axis_x, &normalData->sticks[0].axis_y, 0, 255);
    NormalizeAxis(joystick_data.Rx, joystick_data.Ry, GetConfig().stickDeadzonePercent[1], &normalData->sticks[1].axis_x, &normalData->sticks[1].axis_y, 0, 255);

    for (int i = 0; i < MAX_CONTROLLER_BUTTONS; i++)
    {
        ControllerButton button = GetConfig().buttons[i];
        if (button == NONE)
            continue;

        normalData->buttons[(button != DEFAULT ? button - 2 : i)] += buttons[i];
    }

    R_SUCCEED();
}
