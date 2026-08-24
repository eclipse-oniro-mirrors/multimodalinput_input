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

#include "accesstoken_kit.h"
#include "error_multimodal.h"
#include "mmi_log.h"
#include "mmi_service.h"
#include "permission_helper.h"
#include "proto.h"
#include "util_napi_error_codes.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "SetCustomCursorInnerTest"

namespace OHOS {
namespace MMI {

namespace {
int32_t g_mockTokenType = 0;
}

int32_t PermissionHelper::GetTokenType()
{
    return g_mockTokenType;
}

namespace {
using namespace testing::ext;
}

class SetCustomCursorInnerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SetCustomCursorInnerTest::SetUpTestCase() {}

void SetCustomCursorInnerTest::TearDownTestCase() {}

void SetCustomCursorInnerTest::SetUp()
{
    g_mockTokenType = static_cast<int32_t>(
        OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_HAP);
}

void SetCustomCursorInnerTest::TearDown() {}

HWTEST_F(SetCustomCursorInnerTest, SetCustomCursorInner_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_RUNNING;
    g_mockTokenType = static_cast<int32_t>(
        OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_HAP);
    CustomCursorParcel cursorParcel;
    CursorOptionsParcel optionParcel;
    int32_t ret = mmiService.SetCustomCursorInner(-1, cursorParcel, optionParcel);
    EXPECT_EQ(ret, ERROR_WINDOW_ID_PERMISSION_DENIED);
}

HWTEST_F(SetCustomCursorInnerTest, SetCustomCursorInner_002, TestSize.Level2)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_RUNNING;
    g_mockTokenType = static_cast<int32_t>(
        OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE);
    CustomCursorParcel cursorParcel;
    CursorOptionsParcel optionParcel;
    int32_t ret = mmiService.SetCustomCursorInner(-1, cursorParcel, optionParcel);
    EXPECT_NE(ret, ERROR_WINDOW_ID_PERMISSION_DENIED);
}

HWTEST_F(SetCustomCursorInnerTest, SetCustomCursorInner_003, TestSize.Level2)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_RUNNING;
    g_mockTokenType = static_cast<int32_t>(
        OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL);
    CustomCursorParcel cursorParcel;
    CursorOptionsParcel optionParcel;
    int32_t ret = mmiService.SetCustomCursorInner(-1, cursorParcel, optionParcel);
    EXPECT_EQ(ret, ERROR_WINDOW_ID_PERMISSION_DENIED);
}

HWTEST_F(SetCustomCursorInnerTest, SetCustomCursorInner_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_RUNNING;
    g_mockTokenType = static_cast<int32_t>(
        OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_HAP);
    CustomCursorParcel cursorParcel;
    CursorOptionsParcel optionParcel;
    int32_t ret = mmiService.SetCustomCursorInner(0, cursorParcel, optionParcel);
    EXPECT_NE(ret, ERROR_WINDOW_ID_PERMISSION_DENIED);
}

HWTEST_F(SetCustomCursorInnerTest, SetCustomCursorInner_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_RUNNING;
    g_mockTokenType = static_cast<int32_t>(
        OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_HAP);
    CustomCursorParcel cursorParcel;
    CursorOptionsParcel optionParcel;
    int32_t ret = mmiService.SetCustomCursorInner(1, cursorParcel, optionParcel);
    EXPECT_NE(ret, ERROR_WINDOW_ID_PERMISSION_DENIED);
}

HWTEST_F(SetCustomCursorInnerTest, SetCustomCursorInner_006, TestSize.Level2)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_RUNNING;
    g_mockTokenType = static_cast<int32_t>(
        OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_HAP);
    CustomCursorParcel cursorParcel;
    CursorOptionsParcel optionParcel;
    int32_t ret = mmiService.SetCustomCursorInner(INT32_MIN, cursorParcel, optionParcel);
    EXPECT_EQ(ret, ERROR_WINDOW_ID_PERMISSION_DENIED);
}

HWTEST_F(SetCustomCursorInnerTest, SetCustomCursorInner_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_NOT_START;
    CustomCursorParcel cursorParcel;
    CursorOptionsParcel optionParcel;
    int32_t ret = mmiService.SetCustomCursorInner(-1, cursorParcel, optionParcel);
    EXPECT_EQ(ret, MMISERVICE_NOT_RUNNING);
}

HWTEST_F(SetCustomCursorInnerTest, SetCustomCursorInner_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_RUNNING;
    g_mockTokenType = static_cast<int32_t>(
        OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_HAP);
    CustomCursorParcel cursorParcel;
    CursorOptionsParcel optionParcel;
    ErrCode ret = mmiService.SetCustomCursor(-1, cursorParcel, optionParcel);
    EXPECT_NE(ret, RET_OK);
}

} // namespace MMI
} // namespace OHOS
