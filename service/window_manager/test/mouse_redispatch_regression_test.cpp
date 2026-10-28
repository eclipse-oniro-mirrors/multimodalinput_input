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
#include <memory>

#include "mouse_redispatch_store.h"
#include "pointer_event.h"
#include "touch_redispatch_store.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t TEST_DEVICE_ID { 1 };
constexpr int32_t TEST_POINTER_ID { 0 };
constexpr int32_t TEST_WINDOW_ID { 42 };
constexpr float TEST_Z_ORDER { 1.0f };
} // namespace

class MouseRedispatchRegressionTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}

protected:
    static std::shared_ptr<PointerEvent> CreatePointerEvent(int32_t pointerAction, int32_t sourceType,
        int32_t toolType)
    {
        auto pointerEvent = PointerEvent::Create();
        if (pointerEvent == nullptr) {
            return nullptr;
        }
        pointerEvent->SetSourceType(sourceType);
        pointerEvent->SetPointerAction(pointerAction);
        pointerEvent->SetDeviceId(TEST_DEVICE_ID);
        pointerEvent->SetPointerId(TEST_POINTER_ID);
        pointerEvent->SetZOrder(TEST_Z_ORDER);
        pointerEvent->SetTargetWindowId(TEST_WINDOW_ID);
        PointerEvent::PointerItem item;
        item.SetPointerId(TEST_POINTER_ID);
        item.SetToolType(toolType);
        pointerEvent->AddPointerItem(item);
        return pointerEvent;
    }
};

/**
 * @tc.name: MouseRedispatchRegressionTest_MouseAbandonUnchanged_001
 * @tc.desc: Unpressed mouse redispatch shall always pass admission, axis session keeps
 *           AXIS_BEGIN/AXIS_END lifecycle (DD-5, AC-8.4)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseRedispatchRegressionTest, MouseRedispatchRegressionTest_MouseAbandonUnchanged_001,
    TestSize.Level1)
{
    MouseRedispatchStore store;
    auto moveEvent = CreatePointerEvent(PointerEvent::POINTER_ACTION_MOVE,
        PointerEvent::SOURCE_TYPE_MOUSE, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(moveEvent, nullptr);

    EXPECT_FALSE(store.Abandon(moveEvent));

    auto axisBeginEvent = CreatePointerEvent(PointerEvent::POINTER_ACTION_AXIS_BEGIN,
        PointerEvent::SOURCE_TYPE_MOUSE, PointerEvent::TOOL_TYPE_FINGER);
    auto axisEndEvent = CreatePointerEvent(PointerEvent::POINTER_ACTION_AXIS_END,
        PointerEvent::SOURCE_TYPE_MOUSE, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(axisBeginEvent, nullptr);
    ASSERT_NE(axisEndEvent, nullptr);

    EXPECT_TRUE(store.Abandon(axisBeginEvent));
    store.SetWindowActive(TEST_WINDOW_ID);
    EXPECT_FALSE(store.Abandon(axisBeginEvent));

    EXPECT_FALSE(store.Abandon(axisEndEvent));
    EXPECT_TRUE(store.Abandon(axisBeginEvent));
}

/**
 * @tc.name: MouseRedispatchRegressionTest_FingerLifecycleUnchanged_001
 * @tc.desc: Touch finger redispatch lifecycle shall keep existing behavior (AC-7 constraint:
 *           finger redispatch behavior unchanged)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseRedispatchRegressionTest, MouseRedispatchRegressionTest_FingerLifecycleUnchanged_001,
    TestSize.Level1)
{
    TouchRedispatchStore store;
    auto downEvent = CreatePointerEvent(PointerEvent::POINTER_ACTION_DOWN,
        PointerEvent::SOURCE_TYPE_TOUCHSCREEN, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(downEvent, nullptr);

    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER, downEvent);
    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID));
    EXPECT_EQ(store.GetFingerWindowId(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID), TEST_WINDOW_ID);

    auto upEvent = CreatePointerEvent(PointerEvent::POINTER_ACTION_UP,
        PointerEvent::SOURCE_TYPE_TOUCHSCREEN, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(upEvent, nullptr);
    EXPECT_FALSE(store.Abandon(upEvent));
    EXPECT_FALSE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID));
    EXPECT_EQ(store.GetFingerWindowId(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID), -1);
}

/**
 * @tc.name: MouseRedispatchRegressionTest_StylusGestureStateUnchanged_001
 * @tc.desc: Stylus redispatch state shall keep supporting the full gesture event sequence
 *           (AC-7.2): DOWN/MOVE/UP all pass abandon while the pen stays active
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseRedispatchRegressionTest, MouseRedispatchRegressionTest_StylusGestureStateUnchanged_001,
    TestSize.Level1)
{
    TouchRedispatchStore store;
    auto downEvent = CreatePointerEvent(PointerEvent::POINTER_ACTION_DOWN,
        PointerEvent::SOURCE_TYPE_TOUCHSCREEN, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(downEvent, nullptr);
    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN, downEvent);

    auto moveEvent = CreatePointerEvent(PointerEvent::POINTER_ACTION_MOVE,
        PointerEvent::SOURCE_TYPE_TOUCHSCREEN, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(moveEvent, nullptr);
    EXPECT_FALSE(store.Abandon(moveEvent));

    auto upEvent = CreatePointerEvent(PointerEvent::POINTER_ACTION_UP,
        PointerEvent::SOURCE_TYPE_TOUCHSCREEN, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(upEvent, nullptr);
    EXPECT_FALSE(store.Abandon(upEvent));

    auto proximityOut = CreatePointerEvent(PointerEvent::POINTER_ACTION_PROXIMITY_OUT,
        PointerEvent::SOURCE_TYPE_TOUCHSCREEN, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(proximityOut, nullptr);
    EXPECT_FALSE(store.Abandon(proximityOut));
    EXPECT_FALSE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN));
}

/**
 * @tc.name: MouseRedispatchRegressionTest_StylusFingerMutualIsolation_001
 * @tc.desc: Finger termination shall not consume stylus state and vice versa (EX-2 regression)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseRedispatchRegressionTest, MouseRedispatchRegressionTest_StylusFingerMutualIsolation_001,
    TestSize.Level1)
{
    TouchRedispatchStore store;
    auto penEvent = CreatePointerEvent(PointerEvent::POINTER_ACTION_DOWN,
        PointerEvent::SOURCE_TYPE_TOUCHSCREEN, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(penEvent, nullptr);
    auto fingerEvent = CreatePointerEvent(PointerEvent::POINTER_ACTION_DOWN,
        PointerEvent::SOURCE_TYPE_TOUCHSCREEN, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(fingerEvent, nullptr);
    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN, penEvent);
    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER, fingerEvent);

    auto fingerUp = CreatePointerEvent(PointerEvent::POINTER_ACTION_UP,
        PointerEvent::SOURCE_TYPE_TOUCHSCREEN, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(fingerUp, nullptr);
    EXPECT_FALSE(store.Abandon(fingerUp));
    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN));

    auto penProximityOut = CreatePointerEvent(PointerEvent::POINTER_ACTION_PROXIMITY_OUT,
        PointerEvent::SOURCE_TYPE_TOUCHSCREEN, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(penProximityOut, nullptr);
    EXPECT_FALSE(store.Abandon(penProximityOut));
    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER));
}
} // namespace MMI
} // namespace OHOS
