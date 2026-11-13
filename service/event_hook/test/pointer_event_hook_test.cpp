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
#include <cstdio>
#include <atomic>

#include "pointer_event_hook.h"
#include "input_event_hook.h"
#include "event_dispatch_handler.h"
#include "define_multimodal.h"
#include "error_multimodal.h"
#include "input_event_handler.h"
#include "uds_server.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "PointerEventHookTest"

namespace OHOS {
namespace MMI {
namespace {
const std::string PROGRAM_NAME = "uds_server_test";
constexpr int32_t MODULE_TYPE = 1;
constexpr int32_t UDS_FD = -1;
constexpr int32_t UDS_UID = 456;
constexpr int32_t UDS_PID = 123;
using namespace testing::ext;

std::shared_ptr<PointerEvent> MakeMouseEvent(int32_t eventId)
{
    auto pointerEvent = PointerEvent::Create();
    if (pointerEvent != nullptr) {
        pointerEvent->SetId(eventId);
        pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
        pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
        pointerEvent->SetPointerId(0);
        pointerEvent->SetButtonId(PointerEvent::BUTTON_NONE);
        PointerEvent::PointerItem item;
        item.SetDisplayX(100);
        item.SetDisplayY(100);
        pointerEvent->AddPointerItem(item);
    }
    return pointerEvent;
}
} // namespace

class PointerEventHookTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// Probe hook counting intercepted calls, used for chain forwarding/replay observation
class SpyHook : public InputEventHook {
public:
    SpyHook(SessionPtr session, HookEventType hookType, NextHookGetter nextHookGetter)
        : InputEventHook(session, hookType, nextHookGetter) {}
    bool OnPointerEvent(std::shared_ptr<PointerEvent> pointerEvent) override
    {
        ++pointerCallCount_;
        return true;
    }
    std::atomic<int32_t> pointerCallCount_ { 0 };
};

/**
 * @tc.name: PointerEventHookTest001
 * @tc.desc: OnPointerEvent with nullptr event
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest001, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    PointerEventHook hook(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    EXPECT_FALSE(hook.OnPointerEvent(nullptr));
}

/**
 * @tc.name: PointerEventHookTest002
 * @tc.desc: OnPointerEvent rejects event whose sourceType does not match hook type
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest002, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    PointerEventHook hook(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto touchEvent = MakeMouseEvent(1);
    ASSERT_NE(touchEvent, nullptr);
    touchEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    EXPECT_FALSE(hook.OnPointerEvent(touchEvent));
}

/**
 * @tc.name: PointerEventHookTest003
 * @tc.desc: OnPointerEvent send failure without valid session
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest003, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    PointerEventHook hook(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto mouseEvent = MakeMouseEvent(1);
    ASSERT_NE(mouseEvent, nullptr);
    EXPECT_FALSE(hook.OnPointerEvent(mouseEvent));
}

/**
 * @tc.name: PointerEventHookTest004
 * @tc.desc: DispatchToNextHandler with nullptr event
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest004, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    PointerEventHook hook(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    EXPECT_EQ(hook.DispatchToNextHandler(nullptr), RET_ERR);
}

/**
 * @tc.name: PointerEventHookTest005
 * @tc.desc: DispatchToNextHandler walks the chain: successor accepts
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest005, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto acceptSpy = std::make_shared<SpyHook>(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto hook = std::make_shared<PointerEventHook>(sess, HOOK_EVENT_TYPE_MOUSE,
        [acceptSpy] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return acceptSpy; });
    auto mouseEvent = MakeMouseEvent(1);
    ASSERT_NE(mouseEvent, nullptr);
    EXPECT_EQ(hook->DispatchToNextHandler(mouseEvent), RET_OK);
    EXPECT_EQ(acceptSpy->pointerCallCount_, 1);
}

/**
 * @tc.name: PointerEventHookTest006
 * @tc.desc: DispatchToNextHandler falls back to direct dispatch after whole chain declines
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest006, TestSize.Level0)
{
    InputHandler->BuildInputHandlerChain();
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto hook = std::make_shared<PointerEventHook>(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto mouseEvent = MakeMouseEvent(1);
    ASSERT_NE(mouseEvent, nullptr);
    // Chain-tail direct dispatch: the direct-dispatch handler is void-semantics, mouse events always report success
    EXPECT_EQ(hook->DispatchToNextHandler(mouseEvent), RET_OK);
}

/**
 * @tc.name: PointerEventHookTest007
 * @tc.desc: HandleEventResult consumed removes event from stash
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest007, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto hook = std::make_shared<PointerEventHook>(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto mouseEvent = MakeMouseEvent(1);
    ASSERT_NE(mouseEvent, nullptr);
    hook->expirationChecker_.UpdateInputEvent(mouseEvent);
    EXPECT_NE(hook->GetStashEvent(1), nullptr);
    EXPECT_EQ(hook->HandleEventResult(1, true), RET_OK); // Consume: do not dispatch further
    EXPECT_EQ(hook->GetStashEvent(1), nullptr);
}

/**
 * @tc.name: PointerEventHookTest008
 * @tc.desc: HandleEventResult released forwards the event
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest008, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto acceptSpy = std::make_shared<SpyHook>(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto hook = std::make_shared<PointerEventHook>(sess, HOOK_EVENT_TYPE_MOUSE,
        [acceptSpy] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return acceptSpy; });
    auto mouseEvent = MakeMouseEvent(1);
    ASSERT_NE(mouseEvent, nullptr);
    hook->expirationChecker_.UpdateInputEvent(mouseEvent);
    EXPECT_EQ(hook->HandleEventResult(1, false), RET_OK); // Release: dispatch further
    EXPECT_EQ(acceptSpy->pointerCallCount_, 1);
    EXPECT_EQ(hook->GetStashEvent(1), nullptr);
}

/**
 * @tc.name: PointerEventHookTest009
 * @tc.desc: Late receipt for swept event is treated as handled
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest009, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto hook = std::make_shared<PointerEventHook>(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    EXPECT_EQ(hook->HandleEventResult(999, true), RET_ERR);
    EXPECT_EQ(hook->HandleEventResult(999, false), RET_ERR);
}

/**
 * @tc.name: PointerEventHookTest010
 * @tc.desc: SweepExpiredEvents dispatches stale events and clears stash
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest010, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto hook = std::make_shared<PointerEventHook>(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto staleEvent = MakeMouseEvent(1);
    ASSERT_NE(staleEvent, nullptr);
    int64_t past = std::chrono::duration_cast<std::chrono::milliseconds>(
        (std::chrono::steady_clock::now() - std::chrono::seconds(5)).time_since_epoch()).count();
    hook->expirationChecker_.UpdateInputEvent(staleEvent, past);
    auto freshEvent = MakeMouseEvent(2);
    ASSERT_NE(freshEvent, nullptr);
    hook->expirationChecker_.UpdateInputEvent(freshEvent);

    hook->SweepExpiredEvents();
    EXPECT_EQ(hook->GetStashEvent(1), nullptr);
    EXPECT_NE(hook->GetStashEvent(2), nullptr);
}

/**
 * @tc.name: PointerEventHookTest011
 * @tc.desc: FlushStashEvents forwards to next hook and drains stash
 * @tc.type: FUNC
 */
HWTEST_F(PointerEventHookTest, PointerEventHookTest011, TestSize.Level0)
{
    SessionPtr sess = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    auto spy = std::make_shared<SpyHook>(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto hook = std::make_shared<PointerEventHook>(sess, HOOK_EVENT_TYPE_MOUSE,
        [] (std::shared_ptr<InputEventHook>) -> std::shared_ptr<InputEventHook> { return nullptr; });
    auto mouseEvent1 = MakeMouseEvent(1);
    ASSERT_NE(mouseEvent1, nullptr);
    auto mouseEvent2 = MakeMouseEvent(2);
    ASSERT_NE(mouseEvent2, nullptr);
    hook->expirationChecker_.UpdateInputEvent(mouseEvent1);
    hook->expirationChecker_.UpdateInputEvent(mouseEvent2);

    hook->FlushStashEvents(spy);
    EXPECT_EQ(spy->pointerCallCount_, 2);
    EXPECT_EQ(hook->GetStashEvent(1), nullptr);
    EXPECT_EQ(hook->GetStashEvent(2), nullptr);
}
} // namespace MMI
} // namespace OHOS
