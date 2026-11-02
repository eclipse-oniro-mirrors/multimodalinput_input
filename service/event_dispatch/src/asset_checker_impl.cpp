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

#include "event_dispatch_handler.h"

#include <sstream>

#include "accesstoken_kit.h"
#include "input_event_handler.h"
#include "i_input_windows_manager.h"
#include "mmi_log.h"
#include "saf_gui_fence.h"
#include "util_ex.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_DISPATCH
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "AssetChecker"

namespace OHOS {
namespace MMI {
namespace {
constexpr int64_t WARN_THRESHOLD_US { 100 };
constexpr int64_t ERROR_THRESHOLD_US { 1000 };

struct AppIdentity {
    int32_t uid { -1 };
    Security::AccessToken::ATokenTypeEnum tokenType { Security::AccessToken::TOKEN_INVALID };
    OHOS::Security::SAF::AppInfo appInfo {};
};

struct CheckResult {
    int32_t eventId { -1 };
    int32_t windowId { -1 };
    int32_t ret { 0 };
    bool blocked { false };
    int64_t durationUs { 0 };
    bool valid { false };
};

class AssetCheckerImpl : public AssetChecker {
public:
    bool HandleEvent(const std::shared_ptr<InputEvent>& event) override;
    void Dump(int32_t fd) override;

private:
    bool ResolveApp(AppIdentity& cache, int32_t uid, uint32_t tokenId);
    bool ResolveApps(const std::shared_ptr<InputEvent>& event);

    AppIdentity injector_ {};
    AppIdentity target_ {};
    CheckResult lastResult_ {};
};
} // namespace

std::unique_ptr<AssetChecker> AssetChecker::Create()
{
    return std::make_unique<AssetCheckerImpl>();
}

bool AssetCheckerImpl::ResolveApp(AppIdentity& cache, int32_t uid, uint32_t tokenId)
{
    if (uid == cache.uid) {
        return cache.uid >= 0;
    }
    cache = {};
    if (uid < 0) {
        return false;
    }
    auto tokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (tokenType == Security::AccessToken::TOKEN_INVALID) {
        MMI_HILOGE("GetTokenTypeFlag failed, tokenId:%{private}u, uid:%{private}d", tokenId, uid);
        return false;
    }
    cache.uid = uid;
    cache.tokenType = tokenType;
    if (tokenType != Security::AccessToken::TOKEN_HAP) {
        return true;
    }
    Security::AccessToken::HapBaseInfo hapInfo;
    int32_t ret = Security::AccessToken::AccessTokenKit::GetHapBaseInfoByUid(uid, hapInfo);
    if (ret != 0) {
        MMI_HILOGE("GetHapBaseInfoByUid failed, ret:%{public}d, uid:%{private}d", ret, uid);
        cache = {};
        return false;
    }
    cache.appInfo.bundleName = hapInfo.bundleName;
    cache.appInfo.index = hapInfo.instIndex;
    return true;
}

bool AssetCheckerImpl::ResolveApps(const std::shared_ptr<InputEvent>& event)
{
    auto udsServer = InputHandler->GetUDSServer();
    CHKPR(udsServer, false);
    int32_t targetPid = WIN_MGR->GetPidByDisplayIdAndWindowId(
        event->GetTargetDisplayId(), event->GetTargetWindowId());
    if (targetPid <= 0) {
        MMI_HILOGD("Target window pid not found, windowId:%{public}d, event id:%{private}d",
            event->GetTargetWindowId(), event->GetId());
        return false;
    }
    auto sess = udsServer->GetSessionByPid(targetPid);
    if (!sess) {
        MMI_HILOGD("Session not found for pid:%{public}d, event id:%{private}d", targetPid, event->GetId());
        return false;
    }
    auto injectorInfo = event->GetInjectorInfo();
    if (!injectorInfo.has_value()) {
        MMI_HILOGE("Injector info not set, event id:%{private}d", event->GetId());
        return false;
    }
    if (!ResolveApp(injector_, injectorInfo->uid, injectorInfo->tokenId)) {
        MMI_HILOGE("Resolve injector failed, uid:%{private}d, event id:%{private}d",
            injectorInfo->uid, event->GetId());
        return false;
    }
    if (!ResolveApp(target_, sess->GetUid(), sess->GetTokenId())) {
        MMI_HILOGE("Resolve target failed, uid:%{private}d, event id:%{private}d",
            sess->GetUid(), event->GetId());
        return false;
    }
    return true;
}

bool AssetCheckerImpl::HandleEvent(const std::shared_ptr<InputEvent>& event)
{
    CHKPF(event);
    if (!event->HasFlag(InputEvent::EVENT_FLAG_PENDING_CHECK)) {
        return false;
    }

    if (!ResolveApps(event)) {
        return false;
    }

    if (injector_.tokenType != Security::AccessToken::TOKEN_HAP ||
        target_.tokenType != Security::AccessToken::TOKEN_HAP) {
        lastResult_ = { event->GetId(), event->GetTargetWindowId(), 0, false, 0, true };
        return false;
    }

    MMI_HILOGD("Asset check start, event id:%{private}d, windowId:%{public}d, "
        "injector:uid[%{private}d] type[%{public}d] bundle[%{public}s/%{public}d], "
        "target:uid[%{private}d] type[%{public}d] bundle[%{public}s/%{public}d]",
        event->GetId(), event->GetTargetWindowId(),
        injector_.uid, static_cast<int32_t>(injector_.tokenType), injector_.appInfo.bundleName.c_str(), injector_.appInfo.index,
        target_.uid, static_cast<int32_t>(target_.tokenType), target_.appInfo.bundleName.c_str(), target_.appInfo.index);

    OHOS::Security::SAF::ControlDecision decision = OHOS::Security::SAF::ControlDecision::ALLOW;
    auto startTime = std::chrono::high_resolution_clock::now();
    int32_t ret = OHOS::Security::SAF::SafGuiFence::CheckControlPermission(
        injector_.appInfo, target_.appInfo, decision);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

    bool blockResult = (ret == 0 && decision == OHOS::Security::SAF::ControlDecision::DENY);
    LogLevel level = LOG_DEBUG;
    if (ret != 0 || durationUs > ERROR_THRESHOLD_US) {
        level = LOG_ERROR;
    } else if (durationUs > WARN_THRESHOLD_US) {
        level = LOG_WARN;
    }
    MMI_HILOG_BASE(LOG_CORE, level, MMI_LOG_DOMAIN, MMI_LOG_TAG,
        "Asset check, event id:%{private}d, windowId:%{public}d, ret:%{public}d, "
        "blocked:%{public}d, duration:%{public}" PRId64 "us, "
        "injector:uid[%{private}d] type[%{public}d] bundle[%{public}s/%{public}d], "
        "target:uid[%{private}d] type[%{public}d] bundle[%{public}s/%{public}d]",
        event->GetId(), event->GetTargetWindowId(), ret, blockResult, durationUs,
        injector_.uid, static_cast<int32_t>(injector_.tokenType), injector_.appInfo.bundleName.c_str(), injector_.appInfo.index,
        target_.uid, static_cast<int32_t>(target_.tokenType), target_.appInfo.bundleName.c_str(), target_.appInfo.index);
    lastResult_ = { event->GetId(), event->GetTargetWindowId(), ret, blockResult, durationUs, true };
    return blockResult;
}

void AssetCheckerImpl::Dump(int32_t fd)
{
    if (!lastResult_.valid) {
        mprintf(fd, "AssetChecker: no check performed yet\n");
        return;
    }
    mprintf(fd, "AssetChecker last result:\n");
    mprintf(fd, "  eventId: %d\n", lastResult_.eventId);
    mprintf(fd, "  windowId: %d\n", lastResult_.windowId);
    mprintf(fd, "  ret: %d\n", lastResult_.ret);
    mprintf(fd, "  blocked: %d\n", lastResult_.blocked);
    mprintf(fd, "  duration: %lldus\n", static_cast<long long>(lastResult_.durationUs));
    mprintf(fd, "  injector: uid=%d type=%d bundle=%s index=%d\n",
        injector_.uid, static_cast<int32_t>(injector_.tokenType),
        injector_.appInfo.bundleName.c_str(), injector_.appInfo.index);
    mprintf(fd, "  target: uid=%d type=%d bundle=%s index=%d\n",
        target_.uid, static_cast<int32_t>(target_.tokenType),
        target_.appInfo.bundleName.c_str(), target_.appInfo.index);
}

} // namespace MMI
} // namespace OHOS
