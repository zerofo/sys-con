#include <gtest/gtest.h>
#include "Controllers/BaseController.h"

TEST(UTILS, test_normalize)
{
    //-32768, 32767
    EXPECT_FLOAT_EQ(BaseController::Normalize(-32768, -32768, 32767), -1.0f);
    EXPECT_FLOAT_EQ(BaseController::Normalize(0, -32768, 32767), 0.0f);
    EXPECT_FLOAT_EQ(BaseController::Normalize(32767, -32768, 32767), 1.0f);
    EXPECT_FLOAT_EQ(BaseController::Normalize(0, -32768, 32767), 0.0f);
    EXPECT_FLOAT_EQ(BaseController::Normalize(0, -32768, 32767), 0.0f);
    EXPECT_NEAR(BaseController::Normalize(-128, -32768, 32767), 0.0f, 0.01f);

    // 0, 255
    EXPECT_NEAR(BaseController::Normalize(128, 0, 255), 0.0f, 0.01f);
    EXPECT_FLOAT_EQ(BaseController::Normalize(0, 0, 255), -1.0f);
    EXPECT_NEAR(BaseController::Normalize(109, 0, 255), -0.14f, 0.01f);
    EXPECT_FLOAT_EQ(BaseController::Normalize(0, 0, 255), -1.0f);
}
