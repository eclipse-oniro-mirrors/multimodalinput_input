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

#include "gesture_classifier.h"

#include "gesture_tracker.h"

namespace OHOS {
namespace MMI {
bool IsGestureStartEvent(HookEventType hookType, const std::shared_ptr<KeyEvent> &keyEvent,
    const GestureTracker *tracker)
{
    if (keyEvent == nullptr || keyEvent->GetKeyAction() != KeyEvent::KEY_ACTION_DOWN) {
        return false;
    }
    // An auto-repeat DOWN within an active gesture is a mid event: while the key is registered in
    // the tracker it follows the start-event verdict and does not reset or restart the stream.
    // Repeat flag sources: a DOWN injected by the KeyAutoRepeat timer carries IsRepeatKey(), injected
    // repeats carry IsRepeat(); the original DOWN was dispatched before SelectAutoRepeat set the flag,
    // so it carries no marker on the chain.
    const bool isRepeat = keyEvent->IsRepeatKey() || keyEvent->IsRepeat();
    return !(isRepeat && tracker != nullptr && tracker->IsKnown(hookType, keyEvent->GetKeyCode()));
}

bool IsGestureEndEvent(HookEventType hookType, const std::shared_ptr<KeyEvent> &keyEvent)
{
    if (keyEvent == nullptr) {
        return false;
    }
    auto action = keyEvent->GetKeyAction();
    return action == KeyEvent::KEY_ACTION_UP || action == KeyEvent::KEY_ACTION_CANCEL;
}

int32_t GetGestureKey(HookEventType hookType, const std::shared_ptr<KeyEvent> &keyEvent)
{
    return keyEvent != nullptr ? keyEvent->GetKeyCode() : -1;
}

bool IsGestureStartEvent(HookEventType hookType, const std::shared_ptr<PointerEvent> &pointerEvent)
{
    if (pointerEvent == nullptr) {
        return false;
    }
    auto action = pointerEvent->GetPointerAction();
    if (hookType == HOOK_EVENT_TYPE_TOUCH) {
        return action == PointerEvent::POINTER_ACTION_DOWN;
    }
    if (hookType == HOOK_EVENT_TYPE_AXIS) {
        return action == PointerEvent::POINTER_ACTION_AXIS_BEGIN;
    }
    // MOUSE: BUTTON_DOWN starts a stream; a buttonless MOVE (hover) is an independent event and needs
    // no leading BUTTON_DOWN. PULL_MOVE is a mid event of a pull/drag gesture, not a hover, and still
    // requires a leading start event.
    if (action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        return true;
    }
    return action == PointerEvent::POINTER_ACTION_MOVE &&
        pointerEvent->GetButtonId() == PointerEvent::BUTTON_NONE;
}

bool IsGestureEndEvent(HookEventType hookType, const std::shared_ptr<PointerEvent> &pointerEvent)
{
    if (pointerEvent == nullptr) {
        return false;
    }
    auto action = pointerEvent->GetPointerAction();
    if (hookType == HOOK_EVENT_TYPE_TOUCH) {
        return action == PointerEvent::POINTER_ACTION_UP || action == PointerEvent::POINTER_ACTION_CANCEL;
    }
    if (hookType == HOOK_EVENT_TYPE_AXIS) {
        return action == PointerEvent::POINTER_ACTION_AXIS_END;
    }
    // MOUSE
    return action == PointerEvent::POINTER_ACTION_BUTTON_UP || action == PointerEvent::POINTER_ACTION_CANCEL;
}

bool IsOneOffEvent(HookEventType hookType, const std::shared_ptr<PointerEvent> &pointerEvent)
{
    if (pointerEvent == nullptr) {
        return false;
    }
    return hookType == HOOK_EVENT_TYPE_MOUSE &&
        pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_MOVE &&
        pointerEvent->GetButtonId() == PointerEvent::BUTTON_NONE;
}

int32_t GetGestureKey(HookEventType hookType, const std::shared_ptr<PointerEvent> &pointerEvent)
{
    if (pointerEvent == nullptr) {
        return -1;
    }
    if (hookType == HOOK_EVENT_TYPE_MOUSE) {
        return pointerEvent->GetButtonId();
    }
    // TOUCH: the active-pointer pointerId is unique, distinguishing multi-finger gestures per pointer;
    // AXIS (wheel): a begin/update/end stream is a single scroll track sharing one pointerId, so the
    // whole stream is treated as one gesture (the BEGIN verdict carries over until END cleanup) by design;
    // if multi-source axis events ever need separate tracks, introduce a sourceType+pointerId composite key.
    return pointerEvent->GetPointerId();
}
} // namespace MMI
} // namespace OHOS
