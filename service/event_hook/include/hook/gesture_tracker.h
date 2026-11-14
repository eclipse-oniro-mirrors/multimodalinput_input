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

#ifndef GESTURE_TRACKER_H
#define GESTURE_TRACKER_H

#include <map>
#include <shared_mutex>
#include <vector>

#include "input_handler_type.h"

namespace OHOS {
namespace MMI {
// Chain-level gesture registry keyed by (HookEventType, gestureKey) recording the consumption
// verdict of in-flight gestures (spec 5: the start-event receipt decides mid/end events; the registry
// is the single verdict point and survives hook removal) plus the set of joined hooks (join only via
// start events, the spec-4 wording). Lifecycle: created/reset on start events, Closed after the end
// event is handled, fully cleared on screen-lock suspend; entries carry a TTL fallback sweep against
// leaks caused by lost end events.
class GestureTracker {
public:
    // Start event arrival: creates or resets the entry for the key (and sweeps stale entries)
    void ResolveStart(HookEventType hookType, int32_t gestureKey);
    bool IsKnown(HookEventType hookType, int32_t gestureKey) const;
    bool IsConsumed(HookEventType hookType, int32_t gestureKey) const;
    bool IsJoined(HookEventType hookType, int32_t gestureKey, int32_t pid) const;
    void MarkJoined(HookEventType hookType, int32_t gestureKey, int32_t pid);
    void MarkConsumed(HookEventType hookType, int32_t gestureKey);
    // End event handled: erases the entry
    void Close(HookEventType hookType, int32_t gestureKey);
    // Screen-lock suspend: clears everything
    void Clear();
    // Hook removal: drops its join records (the consumption verdict is kept and survives hook removal)
    void RemovePid(int32_t pid);

private:
    struct GestureEntry {
        bool consumed { false };
        std::vector<int32_t> joinedPids;
        long long bornAtMs { 0 };
    };
    void SweepLocked(HookEventType hookType, long long nowMs);
    static long long NowMs();

    std::map<HookEventType, std::map<int32_t, GestureEntry>> gestures_;
    mutable std::shared_mutex rwMutex_;
};
} // namespace MMI
} // namespace OHOS
#endif // GESTURE_TRACKER_H
