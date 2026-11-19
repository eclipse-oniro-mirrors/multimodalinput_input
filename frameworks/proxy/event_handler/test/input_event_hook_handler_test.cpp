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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>

#include "error_multimodal.h"
#include "i_input_event_consumer.h"
#include "input_event_hook_handler.h"
#include "input_manager.h"
#include "mmi_log.h"
#include "multimodal_event_handler.h"
#include "multimodal_input_connect_manager.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "InputEventHookHandlerTest"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
using namespace testing;
} // namespace

bool MultimodalEventHandler::InitClient(EventHandlerPtr eventHandler)
{
    return true;
}

MultimodalEventHandler::MultimodalEventHandler() {}
MultimodalEventHandler::~MultimodalEventHandler() {}

InputManager* InputManager::GetInstance()
{
    static InputManager instance;
    return &instance;
}

void InputManager::RequestInjectionCallback(int32_t reqId, int32_t status) {}

std::shared_ptr<MultimodalInputConnectManager> g_instance = nullptr;

// 统计「代为放行」回执次数（isConsumed=false 的 NotifyHookEventResult 调用）
int32_t g_notifyPassCount = 0;

std::shared_ptr<MultimodalInputConnectManager> MultimodalInputConnectManager::GetInstance()
{
    if (g_instance == nullptr) {
        g_instance = std::make_shared<MultimodalInputConnectManager>();
    }
    return g_instance;
}

int32_t MultimodalInputConnectManager::AddInputEventHook(HookEventType hookEventType)
{
    if (hookEventType > HOOK_EVENT_TYPE_KEY + HOOK_EVENT_TYPE_MOUSE + HOOK_EVENT_TYPE_TOUCH) {
        return RET_ERR;
    }
    return RET_OK;
}

int32_t MultimodalInputConnectManager::RemoveInputEventHook(HookEventType hookEventType)
{
    if (hookEventType > HOOK_EVENT_TYPE_KEY + HOOK_EVENT_TYPE_MOUSE + HOOK_EVENT_TYPE_TOUCH) {
        return RET_ERR;
    }
    return RET_OK;
}

int32_t MultimodalInputConnectManager::NotifyHookEventResult(int32_t eventId, HookEventType hookEventType,
    bool isConsumed)
{
    if (hookEventType > HOOK_EVENT_TYPE_KEY + HOOK_EVENT_TYPE_MOUSE + HOOK_EVENT_TYPE_TOUCH) {
        return RET_ERR;
    }
    if (!isConsumed) {
        ++g_notifyPassCount;
    }
    return RET_OK;
}

class HookConsumer : public IInputEventConsumer {
public:
    void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const { g_flag = 1; }
    void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const { g_flag = 2; }
    void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const { g_flag = 4; }
public:
    static int32_t g_flag;
};

int32_t HookConsumer::g_flag = 0;

// 统计 OnHookDisabled 通知次数，验证客户端/服务端超时去重后只通知一次
class DisableCountConsumer : public IInputEventConsumer {
public:
    void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const {}
    void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const {}
    void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const {}
    void OnHookDisabled(HookEventType hookEventType) const override { ++g_disableCount; }
    static int32_t g_disableCount;
};

int32_t DisableCountConsumer::g_disableCount = 0;

// 记录超时禁用通知类型的 consumer（验证客户端 1S 超时按 handler 全部类型整体禁用）
class TimeoutDisableConsumer : public IInputEventConsumer {
public:
    void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const {}
    void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const {}
    void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const {}
    void OnHookDisabled(HookEventType hookEventType) const override
    {
        ++g_disableCount;
        g_lastDisabledType = hookEventType;
    }
    static int32_t g_disableCount;
    static HookEventType g_lastDisabledType;
};

int32_t TimeoutDisableConsumer::g_disableCount = 0;
HookEventType TimeoutDisableConsumer::g_lastDisabledType = HOOK_EVENT_TYPE_NONE;

// 回调内重入 RemoveInputEventHook（模拟业务在事件回调内 Enable(false)/Destroy）：
// 修复前 OnKeyEvent/OnPointerEvent 持 rwMutex_ shared_lock 调用回调，重入的
// RemoveInputEventHookLocal unique_lock 同线程死锁；修复后锁外执行回调，可正常完成
class ReentrantRemoveConsumer : public IInputEventConsumer {
public:
    void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
    {
        ++g_callbackCount;
        g_removeResult = INPUT_EVENT_HOOK_HANDLER.RemoveInputEventHook(g_removeType);
    }
    void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
    {
        ++g_callbackCount;
        g_removeResult = INPUT_EVENT_HOOK_HANDLER.RemoveInputEventHook(g_removeType);
    }
    void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const {}
public:
    static int32_t g_callbackCount;
    static int32_t g_removeResult;
    static HookEventType g_removeType;
};

int32_t ReentrantRemoveConsumer::g_callbackCount = 0;
int32_t ReentrantRemoveConsumer::g_removeResult = 0;
HookEventType ReentrantRemoveConsumer::g_removeType = HOOK_EVENT_TYPE_NONE;

class InputEventHookHandlerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
};

void ClearData()
{
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    INPUT_EVENT_HOOK_HANDLER.consumer_ = nullptr;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = nullptr;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = nullptr;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = nullptr;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.axisHookCallback_ = nullptr;
}

/**
 * @tc.name: InputEventHookHandlerTest_AddInputEventHookLocal_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_AddInputEventHookLocal_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    HookEventType hookEventType = 7;
    auto ret = INPUT_EVENT_HOOK_HANDLER.AddInputEventHookLocal(consumer, hookEventType);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_AddInputEventHookLocal_002
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_AddInputEventHookLocal_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    HookEventType hookEventType = 0;
    auto ret = INPUT_EVENT_HOOK_HANDLER.AddInputEventHookLocal(consumer, hookEventType);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_AddInputEventHook_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_AddInputEventHook_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    HookEventType hookEventType = 1;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 1;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.AddInputEventHook(consumer, hookEventType);
    EXPECT_EQ(ret, ERROR_REPEAT_HANDLER);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_AddInputEventHook_002
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_AddInputEventHook_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    HookEventType hookEventType = 8;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.AddInputEventHook(consumer, hookEventType);
    EXPECT_EQ(ret, RET_ERR);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_AddInputEventHook_003
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_AddInputEventHook_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    HookEventType hookEventType = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.AddInputEventHook(consumer, hookEventType);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_AddInputEventHook_004
 * @tc.desc: Test AddInputEventHook rejects a second hook type in the same process
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_AddInputEventHook_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    HookEventType hookEventType = HOOK_EVENT_TYPE_MOUSE;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = HOOK_EVENT_TYPE_KEY;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.AddInputEventHook(consumer, hookEventType);
    EXPECT_EQ(ret, ERROR_REPEAT_HANDLER);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_RemoveInputEventHookLocal_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_RemoveInputEventHookLocal_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    HookEventType hookEventType = 7;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 7;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.RemoveInputEventHookLocal(hookEventType);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_RemoveInputEventHookLocal_002
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_RemoveInputEventHookLocal_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    HookEventType hookEventType = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.RemoveInputEventHookLocal(hookEventType);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_RemoveInputEventHook_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_RemoveInputEventHook_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    HookEventType hookEventType = 8;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.RemoveInputEventHook(hookEventType);
    EXPECT_EQ(ret, RET_ERR);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_RemoveInputEventHook_002
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_RemoveInputEventHook_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    HookEventType hookEventType = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.RemoveInputEventHook(hookEventType);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_DispatchToNextHandler_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_DispatchToNextHandler_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    int32_t eventId = 0;
    HookEventType hookEventType = 1;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 1;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.NotifyHookEventResult(eventId, hookEventType, false);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_DispatchToNextHandler_002
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_DispatchToNextHandler_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    int32_t eventId = 0;
    HookEventType hookEventType = 4;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 4;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.NotifyHookEventResult(eventId, hookEventType, false);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_DispatchToNextHandler_003
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_DispatchToNextHandler_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    int32_t eventId = 0;
    HookEventType hookEventType = 2;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 2;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.NotifyHookEventResult(eventId, hookEventType, false);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_DispatchToNextHandler_004
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_DispatchToNextHandler_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    int32_t eventId = 0;
    HookEventType hookEventType = 8;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto ret = INPUT_EVENT_HOOK_HANDLER.NotifyHookEventResult(eventId, hookEventType, false);
    EXPECT_EQ(ret, RET_ERR);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnPointerEvent_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnPointerEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 2;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.OnPointerEvent(pointerEvent);
    EXPECT_EQ(consumer->g_flag, 2);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnPointerEvent_002
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnPointerEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 4;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.OnPointerEvent(pointerEvent);
    EXPECT_EQ(consumer->g_flag, 2);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnPointerEvent_003
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnPointerEvent_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHPAD);
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.OnPointerEvent(pointerEvent);
    EXPECT_NE(consumer->g_flag, 2);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_AddKeyHook_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_AddKeyHook_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    INPUT_EVENT_HOOK_HANDLER.OnConnected();
    std::function<void(std::shared_ptr<KeyEvent>)> keyHook = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.AddKeyHook(keyHook);
    INPUT_EVENT_HOOK_HANDLER.OnConnected();
    EXPECT_EQ(INPUT_EVENT_HOOK_HANDLER.currentHookStats_, 1);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_AddMouseHook_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_AddMouseHook_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    std::function<void(std::shared_ptr<PointerEvent>)> mouseHook = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.AddMouseHook(mouseHook);
    INPUT_EVENT_HOOK_HANDLER.OnConnected();
    EXPECT_EQ(INPUT_EVENT_HOOK_HANDLER.currentHookStats_, 4);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_AddTouchHook_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_AddTouchHook_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    std::function<void(std::shared_ptr<PointerEvent>)> touchHook = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.AddTouchHook(touchHook);
    INPUT_EVENT_HOOK_HANDLER.OnConnected();
    EXPECT_EQ(INPUT_EVENT_HOOK_HANDLER.currentHookStats_, 2);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_AddInputEventHookToServer_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_AddInputEventHookToServer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    HookEventType hookEventType = 0;
    auto ret = INPUT_EVENT_HOOK_HANDLER.AddInputEventHookToServer(hookEventType);
    EXPECT_EQ(ret, RET_OK);
    ClearData();

    hookEventType = 8;
    ret = INPUT_EVENT_HOOK_HANDLER.AddInputEventHookToServer(hookEventType);
    EXPECT_EQ(ret, RET_ERR);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_RemoveInputEventHookOfServer_001
 * @tc.desc: Test the function NotifyDevCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_RemoveInputEventHookOfServer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    HookEventType hookEventType = 0;
    auto ret = INPUT_EVENT_HOOK_HANDLER.RemoveInputEventHookOfServer(hookEventType);
    EXPECT_EQ(ret, RET_OK);
    ClearData();

    hookEventType = 8;
    ret = INPUT_EVENT_HOOK_HANDLER.RemoveInputEventHookOfServer(hookEventType);
    EXPECT_EQ(ret, RET_ERR);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnKeyEvent_001
 * @tc.desc: Test OnKeyEvent with valid keyEvent and keyHookCallback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnKeyEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 1;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.OnKeyEvent(keyEvent);
    EXPECT_EQ(consumer->g_flag, 1);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnKeyEvent_002
 * @tc.desc: Test OnKeyEvent with nullptr keyEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnKeyEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 1;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.OnKeyEvent(nullptr);
    EXPECT_EQ(consumer->g_flag, 0);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnKeyEvent_003
 * @tc.desc: Test OnKeyEvent when keyHookCallback is not set (nullptr callback branch)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnKeyEvent_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 0;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = nullptr;
    INPUT_EVENT_HOOK_HANDLER.OnKeyEvent(keyEvent);
    SUCCEED();
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnKeyEventNullCallbackPass_001
 * @tc.desc: Cleared-callback race: proxy pass-through replies immediately (no 1.5S delay)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnKeyEventNullCallbackPass_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    g_notifyPassCount = 0;
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetId(1);
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = HOOK_EVENT_TYPE_KEY;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = nullptr;
    INPUT_EVENT_HOOK_HANDLER.OnKeyEvent(keyEvent);
    // 代为放行：立即回执 isConsumed=false，业务无感知
    EXPECT_EQ(g_notifyPassCount, 1);
    EXPECT_EQ(consumer->g_flag, 0);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_DispatchToNextHandler_006
 * @tc.desc: Test DispatchToNextHandler with valid eventId for KEY hook
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_DispatchToNextHandler_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t eventId = 1;
    HookEventType hookEventType = 1;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 1;
    auto ret = INPUT_EVENT_HOOK_HANDLER.NotifyHookEventResult(eventId, hookEventType, false);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_DispatchToNextHandler_007
 * @tc.desc: Test DispatchToNextHandler with valid eventId for MOUSE hook
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_DispatchToNextHandler_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t eventId = 1;
    HookEventType hookEventType = 4;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 4;
    auto ret = INPUT_EVENT_HOOK_HANDLER.NotifyHookEventResult(eventId, hookEventType, false);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_DispatchToNextHandler_008
 * @tc.desc: Test DispatchToNextHandler with valid eventId for TOUCH hook
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_DispatchToNextHandler_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t eventId = 1;
    HookEventType hookEventType = 2;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 2;
    auto ret = INPUT_EVENT_HOOK_HANDLER.NotifyHookEventResult(eventId, hookEventType, false);
    EXPECT_EQ(ret, RET_OK);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_IsHookExisted_001
 * @tc.desc: Test IsHookExisted with only TOUCH hook active
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_IsHookExisted_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 2;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    EXPECT_TRUE(INPUT_EVENT_HOOK_HANDLER.IsHookExisted(HOOK_EVENT_TYPE_TOUCH));
    EXPECT_FALSE(INPUT_EVENT_HOOK_HANDLER.IsHookExisted(HOOK_EVENT_TYPE_KEY));
    EXPECT_FALSE(INPUT_EVENT_HOOK_HANDLER.IsHookExisted(HOOK_EVENT_TYPE_MOUSE));
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_IsHookExisted_002
 * @tc.desc: Test IsHookExisted with only MOUSE hook active
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_IsHookExisted_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<HookConsumer>();
    consumer->g_flag = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = 4;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    EXPECT_TRUE(INPUT_EVENT_HOOK_HANDLER.IsHookExisted(HOOK_EVENT_TYPE_MOUSE));
    EXPECT_FALSE(INPUT_EVENT_HOOK_HANDLER.IsHookExisted(HOOK_EVENT_TYPE_KEY));
    EXPECT_FALSE(INPUT_EVENT_HOOK_HANDLER.IsHookExisted(HOOK_EVENT_TYPE_TOUCH));
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_DisableHookLocal_001
 * @tc.desc: DisableHookLocal hits once then misses: dedup adjudication point
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_DisableHookLocal_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<DisableCountConsumer>();
    DisableCountConsumer::g_disableCount = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = HOOK_EVENT_TYPE_KEY;
    INPUT_EVENT_HOOK_HANDLER.consumer_ = consumer;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    // 首次命中：清位+清回调，返回 true
    EXPECT_TRUE(INPUT_EVENT_HOOK_HANDLER.DisableHookLocal(HOOK_EVENT_TYPE_KEY));
    EXPECT_EQ(INPUT_EVENT_HOOK_HANDLER.currentHookStats_ & HOOK_EVENT_TYPE_KEY, 0);
    // 二次未命中：已被禁用，返回 false（去重裁决点）
    EXPECT_FALSE(INPUT_EVENT_HOOK_HANDLER.DisableHookLocal(HOOK_EVENT_TYPE_KEY));
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnHookDisabledByServer_001
 * @tc.desc: Server disable notifies once; late client timeout does not re-notify
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnHookDisabledByServer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<DisableCountConsumer>();
    DisableCountConsumer::g_disableCount = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = HOOK_EVENT_TYPE_MOUSE;
    INPUT_EVENT_HOOK_HANDLER.consumer_ = consumer;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    // 服务端 1.5S 超时通知：命中 → 通知一次
    INPUT_EVENT_HOOK_HANDLER.OnHookDisabledByServer(HOOK_EVENT_TYPE_MOUSE);
    EXPECT_EQ(DisableCountConsumer::g_disableCount, 1);
    EXPECT_EQ(INPUT_EVENT_HOOK_HANDLER.currentHookStats_ & HOOK_EVENT_TYPE_MOUSE, 0);
    // 客户端 1S 迟到路径（模拟回调返回后检查）：位已清，不再通知
    EXPECT_FALSE(INPUT_EVENT_HOOK_HANDLER.DisableHookLocal(HOOK_EVENT_TYPE_MOUSE));
    EXPECT_EQ(DisableCountConsumer::g_disableCount, 1);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnHookDisabledByServer_002
 * @tc.desc: Multi-type mask clears all bits and notifies once (GAP-12 whole-process disable)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnHookDisabledByServer_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<DisableCountConsumer>();
    DisableCountConsumer::g_disableCount = 0;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = HOOK_EVENT_TYPE_KEY | HOOK_EVENT_TYPE_MOUSE;
    INPUT_EVENT_HOOK_HANDLER.consumer_ = consumer;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.mouseHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    // 服务端按进程整体禁用：单包携带全部类型位，一次清除、只通知一次
    INPUT_EVENT_HOOK_HANDLER.OnHookDisabledByServer(HOOK_EVENT_TYPE_KEY | HOOK_EVENT_TYPE_MOUSE);
    EXPECT_EQ(DisableCountConsumer::g_disableCount, 1);
    EXPECT_EQ(INPUT_EVENT_HOOK_HANDLER.currentHookStats_, 0u);
    EXPECT_FALSE(INPUT_EVENT_HOOK_HANDLER.IsHookExisted(HOOK_EVENT_TYPE_KEY));
    EXPECT_FALSE(INPUT_EVENT_HOOK_HANDLER.IsHookExisted(HOOK_EVENT_TYPE_MOUSE));
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnConnectedFailFallback_001
 * @tc.desc: Reconnect re-add failure degrades to disable + single notify
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnConnectedFailFallback_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<DisableCountConsumer>();
    DisableCountConsumer::g_disableCount = 0;
    // AXIS(8) 走 mock AddInputEventHook 的 RET_ERR 分支，模拟重连后 re-add 失败
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = HOOK_EVENT_TYPE_AXIS;
    INPUT_EVENT_HOOK_HANDLER.consumer_ = consumer;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.axisHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    INPUT_EVENT_HOOK_HANDLER.OnConnected();
    EXPECT_EQ(DisableCountConsumer::g_disableCount, 1);
    EXPECT_EQ(INPUT_EVENT_HOOK_HANDLER.currentHookStats_ & HOOK_EVENT_TYPE_AXIS, 0);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnKeyEventReentrantRemove_001
 * @tc.desc: OnKeyEvent callback reentering RemoveInputEventHook must not deadlock and must disable the hook
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnKeyEventReentrantRemove_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<ReentrantRemoveConsumer>();
    ReentrantRemoveConsumer::g_callbackCount = 0;
    ReentrantRemoveConsumer::g_removeResult = 0;
    ReentrantRemoveConsumer::g_removeType = HOOK_EVENT_TYPE_KEY;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = HOOK_EVENT_TYPE_KEY;
    INPUT_EVENT_HOOK_HANDLER.consumer_ = consumer;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.keyHookCallback_ = [consumer](std::shared_ptr<KeyEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    INPUT_EVENT_HOOK_HANDLER.OnKeyEvent(keyEvent);
    EXPECT_EQ(ReentrantRemoveConsumer::g_callbackCount, 1);
    EXPECT_EQ(ReentrantRemoveConsumer::g_removeResult, RET_OK);
    EXPECT_EQ(INPUT_EVENT_HOOK_HANDLER.currentHookStats_ & HOOK_EVENT_TYPE_KEY, 0u);
    ClearData();
}

/**
 * @tc.name: InputEventHookHandlerTest_OnPointerEventReentrantRemove_001
 * @tc.desc: OnPointerEvent touch callback reentering RemoveInputEventHook must not deadlock
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventHookHandlerTest, InputEventHookHandlerTest_OnPointerEventReentrantRemove_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto consumer = std::make_shared<ReentrantRemoveConsumer>();
    ReentrantRemoveConsumer::g_callbackCount = 0;
    ReentrantRemoveConsumer::g_removeResult = 0;
    ReentrantRemoveConsumer::g_removeType = HOOK_EVENT_TYPE_TOUCH;
    INPUT_EVENT_HOOK_HANDLER.currentHookStats_ = HOOK_EVENT_TYPE_TOUCH;
    INPUT_EVENT_HOOK_HANDLER.consumer_ = consumer;
    INPUT_EVENT_HOOK_HANDLER.hookConsumer_.touchHookCallback_ = [consumer](std::shared_ptr<PointerEvent> event) {
        consumer->OnInputEvent(event);
    };
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    INPUT_EVENT_HOOK_HANDLER.OnPointerEvent(pointerEvent);
    EXPECT_EQ(ReentrantRemoveConsumer::g_callbackCount, 1);
    EXPECT_EQ(ReentrantRemoveConsumer::g_removeResult, RET_OK);
    EXPECT_EQ(INPUT_EVENT_HOOK_HANDLER.currentHookStats_ & HOOK_EVENT_TYPE_TOUCH, 0u);
    ClearData();
}
/**
 * @tc.name: InputEventHookHandlerTest_TimeoutDisableAllTypes_001
 * @tc.desc: Client 1s timeout disables the whole handler (all types), not only the timeout type
 * @tc.type: FUNC
 * @tc.require:
 */
} // namespace MMI
} // namespace OHOS
