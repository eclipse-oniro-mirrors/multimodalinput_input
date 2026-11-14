/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <atomic>

#include "key_event_hook.h"
#include "input_event_hook.h"
#include "event_dispatch_handler.h"
#include "define_multimodal.h"
#include "error_multimodal.h"
#include "input_event_handler.h"
#include "uds_server.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "KeyEventHookTest"

namespace OHOS {
namespace MMI {
namespace {
const std::string PROGRAM_NAME = "uds_server_test";
constexpr int32_t MODULE_TYPE = 1;
constexpr int32_t UDS_FD = -1;
constexpr int32_t UDS_UID = 456;
constexpr int32_t UDS_PID = 123;
using namespace testing::ext;

std::shared_ptr<KeyEvent> MakeKeyEvent(int32_t eventId, int32_t keyCode = KeyEvent::KEYCODE_A)
{
    auto keyEvent = KeyEvent::Create();
    if (keyEvent != nullptr) {
        keyEvent->SetId(eventId);
        keyEvent->SetKeyCode(keyCode);
        keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    }
    return keyEvent;
}
} // namespace

class KeyEventHookTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// Probe hook counting intercepted calls, used for chain forwarding/replay observation
class SpyHook : public InputEventHook {
public:
    SpyHook(SessionPtr session, NextHookGetter nextHookGetter)
        : InputEventHook(session, HOOK_EVENT_TYPE_KEY, nextHookGetter) {}
    bool OnKeyEvent(std::shared_ptr<KeyEvent> keyEvent) override
    {
        ++keyCallCount_;
        return true;
    }
    std::atomic<int32_t> keyCallCount_ { 0 };
};

/**
 * @tc.name: KeyEventHookTest001
 * @tc.desc: OnKeyEvent with nullptr event
 * @tc.type: FUNC
 */
HWTEST_F(KeyEventHookTest, KeyEventHookTest001, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    KeyEventHook hook(sess, [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> {
        return nullptr;
    });
    EXPECT_FALSE(hook.OnKeyEvent(nullptr));
}

/**
 * @tc.name: KeyEventHookTest002
 * @tc.desc: OnKeyEvent send failure without valid session
 * @tc.type: FUNC
 */
HWTEST_F(KeyEventHookTest, KeyEventHookTest002, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    KeyEventHook hook(sess, [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> {
        return nullptr;
    });
    auto keyEvent = MakeKeyEvent(1);
    ASSERT_NE(keyEvent, nullptr);
    EXPECT_FALSE(hook.OnKeyEvent(keyEvent));
}

/**
 * @tc.name: KeyEventHookTest003
 * @tc.desc: DispatchToNextHandler with nullptr event
 * @tc.type: FUNC
 */
HWTEST_F(KeyEventHookTest, KeyEventHookTest003, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    KeyEventHook hook(sess, [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> {
        return nullptr;
    });
    EXPECT_EQ(hook.DispatchToNextHandler(nullptr), RET_ERR);
}

/**
 * @tc.name: KeyEventHookTest004
 * @tc.desc: DispatchToNextHandler walks the chain: successor accepts
 * @tc.type: FUNC
 */
HWTEST_F(KeyEventHookTest, KeyEventHookTest004, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto acceptSpy = std::make_shared<SpyHook>(sess,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto hook = std::make_shared<KeyEventHook>(sess,
        [acceptSpy] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return acceptSpy; });
    auto keyEvent = MakeKeyEvent(1);
    ASSERT_NE(keyEvent, nullptr);
    EXPECT_EQ(hook->DispatchToNextHandler(keyEvent), RET_OK);
    EXPECT_EQ(acceptSpy->keyCallCount_, 1);
}

/**
 * @tc.name: KeyEventHookTest005
 * @tc.desc: DispatchToNextHandler falls back to direct dispatch after whole chain declines
 * @tc.type: FUNC
 */
HWTEST_F(KeyEventHookTest, KeyEventHookTest005, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto hook = std::make_shared<KeyEventHook>(sess,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto keyEvent = MakeKeyEvent(1);
    ASSERT_NE(keyEvent, nullptr);
    // Chain-tail direct dispatch: no UDS server in the test environment so it fails with RET_ERR
    // (see the SweepExpiredEvents test for the nothing-dropped semantics)
    EXPECT_EQ(hook->DispatchToNextHandler(keyEvent), RET_ERR);
}

/**
 * @tc.name: KeyEventHookTest006
 * @tc.desc: HandleEventResult consumed removes event from stash
 * @tc.type: FUNC
 */
HWTEST_F(KeyEventHookTest, KeyEventHookTest006, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto hook = std::make_shared<KeyEventHook>(sess,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto keyEvent = MakeKeyEvent(1);
    ASSERT_NE(keyEvent, nullptr);
    hook->expirationChecker_.UpdateInputEvent(keyEvent);
    EXPECT_NE(hook->GetStashEvent(1), nullptr);
    EXPECT_EQ(hook->HandleEventResult(1, true), RET_OK); // Consume: do not dispatch further
    EXPECT_EQ(hook->GetStashEvent(1), nullptr);
}

/**
 * @tc.name: KeyEventHookTest007
 * @tc.desc: HandleEventResult released forwards the event
 * @tc.type: FUNC
 */
HWTEST_F(KeyEventHookTest, KeyEventHookTest007, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto acceptSpy = std::make_shared<SpyHook>(sess,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto hook = std::make_shared<KeyEventHook>(sess,
        [acceptSpy] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return acceptSpy; });
    auto keyEvent = MakeKeyEvent(1);
    ASSERT_NE(keyEvent, nullptr);
    hook->expirationChecker_.UpdateInputEvent(keyEvent);
    EXPECT_EQ(hook->HandleEventResult(1, false), RET_OK); // Release: dispatch further
    EXPECT_EQ(acceptSpy->keyCallCount_, 1);
    EXPECT_EQ(hook->GetStashEvent(1), nullptr);
}

/**
 * @tc.name: KeyEventHookTest008
 * @tc.desc: Late receipt for swept event is treated as handled
 * @tc.type: FUNC
 */
HWTEST_F(KeyEventHookTest, KeyEventHookTest008, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto hook = std::make_shared<KeyEventHook>(sess,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    // Not in the stash (already dispatched by the timeout fallback): late receipt treated as handled
    EXPECT_EQ(hook->HandleEventResult(999, true), RET_ERR);
    EXPECT_EQ(hook->HandleEventResult(999, false), RET_ERR);
}

/**
 * @tc.name: KeyEventHookTest009
 * @tc.desc: SweepExpiredEvents dispatches stale events and clears stash
 * @tc.type: FUNC
 */
HWTEST_F(KeyEventHookTest, KeyEventHookTest009, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto hook = std::make_shared<KeyEventHook>(sess,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto staleEvent = MakeKeyEvent(1);
    ASSERT_NE(staleEvent, nullptr);
    int64_t past = std::chrono::duration_cast<std::chrono::milliseconds>(
        (std::chrono::steady_clock::now() - std::chrono::seconds(5)).time_since_epoch()).count();
    hook->expirationChecker_.UpdateInputEvent(staleEvent, past);
    auto freshEvent = MakeKeyEvent(2);
    ASSERT_NE(freshEvent, nullptr);
    hook->expirationChecker_.UpdateInputEvent(freshEvent);

    hook->SweepExpiredEvents();
    // Expired events are dispatched by the fallback (removed from the stash); fresh events are kept
    EXPECT_EQ(hook->GetStashEvent(1), nullptr);
    EXPECT_NE(hook->GetStashEvent(2), nullptr);
}

/**
 * @tc.name: KeyEventHookTest010
 * @tc.desc: FlushStashEvents forwards to next hook and drains stash
 * @tc.type: FUNC
 */
HWTEST_F(KeyEventHookTest, KeyEventHookTest010, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto spy = std::make_shared<SpyHook>(sess,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto hook = std::make_shared<KeyEventHook>(sess,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto keyEvent1 = MakeKeyEvent(1);
    ASSERT_NE(keyEvent1, nullptr);
    auto keyEvent2 = MakeKeyEvent(2);
    ASSERT_NE(keyEvent2, nullptr);
    hook->expirationChecker_.UpdateInputEvent(keyEvent1);
    hook->expirationChecker_.UpdateInputEvent(keyEvent2);

    hook->FlushStashEvents(spy);
    EXPECT_EQ(spy->keyCallCount_, 2);
    EXPECT_EQ(hook->GetStashEvent(1), nullptr);
    EXPECT_EQ(hook->GetStashEvent(2), nullptr);
}
} // namespace MMI
} // namespace OHOS
