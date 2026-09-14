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
#include <climits>
#include <cstdint>
#include <vector>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MMI::InputCli;

class TouchSwipeCommandTest : public Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(TouchSwipeCommandTest, BuildSwipeSteps_ZeroDuration_NoSteps, TestSize.Level1)
{
    const auto steps = BuildSwipeSteps(0, 1, 2, 3, 4);
    EXPECT_TRUE(steps.empty());
}

HWTEST_F(TouchSwipeCommandTest, BuildSwipeSteps_SixteenMs_SingleTargetStep, TestSize.Level1)
{
    const auto steps = BuildSwipeSteps(16, 1, 2, 3, 4);
    EXPECT_EQ(steps.size(), 1U);
    EXPECT_EQ(steps[0].x, 3);
    EXPECT_EQ(steps[0].y, 4);
    EXPECT_EQ(steps[0].delayMs, 16);
    EXPECT_TRUE(steps[0].move);
}

HWTEST_F(TouchSwipeCommandTest, BuildSwipeSteps_SeventeenMs_SplitsIntoTwoSteps, TestSize.Level1)
{
    const auto steps = BuildSwipeSteps(17, 0, 0, 10, 10);
    EXPECT_EQ(steps.size(), 2U);
    EXPECT_EQ(steps[0].delayMs + steps[1].delayMs, 17);
    EXPECT_EQ(steps.back().x, 10);
    EXPECT_EQ(steps.back().y, 10);
}

HWTEST_F(TouchSwipeCommandTest, BuildSwipeSteps_ShortDuration_SingleTargetStep, TestSize.Level1)
{
    const auto steps = BuildSwipeSteps(1, 1, 2, 3, 4);
    EXPECT_EQ(steps.size(), 1U);
    EXPECT_EQ(steps[0].x, 3);
    EXPECT_EQ(steps[0].y, 4);
    EXPECT_EQ(steps[0].delayMs, 1);
    EXPECT_TRUE(steps[0].move);
}

HWTEST_F(TouchSwipeCommandTest, BuildSwipeSteps_SameSourceAndTarget_NoMoveStep, TestSize.Level1)
{
    const auto steps = BuildSwipeSteps(32, 5, 6, 5, 6);
    EXPECT_EQ(steps.size(), 2U);
    EXPECT_FALSE(steps[0].move);
    EXPECT_FALSE(steps[1].move);
}

HWTEST_F(TouchSwipeCommandTest, BuildSwipeSteps_ExtremeCoordinates_NoOverflow, TestSize.Level1)
{
    const auto steps = BuildSwipeSteps(32, 0, 0, INT32_MAX, INT32_MAX);
    EXPECT_EQ(steps.size(), 2U);
    EXPECT_GT(steps[0].x, 0);
    EXPECT_GT(steps[0].y, 0);
    EXPECT_EQ(steps[1].x, INT32_MAX);
    EXPECT_EQ(steps[1].y, INT32_MAX);
}
