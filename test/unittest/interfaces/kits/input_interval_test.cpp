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

/**
 * @file input_interval_test.cpp
 * @brief FEAT-30093682：EVENT_FLAG_NEED_UPDATE_LAST_INPUT_TIME 标签机制测试。
 *
 * 未打标注入（ODK C API，key + touch 覆盖键/指针两条注入入口）：不重置间隔（AC-3.x）；
 * 打标注入（innerkits AddFlag，模拟 cooperate 打标链路）：刷新间隔且随时间递增（AC-1.x）。
 *
 * 运行前提（板侧）：MMI 服务已启动；产品形态为 PC 或调用方具备注入权限（root 通常满足）；
 * 亮屏解锁；运行期间静置（无物理键鼠/触控输入）。
 * MEASURE_SLACK_MS 为同进程 sleep 计时 + IPC 往返 + 服务主线程调度的测量误差上界，可按实测收紧。
 */

#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <thread>

#include <gtest/gtest.h>

#include "input_event.h"
#include "input_manager.h"
#include "key_event.h"
#include "oh_input_manager.h"
#include "oh_key_code.h"
#include "pointer_event.h"

namespace {
constexpr int64_t US_PER_MS { 1000 };
/* 注入前静置时长与陈旧基线门槛：保证"最后一次输入"确定性地早于该门槛 */
constexpr int64_t ARM_SLEEP_MS { 1200 };
constexpr int64_t ARM_MIN_INTERVAL_MS { 1000 };
constexpr int64_t WAIT_MS { 500 };
constexpr int64_t MEASURE_SLACK_MS { 100 };
constexpr int32_t INJECT_POINT_X { 100 };
constexpr int32_t INJECT_POINT_Y { 100 };

int64_t QueryIntervalUs()
{
    int64_t intervalUs = -1;
    Input_Result ret = OH_Input_GetIntervalSinceLastInput(&intervalUs);
    EXPECT_EQ(ret, INPUT_SUCCESS);
    EXPECT_GE(intervalUs, 0);
    return intervalUs;
}

void SleepForMs(int64_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

/* 构造陈旧基线：静置后断言间隔不小于门槛（期间有物理输入混入则失败，静置后重跑） */
int64_t ArmStaleBaseline()
{
    SleepForMs(ARM_SLEEP_MS);
    int64_t intervalUs = QueryIntervalUs();
    EXPECT_GE(intervalUs, ARM_MIN_INTERVAL_MS * US_PER_MS);
    return intervalUs;
}

void InjectKeyTap(int32_t keyCode)
{
    Input_KeyEvent* keyEvent = OH_Input_CreateKeyEvent();
    ASSERT_NE(keyEvent, nullptr);
    OH_Input_SetKeyEventKeyCode(keyEvent, keyCode);
    OH_Input_SetKeyEventAction(keyEvent, KEY_ACTION_DOWN);
    EXPECT_EQ(OH_Input_InjectKeyEvent(keyEvent), INPUT_SUCCESS);
    OH_Input_SetKeyEventAction(keyEvent, KEY_ACTION_UP);
    EXPECT_EQ(OH_Input_InjectKeyEvent(keyEvent), INPUT_SUCCESS);
    OH_Input_DestroyKeyEvent(&keyEvent);
}

void InjectTouchTap()
{
    Input_TouchEvent* touchEvent = OH_Input_CreateTouchEvent();
    ASSERT_NE(touchEvent, nullptr);
    OH_Input_SetTouchEventDisplayX(touchEvent, INJECT_POINT_X);
    OH_Input_SetTouchEventDisplayY(touchEvent, INJECT_POINT_Y);
    OH_Input_SetTouchEventAction(touchEvent, TOUCH_ACTION_DOWN);
    EXPECT_EQ(OH_Input_InjectTouchEvent(touchEvent), INPUT_SUCCESS);
    OH_Input_SetTouchEventAction(touchEvent, TOUCH_ACTION_UP);
    EXPECT_EQ(OH_Input_InjectTouchEvent(touchEvent), INPUT_SUCCESS);
    OH_Input_DestroyTouchEvent(&touchEvent);
}

void InjectTaggedKeyTap(int32_t keyCode)
{
    std::shared_ptr<MMI::KeyEvent> keyEvent = MMI::KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetKeyCode(keyCode);
    keyEvent->SetKeyAction(MMI::KeyEvent::KEY_ACTION_DOWN);
    keyEvent->AddFlag(MMI::InputEvent::EVENT_FLAG_NEED_UPDATE_LAST_INPUT_TIME);
    MMI::InputManager::GetInstance()->SimulateInputEvent(keyEvent);
    keyEvent->SetKeyAction(MMI::KeyEvent::KEY_ACTION_UP);
    MMI::InputManager::GetInstance()->SimulateInputEvent(keyEvent);
}

void InjectTaggedPointerMove()
{
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    MMI::PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDownTime(0);
    item.SetDisplayX(INJECT_POINT_X);
    item.SetDisplayY(INJECT_POINT_Y);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetPointerAction(MMI::PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->AddFlag(MMI::InputEvent::EVENT_FLAG_NEED_UPDATE_LAST_INPUT_TIME);
    MMI::InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
}
} // namespace

class InputIntervalTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
};

/**
 * @tc.name: InputIntervalTest_NoInject_IntervalKeepsGrowing
 * @tc.desc: AC-2.2 回归保护：无注入场景下间隔随时间单调增长，无虚假更新
 * @tc.type: FUNC
 * @tc.require: FEAT-30093682
 */
HWTEST_F(InputIntervalTest, InputIntervalTest_NoInject_IntervalKeepsGrowing, TestSize.Level1)
{
    int64_t beforeUs = QueryIntervalUs();
    SleepForMs(WAIT_MS);
    int64_t afterUs = QueryIntervalUs();
    printf("NoInject: before=%" PRId64 "us, after=%" PRId64 "us\n", beforeUs, afterUs);
    EXPECT_GT(afterUs, beforeUs);
}

/**
 * @tc.name: InputIntervalTest_UntaggedInject_NotUpdate
 * @tc.desc: AC-3.1/3.2：未打标注入（ODK 链路，key + touch 覆盖键/指针入口）后间隔不被重置
 * @tc.type: FUNC
 * @tc.require: FEAT-30093682
 */
HWTEST_F(InputIntervalTest, InputIntervalTest_UntaggedInject_NotUpdate, TestSize.Level1)
{
    int64_t beforeUs = ArmStaleBaseline();
    InjectKeyTap(KEYCODE_A);
    InjectTouchTap();
    SleepForMs(WAIT_MS);
    int64_t afterUs = QueryIntervalUs();
    printf("UntaggedInject: before=%" PRId64 "us, after=%" PRId64 "us\n", beforeUs, afterUs);
    /* 未携带标签：时间戳不刷新，间隔不小于注入前记录值，仍保持陈旧 */
    EXPECT_GE(afterUs, beforeUs);
    EXPECT_GE(afterUs, ARM_MIN_INTERVAL_MS * US_PER_MS);
}

/**
 * @tc.name: InputIntervalTest_TaggedInject_ResetAndAdvance
 * @tc.desc: AC-1.1/1.2/1.3：打标注入（模拟 cooperate 打标链路）后间隔以注入处理时刻为
 *           起点刷新，再次打标注入再次刷新，无注入时随时间递增
 * @tc.type: FUNC
 * @tc.require: FEAT-30093682
 */
HWTEST_F(InputIntervalTest, InputIntervalTest_TaggedInject_ResetAndAdvance, TestSize.Level1)
{
    ArmStaleBaseline();
    InjectTaggedKeyTap(KEYCODE_A);
    SleepForMs(WAIT_MS);
    int64_t firstUs = QueryIntervalUs();
    printf("TaggedInject key: interval=%" PRId64 "us (bound=%" PRId64 "us)\n", firstUs,
        (WAIT_MS + MEASURE_SLACK_MS) * US_PER_MS);
    /* 打标 KeyEvent 刷新时间戳（AC-1.1） */
    EXPECT_LE(firstUs, (WAIT_MS + MEASURE_SLACK_MS) * US_PER_MS);

    InjectTaggedPointerMove();
    SleepForMs(WAIT_MS);
    int64_t secondUs = QueryIntervalUs();
    printf("TaggedInject pointer: interval=%" PRId64 "us (bound=%" PRId64 "us)\n", secondUs,
        (WAIT_MS + MEASURE_SLACK_MS) * US_PER_MS);
    /* 打标 PointerEvent 再次刷新（AC-1.2） */
    EXPECT_LE(secondUs, (WAIT_MS + MEASURE_SLACK_MS) * US_PER_MS);

    SleepForMs(WAIT_MS);
    int64_t thirdUs = QueryIntervalUs();
    printf("TaggedInject advance: second=%" PRId64 "us, third=%" PRId64 "us\n", secondUs, thirdUs);
    /* 无后续注入时间隔以最近一次注入处理时刻为锚点，随实际经过时间递增（AC-1.3） */
    EXPECT_GT(thirdUs, secondUs);
    EXPECT_LE(thirdUs - secondUs, (WAIT_MS + MEASURE_SLACK_MS) * US_PER_MS);
}
