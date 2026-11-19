/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <cstdint>

#include "oh_input_manager.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;

// An opaque handle address never registered by OH_Input_EventHandler_Create, covering the "unregistered handler" branch.
constexpr uintptr_t FAKE_HANDLER_ADDR { 0x1 };
// The mask only allows the KEY/MOUSE/TOUCH/AXIS bits; bit 5 is invalid.
constexpr OH_Input_EventTypeMask INVALID_MASK { 1ULL << 4 };
constexpr OH_Input_EventTypeMask KEY_MASK { OH_INPUT_EVENT_TYPE_MASK_KEY };
constexpr OH_Input_EventTypeMask POINTER_MASK { OH_INPUT_EVENT_TYPE_MASK_MOUSE | OH_INPUT_EVENT_TYPE_MASK_TOUCH };
constexpr OH_Input_EventTypeMask ALL_MASK { OH_INPUT_EVENT_TYPE_MASK_KEY | OH_INPUT_EVENT_TYPE_MASK_MOUSE |
    OH_INPUT_EVENT_TYPE_MASK_TOUCH | OH_INPUT_EVENT_TYPE_MASK_AXIS };

constexpr OH_Input_EventType UNSET_EVENT_TYPE { static_cast<OH_Input_EventType>(-1) };

// The callback returns non-NULL (the original event) to pass the event through; this test binary covers
// only pure local paths and the callback is never actually invoked.
const OH_Input_Event* PassThroughOnEvent(OH_Input_EventHandler*, const OH_Input_Event* event)
{
    return event;
}

void OnDisabled(OH_Input_EventHandler*) {}

OH_Input_EventHandler* FakeHandler()
{
    return reinterpret_cast<OH_Input_EventHandler*>(FAKE_HANDLER_ADDR);
}

const OH_Input_Event* FakeEvent()
{
    return reinterpret_cast<const OH_Input_Event*>(FAKE_HANDLER_ADDR);
}
} // namespace

class InputNativeEventHandlerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: InputNativeEventHandlerTest_Create_001
 * @tc.desc: OH_Input_EventHandler_Create returns INPUT_PARAMETER_ERROR when handler is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Create_001, TestSize.Level1)
{
    Input_Result ret = OH_Input_EventHandler_Create(KEY_MASK, PassThroughOnEvent, OnDisabled, nullptr);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Create_002
 * @tc.desc: OH_Input_EventHandler_Create returns INPUT_PARAMETER_ERROR when onEventFunc is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Create_002, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    Input_Result ret = OH_Input_EventHandler_Create(KEY_MASK, nullptr, OnDisabled, &handler);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Create_003
 * @tc.desc: OH_Input_EventHandler_Create returns INPUT_PARAMETER_ERROR when onDisabledFunc is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Create_003, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    Input_Result ret = OH_Input_EventHandler_Create(KEY_MASK, PassThroughOnEvent, nullptr, &handler);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Create_004
 * @tc.desc: OH_Input_EventHandler_Create returns INPUT_PARAMETER_ERROR when mask is zero
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Create_004, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    Input_Result ret = OH_Input_EventHandler_Create(0, PassThroughOnEvent, OnDisabled, &handler);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Create_005
 * @tc.desc: OH_Input_EventHandler_Create returns INPUT_PARAMETER_ERROR when mask contains unsupported bits
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Create_005, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    Input_Result ret = OH_Input_EventHandler_Create(INVALID_MASK, PassThroughOnEvent, OnDisabled, &handler);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Create_006
 * @tc.desc: OH_Input_EventHandler_Create succeeds with a single supported mask bit
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Create_006, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    Input_Result ret = OH_Input_EventHandler_Create(KEY_MASK, PassThroughOnEvent, OnDisabled, &handler);
    EXPECT_EQ(ret, INPUT_SUCCESS);
    ASSERT_NE(handler, nullptr);
    OH_Input_EventHandler_Destroy(handler);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Create_007
 * @tc.desc: OH_Input_EventHandler_Create succeeds with a combined supported mask
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Create_007, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    Input_Result ret = OH_Input_EventHandler_Create(POINTER_MASK, PassThroughOnEvent, OnDisabled, &handler);
    EXPECT_EQ(ret, INPUT_SUCCESS);
    ASSERT_NE(handler, nullptr);
    OH_Input_EventHandler_Destroy(handler);
}

/**
 * @tc.name: InputNativeEventHandlerTest_GetMask_001
 * @tc.desc: OH_Input_EventHandler_GetMask returns INPUT_PARAMETER_ERROR when handler is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_GetMask_001, TestSize.Level1)
{
    OH_Input_EventTypeMask mask { 0 };
    Input_Result ret = OH_Input_EventHandler_GetMask(nullptr, &mask);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_GetMask_002
 * @tc.desc: OH_Input_EventHandler_GetMask returns INPUT_PARAMETER_ERROR when mask is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_GetMask_002, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    Input_Result createRet = OH_Input_EventHandler_Create(KEY_MASK, PassThroughOnEvent, OnDisabled, &handler);
    ASSERT_EQ(createRet, INPUT_SUCCESS);
    Input_Result ret = OH_Input_EventHandler_GetMask(handler, nullptr);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
    OH_Input_EventHandler_Destroy(handler);
}

/**
 * @tc.name: InputNativeEventHandlerTest_GetMask_003
 * @tc.desc: OH_Input_EventHandler_GetMask returns INPUT_PARAMETER_ERROR for an unregistered handler
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_GetMask_003, TestSize.Level1)
{
    OH_Input_EventTypeMask mask { 0 };
    Input_Result ret = OH_Input_EventHandler_GetMask(FakeHandler(), &mask);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_GetMask_004
 * @tc.desc: OH_Input_EventHandler_GetMask returns the mask passed to Create
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_GetMask_004, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    ASSERT_EQ(OH_Input_EventHandler_Create(KEY_MASK, PassThroughOnEvent, OnDisabled, &handler), INPUT_SUCCESS);

    OH_Input_EventTypeMask mask { 0 };
    Input_Result ret = OH_Input_EventHandler_GetMask(handler, &mask);
    EXPECT_EQ(ret, INPUT_SUCCESS);
    EXPECT_EQ(mask, KEY_MASK);
    OH_Input_EventHandler_Destroy(handler);
}

/**
 * @tc.name: InputNativeEventHandlerTest_GetMask_005
 * @tc.desc: OH_Input_EventHandler_GetMask returns the combined mask passed to Create
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_GetMask_005, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    ASSERT_EQ(OH_Input_EventHandler_Create(ALL_MASK, PassThroughOnEvent, OnDisabled, &handler), INPUT_SUCCESS);

    OH_Input_EventTypeMask mask { 0 };
    Input_Result ret = OH_Input_EventHandler_GetMask(handler, &mask);
    EXPECT_EQ(ret, INPUT_SUCCESS);
    EXPECT_EQ(mask, ALL_MASK);
    OH_Input_EventHandler_Destroy(handler);
}

/**
 * @tc.name: InputNativeEventHandlerTest_IsEnabled_001
 * @tc.desc: OH_Input_EventHandler_IsEnabled returns INPUT_PARAMETER_ERROR when handler is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_IsEnabled_001, TestSize.Level1)
{
    bool enabled { false };
    Input_Result ret = OH_Input_EventHandler_IsEnabled(nullptr, &enabled);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_IsEnabled_002
 * @tc.desc: OH_Input_EventHandler_IsEnabled returns INPUT_PARAMETER_ERROR when enabled is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_IsEnabled_002, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    ASSERT_EQ(OH_Input_EventHandler_Create(KEY_MASK, PassThroughOnEvent, OnDisabled, &handler), INPUT_SUCCESS);
    Input_Result ret = OH_Input_EventHandler_IsEnabled(handler, nullptr);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
    OH_Input_EventHandler_Destroy(handler);
}

/**
 * @tc.name: InputNativeEventHandlerTest_IsEnabled_003
 * @tc.desc: OH_Input_EventHandler_IsEnabled returns INPUT_PARAMETER_ERROR for an unregistered handler
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_IsEnabled_003, TestSize.Level1)
{
    bool enabled { false };
    Input_Result ret = OH_Input_EventHandler_IsEnabled(FakeHandler(), &enabled);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_IsEnabled_004
 * @tc.desc: A newly created handler is disabled by default
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_IsEnabled_004, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    ASSERT_EQ(OH_Input_EventHandler_Create(KEY_MASK, PassThroughOnEvent, OnDisabled, &handler), INPUT_SUCCESS);

    bool enabled { true };
    Input_Result ret = OH_Input_EventHandler_IsEnabled(handler, &enabled);
    EXPECT_EQ(ret, INPUT_SUCCESS);
    EXPECT_FALSE(enabled);
    OH_Input_EventHandler_Destroy(handler);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Enable_001
 * @tc.desc: OH_Input_EventHandler_Enable returns INPUT_PARAMETER_ERROR when handler is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Enable_001, TestSize.Level1)
{
    Input_Result ret = OH_Input_EventHandler_Enable(nullptr, true);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Enable_002
 * @tc.desc: OH_Input_EventHandler_Enable returns INPUT_PARAMETER_ERROR for an unregistered handler
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Enable_002, TestSize.Level1)
{
    Input_Result ret = OH_Input_EventHandler_Enable(FakeHandler(), true);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Enable_003
 * @tc.desc: Disabling an already-disabled handler succeeds without side effects
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Enable_003, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    ASSERT_EQ(OH_Input_EventHandler_Create(KEY_MASK, PassThroughOnEvent, OnDisabled, &handler), INPUT_SUCCESS);

    Input_Result ret = OH_Input_EventHandler_Enable(handler, false);
    EXPECT_EQ(ret, INPUT_SUCCESS);

    bool enabled { true };
    ASSERT_EQ(OH_Input_EventHandler_IsEnabled(handler, &enabled), INPUT_SUCCESS);
    EXPECT_FALSE(enabled);
    OH_Input_EventHandler_Destroy(handler);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Destroy_001
 * @tc.desc: OH_Input_EventHandler_Destroy is a no-op when handler is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Destroy_001, TestSize.Level1)
{
    OH_Input_EventHandler_Destroy(nullptr);
}

/**
 * @tc.name: InputNativeEventHandlerTest_Destroy_002
 * @tc.desc: Destroying a created handler twice does not crash and unregisters it
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_Destroy_002, TestSize.Level1)
{
    OH_Input_EventHandler* handler { nullptr };
    ASSERT_EQ(OH_Input_EventHandler_Create(KEY_MASK, PassThroughOnEvent, OnDisabled, &handler), INPUT_SUCCESS);

    OH_Input_EventHandler_Destroy(handler);
    OH_Input_EventHandler_Destroy(handler);

    bool enabled { false };
    EXPECT_EQ(OH_Input_EventHandler_IsEnabled(handler, &enabled), INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_GetType_001
 * @tc.desc: OH_Input_Event_GetType returns INPUT_PARAMETER_ERROR when event is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_GetType_001, TestSize.Level1)
{
    OH_Input_EventType type { UNSET_EVENT_TYPE };
    Input_Result ret = OH_Input_Event_GetType(nullptr, &type);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_GetType_002
 * @tc.desc: OH_Input_Event_GetType returns INPUT_PARAMETER_ERROR when type is null
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_GetType_002, TestSize.Level1)
{
    Input_Result ret = OH_Input_Event_GetType(FakeEvent(), nullptr);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_AsKeyEvent_001
 * @tc.desc: OH_Input_Event_AsKeyEvent returns INPUT_PARAMETER_ERROR on null arguments
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_AsKeyEvent_001, TestSize.Level1)
{
    const Input_KeyEvent* keyEvent { nullptr };
    EXPECT_EQ(OH_Input_Event_AsKeyEvent(nullptr, &keyEvent), INPUT_PARAMETER_ERROR);
    EXPECT_EQ(OH_Input_Event_AsKeyEvent(FakeEvent(), nullptr), INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_AsMouseEvent_001
 * @tc.desc: OH_Input_Event_AsMouseEvent returns INPUT_PARAMETER_ERROR on null arguments
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_AsMouseEvent_001, TestSize.Level1)
{
    const Input_MouseEvent* mouseEvent { nullptr };
    EXPECT_EQ(OH_Input_Event_AsMouseEvent(nullptr, &mouseEvent), INPUT_PARAMETER_ERROR);
    EXPECT_EQ(OH_Input_Event_AsMouseEvent(FakeEvent(), nullptr), INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_AsTouchEvent_001
 * @tc.desc: OH_Input_Event_AsTouchEvent returns INPUT_PARAMETER_ERROR on null arguments
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_AsTouchEvent_001, TestSize.Level1)
{
    const Input_TouchEvent* touchEvent { nullptr };
    EXPECT_EQ(OH_Input_Event_AsTouchEvent(nullptr, &touchEvent), INPUT_PARAMETER_ERROR);
    EXPECT_EQ(OH_Input_Event_AsTouchEvent(FakeEvent(), nullptr), INPUT_PARAMETER_ERROR);
}

/**
 * @tc.name: InputNativeEventHandlerTest_AsAxisEvent_001
 * @tc.desc: OH_Input_Event_AsAxisEvent returns INPUT_PARAMETER_ERROR on null arguments
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputNativeEventHandlerTest, InputNativeEventHandlerTest_AsAxisEvent_001, TestSize.Level1)
{
    const Input_AxisEvent* axisEvent { nullptr };
    EXPECT_EQ(OH_Input_Event_AsAxisEvent(nullptr, &axisEvent), INPUT_PARAMETER_ERROR);
    EXPECT_EQ(OH_Input_Event_AsAxisEvent(FakeEvent(), nullptr), INPUT_PARAMETER_ERROR);
}
} // namespace MMI
} // namespace OHOS
