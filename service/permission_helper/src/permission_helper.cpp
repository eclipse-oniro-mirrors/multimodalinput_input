/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "permission_helper.h"
#include "ipc_skeleton.h"
#include "mmi_log.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, MMI_LOG_DOMAIN, "PermissionHelper"};
} // namespace

bool PermissionHelper::CheckPermission(uint32_t required)
{
    CALL_DEBUG_ENTER;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (tokenType == OHOS::Security::AccessToken::TOKEN_HAP) {
        return CheckHapPermission(tokenId, required);
    } else if (tokenType == OHOS::Security::AccessToken::TOKEN_NATIVE) {
        return CheckNativePermission(tokenId, required);
    } else {
        MMI_HILOGE("unsupported token type:%{public}d", tokenType);
        return false;
    }
}

bool PermissionHelper::CheckMonitor()
{
    CALL_DEBUG_ENTER;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    static const std::string inputMonitor = "ohos.permission.INPUT_MONITORING";
    if (tokenType == OHOS::Security::AccessToken::TOKEN_HAP) {
        return CheckMonitorHap(tokenId, inputMonitor);
    } else if (tokenType == OHOS::Security::AccessToken::TOKEN_NATIVE) {
        return CheckMonitorNative(tokenId, inputMonitor);
    } else {
        MMI_HILOGE("unsupported token type:%{public}d", tokenType);
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
        MMI_HILOGE("check hap permission failed");
        return false;
    }
    MMI_HILOGI("check hap permission success");
    return true;
}

bool PermissionHelper::CheckNativePermission(uint32_t tokenId, uint32_t required)
{
    OHOS::Security::AccessToken::NativeTokenInfo findInfo;
    if (OHOS::Security::AccessToken::AccessTokenKit::GetNativeTokenInfo(tokenId, findInfo) != 0) {
        MMI_HILOGE("GetNativeTokenInfo failed");
        return false;
    }
    if (!((1 << findInfo.apl) & required)) {
        MMI_HILOGE("check native permission failed");
        return false;
    }
    MMI_HILOGI("check native permission success");
    return true;
}

bool PermissionHelper::CheckMonitorHap(uint32_t tokenId, const std::string &required)
{
    int32_t ret = OHOS::Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, required);
    if (ret != OHOS::Security::AccessToken::PERMISSION_GRANTED) {
        MMI_HILOGE("check hap permission failed ret:%{public}d", ret);
        return false;
    }
    MMI_HILOGI("check hap permission success");
    return true;
}

bool PermissionHelper::CheckMonitorNative(uint32_t tokenId, const std::string &required)
{
    int32_t ret = OHOS::Security::AccessToken::AccessTokenKit::VerifyNativeToken(tokenId, required);
    if (ret != OHOS::Security::AccessToken::PERMISSION_GRANTED) {
        MMI_HILOGE("check native permission failed ret:%{public}d", ret);
        return false;
    }
    MMI_HILOGI("check native permission success");
    return true;
}
} // namespace MMI
} // namespace OHOS