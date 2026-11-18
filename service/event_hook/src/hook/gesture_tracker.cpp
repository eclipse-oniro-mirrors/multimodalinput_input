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

#include "gesture_tracker.h"

#include <algorithm>
#include <chrono>

#include "mmi_log.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_HANDLER
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "GestureTracker"

namespace OHOS {
namespace MMI {
namespace {
// Fallback entry lifetime: a gesture normally lasts seconds (press to release); entries are
// reclaimed here when an end event is lost.
constexpr long long GESTURE_ENTRY_TTL_MS { 60000 };
} // namespace

long long GestureTracker::NowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void GestureTracker::SweepLocked(HookEventType hookType, long long nowMs)
{
    auto typeIter = gestures_.find(hookType);
    if (typeIter == gestures_.end()) {
        return;
    }
    auto &entries = typeIter->second;
    for (auto it = entries.begin(); it != entries.end();) {
        if (nowMs - it->second.bornAtMs >= GESTURE_ENTRY_TTL_MS) {
            MMI_HILOGW("Sweep stale gesture entry, hookType:%{public}u, gestureKey:%{public}d",
                hookType, it->first);
            it = entries.erase(it);
        } else {
            ++it;
        }
    }
    if (entries.empty()) {
        gestures_.erase(typeIter);
    }
}

void GestureTracker::ResolveStart(HookEventType hookType, int32_t gestureKey)
{
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    long long nowMs = NowMs();
    SweepLocked(hookType, nowMs);
    auto &entry = gestures_[hookType][gestureKey];
    // Same-key reset: when the previous gesture's end event was lost, a new gesture starts clean
    entry.consumed = false;
    entry.joinedPids.clear();
    entry.bornAtMs = nowMs;
}

bool GestureTracker::IsKnown(HookEventType hookType, int32_t gestureKey) const
{
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto typeIter = gestures_.find(hookType);
    return typeIter != gestures_.end() && typeIter->second.find(gestureKey) != typeIter->second.end();
}

bool GestureTracker::IsConsumed(HookEventType hookType, int32_t gestureKey) const
{
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto typeIter = gestures_.find(hookType);
    if (typeIter == gestures_.end()) {
        return false;
    }
    auto keyIter = typeIter->second.find(gestureKey);
    return keyIter != typeIter->second.end() && keyIter->second.consumed;
}

bool GestureTracker::IsJoined(HookEventType hookType, int32_t gestureKey, int32_t pid) const
{
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto typeIter = gestures_.find(hookType);
    if (typeIter == gestures_.end()) {
        return false;
    }
    auto keyIter = typeIter->second.find(gestureKey);
    if (keyIter == typeIter->second.end()) {
        return false;
    }
    const auto &pids = keyIter->second.joinedPids;
    return std::find(pids.begin(), pids.end(), pid) != pids.end();
}

void GestureTracker::MarkJoined(HookEventType hookType, int32_t gestureKey, int32_t pid)
{
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    auto typeIter = gestures_.find(hookType);
    if (typeIter == gestures_.end()) {
        return;
    }
    auto keyIter = typeIter->second.find(gestureKey);
    if (keyIter == typeIter->second.end()) {
        return;
    }
    auto &pids = keyIter->second.joinedPids;
    if (std::find(pids.begin(), pids.end(), pid) == pids.end()) {
        pids.push_back(pid);
    }
}

void GestureTracker::MarkConsumed(HookEventType hookType, int32_t gestureKey)
{
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    auto typeIter = gestures_.find(hookType);
    if (typeIter == gestures_.end()) {
        return;
    }
    auto keyIter = typeIter->second.find(gestureKey);
    if (keyIter != typeIter->second.end()) {
        keyIter->second.consumed = true;
    }
}

void GestureTracker::Close(HookEventType hookType, int32_t gestureKey)
{
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    auto typeIter = gestures_.find(hookType);
    if (typeIter == gestures_.end()) {
        return;
    }
    typeIter->second.erase(gestureKey);
    if (typeIter->second.empty()) {
        gestures_.erase(typeIter);
    }
}

void GestureTracker::Clear()
{
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    gestures_.clear();
}

void GestureTracker::RemovePid(int32_t pid)
{
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    for (auto &[hookType, entries] : gestures_) {
        for (auto &[gestureKey, entry] : entries) {
            entry.joinedPids.erase(std::remove(entry.joinedPids.begin(), entry.joinedPids.end(), pid),
                entry.joinedPids.end());
        }
    }
}
} // namespace MMI
} // namespace OHOS
