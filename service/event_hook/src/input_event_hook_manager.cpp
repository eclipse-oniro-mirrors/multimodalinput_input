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

#include "input_event_hook_manager.h"

#include <utility>
#include <vector>

#include "display_event_monitor.h"
#include "error_multimodal.h"
#include "input_event_data_transformation.h"
#include "input_event_handler.h"
#include "i_input_windows_manager.h"
#include "util_ex.h"
#include "window_info.h"
#include "pointer_event_hook.h"
#include "key_event_hook.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_HANDLER
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "InputEventHookManager"

namespace OHOS {
namespace MMI {
namespace {
bool CheckTargetWindowSkipHook(const std::shared_ptr<InputEvent> event)
{
    CHKPF(event);
    int32_t targetWindowId = event->GetTargetWindowId();
    int32_t targetDisplayId = event->GetTargetDisplayId();
    if (targetWindowId >= 0 && targetDisplayId >= 0) {
        auto windowInfo = WIN_MGR->GetWindowAndDisplayInfo(targetWindowId, targetDisplayId);
        if (windowInfo.has_value() && (windowInfo->flags & WindowInputPolicy::FLAG_SKIP_HOOK)) {
            return true;
        }
    }
    return false;
}

bool IsAxisEvent(int32_t action)
{
    return action == PointerEvent::POINTER_ACTION_AXIS_BEGIN ||
        action == PointerEvent::POINTER_ACTION_AXIS_UPDATE ||
        action == PointerEvent::POINTER_ACTION_AXIS_END;
}
} // namespace

InputEventHookManager::~InputEventHookManager() {}

int32_t InputEventHookManager::AddInputEventHook(int32_t pid, HookEventType hookEventType)
{
    CALL_DEBUG_ENTER;
    Init();
    if (IsHookExistedByPid(pid)) {
        MMI_HILOGE("Hook already existed for pid:%{public}d", pid);
        return ERROR_REPEAT_INTERCEPTOR;
    }
    auto udsServerPtr = InputHandler->GetUDSServer();
    CHKPR(udsServerPtr, RET_ERR);
    auto session = udsServerPtr->GetSessionByPid(pid);
    auto nextHookGetter = [this] (std::shared_ptr<InputEventHook> hook) -> std::shared_ptr<InputEventHook> {
        return this->GetNextHook(hook);
    };
    if (hookEventType & HOOK_EVENT_TYPE_KEY) {
        PrependHook(HOOK_EVENT_TYPE_KEY, std::make_shared<KeyEventHook>(session, nextHookGetter));
    }
    if (hookEventType & HOOK_EVENT_TYPE_MOUSE) {
        PrependHook(HOOK_EVENT_TYPE_MOUSE, std::make_shared<PointerEventHook>(session, HOOK_EVENT_TYPE_MOUSE,
            nextHookGetter));
    }
    if (hookEventType & HOOK_EVENT_TYPE_TOUCH) {
        PrependHook(HOOK_EVENT_TYPE_TOUCH, std::make_shared<PointerEventHook>(session, HOOK_EVENT_TYPE_TOUCH,
            nextHookGetter));
    }
    if (hookEventType & HOOK_EVENT_TYPE_AXIS) {
        PrependHook(HOOK_EVENT_TYPE_AXIS, std::make_shared<PointerEventHook>(session, HOOK_EVENT_TYPE_AXIS,
            nextHookGetter));
    }
    MMI_HILOGI("AddInputEventHook success %{public}u", hookEventType);
    return RET_OK;
}

int32_t InputEventHookManager::RemoveInputEventHook(int32_t pid, HookEventType hookEventType)
{
    CALL_DEBUG_ENTER;
    if (hookEventType & HOOK_EVENT_TYPE_KEY) {
        RemoveHookByPid(pid, HOOK_EVENT_TYPE_KEY);
    }
    if (hookEventType & HOOK_EVENT_TYPE_MOUSE) {
        RemoveHookByPid(pid, HOOK_EVENT_TYPE_MOUSE);
    }
    if (hookEventType & HOOK_EVENT_TYPE_TOUCH) {
        RemoveHookByPid(pid, HOOK_EVENT_TYPE_TOUCH);
    }
    if (hookEventType & HOOK_EVENT_TYPE_AXIS) {
        RemoveHookByPid(pid, HOOK_EVENT_TYPE_AXIS);
    }
    MMI_HILOGI("RemoveInputEventHook success %{public}u", hookEventType);
    return RET_OK;
}

void InputEventHookManager::HandleKeyEvent(const std::shared_ptr<KeyEvent> keyEvent)
{
    if (CheckTargetWindowSkipHook(keyEvent)) {
        MMI_HILOGD("Target window has FLAG_SKIP_HOOK, skip hook");
    } else if (IsHookSuspended()) {
        MMI_HILOGD("Screen is locked, skip hook");
    } else if (IsHooksExisted(HOOK_EVENT_TYPE_KEY) && HandleHooks(keyEvent)) {
        return;
    }
    CHKPV(nextHandler_);
    nextHandler_->HandleKeyEvent(keyEvent);
}

void InputEventHookManager::HandlePointerEvent(const std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    HookEventType hookType { HOOK_EVENT_TYPE_NONE };
    if (pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_MOUSE) {
        hookType = IsAxisEvent(pointerEvent->GetPointerAction()) ? HOOK_EVENT_TYPE_AXIS : HOOK_EVENT_TYPE_MOUSE;
    } else if (pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) {
        hookType = HOOK_EVENT_TYPE_TOUCH;
    }
    if (hookType != HOOK_EVENT_TYPE_NONE) {
        bool accepted { false };
        if (CheckTargetWindowSkipHook(pointerEvent)) {
            MMI_HILOGD("Target window has FLAG_SKIP_HOOK, skip hook");
        } else if (IsHookSuspended()) {
            MMI_HILOGD("Screen is locked, skip hook");
        } else if (IsHooksExisted(hookType)) {
            accepted = HandleHooks(pointerEvent);
        }
        if (accepted) {
            return;
        }
    }
    CHKPV(nextHandler_);
    nextHandler_->HandlePointerEvent(pointerEvent);
}

void InputEventHookManager::HandleTouchEvent(const std::shared_ptr<PointerEvent> pointerEvent)
{
    if (CheckTargetWindowSkipHook(pointerEvent)) {
        MMI_HILOGD("Target window has FLAG_SKIP_HOOK, skip hook");
    } else if (IsHookSuspended()) {
        MMI_HILOGD("Screen is locked, skip hook");
    } else if (IsHooksExisted(HOOK_EVENT_TYPE_TOUCH) && HandleHooks(pointerEvent)) {
        return;
    }
    CHKPV(nextHandler_);
    nextHandler_->HandleTouchEvent(pointerEvent);
}

int32_t InputEventHookManager::NotifyHookEventResult(int32_t pid, int32_t eventId, HookEventType hookEventType,
    bool isConsumed)
{
    CALL_DEBUG_ENTER;
    auto hook = GetHookByPid(pid, hookEventType);
    CHKPR(hook, RET_ERR);
    return hook->HandleEventResult(eventId, isConsumed);
}

bool InputEventHookManager::IsHooksExisted(HookEventType hookEventType)
{
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = hooks_.find(hookEventType);
    if (it == hooks_.end()) {
        return false;
    }
    return !(it->second.empty());
}

void InputEventHookManager::Dump(int32_t fd, const std::vector<std::string> &args)
{
    CALL_DEBUG_ENTER;
    mprintf(fd, "Hook information:\t");
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    for (const auto &[hookType, hooks] : hooks_) {
        mprintf(fd, "HookType:%10u Hook count:%zu\n", hookType, hooks.size());
        for (const auto &hook : hooks) {
            CHKPC(hook);
            mprintf(fd, "HookPid:%10d programName:%s", hook->GetHookPid(), hook->GetProgramName().c_str());
        }
    }
}

void InputEventHookManager::Init()
{
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    if (isInitialized_.load())  {
        return;
    }
    InitSessionLostCallback();
    isInitialized_.store(true);
}

void InputEventHookManager::InitSessionLostCallback()
{
    CALL_DEBUG_ENTER;
    auto udsServerPtr = InputHandler->GetUDSServer();
    CHKPV(udsServerPtr);
    udsServerPtr->AddSessionDeletedCallback(
        [this] (SessionPtr session) {
            return this->OnSessionLost(session);
        }
    );
}

void InputEventHookManager::OnSessionLost(SessionPtr session)
{
    CALL_DEBUG_ENTER;
    CHKPV(session);
    auto pid = session->GetPid();
    RemoveHookByPid(pid, HOOK_EVENT_TYPE_KEY);
    RemoveHookByPid(pid, HOOK_EVENT_TYPE_MOUSE);
    RemoveHookByPid(pid, HOOK_EVENT_TYPE_TOUCH);
    RemoveHookByPid(pid, HOOK_EVENT_TYPE_AXIS);
}

void InputEventHookManager::PrependHook(HookEventType hookEventType, std::shared_ptr<InputEventHook> hook)
{
    CALL_DEBUG_ENTER;
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    hooks_[hookEventType].push_front(hook);
}


bool InputEventHookManager::IsHookExistedByPid(int32_t pid)
{
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    for (const auto &[hookType, hookList] : hooks_) {
        auto iter = std::find_if(hookList.begin(), hookList.end(),
            [pid](const std::shared_ptr<InputEventHook>& hook) {
                return (hook != nullptr) && hook->GetHookPid() == pid;
            }
        );
        if (iter != hookList.end()) {
            return true;
        }
    }
    return false;
}

bool InputEventHookManager::IsHookSuspended()
{
    bool locked = DISPLAY_MONITOR->GetScreenLocked();
    bool last = lastScreenLocked_.exchange(locked);
    if (locked && !last) {
        // Lock edge: suspend all hooks (replay in-flight events, reset gesture and stream state,
        // cancel the timeout monitor); keep the registration without notifying the business;
        // Suspend is idempotent and harmless when triggered repeatedly.
        // Collect inside the lock, suspend outside it: Suspend runs FlushStashEvents->GetNextHook which
        // re-acquires rwMutex_, so nested locking would risk a deadlock
        std::vector<std::shared_ptr<InputEventHook>> suspendHooks;
        {
            std::shared_lock<std::shared_mutex> lock(rwMutex_);
            for (auto &[hookType, hookList] : hooks_) {
                for (auto &hook : hookList) {
                    if (hook) {
                        suspendHooks.push_back(hook);
                    }
                }
            }
        }
        if (!suspendHooks.empty()) {
            MMI_HILOGI("Screen locked, suspend %{public}zu hooks", suspendHooks.size());
            for (auto &hook : suspendHooks) {
                hook->Suspend();
            }
        }
    }
    return locked;
}

std::shared_ptr<InputEventHook> InputEventHookManager::GetHookByPid(int32_t pid, HookEventType hookEventType)
{
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = hooks_.find(hookEventType);
    if (it == hooks_.end()) {
        MMI_HILOGW("No hook existed for event type %{public}d", static_cast<int32_t>(hookEventType));
        return nullptr;
    }
    auto& hookList = it->second;
    auto iter = std::find_if(hookList.begin(), hookList.end(), [pid](const std::shared_ptr<InputEventHook>& hook) {
            return (hook != nullptr) && hook->GetHookPid() == pid;
        }
    );
    if (iter == hookList.end()) {
        return nullptr;
    }
    return *iter;
}

int32_t InputEventHookManager::RemoveHookByPid(int32_t pid, HookEventType hookEventType)
{
    std::vector<std::pair<std::shared_ptr<InputEventHook>, std::shared_ptr<InputEventHook>>> flushList;
    {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        auto it = hooks_.find(hookEventType);
        if (it == hooks_.end()) {
            MMI_HILOGW("No hook existed for event type %{public}d", static_cast<int32_t>(hookEventType));
            return RET_OK;
        }
        auto& hookList = it->second;
        for (auto iter = hookList.begin(); iter != hookList.end();) {
            if (!*iter || (*iter)->GetHookPid() == RET_ERR || (*iter)->GetHookPid() == pid) {
                auto nextIter = std::next(iter);
                auto nextHook = (nextIter != hookList.end()) ? *nextIter : nullptr;
                flushList.emplace_back(*iter, nextHook);
                iter = hookList.erase(iter);
            } else {
                ++iter;
            }
        }
    }
    for (auto &[hook, nextHook] : flushList) {
        if (hook) {
            hook->FlushStashEvents(nextHook);
        }
    }
    MMI_HILOGI("RemoveHookByPid success pid:%{public}d %{public}u", pid, hookEventType);
    return RET_OK;
}



std::shared_ptr<InputEventHook> InputEventHookManager::GetNextHook(std::shared_ptr<InputEventHook> hook)
{
    CALL_DEBUG_ENTER;
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    CHKPP(hook);
    auto hookEventType = hook->GetHookEventType();
    auto it = hooks_.find(hookEventType);
    if (it == hooks_.end()) {
        return nullptr;
    }
    auto& hookList = it->second;
    auto iter = std::find(hookList.begin(), hookList.end(), hook);
    if (iter == hookList.end()) {
        MMI_HILOGE("No hook existed");
        return nullptr;
    }
    auto nextIter = std::next(iter);
    if (nextIter == hookList.end()) {
        MMI_HILOGW("No next hook existed");
        return nullptr;
    }
    return *nextIter;
}

bool InputEventHookManager::HandleHooks(const std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPF(keyEvent);
    std::vector<std::shared_ptr<InputEventHook>> snapshot;
    {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        auto it = hooks_.find(HOOK_EVENT_TYPE_KEY);
        if (it == hooks_.end()) {
            MMI_HILOGD("No hook for event type %{public}u", HOOK_EVENT_TYPE_KEY);
            return false;
        }
        snapshot.assign(it->second.begin(), it->second.end());
    }
    // A head-of-chain decline (gate skip or send failure) does not end interception: offer the event
    // along the chain one by one; only when the whole chain declines return false so the caller
    // dispatches normally - the event is never bypassed or dropped
    for (const auto &hook : snapshot) {
        if (hook != nullptr && hook->OnKeyEvent(keyEvent)) {
            return true;
        }
    }
    return false;
}

bool InputEventHookManager::HandleHooks(const std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPF(pointerEvent);
    HookEventType hookEventType { HOOK_EVENT_TYPE_NONE };
    if (auto sourceType = pointerEvent->GetSourceType(); sourceType == PointerEvent::SOURCE_TYPE_MOUSE) {
        hookEventType = IsAxisEvent(pointerEvent->GetPointerAction()) ?
            HOOK_EVENT_TYPE_AXIS : HOOK_EVENT_TYPE_MOUSE;
    } else if (sourceType == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) {
        hookEventType = HOOK_EVENT_TYPE_TOUCH;
    } else {
        MMI_HILOGW("Unsupported sourceType");
        return false;
    }
    std::vector<std::shared_ptr<InputEventHook>> snapshot;
    {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        auto it = hooks_.find(hookEventType);
        if (it == hooks_.end()) {
            MMI_HILOGD("No hook for event type %{public}u", hookEventType);
            return false;
        }
        snapshot.assign(it->second.begin(), it->second.end());
    }
    for (const auto &hook : snapshot) {
        if (hook != nullptr && hook->OnPointerEvent(pointerEvent)) {
            return true;
        }
    }
    return false;
}
} // namespace MMI
} // namespace OHOS
