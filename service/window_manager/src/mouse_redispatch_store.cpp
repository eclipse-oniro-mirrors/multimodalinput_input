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

#include "mouse_redispatch_store.h"

#include "input_windows_manager.h"
#include "mmi_log.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_WINDOW
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "MouseRedispatchStore"

namespace OHOS {
namespace MMI {
MouseRedispatchStore::Guard::Guard(const std::shared_ptr<PointerEvent>& event)
{
    auto& store = WIN_MGR->GetMouseRedispatchStore();
    store.active_ = true;
    store.zOrder_ = event->GetZOrder();
}

MouseRedispatchStore::Guard::~Guard()
{
    auto& store = WIN_MGR->GetMouseRedispatchStore();
    store.active_ = false;
    store.zOrder_ = 0.0f;
}

void MouseRedispatchStore::CacheLastEvent(const std::shared_ptr<PointerEvent>& event)
{
    lastEventMap_[zOrder_] = event;
}

std::shared_ptr<PointerEvent> MouseRedispatchStore::GetLastEvent() const
{
    auto iter = lastEventMap_.find(zOrder_);
    if (iter != lastEventMap_.end()) {
        return iter->second;
    }
    return nullptr;
}

std::shared_ptr<PointerEvent> MouseRedispatchStore::GetLastEvent(float zOrder) const
{
    auto iter = lastEventMap_.find(zOrder);
    if (iter != lastEventMap_.end()) {
        return iter->second;
    }
    return nullptr;
}

void MouseRedispatchStore::SetAxisBeginWindow(const std::optional<WindowInfo>& window)
{
    axisBeginWindowMap_[zOrder_] = window;
}

std::optional<WindowInfo> MouseRedispatchStore::GetAxisBeginWindow() const
{
    auto iter = axisBeginWindowMap_.find(zOrder_);
    if (iter != axisBeginWindowMap_.end()) {
        return iter->second;
    }
    return std::nullopt;
}

void MouseRedispatchStore::EraseAxisBeginWindow(float zOrder)
{
    axisBeginWindowMap_.erase(zOrder);
}

const std::map<float, std::optional<WindowInfo>>& MouseRedispatchStore::GetAxisBeginWindowMap() const
{
    return axisBeginWindowMap_;
}

void MouseRedispatchStore::SetButtonDownWindow(float zOrder, const WindowInfo& window)
{
    buttonDownWindowMap_[zOrder] = window;
}

std::optional<WindowInfo> MouseRedispatchStore::GetButtonDownWindow(float zOrder) const
{
    auto iter = buttonDownWindowMap_.find(zOrder);
    if (iter != buttonDownWindowMap_.end()) {
        return iter->second;
    }
    return std::nullopt;
}

void MouseRedispatchStore::EraseButtonDownWindow(float zOrder)
{
    buttonDownWindowMap_.erase(zOrder);
}

float MouseRedispatchStore::FindButtonSessionZOrder(int32_t windowId) const
{
    for (const auto& [zOrder, window] : buttonDownWindowMap_) {
        if (window && window->id == windowId) {
            return zOrder;
        }
    }
    return 0.0f;
}

const std::set<int32_t>& MouseRedispatchStore::GetActiveWindows(MouseSessionType sessionType) const
{
    return (sessionType == MouseSessionType::BUTTON) ? activeButtonWindows_ : activeAxisWindows_;
}

std::set<int32_t>& MouseRedispatchStore::GetActiveWindows(MouseSessionType sessionType)
{
    return (sessionType == MouseSessionType::BUTTON) ? activeButtonWindows_ : activeAxisWindows_;
}

void MouseRedispatchStore::SetWindowActive(int32_t windowId)
{
    SetWindowActive(MouseSessionType::AXIS, windowId);
}

void MouseRedispatchStore::SetWindowActive(MouseSessionType sessionType, int32_t windowId)
{
    GetActiveWindows(sessionType).insert(windowId);
}

bool MouseRedispatchStore::IsWindowActive(int32_t windowId) const
{
    return IsWindowActive(MouseSessionType::AXIS, windowId);
}

bool MouseRedispatchStore::IsWindowActive(MouseSessionType sessionType, int32_t windowId) const
{
    return GetActiveWindows(sessionType).count(windowId) > 0;
}

void MouseRedispatchStore::DeactivateWindow(int32_t windowId)
{
    DeactivateWindow(MouseSessionType::AXIS, windowId);
}

void MouseRedispatchStore::DeactivateWindow(MouseSessionType sessionType, int32_t windowId)
{
    GetActiveWindows(sessionType).erase(windowId);
}

bool MouseRedispatchStore::Abandon(const std::shared_ptr<PointerEvent>& pointerEvent)
{
    CHKPF(pointerEvent);
    int32_t action = pointerEvent->GetPointerAction();
    int32_t windowId = pointerEvent->GetTargetWindowId();
    bool pressed = !pointerEvent->GetPressedButtons().empty();
    PointerEvent::PointerItem pointerItem;
    int32_t toolType = PointerEvent::TOOL_TYPE_MOUSE;
    if (pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
        toolType = pointerItem.GetToolType();
    }
    if (action == PointerEvent::POINTER_ACTION_AXIS_BEGIN ||
        action == PointerEvent::POINTER_ACTION_AXIS_UPDATE ||
        action == PointerEvent::POINTER_ACTION_AXIS_END) {
        if (toolType != PointerEvent::TOOL_TYPE_TOUCHPAD) {
            MMI_HILOGD("Abandon mouse redispatch, mouse wheel is not supported, toolType:%{public}d", toolType);
            return true;
        }
        if (!IsWindowActive(MouseSessionType::AXIS, windowId)) {
            MMI_HILOGD("Abandon mouse redispatch, axis window:%{public}d not active", windowId);
            return true;
        }
        if (action == PointerEvent::POINTER_ACTION_AXIS_END) {
            DeactivateWindow(MouseSessionType::AXIS, windowId);
        }
        return false;
    }
    if (action == PointerEvent::POINTER_ACTION_BUTTON_DOWN || pressed ||
        action == PointerEvent::POINTER_ACTION_BUTTON_UP) {
        auto buttons = pointerEvent->GetPressedButtons();
        bool primaryOnly = (buttons.size() == 1 && buttons.count(PointerEvent::MOUSE_BUTTON_LEFT) > 0);
        bool primaryUp = (action == PointerEvent::POINTER_ACTION_BUTTON_UP && buttons.empty());
        if (!primaryOnly && !primaryUp) {
            MMI_HILOGD("Abandon mouse redispatch, non-primary button is not supported, action:%{public}d", action);
            return true;
        }
        if (!IsWindowActive(MouseSessionType::BUTTON, windowId)) {
            MMI_HILOGD("Abandon mouse redispatch, button window:%{public}d not active", windowId);
            return true;
        }
        if (action == PointerEvent::POINTER_ACTION_BUTTON_UP && pointerEvent->GetPressedButtons().empty()) {
            DeactivateWindow(MouseSessionType::BUTTON, windowId);
            EraseButtonDownWindow(zOrder_);
            MMI_HILOGD("Deactivate button session on BUTTON_UP, window:%{public}d", windowId);
        }
        return false;
    }
    return false;
}

void MouseRedispatchStore::ClearDeviceEvents(int32_t deviceId)
{
    for (auto iter = lastEventMap_.begin(); iter != lastEventMap_.end();) {
        if (iter->second != nullptr && iter->second->GetDeviceId() == deviceId) {
            iter = lastEventMap_.erase(iter);
        } else {
            ++iter;
        }
    }
}

void MouseRedispatchStore::Reset()
{
    lastEventMap_.clear();
    axisBeginWindowMap_.clear();
    buttonDownWindowMap_.clear();
    activeAxisWindows_.clear();
    activeButtonWindows_.clear();
}
} // namespace MMI
} // namespace OHOS
