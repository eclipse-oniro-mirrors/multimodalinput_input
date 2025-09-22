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

#include <cstdio>
#include <gtest/gtest.h>
#include <securec.h>

#include "account_manager.h"
#include "key_event.h"
#include "mmi_log.h"
#include "want.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "AccountManagerTest"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t TEST_ACCOUNT_ID_001 { 1 };
constexpr int32_t TEST_ACCOUNT_ID_002 { 2 };
constexpr int32_t MAIN_ACCOUNT_ID { 100 };
constexpr size_t DEFAULT_BUFFER_LENGTH { 512 };
const std::string SECURE_SETTING_URI_PROXY {""};
const std::string TEST_STR_DISPLAY_ID { "0" };
} // namespace

class AccountManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void)
    {
        int32_t accountId = ACCOUNT_MGR->currentAccountId_;
        EXPECT_EQ(accountId, MAIN_ACCOUNT_ID);
        ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->accounts_[accountId]->switchObserver_ = nullptr);
        ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->accounts_[accountId]->onScreenLockedSwitchObserver_ = nullptr);
        ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->accounts_[accountId]->configObserver_ = nullptr);
    }
};

/**
 * @tc.name: AccountManagerTest_GetInstance_01
 * @tc.desc: Test the funcation GetInstance
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_GetInstance_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    AccountManager::instance_ = nullptr;
    ASSERT_NE(ACCOUNT_MGR, nullptr);
}

/**
 * @tc.name: AccountManagerTest_SubscribeCommonEvent_01
 * @tc.desc: Test the funcation SubscribeCommonEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_SubscribeCommonEvent_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    ACCOUNT_MGR->subscriber_ = nullptr;
    ACCOUNT_MGR->timerId_ = -1;
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->SubscribeCommonEvent());
}

/**
 * @tc.name: AccountManagerTest_UnsubscribeCommonEvent_01
 * @tc.desc: Test the funcation UnsubscribeCommonEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_UnsubscribeCommonEvent_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->SubscribeCommonEvent());
    ACCOUNT_MGR->subscriber_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->UnsubscribeCommonEvent());
}

/**
 * @tc.name: AccountManagerTest_SubscribeCommonEvent_02
 * @tc.desc: Test the funcation SubscribeCommonEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_SubscribeCommonEvent_02, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    ACCOUNT_MGR->subscriber_ = nullptr;
    ACCOUNT_MGR->timerId_ = 1;
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->SubscribeCommonEvent());
}

/**
 * @tc.name: AccountManagerTest_OnAddUser_01
 * @tc.desc: Test the funcation OnAddUser
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_OnAddUser_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = TEST_ACCOUNT_ID_001;
    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnAddUser(data));

    auto ret = ACCOUNT_MGR->accounts_.emplace(accountId, std::make_unique<AccountManager::AccountSetting>(accountId));
    EXPECT_FALSE(ret.second);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnAddUser(data));
}

/**
 * @tc.name: AccountManagerTest_OnRemoveUser_01
 * @tc.desc: Test the funcation OnRemoveUser
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_OnRemoveUser_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = TEST_ACCOUNT_ID_002;
    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnAddUser(data));
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_OnCommonEvent_01
 * @tc.desc: Test the funcation OnCommonEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_OnCommonEvent_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EventFwk::CommonEventData data;
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnCommonEvent(data));
}

/**
 * @tc.name: AccountManagerTest_OnCommonEvent_02
 * @tc.desc: Test the funcation OnCommonEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_OnCommonEvent_02, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EventFwk::CommonEventData data;
    std::string action = data.GetWant().GetAction();
    auto func = [](const EventFwk::CommonEventData &) {};
    ACCOUNT_MGR->handlers_[action] = func;
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnCommonEvent(data));
}

/**
 * @tc.name: AccountManagerTest_OnSwitchUser_01
 * @tc.desc: Test OnSwitchUser with empty account
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_OnSwitchUser_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = TEST_ACCOUNT_ID_002;
    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnSwitchUser(data));

    EXPECT_EQ(ACCOUNT_MGR->currentAccountId_, accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnSwitchUser(data));
}

/**
 * @tc.name: AccountManagerTest_OnSwitchUser_02
 * @tc.desc: Test OnSwitchUser with used account
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_OnSwitchUser_02, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = TEST_ACCOUNT_ID_001;
    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnSwitchUser(data));

    data.SetCode(MAIN_ACCOUNT_ID);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnSwitchUser(data));

    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_GetCurrentAccountSetting
 * @tc.desc: Test the funcation GetCurrentAccountSetting
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_GetCurrentAccountSetting_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto accountSetting = ACCOUNT_MGR->GetCurrentAccountSetting();
    int32_t accountId = accountSetting.GetAccountId();
    EXPECT_EQ(accountId, MAIN_ACCOUNT_ID);
}

/**
 * @tc.name: AccountManagerTest_GetCurrentAccountSetting_002
 * @tc.desc: Test the funcation GetCurrentAccountSetting
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_GetCurrentAccountSetting_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    AccountManager accountManager;
    accountManager.timerId_ = 0;
    accountManager.currentAccountId_ = 10;
    accountManager.accounts_.clear();
    auto accountSetting = accountManager.GetCurrentAccountSetting();
    EXPECT_EQ(accountManager.accounts_.size(), 1);
    EXPECT_EQ(accountSetting.GetAccountId(), 10);
}

/**
 * @tc.name: AccountManagerTest_InitializeSetting_01
 * @tc.desc: Test the funcation InitializeSetting
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_InitializeSetting_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 1;
    AccountManager::AccountSetting accountSetting(accountId);
    accountSetting.switchObserver_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(accountSetting.InitializeSetting());

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_InitializeSetting_02
 * @tc.desc: Test the funcation InitializeSetting
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_InitializeSetting_02, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 2;
    AccountManager::AccountSetting accountSetting(accountId);
    accountSetting.onScreenLockedSwitchObserver_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(accountSetting.InitializeSetting());

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_InitializeSetting_03
 * @tc.desc: Test the funcation InitializeSetting
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_InitializeSetting_03, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 3;
    AccountManager::AccountSetting accountSetting(accountId);
    accountSetting.configObserver_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(accountSetting.InitializeSetting());

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_ReadSwitchStatus_01
 * @tc.desc: Test the funcation ReadSwitchStatus
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_ReadSwitchStatus_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 3;
    AccountManager::AccountSetting accountSetting(accountId);
    accountSetting.accountId_ = -1;
    std::string key = "down";
    bool currentSwitchStatus = true;
    bool ret = accountSetting.ReadSwitchStatus(key, currentSwitchStatus);
    EXPECT_FALSE(ret);

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_ReadSwitchStatus_02
 * @tc.desc: Test the funcation ReadSwitchStatus
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_ReadSwitchStatus_02, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 5;
    AccountManager::AccountSetting accountSetting(accountId);
    accountSetting.accountId_ = 2;
    std::string key = "invaild";
    bool currentSwitchStatus = false;

    char buf[DEFAULT_BUFFER_LENGTH] {};
    EXPECT_FALSE(sprintf_s(buf, sizeof(buf), SECURE_SETTING_URI_PROXY.c_str(), accountSetting.accountId_) > 0);
    bool ret = accountSetting.ReadSwitchStatus(key, currentSwitchStatus);
    EXPECT_FALSE(ret);

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_ReadLongPressTime_01
 * @tc.desc: Test the funcation ReadLongPressTime
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_ReadLongPressTime_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 3;
    AccountManager::AccountSetting accountSetting(accountId);
    accountSetting.accountId_ = -1;
    ASSERT_NO_FATAL_FAILURE(accountSetting.ReadLongPressTime());

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_ReadLongPressTime_02
 * @tc.desc: Test the funcation ReadLongPressTime
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_ReadLongPressTime_02, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 3;
    AccountManager::AccountSetting accountSetting(accountId);
    accountSetting.accountId_ = 2;

    char buf[DEFAULT_BUFFER_LENGTH] {};
    EXPECT_FALSE(sprintf_s(buf, sizeof(buf), SECURE_SETTING_URI_PROXY.c_str(), accountSetting.accountId_) > 0);
    ASSERT_NO_FATAL_FAILURE(accountSetting.ReadLongPressTime());

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_AccShortcutTimeout_01
 * @tc.desc: Test the funcation AccShortcutTimeout
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_AccShortcutTimeout_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 1;
    std::string key = "testKey";
    AccountManager::AccountSetting accountSetting(accountId);
    auto accountMgr = ACCOUNT_MGR;
    accountMgr->accounts_.emplace(accountId, std::make_unique<AccountManager::AccountSetting>(accountId));
    ASSERT_NO_FATAL_FAILURE(accountSetting.AccShortcutTimeout(accountId, key));

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_AccShortcutTimeout_02
 * @tc.desc: Test the funcation AccShortcutTimeout
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_AccShortcutTimeout_02, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 2;
    std::string key = "testKey";
    AccountManager::AccountSetting accountSetting(accountId);
    auto accountMgr = ACCOUNT_MGR;
    ASSERT_NO_FATAL_FAILURE(accountSetting.AccShortcutTimeout(accountId, key));

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_AccShortcutEnabled_01
 * @tc.desc: Test the funcation AccShortcutEnabled
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_AccShortcutEnabled_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 1;
    std::string key = "shortcutKey";
    AccountManager::AccountSetting accountSetting(accountId);
    auto accountMgr = ACCOUNT_MGR;
    accountMgr->accounts_.emplace(accountId, std::make_unique<AccountManager::AccountSetting>(accountId));
    ASSERT_NO_FATAL_FAILURE(accountSetting.AccShortcutEnabled(accountId, key));

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_AccShortcutEnabled_02
 * @tc.desc: Test the funcation AccShortcutEnabled
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_AccShortcutEnabled_02, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 2;
    std::string key = "shortcutKey";
    AccountManager::AccountSetting accountSetting(accountId);
    auto accountMgr = ACCOUNT_MGR;
    ASSERT_NO_FATAL_FAILURE(accountSetting.AccShortcutEnabled(accountId, key));

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_AccShortcutEnabledOnScreenLocked_01
 * @tc.desc: Test the funcation AccShortcutEnabledOnScreenLocked
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_AccShortcutEnabledOnScreenLocked_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 1;
    std::string key = "shortcutKey";
    AccountManager::AccountSetting accountSetting(accountId);
    auto accountMgr = ACCOUNT_MGR;
    accountMgr->accounts_.emplace(accountId, std::make_unique<AccountManager::AccountSetting>(accountId));
    ASSERT_NO_FATAL_FAILURE(accountSetting.AccShortcutEnabledOnScreenLocked(accountId, key));

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_AccShortcutEnabledOnScreenLocked_02
 * @tc.desc: Test the funcation AccShortcutEnabledOnScreenLocked
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_AccShortcutEnabledOnScreenLocked_02, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 2;
    std::string key = "shortcutKey";
    AccountManager::AccountSetting accountSetting(accountId);
    auto accountMgr = ACCOUNT_MGR;
    ASSERT_NO_FATAL_FAILURE(accountSetting.AccShortcutEnabledOnScreenLocked(accountId, key));

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_ReadSwitchStatus_03
 * @tc.desc: Test the funcation ReadSwitchStatus
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_ReadSwitchStatus_03, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = -1;
    AccountManager::AccountSetting accountSetting(accountId);
    std::string key = "down";
    bool currentSwitchStatus = true;
    bool ret = accountSetting.ReadSwitchStatus(key, currentSwitchStatus);
    EXPECT_FALSE(ret);

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_ReadSwitchStatus_04
 * @tc.desc: Test the funcation ReadSwitchStatus
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_ReadSwitchStatus_04, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 5;
    AccountManager::AccountSetting accountSetting(accountId);
    std::string key = "invaild";
    bool currentSwitchStatus = false;

    char buf[DEFAULT_BUFFER_LENGTH] {};
    EXPECT_FALSE(sprintf_s(buf, sizeof(buf), SECURE_SETTING_URI_PROXY.c_str(), accountSetting.accountId_) > 0);
    bool ret = accountSetting.ReadSwitchStatus(key, currentSwitchStatus);
    EXPECT_FALSE(ret);

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_ReadLongPressTime_03
 * @tc.desc: Test the funcation ReadLongPressTime
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_ReadLongPressTime_03, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = -1;
    AccountManager::AccountSetting accountSetting(accountId);
    ASSERT_NO_FATAL_FAILURE(accountSetting.ReadLongPressTime());

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_ReadLongPressTime_04
 * @tc.desc: Test the funcation ReadLongPressTime
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_ReadLongPressTime_04, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 3;
    AccountManager::AccountSetting accountSetting(accountId);
    accountSetting.accountId_ = 2;
    char buf[DEFAULT_BUFFER_LENGTH] {};
    EXPECT_FALSE(sprintf_s(buf, sizeof(buf), SECURE_SETTING_URI_PROXY.c_str(), accountSetting.accountId_) > 0);
    ASSERT_NO_FATAL_FAILURE(accountSetting.ReadLongPressTime());

    EventFwk::CommonEventData data;
    data.SetCode(accountId);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

/**
 * @tc.name: AccountManagerTest_OnReceiveEvent
 * @tc.desc: Test the funcation OnReceiveEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_OnReceiveEvent, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EventFwk::CommonEventData data;
    auto subscriber = ACCOUNT_MGR->subscriber_;
    ASSERT_NE(subscriber, nullptr);
    ASSERT_NO_FATAL_FAILURE(subscriber->OnReceiveEvent(data));
}

/**
 * @tc.name: AccountManagerTest_Operator
 * @tc.desc: Test the funcation operator=
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_Operator, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t accountId = 3;
    int32_t timeout = 5000;
    AccountManager::AccountSetting accountSetting(accountId);
    accountSetting.accShortcutEnabled_ = true;
    accountSetting.accShortcutEnabledOnScreenLocked_ = true;
    accountSetting.accShortcutTimeout_ = timeout;
    AccountManager::AccountSetting accountSetting1 = accountSetting;
    EXPECT_EQ(accountSetting1.accountId_, accountId);
    EXPECT_TRUE(accountSetting1.accShortcutEnabled_);
    EXPECT_TRUE(accountSetting1.accShortcutEnabledOnScreenLocked_);
    EXPECT_EQ(accountSetting1.accShortcutTimeout_, timeout);
}

/**
 * @tc.name: AccountManagerTest_SetupMainAccount
 * @tc.desc: Test the funcation SetupMainAccount
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_SetupMainAccount, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    ACCOUNT_MGR->accounts_.clear();
    auto ret = ACCOUNT_MGR->accounts_.emplace(MAIN_ACCOUNT_ID,
                                                    std::make_unique<AccountManager::AccountSetting>(MAIN_ACCOUNT_ID));
    EXPECT_TRUE(ret.second);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->SetupMainAccount());

    ret = ACCOUNT_MGR->accounts_.emplace(MAIN_ACCOUNT_ID,
                                                    std::make_unique<AccountManager::AccountSetting>(MAIN_ACCOUNT_ID));
    EXPECT_FALSE(ret.second);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->SetupMainAccount());
}

/**
 * @tc.name: AccountManagerTest_GetCurrentAccountId_01
 * @tc.desc: Test the funcation GetCurrentAccountId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AccountManagerTest, AccountManagerTest_GetCurrentAccountId_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EventFwk::CommonEventData data;
    data.SetCode(MAIN_ACCOUNT_ID);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnSwitchUser(data));
    EXPECT_EQ(MAIN_ACCOUNT_ID, ACCOUNT_MGR->GetCurrentAccountId());
 
    data.SetCode(TEST_ACCOUNT_ID_001);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnSwitchUser(data));
    EXPECT_EQ(TEST_ACCOUNT_ID_001, ACCOUNT_MGR->GetCurrentAccountId());
 
    data.SetCode(MAIN_ACCOUNT_ID);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnSwitchUser(data));
    EXPECT_EQ(MAIN_ACCOUNT_ID, ACCOUNT_MGR->GetCurrentAccountId());
 
    data.SetCode(TEST_ACCOUNT_ID_001);
    ASSERT_NO_FATAL_FAILURE(ACCOUNT_MGR->OnRemoveUser(data));
}

} // namespace MMI
} // namespace OHOS