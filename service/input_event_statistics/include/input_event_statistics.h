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

#ifndef INPUT_EVENT_STATISTICS_H
#define INPUT_EVENT_STATISTICS_H

#include <atomic>
#include <cstdint>
#include <memory>

#include "pointer_event.h"

namespace OHOS {
namespace MMI {
/*
 * Input event statistics (DFX only, no public API).
 *
 * Collects, reports and dumps three kinds of aggregated counters:
 *  - pointer motion event interval (mouse/touchpad);
 *  - mouse pointer-move event dispatch latency (libinput arrival at
 *    MouseEventNormalize::OnEvent, before normalize/gesture interpretation ->
 *    UDS send done, move-action events only);
 *  - touchpad right-click gesture count, split by gesture source (two-finger
 *    tap / press in the right-click area).
 *
 * Device scope: every device reported by libinput (kernel /dev/input node,
 * including uinput created ones) is counted as a physical device; only UDS
 * injected events carrying EVENT_FLAG_SIMULATE are excluded.
 *
 * Threading: all notification entries (OnPointerMotionEvent/MarkMouseDispatchStart/
 * RecordMouseDispatchLatency/RecordTouchpadRightClick/OnDeviceRemoved) are invoked on the
 * server epoll thread; TryReport is invoked by the report timer on the same thread;
 * Dump is posted as a sync task on the server thread as well. Bucket counters are
 * atomics so that any incidental cross-thread read is still safe.
 *
 * Performance: notification entries only do integer compare/increment on the hot path
 * (no allocation, no string formatting, no logging), see docs constraint <=50us.
 */
class InputEventStatistics final {
public:
    // Number of buckets of each histogram.
    static constexpr size_t BUCKET_COUNT = 5;
    // Motion interval buckets (us, left-closed right-open): [0,8ms) [8,16ms) [16,33ms) [33,50ms) [50ms,+)
    static constexpr int64_t MOVE_INTERVAL_BOUNDS[BUCKET_COUNT] = { 8000, 16000, 33000, 50000, INT64_MAX };
    // Dispatch latency buckets (us, left-closed right-open): [0,2ms) [2,4ms) [4,8ms) [8,16ms) [16ms,+)
    static constexpr int64_t DISPATCH_LATENCY_BOUNDS[BUCKET_COUNT] = { 2000, 4000, 8000, 16000, INT64_MAX };

    // Named bucket indices, in the same order as the *_BOUNDS arrays above and as the
    // BELOW8MS/BELOW16MS/... fields of the STATISTIC events in hisysevent.yaml.
    enum MoveIntervalBucket : size_t {
        MOVE_INTERVAL_BELOW_8MS = 0,
        MOVE_INTERVAL_FROM_8_TO_16MS,
        MOVE_INTERVAL_FROM_16_TO_33MS,
        MOVE_INTERVAL_FROM_33_TO_50MS,
        MOVE_INTERVAL_ABOVE_50MS,
    };
    enum DispatchLatencyBucket : size_t {
        DISPATCH_LATENCY_BELOW_2MS = 0,
        DISPATCH_LATENCY_FROM_2_TO_4MS,
        DISPATCH_LATENCY_FROM_4_TO_8MS,
        DISPATCH_LATENCY_FROM_8_TO_16MS,
        DISPATCH_LATENCY_ABOVE_16MS,
    };

    // ---------------- notification entries (event pipeline) ----------------
    // Called on pointer motion events (POINTER_MOTION of mice, POINTER_MOTION_TOUCHPAD of
    // touchpads) from MouseEventNormalize::OnEvent. Handles interval bucketing; the first
    // event of a stream only sets the baseline and is not counted.
    static void OnPointerMotionEvent(int32_t deviceId);

    // Called on pointer motion events (POINTER_MOTION of mice, POINTER_MOTION_TOUCHPAD of
    // touchpads) from MouseEventNormalize::OnEvent (libinput arrival, before normalize
    // and gesture interpretation); marks the receive time of the current move event as
    // the dispatch-latency start, so the measured span covers the full in-service
    // processing (normalize + gesture interpretation + routing + dispatch).
    // Source-type/move-action/simulate filtering stays on the record side
    // (RecordMouseDispatchLatency).
    static void MarkMouseDispatchStart(int32_t deviceId);

    // Called after the UDS socket send of the pointer event succeeded; records the
    // dispatch latency once per marked event, for mouse/touchpad sourced move-action
    // events only (button/wheel events share the pipeline but are not counted).
    // Events that never reach a successful UDS send (intercepted/consumed/dropped)
    // are not counted.
    static void RecordMouseDispatchLatency(const std::shared_ptr<PointerEvent> &point);

    // Called when a touchpad right-click gesture is recognized (two-finger tap or
    // right button zone press). Counted per gesture source: isTwoFingerTap=true counts
    // as a two-finger tap gesture, otherwise as a press in the right-click area.
    static void RecordTouchpadRightClick(bool isTwoFingerTap);

    // ---------------- lifecycle ----------------
    // Drops the motion-interval baseline of the device when it is removed, so no
    // interval is computed across the removal gap.
    static void OnDeviceRemoved(int32_t deviceId);

    // Device trim switch: statistics (record/report) are disabled when trimmed.
    static void SetDeviceTrimmed(bool trimmed);
    static bool IsDeviceTrimmed();

    // ---------------- report and dump ----------------
    // Starts the periodic report timer (aligned to each wall-clock hour).
    static void StartReportTimer();

    // Reports non-empty statistics via HiSysEvent (STATISTIC), prints a summary log and
    // resets the successfully reported counters. Does nothing when there is no data.
    static void TryReport();

    // Resets all counters and baselines.
    static void Reset();

    // Dumps all statistics to fd (zero values when no data, never fails).
    static void Dump(int32_t fd);

    // ---------------- state accessors (report/dump/tests) ----------------
    static uint64_t GetMoveIntervalCount(size_t bucket);
    static uint64_t GetDispatchLatencyCount(size_t bucket);
    static uint64_t GetTouchpadTwoFingerTapCount();
    static uint64_t GetTouchpadPressRightClickAreaCount();

    // ---------------- testable core ----------------
    // Core motion interval logic: computes the interval against the previous timestamp
    // (only when the previous event came from the same device) and buckets it.
    static void NotifyMotionEvent(int32_t deviceId, int64_t timestampUs);

    // Bucketing helpers (negative values are ignored).
    static void RecordMoveInterval(int64_t intervalUs);
    static void RecordDispatchLatency(int64_t latencyUs);

private:
    // Prints the one-line summary log of a report window (hourly low frequency path).
    static void LogReportSummary(const uint32_t (&moveCounts)[BUCKET_COUNT],
        const uint32_t (&latencyCounts)[BUCKET_COUNT], uint32_t twoFingerTapCount,
        uint32_t pressRightClickAreaCount);

    static std::atomic<uint64_t> moveIntervalCounts_[BUCKET_COUNT];
    static std::atomic<uint64_t> dispatchLatencyCounts_[BUCKET_COUNT];
    static std::atomic<uint64_t> touchpadTwoFingerTapCount_;
    static std::atomic<uint64_t> touchpadPressRightClickAreaCount_;
    static std::atomic<bool> deviceTrimmed_;

    // The following members are only accessed on the server epoll thread.
    static int64_t lastMoveEventTimeUs_;
    static int32_t lastMoveDeviceId_;
    static int64_t dispatchStartTimeUs_;
    static bool dispatchStartPending_;
    static std::atomic<int64_t> lastReportHour_;
    static int32_t reportTimerId_;
};
} // namespace MMI
} // namespace OHOS
#endif // INPUT_EVENT_STATISTICS_H
