/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <string>

#include <gtest/gtest.h>

#include "input_handler_type.h"
#include "input_manager.h"
#include "input_transfer_station.h"
#include "mmi_log.h"
#include "multimodal_event_handler.h"
#include "util.h"
#include "window_utils_test.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
enum class TestScene { EXCEPTION_TEST, NORMAL_TEST, };
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "InputManagerTest" };
enum class RECV_FLAG {
    RECV_FOCUS = 0x00000000,
    RECV_MONITOR,
    RECV_INTERCEPT,
    RECV_MARK_CONSUMED,
};
constexpr int32_t DEFAULT_POINTER_ID = 0;
constexpr int32_t DEFAULT_DEVICE_ID = 0;
constexpr int32_t TIME_WAIT_FOR_EVENT = 1000;
constexpr int32_t TIME_WAIT_FOR_OP = 100;
constexpr int32_t NANOSECOND_TO_MILLISECOND = 1000000;
constexpr int32_t SEC_TO_NANOSEC = 1000000000;
constexpr int32_t INDEX_FIRST = 1;
constexpr int32_t INDEX_SECOND = 2;
constexpr int32_t INDEX_THIRD = 3;
}

class InputManagerTest : public testing::Test {
public:
    virtual void SetUp() override;
    virtual void TearDown() override;
    static void SetUpTestCase();
    static void TearDownTestCase();
    static int64_t GetNanoTime();
    static std::string GetEventDump();
    static void AddEventDump(std::string eventDump);
    static std::shared_ptr<PointerEvent> SetupPointerEvent001();
    static std::shared_ptr<PointerEvent> SetupPointerEvent002();
    static std::shared_ptr<PointerEvent> SetupPointerEvent003();
    static std::shared_ptr<PointerEvent> SetupPointerEvent005();
    static std::shared_ptr<PointerEvent> SetupPointerEvent006();
    static std::shared_ptr<PointerEvent> SetupPointerEvent007();
    static std::shared_ptr<PointerEvent> SetupPointerEvent009();
    static std::shared_ptr<PointerEvent> SetupPointerEvent010();
    static std::shared_ptr<PointerEvent> SetupPointerEvent011();
    static std::shared_ptr<PointerEvent> SetupPointerEvent012();
    static std::shared_ptr<PointerEvent> SetupPointerEvent013();
    static std::string DumpInputEvent(const std::shared_ptr<PointerEvent>& pointerEvent);
    static std::string DumpInputEvent(const std::shared_ptr<KeyEvent>& keyEvent);
    std::shared_ptr<PointerEvent> TestMarkConsumedStep1();
    std::shared_ptr<PointerEvent> TestMarkConsumedStep2();
    void TestMarkConsumedStep3(int32_t monitorId, int32_t eventId);
    void TestMarkConsumedStep4();
    void TestMarkConsumedStep5();
    void TestMarkConsumedStep6();
    static RECV_FLAG location_;
private:
    static std::mutex mutex_;
    static std::list<std::string> strEventDump_;
    static std::condition_variable conditionVariable_;
};

class InputEventConsumer : public IInputEventConsumer {
public:
    void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
    void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
    void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override {};
};

class InputEventCallback : public IInputEventConsumer {
public:
    virtual void OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const override {};
    virtual void OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const override;
    virtual void OnInputEvent(std::shared_ptr<AxisEvent> axisEvent) const override {};
};

RECV_FLAG InputManagerTest::location_ = RECV_FLAG::RECV_FOCUS;
std::mutex InputManagerTest::mutex_;
std::list<std::string> InputManagerTest::strEventDump_;
std::condition_variable InputManagerTest::conditionVariable_;

void InputEventConsumer::OnInputEvent(std::shared_ptr<KeyEvent> keyEvent) const
{
    CALL_LOG_ENTER;
    if (InputManagerTest::location_ == RECV_FLAG::RECV_FOCUS ||
        InputManagerTest::location_ == RECV_FLAG::RECV_MARK_CONSUMED) {
        keyEvent->MarkProcessed();
        ASSERT_TRUE(keyEvent != nullptr);
        InputManagerTest::AddEventDump(InputManagerTest::DumpInputEvent(keyEvent));
    }
}

void InputEventConsumer::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    CALL_LOG_ENTER;
    if (InputManagerTest::location_ == RECV_FLAG::RECV_FOCUS ||
        InputManagerTest::location_ == RECV_FLAG::RECV_MARK_CONSUMED) {
        pointerEvent->MarkProcessed();
        ASSERT_TRUE(pointerEvent != nullptr);
        InputManagerTest::AddEventDump(InputManagerTest::DumpInputEvent(pointerEvent));
    }
}

void InputEventCallback::OnInputEvent(std::shared_ptr<PointerEvent> pointerEvent) const
{
    CALL_LOG_ENTER;
    if (InputManagerTest::location_ != RECV_FLAG::RECV_MARK_CONSUMED) {
        InputManagerTest::location_ = RECV_FLAG::RECV_MONITOR;
        ASSERT_TRUE(pointerEvent != nullptr);
        InputManagerTest::AddEventDump(InputManagerTest::DumpInputEvent(pointerEvent));
    }
}

template<typename sharedType>
static std::shared_ptr<sharedType> GetPtr()
{
    return std::make_shared<sharedType>();
}

void InputManagerTest::AddEventDump(std::string eventDump)
{
    CALL_LOG_ENTER;
    std::lock_guard<std::mutex> lockGuard(mutex_);
    if (eventDump.empty()) {
        strEventDump_.clear();
        return;
    }
    strEventDump_.push_back(eventDump);
    MMI_HILOGD("Setting the Dump event, strEventDump_ : %{public}s", eventDump.c_str());
    conditionVariable_.notify_one();
}

std::string InputManagerTest::GetEventDump()
{
    CALL_LOG_ENTER;
    std::unique_lock<std::mutex> uniqueLock(mutex_);
    std::string str = "";
    if (strEventDump_.empty()) {
        MMI_HILOGD("Waiting for an event to fire");
        if (conditionVariable_.wait_for(uniqueLock,
            std::chrono::milliseconds(TIME_WAIT_FOR_EVENT)) == std::cv_status::timeout) {
            MMI_HILOGD("Timeout");
            return str;
        }
    }
    str = strEventDump_.front();
    strEventDump_.pop_front();
    return str;
}

void InputManagerTest::SetUpTestCase()
{
    CALL_LOG_ENTER;
    ASSERT_TRUE(WindowUtilsTest::GetInstance()->DrawTestWindow());
    sptr<Rosen::Window> window_ = WindowUtilsTest::GetInstance()->GetWindow();
    ASSERT_TRUE(window_ != nullptr);
    auto listener_ = GetPtr<InputEventConsumer>();
    ASSERT_TRUE(listener_ != nullptr);
    MMI::InputManager::GetInstance()->SetWindowInputEventConsumer(listener_);
    Rosen::InputTransferStation::GetInstance().SetInputListener(window_->GetWindowId(), listener_);
    Rosen::InputTransferStation::GetInstance().AddInputWindow(window_);
}

void InputManagerTest::TearDownTestCase()
{
}

void InputManagerTest::SetUp()
{
    location_ = RECV_FLAG::RECV_FOCUS;
}

void InputManagerTest::TearDown()
{
    AddEventDump("");
}

int64_t InputManagerTest::GetNanoTime()
{
    struct timespec time = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &time);
    return static_cast<int64_t>(time.tv_sec) * SEC_TO_NANOSEC + time.tv_nsec;
}

std::string InputManagerTest::DumpInputEvent(const std::shared_ptr<PointerEvent>& pointerEvent)
{
    const int precision = 2;
    std::ostringstream ostream;
    std::vector<int32_t> pointerIds { pointerEvent->GetPointersIdList() };
    ostream << "ClientMsgHandler: in OnPointerEvent"
         << ", EventType:" << pointerEvent->GetEventType()
         << ",ActionTime:" << pointerEvent->GetActionTime()
         << ",Action:" << pointerEvent->GetAction()
         << ",ActionStartTime:" << pointerEvent->GetActionStartTime()
         << ",Flag:" << pointerEvent->GetFlag()
         << ",PointerAction:" << pointerEvent->DumpPointerAction()
         << ",SourceType:" << pointerEvent->DumpSourceType()
         << ",ButtonId:" << pointerEvent->GetButtonId()
         << ",VerticalAxisValue:" << std::fixed << std::setprecision(precision)
         << pointerEvent->GetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_VERTICAL)
         << ",HorizontalAxisValue:" << std::fixed << std::setprecision(precision)
         << pointerEvent->GetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_HORIZONTAL);
    for (const auto& pointerId : pointerIds) {
        PointerEvent::PointerItem item;
        if (!pointerEvent->GetPointerItem(pointerId, item)) {
            MMI_HILOGE("Invalid pointer: %{public}d.", pointerId);
            return ostream.str();
        }
        ostream << ",ClientMsgHandler: in OnPointerEvent, DownTime:" << item.GetDownTime()
            << ",IsPressed:" << std::boolalpha << item.IsPressed()
            << ",GlobalX:-\\{0,1\\}[[:digit:]]\\{1,\\},GlobalY:-\\{0,1\\}[[:digit:]]\\{1,\\}"
            << ",LocalX:-\\{0,1\\}[[:digit:]]\\{1,\\},LocalY:-\\{0,1\\}[[:digit:]]\\{1,\\}"
            << ",Width:" << item.GetWidth() << ",Height:" << item.GetHeight()
            << ",TiltX:" << std::fixed << std::setprecision(precision) << item.GetTiltX()
            << ",TiltY:" << std::fixed << std::setprecision(precision) << item.GetTiltY()
            << ",ToolGlobalX:" << item.GetToolGlobalX() << ",ToolGlobalY:" << item.GetToolGlobalY()
            << ",ToolLocalX:" << item.GetToolLocalX() << ",ToolLocalY:" << item.GetToolLocalY()
            << ",ToolWidth:" << item.GetToolWidth() << ",ToolHeight:" << item.GetToolHeight()
            << ",Pressure:" << item.GetPressure() << ",ToolType:" << item.GetToolType()
            << ",LongAxis:" << item.GetLongAxis() << ",ShortAxis:" << item.GetShortAxis();
    }
    return ostream.str();
}

std::string InputManagerTest::DumpInputEvent(const std::shared_ptr<KeyEvent>& keyEvent)
{
    std::ostringstream strm;
    strm << "InputManagerTest: in OnKeyEvent"
         << ", KeyCode:" << keyEvent->GetKeyCode()
         << ", ActionTime:" << keyEvent->GetActionTime()
         << ", Action:" << keyEvent->GetAction()
         << ", ActionStartTime:" << keyEvent->GetActionStartTime()
         << ", EventType:" << keyEvent->GetEventType()
         << ", KeyAction:" << keyEvent->GetKeyAction();
    return strm.str();
}

template<typename EventT>
static void TestSimulateInputEvent(EventT& event, const TestScene& testScene = TestScene::NORMAL_TEST)
{
    CALL_LOG_ENTER;
    std::string sPointeE = InputManagerTest::DumpInputEvent(event);
    InputManager::GetInstance()->SimulateInputEvent(event);
    std::string sPointerEs = InputManagerTest::GetEventDump();
    MMI_HILOGD("sPointerEs:%{public}s", sPointerEs.c_str());
    if (testScene == TestScene::NORMAL_TEST) {
        EXPECT_STREQ(sPointeE.c_str(), sPointerEs.c_str());
    } else {
        EXPECT_STRNE(sPointeE.c_str(), sPointerEs.c_str());
    }
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent001()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);   // test code，set the PointerId = 0
    item.SetGlobalX(523);   // test code，set the GlobalX = 523
    item.SetGlobalY(723);   // test code，set the GlobalY = 723
    item.SetPressure(5);    // test code，set the Pressure = 5
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    item.SetPointerId(1);   // test code，set the PointerId = 1
    item.SetGlobalX(610);   // test code，set the GlobalX = 610
    item.SetGlobalY(910);   // test code，set the GlobalY = 910
    item.SetPressure(7);    // test code，set the Pressure = 7
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetPointerId(1);  // test code，set the PointerId = 1
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent002()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);   // test code，set the PointerId = 0
    item.SetGlobalX(523);   // test code，set the GlobalX = 523
    item.SetGlobalY(723);   // test code，set the GlobalY = 723
    item.SetPressure(5);    // test code，set the Pressure = 5
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    item.SetPointerId(1);   // test code，set the PointerId = 1
    item.SetGlobalX(600);   // test code，set the GlobalX = 600
    item.SetGlobalY(610);   // test code，set the GlobalY = 610
    item.SetPressure(7);    // test code，set the Pressure = 7
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetPointerId(1);  // test code，set the PointerId = 1
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent003()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);   // test code，set the PointerId = 0
    item.SetGlobalX(523);   // test code，set the GlobalX = 523
    item.SetGlobalY(723);   // test code，set the GlobalY = 723
    item.SetPressure(5);    // test code，set the Pressure = 5
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    item.SetPointerId(1);   // test code，set the PointerId = 1
    item.SetGlobalX(623);   // test code，set the GlobalX = 623
    item.SetGlobalY(823);   // test code，set the GlobalY = 823
    item.SetPressure(0);    // test code，set the Pressure = 0
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetPointerId(1);  // test code，set the PointerId = 1
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent005()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    int64_t downTime = GetNanoTime()/NANOSECOND_TO_MILLISECOND;
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    pointerEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDownTime(downTime);
    item.SetPressed(true);

    item.SetGlobalX(50);
    item.SetGlobalY(50);
    item.SetLocalX(70);
    item.SetLocalY(70);

    item.SetWidth(0);
    item.SetHeight(0);
    item.SetPressure(0);
    item.SetDeviceId(0);
    pointerEvent->AddPointerItem(item);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent006()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetPointerId(1);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDownTime(0);
    item.SetPressed(false);

    item.SetGlobalX(50);
    item.SetGlobalY(50);
    item.SetLocalX(70);
    item.SetLocalY(70);

    item.SetWidth(0);
    item.SetHeight(0);
    item.SetPressure(0);
    item.SetDeviceId(0);
    pointerEvent->AddPointerItem(item);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent007()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    int64_t downTime = GetNanoTime()/NANOSECOND_TO_MILLISECOND;
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    pointerEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDownTime(downTime);
    item.SetPressed(false);

    item.SetGlobalX(50);
    item.SetGlobalY(50);
    item.SetLocalX(70);
    item.SetLocalY(70);

    item.SetWidth(0);
    item.SetHeight(0);
    item.SetPressure(0);
    item.SetDeviceId(0);
    pointerEvent->AddPointerItem(item);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent009()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, -1.0000);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDownTime(0);
    item.SetPressed(false);

    item.SetGlobalX(50);
    item.SetGlobalY(50);
    item.SetLocalX(70);
    item.SetLocalY(70);

    item.SetWidth(0);
    item.SetHeight(0);
    item.SetPressure(0);
    item.SetDeviceId(0);
    pointerEvent->AddPointerItem(item);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent010()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, 30.0);
    pointerEvent->SetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_HORIZONTAL, 40.0);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDownTime(0);
    item.SetPressed(false);

    item.SetGlobalX(200);
    item.SetGlobalY(200);
    item.SetLocalX(300);
    item.SetLocalY(300);

    item.SetWidth(0);
    item.SetHeight(0);
    item.SetPressure(0);
    item.SetDeviceId(0);
    pointerEvent->AddPointerItem(item);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent011()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetPointerId(1);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDownTime(0);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetLocalX(323);
    item.SetLocalY(453);
    item.SetWidth(0);
    item.SetHeight(0);
    item.SetTiltX(2.12);
    item.SetTiltY(5.43);
    item.SetPressure(0.15);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);

    item.SetPointerId(1);
    item.SetDownTime(0);
    item.SetGlobalX(50);
    item.SetGlobalY(50);
    item.SetLocalX(70);
    item.SetLocalY(70);
    item.SetWidth(0);
    item.SetHeight(0);
    item.SetTiltX(12.22);
    item.SetTiltY(15.33);
    item.SetPressure(0.45);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent012()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetPointerId(1);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDownTime(0);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetLocalX(323);
    item.SetLocalY(453);
    item.SetWidth(0);
    item.SetHeight(0);
    item.SetTiltX(2.12);
    item.SetTiltY(5.43);
    item.SetPressure(0.15);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);

    item.SetPointerId(1);
    item.SetDownTime(0);
    item.SetGlobalX(50);
    item.SetGlobalY(50);
    item.SetLocalX(70);
    item.SetLocalY(70);
    item.SetWidth(0);
    item.SetHeight(0);
    item.SetTiltX(12.22);
    item.SetTiltY(15.33);
    item.SetPressure(0.45);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::SetupPointerEvent013()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetPointerId(1);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDownTime(0);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetLocalX(323);
    item.SetLocalY(453);
    item.SetWidth(0);
    item.SetHeight(0);
    item.SetTiltX(2.12);
    item.SetTiltY(5.43);
    item.SetPressure(0.15);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);

    item.SetPointerId(1);
    item.SetDownTime(0);
    item.SetGlobalX(50);
    item.SetGlobalY(50);
    item.SetLocalX(70);
    item.SetLocalY(70);
    item.SetWidth(0);
    item.SetHeight(0);
    item.SetTiltX(12.22);
    item.SetTiltY(15.33);
    item.SetPressure(0.45);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::TestMarkConsumedStep1()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);   // test code，set the PointerId = 0
    item.SetGlobalX(523);   // test code，set the GlobalX = 523
    item.SetGlobalY(723);   // test code，set the GlobalY = 723
    item.SetPressure(5);    // test code，set the Pressure = 5
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    pointerEvent->SetId(std::numeric_limits<int32_t>::max() - INDEX_THIRD);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetPointerId(0);  // test code，set the PointerId = 1
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);

    TestSimulateInputEvent(pointerEvent);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    return pointerEvent;
}

std::shared_ptr<PointerEvent> InputManagerTest::TestMarkConsumedStep2()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);   // test code，set the PointerId = 0
    item.SetGlobalX(623);  // test code，set the GlobalX = 623
    item.SetGlobalY(723);   // test code，set the GlobalY = 723
    item.SetPressure(5);    // test code，set the Pressure = 5
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    pointerEvent->SetId(std::numeric_limits<int32_t>::max() - INDEX_SECOND);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetPointerId(0);  // test code，set the PointerId = 1
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);

    TestSimulateInputEvent(pointerEvent);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    return pointerEvent;
}

void InputManagerTest::TestMarkConsumedStep3(int32_t monitorId, int32_t eventId)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    location_ = RECV_FLAG::RECV_MARK_CONSUMED;
    InputManager::GetInstance()->MarkConsumed(monitorId, eventId);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
}

void InputManagerTest::TestMarkConsumedStep4()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPV(pointerEvent);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);   // test code，set the PointerId = 0
    item.SetGlobalX(523);  // test code，set the GlobalX = 523
    item.SetGlobalY(723);   // test code，set the GlobalY = 723
    item.SetPressure(5);    // test code，set the Pressure = 5
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    pointerEvent->SetId(std::numeric_limits<int32_t>::max() - INDEX_FIRST);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetPointerId(0);  // test code，set the PointerId = 1
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);

    TestSimulateInputEvent(pointerEvent, TestScene::EXCEPTION_TEST);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
}

void InputManagerTest::TestMarkConsumedStep5()
{
    auto pointerEvent = PointerEvent::Create();
    CHKPV(pointerEvent);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);   // test code，set the PointerId = 0
    item.SetGlobalX(523);  // test code，set the GlobalX = 523
    item.SetGlobalY(723);   // test code，set the GlobalY = 723
    item.SetPressure(5);    // test code，set the Pressure = 5
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    pointerEvent->SetId(std::numeric_limits<int32_t>::max());
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetPointerId(0);  // test code，set the PointerId = 0
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);

    TestSimulateInputEvent(pointerEvent, TestScene::EXCEPTION_TEST);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
}

void InputManagerTest::TestMarkConsumedStep6()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    auto pointerEvent = PointerEvent::Create();
    CHKPV(pointerEvent);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);   // test code，set the PointerId = 0
    item.SetGlobalX(523);   // test code，set the GlobalX = 523
    item.SetGlobalY(723);   // test code，set the GlobalY = 723
    item.SetPressure(5);    // test code，set the Pressure = 5
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);

    pointerEvent->SetId(std::numeric_limits<int32_t>::max());
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetPointerId(0);  // test code，set the PointerId = 0
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);

    TestSimulateInputEvent(pointerEvent);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
}

/**
 * @tc.name: MultimodalEventHandler_SimulateKeyEvent_001
 * @tc.desc: Verify simulate the back key is long pressed and lifted
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_SimulateKeyEvent_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    int64_t downTime = GetNanoTime()/NANOSECOND_TO_MILLISECOND;
    std::shared_ptr<KeyEvent> injectDownEvent = KeyEvent::Create();
    ASSERT_TRUE(injectDownEvent != nullptr);
    KeyEvent::KeyItem kitDown;
    kitDown.SetKeyCode(KeyEvent::KEYCODE_BACK);
    kitDown.SetPressed(true);
    kitDown.SetDownTime(downTime);
    injectDownEvent->SetKeyCode(KeyEvent::KEYCODE_BACK);
    injectDownEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    injectDownEvent->AddPressedKeyItems(kitDown);
    TestSimulateInputEvent(injectDownEvent);

    std::shared_ptr<KeyEvent> injectUpEvent = KeyEvent::Create();
    ASSERT_TRUE(injectUpEvent != nullptr);
    downTime = GetNanoTime()/NANOSECOND_TO_MILLISECOND;
    KeyEvent::KeyItem kitUp;
    kitUp.SetKeyCode(KeyEvent::KEYCODE_BACK);
    kitUp.SetPressed(false);
    kitUp.SetDownTime(downTime);
    injectUpEvent->SetKeyCode(KeyEvent::KEYCODE_BACK);
    injectUpEvent->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    injectUpEvent->RemoveReleasedKeyItems(kitUp);
    TestSimulateInputEvent(injectUpEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulateKeyEvent_002
 * @tc.desc: Verify simulate the back home is pressed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_SimulateKeyEvent_002, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<KeyEvent> injectDownEvent = KeyEvent::Create();
    ASSERT_TRUE(injectDownEvent != nullptr);
    int64_t downTime = -1;
    KeyEvent::KeyItem kitDown;
    kitDown.SetKeyCode(KeyEvent::KEYCODE_HOME);
    kitDown.SetPressed(true);
    kitDown.SetDownTime(downTime);
    injectDownEvent->SetKeyCode(KeyEvent::KEYCODE_HOME);
    injectDownEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    injectDownEvent->AddPressedKeyItems(kitDown);
    TestSimulateInputEvent(injectDownEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulateKeyEvent_003
 * @tc.desc: Verify simulate the back key is pressed and lifted
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_SimulateKeyEvent_003, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<KeyEvent> injectDownEvent = KeyEvent::Create();
    ASSERT_TRUE(injectDownEvent != nullptr);
    int64_t downTime = 0;
    KeyEvent::KeyItem kitDown;
    kitDown.SetKeyCode(KeyEvent::KEYCODE_BACK);
    kitDown.SetPressed(true);
    kitDown.SetDownTime(downTime);
    injectDownEvent->SetKeyCode(KeyEvent::KEYCODE_BACK);
    injectDownEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    injectDownEvent->AddPressedKeyItems(kitDown);
    TestSimulateInputEvent(injectDownEvent);

    std::shared_ptr<KeyEvent> injectUpEvent = KeyEvent::Create();
    ASSERT_TRUE(injectUpEvent != nullptr);
    KeyEvent::KeyItem kitUp;
    kitUp.SetKeyCode(KeyEvent::KEYCODE_BACK);
    kitUp.SetPressed(false);
    kitUp.SetDownTime(downTime);
    injectUpEvent->SetKeyCode(KeyEvent::KEYCODE_BACK);
    injectUpEvent->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    injectUpEvent->RemoveReleasedKeyItems(kitUp);
    TestSimulateInputEvent(injectUpEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulateKeyEvent_004
 * @tc.desc: Verify simulate key exception event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_SimulateKeyEvent_004, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<KeyEvent> injectDownEvent = KeyEvent::Create();
    ASSERT_TRUE(injectDownEvent != nullptr);
    int64_t downTime = GetNanoTime()/NANOSECOND_TO_MILLISECOND;
    KeyEvent::KeyItem kitDown;
    kitDown.SetKeyCode(KeyEvent::KEYCODE_UNKNOWN);
    kitDown.SetPressed(true);
    kitDown.SetDownTime(downTime);
    injectDownEvent->SetKeyCode(KeyEvent::KEYCODE_UNKNOWN);
    injectDownEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    injectDownEvent->AddPressedKeyItems(kitDown);
    TestSimulateInputEvent(injectDownEvent, TestScene::EXCEPTION_TEST);
}

/**
 * @tc.name: MultimodalEventHandler_SimulateKeyEvent_005
 * @tc.desc: Verify simulate the fn key is long pressed and lifted
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_SimulateKeyEvent_005, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<KeyEvent> injectDownEvent = KeyEvent::Create();
    ASSERT_TRUE(injectDownEvent != nullptr);
    int64_t downTime = GetNanoTime()/NANOSECOND_TO_MILLISECOND;
    KeyEvent::KeyItem kitDown;
    kitDown.SetKeyCode(KeyEvent::KEYCODE_FN);
    kitDown.SetPressed(true);
    kitDown.SetDownTime(downTime);
    injectDownEvent->SetKeyCode(KeyEvent::KEYCODE_FN);
    injectDownEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    injectDownEvent->AddPressedKeyItems(kitDown);
    TestSimulateInputEvent(injectDownEvent);


    std::shared_ptr<KeyEvent> injectUpEvent = KeyEvent::Create();
    ASSERT_TRUE(injectUpEvent != nullptr);
    downTime = GetNanoTime()/NANOSECOND_TO_MILLISECOND;
    KeyEvent::KeyItem kitUp;
    kitUp.SetKeyCode(KeyEvent::KEYCODE_FN);
    kitUp.SetPressed(false);
    kitUp.SetDownTime(downTime);
    injectUpEvent->SetKeyCode(KeyEvent::KEYCODE_FN);
    injectUpEvent->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    injectUpEvent->RemoveReleasedKeyItems(kitUp);
    TestSimulateInputEvent(injectUpEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_001
 * @tc.desc: Verify simulate screen down event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_SimulatePoniterEvent_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent001() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_002
 * @tc.desc: Verify simulate screen move event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_SimulatePoniterEvent_002, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent002() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_003
 * @tc.desc: Verify simulate screen up event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_003, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent003() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_004
 * @tc.desc: Verify simulate screen exception event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_004, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerId(-1);
    TestSimulateInputEvent(pointerEvent, TestScene::EXCEPTION_TEST);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_005
 * @tc.desc: Verify simulate mouse down event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_005, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent005() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_006
 * @tc.desc: Verify simulate mouse move event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_006, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent006() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_007
 * @tc.desc: Verify simulate mouse up event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_007, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent007() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_008
 * @tc.desc: Verify simulate mouse exception event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_008, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerId(-1);
    TestSimulateInputEvent(pointerEvent, TestScene::EXCEPTION_TEST);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_009
 * @tc.desc: Verify simulate mouse VERTICAL axis event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_009, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent009() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_010
 * @tc.desc: Verify simulate mouse VERTICAL HORIZONTAL axis event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_010, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent010() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_011
 * @tc.desc: Verify simulate mouse AXIS_BEGIN event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_011, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_BEGIN);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, 30.0);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDownTime(0);
    item.SetPressed(false);

    item.SetGlobalX(200);
    item.SetGlobalY(200);
    item.SetLocalX(300);
    item.SetLocalY(300);

    item.SetWidth(0);
    item.SetHeight(0);
    item.SetPressure(0);
    item.SetDeviceId(0);
    pointerEvent->AddPointerItem(item);

    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_012
 * @tc.desc: Verify simulate mouse AXIS_UPDATE event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_012, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, 30.0);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDownTime(0);
    item.SetPressed(false);

    item.SetGlobalX(200);
    item.SetGlobalY(200);
    item.SetLocalX(300);
    item.SetLocalY(300);

    item.SetWidth(0);
    item.SetHeight(0);
    item.SetPressure(0);
    item.SetDeviceId(0);
    pointerEvent->AddPointerItem(item);

    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePoniterEvent_013
 * @tc.desc: Verify simulate mouse AXIS_END event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePoniterEvent_013, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_END);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, 30.0);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDownTime(0);
    item.SetPressed(false);

    item.SetGlobalX(200);
    item.SetGlobalY(200);
    item.SetLocalX(300);
    item.SetLocalY(300);

    item.SetWidth(0);
    item.SetHeight(0);
    item.SetPressure(0);
    item.SetDeviceId(0);
    pointerEvent->AddPointerItem(item);

    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePencil2Event_001
 * @tc.desc: Verify simulate pencil2 down event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePencil2Event_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent011() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePencil2Event_002
 * @tc.desc: Verify simulate pencil2 move event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePencil2Event_002, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent012() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: MultimodalEventHandler_SimulatePencil2Event_003
 * @tc.desc: Verify simulate pencil2 up event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, MultimodalEventHandler_SimulatePencil2Event_003, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::shared_ptr<PointerEvent> pointerEvent { SetupPointerEvent013() };
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: InputManager_Pencil2InputEvent_004
 * @tc.desc: Verify simulate exception event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManager_Pencil2InputEvent_004, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerId(-1);
    TestSimulateInputEvent(pointerEvent, TestScene::EXCEPTION_TEST);
}

/**
 * @tc.name: InputManager_NotResponse_001
 * @tc.desc: detection of not response
 * @tc.type: FUNC
 * @tc.require:AR000GJG6G
 */
HWTEST_F(InputManagerTest, InputManager_NotResponse_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);

    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetPressure(5);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerId(0);
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: InputManager_NotResponse_002
 * @tc.desc: detection of not response
 * @tc.type: FUNC
 * @tc.require:SR000GGN6G
 */
HWTEST_F(InputManagerTest, InputManager_NotResponse_002, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);

    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetPressure(5);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerId(0);
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
}

/**
 * @tc.name: InputManagerTest_SubscribeKeyEvent_001
 * @tc.desc: Verify invalid parameter.
 * @tc.type: FUNC
 * @tc.require:SR000GGQL4  AR000GJNGN
 * @tc.author: yangguang
 */
HWTEST_F(InputManagerTest, InputManagerTest_SubscribeKeyEvent_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::set<int32_t> preKeys;
    std::shared_ptr<KeyOption> keyOption = std::make_shared<KeyOption>();
    keyOption->SetPreKeys(preKeys);
    keyOption->SetFinalKey(KeyEvent::KEYCODE_VOLUME_MUTE);
    keyOption->SetFinalKeyDown(true);
    keyOption->SetFinalKeyDownDuration(0);
    int32_t response = -1;
    response = InputManager::GetInstance()->SubscribeKeyEvent(keyOption, nullptr);
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    EXPECT_TRUE(response < 0);
#else
   EXPECT_TRUE(response == ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    InputManager::GetInstance()->UnsubscribeKeyEvent(response);
}

/**
 * @tc.name: InputManagerTest_SubscribeKeyEvent_02
 * @tc.desc: Verify subscribe power key event.
 * @tc.type: FUNC
 * @tc.require:SR000GGQL4  AR000GJNGN
 * @tc.author: zhaoxueyuan
 */
HWTEST_F(InputManagerTest, InputManagerTest_SubscribeKeyEvent_02, TestSize.Level1)
{
    CALL_LOG_ENTER;
    ASSERT_TRUE(MMIEventHdl.InitClient());
    // 电源键长按按下订阅
    std::set<int32_t> preKeys;
    std::shared_ptr<KeyOption> keyOption = std::make_shared<KeyOption>();
    keyOption->SetPreKeys(preKeys);
    keyOption->SetFinalKey(KeyEvent::KEYCODE_POWER);
    keyOption->SetFinalKeyDown(true);
    keyOption->SetFinalKeyDownDuration(2000);
    int32_t subscribeId1 = -1;
    subscribeId1 = InputManager::GetInstance()->SubscribeKeyEvent(keyOption,
        [](std::shared_ptr<KeyEvent> keyEvent) {
        MMI_HILOGD("KeyEvent:%{public}d,KeyCode:%{public}d,ActionTime:%{public}" PRId64 ","
                   "ActionStartTime:%{public}" PRId64 ",Action:%{public}d,KeyAction:%{public}d,"
                   "EventType:%{public}d,flag:%{public}u",
                   keyEvent->GetId(), keyEvent->GetKeyCode(), keyEvent->GetActionTime(),
                   keyEvent->GetActionStartTime(), keyEvent->GetAction(), keyEvent->GetKeyAction(),
                   keyEvent->GetEventType(), keyEvent->GetFlag());
        MMI_HILOGD("subscribe key event KEYCODE_POWER down trigger callback");
    });
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    EXPECT_TRUE(subscribeId1 >= 0);
#else
   EXPECT_TRUE(subscribeId1 == ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    // 电源键抬起订阅
    std::shared_ptr<KeyOption> keyOption2 = std::make_shared<KeyOption>();
    keyOption2->SetPreKeys(preKeys);
    keyOption2->SetFinalKey(KeyEvent::KEYCODE_POWER);
    keyOption2->SetFinalKeyDown(false);
    keyOption2->SetFinalKeyDownDuration(0);
    int32_t subscribeId2 = -1;
    subscribeId2 = InputManager::GetInstance()->SubscribeKeyEvent(keyOption2,
        [](std::shared_ptr<KeyEvent> keyEvent) {
        MMI_HILOGD("KeyEvent:%{public}d,KeyCode:%{public}d,ActionTime:%{public}" PRId64 ","
                   "ActionStartTime:%{public}" PRId64 ",Action:%{public}d,KeyAction:%{public}d,"
                   "EventType:%{public}d,flag:%{public}u",
                   keyEvent->GetId(), keyEvent->GetKeyCode(), keyEvent->GetActionTime(),
                   keyEvent->GetActionStartTime(), keyEvent->GetAction(), keyEvent->GetKeyAction(),
                   keyEvent->GetEventType(), keyEvent->GetFlag());
        MMI_HILOGD("subscribe key event KEYCODE_POWER up trigger callback");
    });
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    EXPECT_TRUE(subscribeId2 >= 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    InputManager::GetInstance()->UnsubscribeKeyEvent(subscribeId1);
    InputManager::GetInstance()->UnsubscribeKeyEvent(subscribeId2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

/**
 * @tc.name: TestGetKeystrokeAbility_001
 * @tc.desc: Verify SupportKeys
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, TestGetKeystrokeAbility_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    std::vector<int32_t> keyCodes = {17, 22, 2055};
    InputManager::GetInstance()->SupportKeys(0, keyCodes, [](std::vector<bool> keystrokeAbility) {
        MMI_HILOGD("TestGetKeystrokeAbility_001 callback ok");
    });
    MMI_HILOGD("stop TestGetKeystrokeAbility_001");
}

/**
 * @tc.name: TestInputEventInterceptor_001
 * @tc.desc: Verify mouse down event interceptor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, TestInputEventInterceptor_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(DEFAULT_POINTER_ID);
    item.SetDownTime(10010);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetPointerId(DEFAULT_POINTER_ID);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    auto interceptor = GetPtr<InputEventCallback>();
    int32_t interceptorId { InputManager::GetInstance()->AddInterceptor(interceptor) };
    EXPECT_TRUE(IsValidHandlerId(interceptorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(interceptorId)) {
        InputManager::GetInstance()->RemoveInterceptor(interceptorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: TestInputEventInterceptor_002
 * @tc.desc: Verify mouse move event interceptor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, TestInputEventInterceptor_002, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(DEFAULT_POINTER_ID);
    item.SetDownTime(10010);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetPointerId(DEFAULT_POINTER_ID);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    auto interceptor = GetPtr<InputEventCallback>();
    int32_t interceptorId { InputManager::GetInstance()->AddInterceptor(interceptor) };
    EXPECT_TRUE(IsValidHandlerId(interceptorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(interceptorId)) {
        InputManager::GetInstance()->RemoveInterceptor(interceptorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: TestInputEventInterceptor_003
 * @tc.desc: Verify mouse up event interceptor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, TestInputEventInterceptor_003, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(DEFAULT_POINTER_ID);
    item.SetDownTime(10010);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetPointerId(DEFAULT_POINTER_ID);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    auto interceptor = GetPtr<InputEventCallback>();
    int32_t interceptorId { InputManager::GetInstance()->AddInterceptor(interceptor) };
    EXPECT_TRUE(IsValidHandlerId(interceptorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(interceptorId)) {
        InputManager::GetInstance()->RemoveInterceptor(interceptorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: TestInputEventInterceptor_004
 * @tc.desc: Verify multiple interceptor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, TestInputEventInterceptor_004, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDownTime(10010);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHPAD);

    const std::vector<int32_t>::size_type N_TEST_CASES { 3 };
    std::vector<int32_t> ids(N_TEST_CASES);
    auto interceptor = GetPtr<InputEventCallback>();

    for (std::vector<int32_t>::size_type i = 0; i < N_TEST_CASES; ++i) {
        ids[i] = InputManager::GetInstance()->AddInterceptor(interceptor);
        EXPECT_TRUE(IsValidHandlerId(ids[i]));
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }

    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);

    for (const auto& id : ids) {
        std::string sPointerEs = InputManagerTest::GetEventDump();
        MMI_HILOGD("sPointerEs : %{public}s", sPointerEs.c_str());
        ASSERT_TRUE(!sPointerEs.empty());
        if (IsValidHandlerId(id)) {
            InputManager::GetInstance()->RemoveInterceptor(id);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: TestInputEventInterceptor_005
 * @tc.desc: Verify mouse button interceptor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, TestInputEventInterceptor_005, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    pointerEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
    pointerEvent->SetPointerId(DEFAULT_POINTER_ID);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);
    PointerEvent::PointerItem item;
    item.SetPointerId(DEFAULT_POINTER_ID);
    item.SetDownTime(GetNanoTime() / NANOSECOND_TO_MILLISECOND);
    item.SetPressed(true);
    item.SetGlobalX(200);
    item.SetGlobalY(300);
    pointerEvent->AddPointerItem(item);

    auto interceptor = GetPtr<InputEventCallback>();
    int32_t interceptorId { InputManager::GetInstance()->AddInterceptor(interceptor) };
    EXPECT_TRUE(IsValidHandlerId(interceptorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(interceptorId)) {
        InputManager::GetInstance()->RemoveInterceptor(interceptorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: TestInputEventInterceptor_006
 * @tc.desc: Verify touchscreen interceptor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, TestInputEventInterceptor_006, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);   // test code，set the PointerId = 0
    item.SetGlobalX(523);   // test code，set the GlobalX = 523
    item.SetGlobalY(723);   // test code，set the GlobalY = 723
    item.SetPressure(5);    // test code，set the Pressure = 5
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);
    item.SetPointerId(1);   // test code，set the PointerId = 1
    item.SetGlobalX(710);   // test code，set the GlobalX = 710
    item.SetGlobalY(910);   // test code，set the GlobalY = 910
    item.SetPressure(7);    // test code，set the Pressure = 7
    item.SetDeviceId(1);    // test code，set the DeviceId = 1
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetPointerId(1);  // test code，set the PointerId = 1
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);

    auto interceptor = GetPtr<InputEventCallback>();
    int32_t interceptorId { InputManager::GetInstance()->AddInterceptor(interceptor) };
    EXPECT_TRUE(IsValidHandlerId(interceptorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(interceptorId)) {
        InputManager::GetInstance()->RemoveInterceptor(interceptorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_OnAddScreenMonitor_001
 * @tc.desc: Verify touchscreen down event monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_OnAddScreenMonitor_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = SetupPointerEvent001();
    ASSERT_TRUE(pointerEvent != nullptr);

    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
    EXPECT_TRUE(IsValidHandlerId(monitorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_OnAddScreenMonitor_002
 * @tc.desc: Verify touchscreen move event multiple monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_OnAddScreenMonitor_002, TestSize.Level1)
{
    CALL_LOG_ENTER;
    const std::vector<int32_t>::size_type N_TEST_CASES { 3 };
    std::vector<int32_t> ids(N_TEST_CASES);
    std::vector<std::shared_ptr<InputEventCallback>> cbs(N_TEST_CASES);

    for (std::vector<int32_t>::size_type i = 0; i < N_TEST_CASES; i++) {
        cbs[i] = GetPtr<InputEventCallback>();
        ASSERT_TRUE(cbs[i] != nullptr);
        ids[i] = InputManager::GetInstance()->AddMonitor(cbs[i]);
        EXPECT_TRUE(IsValidHandlerId(ids[i]));
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }

    auto pointerEvent = SetupPointerEvent002();
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);

    for (const auto& id : ids) {
        std::string sPointerEs = InputManagerTest::GetEventDump();
        MMI_HILOGD("sPointerEs : %{public}s", sPointerEs.c_str());
        ASSERT_TRUE(!sPointerEs.empty());
        if (IsValidHandlerId(id)) {
            InputManager::GetInstance()->RemoveMonitor(id);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_OnAddScreenMonitor_003
 * @tc.desc: Verify touchscreen up event monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_OnAddScreenMonitor_003, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = SetupPointerEvent003();
    ASSERT_TRUE(pointerEvent != nullptr);

    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
    EXPECT_TRUE(IsValidHandlerId(monitorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_OnAddScreenMonitor_004
 * @tc.desc: Verify touchscreen MarkConsumed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_OnAddScreenMonitor_004, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
    EXPECT_TRUE(IsValidHandlerId(monitorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestMarkConsumedStep1();
    auto pointerEvent = TestMarkConsumedStep2();

    TestMarkConsumedStep3(monitorId, pointerEvent->GetId());

    TestMarkConsumedStep4();
    TestMarkConsumedStep5();

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_OnAddScreenMonitor_005
 * @tc.desc:  Verify touchscreen MarkConsumed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_OnAddScreenMonitor_005, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
    EXPECT_TRUE(IsValidHandlerId(monitorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    auto pointerEvent = TestMarkConsumedStep1();

    TestMarkConsumedStep3(monitorId, pointerEvent->GetId());

    TestMarkConsumedStep4();
    TestMarkConsumedStep6();

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_OnAddTouchPadMonitor_001
 * @tc.desc: Verify touchpad down event monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_OnAddTouchPadMonitor_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDownTime(10010);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    std::vector<std::string> sLogs1 { SearchLog(command1, true) };
    int32_t monitorId { };
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
#ifdef OHOS_BUILD_ENABLE_MONITOR
    EXPECT_TRUE(!IsValidHandlerId(monitorId));
#else
    EXPECT_EQ(monitorId, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_MONITOR
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_OnAddTouchPadMonitor_002
 * @tc.desc: Verify touchpad move event monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_OnAddTouchPadMonitor_002, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDownTime(10010);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    std::vector<std::string> sLogs1 { SearchLog(command1, true) };

    int32_t monitorId { };
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
#ifdef OHOS_BUILD_ENABLE_MONITOR
    EXPECT_TRUE(!IsValidHandlerId(monitorId));
#else
    EXPECT_EQ(monitorId, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_MONITOR
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_OnAddTouchPadMonitor_003
 * @tc.desc: Verify touchpad up event monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_OnAddTouchPadMonitor_003, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDownTime(10010);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    std::vector<std::string> sLogs1 { SearchLog(command1, true) };
    int32_t monitorId { };
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
#ifdef OHOS_BUILD_ENABLE_MONITOR
    EXPECT_TRUE(!IsValidHandlerId(monitorId));
#else
    EXPECT_EQ(monitorId, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_MONITOR
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_OnAddTouchPadMonitor_004
 * @tc.desc: Verify touchpad multiple monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_OnAddTouchPadMonitor_004, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDownTime(10010);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    const std::vector<int32_t>::size_type N_TEST_CASES { 3 };
    std::vector<int32_t> ids(N_TEST_CASES);
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    for (std::vector<int32_t>::size_type i = 0; i < N_TEST_CASES; ++i) {
        ids[i] = InputManager::GetInstance()->AddMonitor(callBackPtr);
#ifdef OHOS_BUILD_ENABLE_MONITOR
        EXPECT_TRUE(!IsValidHandlerId(ids[i]));
#else
        EXPECT_EQ(ids[i], ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_MONITOR
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }

    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);

    for (const auto& id : ids) {
        std::string sPointerEs = InputManagerTest::GetEventDump();
        MMI_HILOGD("sPointerEs : %{public}s", sPointerEs.c_str());
        ASSERT_TRUE(!sPointerEs.empty());
        if (IsValidHandlerId(id)) {
            InputManager::GetInstance()->RemoveMonitor(id);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_OnAddTouchPadMonitor_005
 * @tc.desc: Verify touchpad monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_OnAddTouchPadMonitor_005, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDownTime(10010);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    std::vector<std::string> sLogs1 { SearchLog(command1, true) };
    int32_t monitorId { };
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
#ifdef OHOS_BUILD_ENABLE_MONITOR
    EXPECT_TRUE(!IsValidHandlerId(monitorId));
#else
    EXPECT_EQ(monitorId, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_MONITOR
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManager_TouchPadSimulateInputEvent_001
 * @tc.desc: Verify touchpad simulate and monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManager_TouchPadSimulateInputEvent_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId { InputManager::GetInstance()->AddMonitor(callBackPtr) };
#ifdef OHOS_BUILD_ENABLE_MONITOR
    EXPECT_TRUE(!(monitorId >= MIN_HANDLER_ID));
#else
    EXPECT_EQ(monitorId >= MIN_HANDLER_ID, false);
#endif // OHOS_BUILD_ENABLE_MONITOR
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    int64_t actionTime = GetSysClockTime();
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item { };
    item.SetPointerId(DEFAULT_POINTER_ID);
    item.SetDownTime(actionTime);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(DEFAULT_DEVICE_ID);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetActionTime(actionTime);
    pointerEvent->SetPointerId(DEFAULT_POINTER_ID);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManager_TouchPadSimulateInputEvent_002
 * @tc.desc: Verify touchpad simulate and monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManager_TouchPadSimulateInputEvent_002, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId { InputManager::GetInstance()->AddMonitor(callBackPtr) };
#ifdef OHOS_BUILD_ENABLE_MONITOR
    EXPECT_TRUE(!(monitorId >= MIN_HANDLER_ID));
#else
    EXPECT_EQ(monitorId >= MIN_HANDLER_ID, false);
#endif // OHOS_BUILD_ENABLE_MONITOR
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    int64_t actionTime = GetSysClockTime();
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item { };
    item.SetPointerId(DEFAULT_POINTER_ID);
    item.SetDownTime(actionTime);
    item.SetPressed(true);
    item.SetGlobalX(700);
    item.SetGlobalY(610);
    item.SetDeviceId(DEFAULT_DEVICE_ID);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetActionTime(actionTime);
    pointerEvent->SetPointerId(DEFAULT_POINTER_ID);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManager_TouchPadSimulateInputEvent_003
 * @tc.desc: Verify touchpad simulate and monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManager_TouchPadSimulateInputEvent_003, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId { InputManager::GetInstance()->AddMonitor(callBackPtr) };
#ifdef OHOS_BUILD_ENABLE_MONITOR
    EXPECT_TRUE(!(monitorId >= MIN_HANDLER_ID));
#else
    EXPECT_EQ(monitorId >= MIN_HANDLER_ID, false);
#endif // OHOS_BUILD_ENABLE_MONITOR
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    int64_t actionTime = GetSysClockTime();
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item { };
    item.SetPointerId(DEFAULT_POINTER_ID);
    item.SetDownTime(actionTime);
    item.SetPressed(false);
    item.SetGlobalX(50);
    item.SetGlobalY(50);
    item.SetDeviceId(DEFAULT_DEVICE_ID);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetActionTime(actionTime);
    pointerEvent->SetPointerId(DEFAULT_POINTER_ID);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManager_TouchPadSimulateInputEvent_004
 * @tc.desc: Verify touchpad simulate and monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManager_TouchPadSimulateInputEvent_004, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId { InputManager::GetInstance()->AddMonitor(callBackPtr) };
#ifdef OHOS_BUILD_ENABLE_MONITOR
    EXPECT_TRUE(!(monitorId >= MIN_HANDLER_ID));
#else
    EXPECT_EQ(monitorId >= MIN_HANDLER_ID, false);
#endif // OHOS_BUILD_ENABLE_MONITOR
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    int64_t actionTime = GetSysClockTime();
    auto pointerEvent = PointerEvent::Create();
    ASSERT_TRUE(pointerEvent != nullptr);
    PointerEvent::PointerItem item { };
    item.SetPointerId(DEFAULT_POINTER_ID);
    item.SetDownTime(actionTime);
    item.SetPressed(true);
    item.SetGlobalX(523);
    item.SetGlobalY(723);
    item.SetDeviceId(DEFAULT_DEVICE_ID);
    pointerEvent->AddPointerItem(item);
    item.SetPointerId(1);
    item.SetDownTime(actionTime);
    item.SetPressed(true);
    item.SetGlobalX(540);
    item.SetGlobalY(740);
    item.SetDeviceId(DEFAULT_DEVICE_ID);
    pointerEvent->AddPointerItem(item);
    item.SetPointerId(2);
    item.SetDownTime(actionTime);
    item.SetPressed(true);
    item.SetGlobalX(560);
    item.SetGlobalY(760);
    item.SetDeviceId(DEFAULT_DEVICE_ID);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetActionTime(actionTime);
    pointerEvent->SetPointerId(DEFAULT_POINTER_ID);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_AddMouseMonitor_001
 * @tc.desc: Verify mouse down event monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_AddMouseMonitor_001, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
    EXPECT_TRUE(IsValidHandlerId(monitorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    auto pointerEvent = SetupPointerEvent005();
    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
#else
    if (id1 == ERROR_UNSUPPORT) {
        InputManager::GetInstance()->RemoveMonitor(id1);
    }
#endif // OHOS_BUILD_ENABLE_MONITOR
}

/**
 * @tc.name: InputManagerTest_AddMouseMonitor_002
 * @tc.desc: Verify mouse move event monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_AddMouseMonitor_002, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
    EXPECT_TRUE(IsValidHandlerId(monitorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    auto pointerEvent = SetupPointerEvent006();
    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }

    TestSimulateInputEvent(pointerEvent, TestScene::EXCEPTION_TEST);
}


/**
 * @tc.name: InputManagerTest_AddMouseMonitor_003
 * @tc.desc: Verify mouse up event monitor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_AddMouseMonitor_003, TestSize.Level1)
{
    CALL_LOG_ENTER;
    auto callBackPtr = GetPtr<InputEventCallback>();
    ASSERT_TRUE(callBackPtr != nullptr);
    int32_t monitorId = InputManager::GetInstance()->AddMonitor(callBackPtr);
    EXPECT_TRUE(IsValidHandlerId(monitorId));
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));

    auto pointerEvent = SetupPointerEvent007();;
    ASSERT_TRUE(pointerEvent != nullptr);
    TestSimulateInputEvent(pointerEvent);

    if (IsValidHandlerId(monitorId)) {
        InputManager::GetInstance()->RemoveMonitor(monitorId);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
}

/**
 * @tc.name: InputManagerTest_AddMouseMonitor_004
 * @tc.desc: Verify monitor upper limit
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_AddMouseMonitor_004, TestSize.Level1)
{
    CALL_LOG_ENTER;
    const std::vector<int32_t>::size_type N_TEST_CASES { MAX_N_INPUT_HANDLERS };
    std::vector<int32_t> ids(N_TEST_CASES);
    int32_t maxMonitor = 0;

    for (std::vector<int32_t>::size_type i = 0; i < N_TEST_CASES; ++i) {
        auto callBackPtr =  GetPtr<InputEventCallback>();
        ASSERT_TRUE(callBackPtr != nullptr);
        ids[i] = InputManager::GetInstance()->AddMonitor(callBackPtr);
        EXPECT_TRUE(IsValidHandlerId(ids[i]));
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }

    auto pointerEvent = SetupPointerEvent007();
    ASSERT_TRUE(pointerEvent != nullptr);
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);

    for (const auto& id : ids) {
        if (!InputManagerTest::GetEventDump().empty()) {
            maxMonitor++;
        }
        if (IsValidHandlerId(id)) {
            InputManager::GetInstance()->RemoveMonitor(id);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
#else
    std::vector<std::string> tLogs { SearchLog(command, sLogs) };
    if (id1 == ERROR_UNSUPPORT) {
        InputManager::GetInstance()->RemoveMonitor(id1);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
#endif // OHOS_BUILD_ENABLE_MONITOR
}

#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING
/**
 * @tc.name: InputManagerTest_MoveMouse_01
 * @tc.desc: Verify move mouse
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_MoveMouse_01, TestSize.Level1)
{
    CALL_LOG_ENTER;
    InputManager::GetInstance()->MoveMouse(50, 50);
}

/**
 * @tc.name: InputManagerTest_MoveMouse_02
 * @tc.desc: Verify move mouse
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_MoveMouse_02, TestSize.Level1)
{
    CALL_LOG_ENTER;
    InputManager::GetInstance()->MoveMouse(-1000, 100);
}
#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING

static int32_t deviceIDtest = 0;
static void GetKeyboardTypeCallback(int32_t keyboardType)
{
    switch (keyboardType) {
        case KEYBOARD_TYPE_NONE: {
            MMI_HILOGD("deviceIDtest:%{public}d-->KeyboardType: %{public}s", deviceIDtest, "None");
            break;
            }
        case KEYBOARD_TYPE_UNKNOWN: {
            MMI_HILOGD("deviceIDtest:%{public}d-->KeyboardType: %{public}s", deviceIDtest, "unknown");
            break;
        }
        case KEYBOARD_TYPE_ALPHABETICKEYBOARD: {
            MMI_HILOGD("deviceIDtest:%{public}d-->KeyboardType: %{public}s", deviceIDtest, "alphabetickeyboard");
            break;
        }
        case KEYBOARD_TYPE_DIGITALKEYBOARD: {
            MMI_HILOGD("deviceIDtest:%{public}d-->KeyboardType: %{public}s", deviceIDtest, "digitalkeyboard");
            break;
        }
        case KEYBOARD_TYPE_HANDWRITINGPEN: {
            MMI_HILOGD("deviceIDtest:%{public}d-->KeyboardType: %{public}s", deviceIDtest, "handwritingpen");
            break;
        }
        case KEYBOARD_TYPE_REMOTECONTROL: {
            MMI_HILOGD("deviceIDtest:%{public}d-->KeyboardType: %{public}s", deviceIDtest, "remotecontrol");
            break;
        }
        default: {
            MMI_HILOGW("Error obtaining keyboard type");
            break;
        }
    }
}

/**
 * @tc.name: InputManagerTest_GetKeyboardType
 * @tc.desc: Verify Get Keyboard Type
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputManagerTest, InputManagerTest_GetKeyboardType, TestSize.Level1)
{
    MMI_HILOGD("Start InputManagerTest_GetKeyboardType");
    for (int32_t i = 0; i < 20; ++i)
    {
        deviceIDtest = i;
        InputManager::GetInstance()->GetKeyboardType(i, GetKeyboardTypeCallback);
        MMI_HILOGD("i:%{public}d", i);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP));
    MMI_HILOGD("Stop InputManagerTest_GetKeyboardType");
}
} // namespace MMI
} // namespace OHOS