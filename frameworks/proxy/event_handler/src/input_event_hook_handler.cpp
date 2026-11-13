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

#include "input_event_hook_handler.h"


#include "error_multimodal.h"
#include "mmi_log.h"
#include "multimodal_event_handler.h"
#include "multimodal_input_connect_manager.h"
#include "input_handler_type.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "InputEventHookHandler"

namespace OHOS {
namespace MMI {

InputEventHookHandler &InputEventHookHandler::GetInstance()
{
    static InputEventHookHandler instance;
    return instance;
}

int32_t InputEventHookHandler::AddInputEventHookLocal(std::shared_ptr<IInputEventConsumer> consumer,
    HookEventType hookEventType)
{
    CALL_DEBUG_ENTER;
    CHKPR(consumer, ERROR_INVALID_PARAMETER);
    {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        consumer_ = consumer;
    }
    if (hookEventType & HOOK_EVENT_TYPE_KEY) {
        AddKeyHook([consumer](std::shared_ptr<KeyEvent> event) {
            consumer->OnInputEvent(event);
        });
    }
    if (hookEventType & HOOK_EVENT_TYPE_MOUSE) {
        AddMouseHook([consumer](std::shared_ptr<PointerEvent> event) {
            consumer->OnInputEvent(event);
        });
    }
    if (hookEventType & HOOK_EVENT_TYPE_TOUCH) {
        AddTouchHook([consumer](std::shared_ptr<PointerEvent> event) {
            consumer->OnInputEvent(event);
        });
    }
    if (hookEventType & HOOK_EVENT_TYPE_AXIS) {
        AddAxisHook([consumer](std::shared_ptr<PointerEvent> event) {
            consumer->OnInputEvent(event);
        });
    }
    MMI_HILOGI("AddInputEventHookLocal success hookEventType:%{public}u", hookEventType);
    return RET_OK;
}

int32_t InputEventHookHandler::AddInputEventHook(std::shared_ptr<IInputEventConsumer> consumer,
    HookEventType hookEventType)
{
    CALL_DEBUG_ENTER;
    CHKPR(consumer, RET_ERR);
    if (!MMIEventHdl.InitClient()) {
        MMI_HILOGE("Client init failed");
        return RET_ERR;
    }
    if (currentHookStats_.load(std::memory_order_relaxed) != 0) {
        MMI_HILOGE("Hook already existed, hookEventType:%{public}u", hookEventType);
        return ERROR_REPEAT_INTERCEPTOR;
    }
    if (int32_t ret = AddInputEventHookToServer(hookEventType); ret != RET_OK) {
        MMI_HILOGE("AddInputEventHook failed hookEventType:%{public}u", hookEventType);
        RemoveInputEventHookLocal(hookEventType);
        return ret;
    }
    if (int32_t ret = AddInputEventHookLocal(consumer, hookEventType); ret != RET_OK) {
        MMI_HILOGE("AddInputEventHook failed hookEventType:%{public}u", hookEventType);
        return ret;
    }
    MMI_HILOGI("AddInputEventHook success hookEventType:%{public}u", hookEventType);
    return RET_OK;
}

int32_t InputEventHookHandler::RemoveInputEventHookLocal(HookEventType hookEventType)
{
    CALL_DEBUG_ENTER;
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    // CheckHookStatsBit returns bool: never write (mask & bit) & CheckHookStatsBit(bit) - AND-ing
    // non-KEY bits with 0/1 always evaluates to 0, so TOUCH/MOUSE/AXIS bits would never be cleared
    if (((hookEventType & HOOK_EVENT_TYPE_KEY) != 0) && CheckHookStatsBit(HOOK_EVENT_TYPE_KEY)) {
        hookConsumer_.keyHookCallback_ = nullptr;
        ClearHookStatsBit(HOOK_EVENT_TYPE_KEY);
    }
    if (((hookEventType & HOOK_EVENT_TYPE_MOUSE) != 0) && CheckHookStatsBit(HOOK_EVENT_TYPE_MOUSE)) {
        hookConsumer_.mouseHookCallback_ = nullptr;
        ClearHookStatsBit(HOOK_EVENT_TYPE_MOUSE);
    }
    if (((hookEventType & HOOK_EVENT_TYPE_TOUCH) != 0) && CheckHookStatsBit(HOOK_EVENT_TYPE_TOUCH)) {
        hookConsumer_.touchHookCallback_ = nullptr;
        ClearHookStatsBit(HOOK_EVENT_TYPE_TOUCH);
    }
    if (((hookEventType & HOOK_EVENT_TYPE_AXIS) != 0) && CheckHookStatsBit(HOOK_EVENT_TYPE_AXIS)) {
        hookConsumer_.axisHookCallback_ = nullptr;
        ClearHookStatsBit(HOOK_EVENT_TYPE_AXIS);
    }
    return RET_OK;
}

int32_t InputEventHookHandler::RemoveInputEventHook(HookEventType hookEventType)
{
    if (int32_t ret = RemoveInputEventHookLocal(hookEventType); ret != RET_OK) {
        MMI_HILOGE("RemoveInputEventHookLocal failed ret:%{public}d, hookEventType:%{public}u", ret, hookEventType);
        return ret;
    }
    if (int32_t ret = RemoveInputEventHookOfServer(hookEventType); ret != RET_OK) {
        MMI_HILOGE("Remove hook failed ret:%{public}d, hookEventType:%{public}u", ret, hookEventType);
        return ret;
    }
    MMI_HILOGI("Remove hook success hookEventType:%{public}u", hookEventType);
    return RET_OK;
}

int32_t InputEventHookHandler::NotifyHookEventResult(int32_t eventId, HookEventType hookEventType, bool isConsumed)
{
    CALL_DEBUG_ENTER;
    return MULTIMODAL_INPUT_CONNECT_MGR->NotifyHookEventResult(eventId, hookEventType, isConsumed);
}

void InputEventHookHandler::OnKeyEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPV(keyEvent);
    std::shared_ptr<IInputEventConsumer> consumer;
    std::function<void(std::shared_ptr<KeyEvent>)> hookCallback;
    {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        if (hookConsumer_.keyHookCallback_ == nullptr) {
            // Callback already cleared (disable vs in-flight event race): report pass-through so the
            // server dispatches the stashed event immediately instead of waiting for the timeout
            NotifyHookEventResult(keyEvent->GetId(), HOOK_EVENT_TYPE_KEY, false);
            return;
        }
        // Snapshot the callback inside the lock and invoke it outside: a re-entrant Enable(false)/Destroy
        // inside the callback would take a unique_lock on rwMutex_ via RemoveInputEventHookLocal while this
        // thread still holds the shared_lock - the non-recursive shared_mutex would self-deadlock
        hookCallback = hookConsumer_.keyHookCallback_;
        consumer = consumer_;
    }
    hookCallback(keyEvent);
}

void InputEventHookHandler::OnPointerEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    auto sourceType = pointerEvent->GetSourceType();
    HookEventType hookEventType { HOOK_EVENT_TYPE_NONE };
    std::shared_ptr<IInputEventConsumer> consumer;
    std::function<void(std::shared_ptr<PointerEvent>)> hookCallback;
    {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        if (sourceType == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) {
            hookEventType = HOOK_EVENT_TYPE_TOUCH;
            if (hookConsumer_.touchHookCallback_ == nullptr) {
                // Callback already cleared (disable vs in-flight event race): report pass-through instead
                // of waiting for the timeout
                NotifyHookEventResult(pointerEvent->GetId(), HOOK_EVENT_TYPE_TOUCH, false);
                return;
            }
            // Snapshot the callback inside the lock and invoke it outside (same as OnKeyEvent, prevents
            // self-deadlock from re-entrant Enable/Destroy inside the callback)
            hookCallback = hookConsumer_.touchHookCallback_;
        } else if (sourceType == PointerEvent::SOURCE_TYPE_MOUSE) {
            hookEventType = HOOK_EVENT_TYPE_MOUSE;
            if (hookConsumer_.mouseHookCallback_ == nullptr) {
                NotifyHookEventResult(pointerEvent->GetId(), HOOK_EVENT_TYPE_MOUSE, false);
                return;
            }
            hookCallback = hookConsumer_.mouseHookCallback_;
        } else {
            MMI_HILOGD("Unsupported sourceType:%{public}d", sourceType);
            return;
        }
        consumer = consumer_;
    }
    hookCallback(pointerEvent);
}

void InputEventHookHandler::OnAxisEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    std::shared_ptr<IInputEventConsumer> consumer;
    std::function<void(std::shared_ptr<PointerEvent>)> hookCallback;
    {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        if (hookConsumer_.axisHookCallback_ == nullptr) {
            // 回调已清（禁用与在途事件竞态）：代为放行，不等 1.5S 超时（消费继承由服务端统一裁决）
            NotifyHookEventResult(pointerEvent->GetId(), HOOK_EVENT_TYPE_AXIS, false);
            return;
        }
        // 锁内仅快照回调，锁外执行（防回调内重入 Enable/Destroy 死锁，同 OnKeyEvent）
        hookCallback = hookConsumer_.axisHookCallback_;
        consumer = consumer_;
    }
    hookCallback(pointerEvent);
}

void InputEventHookHandler::OnConnected()
{
    CALL_DEBUG_ENTER;
    if (currentHookStats_.load() == 0) {
        MMI_HILOGI("No hook existed");
        return;
    }
    if (!MMIEventHdl.InitClient()) {
        MMI_HILOGE("Client init failed");
        return;
    }
    uint32_t hookEventType { 0 };
    {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        if (CheckHookStatsBit(HOOK_EVENT_TYPE_KEY) && hookConsumer_.keyHookCallback_) {
            hookEventType |= HOOK_EVENT_TYPE_KEY;
        }
        if (CheckHookStatsBit(HOOK_EVENT_TYPE_TOUCH) && hookConsumer_.touchHookCallback_) {
            hookEventType |= HOOK_EVENT_TYPE_TOUCH;
        }
        if (CheckHookStatsBit(HOOK_EVENT_TYPE_MOUSE) && hookConsumer_.mouseHookCallback_) {
            hookEventType |= HOOK_EVENT_TYPE_MOUSE;
        }
        if (CheckHookStatsBit(HOOK_EVENT_TYPE_AXIS) && hookConsumer_.axisHookCallback_) {
            hookEventType |= HOOK_EVENT_TYPE_AXIS;
        }
    }
    // Reconnect recovery: re-registers the surviving types; on failure falls back to disable + notify
    // so the business is not left believing the hook is still active
    if (int32_t ret = AddInputEventHookToServer(hookEventType); ret != RET_OK) {
        MMI_HILOGE("Re-add hook to server failed, ret:%{public}d, hookEventType:%{public}u", ret, hookEventType);
        std::shared_ptr<IInputEventConsumer> consumer;
        if (DisableHookLocal(hookEventType)) {
            std::shared_lock<std::shared_mutex> lock(rwMutex_);
            consumer = consumer_;
        }
        if (consumer) {
            consumer->OnHookDisabled(hookEventType);
        }
    }
}

void InputEventHookHandler::OnHookDisabledByServer(HookEventType hookEventType)
{
    CALL_DEBUG_ENTER;
    MMI_HILOGI("Hook disabled by server, hookEventType:%{public}u", hookEventType);
    std::shared_ptr<IInputEventConsumer> consumer;
    if (DisableHookLocal(hookEventType)) {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        consumer = consumer_;
    }
    if (consumer) {
        consumer->OnHookDisabled(hookEventType);
    }
}

bool InputEventHookHandler::DisableHookLocal(HookEventType hookEventType)
{
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    bool disabled { false };
    // CheckHookStatsBit returns bool: never write (mask & bit) & CheckHookStatsBit(bit) - AND-ing
    // non-KEY bits with 0/1 always evaluates to 0, so TOUCH/MOUSE/AXIS bits would never be cleared
    if (((hookEventType & HOOK_EVENT_TYPE_KEY) != 0) && CheckHookStatsBit(HOOK_EVENT_TYPE_KEY)) {
        hookConsumer_.keyHookCallback_ = nullptr;
        ClearHookStatsBit(HOOK_EVENT_TYPE_KEY);
        disabled = true;
    }
    if (((hookEventType & HOOK_EVENT_TYPE_MOUSE) != 0) && CheckHookStatsBit(HOOK_EVENT_TYPE_MOUSE)) {
        hookConsumer_.mouseHookCallback_ = nullptr;
        ClearHookStatsBit(HOOK_EVENT_TYPE_MOUSE);
        disabled = true;
    }
    if (((hookEventType & HOOK_EVENT_TYPE_TOUCH) != 0) && CheckHookStatsBit(HOOK_EVENT_TYPE_TOUCH)) {
        hookConsumer_.touchHookCallback_ = nullptr;
        ClearHookStatsBit(HOOK_EVENT_TYPE_TOUCH);
        disabled = true;
    }
    if (((hookEventType & HOOK_EVENT_TYPE_AXIS) != 0) && CheckHookStatsBit(HOOK_EVENT_TYPE_AXIS)) {
        hookConsumer_.axisHookCallback_ = nullptr;
        ClearHookStatsBit(HOOK_EVENT_TYPE_AXIS);
        disabled = true;
    }
    return disabled;
}

void InputEventHookHandler::AddKeyHook(std::function<void(std::shared_ptr<KeyEvent>)> keyHook)
{
    CHKPV(keyHook);
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    hookConsumer_.keyHookCallback_ = keyHook;
    SetHookStatsBit(HOOK_EVENT_TYPE_KEY);
}

void InputEventHookHandler::AddMouseHook(std::function<void(std::shared_ptr<PointerEvent>)> mouseHook)
{
    CHKPV(mouseHook);
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    hookConsumer_.mouseHookCallback_ = mouseHook;
    SetHookStatsBit(HOOK_EVENT_TYPE_MOUSE);
}

void InputEventHookHandler::AddTouchHook(std::function<void(std::shared_ptr<PointerEvent>)> touchHook)
{
    CHKPV(touchHook);
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    hookConsumer_.touchHookCallback_ = touchHook;
    SetHookStatsBit(HOOK_EVENT_TYPE_TOUCH);
}

void InputEventHookHandler::AddAxisHook(std::function<void(std::shared_ptr<PointerEvent>)> axisHook)
{
    CHKPV(axisHook);
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    hookConsumer_.axisHookCallback_ = axisHook;
    SetHookStatsBit(HOOK_EVENT_TYPE_AXIS);
}

int32_t InputEventHookHandler::AddInputEventHookToServer(HookEventType hookEventType)
{
    CALL_DEBUG_ENTER;
    if (int32_t ret = MULTIMODAL_INPUT_CONNECT_MGR->AddInputEventHook(hookEventType); ret != RET_OK) {
        MMI_HILOGE("AddInputEventHook to server, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t InputEventHookHandler::RemoveInputEventHookOfServer(HookEventType hookEventType)
{
    CALL_DEBUG_ENTER;
    if (int32_t ret = MULTIMODAL_INPUT_CONNECT_MGR->RemoveInputEventHook(hookEventType); ret != RET_OK) {
        MMI_HILOGE("RemoveInputEventHook of server, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

bool InputEventHookHandler::CheckHookStatsBit(HookEventType hookEventType)
{
    return (currentHookStats_.load(std::memory_order_relaxed) & static_cast<uint32_t>(hookEventType)) != 0;
}

void InputEventHookHandler::SetHookStatsBit(HookEventType hookEventType)
{
    currentHookStats_.fetch_or(static_cast<uint32_t>(hookEventType), std::memory_order_relaxed);
}

void InputEventHookHandler::ClearHookStatsBit(HookEventType hookEventType)
{
    currentHookStats_.fetch_and(~static_cast<uint32_t>(hookEventType), std::memory_order_relaxed);
}

bool InputEventHookHandler::IsHookExisted(HookEventType hookEventType)
{
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    if (bool flag = ((hookEventType & HOOK_EVENT_TYPE_KEY) != 0) &&
        CheckHookStatsBit(HOOK_EVENT_TYPE_KEY) && hookConsumer_.keyHookCallback_; flag) {
        return true;
    }
    if (bool flag = ((hookEventType & HOOK_EVENT_TYPE_TOUCH) != 0) &&
        CheckHookStatsBit(HOOK_EVENT_TYPE_TOUCH) && hookConsumer_.touchHookCallback_; flag) {
        return true;
    }
    if (bool flag = ((hookEventType & HOOK_EVENT_TYPE_MOUSE) != 0) &&
        CheckHookStatsBit(HOOK_EVENT_TYPE_MOUSE) && hookConsumer_.mouseHookCallback_; flag) {
        return true;
    }
    if (bool flag = ((hookEventType & HOOK_EVENT_TYPE_AXIS) != 0) &&
        CheckHookStatsBit(HOOK_EVENT_TYPE_AXIS) && hookConsumer_.axisHookCallback_; flag) {
        return true;
    }
    return false;
}

} // namespace MMI
} // namespace OHOS
