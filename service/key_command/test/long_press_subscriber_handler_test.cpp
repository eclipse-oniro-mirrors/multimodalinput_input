/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "ipc_skeleton.h"

#include "long_press_subscriber_handler.h"
#include "uds_session.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "LongPressSubscribeHandlerTest"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
const std::string PROGRAM_NAME = "LongPressSubscribeHandlerTest";
constexpr int32_t MODULE_TYPE = 1;
constexpr int32_t UDS_FD = -1;
constexpr int32_t UDS_UID = 100;
constexpr int32_t UDS_PID = 100;
} // namespace

class LongPressSubscribeHandlerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    std::shared_ptr<PointerEvent> SetupZeroFingerDownEvent();
    std::shared_ptr<PointerEvent> SetupSingleFingerDownEvent();
    std::shared_ptr<PointerEvent> SetupDoubleFingerDownEvent();
    std::shared_ptr<PointerEvent> SetupThreeFingerDownEvent();
};

std::shared_ptr<PointerEvent> LongPressSubscribeHandlerTest::SetupZeroFingerDownEvent()
{
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> LongPressSubscribeHandlerTest::SetupSingleFingerDownEvent()
{
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    int32_t downX = 100;
    int32_t downY = 200;
    item.SetDisplayX(downX);
    item.SetDisplayY(downY);
    item.SetPressed(true);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> LongPressSubscribeHandlerTest::SetupDoubleFingerDownEvent()
{
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    PointerEvent::PointerItem item;
    PointerEvent::PointerItem item2;
    item.SetPointerId(0);
    item.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    int32_t downX = 100;
    int32_t downY = 200;
    item.SetDisplayX(downX);
    item.SetDisplayY(downY);
    item.SetPressed(true);
    item.SetDownTime(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);

    item2.SetPointerId(1);
    item2.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    int32_t secondDownX = 120;
    int32_t secondDownY = 220;
    item2.SetDisplayX(secondDownX);
    item2.SetDisplayY(secondDownY);
    item2.SetPressed(true);
    int64_t downTime = 100000;
    item2.SetDownTime(downTime);
    pointerEvent->SetPointerId(1);
    pointerEvent->AddPointerItem(item2);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> LongPressSubscribeHandlerTest::SetupThreeFingerDownEvent()
{
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    PointerEvent::PointerItem item;
    PointerEvent::PointerItem item2;
    PointerEvent::PointerItem item3;
    item.SetPointerId(0);
    item.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    int32_t downX = 100;
    int32_t downY = 200;
    item.SetDisplayX(downX);
    item.SetDisplayY(downY);
    item.SetPressed(true);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);

    item2.SetPointerId(1);
    item2.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    int32_t secondDownX = 120;
    int32_t secondDownY = 220;
    item2.SetDisplayX(secondDownX);
    item2.SetDisplayY(secondDownY);
    item2.SetPressed(true);
    pointerEvent->SetPointerId(1);
    pointerEvent->AddPointerItem(item2);

    int32_t pointerId2 = 2;
    item3.SetPointerId(pointerId2);
    item3.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    int32_t thirdDownX = 140;
    int32_t thirdDownY = 240;
    item3.SetDisplayX(thirdDownX);
    item3.SetDisplayY(thirdDownY);
    item3.SetPressed(true);
    pointerEvent->SetPointerId(pointerId2);
    pointerEvent->AddPointerItem(item3);

    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    return pointerEvent;
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_SubscribeLongPressEvent_001
 * @tc.desc: Verify invalid subscribeId when subscribe long press event
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_SubscribeLongPressEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId = -1;
    LongPressRequest longPressRequest {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId, longPressRequest);
    EXPECT_TRUE(ret < 0);
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_SubscribeLongPressEvent_002
 * @tc.desc: Verify valid subscribeId when subscribe long press event
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_SubscribeLongPressEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId = 0;
    LongPressRequest longPressRequest {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId, longPressRequest);
    EXPECT_TRUE(ret >= 0);
    ret = LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId);
    EXPECT_TRUE(ret == RET_OK);
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_UnsubscribeLongPressEvent_001
 * @tc.desc: Verify invalid subscribeId when unsubscribe long press event
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_UnsubscribeLongPressEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId = -1;
    int32_t ret = LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId);
    EXPECT_TRUE(ret < 0);
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_UnsubscribeLongPressEvent_002
 * @tc.desc: Verify invalid subscribeId when unsubscribe long press event
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_UnsubscribeLongPressEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId = 0;
    LongPressRequest longPressRequest {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId, longPressRequest);
    EXPECT_TRUE(ret >= 0);
    subscribeId = -1;
    ret = LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId);
    EXPECT_TRUE(ret == RET_ERR);
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_UnsubscribeLongPressEvent_003
 * @tc.desc: Verify invalid sess when unsubscribe long press event
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_UnsubscribeLongPressEvent_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId = 0;
    LongPressRequest longPressRequest {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId, longPressRequest);
    EXPECT_TRUE(ret >= 0);
    sess = nullptr;
    ret = LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId);
    EXPECT_TRUE(ret == ERROR_NULL_POINTER);
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_UnsubscribeLongPressEvent_004
 * @tc.desc: Verify invalid sess and invalid subscribeId when unsubscribe long press event
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_UnsubscribeLongPressEvent_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId = 0;
    LongPressRequest longPressRequest {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId, longPressRequest);
    EXPECT_TRUE(ret >= 0);
    sess = nullptr;
    subscribeId = -1;
    ret = LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId);
    EXPECT_TRUE(ret == ERROR_NULL_POINTER);
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_UnsubscribeLongPressEvent_005
 * @tc.desc: Verify !subscribers.empty()
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_UnsubscribeLongPressEvent_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);

    int32_t subscribeId2 = 1;
    LongPressRequest longPressRequest2 {
        .fingerCount = 1,
        .duration = 900,
    };
    ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId2, longPressRequest2);
    EXPECT_TRUE(ret >= 0);

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId1));
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId2));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_AddDurationTimer_001
 * @tc.desc: Verify durationTimer.duration == duration
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_AddDurationTimer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);

    int32_t subscribeId2 = 1;
    LongPressRequest longPressRequest2 {
        .fingerCount = 2,
        .duration = 300,
    };
    ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId2, longPressRequest2);
    EXPECT_TRUE(ret >= 0);

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId1));
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId2));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_RemoveDurationTimer_001
 * @tc.desc: Verify durationTimer.duration == duration
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_RemoveDurationTimer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);

    int32_t subscribeId2 = 1;
    LongPressRequest longPressRequest2 {
        .fingerCount = 2,
        .duration = 300,
    };
    ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId2, longPressRequest2);
    EXPECT_TRUE(ret >= 0);

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId1));
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId2));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_RemoveDurationTimer_002
 * @tc.desc: Verify it->first.second == duration && !it->second.empty()
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_RemoveDurationTimer_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);

    int32_t subscribeId2 = 1;
    LongPressRequest longPressRequest2 {
        .fingerCount = 1,
        .duration = 300,
    };
    ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId2, longPressRequest2);
    EXPECT_TRUE(ret >= 0);

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId1));
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId2));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_RemoveDurationTimer_003
 * @tc.desc: Verify it->first.second != duration && !it->second.empty()
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_RemoveDurationTimer_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);

    int32_t subscribeId2 = 1;
    LongPressRequest longPressRequest2 {
        .fingerCount = 1,
        .duration = 900,
    };
    ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId2, longPressRequest2);
    EXPECT_TRUE(ret >= 0);

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId1));
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId2));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_OnSubscribeLongPressEvent_001
 * @tc.desc: Verify subscriberInfos_.find(pair) == subscriberInfos_.end()
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_OnSubscribeLongPressEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t fingerCount = 1;
    int32_t duration = 300;
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->OnSubscribeLongPressEvent(fingerCount, duration));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_OnSubscribeLongPressEvent_002
 * @tc.desc: Verify subscriberInfos_.find(pair) != subscriberInfos_.end()
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_OnSubscribeLongPressEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);
    int32_t fingerCount = 1;
    int32_t duration = 300;
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->OnSubscribeLongPressEvent(fingerCount, duration));
    
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId1));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_InsertSubScriber_001
 * @tc.desc: Verify subscriber->sess_ != nullptr && sub->id_ == subscriber->id_ && sub->sess_ == subscriber->sess
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_InsertSubScriber_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);

    int32_t subscribeId2 = 0;
    LongPressRequest longPressRequest2 {
        .fingerCount = 1,
        .duration = 900,
    };
    ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId2, longPressRequest2);
    EXPECT_TRUE(ret >= 0);

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId1));
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId2));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_InsertSubScriber_002
 * @tc.desc: Verify subscriber->sess_ != nullptr && sub->id_ == subscriber->id_ && sub->sess_ != subscriber->sess
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_InsertSubScriber_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess1 = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess1, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);

    SessionPtr sess2 = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId2 = 0;
    LongPressRequest longPressRequest2 {
        .fingerCount = 1,
        .duration = 900,
    };
    ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess2, subscribeId2, longPressRequest2);
    EXPECT_TRUE(ret >= 0);

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess1, subscribeId1));
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess2, subscribeId2));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_InsertSubScriber_003
 * @tc.desc: Verify subscriber->sess_ != nullptr && sub->id_ != subscriber->id_ && sub->sess_ != subscriber->sess
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_InsertSubScriber_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess1 = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess1, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);

    SessionPtr sess2 = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId2 = 1;
    LongPressRequest longPressRequest2 {
        .fingerCount = 1,
        .duration = 900,
    };
    ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess2, subscribeId2, longPressRequest2);
    EXPECT_TRUE(ret >= 0);

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess1, subscribeId1));
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess2, subscribeId2));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_OnSessionDelete_001
 * @tc.desc: Verify (*iter)->sess_ == sess and subscribers.empty()
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_OnSessionDelete_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess1 = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess1, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->OnSessionDelete(sess1));

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess1, subscribeId1));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_OnSessionDelete_002
 * @tc.desc: Verify (*iter)->sess_ != sess and !subscribers.empty()
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_OnSessionDelete_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr sess1 = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId1 = 0;
    LongPressRequest longPressRequest1 {
        .fingerCount = 1,
        .duration = 300,
    };
    int32_t ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess1, subscribeId1, longPressRequest1);
    EXPECT_TRUE(ret >= 0);

    SessionPtr sess2 = std::make_shared<UDSSession>("LongPressSubscribeHandlerTest", MODULE_TYPE, UDS_FD, UDS_UID,
        UDS_PID);
    int32_t subscribeId2 = 1;
    LongPressRequest longPressRequest2 {
        .fingerCount = 1,
        .duration = 300,
    };
    ret = LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess2, subscribeId2, longPressRequest2);
    EXPECT_TRUE(ret >= 0);

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->OnSessionDelete(sess2));
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->OnSessionDelete(sess1));

    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess1, subscribeId1));
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess2, subscribeId2));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_HandleFingerGestureDownEvent_001
 * @tc.desc: Verify if (fingerCount > 0 && fingerCount <= TwoFingerGesture::MAX_TOUCH_NUM)
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_HandleFingerGestureDownEvent_001,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = SetupZeroFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureDownEvent(pointerEvent));
    
    pointerEvent = SetupSingleFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureDownEvent(pointerEvent));

    pointerEvent = SetupDoubleFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureDownEvent(pointerEvent));

    pointerEvent = SetupThreeFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureDownEvent(pointerEvent));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_HandleFingerGestureDownEvent_002
 * @tc.desc: Verify if (pressTimeInterval > TWO_FINGERS_TIME_LIMIT)
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_HandleFingerGestureDownEvent_002,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = SetupDoubleFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    auto fingerCount = pointerEvent->GetPointerIds().size();
    ASSERT_TRUE(fingerCount == 2);
    PointerEvent::PointerItem item;
    int32_t pointerId = 1;
    bool result = pointerEvent->GetPointerItem(pointerId, item);
    ASSERT_TRUE(result);
    item.SetDownTime(item.GetDownTime() + 150000);
    pointerEvent->UpdatePointerItem(pointerId, item);
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureDownEvent(pointerEvent));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_HandleFingerGestureMoveEvent_001
 * @tc.desc: Verify if (isAllTimerClosed)
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_HandleFingerGestureMoveEvent_001,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = SetupThreeFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    LONG_PRESS_EVENT_HANDLER->isAllTimerClosed = true;
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureMoveEvent(pointerEvent));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_HandleFingerGestureMoveEvent_002
 * @tc.desc: Verify if (fingerCount > static_cast<size_t>(TWO_FINGER))
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_HandleFingerGestureMoveEvent_002,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = SetupThreeFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    LONG_PRESS_EVENT_HANDLER->isAllTimerClosed = false;
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureMoveEvent(pointerEvent));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_HandleFingerGestureMoveEvent_003
 * @tc.desc: Verify if (pos == std::end(fingerGesture_.touches))
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_HandleFingerGestureMoveEvent_003,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = SetupSingleFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureDownEvent(pointerEvent));
    
    PointerEvent::PointerItem item;
    int32_t pointerId = 0;
    bool result = pointerEvent->GetPointerItem(pointerId, item);
    ASSERT_TRUE(result);
    item.SetPointerId(item.GetPointerId() + 1);
    pointerEvent->UpdatePointerItem(pointerId, item);

    LONG_PRESS_EVENT_HANDLER->isAllTimerClosed = false;
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureMoveEvent(pointerEvent));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_HandleFingerGestureMoveEvent_004
 * @tc.desc: Verify if (pos == std::end(fingerGesture_.touches))
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_HandleFingerGestureMoveEvent_004,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = SetupSingleFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureDownEvent(pointerEvent));

    LONG_PRESS_EVENT_HANDLER->isAllTimerClosed = false;
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureMoveEvent(pointerEvent));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_HandleFingerGestureMoveEvent_005
 * @tc.desc: Verify if (moveDistance > TOUCH_MOVE_THRESHOLD)
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_HandleFingerGestureMoveEvent_005,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = SetupSingleFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureDownEvent(pointerEvent));

    PointerEvent::PointerItem item;
    int32_t pointerId = 0;
    bool result = pointerEvent->GetPointerItem(pointerId, item);
    ASSERT_TRUE(result);
    item.SetDisplayX(item.GetDisplayX() + 100);
    pointerEvent->UpdatePointerItem(pointerId, item);

    LONG_PRESS_EVENT_HANDLER->isAllTimerClosed = false;
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureMoveEvent(pointerEvent));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_HandleFingerGestureUpEvent_001
 * @tc.desc: Verify if (isAllTimerClosed)
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_HandleFingerGestureUpEvent_001,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = SetupSingleFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    LONG_PRESS_EVENT_HANDLER->isAllTimerClosed = true;
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureUpEvent(pointerEvent));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_HandleFingerGestureUpEvent_002
 * @tc.desc: Verify if (touchEvent->GetPointerIds().size() > static_cast<size_t>(TWO_FINGER))
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_HandleFingerGestureUpEvent_002,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = SetupThreeFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    LONG_PRESS_EVENT_HANDLER->isAllTimerClosed = false;
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureUpEvent(pointerEvent));
}

/**
 * @tc.name: LongPressSubscribeHandlerTest_HandleFingerGestureUpEvent_003
 * @tc.desc: Verify if (touchEvent->GetPointerIds().size() > static_cast<size_t>(TWO_FINGER))
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(LongPressSubscribeHandlerTest, LongPressSubscribeHandlerTest_HandleFingerGestureUpEvent_003,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = SetupDoubleFingerDownEvent();
    ASSERT_TRUE(pointerEvent != nullptr);
    LONG_PRESS_EVENT_HANDLER->isAllTimerClosed = false;
    ASSERT_NO_FATAL_FAILURE(LONG_PRESS_EVENT_HANDLER->HandleFingerGestureUpEvent(pointerEvent));
}
} // namespace MMI
} // namespace OHOS