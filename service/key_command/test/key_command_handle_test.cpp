/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

 #include "key_command_handler_util.h"
 #include "mmi_log.h"
 
 #undef MMI_LOG_TAG
 #define MMI_LOG_TAG "KeyCommandHandlerUtilTest"
 
 namespace OHOS {
 namespace MMI {
 namespace {
 using namespace testing::ext;
 } // namespace
 
 class KeyCommandHandlerUtilTest : public testing::Test {
 public:
     static void SetUpTestCase(void) {}
     static void TearDownTestCase(void) {}
 };
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_IsSpecialType_001
  * @tc.desc: Test the function IsSpecialType
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_IsSpecialType_001, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     SpecialType type = SPECIAL_ALL;
     int32_t keyCode = 1;
     bool result = OHOS::MMI::IsSpecialType(keyCode, type);
     EXPECT_FALSE(result);
     type = SUBSCRIBER_BEFORE_DELAY;
     keyCode = 2;
     result = OHOS::MMI::IsSpecialType(keyCode, type);
     EXPECT_FALSE(result);
     type = KEY_DOWN_ACTION;
     keyCode = 3;
     result = OHOS::MMI::IsSpecialType(keyCode, type);
     EXPECT_FALSE(result);
     type = KEY_DOWN_ACTION;
     keyCode = -1;
     result = OHOS::MMI::IsSpecialType(keyCode, type);
     EXPECT_FALSE(result);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetBusinessId_001
  * @tc.desc: Test the function GetBusinessId
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetBusinessId_001, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON *jsonData = cJSON_CreateString("not an object");
     std::string businessIdValue;
     std::vector<std::string> businessIds;
     bool result = OHOS::MMI::GetBusinessId(jsonData, businessIdValue, businessIds);
     EXPECT_FALSE(result);
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetBusinessId_002
  * @tc.desc: Test the function GetBusinessId
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetBusinessId_002, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON *jsonData = cJSON_CreateObject();
     cJSON_AddItemToObject(jsonData, "businessId", cJSON_CreateNumber(123));
     std::string businessIdValue;
     std::vector<std::string> businessIds;
     bool result = OHOS::MMI::GetBusinessId(jsonData, businessIdValue, businessIds);
     EXPECT_FALSE(result);
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_001
  * @tc.desc: Test the function GetPreKeys
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_001, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     ShortcutKey shortcutKey;
     bool result = OHOS::MMI::GetPreKeys(jsonData, shortcutKey);
     EXPECT_FALSE(result);
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_002
  * @tc.desc: Test the function GetPreKeys
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_002, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     cJSON* preKey = cJSON_CreateArray();
     for (int i = 0; i < MAX_PREKEYS_NUM + 1; ++i) {
         cJSON_AddItemToArray(preKey, cJSON_CreateNumber(i));
     }
     cJSON_AddItemToObject(jsonData, "preKey", preKey);
     ShortcutKey shortcutKey;
     bool result = OHOS::MMI::GetPreKeys(jsonData, shortcutKey);
     EXPECT_FALSE(result);
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_003
  * @tc.desc: Test the function GetPreKeys
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_003, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     cJSON_AddItemToObject(jsonData, "preKey", cJSON_CreateString("invalid"));
     ShortcutKey shortcutKey;
     bool result = OHOS::MMI::GetPreKeys(jsonData, shortcutKey);
     EXPECT_FALSE(result);
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_004
  * @tc.desc: Test the function GetPreKeys
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_004, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     cJSON* preKey = cJSON_CreateArray();
     cJSON_AddItemToArray(preKey, cJSON_CreateNumber(-1));
     cJSON_AddItemToObject(jsonData, "preKey", preKey);
     ShortcutKey shortcutKey;
     bool result = OHOS::MMI::GetPreKeys(jsonData, shortcutKey);
     EXPECT_FALSE(result);
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_005
  * @tc.desc: Test the function GetPreKeys
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_005, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     cJSON* preKey = cJSON_CreateArray();
     cJSON_AddItemToArray(preKey, cJSON_CreateNumber(1));
     cJSON_AddItemToArray(preKey, cJSON_CreateNumber(1));
     cJSON_AddItemToObject(jsonData, "preKey", preKey);
     ShortcutKey shortcutKey;
     bool result = OHOS::MMI::GetPreKeys(jsonData, shortcutKey);
     EXPECT_FALSE(result);
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_IsSpecialType_002
  * @tc.desc: Test keyCode is not in SPECIAL_KEYS
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_IsSpecialType_002, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     int32_t keyCode = 999;
     SpecialType type = SpecialType::SPECIAL_ALL;
     EXPECT_FALSE(OHOS::MMI::IsSpecialType(keyCode, type));
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_IsSpecialType_003
  * @tc.desc: The corresponding value is not equal to SpecialType.: SPECIAL_ALL and input type
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_IsSpecialType_003, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     int32_t keyCode = 16;
     SpecialType type = SpecialType::SPECIAL_ALL;
     EXPECT_FALSE(OHOS::MMI::IsSpecialType(keyCode, type));
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_IsSpecialType_004
  * @tc.desc: The test keyCode is in SPECIAL_KEYS and the value is equal to SpecialType.: SPECIAL_ALL
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_IsSpecialType_004, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     int32_t keyCode = 0;
     SpecialType type = SpecialType::SPECIAL_ALL;
     EXPECT_FALSE(OHOS::MMI::IsSpecialType(keyCode, type));
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetBusinessId_003
  * @tc.desc: Test the scenario where the JSON object is not a valid object
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetBusinessId_003, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON *jsonData = nullptr;
     std::string businessIdValue;
     std::vector<std::string> businessIds;
     bool result = OHOS::MMI::GetBusinessId(jsonData, businessIdValue, businessIds);
     EXPECT_FALSE(result);
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetBusinessId_004
  * @tc.desc: Test the scenario where businessId is not a string
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetBusinessId_004, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON *jsonData = cJSON_CreateObject();
     std::vector<std::string> businessIds;
     cJSON_AddItemToObject(jsonData, "businessIds", cJSON_CreateNumber(123));
     std::string businessIdValue;
     bool result = OHOS::MMI::GetBusinessId(jsonData, businessIdValue, businessIds);
     EXPECT_FALSE(result);
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetBusinessId_005
  * @tc.desc: Test the normal running condition
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetBusinessId_005, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON *jsonData = cJSON_CreateObject();
     std::vector<std::string> businessIds;
     cJSON_AddStringToObject(jsonData, "businessId", "testBusinessId");
     std::string businessIdValue;
     bool result = OHOS::MMI::GetBusinessId(jsonData, businessIdValue, businessIds);
     EXPECT_TRUE(result);
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_006
  * @tc.desc: Test the case that the input jsonData is not an object
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_006, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = nullptr;
     ShortcutKey shortcutKey;
     EXPECT_FALSE(OHOS::MMI::GetPreKeys(jsonData, shortcutKey));
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_007
  * @tc.desc: Test the case that preKey is not an array
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_007, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     cJSON_AddItemToObject(jsonData, "preKey", cJSON_CreateString("test"));
     ShortcutKey shortcutKey;
     EXPECT_FALSE(OHOS::MMI::GetPreKeys(jsonData, shortcutKey));
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_008
  * @tc.desc: Test the case that the size of preKey exceeds MAX_PREKEYS_NUM
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_008, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     cJSON* preKey = cJSON_CreateArray();
     for (int i = 0; i < 10; ++i) {
         cJSON_AddItemToArray(preKey, cJSON_CreateNumber(i));
     }
     cJSON_AddItemToObject(jsonData, "preKey", preKey);
     ShortcutKey shortcutKey;
     EXPECT_FALSE(OHOS::MMI::GetPreKeys(jsonData, shortcutKey));
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_009
  * @tc.desc: Test if the element in preKey is not a number
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_009, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     cJSON* preKey = cJSON_CreateArray();
     cJSON_AddItemToArray(preKey, cJSON_CreateString("not a number"));
     cJSON_AddItemToObject(jsonData, "preKey", preKey);
     ShortcutKey shortcutKey;
     EXPECT_FALSE(OHOS::MMI::GetPreKeys(jsonData, shortcutKey));
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_010
  * @tc.desc: Tests if the number in preKey is less than 0
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_010, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     cJSON* preKey = cJSON_CreateArray();
     cJSON_AddItemToArray(preKey, cJSON_CreateNumber(-1));
     cJSON_AddItemToObject(jsonData, "preKey", preKey);
     ShortcutKey shortcutKey;
     EXPECT_FALSE(OHOS::MMI::GetPreKeys(jsonData, shortcutKey));
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_011
  * @tc.desc: Test the duplicated number in preKey
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_011, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     cJSON* preKey = cJSON_CreateArray();
     cJSON_AddItemToArray(preKey, cJSON_CreateNumber(1));
     cJSON_AddItemToArray(preKey, cJSON_CreateNumber(1));
     cJSON_AddItemToObject(jsonData, "preKey", preKey);
     ShortcutKey shortcutKey;
     EXPECT_FALSE(OHOS::MMI::GetPreKeys(jsonData, shortcutKey));
     cJSON_Delete(jsonData);
 }
 
 /**
  * @tc.name: KeyCommandHandlerUtilTest_GetPreKeys_012
  * @tc.desc: Test the normal running condition
  * @tc.type: FUNC
  * @tc.require:
  */
 HWTEST_F(KeyCommandHandlerUtilTest, KeyCommandHandlerUtilTest_GetPreKeys_012, TestSize.Level1)
 {
     CALL_TEST_DEBUG;
     cJSON* jsonData = cJSON_CreateObject();
     cJSON* preKey = cJSON_CreateArray();
     cJSON_AddItemToObject(jsonData, "preKey", preKey);
     cJSON_AddItemToArray(preKey, cJSON_CreateNumber(1));
     cJSON_AddItemToArray(preKey, cJSON_CreateNumber(2));
     cJSON_AddItemToArray(preKey, cJSON_CreateNumber(3));
     cJSON_AddItemToArray(preKey, cJSON_CreateNumber(4));
     ShortcutKey shortcutKey;
     EXPECT_TRUE(OHOS::MMI::GetPreKeys(jsonData, shortcutKey));
     cJSON_Delete(jsonData);
 }
 } // namespace MMI
} // namespace OHOS