#include "Controllers/GenericHIDController.h"
#include <cmath>
#include <string.h>

// https://www.usb.org/sites/default/files/documents/hid1_11.pdf  p55

GenericHIDController::GenericHIDController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : IController(std::move(device), config, std::move(logger)),
      m_inputCount(0)
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
        break; // Stop after the first interface
    }

    if (!m_inPipe)
        R_RETURN(CONTROL_ERR_INVALID_ENDPOINT);

    m_features[SUPPORTS_RUMBLE] = (m_outPipe != nullptr); // refresh input count

    // If has reportID
    for (int i = 0; i < CONTROLLER_MAX_INPUTS; i++)
    {
        uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
        size_t size = sizeof(input_bytes);

        R_TRY(m_inPipe->Read(input_bytes, &size));

        uint16_t report_id = input_bytes[0];
        if (report_id > m_inputCount && report_id <= CONTROLLER_MAX_INPUTS)
            m_inputCount = report_id;
    }

    LogPrint(LogLevelInfo, "GenericHIDController USB interfaces opened successfully (%d inputs) !", m_inputCount);

    R_SUCCEED();
}

void GenericHIDController::CloseInterfaces()
{
    m_device->Close();
}

uint16_t GenericHIDController::GetInputCount()
{
    return m_inputCount;
}

ams::Result GenericHIDController::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
{
    uint8_t input_bytes[CONTROLLER_INPUT_BUFFER_SIZE];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe->Read(input_bytes, &size));

    LogPrint(LogLevelDebug, "GenericHIDController ReadInput %d bytes", size);
    LogBuffer(LogLevelDebug, input_bytes, size);

    *input_idx = input_bytes[0] - 1;
    if (*input_idx >= m_inputCount)
        R_RETURN(CONTROL_ERR_UNEXPECTED_DATA);

    GenericHIDButtonData *buttonData = reinterpret_cast<GenericHIDButtonData *>(input_bytes);

    /*
        normalData->triggers[0] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[0], buttonData->trigger_left_pressure);
        normalData->triggers[1] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[1], buttonData->trigger_right_pressure);

        NormalizeAxis(buttonData->stick_left_x, buttonData->stick_left_y, GetConfig().stickDeadzonePercent[0],
                      &normalData->sticks[0].axis_x, &normalData->sticks[0].axis_y);
        NormalizeAxis(buttonData->stick_right_x, buttonData->stick_right_y, GetConfig().stickDeadzonePercent[1],
                      &normalData->sticks[1].axis_x, &normalData->sticks[1].axis_y);
    */
    // Button 1, 2, 3, 4 has been mapped according to remote control: Guilikit, Xbox360
    bool buttons[MAX_CONTROLLER_BUTTONS] = {
        buttonData->button4,                 // X
        buttonData->button2,                 // A
        buttonData->button1,                 // B
        buttonData->button3,                 // Y
        false,                               // buttonData->stick_left_click,
        false,                               // buttonData->stick_right_click,
        buttonData->button5,                 // L
        buttonData->button6,                 // R
        buttonData->button7,                 // ZL
        buttonData->button8,                 // ZR
        buttonData->button9,                 // Minus
        buttonData->button10,                // Plus
        buttonData->dpad_up_down == 0x00,    // UP
        buttonData->dpad_left_right == 0xFF, // RIGHT
        buttonData->dpad_up_down == 0xFF,    // DOWN
        buttonData->dpad_left_right == 0x00, // LEFT
        false,                               // Capture
        false,                               // Home
    };

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
