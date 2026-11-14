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

#include "expiration_checker.h"

#include <algorithm>
#include <chrono>

#include "mmi_log.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_HANDLER
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "ExpirationChecker"

namespace OHOS {
namespace MMI {
namespace {
constexpr long long STASH_EVENT_TIMEOUT_MS { 1500 };
}

void ExpirationChecker::UpdateInputEvent(const std::shared_ptr<InputEvent> event)
{
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    UpdateInputEvent(event, now);
}

void ExpirationChecker::UpdateInputEvent(const std::shared_ptr<InputEvent> event, long long timeStampRcvd)
{
    CALL_DEBUG_ENTER;
    CHKPV(event);
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    stashEvents_.push_back(StashEvent {
        .timeStampRcvd = timeStampRcvd,
        .eventId = event->GetId(),
        .event = event,
    });
}

std::shared_ptr<InputEvent> ExpirationChecker::GetEvent(int32_t eventId)
{
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto iter = std::find_if(stashEvents_.begin(), stashEvents_.end(), [eventId] (const auto &event) {
        return event.eventId == eventId;
    });
    if (iter == stashEvents_.end()) {
        return nullptr;
    }
    return iter->event;
}

std::vector<std::shared_ptr<InputEvent>> ExpirationChecker::TakeExpiredEvents()
{
    std::vector<std::shared_ptr<InputEvent>> expiredEvents;
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    for (auto iter = stashEvents_.begin(); iter != stashEvents_.end();) {
        if (now - iter->timeStampRcvd >= STASH_EVENT_TIMEOUT_MS) {
            if (iter->event != nullptr) {
                expiredEvents.push_back(std::move(iter->event));
            }
            iter = stashEvents_.erase(iter);
        } else {
            ++iter;
        }
    }
    return expiredEvents;
}

void ExpirationChecker::RemoveEvent(int32_t eventId)
{
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    stashEvents_.erase(std::remove_if(stashEvents_.begin(), stashEvents_.end(), [eventId] (const auto &event) {
        return event.eventId == eventId; }),
        stashEvents_.end());
}

std::vector<std::shared_ptr<InputEvent>> ExpirationChecker::DrainStashEvents()
{
    std::vector<std::shared_ptr<InputEvent>> events;
    events.reserve(stashEvents_.size());
    for (auto &stashEvent : stashEvents_) {
        if (stashEvent.event != nullptr) {
            events.push_back(std::move(stashEvent.event));
        }
    }
    stashEvents_.clear();
    return events;
}
} // namespace MMI
} // namespace OHOS