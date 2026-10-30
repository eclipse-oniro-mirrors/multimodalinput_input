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

#if MMI_ENABLE_EVENT_STATISTICS

#include <cinttypes>

#include "hisysevent.h"
#include "timer_manager.h"
#include "util_ex.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_SERVER
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "InputEventStatistics"

namespace OHOS {
namespace MMI {
namespace {
/// Report window length: flush counters to HiSysEvent every hour.
constexpr int32_t REPORT_INTERVAL_MS { 60 * 60 * 1000 };
/// Microseconds in one millisecond, used to normalize kernel timestamps.
constexpr int64_t US_PER_MS { 1000 };
/// Interval below this value (ms) counts as a double click / double tap.
constexpr int64_t DOUBLE_CLICK_INTERVAL_MS { 300 };
/// Touch pressure below this value is treated as a suspicious light touch.
constexpr double LOW_PRESSURE_THRESHOLD { 0.1 };

/// Touch tap interval edges in ms: [0,80) [80,200) [200,500) [500,+inf).
constexpr int64_t TOUCH_INTERVAL_EDGE1_MS { 80 };
constexpr int64_t TOUCH_INTERVAL_EDGE2_MS { 200 };
constexpr int64_t TOUCH_INTERVAL_EDGE3_MS { 500 };
/// Key press duration / key interval edges in ms: [0,100) [100,300) [300,1000) [1000,+inf).
constexpr int64_t KEY_DURATION_EDGE1_MS { 100 };
constexpr int64_t KEY_DURATION_EDGE2_MS { 300 };
constexpr int64_t KEY_DURATION_EDGE3_MS { 1000 };
/// Mouse / touch move interval edges in ms: [0,8) [8,16) [16,33) [33,+inf), frame-rate aligned.
constexpr int64_t MOUSE_MOVE_EDGE1_MS { 8 };
constexpr int64_t MOUSE_MOVE_EDGE2_MS { 16 };
constexpr int64_t MOUSE_MOVE_EDGE3_MS { 33 };
/// Touchpad swipe interval edges in ms: [0,100) [100,300) [300,800) [800,+inf).
constexpr int64_t TOUCHPAD_SWIPE_EDGE1_MS { 100 };
constexpr int64_t TOUCHPAD_SWIPE_EDGE2_MS { 300 };
constexpr int64_t TOUCHPAD_SWIPE_EDGE3_MS { 800 };

const char* const DUMP_FILTER_TOUCH { "touch" };
const char* const DUMP_FILTER_KEY { "key" };
const char* const DUMP_FILTER_MOUSE { "mouse" };
const char* const DUMP_FILTER_TOUCHPAD { "touchpad" };
const char* const DUMP_FILTER_JOYSTICK { "joystick" };
const char* const DUMP_FILTER_ALL { "all" };
const char* const DUMP_FILTER_RESET { "reset" };
const char* const DUMP_FILTER_REPORT { "report" };

uint32_t LoadAndReset(std::atomic<uint32_t> &counter)
{
    return counter.exchange(0, std::memory_order_relaxed);
}

void IncBucket(std::atomic<uint32_t> (&buckets)[InputEventStatistics::INTERVAL_BUCKET_COUNT], size_t index)
{
    if (index < InputEventStatistics::INTERVAL_BUCKET_COUNT) {
        buckets[index].fetch_add(1, std::memory_order_relaxed);
    }
}
} // namespace

InputEventStatistics::TouchScreenStats InputEventStatistics::touchScreenStats_;
InputEventStatistics::KeyBoardStats InputEventStatistics::keyBoardStats_;
InputEventStatistics::MouseStats InputEventStatistics::mouseStats_;
InputEventStatistics::TouchpadStats InputEventStatistics::touchpadStats_;
InputEventStatistics::JoystickStats InputEventStatistics::joystickStats_;

std::atomic<int64_t> InputEventStatistics::lastTouchDownTime_ { 0 };
std::atomic<int64_t> InputEventStatistics::lastTouchMoveTime_ { 0 };
std::atomic<int64_t> InputEventStatistics::lastMouseMoveTime_ { 0 };
std::atomic<int64_t> InputEventStatistics::lastTouchpadSwipeTime_ { 0 };
std::atomic<int64_t> InputEventStatistics::lastTouchpadTapTime_ { 0 };
std::atomic<int64_t> InputEventStatistics::lastLeftClickTime_ { 0 };
std::atomic<int64_t> InputEventStatistics::lastKeyDownTime_ { 0 };
std::atomic<int64_t> InputEventStatistics::keyDownTime_ { 0 };
std::atomic<bool> InputEventStatistics::timerStarted_ { false };
std::atomic<uint64_t> InputEventStatistics::reportWindowCount_ { 0 };
std::atomic<int64_t> InputEventStatistics::windowStartTime_ { 0 };
std::mutex InputEventStatistics::dumpMutex_;

size_t InputEventStatistics::IntervalBucket(int64_t intervalUs, const int64_t (&edges)[INTERVAL_BUCKET_COUNT - 1])
{
    const int64_t intervalMs = intervalUs / US_PER_MS;
    for (size_t i = 0; i < INTERVAL_BUCKET_COUNT - 1; ++i) {
        if (intervalMs < edges[i]) {
            return i;
        }
    }
    return INTERVAL_BUCKET_COUNT - 1;
}

void InputEventStatistics::RecordInterval(int64_t nowUs, std::atomic<int64_t> &lastTime,
    std::atomic<uint32_t> (&buckets)[INTERVAL_BUCKET_COUNT], const int64_t (&edges)[INTERVAL_BUCKET_COUNT - 1])
{
    const int64_t last = lastTime.exchange(nowUs, std::memory_order_relaxed);
    if (last <= 0) {
        return;
    }
    const int64_t intervalUs = nowUs - last;
    if (intervalUs >= 0) {
        IncBucket(buckets, IntervalBucket(intervalUs, edges));
    }
}

void InputEventStatistics::RecordPointerEvent(const std::shared_ptr<PointerEvent> &pointerEvent)
{
    CHKPV(pointerEvent);
    // Injected events do not reflect real user behavior, keep them out of statistics.
    if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_SIMULATE) ||
        pointerEvent->HasFlag(InputEvent::EVENT_FLAG_UITEST)) {
        return;
    }
    if (!timerStarted_.load(std::memory_order_relaxed)) {
        StartReportTimer();
    }
    switch (pointerEvent->GetSourceType()) {
        case PointerEvent::SOURCE_TYPE_TOUCHSCREEN: {
            RecordTouchScreenEvent(pointerEvent);
            break;
        }
        case PointerEvent::SOURCE_TYPE_MOUSE: {
            RecordMouseEvent(pointerEvent);
            break;
        }
        case PointerEvent::SOURCE_TYPE_TOUCHPAD: {
            RecordTouchpadEvent(pointerEvent);
            break;
        }
        case PointerEvent::SOURCE_TYPE_JOYSTICK: {
            RecordJoystickEvent(pointerEvent);
            break;
        }
        default: {
            break;
        }
    }
}

void InputEventStatistics::RecordKeyEvent(const std::shared_ptr<KeyEvent> &keyEvent)
{
    CHKPV(keyEvent);
    if (keyEvent->HasFlag(InputEvent::EVENT_FLAG_SIMULATE) ||
        keyEvent->HasFlag(InputEvent::EVENT_FLAG_UITEST)) {
        return;
    }
    if (!timerStarted_.load(std::memory_order_relaxed)) {
        StartReportTimer();
    }
    const int32_t keyAction = keyEvent->GetKeyAction();
    if (keyEvent->IsRepeat()) {
        keyBoardStats_.keyRepeatTimes.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    if (keyAction == KeyEvent::KEY_ACTION_DOWN) {
        keyBoardStats_.keyDownTimes.fetch_add(1, std::memory_order_relaxed);
        // A key event carrying more than one pressed item is a combination key.
        if (keyEvent->GetKeyItems().size() > 1) {
            keyBoardStats_.combinationKeyTimes.fetch_add(1, std::memory_order_relaxed);
        }
        if (keyEvent->GetFunctionKey(KeyEvent::NUM_LOCK_FUNCTION_KEY) ||
            keyEvent->GetFunctionKey(KeyEvent::CAPS_LOCK_FUNCTION_KEY) ||
            keyEvent->GetFunctionKey(KeyEvent::SCROLL_LOCK_FUNCTION_KEY)) {
            keyBoardStats_.functionKeyTimes.fetch_add(1, std::memory_order_relaxed);
        }
        static const int64_t keyIntervalEdges[] = {
            KEY_DURATION_EDGE1_MS, KEY_DURATION_EDGE2_MS, KEY_DURATION_EDGE3_MS
        };
        RecordInterval(keyEvent->GetActionTime(), lastKeyDownTime_, keyBoardStats_.keyIntervalBuckets,
            keyIntervalEdges);
        keyDownTime_.store(keyEvent->GetActionTime(), std::memory_order_relaxed);
    } else if (keyAction == KeyEvent::KEY_ACTION_UP) {
        keyBoardStats_.keyUpTimes.fetch_add(1, std::memory_order_relaxed);
        const int64_t downTime = keyDownTime_.exchange(0, std::memory_order_relaxed);
        if (downTime > 0) {
            static const int64_t edges[] = {
                KEY_DURATION_EDGE1_MS, KEY_DURATION_EDGE2_MS, KEY_DURATION_EDGE3_MS
            };
            const int64_t durationUs = keyEvent->GetActionTime() - downTime;
            if (durationUs >= 0) {
                IncBucket(keyBoardStats_.pressDurationBuckets, IntervalBucket(durationUs, edges));
            }
        }
    }
}

void InputEventStatistics::RecordTouchScreenEvent(const std::shared_ptr<PointerEvent> &pointerEvent)
{
    const int32_t pointerAction = pointerEvent->GetPointerAction();
    switch (pointerAction) {
        case PointerEvent::POINTER_ACTION_DOWN: {
            touchScreenStats_.touchDownTimes.fetch_add(1, std::memory_order_relaxed);
            const size_t fingerCount = pointerEvent->GetPointerIds().size();
            if (fingerCount <= 1) {
                touchScreenStats_.singleFingerTouchTimes.fetch_add(1, std::memory_order_relaxed);
            } else {
                touchScreenStats_.multiFingerTouchTimes.fetch_add(1, std::memory_order_relaxed);
            }
            uint32_t expected = touchScreenStats_.maxFingerCount.load(std::memory_order_relaxed);
            while (fingerCount > expected &&
                !touchScreenStats_.maxFingerCount.compare_exchange_weak(expected,
                    static_cast<uint32_t>(fingerCount), std::memory_order_relaxed)) {
            }
            // Light touches are prone to palm rejection, track them separately.
            PointerEvent::PointerItem item;
            if (pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item) &&
                item.GetPressure() < LOW_PRESSURE_THRESHOLD) {
                touchScreenStats_.lowPressureTimes.fetch_add(1, std::memory_order_relaxed);
            }
            static const int64_t tapEdges[] = {
                TOUCH_INTERVAL_EDGE1_MS, TOUCH_INTERVAL_EDGE2_MS, TOUCH_INTERVAL_EDGE3_MS
            };
            RecordInterval(pointerEvent->GetActionTime(), lastTouchDownTime_,
                touchScreenStats_.tapIntervalBuckets, tapEdges);
            break;
        }
        case PointerEvent::POINTER_ACTION_MOVE: {
            touchScreenStats_.touchMoveTimes.fetch_add(1, std::memory_order_relaxed);
            static const int64_t moveEdges[] = {
                MOUSE_MOVE_EDGE1_MS, MOUSE_MOVE_EDGE2_MS, MOUSE_MOVE_EDGE3_MS
            };
            RecordInterval(pointerEvent->GetActionTime(), lastTouchMoveTime_,
                touchScreenStats_.touchMoveIntervalBuckets, moveEdges);
            break;
        }
        case PointerEvent::POINTER_ACTION_UP: {
            touchScreenStats_.touchUpTimes.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        case PointerEvent::POINTER_ACTION_CANCEL: {
            touchScreenStats_.touchCancelTimes.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        default: {
            break;
        }
    }
}

void InputEventStatistics::RecordMouseEvent(const std::shared_ptr<PointerEvent> &pointerEvent)
{
    const int32_t pointerAction = pointerEvent->GetPointerAction();
    switch (pointerAction) {
        case PointerEvent::POINTER_ACTION_MOVE: {
            mouseStats_.moveTimes.fetch_add(1, std::memory_order_relaxed);
            static const int64_t edges[] = {
                MOUSE_MOVE_EDGE1_MS, MOUSE_MOVE_EDGE2_MS, MOUSE_MOVE_EDGE3_MS
            };
            RecordInterval(pointerEvent->GetActionTime(), lastMouseMoveTime_,
                mouseStats_.moveIntervalBuckets, edges);
            break;
        }
        case PointerEvent::POINTER_ACTION_BUTTON_DOWN: {
            const int32_t buttonId = pointerEvent->GetButtonId();
            if (buttonId == PointerEvent::MOUSE_BUTTON_LEFT) {
                mouseStats_.leftClickTimes.fetch_add(1, std::memory_order_relaxed);
                // Two left clicks within the threshold form a double click.
                const int64_t lastClick = lastLeftClickTime_.exchange(pointerEvent->GetActionTime(),
                    std::memory_order_relaxed);
                if (lastClick > 0 &&
                    (pointerEvent->GetActionTime() - lastClick) / US_PER_MS < DOUBLE_CLICK_INTERVAL_MS) {
                    mouseStats_.doubleClickTimes.fetch_add(1, std::memory_order_relaxed);
                }
            } else if (buttonId == PointerEvent::MOUSE_BUTTON_RIGHT) {
                mouseStats_.rightClickTimes.fetch_add(1, std::memory_order_relaxed);
            } else if (buttonId == PointerEvent::MOUSE_BUTTON_MIDDLE) {
                mouseStats_.middleClickTimes.fetch_add(1, std::memory_order_relaxed);
            }
            break;
        }
        case PointerEvent::POINTER_ACTION_AXIS_UPDATE: {
            const int32_t axes = pointerEvent->GetAxes();
            if (PointerEvent::HasAxis(axes, PointerEvent::AXIS_TYPE_SCROLL_VERTICAL)) {
                mouseStats_.verticalScrollTimes.fetch_add(1, std::memory_order_relaxed);
                // Wheel direction is derived from the sign of the vertical axis value.
                const double axisValue = pointerEvent->GetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_VERTICAL);
                if (axisValue > 0) {
                    mouseStats_.scrollUpTimes.fetch_add(1, std::memory_order_relaxed);
                } else if (axisValue < 0) {
                    mouseStats_.scrollDownTimes.fetch_add(1, std::memory_order_relaxed);
                }
            }
            if (PointerEvent::HasAxis(axes, PointerEvent::AXIS_TYPE_SCROLL_HORIZONTAL)) {
                mouseStats_.horizontalScrollTimes.fetch_add(1, std::memory_order_relaxed);
            }
            break;
        }
        default: {
            break;
        }
    }
}

void InputEventStatistics::RecordTouchpadEvent(const std::shared_ptr<PointerEvent> &pointerEvent)
{
    const int32_t pointerAction = pointerEvent->GetPointerAction();
    const int32_t axes = pointerEvent->GetAxes();
    switch (pointerAction) {
        case PointerEvent::POINTER_ACTION_MOVE: {
            touchpadStats_.moveTimes.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        case PointerEvent::POINTER_ACTION_DOWN: {
            // Finger count of a touchpad gesture, clamped for histogram indexing.
            size_t fingerCount = pointerEvent->GetPointerIds().size();
            if (fingerCount > MAX_FINGER_COUNT) {
                fingerCount = MAX_FINGER_COUNT;
            }
            touchpadStats_.fingerCountTimes[fingerCount].fetch_add(1, std::memory_order_relaxed);
            touchpadStats_.tapTimes.fetch_add(1, std::memory_order_relaxed);
            // Two taps within the threshold form a double tap.
            const int64_t lastTap = lastTouchpadTapTime_.exchange(pointerEvent->GetActionTime(),
                std::memory_order_relaxed);
            if (lastTap > 0 &&
                (pointerEvent->GetActionTime() - lastTap) / US_PER_MS < DOUBLE_CLICK_INTERVAL_MS) {
                touchpadStats_.doubleTapTimes.fetch_add(1, std::memory_order_relaxed);
            }
            break;
        }
        case PointerEvent::POINTER_ACTION_BUTTON_DOWN: {
            touchpadStats_.clickTimes.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        case PointerEvent::POINTER_ACTION_AXIS_UPDATE:
        case PointerEvent::POINTER_ACTION_AXIS_BEGIN: {
            if (PointerEvent::HasAxis(axes, PointerEvent::AXIS_TYPE_PINCH)) {
                touchpadStats_.pinchTimes.fetch_add(1, std::memory_order_relaxed);
            } else if (PointerEvent::HasAxis(axes, PointerEvent::AXIS_TYPE_SCROLL_VERTICAL) ||
                       PointerEvent::HasAxis(axes, PointerEvent::AXIS_TYPE_SCROLL_HORIZONTAL)) {
                touchpadStats_.scrollTimes.fetch_add(1, std::memory_order_relaxed);
            }
            break;
        }
        case PointerEvent::POINTER_ACTION_SWIPE_BEGIN: {
            touchpadStats_.swipeTimes.fetch_add(1, std::memory_order_relaxed);
            static const int64_t edges[] = {
                TOUCHPAD_SWIPE_EDGE1_MS, TOUCHPAD_SWIPE_EDGE2_MS, TOUCHPAD_SWIPE_EDGE3_MS
            };
            RecordInterval(pointerEvent->GetActionTime(), lastTouchpadSwipeTime_,
                touchpadStats_.swipeIntervalBuckets, edges);
            break;
        }
        default: {
            break;
        }
    }
}

void InputEventStatistics::RecordJoystickEvent(const std::shared_ptr<PointerEvent> &pointerEvent)
{
    const int32_t pointerAction = pointerEvent->GetPointerAction();
    switch (pointerAction) {
        case PointerEvent::POINTER_ACTION_BUTTON_DOWN: {
            joystickStats_.buttonDownTimes.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        case PointerEvent::POINTER_ACTION_BUTTON_UP: {
            joystickStats_.buttonUpTimes.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        case PointerEvent::POINTER_ACTION_AXIS_UPDATE: {
            joystickStats_.axisMoveTimes.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        default: {
            break;
        }
    }
}

void InputEventStatistics::StartReportTimer()
{
    bool expected = false;
    if (!timerStarted_.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
        MMI_HILOGD("Input event statistics timer already started");
        return;
    }
    TimerMgr->AddLongTimer(REPORT_INTERVAL_MS, -1, [] {
        TryReport(false);
    }, "InputEventStatistics-Report");
    windowStartTime_.store(GetSysClockTime(), std::memory_order_relaxed);
    MMI_HILOGI("Input event statistics report timer started, interval:%{public}dms", REPORT_INTERVAL_MS);
}

void InputEventStatistics::TryReport(bool force)
{
    const uint32_t touchDown = touchScreenStats_.touchDownTimes.load(std::memory_order_relaxed);
    const uint32_t keyDown = keyBoardStats_.keyDownTimes.load(std::memory_order_relaxed);
    const uint32_t mouseMove = mouseStats_.moveTimes.load(std::memory_order_relaxed);
    const uint32_t touchpadMove = touchpadStats_.moveTimes.load(std::memory_order_relaxed);
    const uint32_t joystickButton = joystickStats_.buttonDownTimes.load(std::memory_order_relaxed);
    const bool emptyWindow = (touchDown == 0 && keyDown == 0 && mouseMove == 0 && touchpadMove == 0 &&
        joystickButton == 0);
    if (emptyWindow && !force) {
        MMI_HILOGD("Statistics window is empty, skip report");
        return;
    }
    reportWindowCount_.fetch_add(1, std::memory_order_relaxed);

    TouchScreenSnapshot touchStat = {
        .touchDownTimes = LoadAndReset(touchScreenStats_.touchDownTimes),
        .touchMoveTimes = LoadAndReset(touchScreenStats_.touchMoveTimes),
        .touchUpTimes = LoadAndReset(touchScreenStats_.touchUpTimes),
        .touchCancelTimes = LoadAndReset(touchScreenStats_.touchCancelTimes),
        .singleFingerTouchTimes = LoadAndReset(touchScreenStats_.singleFingerTouchTimes),
        .multiFingerTouchTimes = LoadAndReset(touchScreenStats_.multiFingerTouchTimes),
        .maxFingerCount = LoadAndReset(touchScreenStats_.maxFingerCount),
        .lowPressureTimes = LoadAndReset(touchScreenStats_.lowPressureTimes),
    };
    for (size_t i = 0; i < INTERVAL_BUCKET_COUNT; ++i) {
        touchStat.tapIntervalBuckets[i] = LoadAndReset(touchScreenStats_.tapIntervalBuckets[i]);
        touchStat.touchMoveIntervalBuckets[i] = LoadAndReset(touchScreenStats_.touchMoveIntervalBuckets[i]);
    }

    KeyBoardSnapshot keyStat = {
        .keyDownTimes = LoadAndReset(keyBoardStats_.keyDownTimes),
        .keyUpTimes = LoadAndReset(keyBoardStats_.keyUpTimes),
        .keyRepeatTimes = LoadAndReset(keyBoardStats_.keyRepeatTimes),
        .combinationKeyTimes = LoadAndReset(keyBoardStats_.combinationKeyTimes),
        .functionKeyTimes = LoadAndReset(keyBoardStats_.functionKeyTimes),
    };
    for (size_t i = 0; i < INTERVAL_BUCKET_COUNT; ++i) {
        keyStat.pressDurationBuckets[i] = LoadAndReset(keyBoardStats_.pressDurationBuckets[i]);
        keyStat.keyIntervalBuckets[i] = LoadAndReset(keyBoardStats_.keyIntervalBuckets[i]);
    }

    MouseSnapshot mouseStat = {
        .moveTimes = LoadAndReset(mouseStats_.moveTimes),
        .leftClickTimes = LoadAndReset(mouseStats_.leftClickTimes),
        .rightClickTimes = LoadAndReset(mouseStats_.rightClickTimes),
        .middleClickTimes = LoadAndReset(mouseStats_.middleClickTimes),
        .verticalScrollTimes = LoadAndReset(mouseStats_.verticalScrollTimes),
        .horizontalScrollTimes = LoadAndReset(mouseStats_.horizontalScrollTimes),
        .dragTimes = LoadAndReset(mouseStats_.dragTimes),
        .doubleClickTimes = LoadAndReset(mouseStats_.doubleClickTimes),
        .scrollUpTimes = LoadAndReset(mouseStats_.scrollUpTimes),
        .scrollDownTimes = LoadAndReset(mouseStats_.scrollDownTimes),
    };
    for (size_t i = 0; i < INTERVAL_BUCKET_COUNT; ++i) {
        mouseStat.moveIntervalBuckets[i] = LoadAndReset(mouseStats_.moveIntervalBuckets[i]);
    }

    TouchpadSnapshot touchpadStat = {
        .moveTimes = LoadAndReset(touchpadStats_.moveTimes),
        .tapTimes = LoadAndReset(touchpadStats_.tapTimes),
        .clickTimes = LoadAndReset(touchpadStats_.clickTimes),
        .scrollTimes = LoadAndReset(touchpadStats_.scrollTimes),
        .swipeTimes = LoadAndReset(touchpadStats_.swipeTimes),
        .pinchTimes = LoadAndReset(touchpadStats_.pinchTimes),
        .doubleTapTimes = LoadAndReset(touchpadStats_.doubleTapTimes),
    };
    for (size_t i = 0; i <= MAX_FINGER_COUNT; ++i) {
        touchpadStat.fingerCountTimes[i] = LoadAndReset(touchpadStats_.fingerCountTimes[i]);
    }
    for (size_t i = 0; i < INTERVAL_BUCKET_COUNT; ++i) {
        touchpadStat.swipeIntervalBuckets[i] = LoadAndReset(touchpadStats_.swipeIntervalBuckets[i]);
    }

    JoystickSnapshot joystickStat = {
        .buttonDownTimes = LoadAndReset(joystickStats_.buttonDownTimes),
        .buttonUpTimes = LoadAndReset(joystickStats_.buttonUpTimes),
        .axisMoveTimes = LoadAndReset(joystickStats_.axisMoveTimes),
    };

    windowStartTime_.store(GetSysClockTime(), std::memory_order_relaxed);
    MMI_HILOGI("Report input event statistics, window:%{public}" PRIu64
        ", touch down:%{public}u, key down:%{public}u, mouse move:%{public}u, touchpad move:%{public}u,"
        " joystick button:%{public}u",
        reportWindowCount_.load(std::memory_order_relaxed),
        touchStat.touchDownTimes, keyStat.keyDownTimes, mouseStat.moveTimes, touchpadStat.moveTimes,
        joystickStat.buttonDownTimes);
    ReportTouchScreenStat(touchStat);
    ReportKeyBoardStat(keyStat);
    ReportMouseStat(mouseStat);
    ReportTouchpadStat(touchpadStat);
    ReportJoystickStat(joystickStat);
}

void InputEventStatistics::ReportTouchScreenStat(const TouchScreenSnapshot &snapshot)
{
    int32_t ret = HiSysEventWrite(
        OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MODAL_INPUT,
        "INPUT_TOUCH_SCREEN_STATISTIC",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "TOUCH_DOWN_TIMES", snapshot.touchDownTimes,
        "TOUCH_MOVE_TIMES", snapshot.touchMoveTimes,
        "TOUCH_UP_TIMES", snapshot.touchUpTimes,
        "TOUCH_CANCEL_TIMES", snapshot.touchCancelTimes,
        "SINGLE_FINGER_TIMES", snapshot.singleFingerTouchTimes,
        "MULTI_FINGER_TIMES", snapshot.multiFingerTouchTimes,
        "MAX_FINGER_COUNT", snapshot.maxFingerCount,
        "LOW_PRESSURE_TIMES", snapshot.lowPressureTimes,
        "TAP_INTERVAL_BELOW80MS", snapshot.tapIntervalBuckets[0],
        "TAP_INTERVAL_BELOW200MS", snapshot.tapIntervalBuckets[1],
        "TAP_INTERVAL_BELOW500MS", snapshot.tapIntervalBuckets[2],
        "TAP_INTERVAL_ABOVE500MS", snapshot.tapIntervalBuckets[3],
        "TOUCH_MOVE_INTERVAL_BELOW8MS", snapshot.touchMoveIntervalBuckets[0],
        "TOUCH_MOVE_INTERVAL_BELOW16MS", snapshot.touchMoveIntervalBuckets[1],
        "TOUCH_MOVE_INTERVAL_BELOW33MS", snapshot.touchMoveIntervalBuckets[2],
        "TOUCH_MOVE_INTERVAL_ABOVE33MS", snapshot.touchMoveIntervalBuckets[3],
        "MSG", "Touch screen event statistics of one report window");
    if (ret != 0) {
        MMI_HILOGE("HiviewDFX Write failed, ret:%{public}d", ret);
    }
}

void InputEventStatistics::ReportKeyBoardStat(const KeyBoardSnapshot &snapshot)
{
    int32_t ret = HiSysEventWrite(
        OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MODAL_INPUT,
        "INPUT_KEY_EVENT_STATISTIC",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "KEY_DOWN_TIMES", snapshot.keyDownTimes,
        "KEY_UP_TIMES", snapshot.keyUpTimes,
        "KEY_REPEAT_TIMES", snapshot.keyRepeatTimes,
        "COMBINATION_KEY_TIMES", snapshot.combinationKeyTimes,
        "FUNCTION_KEY_TIMES", snapshot.functionKeyTimes,
        "PRESS_DURATION_BELOW100MS", snapshot.pressDurationBuckets[0],
        "PRESS_DURATION_BELOW300MS", snapshot.pressDurationBuckets[1],
        "PRESS_DURATION_BELOW1000MS", snapshot.pressDurationBuckets[2],
        "PRESS_DURATION_ABOVE1000MS", snapshot.pressDurationBuckets[3],
        "KEY_INTERVAL_BELOW100MS", snapshot.keyIntervalBuckets[0],
        "KEY_INTERVAL_BELOW300MS", snapshot.keyIntervalBuckets[1],
        "KEY_INTERVAL_BELOW1000MS", snapshot.keyIntervalBuckets[2],
        "KEY_INTERVAL_ABOVE1000MS", snapshot.keyIntervalBuckets[3],
        "MSG", "Keyboard event statistics of one report window");
    if (ret != 0) {
        MMI_HILOGE("HiviewDFX Write failed, ret:%{public}d", ret);
    }
}

void InputEventStatistics::ReportMouseStat(const MouseSnapshot &snapshot)
{
    int32_t ret = HiSysEventWrite(
        OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MODAL_INPUT,
        "INPUT_MOUSE_EVENT_STATISTIC",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "MOVE_TIMES", snapshot.moveTimes,
        "LEFT_CLICK_TIMES", snapshot.leftClickTimes,
        "RIGHT_CLICK_TIMES", snapshot.rightClickTimes,
        "MIDDLE_CLICK_TIMES", snapshot.middleClickTimes,
        "VERTICAL_SCROLL_TIMES", snapshot.verticalScrollTimes,
        "HORIZONTAL_SCROLL_TIMES", snapshot.horizontalScrollTimes,
        "DRAG_TIMES", snapshot.dragTimes,
        "DOUBLE_CLICK_TIMES", snapshot.doubleClickTimes,
        "SCROLL_UP_TIMES", snapshot.scrollUpTimes,
        "SCROLL_DOWN_TIMES", snapshot.scrollDownTimes,
        "MOVE_INTERVAL_BELOW8MS", snapshot.moveIntervalBuckets[0],
        "MOVE_INTERVAL_BELOW16MS", snapshot.moveIntervalBuckets[1],
        "MOVE_INTERVAL_BELOW33MS", snapshot.moveIntervalBuckets[2],
        "MOVE_INTERVAL_ABOVE33MS", snapshot.moveIntervalBuckets[3],
        "MSG", "Mouse event statistics of one report window");
    if (ret != 0) {
        MMI_HILOGE("HiviewDFX Write failed, ret:%{public}d", ret);
    }
}

void InputEventStatistics::ReportTouchpadStat(const TouchpadSnapshot &snapshot)
{
    int32_t ret = HiSysEventWrite(
        OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MODAL_INPUT,
        "INPUT_TOUCHPAD_EVENT_STATISTIC",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "MOVE_TIMES", snapshot.moveTimes,
        "TAP_TIMES", snapshot.tapTimes,
        "CLICK_TIMES", snapshot.clickTimes,
        "SCROLL_TIMES", snapshot.scrollTimes,
        "SWIPE_TIMES", snapshot.swipeTimes,
        "PINCH_TIMES", snapshot.pinchTimes,
        "DOUBLE_TAP_TIMES", snapshot.doubleTapTimes,
        "ONE_FINGER_TIMES", snapshot.fingerCountTimes[1],
        "TWO_FINGER_TIMES", snapshot.fingerCountTimes[2],
        "THREE_FINGER_TIMES", snapshot.fingerCountTimes[3],
        "FOUR_FINGER_TIMES", snapshot.fingerCountTimes[4],
        "FIVE_FINGER_TIMES", snapshot.fingerCountTimes[5],
        "SWIPE_INTERVAL_BELOW100MS", snapshot.swipeIntervalBuckets[0],
        "SWIPE_INTERVAL_BELOW300MS", snapshot.swipeIntervalBuckets[1],
        "SWIPE_INTERVAL_BELOW800MS", snapshot.swipeIntervalBuckets[2],
        "SWIPE_INTERVAL_ABOVE800MS", snapshot.swipeIntervalBuckets[3],
        "MSG", "Touchpad event statistics of one report window");
    if (ret != 0) {
        MMI_HILOGE("HiviewDFX Write failed, ret:%{public}d", ret);
    }
}

void InputEventStatistics::ReportJoystickStat(const JoystickSnapshot &snapshot)
{
    int32_t ret = HiSysEventWrite(
        OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MODAL_INPUT,
        "INPUT_JOYSTICK_EVENT_STATISTIC",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "BUTTON_DOWN_TIMES", snapshot.buttonDownTimes,
        "BUTTON_UP_TIMES", snapshot.buttonUpTimes,
        "AXIS_MOVE_TIMES", snapshot.axisMoveTimes,
        "MSG", "Joystick event statistics of one report window");
    if (ret != 0) {
        MMI_HILOGE("HiviewDFX Write failed, ret:%{public}d", ret);
    }
}

void InputEventStatistics::Reset()
{
    touchScreenStats_.touchDownTimes.store(0, std::memory_order_relaxed);
    touchScreenStats_.touchMoveTimes.store(0, std::memory_order_relaxed);
    touchScreenStats_.touchUpTimes.store(0, std::memory_order_relaxed);
    touchScreenStats_.touchCancelTimes.store(0, std::memory_order_relaxed);
    touchScreenStats_.singleFingerTouchTimes.store(0, std::memory_order_relaxed);
    touchScreenStats_.multiFingerTouchTimes.store(0, std::memory_order_relaxed);
    touchScreenStats_.maxFingerCount.store(0, std::memory_order_relaxed);
    touchScreenStats_.lowPressureTimes.store(0, std::memory_order_relaxed);
    for (auto &bucket : touchScreenStats_.tapIntervalBuckets) {
        bucket.store(0, std::memory_order_relaxed);
    }
    for (auto &bucket : touchScreenStats_.touchMoveIntervalBuckets) {
        bucket.store(0, std::memory_order_relaxed);
    }
    keyBoardStats_.keyDownTimes.store(0, std::memory_order_relaxed);
    keyBoardStats_.keyUpTimes.store(0, std::memory_order_relaxed);
    keyBoardStats_.keyRepeatTimes.store(0, std::memory_order_relaxed);
    keyBoardStats_.combinationKeyTimes.store(0, std::memory_order_relaxed);
    keyBoardStats_.functionKeyTimes.store(0, std::memory_order_relaxed);
    for (auto &bucket : keyBoardStats_.pressDurationBuckets) {
        bucket.store(0, std::memory_order_relaxed);
    }
    for (auto &bucket : keyBoardStats_.keyIntervalBuckets) {
        bucket.store(0, std::memory_order_relaxed);
    }
    mouseStats_.moveTimes.store(0, std::memory_order_relaxed);
    mouseStats_.leftClickTimes.store(0, std::memory_order_relaxed);
    mouseStats_.rightClickTimes.store(0, std::memory_order_relaxed);
    mouseStats_.middleClickTimes.store(0, std::memory_order_relaxed);
    mouseStats_.verticalScrollTimes.store(0, std::memory_order_relaxed);
    mouseStats_.horizontalScrollTimes.store(0, std::memory_order_relaxed);
    mouseStats_.dragTimes.store(0, std::memory_order_relaxed);
    mouseStats_.doubleClickTimes.store(0, std::memory_order_relaxed);
    mouseStats_.scrollUpTimes.store(0, std::memory_order_relaxed);
    mouseStats_.scrollDownTimes.store(0, std::memory_order_relaxed);
    for (auto &bucket : mouseStats_.moveIntervalBuckets) {
        bucket.store(0, std::memory_order_relaxed);
    }
    touchpadStats_.moveTimes.store(0, std::memory_order_relaxed);
    touchpadStats_.tapTimes.store(0, std::memory_order_relaxed);
    touchpadStats_.clickTimes.store(0, std::memory_order_relaxed);
    touchpadStats_.scrollTimes.store(0, std::memory_order_relaxed);
    touchpadStats_.swipeTimes.store(0, std::memory_order_relaxed);
    touchpadStats_.pinchTimes.store(0, std::memory_order_relaxed);
    touchpadStats_.doubleTapTimes.store(0, std::memory_order_relaxed);
    for (auto &times : touchpadStats_.fingerCountTimes) {
        times.store(0, std::memory_order_relaxed);
    }
    for (auto &bucket : touchpadStats_.swipeIntervalBuckets) {
        bucket.store(0, std::memory_order_relaxed);
    }
    joystickStats_.buttonDownTimes.store(0, std::memory_order_relaxed);
    joystickStats_.buttonUpTimes.store(0, std::memory_order_relaxed);
    joystickStats_.axisMoveTimes.store(0, std::memory_order_relaxed);
    lastTouchDownTime_.store(0, std::memory_order_relaxed);
    lastTouchMoveTime_.store(0, std::memory_order_relaxed);
    lastMouseMoveTime_.store(0, std::memory_order_relaxed);
    lastTouchpadSwipeTime_.store(0, std::memory_order_relaxed);
    lastTouchpadTapTime_.store(0, std::memory_order_relaxed);
    lastLeftClickTime_.store(0, std::memory_order_relaxed);
    lastKeyDownTime_.store(0, std::memory_order_relaxed);
    keyDownTime_.store(0, std::memory_order_relaxed);
}

void InputEventStatistics::Dump(int32_t fd, const std::vector<std::string> &args)
{
    std::lock_guard<std::mutex> lock(dumpMutex_);
    std::string filter = DUMP_FILTER_ALL;
    for (const auto &arg : args) {
        if (arg == DUMP_FILTER_TOUCH || arg == DUMP_FILTER_KEY || arg == DUMP_FILTER_MOUSE ||
            arg == DUMP_FILTER_TOUCHPAD || arg == DUMP_FILTER_JOYSTICK || arg == DUMP_FILTER_ALL) {
            filter = arg;
        } else if (arg == DUMP_FILTER_RESET) {
            Reset();
            mprintf(fd, "Input event statistics has been reset\n");
            return;
        } else if (arg == DUMP_FILTER_REPORT) {
            TryReport(true);
            mprintf(fd, "Input event statistics window has been reported\n");
            return;
        }
    }
    DumpSummary(fd);
    if (filter == DUMP_FILTER_ALL || filter == DUMP_FILTER_TOUCH) {
        DumpTouchScreen(fd);
    }
    if (filter == DUMP_FILTER_ALL || filter == DUMP_FILTER_KEY) {
        DumpKeyBoard(fd);
    }
    if (filter == DUMP_FILTER_ALL || filter == DUMP_FILTER_MOUSE) {
        DumpMouse(fd);
    }
    if (filter == DUMP_FILTER_ALL || filter == DUMP_FILTER_TOUCHPAD) {
        DumpTouchpad(fd);
    }
    if (filter == DUMP_FILTER_ALL || filter == DUMP_FILTER_JOYSTICK) {
        DumpJoystick(fd);
    }
}

void InputEventStatistics::DumpSummary(int32_t fd)
{
    mprintf(fd, "Input Event Statistics:\n");
    mprintf(fd, "  Report window count: %" PRIu64 "\n", reportWindowCount_.load(std::memory_order_relaxed));
    const int64_t windowStart = windowStartTime_.load(std::memory_order_relaxed);
    if (windowStart > 0) {
        const int64_t elapsedMs = (GetSysClockTime() - windowStart) / US_PER_MS;
        mprintf(fd, "  Current window elapsed time (ms): %" PRId64 "\n", elapsedMs);
    }
    mprintf(fd, "  TouchScreen down/move/up/cancel: %u/%u/%u/%u\n",
        touchScreenStats_.touchDownTimes.load(std::memory_order_relaxed),
        touchScreenStats_.touchMoveTimes.load(std::memory_order_relaxed),
        touchScreenStats_.touchUpTimes.load(std::memory_order_relaxed),
        touchScreenStats_.touchCancelTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Keyboard down/up/repeat/combo: %u/%u/%u/%u\n",
        keyBoardStats_.keyDownTimes.load(std::memory_order_relaxed),
        keyBoardStats_.keyUpTimes.load(std::memory_order_relaxed),
        keyBoardStats_.keyRepeatTimes.load(std::memory_order_relaxed),
        keyBoardStats_.combinationKeyTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Mouse move/left/right/middle: %u/%u/%u/%u\n",
        mouseStats_.moveTimes.load(std::memory_order_relaxed),
        mouseStats_.leftClickTimes.load(std::memory_order_relaxed),
        mouseStats_.rightClickTimes.load(std::memory_order_relaxed),
        mouseStats_.middleClickTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Touchpad move/tap/click/scroll/swipe/pinch: %u/%u/%u/%u/%u/%u\n",
        touchpadStats_.moveTimes.load(std::memory_order_relaxed),
        touchpadStats_.tapTimes.load(std::memory_order_relaxed),
        touchpadStats_.clickTimes.load(std::memory_order_relaxed),
        touchpadStats_.scrollTimes.load(std::memory_order_relaxed),
        touchpadStats_.swipeTimes.load(std::memory_order_relaxed),
        touchpadStats_.pinchTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Joystick buttonDown/buttonUp/axisMove: %u/%u/%u\n",
        joystickStats_.buttonDownTimes.load(std::memory_order_relaxed),
        joystickStats_.buttonUpTimes.load(std::memory_order_relaxed),
        joystickStats_.axisMoveTimes.load(std::memory_order_relaxed));
}

void InputEventStatistics::DumpTouchScreen(int32_t fd)
{
    mprintf(fd, "TouchScreen Statistics:\n");
    mprintf(fd, "  Touch down times: %u\n",
        touchScreenStats_.touchDownTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Touch move times: %u\n",
        touchScreenStats_.touchMoveTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Touch up times: %u\n",
        touchScreenStats_.touchUpTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Touch cancel times: %u\n",
        touchScreenStats_.touchCancelTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Single finger touch times: %u\n",
        touchScreenStats_.singleFingerTouchTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Multi finger touch times: %u\n",
        touchScreenStats_.multiFingerTouchTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Max concurrent finger count: %u\n",
        touchScreenStats_.maxFingerCount.load(std::memory_order_relaxed));
    mprintf(fd, "  Low pressure touch times: %u\n",
        touchScreenStats_.lowPressureTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Tap interval buckets (ms) [0,80)/[80,200)/[200,500)/[500,+inf): %u/%u/%u/%u\n",
        touchScreenStats_.tapIntervalBuckets[0].load(std::memory_order_relaxed),
        touchScreenStats_.tapIntervalBuckets[1].load(std::memory_order_relaxed),
        touchScreenStats_.tapIntervalBuckets[2].load(std::memory_order_relaxed),
        touchScreenStats_.tapIntervalBuckets[3].load(std::memory_order_relaxed));
    mprintf(fd, "  Move interval buckets (ms) [0,8)/[8,16)/[16,33)/[33,+inf): %u/%u/%u/%u\n",
        touchScreenStats_.touchMoveIntervalBuckets[0].load(std::memory_order_relaxed),
        touchScreenStats_.touchMoveIntervalBuckets[1].load(std::memory_order_relaxed),
        touchScreenStats_.touchMoveIntervalBuckets[2].load(std::memory_order_relaxed),
        touchScreenStats_.touchMoveIntervalBuckets[3].load(std::memory_order_relaxed));
}

void InputEventStatistics::DumpKeyBoard(int32_t fd)
{
    mprintf(fd, "Keyboard Statistics:\n");
    mprintf(fd, "  Key down times: %u\n",
        keyBoardStats_.keyDownTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Key up times: %u\n",
        keyBoardStats_.keyUpTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Key repeat times: %u\n",
        keyBoardStats_.keyRepeatTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Combination key times: %u\n",
        keyBoardStats_.combinationKeyTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Function key times: %u\n",
        keyBoardStats_.functionKeyTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Press duration buckets (ms) [0,100)/[100,300)/[300,1000)/[1000,+inf): %u/%u/%u/%u\n",
        keyBoardStats_.pressDurationBuckets[0].load(std::memory_order_relaxed),
        keyBoardStats_.pressDurationBuckets[1].load(std::memory_order_relaxed),
        keyBoardStats_.pressDurationBuckets[2].load(std::memory_order_relaxed),
        keyBoardStats_.pressDurationBuckets[3].load(std::memory_order_relaxed));
    mprintf(fd, "  Key interval buckets (ms) [0,100)/[100,300)/[300,1000)/[1000,+inf): %u/%u/%u/%u\n",
        keyBoardStats_.keyIntervalBuckets[0].load(std::memory_order_relaxed),
        keyBoardStats_.keyIntervalBuckets[1].load(std::memory_order_relaxed),
        keyBoardStats_.keyIntervalBuckets[2].load(std::memory_order_relaxed),
        keyBoardStats_.keyIntervalBuckets[3].load(std::memory_order_relaxed));
}

void InputEventStatistics::DumpMouse(int32_t fd)
{
    mprintf(fd, "Mouse Statistics:\n");
    mprintf(fd, "  Move times: %u\n", mouseStats_.moveTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Left click times: %u\n",
        mouseStats_.leftClickTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Right click times: %u\n",
        mouseStats_.rightClickTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Middle click times: %u\n",
        mouseStats_.middleClickTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Vertical scroll times: %u\n",
        mouseStats_.verticalScrollTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Horizontal scroll times: %u\n",
        mouseStats_.horizontalScrollTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Drag times: %u\n", mouseStats_.dragTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Double click times: %u\n",
        mouseStats_.doubleClickTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Scroll up/down times: %u/%u\n",
        mouseStats_.scrollUpTimes.load(std::memory_order_relaxed),
        mouseStats_.scrollDownTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Move interval buckets (ms) [0,8)/[8,16)/[16,33)/[33,+inf): %u/%u/%u/%u\n",
        mouseStats_.moveIntervalBuckets[0].load(std::memory_order_relaxed),
        mouseStats_.moveIntervalBuckets[1].load(std::memory_order_relaxed),
        mouseStats_.moveIntervalBuckets[2].load(std::memory_order_relaxed),
        mouseStats_.moveIntervalBuckets[3].load(std::memory_order_relaxed));
}

void InputEventStatistics::DumpTouchpad(int32_t fd)
{
    mprintf(fd, "Touchpad Statistics:\n");
    mprintf(fd, "  Move times: %u\n", touchpadStats_.moveTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Tap times: %u\n", touchpadStats_.tapTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Click times: %u\n", touchpadStats_.clickTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Scroll times: %u\n", touchpadStats_.scrollTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Swipe times: %u\n", touchpadStats_.swipeTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Pinch times: %u\n", touchpadStats_.pinchTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Double tap times: %u\n",
        touchpadStats_.doubleTapTimes.load(std::memory_order_relaxed));
    for (size_t i = 1; i <= MAX_FINGER_COUNT; ++i) {
        mprintf(fd, "  Finger count %zu down times: %u\n", i,
            touchpadStats_.fingerCountTimes[i].load(std::memory_order_relaxed));
    }
    mprintf(fd, "  Swipe interval buckets (ms) [0,100)/[100,300)/[300,800)/[800,+inf): %u/%u/%u/%u\n",
        touchpadStats_.swipeIntervalBuckets[0].load(std::memory_order_relaxed),
        touchpadStats_.swipeIntervalBuckets[1].load(std::memory_order_relaxed),
        touchpadStats_.swipeIntervalBuckets[2].load(std::memory_order_relaxed),
        touchpadStats_.swipeIntervalBuckets[3].load(std::memory_order_relaxed));
}

void InputEventStatistics::DumpJoystick(int32_t fd)
{
    mprintf(fd, "Joystick Statistics:\n");
    mprintf(fd, "  Button down times: %u\n",
        joystickStats_.buttonDownTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Button up times: %u\n",
        joystickStats_.buttonUpTimes.load(std::memory_order_relaxed));
    mprintf(fd, "  Axis move times: %u\n",
        joystickStats_.axisMoveTimes.load(std::memory_order_relaxed));
}
} // namespace MMI
} // namespace OHOS

#endif // MMI_ENABLE_EVENT_STATISTICS
