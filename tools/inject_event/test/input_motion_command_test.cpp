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

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

#include "input_motion_command.h"
#include "mock.h"
#include "pointer_event.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;

constexpr char STATE_FILE_PATH[] = "/data/local/tmp/uinput_motion_state";

void ResetMock()
{
    MOCKHANDLER->mockSimulateInputEventRet = RET_OK;
    MOCKHANDLER->mockSimulateInputEventFailFromCount = -1;
    MOCKHANDLER->simulateInputEventCallCount = 0;
    MOCKHANDLER->lastSimulatedPointerEvent = nullptr;
    MOCKHANDLER->simulatedPointerEvents.clear();
}

FingerStateMap LoadStateForVerify()
{
    FingerStateMap states;
    InputMotionCommand command;
    if (command.AcquireLock() == 0) {
        command.LoadState(states);
        command.ReleaseLock();
    }
    return states;
}

class StderrRedirect {
public:
    StderrRedirect()
    {
        savedFd_ = dup(STDERR_FILENO);
        std::string tmpl = "/data/local/tmp/motion_stderr_captureXXXXXX";
        std::vector<char> path(tmpl.begin(), tmpl.end());
        path.push_back('\0');
        tmpFd_ = mkstemp(path.data());
        if (tmpFd_ >= 0) {
            capturePath_ = path.data();
            fflush(stderr);
            dup2(tmpFd_, STDERR_FILENO);
        }
    }

    ~StderrRedirect()
    {
        Restore();
        if (tmpFd_ >= 0) {
            close(tmpFd_);
            unlink(capturePath_.c_str());
        }
    }

    std::string Content()
    {
        if (tmpFd_ < 0) {
            return "";
        }
        fflush(stderr);
        std::string content;
        lseek(tmpFd_, 0, SEEK_SET);
        char buf[256];
        ssize_t n = 0;
        while ((n = read(tmpFd_, buf, sizeof(buf))) > 0) {
            content.append(buf, static_cast<size_t>(n));
        }
        return content;
    }

    void Restore()
    {
        fflush(stderr);
        if (savedFd_ >= 0) {
            dup2(savedFd_, STDERR_FILENO);
            close(savedFd_);
            savedFd_ = -1;
        }
    }

private:
    int savedFd_ { -1 };
    int tmpFd_ { -1 };
    std::string capturePath_;
};
} // namespace

class InputMotionCommandTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: InvalidParameter_FingerIdOutOfRange
 * @tc.desc: Test motion_down with fingerId out of 0-9 range
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, InvalidParameter_FingerIdOutOfRange, TestSize.Level1)
{
    const char* injectArgvs[] = { "uinput", "motion_down", "500", "800", "-f", "10", "-s", "0" };
    char** argv = const_cast<char**>(injectArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(8, argv), 1);
    EXPECT_NE(redirect.Content().find("invalid fingerId, range is 0-9"), std::string::npos);
}

/**
 * @tc.name: InvalidParameter_NegativeCoordinate
 * @tc.desc: Test motion_down with negative x coordinate
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, InvalidParameter_NegativeCoordinate, TestSize.Level1)
{
    const char* injectArgvs[] = { "uinput", "motion_down", "-1", "800" };
    char** argv = const_cast<char**>(injectArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(4, argv), 1);
    EXPECT_NE(redirect.Content().find("Coordinate value must be greater or equal than 0"), std::string::npos);
}

/**
 * @tc.name: InvalidParameter_MissingParameter
 * @tc.desc: Test motion_down with missing x/y parameters
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, InvalidParameter_MissingParameter, TestSize.Level1)
{
    const char* injectArgvs[] = { "uinput", "motion_down" };
    char** argv = const_cast<char**>(injectArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(2, argv), 1);
    EXPECT_NE(redirect.Content().find("too few arguments to function"), std::string::npos);
    EXPECT_NE(redirect.Content().find("usage"), std::string::npos);
}

/**
 * @tc.name: ConcurrentAccess_FlockMutex
 * @tc.desc: Test flock mutex between two state file accesses
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, ConcurrentAccess_FlockMutex, TestSize.Level1)
{
    InputMotionCommand first;
    InputMotionCommand second;
    EXPECT_EQ(first.AcquireLock(), 0);
    {
        StderrRedirect redirect;
        EXPECT_EQ(second.AcquireLock(), 1);
        EXPECT_NE(redirect.Content().find("state file is locked by another process"), std::string::npos);
    }
    first.ReleaseLock();
    EXPECT_EQ(second.AcquireLock(), 0);
    second.ReleaseLock();
}

/**
 * @tc.name: StateFile_ForwardCompatibility
 * @tc.desc: Test LoadState tolerates appended fields in future format
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, StateFile_ForwardCompatibility, TestSize.Level1)
{
    const std::string stateFilePath = "/data/local/tmp/uinput_motion_state";
    unlink(stateFilePath.c_str());
    FILE* file = fopen(stateFilePath.c_str(), "w");
    ASSERT_NE(file, nullptr);
    fprintf(file, "1 0 500 800 1 99\n2 0 300 400 1\n3 0 100 100 1 55 888\n");
    fclose(file);
    InputMotionCommand command;
    EXPECT_EQ(command.AcquireLock(), 0);
    FingerStateMap loaded;
    EXPECT_EQ(command.LoadState(loaded), 0);
    command.ReleaseLock();
    ASSERT_EQ(loaded.count(0), 1u);
    ASSERT_EQ(loaded[0].size(), 3u);
    EXPECT_EQ(loaded[0][0].fingerId, 1);
    EXPECT_EQ(loaded[0][0].x, 500);
    EXPECT_EQ(loaded[0][0].downTime, 99);
    EXPECT_EQ(loaded[0][1].fingerId, 2);
    EXPECT_EQ(loaded[0][1].downTime, 0);
    EXPECT_EQ(loaded[0][2].fingerId, 3);
    EXPECT_EQ(loaded[0][2].downTime, 55);
    unlink(stateFilePath.c_str());
}

/**
 * @tc.name: StateFile_InvalidLinesFiltered
 * @tc.desc: Test LoadState filters tampered or legacy invalid lines
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, StateFile_InvalidLinesFiltered, TestSize.Level1)
{
    const std::string stateFilePath = "/data/local/tmp/uinput_motion_state";
    unlink(stateFilePath.c_str());
    FILE* file = fopen(stateFilePath.c_str(), "w");
    ASSERT_NE(file, nullptr);
    fprintf(file, "1 0 500 800 1\n100 0 500 800 1\n2 0 -5 800 1\n3 0 300 -1 1\n4 0 100 200 0\n");
    fclose(file);
    InputMotionCommand command;
    EXPECT_EQ(command.AcquireLock(), 0);
    FingerStateMap loaded;
    EXPECT_EQ(command.LoadState(loaded), 0);
    command.ReleaseLock();
    ASSERT_EQ(loaded.count(0), 1u);
    ASSERT_EQ(loaded[0].size(), 1u);
    EXPECT_EQ(loaded[0][0].fingerId, 1);
    EXPECT_EQ(loaded[0][0].x, 500);
    EXPECT_EQ(loaded[0][0].y, 800);
    unlink(stateFilePath.c_str());
}

/**
 * @tc.name: MotionDown_DuplicatePress_Rejected
 * @tc.desc: Test repeated motion_down on same finger is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MotionDown_DuplicatePress_Rejected, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* downArgvs[] = { "uinput", "motion_down", "500", "800", "-f", "1" };
    char** argv = const_cast<char**>(downArgvs);
    EXPECT_EQ(InputMotionCommand::Handle(6, argv), 0);
    {
        StderrRedirect redirect;
        EXPECT_EQ(InputMotionCommand::Handle(6, argv), 1);
        EXPECT_NE(redirect.Content().find("finger 1 is already pressed"), std::string::npos);
    }
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 1);
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: InvalidParameter_NonNumericOptionValue
 * @tc.desc: Test motion_down with non-numeric option value is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, InvalidParameter_NonNumericOptionValue, TestSize.Level1)
{
    const char* injectArgvs[] = { "uinput", "motion_down", "500", "800", "-f", "abc" };
    char** argv = const_cast<char**>(injectArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(6, argv), 1);
    EXPECT_NE(redirect.Content().find("invalid number for option -f: abc"), std::string::npos);
}

/**
 * @tc.name: InvalidParameter_TooManyPositionals
 * @tc.desc: Test motion_down with too many positional parameters is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, InvalidParameter_TooManyPositionals, TestSize.Level1)
{
    const char* injectArgvs[] = { "uinput", "motion_down", "500", "800", "900" };
    char** argv = const_cast<char**>(injectArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(5, argv), 1);
    EXPECT_NE(redirect.Content().find("too many positional parameters: 900"), std::string::npos);
}

/**
 * @tc.name: InvalidParameter_NegativeScreenId
 * @tc.desc: Test motion_down with negative screenId is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, InvalidParameter_NegativeScreenId, TestSize.Level1)
{
    const char* injectArgvs[] = { "uinput", "motion_down", "500", "800", "-s", "-1" };
    char** argv = const_cast<char**>(injectArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(6, argv), 1);
    EXPECT_NE(redirect.Content().find("invalid displayId"), std::string::npos);
}

/**
 * @tc.name: Handle_NullArgv
 * @tc.desc: Test Handle with null argv returns failure
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, Handle_NullArgv, TestSize.Level1)
{
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(2, nullptr), 1);
    EXPECT_NE(redirect.Content().find("usage"), std::string::npos);
}

/**
 * @tc.name: Handle_AgcTooSmall
 * @tc.desc: Test Handle with argc below minimum returns failure
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, Handle_AgcTooSmall, TestSize.Level1)
{
    const char* injectArgvs[] = { "uinput" };
    char** argv = const_cast<char**>(injectArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(1, argv), 1);
    EXPECT_NE(redirect.Content().find("usage"), std::string::npos);
}

/**
 * @tc.name: ParseOptions_MissingOptionValue
 * @tc.desc: Test motion_down with -f but no value is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, ParseOptions_MissingOptionValue, TestSize.Level1)
{
    const char* injectArgvs[] = { "uinput", "motion_down", "100", "100", "-f" };
    char** argv = const_cast<char**>(injectArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(5, argv), 1);
    EXPECT_NE(redirect.Content().find("missing value for option: -f"), std::string::npos);
}

/**
 * @tc.name: ParseOptions_ScreenOnly_NoCoordinates
 * @tc.desc: Test motion_down with only -s option and no x/y is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, ParseOptions_ScreenOnly_NoCoordinates, TestSize.Level1)
{
    const char* injectArgvs[] = { "uinput", "motion_down", "-s", "0" };
    char** argv = const_cast<char**>(injectArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(4, argv), 1);
    EXPECT_NE(redirect.Content().find("too few arguments"), std::string::npos);
}

/**
 * @tc.name: LoadState_WithoutLock
 * @tc.desc: Test LoadState fails when lock not acquired
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, LoadState_WithoutLock, TestSize.Level1)
{
    InputMotionCommand command;
    FingerStateMap states;
    StderrRedirect redirect;
    EXPECT_EQ(command.LoadState(states), 1);
    EXPECT_NE(redirect.Content().find("not locked"), std::string::npos);
}

/**
 * @tc.name: SaveState_WithoutLock
 * @tc.desc: Test SaveState fails when lock not acquired
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, SaveState_WithoutLock, TestSize.Level1)
{
    InputMotionCommand command;
    FingerStateMap states;
    StderrRedirect redirect;
    EXPECT_EQ(command.SaveState(states), 1);
    EXPECT_NE(redirect.Content().find("not locked"), std::string::npos);
}

/**
 * @tc.name: AcquireLock_DoubleAcquire
 * @tc.desc: Test AcquireLock fails when already held
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, AcquireLock_DoubleAcquire, TestSize.Level1)
{
    InputMotionCommand command;
    EXPECT_EQ(command.AcquireLock(), 0);
    {
        StderrRedirect redirect;
        EXPECT_EQ(command.AcquireLock(), 1);
        EXPECT_NE(redirect.Content().find("already held"), std::string::npos);
    }
    command.ReleaseLock();
}

/**
 * @tc.name: BoundaryFingerId_MinAndMax
 * @tc.desc: Test fingerId boundary values 0 and 9 are accepted
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, BoundaryFingerId_MinAndMax, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* argv0[] = { "uinput", "motion_down", "100", "100", "-f", "0" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(argv0)), 0);
    const char* argv9[] = { "uinput", "motion_down", "200", "200", "-f", "9" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(argv9)), 0);
    FingerStateMap states = LoadStateForVerify();
    ASSERT_EQ(states.count(0), 1u);
    EXPECT_EQ(states[0].size(), 2u);
    EXPECT_EQ(states[0][0].fingerId, 0);
    EXPECT_EQ(states[0][1].fingerId, 9);
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: MultiScreen_StateIsolation
 * @tc.desc: Test finger states are isolated per screen
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MultiScreen_StateIsolation, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* downS0[] = { "uinput", "motion_down", "100", "100", "-f", "0", "-s", "0" };
    EXPECT_EQ(InputMotionCommand::Handle(8, const_cast<char**>(downS0)), 0);
    const char* downS1[] = { "uinput", "motion_down", "200", "200", "-f", "1", "-s", "1" };
    EXPECT_EQ(InputMotionCommand::Handle(8, const_cast<char**>(downS1)), 0);

    FingerStateMap states = LoadStateForVerify();
    ASSERT_EQ(states.size(), 2u);
    ASSERT_EQ(states[0].size(), 1u);
    EXPECT_EQ(states[0][0].fingerId, 0);
    ASSERT_EQ(states[1].size(), 1u);
    EXPECT_EQ(states[1][0].fingerId, 1);

    // finger0 move on screen0 should not affect finger1 on screen1
    const char* moveS0[] = { "uinput", "motion_move", "150", "150", "-f", "0", "-s", "0" };
    EXPECT_EQ(InputMotionCommand::Handle(8, const_cast<char**>(moveS0)), 0);
    states = LoadStateForVerify();
    EXPECT_EQ(states[0][0].x, 150);
    EXPECT_EQ(states[1][0].x, 200);

    // finger1 up on screen1 should not affect finger0 on screen0
    const char* upS1[] = { "uinput", "motion_up", "200", "200", "-f", "1", "-s", "1" };
    EXPECT_EQ(InputMotionCommand::Handle(8, const_cast<char**>(upS1)), 0);
    states = LoadStateForVerify();
    EXPECT_EQ(states.count(1), 0u);
    ASSERT_EQ(states[0].size(), 1u);
    EXPECT_EQ(states[0][0].fingerId, 0);
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: MotionReset_MultiScreen
 * @tc.desc: Test motion_reset handles fingers across multiple screens
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MotionReset_MultiScreen, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* downS0f0[] = { "uinput", "motion_down", "100", "100", "-f", "0", "-s", "0" };
    EXPECT_EQ(InputMotionCommand::Handle(8, const_cast<char**>(downS0f0)), 0);
    const char* downS0f1[] = { "uinput", "motion_down", "200", "200", "-f", "1", "-s", "0" };
    EXPECT_EQ(InputMotionCommand::Handle(8, const_cast<char**>(downS0f1)), 0);
    const char* downS1f2[] = { "uinput", "motion_down", "300", "300", "-f", "2", "-s", "1" };
    EXPECT_EQ(InputMotionCommand::Handle(8, const_cast<char**>(downS1f2)), 0);

    const char* resetArgvs[] = { "uinput", "motion_reset" };
    EXPECT_EQ(InputMotionCommand::Handle(2, const_cast<char**>(resetArgvs)), 0);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 6); // 3 down + 3 up

    FingerStateMap states = LoadStateForVerify();
    EXPECT_TRUE(states.empty());
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: StateFile_EmptyFile
 * @tc.desc: Test LoadState with an empty (0-byte) state file returns empty map
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, StateFile_EmptyFile, TestSize.Level1)
{
    const std::string stateFilePath = "/data/local/tmp/uinput_motion_state";
    unlink(stateFilePath.c_str());
    FILE* file = fopen(stateFilePath.c_str(), "w");
    ASSERT_NE(file, nullptr);
    fclose(file);
    InputMotionCommand command;
    EXPECT_EQ(command.AcquireLock(), 0);
    FingerStateMap loaded;
    EXPECT_EQ(command.LoadState(loaded), 0);
    command.ReleaseLock();
    EXPECT_TRUE(loaded.empty());
    unlink(stateFilePath.c_str());
}

/**
 * @tc.name: HandleDown_AfterCorruptedReset
 * @tc.desc: Test motion_down works after reset cleared a corrupted file
 * @tc.type: FUNC
 * @tc.require: RR-3009058
 */
HWTEST_F(InputMotionCommandTest, HandleDown_AfterCorruptedReset, TestSize.Level1)
{
    const std::string stateFilePath = "/data/local/tmp/uinput_motion_state";
    unlink(stateFilePath.c_str());
    FILE* file = fopen(stateFilePath.c_str(), "w");
    ASSERT_NE(file, nullptr);
    fprintf(file, "garbage data here\n");
    fclose(file);
    ResetMock();
    const char* resetArgvs[] = { "uinput", "motion_reset" };
    EXPECT_EQ(InputMotionCommand::Handle(2, const_cast<char**>(resetArgvs)), 0);
    const char* downArgvs[] = { "uinput", "motion_down", "100", "200", "-f", "3" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs)), 0);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 1);
    FingerStateMap states = LoadStateForVerify();
    ASSERT_EQ(states.count(0), 1u);
    ASSERT_EQ(states[0].size(), 1u);
    EXPECT_EQ(states[0][0].fingerId, 3);
    unlink(stateFilePath.c_str());
}

/**
 * @tc.name: MotionMove_OrderViolation
 * @tc.desc: Test motion_move without prior motion_down is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MotionMove_OrderViolation, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* moveArgvs[] = { "uinput", "motion_move", "100", "100", "-f", "2" };
    char** argv = const_cast<char**>(moveArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(6, argv), 1);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 0);
    EXPECT_NE(redirect.Content().find("finger 2 is not pressed, cannot move"), std::string::npos);
}

/**
 * @tc.name: InjectFailed_DownRolledBack
 * @tc.desc: Test motion_down rolls back state when injection is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, InjectFailed_DownRolledBack, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    MOCKHANDLER->mockSimulateInputEventFailFromCount = 1;
    const char* downArgvs[] = { "uinput", "motion_down", "500", "800", "-f", "1" };
    char** argv = const_cast<char**>(downArgvs);
    {
        StderrRedirect redirect;
        EXPECT_EQ(InputMotionCommand::Handle(6, argv), 1);
        EXPECT_NE(redirect.Content().find("inject event failed"), std::string::npos);
    }
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 1);
    FingerStateMap states = LoadStateForVerify();
    EXPECT_TRUE(states.empty());
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: InjectFailed_MoveKeepsOldCoords
 * @tc.desc: Test motion_move rolls back coordinates when injection is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, InjectFailed_MoveKeepsOldCoords, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* downArgvs[] = { "uinput", "motion_down", "500", "800", "-f", "1" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs)), 0);
    MOCKHANDLER->mockSimulateInputEventFailFromCount = 2;
    const char* moveArgvs[] = { "uinput", "motion_move", "600", "800", "-f", "1" };
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(moveArgvs)), 1);
    EXPECT_NE(redirect.Content().find("inject event failed"), std::string::npos);
    FingerStateMap states = LoadStateForVerify();
    ASSERT_EQ(states.count(0), 1u);
    ASSERT_EQ(states[0].size(), 1u);
    EXPECT_EQ(states[0][0].fingerId, 1);
    EXPECT_EQ(states[0][0].x, 500);
    EXPECT_EQ(states[0][0].y, 800);
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: InjectFailed_UpKeepsFinger
 * @tc.desc: Test motion_up keeps finger state when injection is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, InjectFailed_UpKeepsFinger, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* downArgvs[] = { "uinput", "motion_down", "500", "800", "-f", "1" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs)), 0);
    MOCKHANDLER->mockSimulateInputEventFailFromCount = 2;
    const char* upArgvs[] = { "uinput", "motion_up", "600", "800", "-f", "1" };
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(upArgvs)), 1);
    EXPECT_NE(redirect.Content().find("inject event failed"), std::string::npos);
    FingerStateMap states = LoadStateForVerify();
    ASSERT_EQ(states.count(0), 1u);
    ASSERT_EQ(states[0].size(), 1u);
    EXPECT_EQ(states[0][0].fingerId, 1);
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: Reset_PartialFailure_PersistsRemaining
 * @tc.desc: Test motion_reset persists remaining fingers on partial failure
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, Reset_PartialFailure_PersistsRemaining, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* downArgvs1[] = { "uinput", "motion_down", "100", "100", "-f", "0" };
    const char* downArgvs2[] = { "uinput", "motion_down", "200", "200", "-f", "1" };
    const char* downArgvs3[] = { "uinput", "motion_down", "300", "300", "-f", "2" };
    const char* resetArgvs[] = { "uinput", "motion_reset" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs1)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs2)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs3)), 0);
    MOCKHANDLER->mockSimulateInputEventFailFromCount = 5;
    EXPECT_EQ(InputMotionCommand::Handle(2, const_cast<char**>(resetArgvs)), 1);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 5);
    FingerStateMap states = LoadStateForVerify();
    ASSERT_EQ(states.count(0), 1u);
    ASSERT_EQ(states[0].size(), 2u);
    EXPECT_EQ(states[0][0].fingerId, 1);
    EXPECT_EQ(states[0][1].fingerId, 2);
    MOCKHANDLER->mockSimulateInputEventFailFromCount = -1;
    EXPECT_EQ(InputMotionCommand::Handle(2, const_cast<char**>(resetArgvs)), 0);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 7);
    FingerStateMap cleared = LoadStateForVerify();
    EXPECT_TRUE(cleared.empty());
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: StateFile_ReadWrite
 * @tc.desc: Test FingerState serialization to and from state file
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, StateFile_ReadWrite, TestSize.Level1)
{
    const std::string stateFilePath = "/data/local/tmp/uinput_motion_state";
    unlink(stateFilePath.c_str());
    InputMotionCommand command;
    FingerStateMap states;
    states[0].push_back(FingerState { 1, 0, 500, 800, true, 111 });
    states[0].push_back(FingerState { 2, 0, 300, 400, true, 222 });
    states[1].push_back(FingerState { 3, 1, 100, 200, true, 333 });
    EXPECT_EQ(command.AcquireLock(), 0);
    EXPECT_EQ(command.SaveState(states), 0);
    FingerStateMap loaded;
    EXPECT_EQ(command.LoadState(loaded), 0);
    command.ReleaseLock();
    ASSERT_EQ(loaded.size(), 2u);
    ASSERT_EQ(loaded[0].size(), 2u);
    EXPECT_EQ(loaded[0][0].fingerId, 1);
    EXPECT_EQ(loaded[0][0].screenId, 0);
    EXPECT_EQ(loaded[0][0].x, 500);
    EXPECT_EQ(loaded[0][0].y, 800);
    EXPECT_TRUE(loaded[0][0].isDown);
    EXPECT_EQ(loaded[0][0].downTime, 111);
    EXPECT_EQ(loaded[0][1].fingerId, 2);
    EXPECT_EQ(loaded[0][1].x, 300);
    EXPECT_EQ(loaded[0][1].y, 400);
    EXPECT_EQ(loaded[0][1].downTime, 222);
    ASSERT_EQ(loaded[1].size(), 1u);
    EXPECT_EQ(loaded[1][0].fingerId, 3);
    EXPECT_EQ(loaded[1][0].screenId, 1);
    EXPECT_EQ(loaded[1][0].x, 100);
    EXPECT_EQ(loaded[1][0].y, 200);
    EXPECT_EQ(loaded[1][0].downTime, 333);
    unlink(stateFilePath.c_str());
}

/**
 * @tc.name: MotionDown_BasicInput
 * @tc.desc: Test motion_down injects DOWN event and persists finger state
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MotionDown_BasicInput, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* injectArgvs[] = { "uinput", "motion_down", "500", "800", "-f", "1", "-s", "0" };
    char** argv = const_cast<char**>(injectArgvs);
    EXPECT_EQ(InputMotionCommand::Handle(8, argv), 0);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 1);
    std::shared_ptr<PointerEvent> event = MOCKHANDLER->lastSimulatedPointerEvent;
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->GetPointerAction(), PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_EQ(event->GetPointerId(), 1);
    EXPECT_EQ(event->GetSourceType(), PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    EXPECT_EQ(event->GetTargetDisplayId(), 0);
    EXPECT_TRUE(event->HasFlag(InputEvent::EVENT_FLAG_RAW_POINTER_ID));
    PointerEvent::PointerItem item;
    ASSERT_TRUE(event->GetPointerItem(1, item));
    EXPECT_EQ(item.GetDisplayX(), 500);
    EXPECT_EQ(item.GetDisplayY(), 800);
    EXPECT_TRUE(item.IsPressed());
    EXPECT_GT(item.GetDownTime(), 0);
    EXPECT_EQ(item.GetOriginPointerId(), 1);
    EXPECT_EQ(item.GetToolType(), PointerEvent::TOOL_TYPE_FINGER);
    EXPECT_GT(event->GetActionTime(), static_cast<int64_t>(0));
    EXPECT_GT(event->GetActionStartTime(), static_cast<int64_t>(0));
    FingerStateMap states = LoadStateForVerify();
    ASSERT_EQ(states.count(0), 1u);
    ASSERT_EQ(states[0].size(), 1u);
    EXPECT_EQ(states[0][0].fingerId, 1);
    EXPECT_EQ(states[0][0].x, 500);
    EXPECT_EQ(states[0][0].y, 800);
    EXPECT_TRUE(states[0][0].isDown);
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: MotionDown_SecondFingerCarriesFirst
 * @tc.desc: Test second motion_down event includes the first pressed finger
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MotionDown_SecondFingerCarriesFirst, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* down1[] = { "uinput", "motion_down", "100", "200", "-f", "0" };
    const char* down2[] = { "uinput", "motion_down", "300", "400", "-f", "1" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(down1)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(down2)), 0);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 2);

    // first down event: only finger0
    std::shared_ptr<PointerEvent> firstEvent = MOCKHANDLER->simulatedPointerEvents[0];
    ASSERT_NE(firstEvent, nullptr);
    EXPECT_EQ(firstEvent->GetPointerAction(), PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_EQ(firstEvent->GetPointerId(), 0);
    EXPECT_EQ(firstEvent->GetPointerCount(), 1);

    // second down event: must contain finger0 (history) and finger1 (current)
    std::shared_ptr<PointerEvent> secondEvent = MOCKHANDLER->simulatedPointerEvents[1];
    ASSERT_NE(secondEvent, nullptr);
    EXPECT_EQ(secondEvent->GetPointerAction(), PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_EQ(secondEvent->GetPointerId(), 1);
    EXPECT_EQ(secondEvent->GetPointerCount(), 2);
    PointerEvent::PointerItem item0;
    ASSERT_TRUE(secondEvent->GetPointerItem(0, item0));
    EXPECT_EQ(item0.GetDisplayX(), 100);
    EXPECT_EQ(item0.GetDisplayY(), 200);
    EXPECT_TRUE(item0.IsPressed());
    EXPECT_EQ(item0.GetDeviceId(), -2);
    PointerEvent::PointerItem item1;
    ASSERT_TRUE(secondEvent->GetPointerItem(1, item1));
    EXPECT_EQ(item1.GetDisplayX(), 300);
    EXPECT_EQ(item1.GetDisplayY(), 400);
    EXPECT_TRUE(item1.IsPressed());
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: MotionUp_SecondFingerLeavesFirst
 * @tc.desc: Test motion_up for finger1 keeps finger0 in the event
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MotionUp_SecondFingerLeavesFirst, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* down0[] = { "uinput", "motion_down", "100", "200", "-f", "0" };
    const char* down1[] = { "uinput", "motion_down", "300", "400", "-f", "1" };
    const char* up1[] = { "uinput", "motion_up", "300", "400", "-f", "1" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(down0)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(down1)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(up1)), 0);

    // third event (up finger1): finger0 still pressed, finger1 lifted
    std::shared_ptr<PointerEvent> upEvent = MOCKHANDLER->simulatedPointerEvents[2];
    ASSERT_NE(upEvent, nullptr);
    EXPECT_EQ(upEvent->GetPointerAction(), PointerEvent::POINTER_ACTION_UP);
    EXPECT_EQ(upEvent->GetPointerId(), 1);
    EXPECT_EQ(upEvent->GetPointerCount(), 2);
    PointerEvent::PointerItem item0;
    ASSERT_TRUE(upEvent->GetPointerItem(0, item0));
    EXPECT_EQ(item0.GetDisplayX(), 100);
    EXPECT_EQ(item0.GetDisplayY(), 200);
    EXPECT_TRUE(item0.IsPressed());
    PointerEvent::PointerItem item1;
    ASSERT_TRUE(upEvent->GetPointerItem(1, item1));
    EXPECT_EQ(item1.GetDisplayX(), 300);
    EXPECT_EQ(item1.GetDisplayY(), 400);
    EXPECT_FALSE(item1.IsPressed());
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: MotionMove_CarriesHistoryState
 * @tc.desc: Test motion_move event carries all pressed fingers
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MotionMove_CarriesHistoryState, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* downArgvs1[] = { "uinput", "motion_down", "500", "800", "-f", "1" };
    const char* downArgvs2[] = { "uinput", "motion_down", "300", "400", "-f", "2" };
    const char* moveArgvs[] = { "uinput", "motion_move", "600", "800", "-f", "1" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs1)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs2)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(moveArgvs)), 0);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 3);
    std::shared_ptr<PointerEvent> event = MOCKHANDLER->lastSimulatedPointerEvent;
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->GetPointerAction(), PointerEvent::POINTER_ACTION_MOVE);
    EXPECT_EQ(event->GetPointerId(), 1);
    PointerEvent::PointerItem firstItem;
    ASSERT_TRUE(event->GetPointerItem(1, firstItem));
    EXPECT_EQ(firstItem.GetDisplayX(), 600);
    EXPECT_EQ(firstItem.GetDisplayY(), 800);
    EXPECT_TRUE(firstItem.IsPressed());
    PointerEvent::PointerItem secondItem;
    ASSERT_TRUE(event->GetPointerItem(2, secondItem));
    EXPECT_EQ(secondItem.GetDisplayX(), 300);
    EXPECT_EQ(secondItem.GetDisplayY(), 400);
    EXPECT_TRUE(secondItem.IsPressed());
    EXPECT_GT(secondItem.GetDownTime(), 0);
    FingerStateMap states = LoadStateForVerify();
    ASSERT_EQ(states.count(0), 1u);
    ASSERT_EQ(states[0].size(), 2u);
    EXPECT_EQ(states[0][0].fingerId, 1);
    EXPECT_EQ(states[0][0].x, 600);
    EXPECT_EQ(states[0][0].y, 800);
    EXPECT_EQ(states[0][1].fingerId, 2);
    EXPECT_EQ(states[0][1].x, 300);
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: MotionUp_StateCleared
 * @tc.desc: Test motion_up injects UP event and removes finger state
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MotionUp_StateCleared, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* downArgvs[] = { "uinput", "motion_down", "500", "800", "-f", "1" };
    const char* moveArgvs[] = { "uinput", "motion_move", "600", "800", "-f", "1" };
    const char* upArgvs[] = { "uinput", "motion_up", "600", "800", "-f", "1" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(moveArgvs)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(upArgvs)), 0);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 3);
    std::shared_ptr<PointerEvent> event = MOCKHANDLER->lastSimulatedPointerEvent;
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->GetPointerAction(), PointerEvent::POINTER_ACTION_UP);
    EXPECT_EQ(event->GetPointerId(), 1);
    PointerEvent::PointerItem item;
    ASSERT_TRUE(event->GetPointerItem(1, item));
    EXPECT_EQ(item.GetDisplayX(), 600);
    EXPECT_EQ(item.GetDisplayY(), 800);
    EXPECT_FALSE(item.IsPressed());
    EXPECT_GT(item.GetDownTime(), static_cast<int64_t>(0));
    FingerStateMap states = LoadStateForVerify();
    EXPECT_TRUE(states.empty());
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: MotionUp_OrderViolation_NoPressedFinger
 * @tc.desc: Test motion_up without prior motion_down is rejected
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MotionUp_OrderViolation_NoPressedFinger, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* upArgvs[] = { "uinput", "motion_up", "500", "800", "-f", "1" };
    char** argv = const_cast<char**>(upArgvs);
    StderrRedirect redirect;
    EXPECT_EQ(InputMotionCommand::Handle(6, argv), 1);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 0);
    EXPECT_NE(redirect.Content().find("finger 1 is not pressed, cannot lift"), std::string::npos);
    unlink(STATE_FILE_PATH);
}

/**
 * @tc.name: Reset_CorruptedStateFile_SelfHeals
 * @tc.desc: Test motion_reset self-heals corrupted state file by clearing it
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, Reset_CorruptedStateFile_SelfHeals, TestSize.Level1)
{
    const std::string stateFilePath = "/data/local/tmp/uinput_motion_state";
    unlink(stateFilePath.c_str());
    FILE* file = fopen(stateFilePath.c_str(), "w");
    ASSERT_NE(file, nullptr);
    fprintf(file, "1 0 50\n");
    fclose(file);
    ResetMock();
    const char* resetArgvs[] = { "uinput", "motion_reset" };
    char** argv = const_cast<char**>(resetArgvs);
    {
        StderrRedirect redirect;
        EXPECT_EQ(InputMotionCommand::Handle(2, argv), 0);
        EXPECT_NE(redirect.Content().find(", clearing"), std::string::npos);
    }
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 0);
    FingerStateMap states = LoadStateForVerify();
    EXPECT_TRUE(states.empty());
    unlink(stateFilePath.c_str());
    unlink("/data/local/tmp/uinput_motion_state.tmp");
}

/**
 * @tc.name: MotionReset_AllFingersLifted
 * @tc.desc: Test motion_reset lifts all pressed fingers and clears state
 * @tc.type: FUNC
 * @tc.require: RR-30090858
 */
HWTEST_F(InputMotionCommandTest, MotionReset_AllFingersLifted, TestSize.Level1)
{
    unlink(STATE_FILE_PATH);
    ResetMock();
    const char* downArgvs1[] = { "uinput", "motion_down", "100", "100", "-f", "0" };
    const char* downArgvs2[] = { "uinput", "motion_down", "200", "200", "-f", "1" };
    const char* downArgvs3[] = { "uinput", "motion_down", "300", "300", "-f", "2" };
    const char* resetArgvs[] = { "uinput", "motion_reset" };
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs1)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs2)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(6, const_cast<char**>(downArgvs3)), 0);
    EXPECT_EQ(InputMotionCommand::Handle(2, const_cast<char**>(resetArgvs)), 0);
    EXPECT_EQ(MOCKHANDLER->simulateInputEventCallCount, 6);
    const auto &events = MOCKHANDLER->simulatedPointerEvents;
    ASSERT_EQ(events.size(), 6u);
    for (int32_t i = 0; i < 3; ++i) {
        std::shared_ptr<PointerEvent> event = events[3 + static_cast<size_t>(i)];
        ASSERT_NE(event, nullptr);
        EXPECT_EQ(event->GetPointerAction(), PointerEvent::POINTER_ACTION_UP);
        EXPECT_EQ(event->GetPointerId(), i);
    }
    PointerEvent::PointerItem item;
    ASSERT_TRUE(events[3]->GetPointerItem(0, item));
    EXPECT_EQ(item.GetDisplayX(), 100);
    EXPECT_EQ(item.GetDisplayY(), 100);
    ASSERT_TRUE(events[4]->GetPointerItem(1, item));
    EXPECT_EQ(item.GetDisplayX(), 200);
    EXPECT_EQ(item.GetDisplayY(), 200);
    ASSERT_TRUE(events[5]->GetPointerItem(2, item));
    EXPECT_EQ(item.GetDisplayX(), 300);
    EXPECT_EQ(item.GetDisplayY(), 300);
    FingerStateMap states = LoadStateForVerify();
    EXPECT_TRUE(states.empty());
    unlink(STATE_FILE_PATH);
}
} // namespace MMI
} // namespace OHOS
