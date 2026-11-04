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

class TouchRedispatchStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}

protected:
    static std::shared_ptr<PointerEvent> CreateTouchEvent(int32_t pointerAction, int32_t toolType)
    {
        auto pointerEvent = PointerEvent::Create();
        if (pointerEvent == nullptr) {
            return nullptr;
        }
        pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
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
 * @tc.name: TouchRedispatchStoreTest_StylusFingerIsolation_001
 * @tc.desc: Stylus and finger states with the same (deviceId, pointerId) shall be isolated by toolType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TouchRedispatchStoreTest, TouchRedispatchStoreTest_StylusFingerIsolation_001, TestSize.Level1)
{
    TouchRedispatchStore store;
    auto penEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(penEvent, nullptr);
    auto fingerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(fingerEvent, nullptr);

    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN, penEvent);
    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER, fingerEvent);

    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN));
    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER));
    EXPECT_EQ(store.GetFingerMap().at(TEST_Z_ORDER).size(), 2u);
}

/**
 * @tc.name: TouchRedispatchStoreTest_StylusPencilIsolation_001
 * @tc.desc: TOOL_TYPE_PENCIL shall be isolated from TOOL_TYPE_PEN and finger states
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TouchRedispatchStoreTest, TouchRedispatchStoreTest_StylusPencilIsolation_001, TestSize.Level1)
{
    TouchRedispatchStore store;
    auto pencilEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN, PointerEvent::TOOL_TYPE_PENCIL);
    ASSERT_NE(pencilEvent, nullptr);

    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PENCIL, pencilEvent);

    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PENCIL));
    EXPECT_FALSE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN));
    EXPECT_FALSE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER));
}

/**
 * @tc.name: TouchRedispatchStoreTest_DeactivateIsolation_001
 * @tc.desc: Deactivating the stylus state shall not affect the finger state of the same pointerId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TouchRedispatchStoreTest, TouchRedispatchStoreTest_DeactivateIsolation_001, TestSize.Level1)
{
    TouchRedispatchStore store;
    auto penEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(penEvent, nullptr);
    auto fingerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(fingerEvent, nullptr);
    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN, penEvent);
    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER, fingerEvent);

    store.DeactivateFinger(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID, PointerEvent::TOOL_TYPE_PEN);

    EXPECT_FALSE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN));
    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER));

    store.DeactivateFinger(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID, PointerEvent::TOOL_TYPE_FINGER);
    EXPECT_FALSE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER));
    EXPECT_TRUE(store.GetFingerMap().find(TEST_Z_ORDER) == store.GetFingerMap().end());
}

/**
 * @tc.name: TouchRedispatchStoreTest_StylusWindowIdIsolation_001
 * @tc.desc: Same (deviceId, pointerId) with different toolType shall keep separate windowId records
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TouchRedispatchStoreTest, TouchRedispatchStoreTest_StylusWindowIdIsolation_001, TestSize.Level1)
{
    TouchRedispatchStore store;
    constexpr int32_t penWindowId { 100 };
    constexpr int32_t fingerWindowId { 200 };
    auto penEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(penEvent, nullptr);
    penEvent->SetTargetWindowId(penWindowId);
    auto fingerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(fingerEvent, nullptr);
    fingerEvent->SetTargetWindowId(fingerWindowId);

    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN, penEvent);
    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER, fingerEvent);

    EXPECT_EQ(store.GetFingerWindowId(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN), penWindowId);
    EXPECT_EQ(store.GetFingerWindowId(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER), fingerWindowId);
    EXPECT_EQ(store.GetFingerEvent(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN), penEvent);
    EXPECT_EQ(store.GetFingerEvent(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER), fingerEvent);
}

/**
 * @tc.name: TouchRedispatchStoreTest_StylusAbandonLifecycle_001
 * @tc.desc: Pen PROXIMITY_IN auto-activates, MOVE passes and PROXIMITY_OUT deactivates, toolType isolated
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TouchRedispatchStoreTest, TouchRedispatchStoreTest_StylusAbandonLifecycle_001, TestSize.Level1)
{
    TouchRedispatchStore store;
    auto proximityIn = CreateTouchEvent(PointerEvent::POINTER_ACTION_PROXIMITY_IN,
        PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(proximityIn, nullptr);
    EXPECT_FALSE(store.Abandon(proximityIn));
    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN));
    EXPECT_EQ(store.GetFingerWindowId(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN), TEST_WINDOW_ID);

    auto moveEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_MOVE, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(moveEvent, nullptr);
    EXPECT_FALSE(store.Abandon(moveEvent));
    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN));

    auto proximityOut = CreateTouchEvent(PointerEvent::POINTER_ACTION_PROXIMITY_OUT,
        PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(proximityOut, nullptr);
    EXPECT_FALSE(store.Abandon(proximityOut));
    EXPECT_FALSE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN));
}

/**
 * @tc.name: TouchRedispatchStoreTest_PencilAbandonLifecycle_001
 * @tc.desc: TOOL_TYPE_PENCIL follows the pen abandon path (PROXIMITY_IN activate, PROXIMITY_OUT deactivate)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TouchRedispatchStoreTest, TouchRedispatchStoreTest_PencilAbandonLifecycle_001, TestSize.Level1)
{
    TouchRedispatchStore store;
    auto proximityIn = CreateTouchEvent(PointerEvent::POINTER_ACTION_PROXIMITY_IN,
        PointerEvent::TOOL_TYPE_PENCIL);
    ASSERT_NE(proximityIn, nullptr);
    EXPECT_FALSE(store.Abandon(proximityIn));
    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PENCIL));

    auto upEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_UP, PointerEvent::TOOL_TYPE_PENCIL);
    ASSERT_NE(upEvent, nullptr);
    EXPECT_FALSE(store.Abandon(upEvent));
    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PENCIL));

    auto proximityOut = CreateTouchEvent(PointerEvent::POINTER_ACTION_PROXIMITY_OUT,
        PointerEvent::TOOL_TYPE_PENCIL);
    ASSERT_NE(proximityOut, nullptr);
    EXPECT_FALSE(store.Abandon(proximityOut));
    EXPECT_FALSE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PENCIL));
}

/**
 * @tc.name: TouchRedispatchStoreTest_FingerAbandonRegression_001
 * @tc.desc: Finger abandon lifecycle shall keep existing behavior (UP/CANCEL/HOVER_EXIT deactivate)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TouchRedispatchStoreTest, TouchRedispatchStoreTest_FingerAbandonRegression_001, TestSize.Level1)
{
    TouchRedispatchStore store;
    auto downEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(downEvent, nullptr);

    EXPECT_TRUE(store.Abandon(downEvent));
    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER, downEvent);
    EXPECT_FALSE(store.Abandon(downEvent));

    auto cancelEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_CANCEL,
        PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(cancelEvent, nullptr);
    EXPECT_FALSE(store.Abandon(cancelEvent));
    EXPECT_FALSE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_FINGER));
}

/**
 * @tc.name: TouchRedispatchStoreTest_StylusFingerCrossAbandon_001
 * @tc.desc: Finger abandon shall not consume active stylus state of the same pointerId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TouchRedispatchStoreTest, TouchRedispatchStoreTest_StylusFingerCrossAbandon_001, TestSize.Level1)
{
    TouchRedispatchStore store;
    auto penEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(penEvent, nullptr);
    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN, penEvent);

    auto fingerUpEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_UP, PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(fingerUpEvent, nullptr);
    EXPECT_TRUE(store.Abandon(fingerUpEvent));

    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN));

    auto penMoveEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_MOVE, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(penMoveEvent, nullptr);
    EXPECT_FALSE(store.Abandon(penMoveEvent));
}
} // namespace MMI
} // namespace OHOS
