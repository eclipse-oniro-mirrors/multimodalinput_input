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

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t TEST_DEVICE_ID { 1 };
constexpr int32_t TEST_WINDOW_ID { 42 };
constexpr float TEST_Z_ORDER { 1.0f };
constexpr int32_t BUTTON_LEFT { PointerEvent::MOUSE_BUTTON_LEFT };
} // namespace

class RedispatchMouseSessionTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp()
    {
        store_ = std::make_unique<MouseRedispatchStore>();
    }
    void TearDown() {}

protected:
    std::unique_ptr<MouseRedispatchStore> store_;

    static std::shared_ptr<PointerEvent> CreateMouseEvent(int32_t pointerAction, const std::vector<int32_t>& buttons,
        int32_t toolType = PointerEvent::TOOL_TYPE_MOUSE)
    {
        auto pointerEvent = PointerEvent::Create();
        if (pointerEvent == nullptr) {
            return nullptr;
        }
        pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
        pointerEvent->SetPointerAction(pointerAction);
        pointerEvent->SetDeviceId(TEST_DEVICE_ID);
        pointerEvent->SetPointerId(0);
        pointerEvent->SetZOrder(TEST_Z_ORDER);
        pointerEvent->SetTargetWindowId(TEST_WINDOW_ID);
        for (int32_t buttonId : buttons) {
            pointerEvent->SetButtonPressed(buttonId);
        }
        PointerEvent::PointerItem item;
        item.SetPointerId(0);
        item.SetToolType(toolType);
        pointerEvent->AddPointerItem(item);
        return pointerEvent;
    }

    void ActivateButtonSession()
    {
        store_->SetWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID);
        store_->SetButtonDownWindow(TEST_Z_ORDER, CreateWindowInfo());
    }

    static WindowInfo CreateWindowInfo()
    {
        WindowInfo window;
        window.id = TEST_WINDOW_ID;
        window.zOrder = TEST_Z_ORDER;
        window.flags = 0;
        return window;
    }
};

/**
 * @tc.name: RedispatchMouseSessionTest_AbandonButtonDownRequiresActivation_001
 * @tc.desc: BUTTON_DOWN shall pass after activation and be rejected when not activated (AC-8.1)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchMouseSessionTest, RedispatchMouseSessionTest_AbandonButtonDownRequiresActivation_001,
    TestSize.Level1)
{
    auto downEvent = CreateMouseEvent(PointerEvent::POINTER_ACTION_BUTTON_DOWN, { BUTTON_LEFT });
    ASSERT_NE(downEvent, nullptr);
    EXPECT_TRUE(store_->Abandon(downEvent));

    ActivateButtonSession();
    EXPECT_FALSE(store_->Abandon(downEvent));
}

/**
 * @tc.name: RedispatchMouseSessionTest_AbandonPressedMoveRequiresActivation_001
 * @tc.desc: Pressed MOVE shall require an active button session (AC-8.2)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchMouseSessionTest, RedispatchMouseSessionTest_AbandonPressedMoveRequiresActivation_001,
    TestSize.Level1)
{
    auto pressedMove = CreateMouseEvent(PointerEvent::POINTER_ACTION_MOVE, { BUTTON_LEFT });
    ASSERT_NE(pressedMove, nullptr);
    EXPECT_TRUE(store_->Abandon(pressedMove));

    ActivateButtonSession();
    EXPECT_FALSE(store_->Abandon(pressedMove));
}

/**
 * @tc.name: RedispatchMouseSessionTest_AbandonButtonUpDeactivates_001
 * @tc.desc: BUTTON_UP shall be delivered and deactivate the session when pressed set becomes empty (AC-8.3)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchMouseSessionTest, RedispatchMouseSessionTest_AbandonButtonUpDeactivates_001, TestSize.Level1)
{
    ActivateButtonSession();
    auto upEvent = CreateMouseEvent(PointerEvent::POINTER_ACTION_BUTTON_UP, {});
    ASSERT_NE(upEvent, nullptr);
    EXPECT_FALSE(store_->Abandon(upEvent));
    EXPECT_FALSE(store_->IsWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID));
    EXPECT_EQ(store_->GetButtonDownWindow(TEST_Z_ORDER), std::nullopt);
}

/**
 * @tc.name: RedispatchMouseSessionTest_AbandonComboButtonUpKeepsSession_001
 * @tc.desc: Releasing one button while another is held shall keep the button session (DD-5 combo-key rule)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchMouseSessionTest, RedispatchMouseSessionTest_AbandonComboButtonUpKeepsSession_001, TestSize.Level1)
{
    ActivateButtonSession();
    auto comboUp = CreateMouseEvent(PointerEvent::POINTER_ACTION_BUTTON_UP, { BUTTON_LEFT });
    ASSERT_NE(comboUp, nullptr);
    EXPECT_FALSE(store_->Abandon(comboUp));
    EXPECT_TRUE(store_->IsWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID));
}

/**
 * @tc.name: RedispatchMouseSessionTest_AbandonAxisSessionUnchanged_001
 * @tc.desc: AXIS redispatch admission shall keep current behavior: require axis session, end on AXIS_END
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchMouseSessionTest, RedispatchMouseSessionTest_AbandonAxisSessionUnchanged_001, TestSize.Level1)
{
    auto axisBegin = CreateMouseEvent(PointerEvent::POINTER_ACTION_AXIS_BEGIN, {},
        PointerEvent::TOOL_TYPE_TOUCHPAD);
    auto axisEnd = CreateMouseEvent(PointerEvent::POINTER_ACTION_AXIS_END, {},
        PointerEvent::TOOL_TYPE_TOUCHPAD);
    ASSERT_NE(axisBegin, nullptr);
    ASSERT_NE(axisEnd, nullptr);

    EXPECT_TRUE(store_->Abandon(axisBegin));
    store_->SetWindowActive(MouseSessionType::AXIS, TEST_WINDOW_ID);
    EXPECT_FALSE(store_->Abandon(axisBegin));
    EXPECT_FALSE(store_->Abandon(axisEnd));
    EXPECT_FALSE(store_->IsWindowActive(MouseSessionType::AXIS, TEST_WINDOW_ID));
}

/**
 * @tc.name: RedispatchMouseSessionTest_ButtonSessionFoundByWindow_001
 * @tc.desc: The zOrder of the button session locked on a window shall be discoverable for eviction
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchMouseSessionTest, RedispatchMouseSessionTest_ButtonSessionFoundByWindow_001, TestSize.Level1)
{
    ActivateButtonSession();
    EXPECT_FLOAT_EQ(store_->FindButtonSessionZOrder(TEST_WINDOW_ID), TEST_Z_ORDER);

    store_->EraseButtonDownWindow(TEST_Z_ORDER);
    store_->DeactivateWindow(MouseSessionType::BUTTON, TEST_WINDOW_ID);
    EXPECT_FLOAT_EQ(store_->FindButtonSessionZOrder(TEST_WINDOW_ID), 0.0f);
}
/**
 * @tc.name: RedispatchMouseSessionTest_MouseWheelRejected_001
 * @tc.desc: Mouse wheel axis events (TOOL_TYPE_MOUSE) shall be rejected even when an axis session is
 *           active; only touchpad two-finger axis events support redispatch (BR-7, EX-8)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchMouseSessionTest, RedispatchMouseSessionTest_MouseWheelRejected_001, TestSize.Level1)
{
    auto mouseWheel = CreateMouseEvent(PointerEvent::POINTER_ACTION_AXIS_BEGIN, {},
        PointerEvent::TOOL_TYPE_MOUSE);
    ASSERT_NE(mouseWheel, nullptr);
    EXPECT_TRUE(store_->Abandon(mouseWheel));

    store_->SetWindowActive(MouseSessionType::AXIS, TEST_WINDOW_ID);
    EXPECT_TRUE(store_->Abandon(mouseWheel));
}

/**
 * @tc.name: RedispatchMouseSessionTest_NonPrimaryButtonRejected_001
 * @tc.desc: Non-primary button (middle/side) events shall be rejected and shall not establish a
 *           button session (BR-8, EX-9)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchMouseSessionTest, RedispatchMouseSessionTest_NonPrimaryButtonRejected_001, TestSize.Level1)
{
    auto middleDown = CreateMouseEvent(PointerEvent::POINTER_ACTION_BUTTON_DOWN,
        { PointerEvent::MOUSE_BUTTON_MIDDLE });
    ASSERT_NE(middleDown, nullptr);
    EXPECT_TRUE(store_->Abandon(middleDown));
    EXPECT_FALSE(store_->IsWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID));

    auto pressedMove = CreateMouseEvent(PointerEvent::POINTER_ACTION_MOVE,
        { PointerEvent::MOUSE_BUTTON_MIDDLE });
    ASSERT_NE(pressedMove, nullptr);
    EXPECT_TRUE(store_->Abandon(pressedMove));
}
} // namespace MMI
} // namespace OHOS
