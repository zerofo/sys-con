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
    R_TRY(SendInitBytes());

    R_SUCCEED();
}

ams::Result XboxOneController::ReadInput(NormalizedButtonData *normalData, uint16_t *input_idx)
{
    uint8_t input_bytes[64];
    size_t size = sizeof(input_bytes);

    R_TRY(m_inPipe->Read(input_bytes, &size));

    uint8_t type = input_bytes[0];

    if (type == XBONEINPUT_BUTTON) // Button data
    {
        XboxOneButtonData *buttonData = reinterpret_cast<XboxOneButtonData *>(input_bytes);

        *input_idx = 0;

        normalData->triggers[0] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[0], buttonData->trigger_left);
        normalData->triggers[1] = NormalizeTrigger(GetConfig().triggerDeadzonePercent[1], buttonData->trigger_right);

        NormalizeAxis(buttonData->stick_left_x, buttonData->stick_left_y, GetConfig().stickDeadzonePercent[0],
                      &normalData->sticks[0].axis_x, &normalData->sticks[0].axis_y, -32768, 32767);
        NormalizeAxis(buttonData->stick_right_x, buttonData->stick_right_y, GetConfig().stickDeadzonePercent[1],
                      &normalData->sticks[1].axis_x, &normalData->sticks[1].axis_y, -32768, 32767);

        bool buttons[MAX_CONTROLLER_BUTTONS]{
            buttonData->y,
            buttonData->b,
            buttonData->a,
            buttonData->x,
            buttonData->stick_left_click,
            buttonData->stick_right_click,
            buttonData->bumper_left,
            buttonData->bumper_right,
            normalData->triggers[0] > 0,
            normalData->triggers[1] > 0,
            buttonData->back,
            buttonData->start,
            buttonData->dpad_up,
            buttonData->dpad_right,
            buttonData->dpad_down,
            buttonData->dpad_left,
            buttonData->sync,
            m_GuidePressed,
        };

        for (int i = 0; i != MAX_CONTROLLER_BUTTONS; ++i)
        {
            ControllerButton button = GetConfig().buttons[i];
            if (button == NONE)
                continue;

            normalData->buttons[(button != DEFAULT ? button - 2 : i)] += buttons[i];
        }
    }
    else if (type == XBONEINPUT_GUIDEBUTTON) // Guide button Result
    {
        m_GuidePressed = input_bytes[4];

        // Xbox one S needs to be sent an ack report for guide buttons
        // TODO: needs testing
        if (input_bytes[1] == 0x30)
        {
            R_TRY(WriteAckGuideReport(input_bytes[2]));
        }
    }

    R_SUCCEED();
}

ams::Result XboxOneController::SendInitBytes()
{
    uint16_t vendor = m_device->GetVendor();
    uint16_t product = m_device->GetProduct();
    for (int i = 0; i != (sizeof(init_packets) / sizeof(VendorProductPacket)); ++i)
    {
        if (init_packets[i].VendorID != 0 && init_packets[i].VendorID != vendor)
            continue;
        if (init_packets[i].ProductID != 0 && init_packets[i].ProductID != product)
            continue;

        R_TRY(m_outPipe->Write(init_packets[i].Packet, init_packets[i].Length));
    }

    R_SUCCEED();
}

ams::Result XboxOneController::WriteAckGuideReport(uint8_t sequence)
{
    uint8_t report[] = {
        0x01, 0x20,
        sequence,
        0x09, 0x00, 0x07, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    R_RETURN(m_outPipe->Write(report, sizeof(report)));
}

ams::Result XboxOneController::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    const uint8_t rumble_data[]{
        0x09, 0x00, 0x00,
        0x09, 0x00, 0x0f, 0x00, 0x00,
        strong_magnitude,
        weak_magnitude,
        0xff, 0x00, 0x00};
    R_RETURN(m_outPipe->Write(rumble_data, sizeof(rumble_data)));
}