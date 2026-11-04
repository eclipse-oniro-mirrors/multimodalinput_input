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
#include "window_info.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t TEST_DEVICE_ID { 1 };
constexpr int32_t TEST_WINDOW_ID { 42 };
constexpr int32_t TEST_WINDOW_ID_B { 43 };
constexpr float TEST_Z_ORDER { 1.0f };
constexpr float TEST_Z_ORDER_B { 2.0f };
} // namespace

class MouseRedispatchStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}

protected:
    static std::shared_ptr<PointerEvent> CreateMouseEvent(int32_t pointerAction, int32_t pressedButton)
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
        if (pressedButton >= 0) {
            pointerEvent->SetButtonPressed(pressedButton);
        }
        PointerEvent::PointerItem item;
        item.SetPointerId(0);
        item.SetToolType(PointerEvent::TOOL_TYPE_MOUSE);
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
 * @tc.name: MouseRedispatchStoreTest_ButtonAndAxisSessionCoexist_001
 * @tc.desc: Button session and axis session locked on the same window shall not interfere (DD-5)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseRedispatchStoreTest, MouseRedispatchStoreTest_ButtonAndAxisSessionCoexist_001, TestSize.Level1)
{
    MouseRedispatchStore store;
    store.SetWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID);
    store.SetWindowActive(MouseSessionType::AXIS, TEST_WINDOW_ID);
    EXPECT_TRUE(store.IsWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID));
    EXPECT_TRUE(store.IsWindowActive(MouseSessionType::AXIS, TEST_WINDOW_ID));

    store.DeactivateWindow(MouseSessionType::AXIS, TEST_WINDOW_ID);
    EXPECT_FALSE(store.IsWindowActive(MouseSessionType::AXIS, TEST_WINDOW_ID));
    EXPECT_TRUE(store.IsWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID));

    store.DeactivateWindow(MouseSessionType::BUTTON, TEST_WINDOW_ID);
    EXPECT_FALSE(store.IsWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID));
}

/**
 * @tc.name: MouseRedispatchStoreTest_LegacyOverloadMapsToAxis_001
 * @tc.desc: Legacy single-parameter overloads shall operate on the axis session set
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseRedispatchStoreTest, MouseRedispatchStoreTest_LegacyOverloadMapsToAxis_001, TestSize.Level1)
{
    MouseRedispatchStore store;
    store.SetWindowActive(TEST_WINDOW_ID);
    EXPECT_TRUE(store.IsWindowActive(TEST_WINDOW_ID));
    EXPECT_TRUE(store.IsWindowActive(MouseSessionType::AXIS, TEST_WINDOW_ID));
    EXPECT_FALSE(store.IsWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID));

    store.DeactivateWindow(TEST_WINDOW_ID);
    EXPECT_FALSE(store.IsWindowActive(TEST_WINDOW_ID));
}

/**
 * @tc.name: MouseRedispatchStoreTest_LastEventPerZOrder_001
 * @tc.desc: Cached last events shall be isolated by zOrder
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseRedispatchStoreTest, MouseRedispatchStoreTest_LastEventPerZOrder_001, TestSize.Level1)
{
    MouseRedispatchStore store;
    auto eventA = CreateMouseEvent(PointerEvent::POINTER_ACTION_MOVE, -1);
    auto eventB = CreateMouseEvent(PointerEvent::POINTER_ACTION_MOVE, -1);
    ASSERT_NE(eventA, nullptr);
    ASSERT_NE(eventB, nullptr);

    store.CacheLastEvent(eventA);
    store.GetLastEvent();

    EXPECT_EQ(store.GetLastEvent(TEST_Z_ORDER), eventA);
    EXPECT_EQ(store.GetLastEvent(TEST_Z_ORDER_B), nullptr);
}

/**
 * @tc.name: MouseRedispatchStoreTest_ButtonDownWindowPerZOrder_001
 * @tc.desc: Button session lock window shall be isolated by zOrder and searchable by windowId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseRedispatchStoreTest, MouseRedispatchStoreTest_ButtonDownWindowPerZOrder_001, TestSize.Level1)
{
    MouseRedispatchStore store;
    WindowInfo windowA = CreateWindowInfo(TEST_WINDOW_ID);
    WindowInfo windowB = CreateWindowInfo(TEST_WINDOW_ID_B);

    store.SetButtonDownWindow(TEST_Z_ORDER, windowA);
    store.SetButtonDownWindow(TEST_Z_ORDER_B, windowB);

    EXPECT_EQ(store.GetButtonDownWindow(TEST_Z_ORDER)->id, TEST_WINDOW_ID);
    EXPECT_EQ(store.GetButtonDownWindow(TEST_Z_ORDER_B)->id, TEST_WINDOW_ID_B);
    EXPECT_FLOAT_EQ(store.FindButtonSessionZOrder(TEST_WINDOW_ID), TEST_Z_ORDER);
    EXPECT_FLOAT_EQ(store.FindButtonSessionZOrder(TEST_WINDOW_ID_B), TEST_Z_ORDER_B);
    EXPECT_FLOAT_EQ(store.FindButtonSessionZOrder(-1), 0.0f);

    store.EraseButtonDownWindow(TEST_Z_ORDER);
    EXPECT_EQ(store.GetButtonDownWindow(TEST_Z_ORDER), std::nullopt);
    EXPECT_FLOAT_EQ(store.FindButtonSessionZOrder(TEST_WINDOW_ID), 0.0f);
}

/**
 * @tc.name: MouseRedispatchStoreTest_ResetClearsAll_001
 * @tc.desc: Reset shall clear session records and cached events
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MouseRedispatchStoreTest, MouseRedispatchStoreTest_ResetClearsAll_001, TestSize.Level1)
{
    MouseRedispatchStore store;
    auto event = CreateMouseEvent(PointerEvent::POINTER_ACTION_MOVE, -1);
    ASSERT_NE(event, nullptr);
    store.SetWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID);
    store.SetWindowActive(MouseSessionType::AXIS, TEST_WINDOW_ID);
    store.SetButtonDownWindow(TEST_Z_ORDER, CreateWindowInfo(TEST_WINDOW_ID));
    store.SetAxisBeginWindow(CreateWindowInfo(TEST_WINDOW_ID));

    store.Reset();

    EXPECT_FALSE(store.IsWindowActive(MouseSessionType::BUTTON, TEST_WINDOW_ID));
    EXPECT_FALSE(store.IsWindowActive(MouseSessionType::AXIS, TEST_WINDOW_ID));
    EXPECT_EQ(store.GetButtonDownWindow(TEST_Z_ORDER), std::nullopt);
    EXPECT_EQ(store.GetAxisBeginWindow(), std::nullopt);
    EXPECT_EQ(store.GetLastEvent(TEST_Z_ORDER), nullptr);
}
} // namespace MMI
} // namespace OHOS
