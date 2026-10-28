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

#include "input_windows_manager.h"
#include "mouse_redispatch_store.h"
#include "pointer_event.h"
#include "window_info.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t TEST_DEVICE_ID { 1 };
constexpr int32_t TEST_WINDOW_ID { 42 };
constexpr int32_t TEST_WINDOW_ID_B { 43 };
constexpr float TEST_Z_ORDER { 1.0f };
} // namespace

class MouseHoverEnterLeaveTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}

protected:
    static std::shared_ptr<PointerEvent> CreateHoverMoveEvent(int32_t targetWindowId)
    {
        auto pointerEvent = PointerEvent::Create();
        if (pointerEvent == nullptr) {
            return nullptr;
        }
        pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
        pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
        pointerEvent->SetDeviceId(TEST_DEVICE_ID);
        pointerEvent->SetPointerId(0);
        pointerEvent->SetZOrder(TEST_Z_ORDER);
        pointerEvent->SetTargetWindowId(targetWindowId);
        pointerEvent->AddFlag(InputEvent::EVENT_FLAG_REDISPATCH);
        PointerEvent::PointerItem item;
        item.SetPointerId(0);
        item.SetToolType(PointerEvent::TOOL_TYPE_MOUSE);
        item.SetTargetWindowId(targetWindowId);
        pointerEvent->AddPointerItem(item);
        return pointerEvent;
    }

    static WindowInfo CreateWindowInfo(int32_t windowId)
    {
        WindowInfo window;
        window.id = windowId;
        window.zOrder = TEST_Z_ORDER;
        window.flags = 0;
        return window;
    }
};

/**
 * @tc.name: MouseHoverEnterLeaveTest_FirstHoverRecordsWindowOnly_001
 * @tc.desc: The first hover event shall record the hover window without LEAVE synthesis (DD-5)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseHoverEnterLeaveTest, MouseHoverEnterLeaveTest_FirstHoverRecordsWindowOnly_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto& store = inputWindowsManager.GetMouseRedispatchStore();
    auto hoverMove = CreateHoverMoveEvent(TEST_WINDOW_ID);
    ASSERT_NE(hoverMove, nullptr);

    MouseRedispatchStore::Guard guard(hoverMove);
    inputWindowsManager.SynthesizeHoverEnterLeave(hoverMove, CreateWindowInfo(TEST_WINDOW_ID));

    EXPECT_EQ(store.GetLastHoverWindow(TEST_Z_ORDER), TEST_WINDOW_ID);
}

/**
 * @tc.name: MouseHoverEnterLeaveTest_SameWindowNoUpdate_001
 * @tc.desc: Hover staying in the same window shall not change the hover memory
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseHoverEnterLeaveTest, MouseHoverEnterLeaveTest_SameWindowNoUpdate_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto& store = inputWindowsManager.GetMouseRedispatchStore();
    store.SetLastHoverWindow(TEST_Z_ORDER, TEST_WINDOW_ID);
    auto hoverMove = CreateHoverMoveEvent(TEST_WINDOW_ID);
    ASSERT_NE(hoverMove, nullptr);

    MouseRedispatchStore::Guard guard(hoverMove);
    inputWindowsManager.SynthesizeHoverEnterLeave(hoverMove, CreateWindowInfo(TEST_WINDOW_ID));

    EXPECT_EQ(store.GetLastHoverWindow(TEST_Z_ORDER), TEST_WINDOW_ID);
}

/**
 * @tc.name: MouseHoverEnterLeaveTest_CrossWindowUpdatesMemory_001
 * @tc.desc: Hover crossing windows shall update the hover memory to the new window (DD-5, AC-8.2)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseHoverEnterLeaveTest, MouseHoverEnterLeaveTest_CrossWindowUpdatesMemory_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto& store = inputWindowsManager.GetMouseRedispatchStore();
    store.SetLastHoverWindow(TEST_Z_ORDER, TEST_WINDOW_ID);
    auto hoverMove = CreateHoverMoveEvent(TEST_WINDOW_ID_B);
    ASSERT_NE(hoverMove, nullptr);

    MouseRedispatchStore::Guard guard(hoverMove);
    inputWindowsManager.SynthesizeHoverEnterLeave(hoverMove, CreateWindowInfo(TEST_WINDOW_ID_B));

    EXPECT_EQ(store.GetLastHoverWindow(TEST_Z_ORDER), TEST_WINDOW_ID_B);
}

/**
 * @tc.name: MouseHoverEnterLeaveTest_PressedMoveNoSynthesis_001
 * @tc.desc: Pressed MOVE belongs to the button session and shall not trigger hover synthesis
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseHoverEnterLeaveTest, MouseHoverEnterLeaveTest_PressedMoveNoSynthesis_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto& store = inputWindowsManager.GetMouseRedispatchStore();
    auto pressedMove = CreateHoverMoveEvent(TEST_WINDOW_ID);
    ASSERT_NE(pressedMove, nullptr);
    pressedMove->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);

    MouseRedispatchStore::Guard guard(pressedMove);
    inputWindowsManager.SynthesizeHoverEnterLeave(pressedMove, CreateWindowInfo(TEST_WINDOW_ID));

    EXPECT_EQ(store.GetLastHoverWindow(TEST_Z_ORDER), -1);
}
} // namespace MMI
} // namespace OHOS
