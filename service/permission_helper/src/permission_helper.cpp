/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ipc_skeleton.h"
#include "permission_helper.h"
#include "proto.h"

#include "mmi_log.h"

#include "tokenid_kit.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "PermissionHelper" };
} // namespace

PermissionHelper::PermissionHelper() {}
PermissionHelper::~PermissionHelper() {}

bool PermissionHelper::VerifySystemApp()
{
    MMI_HILOGD("verify system App");
    auto callerToken = IPCSkeleton::GetCallingTokenID();
    auto tokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(callerToken);
    MMI_HILOGD("token type is %{public}d", static_cast<int32_t>(tokenType));
    if (tokenType == OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE
        || tokenType == OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL) {
        MMI_HILOGD("called tokenType is native, verify success");
        return true;
    }
    uint64_t accessTokenIdEx = IPCSkeleton::GetCallingFullTokenID();
    if (!OHOS::Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(accessTokenIdEx)) {
        MMI_HILOGE("system api is called by non-system app");
        return false;
    }
    return true;
}

bool PermissionHelper::CheckPermission(uint32_t required)
{
    CALL_DEBUG_ENTER;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (tokenType == OHOS::Security::AccessToken::TOKEN_HAP) {
        return CheckHapPermission(tokenId, required);
    } else if (tokenType == OHOS::Security::AccessToken::TOKEN_NATIVE) {
        MMI_HILOGD("Token type is native");
        return true;
    } else if (tokenType == OHOS::Security::AccessToken::TOKEN_SHELL) {
        MMI_HILOGI("Token type is shell");
        return true;
    } else {
        MMI_HILOGE("Unsupported token type:%{public}d", tokenType);
        return false;
    }
}

bool PermissionHelper::CheckMonitor()
{
    CALL_DEBUG_ENTER;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if ((tokenType == OHOS::Security::AccessToken::TOKEN_HAP) ||
        (tokenType == OHOS::Security::AccessToken::TOKEN_NATIVE)) {
        return CheckMonitorPermission(tokenId);
    } else if (tokenType == OHOS::Security::AccessToken::TOKEN_SHELL) {
        MMI_HILOGI("Token type is shell");
        return true;
    } else {
        MMI_HILOGE("Unsupported token type:%{public}d", tokenType);
        return false;
    }
}

bool PermissionHelper::CheckInterceptor()
{
    CALL_DEBUG_ENTER;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if ((tokenType == OHOS::Security::AccessToken::TOKEN_HAP) ||
        (tokenType == OHOS::Security::AccessToken::TOKEN_NATIVE)) {
        return CheckInterceptorPermission(tokenId);
    } else if (tokenType == OHOS::Security::AccessToken::TOKEN_SHELL) {
        MMI_HILOGI("Token type is shell");
        return true;
    } else {
        MMI_HILOGE("Unsupported token type:%{public}d", tokenType);
        return false;
    }
}

bool PermissionHelper::CheckHapPermission(uint32_t tokenId, uint32_t required)
{
    OHOS::Security::AccessToken::HapTokenInfo findInfo;
    if (OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, findInfo) != 0) {
        MMI_HILOGE("GetHapTokenInfo failed");
        return false;
    }
    if (!((1 << findInfo.apl) & required)) {
        MMI_HILOGE("Check hap permission failed, name:%{public}s, apl:%{public}d, required:%{public}d",
            findInfo.bundleName.c_str(), findInfo.apl, required);
        return false;
    }
    MMI_HILOGD("Check hap permission success");
    return true;
}

bool PermissionHelper::CheckMonitorPermission(uint32_t tokenId)
{
    static const std::string inputMonitor = "ohos.permission.INPUT_MONITORING";
    int32_t ret = OHOS::Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, inputMonitor);
    if (ret != OHOS::Security::AccessToken::PERMISSION_GRANTED) {
        MMI_HILOGE("Check monitor permission failed ret:%{public}d", ret);
        return false;
    }
    MMI_HILOGD("Check monitor permission success");
    return true;
}

bool PermissionHelper::CheckInterceptorPermission(uint32_t tokenId)
{
    static const std::string inputInterceptor = "ohos.permission.INTERCEPT_INPUT_EVENT";
    int32_t ret = OHOS::Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, inputInterceptor);
    if (ret != OHOS::Security::AccessToken::PERMISSION_GRANTED) {
        MMI_HILOGE("Check interceptor permission failed ret:%{public}d", ret);
        return false;
    }
    MMI_HILOGD("Check interceptor permission success");
    return true;
}

bool PermissionHelper::CheckDispatchControl()
{
    CALL_DEBUG_ENTER;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if ((tokenType == OHOS::Security::AccessToken::TOKEN_HAP) ||
        (tokenType == OHOS::Security::AccessToken::TOKEN_NATIVE)) {
        return CheckDispatchControlPermission(tokenId);
    } else if (tokenType == OHOS::Security::AccessToken::TOKEN_SHELL) {
        MMI_HILOGI("Token type is shell");
        return true;
    } else {
        MMI_HILOGE("Unsupported token type:%{public}d", tokenType);
        return false;
    }
}

bool PermissionHelper::CheckDispatchControlPermission(uint32_t tokenId)
{
    static const std::string inputDispatchControl = "ohos.permission.INPUT_CONTROL_DISPATCHING";
    int32_t ret = OHOS::Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, inputDispatchControl);
    if (ret != OHOS::Security::AccessToken::PERMISSION_GRANTED) {
        MMI_HILOGE("Check input dispatch control permission failed ret:%{public}d", ret);
        return false;
    }
    MMI_HILOGD("Check input dispatch control permission success");
    return true;
}

int32_t PermissionHelper::GetTokenType()
{
    CALL_DEBUG_ENTER;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (tokenType == OHOS::Security::AccessToken::TOKEN_HAP) {
        uint64_t accessTokenIdEx = IPCSkeleton::GetCallingFullTokenID();
        if (OHOS::Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(accessTokenIdEx)) {
            return TokenType::TOKEN_SYSTEM_HAP;
        }
        return TokenType::TOKEN_HAP;
    } else if (tokenType == OHOS::Security::AccessToken::TOKEN_NATIVE) {
        return TokenType::TOKEN_NATIVE;
    }  else if (tokenType == OHOS::Security::AccessToken::TOKEN_SHELL) {
        return TokenType::TOKEN_SHELL;
    } else {
        MMI_HILOGW("Unsupported token type:%{public}d", tokenType);
        return TokenType::TOKEN_INVALID;
    }
}
} // namespace MMI
} // namespace OHOS
