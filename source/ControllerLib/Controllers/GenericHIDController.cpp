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

    LogPrint(LogLevelTrace, "GenericHIDController Got descriptor for interface %d", m_interface->GetDescriptor()->bInterfaceNumber);
    LogBuffer(LogLevelTrace, buffer, size);

    LogPrint(LogLevelDebug, "GenericHIDController Parsing descriptor ...");
    std::shared_ptr<HIDReportDescriptor> descriptor = std::make_shared<HIDReportDescriptor>(buffer, size);

    LogPrint(LogLevelDebug, "GenericHIDController Looking for joystick/gamepad profile ...");
    m_joystick = std::make_shared<HIDJoystick>(descriptor);
    m_joystick_count = m_joystick->getCount();

    if (m_joystick_count == 0)
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

    LogPrint(LogLevelTrace, "GenericHIDController ReadInput %d bytes", size);
    LogBuffer(LogLevelTrace, input_bytes, size);

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

    normalData->triggers[0] = Normalize(GetConfig().triggerDeadzonePercent[0], joystick_data.Z, -32768, 32767);
    normalData->triggers[1] = Normalize(GetConfig().triggerDeadzonePercent[1], joystick_data.Rz, -32768, 32767);

    normalData->sticks[0].axis_x = Normalize(GetConfig().stickDeadzonePercent[0], joystick_data.X, -32768, 32767);
    normalData->sticks[0].axis_y = Normalize(GetConfig().stickDeadzonePercent[0], joystick_data.Y, -32768, 32767);
    normalData->sticks[1].axis_x = Normalize(GetConfig().stickDeadzonePercent[1], joystick_data.Rx, -32768, 32767);
    normalData->sticks[1].axis_y = Normalize(GetConfig().stickDeadzonePercent[1], joystick_data.Ry, -32768, 32767);

    normalData->buttons[ControllerButton::X] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::X]] ? true : false;
    normalData->buttons[ControllerButton::A] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::A]] ? true : false;
    normalData->buttons[ControllerButton::B] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::B]] ? true : false;
    normalData->buttons[ControllerButton::Y] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::Y]] ? true : false;
    normalData->buttons[ControllerButton::LSTICK_CLICK] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::LSTICK_CLICK]] ? true : false;
    normalData->buttons[ControllerButton::RSTICK_CLICK] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::RSTICK_CLICK]] ? true : false;
    normalData->buttons[ControllerButton::L] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::L]] ? true : false;
    normalData->buttons[ControllerButton::R] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::R]] ? true : false;

    normalData->buttons[ControllerButton::ZL] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::ZL]] ? true : false;
    normalData->buttons[ControllerButton::ZR] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::ZR]] ? true : false;

    if (GetConfig().buttons_pin[ControllerButton::ZL] == 0)
        normalData->buttons[ControllerButton::ZL] = normalData->triggers[0] > 0;

    if (GetConfig().buttons_pin[ControllerButton::ZR] == 0)
        normalData->buttons[ControllerButton::ZR] = normalData->triggers[1] > 0;

    normalData->buttons[ControllerButton::MINUS] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::MINUS]] ? true : false;
    normalData->buttons[ControllerButton::PLUS] = joystick_data.buttons[GetConfig().buttons_pin[ControllerButton::PLUS]] ? true : false;
    normalData->buttons[ControllerButton::DPAD_UP] = joystick_data.hat_switch == HIDJoystickHatSwitch::UP || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_LEFT;
    normalData->buttons[ControllerButton::DPAD_RIGHT] = joystick_data.hat_switch == HIDJoystickHatSwitch::RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_RIGHT;
    normalData->buttons[ControllerButton::DPAD_DOWN] = joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_LEFT;
    normalData->buttons[ControllerButton::DPAD_LEFT] = joystick_data.hat_switch == HIDJoystickHatSwitch::LEFT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_LEFT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_LEFT;

    R_SUCCEED();
}
