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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "input_event_statistics.h"
#include "mmi_log.h"
#include "pointer_event.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "InputEventStatisticsTest"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t TEST_DEVICE_ID { 1 };
constexpr int64_t US_PER_MS { 1000 };

std::shared_ptr<PointerEvent> CreatePointerEvent(int32_t sourceType, int32_t deviceId)
{
    auto point = PointerEvent::Create();
    if (point == nullptr) {
        return nullptr;
    }
    point->SetSourceType(sourceType);
    point->SetDeviceId(deviceId);
    return point;
}

uint64_t SumMoveIntervalCounts()
{
    uint64_t total = 0;
    for (size_t i = 0; i < InputEventStatistics::BUCKET_COUNT; ++i) {
        total += InputEventStatistics::GetMoveIntervalCount(i);
    }
    return total;
}

uint64_t SumDispatchLatencyCounts()
{
    uint64_t total = 0;
    for (size_t i = 0; i < InputEventStatistics::BUCKET_COUNT; ++i) {
        total += InputEventStatistics::GetDispatchLatencyCount(i);
    }
    return total;
}

std::string DumpStatsToString(void (*dumpFn)(int32_t))
{
    char path[] = "/data/local/tmp/input_event_statistics_test_XXXXXX";
    int32_t fd = mkstemp(path);
    if (fd < 0) {
        return "";
    }
    dumpFn(fd);
    close(fd);
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    if (remove(path) != 0) {
        MMI_HILOGE("Remove temp file failed, path:%{public}s", path);
    }
    return ss.str();
}
} // namespace

class InputEventStatisticsTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp()
    {
        InputEventStatistics::SetDeviceTrimmed(false);
        InputEventStatistics::OnDeviceRemoved(TEST_DEVICE_ID);
        InputEventStatistics::OnDeviceRemoved(0);
        InputEventStatistics::Reset();
    }
    void TearDown()
    {
        InputEventStatistics::SetDeviceTrimmed(false);
        InputEventStatistics::OnDeviceRemoved(TEST_DEVICE_ID);
        InputEventStatistics::OnDeviceRemoved(0);
        InputEventStatistics::Reset();
    }
};

/**
 * @tc.name: InputEventStatisticsTest_RecordMoveIntervalTest_001
 * @tc.desc: 10ms interval falls into the [8ms,16ms) bucket
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_RecordMoveIntervalTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputEventStatistics::RecordMoveInterval(10 * US_PER_MS);
    EXPECT_EQ(InputEventStatistics::GetMoveIntervalCount(1), 1u);
    EXPECT_EQ(SumMoveIntervalCounts(), 1u);
}

/**
 * @tc.name: InputEventStatisticsTest_RecordMoveIntervalTest_002
 * @tc.desc: interval boundaries are left-closed right-open
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_RecordMoveIntervalTest_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputEventStatistics::RecordMoveInterval(0);
    InputEventStatistics::RecordMoveInterval(8 * US_PER_MS);
    InputEventStatistics::RecordMoveInterval(16 * US_PER_MS);
    InputEventStatistics::RecordMoveInterval(33 * US_PER_MS);
    InputEventStatistics::RecordMoveInterval(50 * US_PER_MS);
    InputEventStatistics::RecordMoveInterval(-1);
    EXPECT_EQ(InputEventStatistics::GetMoveIntervalCount(0), 1u);
    EXPECT_EQ(InputEventStatistics::GetMoveIntervalCount(1), 1u);
    EXPECT_EQ(InputEventStatistics::GetMoveIntervalCount(2), 1u);
    EXPECT_EQ(InputEventStatistics::GetMoveIntervalCount(3), 1u);
    EXPECT_EQ(InputEventStatistics::GetMoveIntervalCount(4), 1u);
}

/**
 * @tc.name: InputEventStatisticsTest_FirstEventNoIntervalTest_001
 * @tc.desc: the first motion event only sets the baseline and is not counted
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_FirstEventNoIntervalTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int64_t baseTime = 1000 * US_PER_MS;
    InputEventStatistics::NotifyMotionEvent(TEST_DEVICE_ID, baseTime);
    EXPECT_EQ(SumMoveIntervalCounts(), 0u);
    InputEventStatistics::NotifyMotionEvent(TEST_DEVICE_ID, baseTime + 10 * US_PER_MS);
    EXPECT_EQ(InputEventStatistics::GetMoveIntervalCount(1), 1u);
}

/**
 * @tc.name: InputEventStatisticsTest_RecordDispatchLatencyTest_001
 * @tc.desc: 3ms latency falls into the [2ms,4ms) bucket
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_RecordDispatchLatencyTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputEventStatistics::RecordDispatchLatency(3 * US_PER_MS);
    EXPECT_EQ(InputEventStatistics::GetDispatchLatencyCount(1), 1u);
    EXPECT_EQ(SumDispatchLatencyCounts(), 1u);
}

/**
 * @tc.name: InputEventStatisticsTest_RecordDispatchLatencyTest_002
 * @tc.desc: dispatched mouse event of a tracked physical device is counted once
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_RecordDispatchLatencyTest_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto point = CreatePointerEvent(PointerEvent::SOURCE_TYPE_MOUSE, TEST_DEVICE_ID);
    ASSERT_NE(point, nullptr);
    point->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    InputEventStatistics::MarkMouseDispatchStart(TEST_DEVICE_ID);
    InputEventStatistics::RecordMouseDispatchLatency(point);
    EXPECT_EQ(SumDispatchLatencyCounts(), 1u);
    // one mark only records once
    InputEventStatistics::RecordMouseDispatchLatency(point);
    EXPECT_EQ(SumDispatchLatencyCounts(), 1u);
}

/**
 * @tc.name: InputEventStatisticsTest_InterceptedEventNoStatTest_001
 * @tc.desc: events never marked (intercepted/consumed) are not counted
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_InterceptedEventNoStatTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto point = CreatePointerEvent(PointerEvent::SOURCE_TYPE_MOUSE, TEST_DEVICE_ID);
    ASSERT_NE(point, nullptr);
    InputEventStatistics::RecordMouseDispatchLatency(point);
    InputEventStatistics::RecordMouseDispatchLatency(nullptr);
    EXPECT_EQ(SumDispatchLatencyCounts(), 0u);
}

/**
 * @tc.name: InputEventStatisticsTest_SimulateNoDispatchStatTest_001
 * @tc.desc: simulated (UDS injected) and non-mouse events are not counted into dispatch latency
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_SimulateNoDispatchStatTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto point = CreatePointerEvent(PointerEvent::SOURCE_TYPE_MOUSE, TEST_DEVICE_ID);
    ASSERT_NE(point, nullptr);
    point->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    // simulate flag (api injected)
    point->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    InputEventStatistics::MarkMouseDispatchStart(TEST_DEVICE_ID);
    InputEventStatistics::RecordMouseDispatchLatency(point);
    EXPECT_EQ(SumDispatchLatencyCounts(), 0u);
    // touchscreen source is not mouse statistics scope
    auto touch = CreatePointerEvent(PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TEST_DEVICE_ID);
    ASSERT_NE(touch, nullptr);
    InputEventStatistics::MarkMouseDispatchStart(TEST_DEVICE_ID);
    InputEventStatistics::RecordMouseDispatchLatency(touch);
    EXPECT_EQ(SumDispatchLatencyCounts(), 0u);
}

/**
 * @tc.name: InputEventStatisticsTest_NonMoveNoDispatchStatTest_001
 * @tc.desc: non-move pointer events (button/wheel) are not counted into dispatch latency
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_NonMoveNoDispatchStatTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto point = CreatePointerEvent(PointerEvent::SOURCE_TYPE_MOUSE, TEST_DEVICE_ID);
    ASSERT_NE(point, nullptr);
    point->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    InputEventStatistics::MarkMouseDispatchStart(TEST_DEVICE_ID);
    InputEventStatistics::RecordMouseDispatchLatency(point);
    EXPECT_EQ(SumDispatchLatencyCounts(), 0u);
}

#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
/**
 * @tc.name: InputEventStatisticsTest_RecordTouchpadRightClickTest_001
 * @tc.desc: touchpad right click is counted per gesture source
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_RecordTouchpadRightClickTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputEventStatistics::RecordTouchpadRightClick(true);
    InputEventStatistics::RecordTouchpadRightClick(true);
    InputEventStatistics::RecordTouchpadRightClick(false);
    EXPECT_EQ(InputEventStatistics::GetTouchpadTwoFingerTapCount(), 2u);
    EXPECT_EQ(InputEventStatistics::GetTouchpadPressRightClickAreaCount(), 1u);
}
#endif // OHOS_BUILD_ENABLE_TOUCHPAD

/**
 * @tc.name: InputEventStatisticsTest_TryReportWithDataTest_001
 * @tc.desc: TryReport with data clears all counters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_TryReportWithDataTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputEventStatistics::RecordMoveInterval(10 * US_PER_MS);
    InputEventStatistics::RecordDispatchLatency(3 * US_PER_MS);
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    InputEventStatistics::RecordTouchpadRightClick(true);
    InputEventStatistics::RecordTouchpadRightClick(false);
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
    InputEventStatistics::TryReport();
    EXPECT_EQ(SumMoveIntervalCounts(), 0u);
    EXPECT_EQ(SumDispatchLatencyCounts(), 0u);
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    EXPECT_EQ(InputEventStatistics::GetTouchpadTwoFingerTapCount(), 0u);
    EXPECT_EQ(InputEventStatistics::GetTouchpadPressRightClickAreaCount(), 0u);
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
}

/**
 * @tc.name: InputEventStatisticsTest_TryReportNoDataTest_001
 * @tc.desc: TryReport without data does nothing and does not crash
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_TryReportNoDataTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputEventStatistics::TryReport();
    EXPECT_EQ(SumMoveIntervalCounts(), 0u);
    EXPECT_EQ(SumDispatchLatencyCounts(), 0u);
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    EXPECT_EQ(InputEventStatistics::GetTouchpadTwoFingerTapCount(), 0u);
    EXPECT_EQ(InputEventStatistics::GetTouchpadPressRightClickAreaCount(), 0u);
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
}

/**
 * @tc.name: InputEventStatisticsTest_ResetClearsAllTest_001
 * @tc.desc: Reset clears all counters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_ResetClearsAllTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputEventStatistics::RecordMoveInterval(10 * US_PER_MS);
    InputEventStatistics::RecordDispatchLatency(3 * US_PER_MS);
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    InputEventStatistics::RecordTouchpadRightClick(true);
    InputEventStatistics::RecordTouchpadRightClick(false);
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
    InputEventStatistics::Reset();
    EXPECT_EQ(SumMoveIntervalCounts(), 0u);
    EXPECT_EQ(SumDispatchLatencyCounts(), 0u);
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    EXPECT_EQ(InputEventStatistics::GetTouchpadTwoFingerTapCount(), 0u);
    EXPECT_EQ(InputEventStatistics::GetTouchpadPressRightClickAreaCount(), 0u);
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
}

/**
 * @tc.name: InputEventStatisticsTest_DumpAllStatisticsTest_001
 * @tc.desc: Dump outputs mouse interval and latency statistics only
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_DumpAllStatisticsTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputEventStatistics::RecordMoveInterval(10 * US_PER_MS);
    InputEventStatistics::RecordDispatchLatency(3 * US_PER_MS);
    std::string dumpStr = DumpStatsToString(InputEventStatistics::Dump);
    EXPECT_NE(dumpStr.find("Input Event Statistics"), std::string::npos);
    EXPECT_NE(dumpStr.find("Mouse Move Interval"), std::string::npos);
    EXPECT_NE(dumpStr.find("Mouse Dispatch Latency"), std::string::npos);
    EXPECT_EQ(dumpStr.find("Touchpad Gesture Statistics"), std::string::npos);
}

#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
/**
 * @tc.name: InputEventStatisticsTest_DumpTouchpadGestureTest_001
 * @tc.desc: DumpTouchpadGesture outputs gesture statistics with trigger sources distinguished
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_DumpTouchpadGestureTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputEventStatistics::RecordTouchpadRightClick(true);
    InputEventStatistics::RecordTouchpadRightClick(false);
    std::string dumpStr = DumpStatsToString(InputEventStatistics::DumpTouchpadGesture);
    EXPECT_NE(dumpStr.find("Touchpad Gesture Statistics"), std::string::npos);
    EXPECT_NE(dumpStr.find("Right Click Count (Two Finger Tap): 1"), std::string::npos);
    EXPECT_NE(dumpStr.find("Right Click Count (Press Right Click Area): 1"), std::string::npos);
    // No touchpad device in the test environment, the libinput-sourced counter dumps 0.
    EXPECT_NE(dumpStr.find("Double Tap And Drag Count: 0"), std::string::npos);
    EXPECT_EQ(dumpStr.find("Mouse Move Interval"), std::string::npos);
    EXPECT_EQ(dumpStr.find("Mouse Dispatch Latency"), std::string::npos);
}

/**
 * @tc.name: InputEventStatisticsTest_TouchpadTapDragQueryTest_001
 * @tc.desc: tap-and-drag count query/reset without touchpad devices returns 0 and does not crash
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_TouchpadTapDragQueryTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_EQ(InputEventStatistics::GetTouchpadDoubleTapAndDragCount(), 0u);
    InputEventStatistics::ResetTouchpadDoubleTapAndDragCount();
    EXPECT_EQ(InputEventStatistics::GetTouchpadDoubleTapAndDragCount(), 0u);
}
#endif // OHOS_BUILD_ENABLE_TOUCHPAD

/**
 * @tc.name: InputEventStatisticsTest_DumpNoDataTest_001
 * @tc.desc: Dump without data outputs zero values and does not fail
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_DumpNoDataTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::string dumpStr = DumpStatsToString(InputEventStatistics::Dump);
    EXPECT_NE(dumpStr.find("Input Event Statistics"), std::string::npos);
    EXPECT_NE(dumpStr.find("Mouse Move Interval"), std::string::npos);
    EXPECT_NE(dumpStr.find("Mouse Dispatch Latency"), std::string::npos);
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    std::string gestureStr = DumpStatsToString(InputEventStatistics::DumpTouchpadGesture);
    EXPECT_NE(gestureStr.find("Touchpad Gesture Statistics"), std::string::npos);
    EXPECT_NE(gestureStr.find("Right Click Count (Two Finger Tap): 0"), std::string::npos);
    EXPECT_NE(gestureStr.find("Right Click Count (Press Right Click Area): 0"), std::string::npos);
    EXPECT_NE(gestureStr.find("Double Tap And Drag Count: 0"), std::string::npos);
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
}

/**
 * @tc.name: InputEventStatisticsTest_DeviceTrimTest_001
 * @tc.desc: statistics recording is disabled when the device is trimmed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_DeviceTrimTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputEventStatistics::SetDeviceTrimmed(true);
    EXPECT_TRUE(InputEventStatistics::IsDeviceTrimmed());
    InputEventStatistics::RecordMoveInterval(10 * US_PER_MS);
    InputEventStatistics::RecordDispatchLatency(3 * US_PER_MS);
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    InputEventStatistics::RecordTouchpadRightClick(true);
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
    EXPECT_EQ(SumMoveIntervalCounts(), 0u);
    EXPECT_EQ(SumDispatchLatencyCounts(), 0u);
#ifdef OHOS_BUILD_ENABLE_TOUCHPAD
    EXPECT_EQ(InputEventStatistics::GetTouchpadTwoFingerTapCount(), 0u);
    EXPECT_EQ(InputEventStatistics::GetTouchpadPressRightClickAreaCount(), 0u);
#endif // OHOS_BUILD_ENABLE_TOUCHPAD
}

/**
 * @tc.name: InputEventStatisticsTest_DeviceRemovedClearsDataTest_001
 * @tc.desc: device removal drops the interval baseline
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventStatisticsTest, InputEventStatisticsTest_DeviceRemovedClearsDataTest_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int64_t baseTime = 1000 * US_PER_MS;
    InputEventStatistics::NotifyMotionEvent(TEST_DEVICE_ID, baseTime);
    InputEventStatistics::NotifyMotionEvent(TEST_DEVICE_ID, baseTime + 10 * US_PER_MS);
    EXPECT_EQ(SumMoveIntervalCounts(), 1u);
    InputEventStatistics::OnDeviceRemoved(TEST_DEVICE_ID);
    // first event after removal only sets the baseline again
    InputEventStatistics::NotifyMotionEvent(TEST_DEVICE_ID, baseTime + 20 * US_PER_MS);
    EXPECT_EQ(SumMoveIntervalCounts(), 1u);
    InputEventStatistics::NotifyMotionEvent(TEST_DEVICE_ID, baseTime + 30 * US_PER_MS);
    EXPECT_EQ(SumMoveIntervalCounts(), 2u);
}
} // namespace MMI
} // namespace OHOS
