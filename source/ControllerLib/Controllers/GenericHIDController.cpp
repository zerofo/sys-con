#include "Controllers/GenericHIDController.h"
#include "HIDReportDescriptor.h"
#include "HIDJoystick.h"
#include <cmath>
#include <string.h>

// https://www.usb.org/sites/default/files/documents/hid1_11.pdf  p55

GenericHIDController::GenericHIDController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : IController(std::move(device), config, std::move(logger))
{
    LogPrint(LogLevelInfo, "GenericHIDController Created for %04x-%04x", m_device->GetVendor(), m_device->GetProduct());
}

GenericHIDController::~GenericHIDController()
{
}

ams::Result GenericHIDController::Initialize()
{
    LogPrint(LogLevelDebug, "GenericHIDController Initializing ...");

    R_TRY(OpenInterfaces());

    R_SUCCEED();
}
void GenericHIDController::Exit()
{
    CloseInterfaces();
}

ams::Result GenericHIDController::OpenInterfaces()
{
    LogPrint(LogLevelDebug, "GenericHIDController Opening interfaces ...");

    R_TRY(m_device->Open());

    // Open each interface, send it a setup packet and get the endpoints if it succeeds
    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        R_TRY(interface->Open());

        if (!m_inPipe)
        {
            for (int i = 0; i < 15; i++)
            {
                IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, i);
                if (inEndpoint)
                {
                    R_TRY(inEndpoint->Open());

                    m_inPipe = inEndpoint;
                    break;
                }
            }
        }

        if (!m_outPipe)
        {
            for (int i = 0; i < 15; i++)
            {
                IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, i);
                if (outEndpoint)
                {
                    R_TRY(outEndpoint->Open());

                    m_outPipe = outEndpoint;
                    break;
                }
            }
        }

        uint8_t buffer[CONTROLLER_HID_REPORT_BUFFER_SIZE];
        uint16_t size = sizeof(buffer);

        R_TRY(interface->ControlTransferInput((uint8_t)USB_ENDPOINT_IN | (uint8_t)USB_RECIPIENT_INTERFACE, USB_REQUEST_GET_DESCRIPTOR, (USB_DT_REPORT << 8) | interface->GetDescriptor()->bInterfaceNumber, 0, buffer, &size));

        LogPrint(LogLevelDebug, "GenericHIDController Got descriptor for interface %d", interface->GetDescriptor()->bInterfaceNumber);
        LogBuffer(LogLevelDebug, buffer, size);

        m_joystick = std::make_shared<HIDJoystick>(HIDReportDescriptor(buffer, size));

        break; // Stop after the first interface
    }

    if (!m_inPipe)
        R_RETURN(CONTROL_ERR_INVALID_ENDPOINT);

    if (!m_joystick)
        R_RETURN(CONTROL_ERR_INVALID_REPORT_DESCRIPTOR);

    if (!m_joystick->isValid())
    {
        LogPrint(LogLevelError, "GenericHIDController HID report descriptor don't contains HID report for joystick");
        R_RETURN(CONTROL_ERR_HID_IS_NOT_JOYSTICK);
    }

    m_features[SUPPORTS_RUMBLE] = (m_outPipe != nullptr); // refresh input count

    LogPrint(LogLevelInfo, "GenericHIDController USB joystick successfully opened (%d inputs detected) !", GetInputCount());

    R_SUCCEED();
}

void GenericHIDController::CloseInterfaces()
{
    m_device->Close();
}

uint16_t GenericHIDController::GetInputCount()
{
    return m_joystick->getCount();
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

    NormalizeAxis(joystick_data.X, joystick_data.Y, GetConfig().stickDeadzonePercent[0], &normalData->sticks[0].axis_x, &normalData->sticks[0].axis_y);
    NormalizeAxis(joystick_data.Rx, joystick_data.Ry, GetConfig().stickDeadzonePercent[1], &normalData->sticks[1].axis_x, &normalData->sticks[1].axis_y);

    for (int i = 0; i < MAX_CONTROLLER_BUTTONS; i++)
    {
        ControllerButton button = GetConfig().buttons[i];
        if (button == NONE)
            continue;

        normalData->buttons[(button != DEFAULT ? button - 2 : i)] += buttons[i];
    }

    R_SUCCEED();
}

bool GenericHIDController::Support(ControllerFeature feature)
{
    switch (feature)
    {
        default:
            return false;
    }
}

float GenericHIDController::NormalizeTrigger(uint8_t deadzonePercent, uint8_t value)
{
    uint8_t deadzone = (UINT8_MAX * deadzonePercent) / 100;
    // If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < deadzone
               ? 0
               : static_cast<float>(value - deadzone) / (UINT8_MAX - deadzone);
}

void GenericHIDController::NormalizeAxis(uint8_t x,
                                         uint8_t y,
                                         uint8_t deadzonePercent,
                                         float *x_out,
                                         float *y_out)
{
    float x_val = x - 127.0f;
    float y_val = 127.0f - y;
    // Determine how far the stick is pushed.
    // This will never exceed 32767 because if the stick is
    // horizontally maxed in one direction, vertically it must be neutral(0) and vice versa
    float real_magnitude = std::sqrt(x_val * x_val + y_val * y_val);
    float real_deadzone = (127 * deadzonePercent) / 100;
    // Check if the controller is outside a circular dead zone.
    if (real_magnitude > real_deadzone)
    {
        // Clip the magnitude at its expected maximum value.
        float magnitude = std::min(127.0f, real_magnitude);
        // Adjust magnitude relative to the end of the dead zone.
        magnitude -= real_deadzone;
        // Normalize the magnitude with respect to its expected range giving a
        // magnitude value of 0.0 to 1.0
        // ratio = (currentValue / maxValue) / realValue
        float ratio = (magnitude / (127 - real_deadzone)) / real_magnitude;
        *x_out = x_val * ratio;
        *y_out = y_val * ratio;
    }
    else
    {
        // If the controller is in the deadzone zero out the magnitude.
        *x_out = *y_out = 0.0f;
    }
}

ams::Result GenericHIDController::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    (void)strong_magnitude;
    (void)weak_magnitude;
    // Not implemented yet
    return 9;
}
