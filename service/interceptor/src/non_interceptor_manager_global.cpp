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

#include "non_interceptor_manager_global.h"

#include "mmi_log.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "NonInterceptorManagerGlobal" };
} // namespace

NonInterceptorManagerGlobal::NonInterceptorManagerGlobal() {}

void NonInterceptorManagerGlobal::OnAddInterceptor(int32_t sourceType, int32_t id, SessionPtr session)
{
    CALL_LOG_ENTER;
    return;
}

void NonInterceptorManagerGlobal::OnRemoveInterceptor(int32_t id)
{
    CALL_LOG_ENTER;
    return;
}

bool NonInterceptorManagerGlobal::OnPointerEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    CALL_LOG_ENTER;
    return false;
}

bool NonInterceptorManagerGlobal::OnKeyEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    CALL_LOG_ENTER;
    return false;
}

std::shared_ptr<IInterceptorManagerGlobal> IInterceptorManagerGlobal::GetInstance()
{
    if (interceptorMgrGPtr_ == nullptr) {
        interceptorMgrGPtr_ = std::make_shared<NonInterceptorManagerGlobal>();
    }
    return interceptorMgrGPtr_;
}
} // namespace MMI
} // namespace OHOS