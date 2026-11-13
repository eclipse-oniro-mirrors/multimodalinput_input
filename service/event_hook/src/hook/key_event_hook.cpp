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

#include "key_event_hook.h"

#include "define_multimodal.h"
#include "error_multimodal.h"
#include "input_event_data_transformation.h"
#include "input_event_handler.h"
#include "mmi_log.h"
#include "proto.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_HANDLER
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "KeyEventHook"

namespace OHOS {
namespace MMI {
bool KeyEventHook::OnKeyEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPF(keyEvent);
    NetPacket pkt(MmiMessageId::ON_INPUT_HOOK_KEY_EVENT);
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet write key event failed");
        return false;
    }
    if (InputEventDataTransformation::KeyEventToNetPacket(keyEvent, pkt) != RET_OK) {
        MMI_HILOGE("Packet key event failed, errCode:%{public}d", STREAM_BUF_WRITE_FAIL);
        return false;
    }
    if (!SendNetPacketToHook(pkt)) {
        MMI_HILOGE("SendNetPacketToHook failed");
        return false;
    }
    // Timeout fallback: first dispatch stale stashed events that exceeded the threshold without a
    // receipt (nothing dropped, no disable), then register the new event
    SweepExpiredEvents();
    expirationChecker_.UpdateInputEvent(keyEvent);
    return true;
}

int32_t KeyEventHook::DispatchToNextHandler(std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPR(keyEvent, RET_ERR);
    // A decline (send failure) is not a failure: offer the event along the chain one by one and
    // dispatch directly to the application at the chain tail, so nothing is dropped
    bool ret = false;
    for (auto hook = GetNextHook(); hook != nullptr; hook = hook->GetNextHook()) {
        if (hook->OnKeyEvent(keyEvent)) {
            ret = true;
            break;
        }
    }
    if (!ret) {
        ret = DispatchDirectly(keyEvent);
    }
    expirationChecker_.RemoveEvent(keyEvent->GetId());
    MMI_HILOGD("DispatchToNextHandler res:%{public}s", ret ? "success" : "failed");
    return ret ? RET_OK : RET_ERR;
}

std::shared_ptr<InputEvent> KeyEventHook::GetStashEvent(int32_t eventId)
{
    return expirationChecker_.GetEvent(eventId);
}

void KeyEventHook::FlushStashEvents(std::shared_ptr<InputEventHook> nextHook)
{
    auto events = expirationChecker_.DrainStashEvents();
    for (const auto &event : events) {
        auto keyEvent = std::static_pointer_cast<KeyEvent>(event);
        CHKPC(keyEvent);
        bool forwarded = false;
        for (auto hook = nextHook; hook != nullptr; hook = hook->GetNextHook()) {
            if (hook->OnKeyEvent(keyEvent)) {
                forwarded = true;
                break;
            }
        }
        if (!forwarded) {
            DispatchDirectly(keyEvent);
        }
    }
}

int32_t KeyEventHook::HandleEventResult(int32_t eventId, bool isConsumed)
{
    auto event = expirationChecker_.GetEvent(eventId);
    CHKPR(event, RET_ERR); // Not in the stash: already dispatched by the timeout fallback; late receipt treated as handled
    if (isConsumed) {
        // Consume: do not dispatch further
        expirationChecker_.RemoveEvent(eventId);
        return RET_OK;
    }
    // Release: dispatch further
    return DispatchToNextHandler(std::static_pointer_cast<KeyEvent>(event));
}

void KeyEventHook::SweepExpiredEvents()
{
    auto events = expirationChecker_.TakeExpiredEvents();
    for (const auto &event : events) {
        DispatchDirectly(std::static_pointer_cast<KeyEvent>(event));
    }
}

bool KeyEventHook::DispatchDirectly(std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPF(keyEvent);
    auto udsServerPtr = InputHandler->GetUDSServer();
    CHKPF(udsServerPtr);
    auto dispatchHandler = InputHandler->GetEventDispatchHandler();
    CHKPF(dispatchHandler);
    if (dispatchHandler->DispatchKeyEventPid(*udsServerPtr, keyEvent) != RET_OK) {
        MMI_HILOGE("DispatchKeyEventPid failed");
        return false;
    }
    return true;
}
} // namespace MMI
} // namespace OHOS
