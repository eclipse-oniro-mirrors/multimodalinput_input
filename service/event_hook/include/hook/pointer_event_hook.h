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

#ifndef POINTER_EVENT_HOOK_H
#define POINTER_EVENT_HOOK_H

#include "input_event_hook.h"
#include "expiration_checker.h"
#include "pointer_event.h"
#include "uds_session.h"

namespace OHOS {
namespace MMI {

class PointerEventHook: public InputEventHook {
public:
    PointerEventHook(SessionPtr session, HookEventType hookType, NextHookGetter nextHookGetter) : InputEventHook(
        session, hookType, nextHookGetter) { }
    bool OnPointerEvent(std::shared_ptr<PointerEvent> pointerEvent) override;
    int32_t DispatchToNextHandler(std::shared_ptr<PointerEvent> pointerEvent) override;
    std::shared_ptr<InputEvent> GetStashEvent(int32_t eventId) override;
    void FlushStashEvents(std::shared_ptr<InputEventHook> nextHook) override;
    int32_t HandleEventResult(int32_t eventId, bool isConsumed) override;

private:
    bool DispatchDirectly(std::shared_ptr<PointerEvent> pointerEvent);
    // Timeout fallback: stash events exceeding the server threshold without a receipt are dispatched
    // directly to the application (nothing dropped, no disable)
    void SweepExpiredEvents();

private:
    ExpirationChecker expirationChecker_;
};
} // namespace MMI
} // namespace OHOS
#endif // POINTER_EVENT_HOOK_H