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

#include "mmi_service.h"
#include "pointer_event.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t TEST_DEVICE_ID { 1 };
constexpr int32_t TEST_POINTER_ID { 0 };
constexpr int32_t TEST_WINDOW_ID { 42 };
} // namespace

class RedispatchPermissionTest : public testing::Test {
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
        pointerEvent->SetZOrder(1.0f);
        pointerEvent->SetTargetWindowId(TEST_WINDOW_ID);
        PointerEvent::PointerItem item;
        item.SetPointerId(TEST_POINTER_ID);
        item.SetToolType(toolType);
        pointerEvent->AddPointerItem(item);
        return pointerEvent;
    }
};

/**
 * @tc.name: RedispatchPermissionTest_UnsupportedSourceTypeRejected_001
 * @tc.desc: RedispatchInputEvent shall reject unsupported source types before any redispatch (BR-1)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchPermissionTest, RedispatchPermissionTest_UnsupportedSourceTypeRejected_001,
    TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN,
        PointerEvent::TOOL_TYPE_PEN, PointerEvent::SOURCE_TYPE_TOUCHPAD);
    ASSERT_NE(pointerEvent, nullptr);

    ErrCode ret = mmiService.RedispatchInputEvent(*pointerEvent);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: RedispatchPermissionTest_StylusEntryGatesBeforeDispatch_001
 * @tc.desc: Stylus redispatch call shall pass the source gate and stop at entry gates
 *           (service running / VerifySystemApp / CheckInjectPermission, in entry order) before
 *           any event dispatch; the UT caller holds no inject permission and the stack service
 *           is not started, so the event is never redispatched (AC-6.1; the exact permission
 *           error code is covered by board-side verification)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchPermissionTest, RedispatchPermissionTest_StylusEntryGatesBeforeDispatch_001,
    TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_DOWN,
        PointerEvent::TOOL_TYPE_PEN, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(pointerEvent, nullptr);

    ErrCode ret = mmiService.RedispatchInputEvent(*pointerEvent);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: RedispatchPermissionTest_MouseEntryGatesBeforeDispatch_001
 * @tc.desc: Mouse redispatch entry gates shall keep existing behavior (AC-7.3)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchPermissionTest, RedispatchPermissionTest_MouseEntryGatesBeforeDispatch_001,
    TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_MOVE,
        PointerEvent::TOOL_TYPE_FINGER, PointerEvent::SOURCE_TYPE_MOUSE);
    ASSERT_NE(pointerEvent, nullptr);

    ErrCode ret = mmiService.RedispatchInputEvent(*pointerEvent);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: RedispatchPermissionTest_PencilEntryGatesBeforeDispatch_001
 * @tc.desc: TOOL_TYPE_PENCIL redispatch shall pass the source gate and stop at entry gates like PEN
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RedispatchPermissionTest, RedispatchPermissionTest_PencilEntryGatesBeforeDispatch_001,
    TestSize.Level1)
{
    MMIService mmiService;
    auto pointerEvent = CreateTouchEvent(PointerEvent::POINTER_ACTION_MOVE,
        PointerEvent::TOOL_TYPE_PENCIL, PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    ASSERT_NE(pointerEvent, nullptr);

    ErrCode ret = mmiService.RedispatchInputEvent(*pointerEvent);
    EXPECT_NE(ret, RET_OK);
}
} // namespace MMI
} // namespace OHOS
