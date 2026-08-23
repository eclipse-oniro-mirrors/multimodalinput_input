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

#include "touch_arbitration_manager.h"

#include <vector>

#include "input_windows_manager.h"
#include "mmi_log.h"
#include "struct_multimodal.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_WINDOW
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "TouchArbitrationManager"

namespace OHOS {
namespace MMI {
TouchArbitrationManager::TouchArbitrationManager(InputWindowsManager *inputWindowsManager)
    : inputWindowsManager_(inputWindowsManager)
{
}

bool TouchArbitrationManager::IsRealTouchEvent(const std::shared_ptr<PointerEvent> &pointerEvent) const
{
    if (pointerEvent == nullptr) {
        MMI_HILOGE("PointerEvent is null");
        return false;
    }
    return pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHSCREEN &&
        pointerEvent->GetDeviceId() >= 0 &&
        pointerEvent->HasFlag(InputEvent::EVENT_FLAG_GENERATE_FROM_REAL) &&
        !pointerEvent->HasFlag(InputEvent::EVENT_FLAG_SIMULATE);
}

bool TouchArbitrationManager::IsVirtualTouchEvent(const std::shared_ptr<PointerEvent> &pointerEvent) const
{
    if (pointerEvent == nullptr) {
        MMI_HILOGE("PointerEvent is null");
        return false;
    }
    return pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHSCREEN &&
        pointerEvent->GetDeviceId() == VIRTUAL_TOUCH_DEVICE_ID;
}

TouchArbitrationSource TouchArbitrationManager::GetTouchArbitrationSource(
    const std::shared_ptr<PointerEvent> &pointerEvent) const
{
    if (pointerEvent == nullptr) {
        MMI_HILOGE("PointerEvent is null");
        return TouchArbitrationSource::OTHER;
    }
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
        return TouchArbitrationSource::OTHER;
    }
    if (pointerItem.GetToolType() != PointerEvent::TOOL_TYPE_FINGER) {
        return TouchArbitrationSource::OTHER;
    }
    if (IsRealTouchEvent(pointerEvent)) {
        return TouchArbitrationSource::REAL_TOUCH;
    }
    if (IsVirtualTouchEvent(pointerEvent)) {
        return TouchArbitrationSource::VIRTUAL_TOUCH;
    }
    return TouchArbitrationSource::OTHER;
}

bool TouchArbitrationManager::HasConflictingTouchSource(int32_t displayId, TouchArbitrationSource source) const
{
    auto displayIter = states_.find(displayId);
    if (displayIter == states_.end()) {
        return false;
    }
    for (const auto &[deviceId, deviceState] : displayIter->second.devices) {
        (void)deviceId;
        if (deviceState.source != source && deviceState.HasActivePointer()) {
            return true;
        }
    }
    return false;
}

bool TouchArbitrationManager::IsActiveTouchArbitrationStream(const std::shared_ptr<PointerEvent> &pointerEvent,
    TouchArbitrationSource source) const
{
    if (pointerEvent == nullptr) {
        MMI_HILOGE("PointerEvent is null");
        return false;
    }
    auto displayIter = states_.find(pointerEvent->GetTargetDisplayId());
    if (displayIter == states_.end()) {
        return false;
    }
    auto deviceIter = displayIter->second.devices.find(pointerEvent->GetDeviceId());
    if (deviceIter == displayIter->second.devices.end()) {
        return false;
    }
    return deviceIter->second.source == source &&
        deviceIter->second.IsPointerInState(pointerEvent->GetPointerId(), TouchArbitrationPointerState::ACTIVE);
}

void TouchArbitrationManager::HandleTouchSourceDownArbitration(
    const std::shared_ptr<PointerEvent> &pointerEvent)
{
    if (pointerEvent == nullptr) {
        MMI_HILOGE("PointerEvent is null");
        return;
    }
#ifdef OHOS_BUILD_ENABLE_ANCO
    if (pointerEvent->GetAncoDeal()) {
        return;
    }
#endif // OHOS_BUILD_ENABLE_ANCO
    const auto source = GetTouchArbitrationSource(pointerEvent);
    if (source == TouchArbitrationSource::OTHER) {
        return;
    }
    const int32_t displayId = pointerEvent->GetTargetDisplayId();
    if (!HasConflictingTouchSource(displayId, source)) {
        return;
    }
    const auto conflictingSource = source == TouchArbitrationSource::REAL_TOUCH ?
        TouchArbitrationSource::VIRTUAL_TOUCH : TouchArbitrationSource::REAL_TOUCH;
    CancelConflictingTouchSource(displayId, conflictingSource);
    MMI_HILOG_DISPATCHI("Touch source down conflict handled, action:%{public}d, source:%{public}d, "
        "conflictingSource:%{public}d, pointerId:%{public}d, displayId:%{public}d",
        pointerEvent->GetPointerAction(), static_cast<int32_t>(source), static_cast<int32_t>(conflictingSource),
        pointerEvent->GetPointerId(), displayId);
}

bool TouchArbitrationManager::HandleClosedTouchArbitrationStream(
    const std::shared_ptr<PointerEvent> &pointerEvent)
{
    if (pointerEvent == nullptr) {
        MMI_HILOGE("PointerEvent is null");
        return false;
    }
    const auto source = GetTouchArbitrationSource(pointerEvent);
    if (source == TouchArbitrationSource::OTHER) {
        return false;
    }
    const int32_t action = pointerEvent->GetPointerAction();
    if (action != PointerEvent::POINTER_ACTION_MOVE &&
        action != PointerEvent::POINTER_ACTION_UP &&
        action != PointerEvent::POINTER_ACTION_CANCEL) {
        return false;
    }

    auto displayIter = states_.find(pointerEvent->GetTargetDisplayId());
    if (displayIter == states_.end()) {
        return false;
    }
    auto deviceIter = displayIter->second.devices.find(pointerEvent->GetDeviceId());
    if (deviceIter == displayIter->second.devices.end() ||
        !deviceIter->second.IsPointerInState(pointerEvent->GetPointerId(),
        TouchArbitrationPointerState::CLOSED)) {
        return false;
    }
    PointerEvent::PointerItem pointerItem;
    if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_CONTROLLER)) {
        MarkClosedPointerItemsCanceled(pointerEvent);
    } else if (pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
        pointerItem.SetPressed(false);
        pointerItem.SetCanceled(true);
        pointerEvent->UpdatePointerItem(pointerEvent->GetPointerId(), pointerItem);
    }
    if (action == PointerEvent::POINTER_ACTION_UP || action == PointerEvent::POINTER_ACTION_CANCEL) {
        ClearTouchArbitrationPointerState(pointerEvent->GetTargetDisplayId(), source, pointerEvent->GetPointerId(),
            pointerEvent->GetDeviceId());
    }
    MMI_HILOG_DISPATCHD("Drop closed touch stream, action:%{public}d, deviceId:%{public}d, "
        "pointerId:%{public}d, displayId:%{public}d",
        action, pointerEvent->GetDeviceId(), pointerEvent->GetPointerId(), pointerEvent->GetTargetDisplayId());
    return true;
}

void TouchArbitrationManager::MarkClosedPointerItemsCanceled(const std::shared_ptr<PointerEvent> &pointerEvent)
{
    auto displayIter = states_.find(pointerEvent->GetTargetDisplayId());
    if (displayIter == states_.end()) {
        return;
    }
    auto deviceIter = displayIter->second.devices.find(pointerEvent->GetDeviceId());
    if (deviceIter == displayIter->second.devices.end()) {
        return;
    }
    // Controller injected events are rebuilt structures without stream state, and non-current
    // pointer items are always filled as pressed, so every closed pointer item in the event
    // must be marked canceled to keep downstream pointer state consistent.
    for (const auto &closedPointerId : pointerEvent->GetPointerIds()) {
        if (!deviceIter->second.IsPointerInState(closedPointerId, TouchArbitrationPointerState::CLOSED)) {
            continue;
        }
        PointerEvent::PointerItem closedItem;
        if (!pointerEvent->GetPointerItem(closedPointerId, closedItem)) {
            MMI_HILOG_DISPATCHE("Can't find closed pointer item, pointerId:%{public}d", closedPointerId);
            continue;
        }
        closedItem.SetPressed(false);
        closedItem.SetCanceled(true);
        pointerEvent->UpdatePointerItem(closedPointerId, closedItem);
    }
}

void TouchArbitrationManager::UpdateTouchArbitrationState(const std::shared_ptr<PointerEvent> &pointerEvent)
{
    if (pointerEvent == nullptr) {
        MMI_HILOGE("PointerEvent is null");
        return;
    }
#ifdef OHOS_BUILD_ENABLE_ANCO
    if (pointerEvent->GetAncoDeal()) {
        return;
    }
#endif // OHOS_BUILD_ENABLE_ANCO
    const auto source = GetTouchArbitrationSource(pointerEvent);
    if (source == TouchArbitrationSource::OTHER) {
        return;
    }
    if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_CONTROLLER)) {
        MarkClosedPointerItemsCanceled(pointerEvent);
    }
    const int32_t action = pointerEvent->GetPointerAction();
    const int32_t displayId = pointerEvent->GetTargetDisplayId();
    const int32_t pointerId = pointerEvent->GetPointerId();
    switch (action) {
        case PointerEvent::POINTER_ACTION_DOWN:
            break;
        case PointerEvent::POINTER_ACTION_MOVE:
            if (!IsActiveTouchArbitrationStream(pointerEvent, source)) {
                return;
            }
            break;
        case PointerEvent::POINTER_ACTION_UP:
        case PointerEvent::POINTER_ACTION_CANCEL:
        case PointerEvent::POINTER_ACTION_PULL_UP:
        case PointerEvent::POINTER_ACTION_PULL_CANCEL:
            ClearTouchArbitrationPointerState(displayId, source, pointerId, pointerEvent->GetDeviceId());
            return;
        default:
            return;
    }
    auto &deviceState = states_[displayId].devices[pointerEvent->GetDeviceId()];
    deviceState.source = source;
    deviceState.currentEvent = pointerEvent;
    deviceState.SetPointerState(pointerId, TouchArbitrationPointerState::ACTIVE);
}

void TouchArbitrationManager::ClearTouchArbitrationPointerState(int32_t displayId, TouchArbitrationSource source,
    int32_t pointerId, int32_t deviceId)
{
    auto displayIter = states_.find(displayId);
    if (displayIter == states_.end()) {
        return;
    }
    auto deviceIter = displayIter->second.devices.find(deviceId);
    if (deviceIter == displayIter->second.devices.end()) {
        return;
    }
    if (deviceIter->second.source != source) {
        return;
    }
    if (!deviceIter->second.ClearPointerState(pointerId)) {
        return;
    }
    if (deviceIter->second.pointerStates.empty()) {
        displayIter->second.devices.erase(deviceIter);
    }
    if (displayIter->second.devices.empty()) {
        states_.erase(displayIter);
    }
}

void TouchArbitrationManager::ClearTouchArbitrationStateByDevice(int32_t deviceId)
{
    for (auto displayIter = states_.begin(); displayIter != states_.end();) {
        displayIter->second.devices.erase(deviceId);
        if (displayIter->second.devices.empty()) {
            displayIter = states_.erase(displayIter);
        } else {
            ++displayIter;
        }
    }
}

void TouchArbitrationManager::CancelConflictingTouchSource(int32_t displayId, TouchArbitrationSource source)
{
    auto displayIter = states_.find(displayId);
    if (displayIter == states_.end()) {
        return;
    }
    struct TouchArbitrationCancelInfo {
        int32_t deviceId { -1 };
        int32_t pointerId { -1 };
        std::shared_ptr<PointerEvent> currentEvent { nullptr };
    };
    std::vector<TouchArbitrationCancelInfo> cancelInfos;
    for (const auto &[deviceId, deviceState] : displayIter->second.devices) {
        if (deviceState.source != source) {
            continue;
        }
        for (const auto &[pointerId, pointerState] : deviceState.pointerStates) {
            if (pointerState != TouchArbitrationPointerState::ACTIVE) {
                continue;
            }
            const auto currentEvent = deviceState.currentEvent;
            cancelInfos.push_back({ deviceId, pointerId, currentEvent });
            if (currentEvent == nullptr) {
                MMI_HILOG_DISPATCHE("Current touch event is null, deviceId:%{public}d, pointerId:%{public}d",
                    deviceId, pointerId);
                continue;
            }
            auto cancelEvent = std::make_shared<PointerEvent>(*currentEvent);
            inputWindowsManager_->DispatchTouchSourceCancel(pointerId, cancelEvent);
            PointerEvent::PointerItem pointerItem;
            if (!currentEvent->GetPointerItem(pointerId, pointerItem)) {
                MMI_HILOG_DISPATCHE("Can't find source pointer item, deviceId:%{public}d, pointerId:%{public}d",
                    deviceId, pointerId);
                continue;
            }
            pointerItem.SetPressed(false);
            pointerItem.SetCanceled(true);
            currentEvent->UpdatePointerItem(pointerId, pointerItem);
        }
    }
    for (const auto &cancelInfo : cancelInfos) {
        inputWindowsManager_->ClearTouchTargetState(displayId, cancelInfo.deviceId, cancelInfo.pointerId);
        auto deviceIter = displayIter->second.devices.find(cancelInfo.deviceId);
        if (deviceIter == displayIter->second.devices.end()) {
            continue;
        }
        deviceIter->second.SetPointerState(cancelInfo.pointerId, TouchArbitrationPointerState::CLOSED);
    }
}
} // namespace MMI
} // namespace OHOS
