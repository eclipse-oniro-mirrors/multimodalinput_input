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

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

#include "command_runner.h"
#include "error_multimodal.h"
#include "mock_controller_factory.h"
#include "printer.h"

using namespace testing;
using namespace testing::ext;
using OHOS::MMI::ERROR_NO_PERMISSION;
using namespace OHOS::MMI::InputCli;

namespace {
void ExpectTouchSuccess(const CommandResult &result, const std::string &action)
{
    EXPECT_EQ(result.code, 0);
    const auto parsed = ParseJson(result);
    EXPECT_EQ(parsed["type"], "result");
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_EQ(parsed["data"]["action"], action);
}

void ExpectTouchError(const CommandResult &result, const std::string &errCode)
{
    const auto parsed = ParseJson(result);
    EXPECT_EQ(parsed["type"], "result");
    EXPECT_EQ(parsed["status"], "failed");
    EXPECT_EQ(parsed["errCode"], errCode);
    EXPECT_FALSE(parsed["errMsg"].get<std::string>().empty());
    EXPECT_FALSE(parsed["suggestion"].get<std::string>().empty());
    EXPECT_EQ(result.stderrText, "");
}
} // namespace

class InputTouchCommandTest : public Test {};

HWTEST_F(InputTouchCommandTest, TouchTap_MissingYParam_ReturnsParameterErrorWithoutIPC, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    fixture.Configure(calls);
    const auto result = OHOS::MMI::InputCli::Run({ "touch-tap", "--x", "500" });
    EXPECT_EQ(result.code, PARAMETER_EXIT);
    ExpectTouchError(result, "ERR_PARAMETER_ERROR");
    EXPECT_EQ(fixture.TouchCreateCount(), 0);
    EXPECT_TRUE(calls.empty());
}

HWTEST_F(InputTouchCommandTest, TouchTap_InvalidDurationAndNegativeCoordinate_ReturnParameterError, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    fixture.Configure(calls);
    const auto durationResult = OHOS::MMI::InputCli::Run({ "touch-tap", "--x", "500", "--y", "800", "--duration", "6000" });
    EXPECT_EQ(durationResult.code, PARAMETER_EXIT);
    ExpectTouchError(durationResult, "ERR_PARAMETER_ERROR");
    const auto coordinateResult = OHOS::MMI::InputCli::Run({ "touch-tap", "--x", "-1", "--y", "800" });
    EXPECT_EQ(coordinateResult.code, PARAMETER_EXIT);
    ExpectTouchError(coordinateResult, "ERR_PARAMETER_ERROR");
    EXPECT_EQ(fixture.TouchCreateCount(), 0);
    EXPECT_TRUE(calls.empty());
}

HWTEST_F(InputTouchCommandTest, TouchSwipe_MissingDstYAndNegativeDuration_ReturnParameterError, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    fixture.Configure(calls);
    const auto missingResult = OHOS::MMI::InputCli::Run({ "touch-swipe", "--srcX", "100", "--srcY", "500", "--dstX", "900" });
    EXPECT_EQ(missingResult.code, PARAMETER_EXIT);
    ExpectTouchError(missingResult, "ERR_PARAMETER_ERROR");
    const auto durationResult = OHOS::MMI::InputCli::Run({ "touch-swipe", "--srcX", "100", "--srcY", "500", "--dstX", "900",
        "--dstY", "500", "--duration", "-1" });
    EXPECT_EQ(durationResult.code, PARAMETER_EXIT);
    ExpectTouchError(durationResult, "ERR_PARAMETER_ERROR");
    EXPECT_EQ(fixture.TouchCreateCount(), 0);
    EXPECT_TRUE(calls.empty());
}

HWTEST_F(InputTouchCommandTest, TouchCommands_RejectUnknownDuplicateAndNegativeDisplayOptions, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    fixture.Configure(calls);
    const auto duplicate = OHOS::MMI::InputCli::Run({ "touch-tap", "--x", "1", "--y", "2", "--x", "3" });
    EXPECT_EQ(duplicate.code, PARAMETER_EXIT);
    ExpectTouchError(duplicate, "ERR_PARAMETER_ERROR");
    const auto display = OHOS::MMI::InputCli::Run({ "touch-swipe", "--srcX", "1", "--srcY", "2", "--dstX", "3",
        "--dstY", "4", "--displayId", "-1" });
    EXPECT_EQ(display.code, PARAMETER_EXIT);
    ExpectTouchError(display, "ERR_PARAMETER_ERROR");
    EXPECT_EQ(fixture.TouchCreateCount(), 0);
    EXPECT_TRUE(calls.empty());
}

HWTEST_F(InputTouchCommandTest, TouchTap_ZeroDuration_InjectsDownThenUp, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    fixture.Configure(calls);
    const auto result = OHOS::MMI::InputCli::Run({ "touch-tap", "--x", "15", "--y", "25", "--duration", "0" });
    ExpectTouchSuccess(result, "touch-tap");
    EXPECT_EQ(result.stderrText, "");
    EXPECT_EQ(fixture.TouchCreateCount(), 1);
    ExpectCalls(calls, { { "CreateTouchController", {} }, { "TouchDown", { 0, 0, 15, 25 } },
        { "TouchUp", { 0, 0, 15, 25 } } });
}

HWTEST_F(InputTouchCommandTest, TouchTap_SpecExample_HoldsForRequestedDuration, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    fixture.Configure(calls);
    const auto start = std::chrono::steady_clock::now();
    const auto result = OHOS::MMI::InputCli::Run({ "touch-tap", "--displayId", "0", "--x", "500", "--y", "800", "--duration", "200" });
    const auto elapsed = std::chrono::steady_clock::now() - start;
    ExpectTouchSuccess(result, "touch-tap");
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 180);
    const auto data = ParseJson(result)["data"];
    EXPECT_EQ(data["displayId"], 0);
    EXPECT_EQ(data["x"], 500);
    EXPECT_EQ(data["y"], 800);
    EXPECT_EQ(data["duration"], 200);
    ExpectCalls(calls, { { "CreateTouchController", {} }, { "TouchDown", { 0, 0, 500, 800 } },
        { "TouchUp", { 0, 0, 500, 800 } } });
}

HWTEST_F(InputTouchCommandTest, TouchSwipe_ZeroDuration_InjectsDownThenUpWithoutMove, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    fixture.Configure(calls);
    const auto result = OHOS::MMI::InputCli::Run({ "touch-swipe", "--displayId", "2", "--srcX", "10", "--srcY", "20",
        "--dstX", "30", "--dstY", "40", "--duration", "0" });
    ExpectTouchSuccess(result, "touch-swipe");
    ExpectCalls(calls, { { "CreateTouchController", {} }, { "TouchDown", { 0, 2, 10, 20 } },
        { "TouchUp", { 0, 2, 30, 40 } } });
}

HWTEST_F(InputTouchCommandTest, TouchSwipe_ShortDuration_InjectsOrderedMovesAndEndpointUp, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    fixture.Configure(calls);
    const auto result = OHOS::MMI::InputCli::Run({ "touch-swipe", "--displayId", "1", "--srcX", "0", "--srcY", "20",
        "--dstX", "100", "--dstY", "40", "--duration", "256" });
    ExpectTouchSuccess(result, "touch-swipe");
    ASSERT_GE(calls.size(), 4U);
    EXPECT_EQ(calls.front().name, "CreateTouchController");
    EXPECT_EQ(calls[1].name, "TouchDown");
    EXPECT_EQ(calls[1].values, (std::vector<int32_t> { 0, 1, 0, 20 }));
    EXPECT_EQ(calls.back().name, "TouchUp");
    EXPECT_EQ(calls.back().values, (std::vector<int32_t> { 0, 1, 100, 40 }));
    EXPECT_GE(std::count_if(calls.begin(), calls.end(), [](const RecordedCall &call) {
        return call.name == "TouchMove";
    }), 1);
    for (size_t index = 2; index + 1 < calls.size(); ++index) {
        ASSERT_EQ(calls[index].name, "TouchMove");
        ASSERT_EQ(calls[index].values.size(), 4U);
        EXPECT_EQ(calls[index].values[0], 0);
        EXPECT_EQ(calls[index].values[1], 1);
        EXPECT_GE(calls[index].values[2], 0);
        EXPECT_LE(calls[index].values[2], 100);
        EXPECT_GE(calls[index].values[3], 20);
        EXPECT_LE(calls[index].values[3], 40);
    }
}

HWTEST_F(InputTouchCommandTest, TouchSwipe_SpecExample_InjectsLinearMoveSequenceAndEndpoint, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    fixture.Configure(calls);
    const auto start = std::chrono::steady_clock::now();
    const auto result = OHOS::MMI::InputCli::Run({ "touch-swipe", "--displayId", "0", "--srcX", "100", "--srcY", "500",
        "--dstX", "900", "--dstY", "500", "--duration", "500" });
    const auto elapsed = std::chrono::steady_clock::now() - start;
    ExpectTouchSuccess(result, "touch-swipe");
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 480);
    const auto data = ParseJson(result)["data"];
    EXPECT_EQ(data["displayId"], 0);
    EXPECT_EQ(data["srcX"], 100);
    EXPECT_EQ(data["srcY"], 500);
    EXPECT_EQ(data["dstX"], 900);
    EXPECT_EQ(data["dstY"], 500);
    EXPECT_EQ(data["duration"], 500);
    ASSERT_GE(calls.size(), 4U);
    EXPECT_EQ(calls.front().name, "CreateTouchController");
    EXPECT_EQ(calls[1].name, "TouchDown");
    EXPECT_EQ(calls[1].values, (std::vector<int32_t> { 0, 0, 100, 500 }));
    EXPECT_EQ(calls.back().name, "TouchUp");
    EXPECT_EQ(calls.back().values, (std::vector<int32_t> { 0, 0, 900, 500 }));
    const auto moveCount = std::count_if(calls.begin(), calls.end(), [](const RecordedCall &call) {
        return call.name == "TouchMove";
    });
    EXPECT_GE(moveCount, 2);
    int32_t previousX = 100;
    for (size_t index = 2; index + 1 < calls.size(); ++index) {
        ASSERT_EQ(calls[index].name, "TouchMove");
        ASSERT_EQ(calls[index].values.size(), 4U);
        EXPECT_EQ(calls[index].values[0], 0);
        EXPECT_EQ(calls[index].values[1], 0);
        EXPECT_GT(calls[index].values[2], previousX);
        EXPECT_LT(calls[index].values[2], 900);
        EXPECT_EQ(calls[index].values[3], 500);
        previousX = calls[index].values[2];
    }
}

HWTEST_F(InputTouchCommandTest, TouchCommands_MapPermissionFailureToExitThree, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    MockControllerOptions options;
    options.touchCreateResult = ERROR_NO_PERMISSION;
    fixture.Configure(calls, options);
    const auto tap = OHOS::MMI::InputCli::Run({ "touch-tap", "--x", "1", "--y", "2" });
    EXPECT_EQ(tap.code, PERMISSION_EXIT);
    ExpectTouchError(tap, "ERR_PERMISSION_DENIED");
    ExpectCalls(calls, { { "CreateTouchController", {} } });
    EXPECT_EQ(fixture.TouchCreateCount(), 1);

    calls.clear();
    ResetMockControllerState();
    fixture.Configure(calls, options);
    const auto swipe = OHOS::MMI::InputCli::Run({ "touch-swipe", "--srcX", "1", "--srcY", "2", "--dstX", "3", "--dstY", "4" });
    EXPECT_EQ(swipe.code, PERMISSION_EXIT);
    ExpectTouchError(swipe, "ERR_PERMISSION_DENIED");
    ExpectCalls(calls, { { "CreateTouchController", {} } });
}

HWTEST_F(InputTouchCommandTest, TouchSwipe_MoveFailureAttemptsUpAtLastSuccessfulPoint, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    MockControllerOptions options;
    options.touchFailureCall = "TouchMove";
    options.touchFailureResult = -1;
    fixture.Configure(calls, options);
    const auto result = OHOS::MMI::InputCli::Run({ "touch-swipe", "--srcX", "10", "--srcY", "20", "--dstX", "100", "--dstY", "200",
        "--duration", "1000" });
    EXPECT_EQ(result.code, SERVICE_EXIT);
    ExpectTouchError(result, "ERR_INPUT_SERVICE_EXCEPTION");
    ASSERT_EQ(calls.size(), 4U);
    EXPECT_EQ(calls[0].name, "CreateTouchController");
    EXPECT_EQ(calls[1].name, "TouchDown");
    EXPECT_EQ(calls[1].values, (std::vector<int32_t> { 0, 0, 10, 20 }));
    EXPECT_EQ(calls[2].name, "TouchMove");
    EXPECT_EQ(calls[3].name, "TouchUp");
    EXPECT_EQ(calls[3].values, (std::vector<int32_t> { 0, 0, 10, 20 }));
}

HWTEST_F(InputTouchCommandTest, TouchDownAndUpFailuresMapToServiceException, TestSize.Level1)
{
    MockControllerFixture fixture;
    std::vector<RecordedCall> calls;
    MockControllerOptions options;
    options.touchFailureCall = "TouchDown";
    options.touchFailureResult = -1;
    fixture.Configure(calls, options);
    const auto down = OHOS::MMI::InputCli::Run({ "touch-tap", "--x", "4", "--y", "5", "--duration", "0" });
    EXPECT_EQ(down.code, SERVICE_EXIT);
    ExpectTouchError(down, "ERR_INPUT_SERVICE_EXCEPTION");
    ExpectCalls(calls, { { "CreateTouchController", {} }, { "TouchDown", { 0, 0, 4, 5 } } });

    calls.clear();
    options.touchFailureCall = "TouchUp";
    fixture.Configure(calls, options);
    const auto up = OHOS::MMI::InputCli::Run({ "touch-tap", "--x", "4", "--y", "5", "--duration", "0" });
    EXPECT_EQ(up.code, SERVICE_EXIT);
    ExpectTouchError(up, "ERR_INPUT_SERVICE_EXCEPTION");
    ExpectCalls(calls, { { "CreateTouchController", {} }, { "TouchDown", { 0, 0, 4, 5 } },
        { "TouchUp", { 0, 0, 4, 5 } } });
}
