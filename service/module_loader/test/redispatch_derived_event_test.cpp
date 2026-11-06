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

#include "i_input_windows_manager.h"
#include "mmi_service.h"
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

class RedispatchDerivedEventTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}

protected:
    static std::shared_ptr<PointerEvent> CreateTouchEvent(int32_t pointerAction, int32_t toolType,
        int32_t sourceType)
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
 * @tc.name: RedispatchDerivedEventTest_DiscardLevitateInWindow_001
 * @tc.desc: LEVITATE_IN_WINDOW derived event shall be discarded on redispatch (AC-4.1)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardLevitateInWindow_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_LEVITATE_IN_WINDOW,
        PointerEvent::TOOL_TYPE_PEN, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardLevitateOutWindow_001
 * @tc.desc: LEVITATE_OUT_WINDOW derived event shall be discarded on redispatch (AC-4.2)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardLevitateOutWindow_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_LEVITATE_OUT_WINDOW,
        PointerEvent::TOOL_TYPE_PEN, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardLevitateInKeepStoreState_001
 * @tc.desc: Discarding LEVITATE_IN_WINDOW shall not touch active stylus/finger states (BR-2, EX-2)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardLevitateInKeepStoreState_001,
    TestSize.Level1)
{
    MMIService mmiService;
    auto& store = WIN_MGR->GetTouchRedispatchStore();
    auto penEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN,
        PointerEvent::TOOL_TYPE_PEN, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(penEvent, nullptr);
    store.SetFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN, penEvent);

    auto levitateIn = CreateTouchEvent(PointerEvent::POINTER_ACTION_LEVITATE_IN_WINDOW,
        PointerEvent::TOOL_TYPE_PEN, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(levitateIn, nullptr);
    EXPECT_EQ(mmiService.RedispatchInputEventInner(levitateIn), RET_OK);

    EXPECT_TRUE(store.IsFingerActive(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN));
    EXPECT_EQ(store.GetFingerWindowId(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID,
        PointerEvent::TOOL_TYPE_PEN), TEST_WINDOW_ID);
    store.DeactivateFinger(TEST_Z_ORDER, TEST_DEVICE_ID, TEST_POINTER_ID, PointerEvent::TOOL_TYPE_PEN);
}

/**
 * @tc.name: RedispatchDerivedEventTest_StylusDownNotDiscarded_001
 * @tc.desc: Stylus DOWN shall pass the derived-event gate; in host UT the dispatch chain is not
 *           initialized so the event is rejected after the gate (full dispatch covered on board)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_StylusDownNotDiscarded_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN,
        PointerEvent::TOOL_TYPE_PEN, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_NE(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
}

/**
 * @tc.name: RedispatchDerivedEventTest_StylusHoverMoveNotDiscarded_001
 * @tc.desc: Stylus LEVITATE_MOVE is not a derived event and shall not be discarded by the gate (AC-3.1)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_StylusHoverMoveNotDiscarded_001,
    TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_LEVITATE_MOVE,
        PointerEvent::TOOL_TYPE_PEN, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_NE(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
}

/**
 * @tc.name: RedispatchDerivedEventTest_MouseHoverEnterNotDiscarded_001
 * @tc.desc: Mouse redispatch behavior shall be unchanged - mouse HOVER_ENTER is not caught by the
 *           touchscreen derived-event gate (constraint: mouse branch untouched)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_MouseHoverEnterNotDiscarded_001,
    TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_HOVER_ENTER,
        PointerEvent::TOOL_TYPE_FINGER, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardEnterWindow_001
 * @tc.desc: Normal-path mouse ENTER_WINDOW derived event shall be discarded on redispatch (BR-2)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardEnterWindow_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_ENTER_WINDOW,
        PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardLeaveWindow_001
 * @tc.desc: Normal-path mouse LEAVE_WINDOW derived event shall be discarded on redispatch (BR-2)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardLeaveWindow_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_LEAVE_WINDOW,
        PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardPullInWindow_001
 * @tc.desc: PULL_IN_WINDOW drag derived event shall be discarded on redispatch (BR-2, EX-7)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardPullInWindow_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_PULL_IN_WINDOW,
        PointerEvent::TOOL_TYPE_FINGER, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardPullOutWindow_001
 * @tc.desc: PULL_OUT_WINDOW drag derived event shall be discarded on redispatch (BR-2, EX-7)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardPullOutWindow_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW,
        PointerEvent::TOOL_TYPE_FINGER, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardPullMove_001
 * @tc.desc: PULL_MOVE drag event shall be discarded on redispatch (BR-2, EX-7)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardPullMove_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_PULL_MOVE,
        PointerEvent::TOOL_TYPE_FINGER, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardMouseHoverMove_001
 * @tc.desc: Hover mouse MOVE (no pressed buttons) shall be discarded at the entry gate (BR-2, EX-10)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardMouseHoverMove_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_MOVE,
        PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_MousePressedMoveNotDiscarded_001
 * @tc.desc: Mouse MOVE with primary button pressed shall not be caught by the entry gate (AC-8.2)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_MousePressedMoveNotDiscarded_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_MOVE,
        PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardNonPrimaryButtonDown_001
 * @tc.desc: Non-primary BUTTON_DOWN (side/middle/combo) shall be discarded at the entry gate (BR-8, EX-9)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardNonPrimaryButtonDown_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_BUTTON_DOWN,
        PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_RIGHT);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardComboButtonDown_001
 * @tc.desc: Combo BUTTON_DOWN (left+right) shall be discarded at the entry gate (BR-8, EX-9)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardComboButtonDown_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_BUTTON_DOWN,
        PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_RIGHT);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_DiscardNonPrimaryButtonUp_001
 * @tc.desc: Non-primary BUTTON_UP (side button release) shall be discarded at the entry gate (BR-8, EX-9)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_DiscardNonPrimaryButtonUp_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_BUTTON_UP,
        PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_RIGHT);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_EQ(pointerEvent->GetTargetWindowId(), -1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_PrimaryButtonDownNotDiscarded_001
 * @tc.desc: Primary BUTTON_DOWN (left only) shall not be caught by the entry gate (AC-8.1)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_PrimaryButtonDownNotDiscarded_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_BUTTON_DOWN,
        PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);

    int32_t ret = mmiService.RedispatchInputEventInner(pointerEvent);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: RedispatchDerivedEventTest_RedispatchCount_001
 * @tc.desc: RedispatchInputEventInner increments redispatchCount on every redispatch request
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_RedispatchCount_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_BUTTON_DOWN,
        PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);
    EXPECT_EQ(pointerEvent->GetRedispatchCount(), 0);

    EXPECT_NE(mmiService.RedispatchInputEventInner(pointerEvent), RET_OK);
    EXPECT_EQ(pointerEvent->GetRedispatchCount(), 1);

    EXPECT_NE(mmiService.RedispatchInputEventInner(pointerEvent), RET_OK);
    EXPECT_EQ(pointerEvent->GetRedispatchCount(), 2);
}

/**
 * @tc.name: RedispatchDerivedEventTest_RedispatchCountDiscardedAlsoCounts_001
 * @tc.desc: redispatchCount increments even when the event is discarded by the entry gate (BR-2)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_RedispatchCountDiscardedAlsoCounts_001,
    TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_MOVE,
        PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);

    EXPECT_EQ(mmiService.RedispatchInputEventInner(pointerEvent), RET_OK);
    EXPECT_EQ(pointerEvent->GetRedispatchCount(), 1);
}

/**
 * @tc.name: RedispatchDerivedEventTest_RedispatchCountTouchCounts_001
 * @tc.desc: Touchscreen redispatch also increments redispatchCount
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchDerivedEventTest, RedispatchDerivedEventTest_RedispatchCountTouchCounts_001, TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN,
        PointerEvent::TOOL_TYPE_PEN, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(pointerEvent, nullptr);

    EXPECT_NE(mmiService.RedispatchInputEventInner(pointerEvent), RET_OK);
    EXPECT_EQ(pointerEvent->GetRedispatchCount(), 1);
}
} // namespace MMI
} // namespace OHOS
