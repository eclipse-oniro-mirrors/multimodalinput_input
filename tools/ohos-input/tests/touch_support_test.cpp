/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include "touch_support.h"
#include <cstdint>
#include <set>
#include <string>
#include <vector>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MMI::InputCli;

class TouchSupportTest : public Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(TouchSupportTest, ParseTouchPoint_MissingOptionalOptions_UsesDefaults, TestSize.Level1)
{
    Options options { { "--x", "10" }, { "--y", "20" } };
    TouchPointOptions result;
    std::string error;
    EXPECT_TRUE(ParseTouchPoint(options, result, error));
    EXPECT_EQ(result.x, 10);
    EXPECT_EQ(result.y, 20);
    EXPECT_EQ(result.displayId, 0);
    EXPECT_EQ(result.duration, DEFAULT_TOUCH_TAP_DURATION_MS);
}

HWTEST_F(TouchSupportTest, ParseTouchPoint_AllOptions_Parsed, TestSize.Level1)
{
    Options options { { "--x", "1" }, { "--y", "2" }, { "--displayId", "3" }, { "--duration", "800" } };
    TouchPointOptions result;
    std::string error;
    EXPECT_TRUE(ParseTouchPoint(options, result, error));
    EXPECT_EQ(result.x, 1);
    EXPECT_EQ(result.y, 2);
    EXPECT_EQ(result.displayId, 3);
    EXPECT_EQ(result.duration, 800);
}

HWTEST_F(TouchSupportTest, ParseTouchPoint_DurationOutOfRange_Rejected, TestSize.Level1)
{
    const char *values[] = { "5001", "-1" };
    for (const char *value : values) {
        Options options { { "--x", "1" }, { "--y", "2" }, { "--duration", value } };
        TouchPointOptions result;
        std::string error;
        EXPECT_FALSE(ParseTouchPoint(options, result, error));
        EXPECT_FALSE(error.empty());
    }
}

HWTEST_F(TouchSupportTest, ParseTouchSwipe_MissingOptionalOptions_UsesDefaults, TestSize.Level1)
{
    Options options { { "--srcX", "1" }, { "--srcY", "2" }, { "--dstX", "3" }, { "--dstY", "4" } };
    TouchSwipeOptions result;
    std::string error;
    EXPECT_TRUE(ParseTouchSwipe(options, result, error));
    EXPECT_EQ(result.srcX, 1);
    EXPECT_EQ(result.srcY, 2);
    EXPECT_EQ(result.dstX, 3);
    EXPECT_EQ(result.dstY, 4);
    EXPECT_EQ(result.displayId, 0);
    EXPECT_EQ(result.duration, DEFAULT_TOUCH_SWIPE_DURATION_MS);
}

HWTEST_F(TouchSupportTest, ParseTouchSwipe_AllOptions_Parsed, TestSize.Level1)
{
    Options options { { "--srcX", "1" }, { "--srcY", "2" }, { "--dstX", "3" }, { "--dstY", "4" },
        { "--displayId", "1" }, { "--duration", "2000" } };
    TouchSwipeOptions result;
    std::string error;
    EXPECT_TRUE(ParseTouchSwipe(options, result, error));
    EXPECT_EQ(result.displayId, 1);
    EXPECT_EQ(result.duration, 2000);
}

HWTEST_F(TouchSupportTest, ParseTouchSwipe_NegativeCoordinate_Rejected, TestSize.Level1)
{
    Options options { { "--srcX", "-1" }, { "--srcY", "2" }, { "--dstX", "3" }, { "--dstY", "4" } };
    TouchSwipeOptions result;
    std::string error;
    EXPECT_FALSE(ParseTouchSwipe(options, result, error));
    EXPECT_FALSE(error.empty());
}

HWTEST_F(TouchSupportTest, ParseTouchSwipe_DurationOutOfRange_Rejected, TestSize.Level1)
{
    const char *values[] = { "5001", "-1" };
    for (const char *value : values) {
        Options options { { "--srcX", "1" }, { "--srcY", "2" }, { "--dstX", "3" }, { "--dstY", "4" },
            { "--duration", value } };
        TouchSwipeOptions result;
        std::string error;
        EXPECT_FALSE(ParseTouchSwipe(options, result, error));
        EXPECT_FALSE(error.empty());
    }
}

HWTEST_F(TouchSupportTest, ParseOptionPairs_UnknownOption_Rejected, TestSize.Level1)
{
    const std::set<std::string> allowed = { "--displayId", "--duration", "--x", "--y" };
    std::vector<std::string> args { "--x", "1", "--y", "2", "--unknown", "x" };
    Options options;
    std::string error;
    EXPECT_FALSE(ParseOptionPairs(args, allowed, options, error));
    EXPECT_FALSE(error.empty());
}
