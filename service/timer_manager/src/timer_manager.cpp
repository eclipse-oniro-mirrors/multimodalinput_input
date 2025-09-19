/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "timer_manager.h"
#include "bytrace_adapter.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "TimerManager"
#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_SERVER

namespace OHOS {
namespace MMI {
namespace {
constexpr int32_t MIN_DELAY { -1 };
constexpr int32_t MIN_INTERVAL { 36 };
constexpr int32_t MAX_INTERVAL_MS { 10000 };
constexpr int32_t MAX_LONG_INTERVAL_MS { 30000 };
constexpr int32_t MAX_TIMER_COUNT { 64 };
constexpr int32_t NONEXISTENT_ID { -1 };
} // namespace

TimerManager::TimerManager() {}
TimerManager::~TimerManager() {}

int32_t TimerManager::AddTimer(int32_t intervalMs, int32_t repeatCount, std::function<void()> callback,
    const std::string &name)
{
    if (intervalMs < MIN_INTERVAL) {
        intervalMs = MIN_INTERVAL;
    } else if (intervalMs > MAX_INTERVAL_MS) {
        intervalMs = MAX_INTERVAL_MS;
    }
    return AddTimerInternal(intervalMs, repeatCount, callback, name);
}

int32_t TimerManager::AddLongTimer(int32_t intervalMs, int32_t repeatCount, std::function<void()> callback,
    const std::string &name)
{
    if (intervalMs < MIN_INTERVAL) {
        intervalMs = MIN_INTERVAL;
    } else if (intervalMs > MAX_LONG_INTERVAL_MS) {
        intervalMs = MAX_INTERVAL_MS;
    }
    return AddTimerInternal(intervalMs, repeatCount, callback, name);
}

int32_t TimerManager::RemoveTimer(int32_t timerId, const std::string &name)
{
    return RemoveTimerInternal(timerId, name);
}

int32_t TimerManager::ResetTimer(int32_t timerId)
{
    return ResetTimerInternal(timerId);
}

bool TimerManager::IsExist(int32_t timerId)
{
    return IsExistInternal(timerId);
}

int32_t TimerManager::CalcNextDelay()
{
    return CalcNextDelayInternal();
}

void TimerManager::ProcessTimers()
{
    ProcessTimersInternal();
}

int32_t TimerManager::TakeNextTimerId()
{
    uint64_t timerSlot = 0;
    uint64_t one = 1;
    std::lock_guard<std::recursive_mutex> lock(timerMutex_);
    for (const auto &timer : timers_) {
        timerSlot |= (one << timer->id);
    }

    for (int32_t i = 0; i < MAX_TIMER_COUNT; i++) {
        if ((timerSlot & (one << i)) == 0) {
            return i;
        }
    }
    return NONEXISTENT_ID;
}

int32_t TimerManager::AddTimerInternal(int32_t intervalMs, int32_t repeatCount, std::function<void()> callback,
    const std::string &name)
{
    if (!callback) {
        return NONEXISTENT_ID;
    }
    auto timer = std::make_unique<TimerItem>();
    timer->intervalMs = intervalMs;
    timer->repeatCount = repeatCount;
    timer->callbackCount = 0;
    timer->name = name;
    auto nowTime = GetMillisTime();
    if (!AddInt64(nowTime, timer->intervalMs, timer->nextCallTime)) {
        MMI_HILOGE("The addition of nextCallTime in TimerItem overflows");
        return NONEXISTENT_ID;
    }
    timer->callback = callback;
    std::lock_guard<std::recursive_mutex> lock(timerMutex_);
    int32_t timerId = TakeNextTimerId();
    if (timerId < 0) {
        return NONEXISTENT_ID;
    }
    timer->id = timerId;
    InsertTimerInternal(timer);
    return timerId;
}

int32_t TimerManager::RemoveTimerInternal(int32_t timerId, const std::string &name)
{
    std::lock_guard<std::recursive_mutex> lock(timerMutex_);
    for (auto it = timers_.begin(); it != timers_.end(); ++it) {
        if ((*it)->id == timerId && (name.empty() || (*it)->name == name)) {
            timers_.erase(it);
            return RET_OK;
        }
    }
    return RET_ERR;
}

int32_t TimerManager::ResetTimerInternal(int32_t timerId)
{
    std::lock_guard<std::recursive_mutex> lock(timerMutex_);
    for (auto it = timers_.begin(); it != timers_.end(); ++it) {
        if ((*it)->id == timerId) {
            auto timer = std::move(*it);
            timers_.erase(it);
            auto nowTime = GetMillisTime();
            if (!AddInt64(nowTime, timer->intervalMs, timer->nextCallTime)) {
                MMI_HILOGE("The addition of nextCallTime in TimerItem overflows");
                return RET_ERR;
            }
            timer->callbackCount = 0;
            InsertTimerInternal(timer);
            return RET_OK;
        }
    }
    return RET_ERR;
}

bool TimerManager::IsExistInternal(int32_t timerId)
{
    std::lock_guard<std::recursive_mutex> lock(timerMutex_);
    for (auto it = timers_.begin(); it != timers_.end(); ++it) {
        if ((*it)->id == timerId) {
            return true;
        }
    }
    return false;
}

void TimerManager::InsertTimerInternal(std::unique_ptr<TimerItem>& timer)
{
    std::lock_guard<std::recursive_mutex> lock(timerMutex_);
    for (auto it = timers_.begin(); it != timers_.end(); ++it) {
        if ((*it)->nextCallTime > timer->nextCallTime) {
            timers_.insert(it, std::move(timer));
            return;
        }
    }
    timers_.push_back(std::move(timer));
}

int32_t TimerManager::CalcNextDelayInternal()
{
    auto delay = MIN_DELAY;
    std::lock_guard<std::recursive_mutex> lock(timerMutex_);
    if (!timers_.empty()) {
        auto nowTime = GetMillisTime();
        const auto& item = *timers_.begin();
        if (nowTime >= item->nextCallTime) {
            delay = 0;
        } else {
            delay = item->nextCallTime - nowTime;
        }
    }
    return delay;
}

void TimerManager::ProcessTimersInternal()
{
    std::lock_guard<std::recursive_mutex> lock(timerMutex_);
    if (timers_.empty()) {
        return;
    }
    auto nowTime = GetMillisTime();
    for (;;) {
        auto it = timers_.begin();
        if (it == timers_.end()) {
            break;
        }
        if ((*it)->nextCallTime > nowTime) {
            break;
        }
        auto curTimer = std::move(*it);
        std::string msg = "StartTimer, Name is: " + curTimer->name;
        BytraceAdapter::MMIServiceTraceStart(BytraceAdapter::MMI_THREAD_LOOP_DEPTH_THREE, msg);
        CrashObjDumper dumper((curTimer->name).c_str());
        timers_.erase(it);
        ++curTimer->callbackCount;
        if ((curTimer->repeatCount >= 1) && (curTimer->callbackCount >= curTimer->repeatCount)) {
            curTimer->callback();
            BytraceAdapter::MMIServiceTraceStop();
            continue;
        }
        if (!AddInt64(curTimer->nextCallTime, curTimer->intervalMs, curTimer->nextCallTime)) {
            MMI_HILOGE("The addition of nextCallTime in TimerItem overflows");
            BytraceAdapter::MMIServiceTraceStop();
            return;
        }
        auto callback = curTimer->callback;
        InsertTimerInternal(curTimer);
        callback();
        BytraceAdapter::MMIServiceTraceStop();
    }
}
} // namespace MMI
} // namespace OHOS
