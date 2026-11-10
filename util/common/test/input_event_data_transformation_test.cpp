/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "input_event_data_transformation.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
using namespace OHOS;
constexpr int64_t TEST_KEY_DOWN_TIME { 100 };
constexpr int64_t TEST_RAW_TIMESTAMP { 123456789 };
} // namespace

class InputEventDataTransformationTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
};

/**
 * @tc.name: UnmarshallingEnhanceData_001
 * @tc.desc: Test UnmarshallingEnhanceData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, UnmarshallingEnhanceData_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    NetPacket pkt(MmiMessageId::ON_KEY_EVENT);
    pkt << InputEventDataTransformation::MAX_HMAC_SIZE + 1;
    int32_t ret = InputEventDataTransformation::UnmarshallingEnhanceData(pkt, keyEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: UnmarshallingEnhanceData_002
 * @tc.desc: Test UnmarshallingEnhanceData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, UnmarshallingEnhanceData_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    NetPacket pkt(MmiMessageId::ON_KEY_EVENT);
    pkt << InputEventDataTransformation::MAX_HMAC_SIZE;
    int32_t ret = InputEventDataTransformation::UnmarshallingEnhanceData(pkt, keyEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: UnmarshallingEnhanceData_003
 * @tc.desc: Test UnmarshallingEnhanceData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, UnmarshallingEnhanceData_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    NetPacket pkt(MmiMessageId::ON_KEY_EVENT);
    pkt << 0;
    int32_t ret = InputEventDataTransformation::UnmarshallingEnhanceData(pkt, keyEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: MarshallingEnhanceData_001
 * @tc.desc: Test MarshallingEnhanceData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, MarshallingEnhanceData_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    NetPacket pkt(MmiMessageId::ON_KEY_EVENT);
    pkt << InputEventDataTransformation::MAX_HMAC_SIZE + 1;
    int32_t ret = InputEventDataTransformation::MarshallingEnhanceData(pointerEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: MarshallingEnhanceData_002
 * @tc.desc: Test MarshallingEnhanceData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, MarshallingEnhanceData_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    NetPacket pkt(MmiMessageId::ON_KEY_EVENT);
    pkt << 0;
    int32_t ret = InputEventDataTransformation::MarshallingEnhanceData(keyEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SwitchEventToNetPacket
 * @tc.desc: Test SwitchEventToNetPacket
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, SwitchEventToNetPacket, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<SwitchEvent> switchEvent = std::make_shared<SwitchEvent>(0);
    ASSERT_NE(switchEvent, nullptr);

    NetPacket pkt(MmiMessageId::ON_KEY_EVENT);
    pkt << 0;
    auto ret = InputEventDataTransformation::SwitchEventToNetPacket(switchEvent, pkt);
    ASSERT_EQ(ret, RET_OK);
    ret = InputEventDataTransformation::NetPacketToSwitchEvent(pkt, switchEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: LongPressEventToNetPacket
 * @tc.desc: Test LongPressEventToNetPacket
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, LongPressEventToNetPacket, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    LongPressEvent longPressEvent = {
    .fingerCount = 1,
    .duration = 1000,
    .pid = 1,
    .displayId = 0,
    .displayX = 100,
    .displayY = 200,
    .result = 0,
    .windowId = 123,
    .pointerId = 456,
    .downTime = 789,
    .bundleName = "com.example.bundle"
    };
    NetPacket pkt(MmiMessageId::ON_SUBSCRIBE_SWITCH);
    auto ret = InputEventDataTransformation::LongPressEventToNetPacket(longPressEvent, pkt);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: KeyEventToNetPacket
 * @tc.desc: Test key event serialization
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, KeyEventToNetPacket, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_A);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    keyEvent->SetRawCode(30);
    keyEvent->SetRepeatCount(2);

    KeyEvent::KeyItem item;
    item.SetKeyCode(KeyEvent::KEYCODE_A);
    item.SetDownTime(TEST_KEY_DOWN_TIME);
    item.SetPressed(true);
    item.SetRawCode(30);
    keyEvent->AddKeyItem(item);

    NetPacket pkt(MmiMessageId::ON_KEY_EVENT);
    ASSERT_EQ(InputEventDataTransformation::KeyEventToNetPacket(keyEvent, pkt), RET_OK);

    auto outEvent = KeyEvent::Create();
    ASSERT_NE(outEvent, nullptr);
    ASSERT_EQ(InputEventDataTransformation::NetPacketToKeyEvent(pkt, outEvent), RET_OK);
    EXPECT_EQ(outEvent->GetRawCode(), 30);
    EXPECT_EQ(outEvent->GetRepeatCount(), 2);
    auto outItem = outEvent->GetKeyItem(KeyEvent::KEYCODE_A);
    ASSERT_TRUE(outItem.has_value());
    EXPECT_EQ(outItem->GetRawCode(), 30);
}

/**
 * @tc.name: PointerEventTouchpadRawExt_001
 * @tc.desc: Test pointer event marshalling and unmarshalling round trip with the touchpad raw extension
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, PointerEventTouchpadRawExt_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto event = PointerEvent::Create();
    ASSERT_NE(event, nullptr);
    event->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHPAD);
    event->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    event->SetPointerId(1);
    event->SetActionTime(TEST_RAW_TIMESTAMP);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDisplayXPos(10.5);
    item.SetDisplayYPos(20.5);
    item.SetPressure(0.8);
    item.SetWindowXPos(100.5);
    item.SetWindowYPos(200.5);
    event->AddPointerItem(item);
    event->SetPressedKeys({ KeyEvent::KEYCODE_CTRL_LEFT });

    PointerEvent::TouchpadRawExt ext;
    ext.SetDeviceWidth(3200.0);
    ext.SetDeviceHeight(2000.0);
    ext.SetSeatSlot(1);
    PointerEvent::TouchpadRawPoint firstPoint;
    firstPoint.SetSeatSlot(1);
    firstPoint.SetX(10.5);
    firstPoint.SetY(20.5);
    PointerEvent::TouchpadRawPoint secondPoint;
    secondPoint.SetSeatSlot(2);
    secondPoint.SetX(100.5);
    secondPoint.SetY(200.5);
    std::vector<PointerEvent::TouchpadRawPoint> rawPoints;
    rawPoints.push_back(firstPoint);
    rawPoints.push_back(secondPoint);
    ext.SetRawPoints(rawPoints);
    event->SetTouchpadRawExt(ext);

    NetPacket pkt(MmiMessageId::ON_POINTER_EVENT);
    ASSERT_EQ(InputEventDataTransformation::Marshalling(event, pkt), RET_OK);
    auto outEvent = PointerEvent::Create();
    ASSERT_NE(outEvent, nullptr);
    ASSERT_EQ(InputEventDataTransformation::Unmarshalling(pkt, outEvent), RET_OK);

    EXPECT_EQ(outEvent->GetSourceType(), PointerEvent::SOURCE_TYPE_TOUCHPAD);
    EXPECT_EQ(outEvent->GetPointerAction(), PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_EQ(outEvent->GetPressedKeys(), event->GetPressedKeys());
    EXPECT_TRUE(outEvent->HasTouchpadRawExt());
    const auto &outExt = outEvent->GetTouchpadRawExt();
    EXPECT_EQ(outExt.GetDeviceWidth(), ext.GetDeviceWidth());
    EXPECT_EQ(outExt.GetDeviceHeight(), ext.GetDeviceHeight());
    EXPECT_EQ(outExt.GetSeatSlot(), ext.GetSeatSlot());
    const auto &outPoints = outExt.GetRawPoints();
    ASSERT_EQ(outPoints.size(), rawPoints.size());
    EXPECT_EQ(outPoints[0].GetSeatSlot(), rawPoints[0].GetSeatSlot());
    EXPECT_EQ(outPoints[0].GetX(), rawPoints[0].GetX());
    EXPECT_EQ(outPoints[0].GetY(), rawPoints[0].GetY());
    EXPECT_EQ(outPoints[1].GetSeatSlot(), rawPoints[1].GetSeatSlot());
    EXPECT_EQ(outPoints[1].GetX(), rawPoints[1].GetX());
    EXPECT_EQ(outPoints[1].GetY(), rawPoints[1].GetY());
}

/**
 * @tc.name: PointerEventTouchpadRawExt_002
 * @tc.desc: Test pointer event round trip without the touchpad raw extension
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, PointerEventTouchpadRawExt_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto event = PointerEvent::Create();
    ASSERT_NE(event, nullptr);
    event->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    event->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    event->SetPointerId(0);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDisplayXPos(30.5);
    item.SetDisplayYPos(40.5);
    event->AddPointerItem(item);

    NetPacket pkt(MmiMessageId::ON_POINTER_EVENT);
    ASSERT_EQ(InputEventDataTransformation::Marshalling(event, pkt), RET_OK);
    auto outEvent = PointerEvent::Create();
    ASSERT_NE(outEvent, nullptr);
    ASSERT_EQ(InputEventDataTransformation::Unmarshalling(pkt, outEvent), RET_OK);
    EXPECT_FALSE(outEvent->HasTouchpadRawExt());
}

/**
 * @tc.name: PointerEventRedispatchCountRoundTrip
 * @tc.desc: redispatchCount survives a NetPacket Marshalling/Unmarshalling round-trip
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventDataTransformationTest, PointerEventRedispatchCountRoundTrip, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    pointerEvent->SetRedispatchCount(2);

    NetPacket pkt(MmiMessageId::ON_POINTER_EVENT);
    ASSERT_EQ(InputEventDataTransformation::Marshalling(pointerEvent, pkt), RET_OK);

    auto outEvent = PointerEvent::Create();
    ASSERT_NE(outEvent, nullptr);
    ASSERT_EQ(InputEventDataTransformation::Unmarshalling(pkt, outEvent), RET_OK);
    EXPECT_EQ(outEvent->GetRedispatchCount(), 2);
}

} // namespace MMI
} // namespace OHOS
