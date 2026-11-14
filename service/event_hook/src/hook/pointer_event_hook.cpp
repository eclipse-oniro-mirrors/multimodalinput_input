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

#include "pointer_event_hook.h"

#include "define_multimodal.h"
#include "error_multimodal.h"
#include "input_event_data_transformation.h"
#include "input_event_handler.h"
#include "mmi_log.h"
#include "proto.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_HANDLER
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "PointerEventHook"

namespace OHOS {
namespace MMI {
bool PointerEventHook::OnPointerEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPF(pointerEvent);
    auto pkt = std::make_shared<NetPacket>(MmiMessageId::INVALID);
    auto sourceType = pointerEvent->GetSourceType();
    auto hookEventType = GetHookEventType();
    if (sourceType == PointerEvent::SOURCE_TYPE_TOUCHSCREEN && hookEventType == HOOK_EVENT_TYPE_TOUCH) {
        pkt = std::make_shared<NetPacket>(MmiMessageId::ON_HOOK_TOUCH_EVENT);
    } else if (sourceType == PointerEvent::SOURCE_TYPE_MOUSE && hookEventType == HOOK_EVENT_TYPE_MOUSE) {
        pkt = std::make_shared<NetPacket>(MmiMessageId::ON_HOOK_MOUSE_EVENT);
    } else if (sourceType == PointerEvent::SOURCE_TYPE_MOUSE && hookEventType == HOOK_EVENT_TYPE_AXIS) {
        pkt = std::make_shared<NetPacket>(MmiMessageId::ON_HOOK_AXIS_EVENT);
    } else {
        MMI_HILOGE("Unsupported sourceType");
        return false;
    }
    CHKPF(pkt);
    if (pkt->ChkRWError()) {
        MMI_HILOGE("Packet write pointerEvent failed");
        return false;
    }
    if (InputEventDataTransformation::Marshalling(pointerEvent, *pkt) != RET_OK) {
        MMI_HILOGE("Packet pointer event failed, errCode:%{public}d", STREAM_BUF_WRITE_FAIL);
        return false;
    }
    if (!SendNetPacketToHook(*pkt)) {
        MMI_HILOGE("SendNetPacketToHook failed");
        return false;
    }
    // Timeout fallback: first dispatch stale stashed events that exceeded the threshold without a
    // receipt (nothing dropped, no disable), then register the new event
    SweepExpiredEvents();
    expirationChecker_.UpdateInputEvent(pointerEvent);
    return true;
}

int32_t PointerEventHook::DispatchToNextHandler(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPR(pointerEvent, RET_ERR);
    // A decline (send failure) is not a failure: offer the event along the chain one by one and
    // dispatch directly to the application at the chain tail, so nothing is dropped
    bool ret = false;
    for (auto hook = GetNextHook(); hook != nullptr; hook = hook->GetNextHook()) {
        if (hook->OnPointerEvent(pointerEvent)) {
            ret = true;
            break;
        }
    }
    if (!ret) {
        ret = DispatchDirectly(pointerEvent);
    }
    expirationChecker_.RemoveEvent(pointerEvent->GetId());
    MMI_HILOGD("DispatchToNextHandler res:%{public}s", ret ? "success" : "failed");
    return ret ? RET_OK : RET_ERR;
}

std::shared_ptr<InputEvent> PointerEventHook::GetStashEvent(int32_t eventId)
{
    return expirationChecker_.GetEvent(eventId);
}

void PointerEventHook::FlushStashEvents(std::shared_ptr<InputEventHook> nextHook)
{
    auto events = expirationChecker_.DrainStashEvents();
    for (const auto &event : events) {
        auto pointerEvent = std::static_pointer_cast<PointerEvent>(event);
        CHKPC(pointerEvent);
        bool forwarded = false;
        for (auto hook = nextHook; hook != nullptr; hook = hook->GetNextHook()) {
            if (hook->OnPointerEvent(pointerEvent)) {
                forwarded = true;
                break;
            }
        }
        if (!forwarded) {
            DispatchDirectly(pointerEvent);
        }
    }
}

int32_t PointerEventHook::HandleEventResult(int32_t eventId, bool isConsumed)
{
    auto event = expirationChecker_.GetEvent(eventId);
    CHKPR(event, RET_ERR); // Not in the stash: already dispatched by the timeout fallback; late receipt treated as handled
    if (isConsumed) {
        // Consume: do not dispatch further
        expirationChecker_.RemoveEvent(eventId);
        return RET_OK;
    }
    // Release: dispatch further
    return DispatchToNextHandler(std::static_pointer_cast<PointerEvent>(event));
}

void PointerEventHook::SweepExpiredEvents()
{
    auto events = expirationChecker_.TakeExpiredEvents();
    for (const auto &event : events) {
        DispatchDirectly(std::static_pointer_cast<PointerEvent>(event));
    }
}

bool PointerEventHook::DispatchDirectly(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPF(pointerEvent);
    auto eventDispatchHandler = InputHandler->GetEventDispatchHandler();
    CHKPF(eventDispatchHandler);
    int32_t sourceType = pointerEvent->GetSourceType();
    if (sourceType == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) {
        eventDispatchHandler->HandleTouchEvent(pointerEvent);
    } else if (sourceType == PointerEvent::SOURCE_TYPE_MOUSE) {
        eventDispatchHandler->HandlePointerEvent(pointerEvent);
    } else {
        MMI_HILOGW("Unsupported sourceType:%{public}d", sourceType);
    }
    MMI_HILOGD("Dispatch directly, id:%{public}d", pointerEvent->GetId());
    return true;
}
} // namespace MMI
} // namespace OHOS