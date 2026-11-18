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

#ifndef GESTURE_CLASSIFIER_H
#define GESTURE_CLASSIFIER_H

#include "input_handler_type.h"
#include "key_event.h"
#include "pointer_event.h"

namespace OHOS {
namespace MMI {
class GestureTracker;

// Gesture classification: identifies start/end/one-off events and the gesture key.
// Gesture key convention: KEY=keyCode, MOUSE=buttonId, TOUCH/AXIS=pointerId (same as the legacy
// GetGestureKey). Implemented in gesture_classifier.cpp.

// The KEY start-event check is registry-aware: an auto-repeat DOWN within an active gesture (the
// key is registered in the tracker) is treated as a mid event and follows the start-event verdict.
bool IsGestureStartEvent(HookEventType hookType, const std::shared_ptr<KeyEvent> &keyEvent,
    const GestureTracker *tracker);
bool IsGestureEndEvent(HookEventType hookType, const std::shared_ptr<KeyEvent> &keyEvent);
int32_t GetGestureKey(HookEventType hookType, const std::shared_ptr<KeyEvent> &keyEvent);

bool IsGestureStartEvent(HookEventType hookType, const std::shared_ptr<PointerEvent> &pointerEvent);
bool IsGestureEndEvent(HookEventType hookType, const std::shared_ptr<PointerEvent> &pointerEvent);
bool IsOneOffEvent(HookEventType hookType, const std::shared_ptr<PointerEvent> &pointerEvent);
int32_t GetGestureKey(HookEventType hookType, const std::shared_ptr<PointerEvent> &pointerEvent);
} // namespace MMI
} // namespace OHOS
#endif // GESTURE_CLASSIFIER_H
