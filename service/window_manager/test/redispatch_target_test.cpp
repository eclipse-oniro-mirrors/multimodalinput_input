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
#include "pointer_event.h"
#include "window_info.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t TEST_DEVICE_ID { 1 };
constexpr int32_t TEST_POINTER_ID { 0 };
constexpr int32_t TEST_WINDOW_ID { 100 };
constexpr float REDISPATCH_Z_ORDER { 1.0f };
constexpr float WINDOW_ABOVE_Z_ORDER { 2.0f };
constexpr float WINDOW_BELOW_Z_ORDER { 0.5f };
} // namespace

class RedispatchTargetTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}

protected:
    static std::shared_ptr<PointerEvent> CreateTouchEvent(int32_t toolType, float zOrder, bool redispatch)
    {
        auto pointerEvent = PointerEvent::Create();
        if (pointerEvent == nullptr) {
            return nullptr;
        }
        pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
        pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
        pointerEvent->SetDeviceId(TEST_DEVICE_ID);
        pointerEvent->SetPointerId(TEST_POINTER_ID);
        pointerEvent->SetZOrder(zOrder);
        pointerEvent->SetTargetWindowId(TEST_WINDOW_ID);
        if (redispatch) {
            pointerEvent->AddFlag(InputEvent::EVENT_FLAG_REDISPATCH);
        }
        PointerEvent::PointerItem item;
        item.SetPointerId(TEST_POINTER_ID);
        item.SetToolType(toolType);
        pointerEvent->AddPointerItem(item);
        return pointerEvent;
    }

    static WindowInfo CreateWindowInfo(float zOrder)
    {
        WindowInfo window;
        window.id = TEST_WINDOW_ID;
        window.zOrder = zOrder;
        window.flags = 0;
        return window;
    }
};

/**
 * @tc.name: RedispatchTargetTest_StylusWindowBelowZOrder_001
 * @tc.desc: Window below the redispatch zOrder shall be a valid stylus redispatch target (BR-5)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchTargetTest, RedispatchTargetTest_StylusWindowBelowZOrder_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto pointerEvent = CreateTouchEvent(PointerEvent::TOOL_TYPE_PEN, REDISPATCH_Z_ORDER, true);
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo window = CreateWindowInfo(WINDOW_BELOW_Z_ORDER);

    EXPECT_TRUE(inputWindowsManager.IsValidZorderWindow(window, pointerEvent));
}

/**
 * @tc.name: RedispatchTargetTest_StylusWindowAboveZOrderFiltered_001
 * @tc.desc: Window above the redispatch zOrder shall be filtered out for stylus redispatch (BR-5)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchTargetTest, RedispatchTargetTest_StylusWindowAboveZOrderFiltered_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto pointerEvent = CreateTouchEvent(PointerEvent::TOOL_TYPE_PEN, REDISPATCH_Z_ORDER, true);
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo window = CreateWindowInfo(WINDOW_ABOVE_Z_ORDER);

    EXPECT_FALSE(inputWindowsManager.IsValidZorderWindow(window, pointerEvent));
}

/**
 * @tc.name: RedispatchTargetTest_StylusWindowEqualZOrderFiltered_001
 * @tc.desc: Window equal to the redispatch zOrder (the gesture window itself) shall be filtered out
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchTargetTest, RedispatchTargetTest_StylusWindowEqualZOrderFiltered_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto pointerEvent = CreateTouchEvent(PointerEvent::TOOL_TYPE_PEN, REDISPATCH_Z_ORDER, true);
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo window = CreateWindowInfo(REDISPATCH_Z_ORDER);

    EXPECT_FALSE(inputWindowsManager.IsValidZorderWindow(window, pointerEvent));
}

/**
 * @tc.name: RedispatchTargetTest_StylusPencilWindowFilter_001
 * @tc.desc: TOOL_TYPE_PENCIL redispatch shall follow the same zOrder filtering (BR-1, BR-5)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchTargetTest, RedispatchTargetTest_StylusPencilWindowFilter_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto pointerEvent = CreateTouchEvent(PointerEvent::TOOL_TYPE_PENCIL, REDISPATCH_Z_ORDER, true);
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo belowWindow = CreateWindowInfo(WINDOW_BELOW_Z_ORDER);
    WindowInfo aboveWindow = CreateWindowInfo(WINDOW_ABOVE_Z_ORDER);

    EXPECT_TRUE(inputWindowsManager.IsValidZorderWindow(belowWindow, pointerEvent));
    EXPECT_FALSE(inputWindowsManager.IsValidZorderWindow(aboveWindow, pointerEvent));
}

/**
 * @tc.name: RedispatchTargetTest_FingerWindowFilter_001
 * @tc.desc: Finger redispatch shall keep the same zOrder filtering behavior (constraint: finger
 *           redispatch behavior unchanged)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchTargetTest, RedispatchTargetTest_FingerWindowFilter_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto pointerEvent = CreateTouchEvent(PointerEvent::TOOL_TYPE_FINGER, REDISPATCH_Z_ORDER, true);
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo belowWindow = CreateWindowInfo(WINDOW_BELOW_Z_ORDER);
    WindowInfo aboveWindow = CreateWindowInfo(WINDOW_ABOVE_Z_ORDER);

    EXPECT_TRUE(inputWindowsManager.IsValidZorderWindow(belowWindow, pointerEvent));
    EXPECT_FALSE(inputWindowsManager.IsValidZorderWindow(aboveWindow, pointerEvent));
}

/**
 * @tc.name: RedispatchTargetTest_NormalEventNoZOrderFilter_001
 * @tc.desc: Non-redispatch events shall not be filtered by zOrder (EX-4: normal path unaffected)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchTargetTest, RedispatchTargetTest_NormalEventNoZOrderFilter_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto pointerEvent = CreateTouchEvent(PointerEvent::TOOL_TYPE_PEN, REDISPATCH_Z_ORDER, false);
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo aboveWindow = CreateWindowInfo(WINDOW_ABOVE_Z_ORDER);

    EXPECT_TRUE(inputWindowsManager.IsValidZorderWindow(aboveWindow, pointerEvent));
}

/**
 * @tc.name: RedispatchTargetTest_ZeroZOrderNoFilter_001
 * @tc.desc: Redispatch event without a specified zOrder shall not filter any window
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchTargetTest, RedispatchTargetTest_ZeroZOrderNoFilter_001, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    auto pointerEvent = CreateTouchEvent(PointerEvent::TOOL_TYPE_PEN, 0.0f, true);
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo aboveWindow = CreateWindowInfo(WINDOW_ABOVE_Z_ORDER);

    EXPECT_TRUE(inputWindowsManager.IsValidZorderWindow(aboveWindow, pointerEvent));
}
} // namespace MMI
} // namespace OHOS
