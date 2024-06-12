#include "Controllers/XboxOneController.h"

// https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c
#define GIP_CMD_ACK          0x01
#define GIP_CMD_IDENTIFY     0x04
#define GIP_CMD_POWER        0x05
#define GIP_CMD_AUTHENTICATE 0x06
#define GIP_CMD_VIRTUAL_KEY  0x07
#define GIP_CMD_RUMBLE       0x09
#define GIP_CMD_LED          0x0a
#define GIP_CMD_FIRMWARE     0x0c
#define GIP_CMD_INPUT        0x20

#define GIP_SEQ(x) (x)

#define GIP_OPT_ACK      0x10
#define GIP_OPT_INTERNAL 0x20

#define GIP_PL_LEN(N) (N)

#define GIP_PWR_ON 0x00
#define GIP_LED_ON 0x01

#define GIP_MOTOR_R   BIT(0)
#define GIP_MOTOR_L   BIT(1)
#define GIP_MOTOR_RT  BIT(2)
#define GIP_MOTOR_LT  BIT(3)
#define GIP_MOTOR_ALL (GIP_MOTOR_R | GIP_MOTOR_L | GIP_MOTOR_RT | GIP_MOTOR_LT)

/*
    Sequence seems important for some controllers
    Linux controller hardcode the sequence to 0, however some controller like the PDP (0e6f:02de and 0e6f:0316) need to have an incremental sequence during init.
*/

static const u8 xboxone_hori_ack_id[] = {
    GIP_CMD_ACK, GIP_OPT_INTERNAL, GIP_SEQ(0), GIP_PL_LEN(9),
    0x00, GIP_CMD_IDENTIFY, GIP_OPT_INTERNAL, 0x3a, 0x00, 0x00, 0x00, 0x80, 0x00};

static const u8 xboxone_power_on[] = {
    GIP_CMD_POWER, GIP_OPT_INTERNAL, GIP_SEQ(1), GIP_PL_LEN(1), GIP_PWR_ON};

static const u8 xboxone_s_init[] = {
    GIP_CMD_POWER, GIP_OPT_INTERNAL, GIP_SEQ(2), 0x0f, 0x06};

static const u8 extra_input_packet_init[] = {
    0x4d, 0x10, 0x01 /*SEQ3?*/, 0x02, 0x07, 0x00};

static const u8 xboxone_pdp_led_on[] = {
    GIP_CMD_LED, GIP_OPT_INTERNAL, GIP_SEQ(3), GIP_PL_LEN(3), 0x00, GIP_LED_ON, 0x14};

static const u8 xboxone_pdp_auth1[] = { // This one is need for 0e6f:02de and 0e6f:0316 and maybe some others ?
    GIP_CMD_AUTHENTICATE, 0xa0, GIP_SEQ(4), 0x00, 0x92, 0x02};

static const u8 xboxone_pdp_auth2[] = {
    GIP_CMD_AUTHENTICATE, GIP_OPT_INTERNAL, GIP_SEQ(5), GIP_PL_LEN(2), 0x01, 0x00};

// Seq on rumble can be reset to 1
static const u8 xboxone_rumblebegin_init[] = {
    GIP_CMD_RUMBLE, 0x00, GIP_SEQ(1), GIP_PL_LEN(9),
    0x00, GIP_MOTOR_ALL, 0x00, 0x00, 0x1D, 0x1D, 0xFF, 0x00, 0x00};

static const u8 xboxone_rumbleend_init[] = {
    GIP_CMD_RUMBLE, 0x00, GIP_SEQ(2), GIP_PL_LEN(9),
    0x00, GIP_MOTOR_ALL, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

struct xboxone_init_packet
{
    u16 idVendor;
    u16 idProduct;
    const u8 *data;
    u8 len;
};

#define XBOXONE_INIT_PKT(_vid, _pid, _data) \
    {                                       \
        .idVendor = (_vid),                 \
        .idProduct = (_pid),                \
        .data = (_data),                    \
        .len = sizeof(_data),               \
    }

static const struct xboxone_init_packet xboxone_init_packets[] = {
    XBOXONE_INIT_PKT(0x0e6f, 0x0165, xboxone_hori_ack_id),
    XBOXONE_INIT_PKT(0x0f0d, 0x0067, xboxone_hori_ack_id),
    XBOXONE_INIT_PKT(0x0000, 0x0000, xboxone_power_on),
    XBOXONE_INIT_PKT(0x045e, 0x02ea, xboxone_s_init),
    XBOXONE_INIT_PKT(0x045e, 0x0b00, xboxone_s_init),
    XBOXONE_INIT_PKT(0x045e, 0x0b00, extra_input_packet_init),
    XBOXONE_INIT_PKT(0x0e6f, 0x0000, xboxone_pdp_led_on),
    XBOXONE_INIT_PKT(0x0e6f, 0x0000, xboxone_pdp_auth1), // This one is not in the linux driver but it is needed for 0e6f:02de and 0e6f:0316 - We send it everytime, we will see if it break something
    XBOXONE_INIT_PKT(0x0e6f, 0x0000, xboxone_pdp_auth2),
    XBOXONE_INIT_PKT(0x24c6, 0x541a, xboxone_rumblebegin_init),
    XBOXONE_INIT_PKT(0x24c6, 0x542a, xboxone_rumblebegin_init),
    XBOXONE_INIT_PKT(0x24c6, 0x543a, xboxone_rumblebegin_init),
    XBOXONE_INIT_PKT(0x24c6, 0x541a, xboxone_rumbleend_init),
    XBOXONE_INIT_PKT(0x24c6, 0x542a, xboxone_rumbleend_init),
    XBOXONE_INIT_PKT(0x24c6, 0x543a, xboxone_rumbleend_init),
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

    R_TRY(m_inPipe[0]->Read(input_bytes, &size, UINT64_MAX));

    uint8_t type = input_bytes[0];

    *input_idx = 0;

    if (type == GIP_CMD_INPUT) // Button data
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
            buttonData->button12,
            m_button13};

        normalData->triggers[0] = Normalize(GetConfig().triggerDeadzonePercent[0], buttonData->trigger_left, 0, 1023);
        normalData->triggers[1] = Normalize(GetConfig().triggerDeadzonePercent[1], buttonData->trigger_right, 0, 1023);

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

        R_SUCCEED();
    }
    else if (type == GIP_CMD_VIRTUAL_KEY) // Mode button (XBOX center button)
    {
        m_button13 = input_bytes[4];

        if (input_bytes[1] == (GIP_OPT_ACK | GIP_OPT_INTERNAL))
            R_TRY(WriteAckModeReport(*input_idx, input_bytes[2]));

        for (int button = 0; button < ControllerButton::COUNT; button++)
        {
            if (GetConfig().buttons_pin[button] == 13)
            {
                normalData->buttons[button] = m_button13;
                R_SUCCEED();
            }
        }
    }

    R_RETURN(CONTROL_ERR_NOTHING_TODO);
}

ams::Result XboxOneController::SendInitBytes(uint16_t input_idx)
{
    uint16_t vendor = m_device->GetVendor();
    uint16_t product = m_device->GetProduct();
    for (size_t i = 0; i < sizeof(xboxone_init_packets) / sizeof(struct xboxone_init_packet); i++)
    {
        if (xboxone_init_packets[i].idVendor != 0 && xboxone_init_packets[i].idVendor != vendor)
            continue;
        if (xboxone_init_packets[i].idProduct != 0 && xboxone_init_packets[i].idProduct != product)
            continue;

        R_TRY(m_outPipe[input_idx]->Write(xboxone_init_packets[i].data, xboxone_init_packets[i].len));

        /*
        // Keep this part of the code, it seems not needed to read while sending init bytes however it can be useful for debugging
        uint8_t buffer[256];
        size_t size = sizeof(buffer);
        m_inPipe[input_idx]->Read(buffer, &size, 1000 * 1000); // 1000ms timeout
        */
    }

    R_SUCCEED();
}

ams::Result XboxOneController::WriteAckModeReport(uint16_t input_idx, uint8_t sequence)
{
    uint8_t report[] = {
        GIP_CMD_ACK, GIP_OPT_INTERNAL,
        sequence,
        GIP_PL_LEN(9), 0x00, GIP_CMD_VIRTUAL_KEY, GIP_OPT_INTERNAL, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};

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