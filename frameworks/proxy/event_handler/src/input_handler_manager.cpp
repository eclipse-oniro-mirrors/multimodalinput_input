/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "input_handler_manager.h"

#include <cinttypes>

#include "bytrace_adapter.h"
#include "input_handler_type.h"
#include "anr_handler.h"
#include "multimodal_event_handler.h"
#include "multimodal_input_connect_manager.h"
#include "mmi_log.h"
#include "napi_constants.h"
#include "net_packet.h"
#include "proto.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "InputHandlerManager" };
} // namespace

InputHandlerManager::InputHandlerManager()
{
    monitorCallback_ =
        std::bind(&InputHandlerManager::OnDispatchEventProcessed, this, std::placeholders::_1, std::placeholders::_2);
}

int32_t InputHandlerManager::AddHandler(InputHandlerType handlerType, std::shared_ptr<IInputEventConsumer> consumer,
    HandleEventType eventType, int32_t priority, uint32_t deviceTags)
{
    CALL_INFO_TRACE;
    CHKPR(consumer, INVALID_HANDLER_ID);
    eventType = HANDLE_EVENT_TYPE_NONE;
    if ((deviceTags & CapabilityToTags(InputDeviceCapability::INPUT_DEV_CAP_KEYBOARD)) != 0) {
        eventType |= HANDLE_EVENT_TYPE_KEY;
    }
    if ((deviceTags & (CapabilityToTags(InputDeviceCapability::INPUT_DEV_CAP_MAX) -
        CapabilityToTags(InputDeviceCapability::INPUT_DEV_CAP_KEYBOARD))) != 0) {
        eventType |= HANDLE_EVENT_TYPE_POINTER;
    }
    std::lock_guard<std::mutex> guard(mtxHandlers_);
    if ((monitorHandlers_.size() + interHandlers_.size()) >= MAX_N_INPUT_HANDLERS) {
        MMI_HILOGE("The number of handlers exceeds the maximum");
        return ERROR_EXCEED_MAX_COUNT;
    }
    int32_t handlerId = GetNextId();
    if (handlerId == INVALID_HANDLER_ID) {
        MMI_HILOGE("Exceeded limit of 32-bit maximum number of integers");
        return INVALID_HANDLER_ID;
    }
    if (eventType == HANDLE_EVENT_TYPE_NONE) {
        MMI_HILOGE("Invalid event type");
        return INVALID_HANDLER_ID;
    }
    const HandleEventType currentType = GetEventType();
    MMI_HILOGD("Register new handler:%{public}d, currentType:%{public}d, deviceTags:%{public}d", handlerId, currentType,
        deviceTags);
    uint32_t currentTags = GetDeviceTags();
    if (RET_OK == AddLocal(handlerId, handlerType, eventType, priority, deviceTags, consumer)) {
        MMI_HILOGD("New handler successfully registered, report to server");
        const HandleEventType newType = GetEventType();
        if (currentType != newType || ((currentTags & deviceTags) != deviceTags)) {
            deviceTags = GetDeviceTags();
            MMI_HILOGD("handlerType:%{public}d, newType:%{public}d, deviceTags:%{public}d, priority:%{public}d",
                handlerType, newType, deviceTags, priority);
            int32_t ret = AddToServer(handlerType, newType, priority, deviceTags);
            if (ret != RET_OK) {
                MMI_HILOGD("Handler:%{public}d permissions failed, remove the monitor", handlerId);
                RemoveLocal(handlerId, handlerType, deviceTags);
                return ret;
            }
        }
    } else {
        handlerId = INVALID_HANDLER_ID;
    }
    return handlerId;
}

void InputHandlerManager::RemoveHandler(int32_t handlerId, InputHandlerType handlerType)
{
    CALL_INFO_TRACE;
    MMI_HILOGD("Unregister handler:%{public}d,type:%{public}d", handlerId, handlerType);
    std::lock_guard<std::mutex> guard(mtxHandlers_);
    const HandleEventType currentType = GetEventType();
    uint32_t currentTags = GetDeviceTags();
    uint32_t deviceTags = 0;
    if (RET_OK == RemoveLocal(handlerId, handlerType, deviceTags)) {
        MMI_HILOGD("Handler:%{public}d deviceTags:%{public}d unregistered, report to server", handlerId, deviceTags);
        const HandleEventType newType = GetEventType();
        const int32_t newLevel = GetPriority();
        const uint64_t newTags = GetDeviceTags();
        if (currentType != newType || ((currentTags & deviceTags) != 0)) {
            RemoveFromServer(handlerType, newType, newLevel, newTags);
        }
    }
}

int32_t InputHandlerManager::AddLocal(int32_t handlerId, InputHandlerType handlerType, HandleEventType eventType,
    int32_t priority, uint32_t deviceTags, std::shared_ptr<IInputEventConsumer> monitor)
{
    InputHandlerManager::Handler handler{
        .handlerId_ = handlerId,
        .handlerType_ = handlerType,
        .eventType_ = eventType,
        .priority_ = priority,
        .deviceTags_ = deviceTags,
        .consumer_ = monitor,
    };
    if (handlerType == InputHandlerType::MONITOR) {
        auto ret = monitorHandlers_.emplace(handler.handlerId_, handler);
        if (!ret.second) {
            MMI_HILOGE("Duplicate handler:%{public}d", handler.handlerId_);
            return RET_ERR;
        }
    }
    if (handlerType == InputHandlerType::INTERCEPTOR) {
        auto iterIndex = interHandlers_.begin();
        for (; iterIndex != interHandlers_.end(); ++iterIndex) {
            if (handler.priority_ < iterIndex->priority_) {
                break;
            }
        }
        auto iter = interHandlers_.emplace(iterIndex, handler);
        if (iter == interHandlers_.end()) {
            MMI_HILOGE("Add new handler failed");
            return RET_ERR;
        }
    }
    return RET_OK;
}

int32_t InputHandlerManager::AddToServer(InputHandlerType handlerType, HandleEventType eventType, int32_t priority,
    uint32_t deviceTags)
{
    int32_t ret = MultimodalInputConnMgr->AddInputHandler(handlerType, eventType, priority, deviceTags);
    if (ret != RET_OK) {
        MMI_HILOGE("Send to server failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t InputHandlerManager::RemoveLocal(int32_t handlerId, InputHandlerType handlerType, uint32_t &deviceTags)
{
    if (handlerType == InputHandlerType::MONITOR) {
        auto iter = monitorHandlers_.find(handlerId);
        if (iter == monitorHandlers_.end()) {
            MMI_HILOGE("No handler with specified");
            return RET_ERR;
        }
        if (handlerType != iter->second.handlerType_) {
            MMI_HILOGE("Unmatched handler type, InputHandlerType:%{public}d,FindHandlerType:%{public}d", handlerType,
                iter->second.handlerType_);
            return RET_ERR;
        }
        monitorHandlers_.erase(iter);
    }

    if (handlerType == InputHandlerType::INTERCEPTOR) {
        for (auto it = interHandlers_.begin(); it != interHandlers_.end(); ++it) {
            if (handlerId == it->handlerId_) {
                deviceTags = it->deviceTags_;
                interHandlers_.erase(it);
                break;
            }
        }
    }
    return RET_OK;
}

void InputHandlerManager::RemoveFromServer(InputHandlerType handlerType, HandleEventType eventType, int32_t priority,
    uint32_t deviceTags)
{
    int32_t ret = MultimodalInputConnMgr->RemoveInputHandler(handlerType, eventType, priority, deviceTags);
    if (ret != 0) {
        MMI_HILOGE("Send to server failed, ret:%{public}d", ret);
    }
}

int32_t InputHandlerManager::GetNextId()
{
    if (nextId_ == std::numeric_limits<int32_t>::max()) {
        MMI_HILOGE("Exceeded limit of 32-bit maximum number of integers");
        return INVALID_HANDLER_ID;
    }
    return nextId_++;
}

std::shared_ptr<IInputEventConsumer> InputHandlerManager::FindHandler(int32_t handlerId)
{
    if (GetHandlerType() == InputHandlerType::MONITOR) {
        auto iter = monitorHandlers_.find(handlerId);
        if (iter != monitorHandlers_.end()) {
            return iter->second.consumer_;
        }
    }
    if (GetHandlerType() == InputHandlerType::INTERCEPTOR) {
        for (const auto &item : interHandlers_) {
            if (item.handlerId_ == handlerId) {
                return item.consumer_;
            }
        }
    }
    return nullptr;
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
void InputHandlerManager::OnInputEvent(std::shared_ptr<KeyEvent> keyEvent, uint32_t deviceTags)
{
    CHK_PID_AND_TID();
    CHKPV(keyEvent);
    std::lock_guard<std::mutex> guard(mtxHandlers_);
    BytraceAdapter::StartBytrace(keyEvent, BytraceAdapter::TRACE_STOP, BytraceAdapter::KEY_INTERCEPT_EVENT);
    if (GetHandlerType() == InputHandlerType::MONITOR) {
        for (const auto &item : monitorHandlers_) {
            if ((item.second.eventType_ & HANDLE_EVENT_TYPE_KEY) != HANDLE_EVENT_TYPE_KEY) {
                continue;
            }
            int32_t handlerId = item.first;
            std::shared_ptr<IInputEventConsumer> consumer = item.second.consumer_;
            CHKPV(consumer);
            consumer->OnInputEvent(keyEvent);
            MMI_HILOGD("Key event id:%{public}d keyCode:%{public}d", handlerId, keyEvent->GetKeyCode());
        }
    }
    if (GetHandlerType() == InputHandlerType::INTERCEPTOR) {
        for (const auto &item : interHandlers_) {
            if ((item.eventType_ & HANDLE_EVENT_TYPE_KEY) != HANDLE_EVENT_TYPE_KEY) {
                continue;
            }
            int32_t handlerId = item.handlerId_;
            std::shared_ptr<IInputEventConsumer> consumer = item.consumer_;
            CHKPV(consumer);
            consumer->OnInputEvent(keyEvent);
            MMI_HILOGD("Key event id:%{public}d keyCode:%{public}d", handlerId, keyEvent->GetKeyCode());
            break;
        }
    }
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
bool InputHandlerManager::CheckInputDeviceSource(
    const std::shared_ptr<PointerEvent> pointerEvent, uint32_t deviceTags) const
{
    if ((pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) &&
        ((deviceTags & CapabilityToTags(InputDeviceCapability::INPUT_DEV_CAP_TOUCH)) ||
        (deviceTags & CapabilityToTags(InputDeviceCapability::INPUT_DEV_CAP_TABLET_TOOL)))) {
        return true;
    } else if ((pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_MOUSE) &&
        (deviceTags & CapabilityToTags(InputDeviceCapability::INPUT_DEV_CAP_POINTER))) {
        return true;
    } else if ((pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHPAD) &&
        (deviceTags & CapabilityToTags(InputDeviceCapability::INPUT_DEV_CAP_POINTER))) {
        return true;
    }
    return false;
}

void InputHandlerManager::GetConsumerInfos(std::shared_ptr<PointerEvent> pointerEvent, uint32_t deviceTags,
    std::map<int32_t, std::shared_ptr<IInputEventConsumer>> &consumerInfos)
{
    std::lock_guard<std::mutex> guard(mtxHandlers_);
    int32_t consumerCount = 0;
    if (GetHandlerType() == InputHandlerType::MONITOR) {
        consumerCount = GetMonitorConsumerInfos(pointerEvent, consumerInfos);
    }
    if (GetHandlerType() == InputHandlerType::INTERCEPTOR) {
        for (const auto &item : interHandlers_) {
            if ((item.eventType_ & HANDLE_EVENT_TYPE_POINTER) != HANDLE_EVENT_TYPE_POINTER) {
                continue;
            }
            if (((deviceTags & item.deviceTags_) == item.deviceTags_) &&
                !CheckInputDeviceSource(pointerEvent, item.deviceTags_)) {
                continue;
            }
            int32_t handlerId = item.handlerId_;
            std::shared_ptr<IInputEventConsumer> consumer = item.consumer_;
            CHKPV(consumer);
            auto ret = consumerInfos.emplace(handlerId, consumer);
            if (!ret.second) {
                MMI_HILOGI("Duplicate handler:%{public}d", handlerId);
                continue;
            }
            consumerCount++;
            break;
        }
    }

    if (consumerCount == 0) {
        MMI_HILOGE("All task post failed");
        return;
    }
    int32_t tokenType = MultimodalInputConnMgr->GetTokenType();
    if (tokenType != TokenType::TOKEN_HAP && tokenType != TokenType::TOKEN_SYSTEM_HAP) {
        return;
    }
    AddMouseEventId(pointerEvent);
    AddProcessedEventId(pointerEvent, consumerCount);
}

void InputHandlerManager::AddMouseEventId(std::shared_ptr<PointerEvent> pointerEvent)
{
    if (pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_MOUSE) {
        mouseEventIds_.emplace(pointerEvent->GetId());
    }
}

void InputHandlerManager::AddProcessedEventId(std::shared_ptr<PointerEvent> pointerEvent, int32_t consumerCount)
{
    if (pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHSCREEN ||
        pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHPAD) {
        processedEvents_.emplace(pointerEvent->GetId(), consumerCount);
    }
}

int32_t InputHandlerManager::GetMonitorConsumerInfos(std::shared_ptr<PointerEvent> pointerEvent,
    std::map<int32_t, std::shared_ptr<IInputEventConsumer>> &consumerInfos)
{
    int32_t consumerCount = 0;
    for (const auto &item : monitorHandlers_) {
        if ((item.second.eventType_ & HANDLE_EVENT_TYPE_POINTER) != HANDLE_EVENT_TYPE_POINTER) {
            continue;
        }
        int32_t handlerId = item.first;
        std::shared_ptr<IInputEventConsumer> consumer = item.second.consumer_;
        CHKPR(consumer, INVALID_HANDLER_ID);
        auto ret = consumerInfos.emplace(handlerId, consumer);
        if (!ret.second) {
            MMI_HILOGI("Duplicate handler:%{public}d", handlerId);
            continue;
        }
        consumerCount++;
    }
    return consumerCount;
}

void InputHandlerManager::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent, uint32_t deviceTags)
{
    CHK_PID_AND_TID();
    CHKPV(pointerEvent);
    BytraceAdapter::StartBytrace(pointerEvent, BytraceAdapter::TRACE_STOP, BytraceAdapter::POINT_INTERCEPT_EVENT);
    std::map<int32_t, std::shared_ptr<IInputEventConsumer>> consumerInfos;
    GetConsumerInfos(pointerEvent, deviceTags, consumerInfos);
    for (const auto &iter : consumerInfos) {
        auto tempEvent = std::make_shared<PointerEvent>(*pointerEvent);
        tempEvent->SetProcessedCallback(monitorCallback_);
        CHKPV(iter.second);
        auto consumer = iter.second;
        consumer->OnInputEvent(tempEvent);
        MMI_HILOGD("Pointer event id:%{public}d pointerId:%{public}d", iter.first, pointerEvent->GetPointerId());
    }
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

#if defined(OHOS_BUILD_ENABLE_INTERCEPTOR) || defined(OHOS_BUILD_ENABLE_MONITOR)
void InputHandlerManager::OnConnected()
{
    CALL_DEBUG_ENTER;
    HandleEventType eventType = GetEventType();
    int32_t priority = GetPriority();
    uint32_t deviceTags = GetDeviceTags();
    if (eventType != HANDLE_EVENT_TYPE_NONE) {
        AddToServer(GetHandlerType(), eventType, priority, deviceTags);
    }
}
#endif // OHOS_BUILD_ENABLE_INTERCEPTOR || OHOS_BUILD_ENABLE_MONITOR

bool InputHandlerManager::HasHandler(int32_t handlerId)
{
    std::lock_guard<std::mutex> guard(mtxHandlers_);
    if (GetHandlerType() == InputHandlerType::MONITOR) {
        auto iter = monitorHandlers_.find(handlerId);
        return (iter != monitorHandlers_.end());
    }
    if (GetHandlerType() == InputHandlerType::INTERCEPTOR) {
        for (const auto &item : interHandlers_) {
            if (item.handlerId_ == handlerId) {
                return true;
            }
        }
    }
    return false;
}

HandleEventType InputHandlerManager::GetEventType() const
{
    uint32_t eventType{ HANDLE_EVENT_TYPE_NONE };
    if (GetHandlerType() == InputHandlerType::MONITOR) {
        if (monitorHandlers_.empty()) {
            MMI_HILOGD("monitorHandlers_ is empty");
            return HANDLE_EVENT_TYPE_NONE;
        }
        for (const auto &inputHandler : monitorHandlers_) {
            eventType |= inputHandler.second.eventType_;
        }
    }

    if (GetHandlerType() == InputHandlerType::INTERCEPTOR) {
        if (interHandlers_.empty()) {
            MMI_HILOGD("interHandlers_ is empty");
            return HANDLE_EVENT_TYPE_NONE;
        }
        for (const auto &interHandler : interHandlers_) {
            eventType |= interHandler.eventType_;
        }
    }
    return eventType;
}

int32_t InputHandlerManager::GetPriority() const
{
    int32_t priority{ DEFUALT_INTERCEPTOR_PRIORITY };
    if (GetHandlerType() == InputHandlerType::INTERCEPTOR) {
        if (!interHandlers_.empty()) {
            priority = interHandlers_.front().priority_;
        }
    }
    return priority;
}

uint32_t InputHandlerManager::GetDeviceTags() const
{
    uint32_t deviceTags = 0;
    if (GetHandlerType() == InputHandlerType::INTERCEPTOR) {
        for (const auto &item : interHandlers_) {
            deviceTags |= item.deviceTags_;
        }
    }
    if (GetHandlerType() == InputHandlerType::MONITOR) {
        for (const auto &item : monitorHandlers_) {
            deviceTags |= item.second.deviceTags_;
        }
    }
    return deviceTags;
}

void InputHandlerManager::OnDispatchEventProcessed(int32_t eventId, int64_t actionTime)
{
    std::lock_guard<std::mutex> guard(mtxHandlers_);
    CALL_DEBUG_ENTER;
    MMIClientPtr client = MMIEventHdl.GetMMIClient();
    CHKPV(client);
    if (mouseEventIds_.find(eventId) != mouseEventIds_.end()) {
        mouseEventIds_.erase(eventId);
        return;
    }
    auto iter = processedEvents_.find(eventId);
    if (iter == processedEvents_.end()) {
        MMI_HILOGE("EventId not in processedEvents_");
        return;
    }
    int32_t count = iter->second;
    processedEvents_.erase(iter);
    count--;
    if (count > 0) {
        processedEvents_.emplace(eventId, count);
        return;
    }
    ANRHDL->SetLastProcessedEventId(ANR_MONITOR, eventId, actionTime);
}
} // namespace MMI
} // namespace OHOS
