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
    EXPECT_NE(redirect.Content().find("invalid coordinate, x and y must be non-negative"), std::string::npos);
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
    states[0].push_back(FingerState { 1, 0, 500, 800, true });
    states[0].push_back(FingerState { 2, 0, 300, 400, true });
    states[1].push_back(FingerState { 3, 1, 100, 200, true });
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
    EXPECT_EQ(loaded[0][1].fingerId, 2);
    EXPECT_EQ(loaded[0][1].x, 300);
    EXPECT_EQ(loaded[0][1].y, 400);
    ASSERT_EQ(loaded[1].size(), 1u);
    EXPECT_EQ(loaded[1][0].fingerId, 3);
    EXPECT_EQ(loaded[1][0].screenId, 1);
    EXPECT_EQ(loaded[1][0].x, 100);
    EXPECT_EQ(loaded[1][0].y, 200);
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
    PointerEvent::PointerItem item;
    ASSERT_TRUE(event->GetPointerItem(1, item));
    EXPECT_EQ(item.GetDisplayX(), 500);
    EXPECT_EQ(item.GetDisplayY(), 800);
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
    PointerEvent::PointerItem secondItem;
    ASSERT_TRUE(event->GetPointerItem(2, secondItem));
    EXPECT_EQ(secondItem.GetDisplayX(), 300);
    EXPECT_EQ(secondItem.GetDisplayY(), 400);
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
