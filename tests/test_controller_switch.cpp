#include <gtest/gtest.h>
#include "Controllers/SwitchController.h"

/* --------------------------- Test setup --------------------------- */

class MockDevice : public IUSBDevice
{
public:
    ControllerResult Open() override { return CONTROLLER_STATUS_SUCCESS; }
    void Close() override {}
    void Reset() override {}
};

class MockLogger : public ILogger
{
public:
    void Log(LogLevel aLogLevel, const char *format, ::std::va_list vl) override {}
    void LogBuffer(LogLevel aLogLevel, const uint8_t *buffer, size_t size) override {}
};

/* --------------------------- Tests --------------------------- */

TEST(Controller, test_switch_button1)
{
    ControllerConfig config;
    RawInputData rawData;
    uint16_t input_idx = 0;
    SwitchController controller(std::make_unique<MockDevice>(), config, std::make_unique<MockLogger>());

    uint8_t buffer[64] = {0x30, 0x0D, 0x91, 0x01, 0x80, 0x00, 0xB9, 0x77, 0x7D, 0xDB, 0xF7, 0x7B, 0x09, 0x00, 0x00, 0x00};
    EXPECT_EQ(controller.ParseData(buffer, sizeof(buffer), &rawData, &input_idx), CONTROLLER_STATUS_SUCCESS);

    EXPECT_TRUE(rawData.buttons[1]);
}

TEST(Controller, test_switch_lstick_left)
{
    ControllerConfig config;
    RawInputData rawData;
    uint16_t input_idx = 0;

    SwitchController controller(std::make_unique<MockDevice>(), config, std::make_unique<MockLogger>());

    uint8_t buffer[64] = {0x30, 0xFC, 0x91, 0x00, 0x80, 0x00, 0xC6, 0xB1, 0x72, 0xE5, 0xE7, 0x79, 0x03, 0x00, 0x00, 0x00};
    EXPECT_EQ(controller.ParseData(buffer, sizeof(buffer), &rawData, &input_idx), CONTROLLER_STATUS_SUCCESS);

    EXPECT_FLOAT_EQ(rawData.X, -1.0f);
}
