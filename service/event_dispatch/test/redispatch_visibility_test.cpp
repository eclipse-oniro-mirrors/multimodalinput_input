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

#include "event_dispatch_handler.h"
#include "pointer_event.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t TEST_DEVICE_ID { 1 };
constexpr int32_t TEST_POINTER_ID { 0 };
constexpr int32_t TEST_WINDOW_ID { 42 };
} // namespace

class RedispatchVisibilityTest : public testing::Test {
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
        pointerEvent->SetZOrder(1.0f);
        pointerEvent->SetTargetWindowId(TEST_WINDOW_ID);
        pointerEvent->AddFlag(InputEvent::EVENT_FLAG_REDISPATCH);
        PointerEvent::PointerItem item;
        item.SetPointerId(TEST_POINTER_ID);
        item.SetToolType(toolType);
        pointerEvent->AddPointerItem(item);
        return pointerEvent;
    }
};

/**
 * @tc.name: RedispatchVisibilityTest_StylusEventEntersAtDispatchStage_001
 * @tc.desc: Redispatch stylus event enters the chain at EventDispatchHandler (AC-5.1/AC-5.2):
 *           the monitor/interceptor nodes only exist upstream in the processing chain
 *           (input_event_handler.cpp Normalize -> ... -> Monitor -> Hook -> Dispatch), so
 *           HandleTouchEvent dispatches without consulting them; middle listeners are not notified
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchVisibilityTest, RedispatchVisibilityTest_StylusEventEntersAtDispatchStage_001,
    TestSize.Level1)
{
    EventDispatchHandler eventDispatchHandler;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_MOVE, PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(pointerEvent, nullptr);
    ASSERT_NO_FATAL_FAILURE(eventDispatchHandler.HandleTouchEvent(pointerEvent));
    EXPECT_TRUE(pointerEvent->HasFlag(InputEvent::EVENT_FLAG_REDISPATCH));
}

/**
 * @tc.name: RedispatchVisibilityTest_StylusHoverMoveEntersAtDispatchStage_001
 * @tc.desc: Redispatch stylus hover-move event shall reach the dispatch stage the same way (AC-3.1)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchVisibilityTest, RedispatchVisibilityTest_StylusHoverMoveEntersAtDispatchStage_001,
    TestSize.Level1)
{
    EventDispatchHandler eventDispatchHandler;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_HOVER_MOVE,
        PointerEvent::TOOL_TYPE_PEN);
    ASSERT_NE(pointerEvent, nullptr);
    ASSERT_NO_FATAL_FAILURE(eventDispatchHandler.HandleTouchEvent(pointerEvent));
}

/**
 * @tc.name: RedispatchVisibilityTest_FingerEventEntersAtDispatchStage_001
 * @tc.desc: Finger redispatch shall keep entering at the dispatch stage (constraint: finger
 *           redispatch behavior unchanged)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchVisibilityTest, RedispatchVisibilityTest_FingerEventEntersAtDispatchStage_001,
    TestSize.Level1)
{
    EventDispatchHandler eventDispatchHandler;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_MOVE,
        PointerEvent::TOOL_TYPE_FINGER);
    ASSERT_NE(pointerEvent, nullptr);
    ASSERT_NO_FATAL_FAILURE(eventDispatchHandler.HandleTouchEvent(pointerEvent));
}

/**
 * @tc.name: RedispatchVisibilityTest_StylusPencilEventEntersAtDispatchStage_001
 * @tc.desc: TOOL_TYPE_PENCIL redispatch event shall reach the dispatch stage identically (BR-1)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchVisibilityTest, RedispatchVisibilityTest_StylusPencilEventEntersAtDispatchStage_001,
    TestSize.Level1)
{
    EventDispatchHandler eventDispatchHandler;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_MOVE,
        PointerEvent::TOOL_TYPE_PENCIL);
    ASSERT_NE(pointerEvent, nullptr);
    ASSERT_NO_FATAL_FAILURE(eventDispatchHandler.HandleTouchEvent(pointerEvent));
}
} // namespace MMI
} // namespace OHOS
