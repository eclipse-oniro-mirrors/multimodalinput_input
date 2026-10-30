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

/**
 * Master switch of the per-category input event statistics feature, off by default.
 * Set to 1 to enable the touch/key/mouse/touchpad statistics, hidumper option
 * "-S" and the periodic HiSysEvent reporting. Define it through build flags
 * (e.g. -DMMI_ENABLE_EVENT_STATISTICS=1) or flip the default here.
 */
#ifndef MMI_ENABLE_EVENT_STATISTICS
#define MMI_ENABLE_EVENT_STATISTICS 0
#endif

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "key_event.h"
#include "pointer_event.h"

namespace OHOS {
namespace MMI {
/**
 * @brief Per-category input event statistics for touch screen, keyboard, mouse,
 * touchpad and joystick.
 *
 * The class is invoked on the event normalization path, so all counters are
 * lock-free (std::atomic). Aggregated snapshots are reported to HiSysEvent
 * once per report window and can be dumped on demand through hidumper:
 *   hidumper -s 3101 -a "-S"                : dump all categories
 *   hidumper -s 3101 -a "-S touch"           : dump touch screen statistics only
 *   hidumper -s 3101 -a "-S key"             : dump keyboard statistics only
 *   hidumper -s 3101 -a "-S mouse"           : dump mouse statistics only
 *   hidumper -s 3101 -a "-S touchpad"        : dump touchpad statistics only
 *   hidumper -s 3101 -a "-S joystick"        : dump joystick statistics only
 *   hidumper -s 3101 -a "-S report"          : report current window now
 *   hidumper -s 3101 -a "-S reset"           : clear all statistics
 */
class InputEventStatistics final {
public:
    /// Number of buckets used by interval/duration histograms.
    static constexpr size_t INTERVAL_BUCKET_COUNT { 4 };
    /// Statistics collected per physical finger count, index 0 is unused.
    static constexpr size_t MAX_FINGER_COUNT { 5 };

    InputEventStatistics() = delete;
    ~InputEventStatistics() = delete;

    /**
     * @brief Record a normalized pointer event into its device category.
     * @param pointerEvent Normalized pointer event, may be nullptr.
     */
    static void RecordPointerEvent(const std::shared_ptr<PointerEvent> &pointerEvent);

    /**
     * @brief Record a normalized key event.
     * @param keyEvent Normalized key event, may be nullptr.
     */
    static void RecordKeyEvent(const std::shared_ptr<KeyEvent> &keyEvent);

    /**
     * @brief Start the periodic report timer. Also started lazily by the first
     * recorded event, so no service startup hook is required. Safe to call
     * more than once.
     */
    static void StartReportTimer();

    /**
     * @brief Report current window to HiSysEvent and reset counters.
     * @param force Report even when the window is empty.
     */
    static void TryReport(bool force);

    /**
     * @brief Dump statistics of hidumper.
     * @param fd Output file descriptor.
     * @param args Optional category filter: touch/key/mouse/touchpad/all/reset.
     */
    static void Dump(int32_t fd, const std::vector<std::string> &args);

    /// Reset every counter to zero.
    static void Reset();

private:
    static void RecordTouchScreenEvent(const std::shared_ptr<PointerEvent> &pointerEvent);
    static void RecordMouseEvent(const std::shared_ptr<PointerEvent> &pointerEvent);
    static void RecordTouchpadEvent(const std::shared_ptr<PointerEvent> &pointerEvent);
    static void RecordJoystickEvent(const std::shared_ptr<PointerEvent> &pointerEvent);

    static void DumpTouchScreen(int32_t fd);
    static void DumpKeyBoard(int32_t fd);
    static void DumpMouse(int32_t fd);
    static void DumpTouchpad(int32_t fd);
    static void DumpJoystick(int32_t fd);
    static void DumpSummary(int32_t fd);

    /// Plain snapshot of one category, filled by TryReport before reporting.
    struct TouchScreenSnapshot {
        uint32_t touchDownTimes;
        uint32_t touchMoveTimes;
        uint32_t touchUpTimes;
        uint32_t touchCancelTimes;
        uint32_t singleFingerTouchTimes;
        uint32_t multiFingerTouchTimes;
        uint32_t maxFingerCount;
        uint32_t lowPressureTimes;
        uint32_t tapIntervalBuckets[INTERVAL_BUCKET_COUNT];
        uint32_t touchMoveIntervalBuckets[INTERVAL_BUCKET_COUNT];
    };
    struct KeyBoardSnapshot {
        uint32_t keyDownTimes;
        uint32_t keyUpTimes;
        uint32_t keyRepeatTimes;
        uint32_t combinationKeyTimes;
        uint32_t functionKeyTimes;
        uint32_t pressDurationBuckets[INTERVAL_BUCKET_COUNT];
        uint32_t keyIntervalBuckets[INTERVAL_BUCKET_COUNT];
    };
    struct MouseSnapshot {
        uint32_t moveTimes;
        uint32_t leftClickTimes;
        uint32_t rightClickTimes;
        uint32_t middleClickTimes;
        uint32_t verticalScrollTimes;
        uint32_t horizontalScrollTimes;
        uint32_t dragTimes;
        uint32_t doubleClickTimes;
        uint32_t scrollUpTimes;
        uint32_t scrollDownTimes;
        uint32_t moveIntervalBuckets[INTERVAL_BUCKET_COUNT];
    };
    struct TouchpadSnapshot {
        uint32_t moveTimes;
        uint32_t tapTimes;
        uint32_t clickTimes;
        uint32_t scrollTimes;
        uint32_t swipeTimes;
        uint32_t pinchTimes;
        uint32_t doubleTapTimes;
        uint32_t fingerCountTimes[MAX_FINGER_COUNT + 1];
        uint32_t swipeIntervalBuckets[INTERVAL_BUCKET_COUNT];
    };
    struct JoystickSnapshot {
        uint32_t buttonDownTimes;
        uint32_t buttonUpTimes;
        uint32_t axisMoveTimes;
    };

    static void ReportTouchScreenStat(const TouchScreenSnapshot &snapshot);
    static void ReportKeyBoardStat(const KeyBoardSnapshot &snapshot);
    static void ReportMouseStat(const MouseSnapshot &snapshot);
    static void ReportTouchpadStat(const TouchpadSnapshot &snapshot);
    static void ReportJoystickStat(const JoystickSnapshot &snapshot);

    static size_t IntervalBucket(int64_t intervalUs, const int64_t (&edges)[INTERVAL_BUCKET_COUNT - 1]);
    static void RecordInterval(int64_t nowUs, std::atomic<int64_t> &lastTime,
        std::atomic<uint32_t> (&buckets)[INTERVAL_BUCKET_COUNT],
        const int64_t (&edges)[INTERVAL_BUCKET_COUNT - 1]);

    struct TouchScreenStats {
        std::atomic<uint32_t> touchDownTimes { 0 };
        std::atomic<uint32_t> touchMoveTimes { 0 };
        std::atomic<uint32_t> touchUpTimes { 0 };
        std::atomic<uint32_t> touchCancelTimes { 0 };
        std::atomic<uint32_t> singleFingerTouchTimes { 0 };
        std::atomic<uint32_t> multiFingerTouchTimes { 0 };
        std::atomic<uint32_t> maxFingerCount { 0 };
        std::atomic<uint32_t> lowPressureTimes { 0 };
        std::atomic<uint32_t> tapIntervalBuckets[INTERVAL_BUCKET_COUNT] = {};
        std::atomic<uint32_t> touchMoveIntervalBuckets[INTERVAL_BUCKET_COUNT] = {};
    };

    struct KeyBoardStats {
        std::atomic<uint32_t> keyDownTimes { 0 };
        std::atomic<uint32_t> keyUpTimes { 0 };
        std::atomic<uint32_t> keyRepeatTimes { 0 };
        std::atomic<uint32_t> combinationKeyTimes { 0 };
        std::atomic<uint32_t> functionKeyTimes { 0 };
        std::atomic<uint32_t> pressDurationBuckets[INTERVAL_BUCKET_COUNT] = {};
        std::atomic<uint32_t> keyIntervalBuckets[INTERVAL_BUCKET_COUNT] = {};
    };

    struct MouseStats {
        std::atomic<uint32_t> moveTimes { 0 };
        std::atomic<uint32_t> leftClickTimes { 0 };
        std::atomic<uint32_t> rightClickTimes { 0 };
        std::atomic<uint32_t> middleClickTimes { 0 };
        std::atomic<uint32_t> verticalScrollTimes { 0 };
        std::atomic<uint32_t> horizontalScrollTimes { 0 };
        std::atomic<uint32_t> dragTimes { 0 };
        std::atomic<uint32_t> doubleClickTimes { 0 };
        std::atomic<uint32_t> scrollUpTimes { 0 };
        std::atomic<uint32_t> scrollDownTimes { 0 };
        std::atomic<uint32_t> moveIntervalBuckets[INTERVAL_BUCKET_COUNT] = {};
    };

    struct TouchpadStats {
        std::atomic<uint32_t> moveTimes { 0 };
        std::atomic<uint32_t> tapTimes { 0 };
        std::atomic<uint32_t> clickTimes { 0 };
        std::atomic<uint32_t> scrollTimes { 0 };
        std::atomic<uint32_t> swipeTimes { 0 };
        std::atomic<uint32_t> pinchTimes { 0 };
        std::atomic<uint32_t> doubleTapTimes { 0 };
        std::atomic<uint32_t> fingerCountTimes[MAX_FINGER_COUNT + 1] = {};
        std::atomic<uint32_t> swipeIntervalBuckets[INTERVAL_BUCKET_COUNT] = {};
    };

    struct JoystickStats {
        std::atomic<uint32_t> buttonDownTimes { 0 };
        std::atomic<uint32_t> buttonUpTimes { 0 };
        std::atomic<uint32_t> axisMoveTimes { 0 };
    };

    static TouchScreenStats touchScreenStats_;
    static KeyBoardStats keyBoardStats_;
    static MouseStats mouseStats_;
    static TouchpadStats touchpadStats_;
    static JoystickStats joystickStats_;

    static std::atomic<int64_t> lastTouchDownTime_;
    static std::atomic<int64_t> lastTouchMoveTime_;
    static std::atomic<int64_t> lastMouseMoveTime_;
    static std::atomic<int64_t> lastTouchpadSwipeTime_;
    static std::atomic<int64_t> lastTouchpadTapTime_;
    static std::atomic<int64_t> lastLeftClickTime_;
    static std::atomic<int64_t> lastKeyDownTime_;
    static std::atomic<int64_t> keyDownTime_;

    static std::atomic<bool> timerStarted_;
    static std::atomic<uint64_t> reportWindowCount_;
    static std::atomic<int64_t> windowStartTime_;
    static std::mutex dumpMutex_;
};
} // namespace MMI
} // namespace OHOS
#endif // INPUT_EVENT_STATISTICS_H
