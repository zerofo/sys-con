#include "Controllers/XboxOneController.h"
#include <vector>

// https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c
#define GIP_CMD_ACK           0x01
#define GIP_CMD_ANNOUNCE      0x02
#define GIP_CMD_STATUS        0x03
#define GIP_CMD_IDENTIFY      0x04
#define GIP_CMD_POWER         0x05
#define GIP_CMD_AUTHENTICATE  0x06
#define GIP_CMD_VIRTUAL_KEY   0x07
#define GIP_CMD_AUDIO_CONTROL 0x08
#define GIP_CMD_RUMBLE        0x09
#define GIP_CMD_LED           0x0a
#define GIP_CMD_FIRMWARE      0x0c
#define GIP_CMD_INPUT         0x20

#define GIP_SEQ(x) (x)

#define GIP_OPT_ACK      0x10
#define GIP_OPT_INTERNAL 0x20

#define GIP_PL_LEN(N) (N)

#define GIP_PWR_ON 0x00
#define GIP_LED_ON 0x01

#define GIP_MOTOR_R   (1 << 0)
#define GIP_MOTOR_L   (1 << 1)
#define GIP_MOTOR_RT  (1 << 2)
#define GIP_MOTOR_LT  (1 << 3)
#define GIP_MOTOR_ALL (GIP_MOTOR_R | GIP_MOTOR_L | GIP_MOTOR_RT | GIP_MOTOR_LT)

/*
How to find correct init sequence ?
    1. Use wireshark to capture the USB traffic
    2. Apply filter: "usb.src == host and usb.endpoint_address == 1" to filter the USB traffic
    3. Find to correct sequence
*/

/*
    "Sequences" seems important for some controllers (Like PDP)
    Linux controller hardcode the sequence to 0, however some controller like the PDP (0e6f:02de and 0e6f:0316) need to have an incremental sequence during init.
*/

static const uint8_t xboxone_hori_pdp_ack_id[] = {
    GIP_CMD_ACK, GIP_OPT_INTERNAL, GIP_SEQ(0), GIP_PL_LEN(9),
    0x00, GIP_CMD_IDENTIFY, GIP_OPT_INTERNAL, 0x3a, 0x00, 0x00, 0x00, 0x80, 0x00};

static const uint8_t xboxone_power_on[] = {
    GIP_CMD_POWER, GIP_OPT_INTERNAL, GIP_SEQ(1), GIP_PL_LEN(1), GIP_PWR_ON};

static const uint8_t xboxone_s_init[] = {
    GIP_CMD_POWER, GIP_OPT_INTERNAL, GIP_SEQ(2), 0x0f, 0x06};

static const uint8_t extra_input_packet_init[] = {
    0x4d, 0x10, 0x01 /*SEQ3?*/, 0x02, 0x07, 0x00};

static const uint8_t xboxone_pdp_led_on[] = {
    GIP_CMD_LED, GIP_OPT_INTERNAL, GIP_SEQ(3), GIP_PL_LEN(3), 0x00, GIP_LED_ON, 0x14};

static const uint8_t xboxone_pdp_auth0[] = { // This one is need for 0e6f:02de and 0e6f:0316 and maybe some others ?
    GIP_CMD_AUTHENTICATE, 0xa0, GIP_SEQ(4), 0x00, 0x92, 0x02};

static const uint8_t xboxone_pdp_8bit_auth[] = {
    GIP_CMD_AUTHENTICATE, GIP_OPT_INTERNAL, GIP_SEQ(5), GIP_PL_LEN(2), 0x01, 0x00};

// Seq on rumble can be reset to 1
static const uint8_t xboxone_rumblebegin_init[] = {
    GIP_CMD_RUMBLE, 0x00, GIP_SEQ(1), GIP_PL_LEN(9),
    0x00, GIP_MOTOR_ALL, 0x00, 0x00, 0x1D, 0x1D, 0xFF, 0x00, 0x00};

static const uint8_t xboxone_rumbleend_init[] = {
    GIP_CMD_RUMBLE, 0x00, GIP_SEQ(2), GIP_PL_LEN(9),
    0x00, GIP_MOTOR_ALL, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

struct xboxone_init_packet
{
    uint16_t idVendor;
    uint16_t idProduct;
    const uint8_t *data;
    int16_t len;
};

#define XBOXONE_INIT_PKT(_vid, _pid, _data) \
    {                                       \
        .idVendor = (_vid),                 \
        .idProduct = (_pid),                \
        .data = (_data),                    \
        .len = sizeof(_data),               \
    }

#define XBOXONE_INIT_PKT_STR(_vid, _pid, _data) \
    {                                           \
        .idVendor = (_vid),                     \
        .idProduct = (_pid),                    \
        .data = (const uint8_t *)(_data),       \
        .len = -1,                              \
    }

static const struct xboxone_init_packet xboxone_init_packets[] = {
    XBOXONE_INIT_PKT(0x0e6f, 0x0165, xboxone_hori_pdp_ack_id), // 0e6f-0165
    XBOXONE_INIT_PKT(0x0f0d, 0x0067, xboxone_hori_pdp_ack_id),
    XBOXONE_INIT_PKT(0x0000, 0x0000, xboxone_power_on),
    XBOXONE_INIT_PKT(0x045e, 0x02ea, xboxone_s_init),
    XBOXONE_INIT_PKT(0x045e, 0x0b00, xboxone_s_init),
    XBOXONE_INIT_PKT(0x045e, 0x0b00, extra_input_packet_init),
    XBOXONE_INIT_PKT(0x0e6f, 0x0000, xboxone_pdp_led_on),
    XBOXONE_INIT_PKT(0x0e6f, 0x0000, xboxone_pdp_auth0), // 0e6f-02de and 0e6f-0316
    XBOXONE_INIT_PKT(0x0e6f, 0x0000, xboxone_pdp_8bit_auth),
    XBOXONE_INIT_PKT(0x2dc8, 0x0000, xboxone_pdp_8bit_auth), // 2dc8-2008
    XBOXONE_INIT_PKT(0x24c6, 0x541a, xboxone_rumblebegin_init),
    XBOXONE_INIT_PKT(0x24c6, 0x542a, xboxone_rumblebegin_init),
    XBOXONE_INIT_PKT(0x24c6, 0x543a, xboxone_rumblebegin_init),
    XBOXONE_INIT_PKT(0x24c6, 0x541a, xboxone_rumbleend_init),
    XBOXONE_INIT_PKT(0x24c6, 0x542a, xboxone_rumbleend_init),
    XBOXONE_INIT_PKT(0x24c6, 0x543a, xboxone_rumbleend_init),
};
/*
static const struct xboxone_init_packet xboxone_init_packets[] = {
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "04200100"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "012002090004203a000000fa00"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "01200209000420220100001200"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "01200209000420340100000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "0520020f060000000000005553000000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "0520030100"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "0a200403000114"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "0630013a00410001002c01010028f1eda4453a6adb2e2108b8eb084c05352e64e5c6267d330ba435765c76fd7ad000000000457bafe90000000000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "01200109000620060000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "0630020e0042000200540000000000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "012002090006203a0000002000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "012002090006205a0000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "0630030e0042000304040000000000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "012003090006203a000000ff02"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "01200309000620220100001702"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "0120030900062044020000f500"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "01200309000620390300000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "06f0043a920200410005010405010100af711ffa412e24f08692f328f6668d77c17f89dc5515b5ec2e14f857453675c40225d9321081d653332e9d4df828e474"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "06a004ba003a2b5f6d5ecb24a98c1855c33957fccd197e2892368c2b65f4cabd1777e0942732addc03308578d94bccb956d196a4434bfa618e730977fd35b22b"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "06a004ba007441bf4884ae4fd21685634c2e07b8eec79d0636ecb58f6c509f1abfbf60f3fc410220430d612bf9be0481d6d8bff856ec3636e9eee6a3deb3dad3"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "06a0043aae01bf427024fc6f76ca92f981226ac1f5b1d70702902e13dfdc9feec72b6f898a44ea6ea26f9ada85100c733a96fce928bd790c1b86abd1c1ff74be"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "06b0042ae801f2e2886657a218bde99a1caf11ec54d4a245ddddeb182d356ac3af48af9ef17429380000000000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "06a004009202"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "01200409000620060000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "0630053200410007002407010020676b579cb3467e514a3c4204319ae919b6c29614fcec7c8950b306e37fd003b10000000000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "01200509000620060000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "0630060e0042000800440000000000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "012006090006203a0000001000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "012006090006204a0000000000"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "062007020100"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "09000109000f00000000ff00eb"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "09000209000f00000000ff00eb"),
    XBOXONE_INIT_PKT_STR(0x0000, 0x0000, "09000309000f00000000ff00eb"),
};
*/
static std::vector<uint8_t> strToByteArray(const std::string &str)
{
    std::vector<uint8_t> byteArray;
    for (size_t i = 0; i < str.size(); i += 2)
    {
        std::string byteStr = str.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
        byteArray.push_back(byte);
    }
    return byteArray;
}

XboxOneController::XboxOneController(std::unique_ptr<IUSBDevice> &&device, const ControllerConfig &config, std::unique_ptr<ILogger> &&logger)
    : BaseController(std::move(device), std::move(config), std::move(logger))
{
}

XboxOneController::~XboxOneController()
{
}

ControllerResult XboxOneController::Initialize()
{
    ControllerResult result = BaseController::Initialize();
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    result = SendInitBytes(0);
    if (result != CONTROLLER_STATUS_SUCCESS)
        return result;

    return CONTROLLER_STATUS_SUCCESS;
}

ControllerResult XboxOneController::ParseData(uint8_t *buffer, size_t size, RawInputData *rawData, uint16_t *input_idx)
{
    (void)input_idx;
    XboxOneButtonData *buttonData = reinterpret_cast<XboxOneButtonData *>(buffer);

    if (buttonData->type == GIP_CMD_INPUT) // Button data
    {
        if (size < sizeof(XboxOneButtonData))
        {
            Log(LogLevelError, "XboxOneController[%04x-%04x] Unexpected data size (%d < %d)", m_device->GetVendor(), m_device->GetProduct(), size, sizeof(XboxOneButtonData));
            return CONTROLLER_STATUS_UNEXPECTED_DATA;
        }

        m_rawInput.buttons[1] = buttonData->button1;
        m_rawInput.buttons[2] = buttonData->button2;
        m_rawInput.buttons[3] = buttonData->button3;
        m_rawInput.buttons[4] = buttonData->button4;
        m_rawInput.buttons[5] = buttonData->button5;
        m_rawInput.buttons[6] = buttonData->button6;
        m_rawInput.buttons[7] = buttonData->button7;
        m_rawInput.buttons[8] = buttonData->button8;
        m_rawInput.buttons[9] = buttonData->button9;
        m_rawInput.buttons[10] = buttonData->button10;
        m_rawInput.buttons[11] = buttonData->button11;

        m_rawInput.analog[ControllerAnalogType_Rx] = BaseController::Normalize(buttonData->trigger_left, 0, 1023);
        m_rawInput.analog[ControllerAnalogType_Ry] = BaseController::Normalize(buttonData->trigger_right, 0, 1023);

        m_rawInput.analog[ControllerAnalogType_X] = BaseController::Normalize(buttonData->stick_left_x, -32768, 32767);
        m_rawInput.analog[ControllerAnalogType_Y] = BaseController::Normalize(-buttonData->stick_left_y, -32768, 32767);
        m_rawInput.analog[ControllerAnalogType_Z] = BaseController::Normalize(buttonData->stick_right_x, -32768, 32767);
        m_rawInput.analog[ControllerAnalogType_Rz] = BaseController::Normalize(-buttonData->stick_right_y, -32768, 32767);

        m_rawInput.buttons[DPAD_UP_BUTTON_ID] = buttonData->dpad_up;
        m_rawInput.buttons[DPAD_RIGHT_BUTTON_ID] = buttonData->dpad_right;
        m_rawInput.buttons[DPAD_DOWN_BUTTON_ID] = buttonData->dpad_down;
        m_rawInput.buttons[DPAD_LEFT_BUTTON_ID] = buttonData->dpad_left;

        *rawData = m_rawInput;

        return CONTROLLER_STATUS_SUCCESS;
    }
    else if (buttonData->type == GIP_CMD_VIRTUAL_KEY) // Mode button (XBOX center button)
    {
        if (size < 6)
        {
            Log(LogLevelError, "XboxOneController[%04x-%04x] Unexpected data size (%d < %d)", m_device->GetVendor(), m_device->GetProduct(), size, sizeof(XboxOneButtonData));
            return CONTROLLER_STATUS_UNEXPECTED_DATA;
        }

        m_rawInput.buttons[12] = buffer[4];

        if (buffer[1] == (GIP_OPT_ACK | GIP_OPT_INTERNAL))
        {
            ControllerResult result = WriteAckModeReport(*input_idx, buffer[2]);
            if (result != CONTROLLER_STATUS_SUCCESS)
                return result;
        }

        *rawData = m_rawInput;

        return CONTROLLER_STATUS_SUCCESS;
    }

    return CONTROLLER_STATUS_NOTHING_TODO;
}

ControllerResult XboxOneController::SendInitBytes(uint16_t input_idx)
{
    if (m_outPipe.size() <= input_idx)
        return CONTROLLER_STATUS_SUCCESS;

    for (size_t i = 0; i < sizeof(xboxone_init_packets) / sizeof(struct xboxone_init_packet); i++)
    {
        if (xboxone_init_packets[i].idVendor != 0 && xboxone_init_packets[i].idVendor != m_device->GetVendor())
            continue;
        if (xboxone_init_packets[i].idProduct != 0 && xboxone_init_packets[i].idProduct != m_device->GetProduct())
            continue;

        if (xboxone_init_packets[i].len == -1)
        {
            std::vector<uint8_t> buffer = strToByteArray(reinterpret_cast<const char *>(xboxone_init_packets[i].data));
            ControllerResult result = m_outPipe[input_idx]->Write(buffer.data(), buffer.size());
            if (result != CONTROLLER_STATUS_SUCCESS)
                return result;
        }
        else
        {
            ControllerResult result = m_outPipe[input_idx]->Write(xboxone_init_packets[i].data, xboxone_init_packets[i].len);
            if (result != CONTROLLER_STATUS_SUCCESS)
                return result;
        }

        /*
        // Keep this part of the code, it seems not needed to read while sending init bytes however it can be useful for debugging
        uint8_t buffer[256];
        size_t size = sizeof(buffer);
        m_inPipe[input_idx]->Read(buffer, &size, 1000 * 1000); // 1000ms timeout
        */
    }

    return CONTROLLER_STATUS_SUCCESS;
}

ControllerResult XboxOneController::WriteAckModeReport(uint16_t input_idx, uint8_t sequence)
{
    uint8_t report[] = {
        GIP_CMD_ACK, GIP_OPT_INTERNAL,
        sequence,
        GIP_PL_LEN(9), 0x00, GIP_CMD_VIRTUAL_KEY, GIP_OPT_INTERNAL, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};

    if (m_outPipe.size() <= input_idx)
        return CONTROLLER_STATUS_SUCCESS;

    return m_outPipe[input_idx]->Write(report, sizeof(report));
}

bool XboxOneController::Support(ControllerFeature feature)
{
    if (feature == SUPPORTS_RUMBLE)
        return true;

    return false;
}

ControllerResult XboxOneController::SetRumble(uint16_t input_idx, float amp_high, float amp_low)
{
    (void)input_idx;
    const uint8_t rumble_data[]{
        0x09, 0x00, 0x00,
        0x09, 0x00, 0x0f, 0x00, 0x00,
        (uint8_t)(amp_high * 255),
        (uint8_t)(amp_low * 255),
        0xff, 0x00, 0x00};

    if (m_outPipe.size() <= input_idx)
        return CONTROLLER_STATUS_INVALID_INDEX;

    return m_outPipe[input_idx]->Write(rumble_data, sizeof(rumble_data));
}