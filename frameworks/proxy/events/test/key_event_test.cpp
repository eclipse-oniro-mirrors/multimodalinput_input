/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "define_multimodal.h"
#include "event_util_test.h"
#include "input_manager.h"
#include "key_event.h"
#include "mmi_log.h"
#include "proto.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "KeyEventTest"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
} // namespace

class KeyEventTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
};

/**
 * @tc.name: KeyEventTest_OnCheckKeyEvent_001
 * @tc.desc: Verify key event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_OnCheckKeyEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto KeyEvent = KeyEvent::Create();
    ASSERT_NE(KeyEvent, nullptr);
    KeyEvent->SetKeyCode(KeyEvent::KEYCODE_UNKNOWN);
    ASSERT_TRUE(!KeyEvent->IsValid());

    KeyEvent->SetKeyCode(KeyEvent::KEYCODE_HOME);
    KeyEvent->SetActionTime(0);
    ASSERT_TRUE(!KeyEvent->IsValid());

    KeyEvent->SetKeyCode(KeyEvent::KEYCODE_HOME);
    KeyEvent->SetActionTime(100);
    KeyEvent->SetKeyAction(KeyEvent::KEY_ACTION_UNKNOWN);
    ASSERT_TRUE(!KeyEvent->IsValid());
}

/**
 * @tc.name: KeyEventTest_OnCheckKeyEvent_002
 * @tc.desc: Verify key event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_OnCheckKeyEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto KeyEvent1 = KeyEvent::Create();
    ASSERT_NE(KeyEvent1, nullptr);
    KeyEvent1->SetKeyCode(KeyEvent::KEYCODE_HOME);
    KeyEvent1->SetActionTime(100);
    KeyEvent1->SetKeyAction(KeyEvent::KEY_ACTION_CANCEL);
    KeyEvent::KeyItem item;
    item.SetKeyCode(KeyEvent::KEYCODE_UNKNOWN);
    KeyEvent1->AddKeyItem(item);
    ASSERT_TRUE(!KeyEvent1->IsValid());

    auto KeyEvent2 = KeyEvent::Create();
    ASSERT_NE(KeyEvent2, nullptr);
    KeyEvent2->SetKeyCode(KeyEvent::KEYCODE_HOME);
    KeyEvent2->SetActionTime(100);
    KeyEvent2->SetKeyAction(KeyEvent::KEY_ACTION_CANCEL);
    item.SetKeyCode(KeyEvent::KEYCODE_HOME);
    item.SetDownTime(0);
    KeyEvent2->AddKeyItem(item);
    ASSERT_TRUE(!KeyEvent2->IsValid());
}

/**
 * @tc.name: KeyEventTest_OnCheckKeyEvent_003
 * @tc.desc: Verify key event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_OnCheckKeyEvent_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto KeyEvent1 = KeyEvent::Create();
    ASSERT_NE(KeyEvent1, nullptr);
    KeyEvent1->SetKeyCode(KeyEvent::KEYCODE_HOME);
    KeyEvent1->SetActionTime(100);
    KeyEvent1->SetKeyAction(KeyEvent::KEY_ACTION_CANCEL);
    KeyEvent::KeyItem item;
    item.SetKeyCode(KeyEvent::KEYCODE_HOME);
    item.SetDownTime(100);
    item.SetPressed(false);
    KeyEvent1->AddKeyItem(item);
    ASSERT_TRUE(!KeyEvent1->IsValid());

    auto KeyEvent2 = KeyEvent::Create();
    ASSERT_NE(KeyEvent2, nullptr);
    KeyEvent2->SetKeyCode(KeyEvent::KEYCODE_HOME);
    KeyEvent2->SetActionTime(100);
    KeyEvent2->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    item.SetKeyCode(KeyEvent::KEYCODE_BACK);
    item.SetDownTime(100);
    item.SetPressed(false);
    KeyEvent2->AddKeyItem(item);
    ASSERT_TRUE(!KeyEvent2->IsValid());
}

/**
 * @tc.name: KeyEventTest_OnCheckKeyEvent_004
 * @tc.desc: Verify key event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_OnCheckKeyEvent_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto KeyEvent1 = KeyEvent::Create();
    ASSERT_NE(KeyEvent1, nullptr);
    KeyEvent1->SetKeyCode(KeyEvent::KEYCODE_HOME);
    KeyEvent1->SetActionTime(100);
    KeyEvent1->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    KeyEvent::KeyItem item1;
    item1.SetKeyCode(KeyEvent::KEYCODE_HOME);
    item1.SetDownTime(100);
    item1.SetPressed(false);
    KeyEvent1->AddKeyItem(item1);
    KeyEvent::KeyItem item2;
    item2.SetKeyCode(KeyEvent::KEYCODE_HOME);
    item2.SetDownTime(100);
    item2.SetPressed(false);
    KeyEvent1->AddKeyItem(item2);
    ASSERT_TRUE(!KeyEvent1->IsValid());

    auto KeyEvent2 = KeyEvent::Create();
    ASSERT_NE(KeyEvent2, nullptr);
    KeyEvent2->SetKeyCode(KeyEvent::KEYCODE_HOME);
    KeyEvent2->SetActionTime(100);
    KeyEvent2->SetKeyAction(KeyEvent::KEY_ACTION_CANCEL);
    ASSERT_TRUE(!KeyEvent2->IsValid());
}

/**
 * @tc.name: KeyEventTest_OnCheckKeyEvent_005
 * @tc.desc: Verify key event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_OnCheckKeyEvent_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto KeyEvent1 = KeyEvent::Create();
    ASSERT_NE(KeyEvent1, nullptr);
    KeyEvent1->SetKeyCode(KeyEvent::KEYCODE_HOME);
    KeyEvent1->SetActionTime(100);
    KeyEvent1->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    KeyEvent::KeyItem item1;
    item1.SetKeyCode(KeyEvent::KEYCODE_HOME);
    item1.SetDownTime(100);
    item1.SetPressed(false);
    KeyEvent1->AddKeyItem(item1);
    KeyEvent::KeyItem item2;
    item2.SetKeyCode(KeyEvent::KEYCODE_BACK);
    item2.SetDownTime(100);
    item2.SetPressed(true);
    KeyEvent1->AddKeyItem(item2);
    ASSERT_TRUE(KeyEvent1->IsValid());

    auto KeyEvent2 = KeyEvent::Create();
    ASSERT_NE(KeyEvent2, nullptr);
    KeyEvent2->SetKeyCode(KeyEvent::KEYCODE_HOME);
    KeyEvent2->SetActionTime(100);
    KeyEvent2->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    item1.SetKeyCode(KeyEvent::KEYCODE_HOME);
    item1.SetDownTime(100);
    item1.SetPressed(true);
    KeyEvent2->AddKeyItem(item1);
    ASSERT_TRUE(KeyEvent2->IsValid());
}

/**
 * @tc.name: KeyEventTest_OnCheckKeyEvent_006
 * @tc.desc: Verify key event
 * @tc.type: FUNC
 * @tc.require: I5QSN3
 */
HWTEST_F(KeyEventTest, KeyEventTest_OnCheckKeyEvent_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto inputEvent = InputEvent::Create();
    ASSERT_NE(inputEvent, nullptr);
    auto event1 = KeyEvent::from(inputEvent);
    ASSERT_EQ(event1, nullptr);
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    auto event2 = KeyEvent::Clone(keyEvent);
    ASSERT_NE(event2, nullptr);
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_BACK);
    keyEvent->SetActionTime(100);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);

    InputManager::GetInstance()->SimulateInputEvent(keyEvent);
    keyEvent->ActionToString(KeyEvent::KEY_ACTION_DOWN);
    keyEvent->KeyCodeToString(KeyEvent::KEYCODE_BACK);
    KeyEvent::KeyItem item;
    item.SetKeyCode(KeyEvent::KEYCODE_BACK);
    item.SetDownTime(100);
    item.SetPressed(true);
    item.SetUnicode(0);
    keyEvent->AddKeyItem(item);
    ASSERT_TRUE(keyEvent->IsValid());
    std::vector<KeyEvent::KeyItem> items = keyEvent->GetKeyItems();
    TestUtil->DumpInputEvent(keyEvent);
}

/**
 * @tc.name: KeyEventTest_GetFunctionKey_001
 * @tc.desc: Set Numlock for keyevent to false
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_GetFunctionKey_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetFunctionKey(KeyEvent::NUM_LOCK_FUNCTION_KEY, false);
    bool result = keyEvent->GetFunctionKey(KeyEvent::NUM_LOCK_FUNCTION_KEY);
    ASSERT_FALSE(result);
}

/**
 * @tc.name: KeyEventTest_GetFunctionKey_002
 * @tc.desc: Set Numlock for keyevent to true
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_GetFunctionKey_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetFunctionKey(KeyEvent::NUM_LOCK_FUNCTION_KEY, true);
    bool result = keyEvent->GetFunctionKey(KeyEvent::NUM_LOCK_FUNCTION_KEY);
    ASSERT_TRUE(result);
}

/**
 * @tc.name: KeyEventTest_GetFunctionKey_003
 * @tc.desc: Set Capslock for keyevent to false
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_GetFunctionKey_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetFunctionKey(KeyEvent::CAPS_LOCK_FUNCTION_KEY, false);
    bool result = keyEvent->GetFunctionKey(KeyEvent::CAPS_LOCK_FUNCTION_KEY);
    ASSERT_FALSE(result);
}

/**
 * @tc.name: KeyEventTest_GetFunctionKey_004
 * @tc.desc: Set Capslock for keyevent to true
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_GetFunctionKey_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetFunctionKey(KeyEvent::CAPS_LOCK_FUNCTION_KEY, true);
    bool result = keyEvent->GetFunctionKey(KeyEvent::CAPS_LOCK_FUNCTION_KEY);
    ASSERT_TRUE(result);
}

/**
 * @tc.name: KeyEventTest_GetKeyIntention_001
 * @tc.desc: GetKey intention
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_GetKeyIntention_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    int32_t result = keyEvent->GetKeyIntention();
    ASSERT_EQ(result, -1);
}

/**
 * @tc.name: KeyEventTest_GetFunctionKey_005
 * @tc.desc: Set Scrolllock for keyevent to false
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_GetFunctionKey_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetFunctionKey(KeyEvent::SCROLL_LOCK_FUNCTION_KEY, false);
    bool result = keyEvent->GetFunctionKey(KeyEvent::SCROLL_LOCK_FUNCTION_KEY);
    ASSERT_FALSE(result);
}

/**
 * @tc.name: KeyEventTest_GetFunctionKey_006
 * @tc.desc: Set Scrolllock for keyevent to true
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_GetFunctionKey_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetFunctionKey(KeyEvent::SCROLL_LOCK_FUNCTION_KEY, true);
    bool result = keyEvent->GetFunctionKey(KeyEvent::SCROLL_LOCK_FUNCTION_KEY);
    ASSERT_TRUE(result);
}

/**
 * @tc.name: KeyEventTest_TransitionFunctionKey_001
 * @tc.desc: Transition keycode to function key
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_TransitionFunctionKey_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    int32_t lockCode = keyEvent->TransitionFunctionKey(KeyEvent::KEYCODE_NUM_LOCK);
    ASSERT_EQ(lockCode, KeyEvent::NUM_LOCK_FUNCTION_KEY);
}

/**
 * @tc.name: KeyEventTest_TransitionFunctionKey_002
 * @tc.desc: Transition keycode to function key
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_TransitionFunctionKey_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    int32_t lockCode = keyEvent->TransitionFunctionKey(KeyEvent::KEYCODE_SCROLL_LOCK);
    ASSERT_EQ(lockCode, KeyEvent::SCROLL_LOCK_FUNCTION_KEY);
}

/**
 * @tc.name: KeyEventTest_TransitionFunctionKey_003
 * @tc.desc: Transition keycode to function key
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_TransitionFunctionKey_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    int32_t lockCode = keyEvent->TransitionFunctionKey(KeyEvent::KEYCODE_CAPS_LOCK);
    ASSERT_EQ(lockCode, KeyEvent::CAPS_LOCK_FUNCTION_KEY);
}

/**
 * @tc.name: KeyEventTest_TransitionFunctionKey_004
 * @tc.desc: Transition not support keycode to function key
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_TransitionFunctionKey_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    int32_t lockCode = keyEvent->TransitionFunctionKey(KeyEvent::KEYCODE_A);
    ASSERT_EQ(lockCode, KeyEvent::UNKNOWN_FUNCTION_KEY);
}

/**
 * @tc.name: KeyEventTest_ReadFromParcel_001
 * @tc.desc: Read from parcel
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_ReadFromParcel_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_HOME);
    keyEvent->SetActionTime(100);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    keyEvent->ActionToString(KeyEvent::KEY_ACTION_DOWN);
    keyEvent->KeyCodeToString(KeyEvent::KEYCODE_HOME);
    KeyEvent::KeyItem item;
    item.SetKeyCode(KeyEvent::KEYCODE_HOME);
    item.SetDownTime(100);
    item.SetPressed(true);
    keyEvent->AddKeyItem(item);
    MessageParcel data;
    bool ret = keyEvent->WriteToParcel(data);
    ASSERT_TRUE(ret);
    ret = keyEvent->ReadFromParcel(data);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: KeyEventTest_ReadFromParcel_002
 * @tc.desc: Read from parcel
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_ReadFromParcel_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetKeyCode(KeyEvent::KEYCODE_HOME);
    keyEvent->SetActionTime(100);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    keyEvent->ActionToString(KeyEvent::KEY_ACTION_DOWN);
    keyEvent->KeyCodeToString(KeyEvent::KEYCODE_HOME);
    KeyEvent::KeyItem item;
    item.SetKeyCode(KeyEvent::KEYCODE_HOME);
    item.SetDownTime(100);
    item.SetPressed(true);
    keyEvent->AddKeyItem(item);
    MessageParcel data;
    bool ret = keyEvent->WriteToParcel(data);
    ASSERT_TRUE(ret);
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    ret = inputEvent->ReadFromParcel(data);
    ASSERT_TRUE(ret);
    int32_t keyCode;
    ret = data.ReadInt32(keyCode);
    ASSERT_TRUE(ret);
    const int32_t keysSize = data.ReadInt32();
    ASSERT_FALSE(keysSize < 0);
    for (int32_t i = 0; i < keysSize; ++i) {
        KeyEvent::KeyItem keyItem = {};
        ret = keyItem.ReadFromParcel(data);
        ASSERT_TRUE(ret);
    }
}

/**
 * @tc.name: KeyEventTest_ReadFromParcel_003
 * @tc.desc: Verify keyoption read from parcel
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_ReadFromParcel_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::set<int32_t> preKeys;
    std::shared_ptr<KeyOption> keyOption = std::make_shared<KeyOption>();
    keyOption->SetPreKeys(preKeys);
    MessageParcel data;
    bool ret = keyOption->ReadFromParcel(data);
    ASSERT_FALSE(ret);
    preKeys.insert(0);
    preKeys.insert(1);
    keyOption->SetPreKeys(preKeys);
    keyOption->SetFinalKey(0);
    keyOption->SetFinalKeyDown(0);
    keyOption->SetFinalKeyDownDuration(0);
    keyOption->SetFinalKeyUpDelay(0);
    keyOption->WriteToParcel(data);
    ret = keyOption->ReadFromParcel(data);
    ASSERT_TRUE(ret);
}

#ifdef OHOS_BUILD_ENABLE_SECURITY_COMPONENT
/**
 * @tc.name: KeyEventTest_SetEnhanceData_001
 * @tc.desc: Set the enhance data.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_SetEnhanceData_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto KeyEvent = KeyEvent::Create();
    ASSERT_NE(KeyEvent, nullptr);
    uint32_t enHanceDataLen = 3;
    uint8_t enhanceDataBuf[enHanceDataLen];
    std::vector<uint8_t> enhanceData;
    for (uint32_t i = 0; i < enHanceDataLen; i++) {
        enhanceData.push_back(enhanceDataBuf[i]);
    }

    ASSERT_NO_FATAL_FAILURE(KeyEvent->SetEnhanceData(enhanceData));
    ASSERT_EQ(KeyEvent->GetEnhanceData(), enhanceData);
}
#endif // OHOS_BUILD_ENABLE_SECURITY_COMPONENT

/**
 * @tc.name: KeyEventTest_IsRepeat_001
 * @tc.desc: Set repeat_ to false
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_IsRepeat_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetRepeat(false);
    bool result = keyEvent->IsRepeat();
    ASSERT_FALSE(result);
}

/**
 * @tc.name: KeyEventTest_IsRepeat_002
 * @tc.desc: Set repeat_ to true
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_IsRepeat_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetRepeat(true);
    bool result = keyEvent->IsRepeat();
    ASSERT_TRUE(result);
}

/**
 * @tc.name: KeyEventTest_Reset
 * @tc.desc: Test Reset
 * @tc.type: FUNC
 * @tc.require: I5HMCX
 */
HWTEST_F(KeyEventTest, KeyEventTest_Reset, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    ASSERT_NO_FATAL_FAILURE(keyEvent->Reset());
}

/**
 * @tc.name: KeyEventTest_ToString
 * @tc.desc: Test the funcation ToString
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_ToString, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    ASSERT_NO_FATAL_FAILURE(keyEvent->ToString());
}

/**
 * @tc.name: KeyEventTest_SetKeyItem_001
 * @tc.desc: Test the funcation SetKeyItem
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_SetKeyItem_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    std::vector<KeyEvent::KeyItem> keyItem = keyEvent->GetKeyItems();
    ASSERT_NO_FATAL_FAILURE(keyEvent->SetKeyItem(keyItem));
}

/**
 * @tc.name: KeyEventTest_IsRepeatKey_001
 * @tc.desc: Test the funcation IsRepeatKey
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_IsRepeatKey_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    ASSERT_FALSE(keyEvent->IsRepeatKey());
}

/**
 * @tc.name: KeyEventTest_SetRepeatKey_001
 * @tc.desc: Test the funcation SetRepeatKey
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_SetRepeatKey_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    bool repeatKey = true;
    ASSERT_NO_FATAL_FAILURE(keyEvent->SetRepeatKey(repeatKey));
    repeatKey = false;
    ASSERT_NO_FATAL_FAILURE(keyEvent->SetRepeatKey(repeatKey));
}

/**
 * @tc.name: KeyEventTest_GetVKeyboardAction_001
 * @tc.desc: Test the funcation GetVKeyboardAction
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_GetVKeyboardAction_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    #ifdef OHOS_BUILD_ENABLE_VKEYBOARD
    ASSERT_NO_FATAL_FAILURE(keyEvent->GetVKeyboardAction());
    #endif // OHOS_BUILD_ENABLE_VKEYBOARD
}

/**
 * @tc.name: KeyEventTest_SetVKeyboardAction_001
 * @tc.desc: Test the funcation SetVKeyboardAction
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_SetVKeyboardAction_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    #ifdef OHOS_BUILD_ENABLE_VKEYBOARD
    VKeyboardAction vkAction = VKeyboardAction::ACTIVATE_KEYBOARD;
    ASSERT_NO_FATAL_FAILURE(keyEvent->SetVKeyboardAction(vkAction));
    #endif // OHOS_BUILD_ENABLE_VKEYBOARD
}

/**
 * @tc.name: KeyEventTest_GetKeyName_001
 * @tc.desc: Test the funcation GetKeyName
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_GetKeyName_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    #ifdef OHOS_BUILD_ENABLE_VKEYBOARD
    keyEvent->keyName_ = "keyName";
    std::string ret = keyEvent->GetKeyName();
    ASSERT_EQ(ret, "keyName");
    #endif // OHOS_BUILD_ENABLE_VKEYBOARD
}

/**
 * @tc.name: KeyEventTest_SetKeyName_001
 * @tc.desc: Test the funcation SetKeyName
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_SetKeyName_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    #ifdef OHOS_BUILD_ENABLE_VKEYBOARD
    std::string keyName = "keyName";
    ASSERT_NO_FATAL_FAILURE(keyEvent->SetKeyName(keyName));
    #endif // OHOS_BUILD_ENABLE_VKEYBOARD
}

/**
 * @tc.name: KeyEventTest_VKeyboardActionToStr_001
 * @tc.desc: Test the funcation VKeyboardActionToStr
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KeyEventTest, KeyEventTest_VKeyboardActionToStr_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    #ifdef OHOS_BUILD_ENABLE_VKEYBOARD
    VKeyboardAction vKeyAction = VKeyboardAction::ACTIVATE_KEYBOARD;
    const char* ret = keyEvent->VKeyboardActionToStr(vKeyAction);
    ASSERT_EQ(ret, "ACTIVATE_KEYBOARD");
    vKeyAction = VKeyboardAction::VKEY_DOWN;
    ret = keyEvent->VKeyboardActionToStr(vKeyAction);
    ASSERT_EQ(ret, "VKEY_DOWN");
    vKeyAction = VKeyboardAction::VKEY_UP;
    ret = keyEvent->VKeyboardActionToStr(vKeyAction);
    ASSERT_EQ(ret, "VKEY_UP");
    vKeyAction = VKeyboardAction::RESET_BUTTON_COLOR;
    ret = keyEvent->VKeyboardActionToStr(vKeyAction);
    ASSERT_EQ(ret, "RESET_BUTTON_COLOR");
    vKeyAction = VKeyboardAction::TWO_FINGERS_IN;
    ret = keyEvent->VKeyboardActionToStr(vKeyAction);
    ASSERT_EQ(ret, "TWO_FINGERS_IN");
    vKeyAction = VKeyboardAction::TWO_FINGERS_OUT;
    ret = keyEvent->VKeyboardActionToStr(vKeyAction);
    ASSERT_EQ(ret, "TWO_FINGERS_OUT");
    vKeyAction = VKeyboardAction::TWO_HANDS_UP;
    ret = keyEvent->VKeyboardActionToStr(vKeyAction);
    ASSERT_EQ(ret, "TWO_HANDS_UP");
    vKeyAction = VKeyboardAction::TWO_HANDS_DOWN;
    ret = keyEvent->VKeyboardActionToStr(vKeyAction);
    ASSERT_EQ(ret, "TWO_HANDS_DOWN");
    vKeyAction = VKeyboardAction::UNKNOWN;
    ret = keyEvent->VKeyboardActionToStr(vKeyAction);
    ASSERT_EQ(ret, "UNKNOWN");
    #endif // OHOS_BUILD_ENABLE_VKEYBOARD
}
} // namespace MMI
} // namespace OHOS
