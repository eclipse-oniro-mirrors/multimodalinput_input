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

#include "input_event_statistics.h"

#include <chrono>
#include <cstring>

#include "dfx_hisysevent.h"
#include "input_device_manager.h"
#include "libinput.h"
#include "timer_manager.h"
#include "util.h"
#include "util_ex.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_DISPATCH
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "InputEventStats"

namespace OHOS {
namespace MMI {
namespace {
// TimerManager long timer upper bound is 30s, so the hourly report is driven by a 30s tick
// which fires the report on the first tick of each new wall-clock hour.
constexpr int32_t REPORT_TICK_INTERVAL_MS { 30000 };
constexpr int32_t REPORT_TIMER_REPEAT_FOREVER { -1 };
constexpr int32_t SECONDS_PER_HOUR { 3600 };

int64_t GetCurrentEpochHour()
{
    auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return nowSec / SECONDS_PER_HOUR;
}
} // namespace

std::atomic<uint64_t> InputEventStatistics::moveIntervalCounts_[BUCKET_COUNT] = {};
std::atomic<uint64_t> InputEventStatistics::dispatchLatencyCounts_[BUCKET_COUNT] = {};
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
std::atomic<uint64_t> InputEventStatistics::touchpadTwoFingerTapCount_ { 0 };
std::atomic<uint64_t> InputEventStatistics::touchpadPressRightClickAreaCount_ { 0 };
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
std::atomic<bool> InputEventStatistics::deviceTrimmed_ { false };
int64_t InputEventStatistics::lastMoveEventTimeUs_ { 0 };
int32_t InputEventStatistics::lastMoveDeviceId_ { -1 };
int64_t InputEventStatistics::dispatchStartTimeUs_ { 0 };
bool InputEventStatistics::dispatchStartPending_ { false };
std::atomic<int64_t> InputEventStatistics::lastReportHour_ { 0 };
int32_t InputEventStatistics::reportTimerId_ { -1 };

// Every device reported by libinput (kernel /dev/input node, including uinput created
// ones) is counted as a physical device; only UDS injected events carrying
// EVENT_FLAG_SIMULATE are excluded on the record side.
void InputEventStatistics::OnPointerMotionEvent(int32_t deviceId)
{
    if (deviceTrimmed_.load(std::memory_order_relaxed)) {
        return;
    }
    NotifyMotionEvent(deviceId, GetSysClockTime());
}

void InputEventStatistics::NotifyMotionEvent(int32_t deviceId, int64_t timestampUs)
{
    if (deviceTrimmed_.load(std::memory_order_relaxed)) {
        return;
    }
    // The first event (or the first event after a device change/removal/report reset)
    // only sets the baseline and produces no interval sample.
    if (lastMoveEventTimeUs_ <= 0 || lastMoveDeviceId_ != deviceId || timestampUs <= lastMoveEventTimeUs_) {
        lastMoveEventTimeUs_ = timestampUs;
        lastMoveDeviceId_ = deviceId;
        return;
    }
    RecordMoveInterval(timestampUs - lastMoveEventTimeUs_);
    lastMoveEventTimeUs_ = timestampUs;
}

void InputEventStatistics::RecordMoveInterval(int64_t intervalUs)
{
    if (deviceTrimmed_.load(std::memory_order_relaxed) || intervalUs < 0) {
        return;
    }
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        if (intervalUs < MOVE_INTERVAL_BOUNDS[i]) {
            moveIntervalCounts_[i].fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }
}

void InputEventStatistics::MarkMouseDispatchStart(int32_t deviceId)
{
    if (deviceTrimmed_.load(std::memory_order_relaxed)) {
        return;
    }
    dispatchStartTimeUs_ = GetSysClockTime();
    dispatchStartPending_ = true;
}

void InputEventStatistics::RecordMouseDispatchLatency(std::shared_ptr<PointerEvent> point)
{
    if (!dispatchStartPending_ || point == nullptr) {
        return;
    }
    dispatchStartPending_ = false;
    if (deviceTrimmed_.load(std::memory_order_relaxed)) {
        return;
    }
    if (point->GetSourceType() != PointerEvent::SOURCE_TYPE_MOUSE &&
        point->GetSourceType() != PointerEvent::SOURCE_TYPE_TOUCHPAD) {
        return;
    }
    if (point->GetPointerAction() != PointerEvent::POINTER_ACTION_MOVE) {
        return;
    }
    if (point->HasFlag(InputEvent::EVENT_FLAG_SIMULATE)) {
        return;
    }
    int64_t latencyUs = GetSysClockTime() - dispatchStartTimeUs_;
    RecordDispatchLatency(latencyUs);
}

void InputEventStatistics::RecordDispatchLatency(int64_t latencyUs)
{
    if (deviceTrimmed_.load(std::memory_order_relaxed) || latencyUs < 0) {
        return;
    }
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        if (latencyUs < DISPATCH_LATENCY_BOUNDS[i]) {
            dispatchLatencyCounts_[i].fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }
}

#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
void InputEventStatistics::RecordTouchpadRightClick(bool isTwoFingerTap)
{
    if (deviceTrimmed_.load(std::memory_order_relaxed)) {
        return;
    }
    if (isTwoFingerTap) {
        touchpadTwoFingerTapCount_.fetch_add(1, std::memory_order_relaxed);
    } else {
        touchpadPressRightClickAreaCount_.fetch_add(1, std::memory_order_relaxed);
    }
}
#endif // OHOS_BUILD_ENABLE_TOUCHPAD

void InputEventStatistics::OnDeviceRemoved(int32_t deviceId)
{
    if (lastMoveDeviceId_ == deviceId) {
        // Drop the baseline so no interval is computed across the device removal gap.
        lastMoveEventTimeUs_ = 0;
        lastMoveDeviceId_ = -1;
    }
}

void InputEventStatistics::SetDeviceTrimmed(bool trimmed)
{
    deviceTrimmed_.store(trimmed, std::memory_order_relaxed);
}

bool InputEventStatistics::IsDeviceTrimmed()
{
    return deviceTrimmed_.load(std::memory_order_relaxed);
}

uint64_t InputEventStatistics::GetMoveIntervalCount(size_t bucket)
{
    if (bucket >= BUCKET_COUNT) {
        return 0;
    }
    return moveIntervalCounts_[bucket].load(std::memory_order_relaxed);
}

uint64_t InputEventStatistics::GetDispatchLatencyCount(size_t bucket)
{
    if (bucket >= BUCKET_COUNT) {
        return 0;
    }
    return dispatchLatencyCounts_[bucket].load(std::memory_order_relaxed);
}

#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
uint64_t InputEventStatistics::GetTouchpadTwoFingerTapCount()
{
    return touchpadTwoFingerTapCount_.load(std::memory_order_relaxed);
}

uint64_t InputEventStatistics::GetTouchpadPressRightClickAreaCount()
{
    return touchpadPressRightClickAreaCount_.load(std::memory_order_relaxed);
}

uint64_t InputEventStatistics::GetTouchpadDoubleTapAndDragCount()
{
    // The counter is maintained by libinput per touchpad device; the reported value
    // is the sum over all touchpads. Empty (no touchpad) returns 0.
    uint64_t totalCount = 0;
    auto originDevices = INPUT_DEV_MGR->GetTouchPadDeviceOrigins();
    for (const auto &originDevice : originDevices) {
        if (originDevice == nullptr) {
            continue;
        }
        totalCount += libinput_device_config_tap_get_drag_count(originDevice);
    }
    return totalCount;
}

void InputEventStatistics::ResetTouchpadDoubleTapAndDragCount()
{
    auto originDevices = INPUT_DEV_MGR->GetTouchPadDeviceOrigins();
    for (const auto &originDevice : originDevices) {
        if (originDevice == nullptr) {
            continue;
        }
        libinput_device_config_tap_reset_drag_count(originDevice);
    }
}
#endif // OHOS_BUILD_ENABLE_TOUCHPAD

void InputEventStatistics::StartReportTimer()
{
    if (reportTimerId_ >= 0) {
        return;
    }
    reportTimerId_ = TimerMgr->AddLongTimer(REPORT_TICK_INTERVAL_MS, REPORT_TIMER_REPEAT_FOREVER, []() {
        int64_t currentHour = GetCurrentEpochHour();
        int64_t lastHour = lastReportHour_.load(std::memory_order_relaxed);
        if (lastHour == 0) {
            // First tick after service start only establishes the hour baseline.
            lastReportHour_.store(currentHour, std::memory_order_relaxed);
            return;
        }
        if (currentHour != lastHour) {
            lastReportHour_.store(currentHour, std::memory_order_relaxed);
            TryReport();
        }
    }, "InputEventStats-HourlyReport");
    if (reportTimerId_ < 0) {
        MMI_HILOGE("Add input event statistics report timer failed");
    }
}

void InputEventStatistics::LogReportSummary(const uint32_t (&moveCounts)[BUCKET_COUNT],
    const uint32_t (&latencyCounts)[BUCKET_COUNT])
{
    MMI_HILOGI("Input event statistics: moveInterval[0-8ms:%{public}u, 8-16ms:%{public}u, 16-33ms:%{public}u,"
        " 33-50ms:%{public}u, 50ms+:%{public}u], dispatchLatency[0-2ms:%{public}u, 2-4ms:%{public}u,"
        " 4-8ms:%{public}u, 8-16ms:%{public}u, 16ms+:%{public}u]",
        moveCounts[MOVE_INTERVAL_BELOW_8MS], moveCounts[MOVE_INTERVAL_FROM_8_TO_16MS],
        moveCounts[MOVE_INTERVAL_FROM_16_TO_33MS], moveCounts[MOVE_INTERVAL_FROM_33_TO_50MS],
        moveCounts[MOVE_INTERVAL_ABOVE_50MS],
        latencyCounts[DISPATCH_LATENCY_BELOW_2MS], latencyCounts[DISPATCH_LATENCY_FROM_2_TO_4MS],
        latencyCounts[DISPATCH_LATENCY_FROM_4_TO_8MS], latencyCounts[DISPATCH_LATENCY_FROM_8_TO_16MS],
        latencyCounts[DISPATCH_LATENCY_ABOVE_16MS]);
}

#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
void InputEventStatistics::LogTouchpadGestureSummary(uint32_t twoFingerTapCount,
    uint32_t pressRightClickAreaCount, uint32_t doubleTapAndDragCount)
{
    MMI_HILOGI("Input event statistics: touchpadRightClick[twoFingerTap:%{public}u,"
        " pressRightClickArea:%{public}u], touchpadDoubleTapAndDrag:%{public}u",
        twoFingerTapCount, pressRightClickAreaCount, doubleTapAndDragCount);
}
#endif // OHOS_BUILD_ENABLE_TOUCHPAD

void InputEventStatistics::TryReport()
{
    if (deviceTrimmed_.load(std::memory_order_relaxed)) {
        return;
    }
    uint32_t moveCounts[BUCKET_COUNT] = {};
    uint64_t moveTotal = 0;
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        moveCounts[i] = static_cast<uint32_t>(moveIntervalCounts_[i].load(std::memory_order_relaxed));
        moveTotal += moveCounts[i];
    }
    uint32_t latencyCounts[BUCKET_COUNT] = {};
    uint64_t latencyTotal = 0;
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        latencyCounts[i] = static_cast<uint32_t>(dispatchLatencyCounts_[i].load(std::memory_order_relaxed));
        latencyTotal += latencyCounts[i];
    }
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    uint32_t twoFingerTapCount =
        static_cast<uint32_t>(touchpadTwoFingerTapCount_.load(std::memory_order_relaxed));
    uint32_t pressRightClickAreaCount =
        static_cast<uint32_t>(touchpadPressRightClickAreaCount_.load(std::memory_order_relaxed));
    // Pull the tap-and-drag count accumulated inside libinput for this window.
    uint32_t doubleTapAndDragCount = static_cast<uint32_t>(GetTouchpadDoubleTapAndDragCount());
    uint64_t gestureTotal = static_cast<uint64_t>(twoFingerTapCount) + pressRightClickAreaCount +
        doubleTapAndDragCount;
    if (moveTotal == 0 && latencyTotal == 0 && gestureTotal == 0) {
        return;
    }
#else
    if (moveTotal == 0 && latencyTotal == 0) {
        return;
    }
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
    LogReportSummary(moveCounts, latencyCounts);
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    LogTouchpadGestureSummary(twoFingerTapCount, pressRightClickAreaCount, doubleTapAndDragCount);
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
    if (moveTotal > 0) {
        int32_t ret = DfxHisysevent::ReportOriginalInputEventIntervalStat(moveCounts[MOVE_INTERVAL_BELOW_8MS],
            moveCounts[MOVE_INTERVAL_FROM_8_TO_16MS], moveCounts[MOVE_INTERVAL_FROM_16_TO_33MS],
            moveCounts[MOVE_INTERVAL_FROM_33_TO_50MS], moveCounts[MOVE_INTERVAL_ABOVE_50MS]);
        if (ret != RET_OK) {
            MMI_HILOGE("Report input event interval stat failed, ret:%{public}d", ret);
        }
    }
    if (latencyTotal > 0) {
        int32_t ret = DfxHisysevent::ReportOriginalEventDispatchLatencyStat(
            latencyCounts[DISPATCH_LATENCY_BELOW_2MS],
            latencyCounts[DISPATCH_LATENCY_FROM_2_TO_4MS], latencyCounts[DISPATCH_LATENCY_FROM_4_TO_8MS],
            latencyCounts[DISPATCH_LATENCY_FROM_8_TO_16MS], latencyCounts[DISPATCH_LATENCY_ABOVE_16MS]);
        if (ret != RET_OK) {
            MMI_HILOGE("Report event dispatch latency stat failed, ret:%{public}d", ret);
        }
    }
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    if (gestureTotal > 0) {
        int32_t ret = DfxHisysevent::ReportInputGestureStat(twoFingerTapCount, pressRightClickAreaCount,
            doubleTapAndDragCount);
        if (ret != RET_OK) {
            MMI_HILOGE("Report input gesture stat failed, ret:%{public}d", ret);
        }
    }
    if (doubleTapAndDragCount > 0) {
        // The libinput counters were consumed by this report window; start the next
        // window from zero (consistent with resetting local counters after a report).
        ResetTouchpadDoubleTapAndDragCount();
    }
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
    Reset();
}

void InputEventStatistics::Reset()
{
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        moveIntervalCounts_[i].store(0, std::memory_order_relaxed);
        dispatchLatencyCounts_[i].store(0, std::memory_order_relaxed);
    }
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    touchpadTwoFingerTapCount_.store(0, std::memory_order_relaxed);
    touchpadPressRightClickAreaCount_.store(0, std::memory_order_relaxed);
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
    lastMoveEventTimeUs_ = 0;
    lastMoveDeviceId_ = -1;
    dispatchStartPending_ = false;
}

void InputEventStatistics::Dump(int32_t fd)
{
    mprintf(fd, "Input Event Statistics:\n");
    mprintf(fd, "  Mouse Move Interval: [0-8ms):%llu [8-16ms):%llu [16-33ms):%llu [33-50ms):%llu"
        " [50ms+):%llu\n",
        static_cast<unsigned long long>(GetMoveIntervalCount(MOVE_INTERVAL_BELOW_8MS)),
        static_cast<unsigned long long>(GetMoveIntervalCount(MOVE_INTERVAL_FROM_8_TO_16MS)),
        static_cast<unsigned long long>(GetMoveIntervalCount(MOVE_INTERVAL_FROM_16_TO_33MS)),
        static_cast<unsigned long long>(GetMoveIntervalCount(MOVE_INTERVAL_FROM_33_TO_50MS)),
        static_cast<unsigned long long>(GetMoveIntervalCount(MOVE_INTERVAL_ABOVE_50MS)));
    mprintf(fd, "  Mouse Dispatch Latency: [0-2ms):%llu [2-4ms):%llu [4-8ms):%llu [8-16ms):%llu"
        " [16ms+):%llu\n",
        static_cast<unsigned long long>(GetDispatchLatencyCount(DISPATCH_LATENCY_BELOW_2MS)),
        static_cast<unsigned long long>(GetDispatchLatencyCount(DISPATCH_LATENCY_FROM_2_TO_4MS)),
        static_cast<unsigned long long>(GetDispatchLatencyCount(DISPATCH_LATENCY_FROM_4_TO_8MS)),
        static_cast<unsigned long long>(GetDispatchLatencyCount(DISPATCH_LATENCY_FROM_8_TO_16MS)),
        static_cast<unsigned long long>(GetDispatchLatencyCount(DISPATCH_LATENCY_ABOVE_16MS)));
}

#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
void InputEventStatistics::DumpTouchpadGesture(int32_t fd)
{
    mprintf(fd, "Touchpad Gesture Statistics:\n");
    mprintf(fd, "  Right Click Count (Two Finger Tap): %llu\n",
        static_cast<unsigned long long>(GetTouchpadTwoFingerTapCount()));
    mprintf(fd, "  Right Click Count (Press Right Click Area): %llu\n",
        static_cast<unsigned long long>(GetTouchpadPressRightClickAreaCount()));
    mprintf(fd, "  Double Tap And Drag Count: %llu\n",
        static_cast<unsigned long long>(GetTouchpadDoubleTapAndDragCount()));
}
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
} // namespace MMI
} // namespace OHOS
