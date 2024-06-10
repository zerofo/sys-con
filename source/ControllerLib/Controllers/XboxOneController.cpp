#include "Controllers/XboxOneController.h"

#define TRIGGER_MAXVALUE 1023

// Following input packets were referenced from https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c
//  and https://github.com/360Controller/360Controller/blob/master/360Controller/_60Controller.cpp

// Enables LED on the PowerA controller but disables input?
static constexpr uint8_t xboxone_powerA_ledOn[] = {
    0x04, 0x20, 0x01, 0x00};

// does something maybe
static constexpr uint8_t xboxone_test_init1[] = {
    0x01, 0x20, 0x01, 0x09, 0x00, 0x04, 0x20, 0x3a,
    0x00, 0x00, 0x00, 0x98, 0x00};

// required for all xbox one controllers
static constexpr uint8_t xboxone_fw2015_init[] = {
    0x05, 0x20, 0x00, 0x01, 0x00};

static constexpr uint8_t xboxone_hori_init[] = {
    0x01, 0x20, 0x00, 0x09, 0x00, 0x04, 0x20, 0x3a,
    0x00, 0x00, 0x00, 0x80, 0x00};

static constexpr uint8_t xboxone_pdp_init1[] = {
    0x0a, 0x20, 0x00, 0x03, 0x00, 0x01, 0x14};

static constexpr uint8_t xboxone_pdp_init2[] = {
    0x06, 0x20, 0x00, 0x02, 0x01, 0x00};

static constexpr uint8_t xboxone_rumblebegin_init[] = {
    0x09, 0x00, 0x00, 0x09, 0x00, 0x0F, 0x00, 0x00,
    0x1D, 0x1D, 0xFF, 0x00, 0x00};

static constexpr uint8_t xboxone_rumbleend_init[] = {
    0x09, 0x00, 0x00, 0x09, 0x00, 0x0F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00};

struct VendorProductPacket
{
    uint16_t VendorID;
    uint16_t ProductID;
    const uint8_t *Packet;
    uint8_t Length;
};

static constexpr VendorProductPacket init_packets[]{
    {0x0e6f, 0x0165, xboxone_hori_init, sizeof(xboxone_hori_init)},
    {0x0f0d, 0x0067, xboxone_hori_init, sizeof(xboxone_hori_init)},

    {0x0000, 0x0000, xboxone_test_init1, sizeof(xboxone_test_init1)},

    {0x0000, 0x0000, xboxone_fw2015_init, sizeof(xboxone_fw2015_init)},
    {0x0e6f, 0x0000, xboxone_pdp_init1, sizeof(xboxone_pdp_init1)},
    {0x0e6f, 0x0000, xboxone_pdp_init2, sizeof(xboxone_pdp_init2)},
    {0x24c6, 0x0000, xboxone_rumblebegin_init, sizeof(xboxone_rumblebegin_init)},
    {0x24c6, 0x0000, xboxone_rumbleend_init, sizeof(xboxone_rumbleend_init)},
};

XboxOneController::XboxOneController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), std::move(config), std::move(logger))
{
}

XboxOneController::~XboxOneController()
{
}

ams::Result XboxOneController::Initialize()
{
    R_TRY(BaseController::Initialize());
    R_TRY(SendInitBytes(0));

    R_SUCCEED();
}

ams::Result XboxOneController::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
{
    uint8_t input_bytes[64];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe[0]->Read(input_bytes, &size, IUSBEndpoint::USB_MODE_BLOCKING));

    uint8_t type = input_bytes[0];

    if (type == XBONEINPUT_BUTTON) // Button data
    {
        XboxOneButtonData *buttonData = reinterpret_cast<XboxOneButtonData *>(input_bytes);

        bool buttons_mapping[MAX_CONTROLLER_BUTTONS]{
            false,
            buttonData->button1,
            buttonData->button2,
            buttonData->button3,
            buttonData->button4,
            buttonData->button5,
            buttonData->button6,
            buttonData->button7,
            buttonData->button8,
            buttonData->button9,
            buttonData->button10,
            buttonData->button11,
            buttonData->button12};

        *input_idx = 0;

        normalData->triggers[0] = Normalize(GetConfig().triggerDeadzonePercent[0], buttonData->trigger_left, 0, 255);
        normalData->triggers[1] = Normalize(GetConfig().triggerDeadzonePercent[1], buttonData->trigger_right, 0, 255);

        normalData->sticks[0].axis_x = Normalize(GetConfig().stickDeadzonePercent[0], buttonData->stick_left_x, -32768, 32767);
        normalData->sticks[0].axis_y = Normalize(GetConfig().stickDeadzonePercent[0], -buttonData->stick_left_y, -32768, 32767);
        normalData->sticks[1].axis_x = Normalize(GetConfig().stickDeadzonePercent[1], buttonData->stick_right_x, -32768, 32767);
        normalData->sticks[1].axis_y = Normalize(GetConfig().stickDeadzonePercent[1], -buttonData->stick_right_y, -32768, 32767);

        normalData->buttons[ControllerButton::X] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::X]] ? true : false;
        normalData->buttons[ControllerButton::A] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::A]] ? true : false;
        normalData->buttons[ControllerButton::B] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::B]] ? true : false;
        normalData->buttons[ControllerButton::Y] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::Y]] ? true : false;
        normalData->buttons[ControllerButton::LSTICK_CLICK] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::LSTICK_CLICK]] ? true : false;
        normalData->buttons[ControllerButton::RSTICK_CLICK] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::RSTICK_CLICK]] ? true : false;
        normalData->buttons[ControllerButton::L] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::L]] ? true : false;
        normalData->buttons[ControllerButton::R] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::R]] ? true : false;

        normalData->buttons[ControllerButton::ZL] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::ZL]] ? true : false;
        normalData->buttons[ControllerButton::ZR] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::ZR]] ? true : false;

        if (GetConfig().buttons_pin[ControllerButton::ZL] == 0)
            normalData->buttons[ControllerButton::ZL] = normalData->triggers[0] > 0;
        if (GetConfig().buttons_pin[ControllerButton::ZR] == 0)
            normalData->buttons[ControllerButton::ZR] = normalData->triggers[1] > 0;

        normalData->buttons[ControllerButton::MINUS] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::MINUS]] ? true : false;
        normalData->buttons[ControllerButton::PLUS] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::PLUS]] ? true : false;
        normalData->buttons[ControllerButton::CAPTURE] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::CAPTURE]] ? true : false;
        normalData->buttons[ControllerButton::HOME] = buttons_mapping[GetConfig().buttons_pin[ControllerButton::HOME]] ? true : false;

        normalData->buttons[ControllerButton::DPAD_UP] = buttonData->dpad_up;
        normalData->buttons[ControllerButton::DPAD_RIGHT] = buttonData->dpad_right;
        normalData->buttons[ControllerButton::DPAD_DOWN] = buttonData->dpad_down;
        normalData->buttons[ControllerButton::DPAD_LEFT] = buttonData->dpad_left;
    }
    else if (type == XBONEINPUT_GUIDEBUTTON) // Guide button Result
    {
        m_GuidePressed = input_bytes[4];
        if (input_bytes[1] == 0x30)
        {
            R_TRY(WriteAckGuideReport(*input_idx, input_bytes[2]));
        }
    }

    R_SUCCEED();
}

ams::Result XboxOneController::SendInitBytes(uint16_t input_idx)
{
    uint16_t vendor = m_device->GetVendor();
    uint16_t product = m_device->GetProduct();
    for (int i = 0; i != (sizeof(init_packets) / sizeof(VendorProductPacket)); ++i)
    {
        if (init_packets[i].VendorID != 0 && init_packets[i].VendorID != vendor)
            continue;
        if (init_packets[i].ProductID != 0 && init_packets[i].ProductID != product)
            continue;

        LogPrint(LogLevelDebug, "XboxOneController: Sending init packet (Id=%d, Length=%d)", i, init_packets[i].Length);
        R_TRY(m_outPipe[input_idx]->Write(init_packets[i].Packet, init_packets[i].Length));
    }

    R_SUCCEED();
}

ams::Result XboxOneController::WriteAckGuideReport(uint16_t input_idx, uint8_t sequence)
{
    uint8_t report[] = {
        0x01, 0x20,
        sequence,
        0x09, 0x00, 0x07, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    R_RETURN(m_outPipe[input_idx]->Write(report, sizeof(report)));
}

bool XboxOneController::Support(ControllerFeature feature)
{
    if (feature == SUPPORTS_RUMBLE)
        return true;

    return false;
}

ams::Result XboxOneController::SetRumble(uint16_t input_idx, float amp_high, float amp_low)
{
    (void)input_idx;
    const uint8_t rumble_data[]{
        0x09, 0x00, 0x00,
        0x09, 0x00, 0x0f, 0x00, 0x00,
        (uint8_t)(amp_high * 255),
        (uint8_t)(amp_low * 255),
        0xff, 0x00, 0x00};
    R_RETURN(m_outPipe[input_idx]->Write(rumble_data, sizeof(rumble_data)));
}