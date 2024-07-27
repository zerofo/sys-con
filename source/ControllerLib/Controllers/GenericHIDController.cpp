#include "Controllers/GenericHIDController.h"
#include "HIDReportDescriptor.h"
#include "HIDJoystick.h"
#include <string.h>

// https://www.usb.org/sites/default/files/documents/hid1_11.pdf  p55

GenericHIDController::GenericHIDController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), config, std::move(logger)),
      m_joystick_count(0)
{
    Log(LogLevelDebug, "GenericHIDController[%04x-%04x] Created !", m_device->GetVendor(), m_device->GetProduct());
}

GenericHIDController::~GenericHIDController()
{
}

ControllerResult GenericHIDController::Initialize()
{
    ControllerResult result = BaseController::Initialize();
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    uint8_t buffer[CONTROLLER_HID_REPORT_BUFFER_SIZE];
    uint16_t size = sizeof(buffer);
    // https://www.usb.org/sites/default/files/hid1_11.pdf

    Log(LogLevelDebug, "GenericHIDController[%04x-%04x] Reading report descriptor ...", m_device->GetVendor(), m_device->GetProduct());
    // Get HID report descriptor
    result = m_interfaces[0]->ControlTransferInput((uint8_t)IUSBEndpoint::USB_ENDPOINT_IN | (uint8_t)USB_RECIPIENT_INTERFACE, USB_REQUEST_GET_DESCRIPTOR, (USB_DT_REPORT << 8) | m_interfaces[0]->GetDescriptor()->bInterfaceNumber, 0, buffer, &size);
    if (result != CONTROLLER_STATUS_SUCCESS)
    {
        Log(LogLevelError, "GenericHIDController[%04x-%04x] Failed to get HID report descriptor", m_device->GetVendor(), m_device->GetProduct());
        return result;
    }

    Log(LogLevelTrace, "GenericHIDController[%04x-%04x] Got descriptor for interface %d", m_device->GetVendor(), m_device->GetProduct(), m_interfaces[0]->GetDescriptor()->bInterfaceNumber);
    LogBuffer(LogLevelTrace, buffer, size);

    Log(LogLevelDebug, "GenericHIDController[%04x-%04x] Parsing descriptor ...", m_device->GetVendor(), m_device->GetProduct());
    std::shared_ptr<HIDReportDescriptor> descriptor = std::make_shared<HIDReportDescriptor>(buffer, size);

    Log(LogLevelDebug, "GenericHIDController[%04x-%04x] Looking for joystick/gamepad profile ...", m_device->GetVendor(), m_device->GetProduct());
    m_joystick = std::make_shared<HIDJoystick>(descriptor);
    m_joystick_count = m_joystick->getCount();

    if (m_joystick_count == 0)
    {
        Log(LogLevelError, "GenericHIDController[%04x-%04x] HID report descriptor don't contains joystick/gamepad", m_device->GetVendor(), m_device->GetProduct());
        return CONTROLLER_STATUS_HID_IS_NOT_JOYSTICK;
    }

    Log(LogLevelInfo, "GenericHIDController[%04x-%04x] USB joystick successfully opened (%d inputs detected) !", m_device->GetVendor(), m_device->GetProduct(), GetInputCount());

    return CONTROLLER_STATUS_SUCCESS;
}

uint16_t GenericHIDController::GetInputCount()
{
    return std::min((int)m_joystick_count, CONTROLLER_MAX_INPUTS);
}

ControllerResult GenericHIDController::ReadRawInput(RawInputData *rawData, uint16_t *input_idx, uint32_t timeout_us)
{
    HIDJoystickData joystick_data;
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = std::min((size_t)m_inPipe[0]->GetDescriptor()->wMaxPacketSize, sizeof(input_bytes));

    ControllerResult result = m_inPipe[0]->Read(input_bytes, &size, timeout_us);
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    if (size == 0)
        return CONTROLLER_STATUS_NOTHING_TODO;

    if (!m_joystick->parseData(input_bytes, (uint16_t)size, &joystick_data))
    {
        Log(LogLevelError, "GenericHIDController[%04x-%04x] Failed to parse input data (size=%d)", m_device->GetVendor(), m_device->GetProduct(), size);
        return CONTROLLER_STATUS_UNEXPECTED_DATA;
    }

    if (joystick_data.index >= GetInputCount())
        return CONTROLLER_STATUS_UNEXPECTED_DATA;

    *input_idx = joystick_data.index;

    for (int i = 0; i < MAX_CONTROLLER_BUTTONS; i++)
        rawData->buttons[i] = joystick_data.buttons[i];

    rawData->Rx = BaseController::Normalize(joystick_data.Rx, -32768, 32767);
    rawData->Ry = BaseController::Normalize(joystick_data.Ry, -32768, 32767);

    rawData->X = BaseController::Normalize(joystick_data.X, -32768, 32767);
    rawData->Y = BaseController::Normalize(joystick_data.Y, -32768, 32767);
    rawData->Z = BaseController::Normalize(joystick_data.Z, -32768, 32767);
    rawData->Rz = BaseController::Normalize(joystick_data.Rz, -32768, 32767);

    rawData->dpad_up = joystick_data.hat_switch == HIDJoystickHatSwitch::UP || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_LEFT;
    rawData->dpad_right = joystick_data.hat_switch == HIDJoystickHatSwitch::RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_RIGHT;
    rawData->dpad_down = joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_RIGHT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_LEFT;
    rawData->dpad_left = joystick_data.hat_switch == HIDJoystickHatSwitch::LEFT || joystick_data.hat_switch == HIDJoystickHatSwitch::UP_LEFT || joystick_data.hat_switch == HIDJoystickHatSwitch::DOWN_LEFT;

    return CONTROLLER_STATUS_SUCCESS;
}
