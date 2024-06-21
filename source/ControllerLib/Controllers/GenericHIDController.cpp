#include "Controllers/GenericHIDController.h"
#include "HIDReportDescriptor.h"
#include "HIDJoystick.h"
#include <string.h>

// https://www.usb.org/sites/default/files/documents/hid1_11.pdf  p55

GenericHIDController::GenericHIDController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger)),
      m_joystick_count(0)
{
    LogPrint(LogLevelDebug, "GenericHIDController[%04x-%04x] Created !", m_device->GetVendor(), m_device->GetProduct());
}

GenericHIDController::~GenericHIDController()
{
}

ams::Result GenericHIDController::Initialize()
{
    R_TRY(BaseController::Initialize());

    uint8_t buffer[CONTROLLER_HID_REPORT_BUFFER_SIZE];
    uint16_t size = sizeof(buffer);
    // https://www.usb.org/sites/default/files/hid1_11.pdf

    LogPrint(LogLevelDebug, "GenericHIDController[%04x-%04x] Reading report descriptor ...", m_device->GetVendor(), m_device->GetProduct());
    // Get HID report descriptor
    R_TRY(m_interfaces[0]->ControlTransferInput((uint8_t)USB_ENDPOINT_IN | (uint8_t)USB_RECIPIENT_INTERFACE, USB_REQUEST_GET_DESCRIPTOR, (USB_DT_REPORT << 8) | m_interfaces[0]->GetDescriptor()->bInterfaceNumber, 0, buffer, &size));

    LogPrint(LogLevelTrace, "GenericHIDController[%04x-%04x] Got descriptor for interface %d", m_device->GetVendor(), m_device->GetProduct(), m_interfaces[0]->GetDescriptor()->bInterfaceNumber);
    LogBuffer(LogLevelTrace, buffer, size);

    LogPrint(LogLevelDebug, "GenericHIDController[%04x-%04x] Parsing descriptor ...", m_device->GetVendor(), m_device->GetProduct());
    std::shared_ptr<HIDReportDescriptor> descriptor = std::make_shared<HIDReportDescriptor>(buffer, size);

    LogPrint(LogLevelDebug, "GenericHIDController[%04x-%04x] Looking for joystick/gamepad profile ...", m_device->GetVendor(), m_device->GetProduct());
    m_joystick = std::make_shared<HIDJoystick>(descriptor);
    m_joystick_count = m_joystick->getCount();

    if (m_joystick_count == 0)
    {
        LogPrint(LogLevelError, "GenericHIDController[%04x-%04x] HID report descriptor don't contains joystick/gamepad", m_device->GetVendor(), m_device->GetProduct());
        R_RETURN(CONTROL_ERR_HID_IS_NOT_JOYSTICK);
    }

    LogPrint(LogLevelInfo, "GenericHIDController[%04x-%04x] USB joystick successfully opened (%d inputs detected) !", m_device->GetVendor(), m_device->GetProduct(), GetInputCount());

    R_SUCCEED();
}

uint16_t GenericHIDController::GetInputCount()
{
    return std::min((int)m_joystick_count, CONTROLLER_MAX_INPUTS);
}

ams::Result GenericHIDController::ReadInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us)
{
    HIDJoystickData joystick_data;
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = std::min(m_inPipe[0]->GetDescriptor()->wMaxPacketSize, (uint16_t)sizeof(input_bytes));

    R_TRY(m_inPipe[0]->Read(input_bytes, &size, timeout_us));
    if (size == 0)
        R_RETURN(CONTROL_ERR_NOTHING_TODO);

    if (!m_joystick->parseData(input_bytes, size, &joystick_data))
    {
        LogPrint(LogLevelError, "GenericHIDController[%04x-%04x] Failed to parse input data (size=%d)", m_device->GetVendor(), m_device->GetProduct(), size);
        R_RETURN(CONTROL_ERR_UNEXPECTED_DATA);
    }

    if (joystick_data.index >= GetInputCount())
        R_RETURN(CONTROL_ERR_UNEXPECTED_DATA);

    *input_idx = joystick_data.index;

    for (int i = 0; i < MAX_CONTROLLER_BUTTONS; i++)
        rawData->buttons[i] = joystick_data.buttons[i];

    rawData->Rx = Normalize(joystick_data.Rx, -32768, 32767);
    rawData->Ry = Normalize(joystick_data.Ry, -32768, 32767);

    rawData->X = Normalize(joystick_data.X, -32768, 32767);
    rawData->Y = Normalize(joystick_data.Y, -32768, 32767);
    rawData->Z = Normalize(joystick_data.Z, -32768, 32767);
    rawData->Rz = Normalize(joystick_data.Rz, -32768, 32767);

    rawData->dpad_up = joystick_data.hat_switch == HIDJoystickHatSwitch::UP || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_LEFT;
    rawData->dpad_right = joystick_data.hat_switch == HIDJoystickHatSwitch::RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_RIGHT;
    rawData->dpad_down = joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_LEFT;
    rawData->dpad_left = joystick_data.hat_switch == HIDJoystickHatSwitch::LEFT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_LEFT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_LEFT;

    R_SUCCEED();
}
