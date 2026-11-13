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

#ifndef INPUT_EVENT_HOOK_H
#define INPUT_EVENT_HOOK_H
#include <shared_mutex>
#include "axis_event.h"
#include "input_event.h"
#include "input_handler_type.h"
#include "key_event.h"
#include "pointer_event.h"
#include "uds_session.h"

namespace OHOS {
namespace MMI {
class InputEventHook;

using NextHookGetter = std::function<std::shared_ptr<InputEventHook>(std::shared_ptr<InputEventHook>)>;

class InputEventHook : public std::enable_shared_from_this<InputEventHook> {
public:
    InputEventHook(SessionPtr session, HookEventType hookType, NextHookGetter nextHookGetter) : session_(session),
        hookEventType_(hookType), nextHookGetter_(nextHookGetter) { }
    virtual ~InputEventHook();
    virtual bool OnKeyEvent(std::shared_ptr<KeyEvent> keyEvent);
    virtual bool OnPointerEvent(std::shared_ptr<PointerEvent> mouseEvent);
    virtual int32_t DispatchToNextHandler(const std::shared_ptr<KeyEvent> keyEvent);
    virtual int32_t DispatchToNextHandler(const std::shared_ptr<PointerEvent> pointerEvent);
    virtual void FlushStashEvents(std::shared_ptr<InputEventHook> nextHook) {}
    virtual int32_t HandleEventResult(int32_t eventId, bool isConsumed);
    virtual std::shared_ptr<InputEvent> GetStashEvent(int32_t eventId) { return nullptr; }
    HookEventType GetHookEventType();
    int32_t GetHookPid();
    bool SendNetPacketToHook(NetPacket &pkt);
    std::shared_ptr<InputEventHook> GetNextHook();
    std::string GetProgramName();
    // Screen-lock suspend: in-flight events are dispatched directly to the target application
    // (nothing dropped; the whole hook chain pauses without cascading into the next hook); the
    // registration is kept without notifying the business, and interception resumes after unlock
    void Suspend();

private:
    std::shared_mutex rwMutex_;
    SessionPtr session_ { nullptr };
    HookEventType hookEventType_ { HOOK_EVENT_TYPE_NONE };
    NextHookGetter nextHookGetter_ { nullptr };
};
} // namespace MMI
} // namespace OHOS
#endif // INPUT_EVENT_HOOK_H