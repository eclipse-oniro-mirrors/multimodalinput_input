/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "event_normalize_handler.h"

#include "bytrace_adapter.h"
#include "define_multimodal.h"
#include "dfx_hisysevent.h"

#include "error_multimodal.h"
#include "event_log_helper.h"
#include "input_device_manager.h"
#include "input_event_handler.h"
#include "key_auto_repeat.h"
#include "key_event_normalize.h"
#include "key_event_value_transformation.h"
#include "libinput_adapter.h"
#include "mmi_log.h"
#include "time_cost_chk.h"
#include "timer_manager.h"
#include "touch_event_normalize.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "EventNormalizeHandler" };
constexpr int32_t FINGER_NUM = 2;
constexpr int32_t MT_TOOL_PALM = 2;
}

void EventNormalizeHandler::HandleEvent(libinput_event* event)
{
    CALL_DEBUG_ENTER;
    CHKPV(event);
    DfxHisysevent::GetDispStartTime();
    auto type = libinput_event_get_type(event);
    TimeCostChk chk("HandleLibinputEvent", "overtime 1000(us)", MAX_INPUT_EVENT_TIME, type);
    if (type == LIBINPUT_EVENT_TOUCH_CANCEL || type == LIBINPUT_EVENT_TOUCH_FRAME) {
        MMI_HILOGD("This touch event is canceled type:%{public}d", type);
        return;
    }
    switch (type) {
        case LIBINPUT_EVENT_DEVICE_ADDED: {
            OnEventDeviceAdded(event);
            break;
        }
        case LIBINPUT_EVENT_DEVICE_REMOVED: {
            OnEventDeviceRemoved(event);
            break;
        }
        case LIBINPUT_EVENT_KEYBOARD_KEY: {
            HandleKeyboardEvent(event);
            DfxHisysevent::CalcKeyDispTimes();
            break;
        }
        case LIBINPUT_EVENT_POINTER_MOTION:
        case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        case LIBINPUT_EVENT_POINTER_BUTTON:
        case LIBINPUT_EVENT_POINTER_BUTTON_TOUCHPAD:
        case LIBINPUT_EVENT_POINTER_AXIS:
        case LIBINPUT_EVENT_POINTER_TAP:
        case LIBINPUT_EVENT_POINTER_MOTION_TOUCHPAD: {
            HandleMouseEvent(event);
            DfxHisysevent::CalcPointerDispTimes();
            break;
        }
        case LIBINPUT_EVENT_TOUCHPAD_DOWN:
        case LIBINPUT_EVENT_TOUCHPAD_UP:
        case LIBINPUT_EVENT_TOUCHPAD_MOTION: {
            HandleTouchPadEvent(event);
            DfxHisysevent::CalcPointerDispTimes();
            break;
        }
        case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN:
        case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE:
        case LIBINPUT_EVENT_GESTURE_SWIPE_END:
        case LIBINPUT_EVENT_GESTURE_PINCH_BEGIN:
        case LIBINPUT_EVENT_GESTURE_PINCH_UPDATE:
        case LIBINPUT_EVENT_GESTURE_PINCH_END: {
            HandleGestureEvent(event);
            DfxHisysevent::CalcPointerDispTimes();
            break;
        }
        case LIBINPUT_EVENT_TOUCH_DOWN:
        case LIBINPUT_EVENT_TOUCH_UP:
        case LIBINPUT_EVENT_TOUCH_MOTION: {
            HandleTouchEvent(event);
            DfxHisysevent::CalcPointerDispTimes();
            break;
        }
        case LIBINPUT_EVENT_TABLET_TOOL_AXIS:
        case LIBINPUT_EVENT_TABLET_TOOL_PROXIMITY:
        case LIBINPUT_EVENT_TABLET_TOOL_TIP: {
            HandleTableToolEvent(event);
            break;
        }
        case LIBINPUT_EVENT_JOYSTICK_BUTTON:
        case LIBINPUT_EVENT_JOYSTICK_AXIS: {
            HandleJoystickEvent(event);
            DfxHisysevent::CalcPointerDispTimes();
            break;
        }
        case LIBINPUT_EVENT_SWITCH_TOGGLE: {
            HandleSwitchInputEvent(event);
            break;
        }
        default: {
            MMI_HILOGW("This device does not support :%d", type);
            break;
        }
    }
    DfxHisysevent::ReportDispTimes();
}

int32_t EventNormalizeHandler::OnEventDeviceAdded(libinput_event *event)
{
    CHKPR(event, ERROR_NULL_POINTER);
    auto device = libinput_event_get_device(event);
    CHKPR(device, ERROR_NULL_POINTER);
    InputDevMgr->OnInputDeviceAdded(device);
    KeyMapMgr->ParseDeviceConfigFile(device);
    KeyRepeat->AddDeviceConfig(device);
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    KeyEventHdr->ResetKeyEvent(device);
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t EventNormalizeHandler::OnEventDeviceRemoved(libinput_event *event)
{
    CHKPR(event, ERROR_NULL_POINTER);
    auto device = libinput_event_get_device(event);
    CHKPR(device, ERROR_NULL_POINTER);
    KeyMapMgr->RemoveKeyValue(device);
    KeyRepeat->RemoveDeviceConfig(device);
    InputDevMgr->OnInputDeviceRemoved(device);
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
void EventNormalizeHandler::HandleKeyEvent(const std::shared_ptr<KeyEvent> keyEvent)
{
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("Keyboard device does not support");
        return;
    }
    DfxHisysevent::GetDispStartTime();
    CHKPV(keyEvent);
    EventLogHelper::PrintEventData(keyEvent);
    nextHandler_->HandleKeyEvent(keyEvent);
    if (keyEvent->IsRepeat()) {
        KeyRepeat->SelectAutoRepeat(keyEvent);
        keyEvent->SetRepeat(false);
    }
    DfxHisysevent::CalcKeyDispTimes();
    DfxHisysevent::ReportDispTimes();
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

#ifdef OHOS_BUILD_ENABLE_POINTER
void EventNormalizeHandler::HandlePointerEvent(const std::shared_ptr<PointerEvent> pointerEvent)
{
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("Pointer device does not support");
        return;
    }
    DfxHisysevent::GetDispStartTime();
    CHKPV(pointerEvent);
    if (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_AXIS_END) {
        MMI_HILOGI("MouseEvent Normalization Results, PointerAction:%{public}d,PointerId:%{public}d,"
            "SourceType:%{public}d,ButtonId:%{public}d,"
            "VerticalAxisValue:%{public}lf,HorizontalAxisValue:%{public}lf",
            pointerEvent->GetPointerAction(), pointerEvent->GetPointerId(), pointerEvent->GetSourceType(),
            pointerEvent->GetButtonId(), pointerEvent->GetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_VERTICAL),
            pointerEvent->GetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_HORIZONTAL));
        PointerEvent::PointerItem item;
        if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item)) {
            MMI_HILOGE("Get pointer item failed. pointer:%{public}d", pointerEvent->GetPointerId());
            return;
        }
        MMI_HILOGI("MouseEvent Item Normalization Results, DownTime:%{public}" PRId64 ",IsPressed:%{public}d,"
            "DisplayX:%{public}d,DisplayY:%{public}d,WindowX:%{public}d,WindowY:%{public}d,"
            "Width:%{public}d,Height:%{public}d,Pressure:%{public}f,Device:%{public}d",
            item.GetDownTime(), static_cast<int32_t>(item.IsPressed()), item.GetDisplayX(), item.GetDisplayY(),
            item.GetWindowX(), item.GetWindowY(), item.GetWidth(), item.GetHeight(), item.GetPressure(),
            item.GetDeviceId());
    }
    WinMgr->UpdateTargetPointer(pointerEvent);
    nextHandler_->HandlePointerEvent(pointerEvent);
    DfxHisysevent::CalcPointerDispTimes();
    DfxHisysevent::ReportDispTimes();
}
#endif // OHOS_BUILD_ENABLE_POINTER

#ifdef OHOS_BUILD_ENABLE_TOUCH
void EventNormalizeHandler::HandleTouchEvent(const std::shared_ptr<PointerEvent> pointerEvent)
{
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("Touchscreen device does not support");
        return;
    }
    DfxHisysevent::GetDispStartTime();
    CHKPV(pointerEvent);
    WinMgr->UpdateTargetPointer(pointerEvent);
    nextHandler_->HandleTouchEvent(pointerEvent);
    DfxHisysevent::CalcPointerDispTimes();
    DfxHisysevent::ReportDispTimes();
}
#endif // OHOS_BUILD_ENABLE_TOUCH

int32_t EventNormalizeHandler::HandleKeyboardEvent(libinput_event* event)
{
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("Keyboard device does not support");
        return ERROR_UNSUPPORT;
    }
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    auto keyEvent = KeyEventHdr->GetKeyEvent();
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    CHKPR(event, ERROR_NULL_POINTER);
    std::vector<int32_t> pressedKeys = keyEvent->GetPressedKeys();
    int32_t lastPressedKey = -1;
    if (!pressedKeys.empty()) {
        lastPressedKey = pressedKeys.back();
        MMI_HILOGD("The last repeat button, keyCode:%{public}d", lastPressedKey);
    }
    auto packageResult = KeyEventHdr->Normalize(event, keyEvent);
    if (packageResult == MULTIDEVICE_SAME_EVENT_MARK) {
        MMI_HILOGD("The same event reported by multi_device should be discarded");
        return RET_OK;
    }
    if (packageResult != RET_OK) {
        MMI_HILOGE("KeyEvent package failed, ret:%{public}d,errCode:%{public}d", packageResult, KEY_EVENT_PKG_FAIL);
        return KEY_EVENT_PKG_FAIL;
    }

    BytraceAdapter::StartBytrace(keyEvent);
    EventLogHelper::PrintEventData(keyEvent);
    nextHandler_->HandleKeyEvent(keyEvent);
    KeyRepeat->SelectAutoRepeat(keyEvent);
    MMI_HILOGD("keyCode:%{public}d, action:%{public}d", keyEvent->GetKeyCode(), keyEvent->GetKeyAction());
#else
    MMI_HILOGW("Keyboard device does not support");
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t EventNormalizeHandler::HandleMouseEvent(libinput_event* event)
{
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("Pointer device does not support");
        return ERROR_UNSUPPORT;
    }
#ifdef OHOS_BUILD_ENABLE_POINTER
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    const auto &keyEvent = KeyEventHdr->GetKeyEvent();
    CHKPR(keyEvent, ERROR_NULL_POINTER);
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    if (MouseEventHdr->OnEvent(event) == RET_ERR) {
        MMI_HILOGE("OnEvent is failed");
        return RET_ERR;
    }
    auto pointerEvent = MouseEventHdr->GetPointerEvent();
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    std::vector<int32_t> pressedKeys = keyEvent->GetPressedKeys();
    for (const int32_t& keyCode : pressedKeys) {
        MMI_HILOGI("Pressed keyCode:%{public}d", keyCode);
    }
    pointerEvent->SetPressedKeys(pressedKeys);
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    BytraceAdapter::StartBytrace(pointerEvent, BytraceAdapter::TRACE_START);
    HandlePalmEvent(event, pointerEvent);
    nextHandler_->HandlePointerEvent(pointerEvent);
#else
    MMI_HILOGW("Pointer device does not support");
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

void EventNormalizeHandler::HandlePalmEvent(libinput_event* event, std::shared_ptr<PointerEvent> pointerEvent)
{
    auto touchpad = libinput_event_get_touchpad_event(event);
    CHKPV(touchpad);
    int32_t toolType = libinput_event_touchpad_get_tool_type(touchpad);
    if (toolType == MT_TOOL_PALM) {
        MMI_HILOGD("ToolType is MT_TOOL_PALM");
        pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    }
}

int32_t EventNormalizeHandler::HandleTouchPadEvent(libinput_event* event)
{
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("Pointer device does not support");
        return ERROR_UNSUPPORT;
    }
#ifdef OHOS_BUILD_ENABLE_POINTER
    CHKPR(event, ERROR_NULL_POINTER);
    auto touchpad = libinput_event_get_touchpad_event(event);
    CHKPR(touchpad, ERROR_NULL_POINTER);
    int32_t seatSlot = libinput_event_touchpad_get_seat_slot(touchpad);
    buttonIds_.insert(seatSlot);
    auto type = libinput_event_get_type(event);
    if (buttonIds_.size() == FINGER_NUM &&
        (type == LIBINPUT_EVENT_TOUCHPAD_DOWN || type == LIBINPUT_EVENT_TOUCHPAD_UP)) {
        MMI_HILOGD("Handle mouse axis event");
        HandleMouseEvent(event);
    }
    return RET_OK;
#else
    MMI_HILOGW("Pointer device does not support");
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t EventNormalizeHandler::HandleGestureEvent(libinput_event* event)
{
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("Pointer device does not support");
        return ERROR_UNSUPPORT;
    }
#ifdef OHOS_BUILD_ENABLE_POINTER
    CHKPR(event, ERROR_NULL_POINTER);
    auto pointerEvent = TouchEventHdr->OnLibInput(event, TouchEventNormalize::DeviceType::TOUCH_PAD);
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    nextHandler_->HandlePointerEvent(pointerEvent);
    auto type = libinput_event_get_type(event);
    if (type == LIBINPUT_EVENT_GESTURE_SWIPE_END || type == LIBINPUT_EVENT_GESTURE_PINCH_END) {
        pointerEvent->RemovePointerItem(pointerEvent->GetPointerId());
        MMI_HILOGD("This touch pad event is up remove this finger");
        if (pointerEvent->GetPointerIds().empty()) {
            MMI_HILOGD("This touch pad event is final finger up remove this finger");
            pointerEvent->Reset();
        }
    }
#else
    MMI_HILOGW("Pointer device does not support");
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t EventNormalizeHandler::HandleTouchEvent(libinput_event* event)
{
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("Touchscreen device does not support");
        return ERROR_UNSUPPORT;
    }
#ifdef OHOS_BUILD_ENABLE_TOUCH
    CHKPR(event, ERROR_NULL_POINTER);
    auto pointerEvent = TouchEventHdr->OnLibInput(event, TouchEventNormalize::DeviceType::TOUCH);
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    BytraceAdapter::StartBytrace(pointerEvent, BytraceAdapter::TRACE_START);
    nextHandler_->HandleTouchEvent(pointerEvent);
    ResetTouchUpEvent(pointerEvent, event);
#else
    MMI_HILOGW("Touchscreen device does not support");
#endif // OHOS_BUILD_ENABLE_TOUCH
    return RET_OK;
}

void EventNormalizeHandler::ResetTouchUpEvent(std::shared_ptr<PointerEvent> pointerEvent,
    struct libinput_event *event)
{
    CHKPV(pointerEvent);
    CHKPV(event);
    auto type = libinput_event_get_type(event);
    if (type == LIBINPUT_EVENT_TOUCH_UP) {
        pointerEvent->RemovePointerItem(pointerEvent->GetPointerId());
        MMI_HILOGD("This touch event is up remove this finger");
        if (pointerEvent->GetPointerIds().empty()) {
            MMI_HILOGD("This touch event is final finger up remove this finger");
            pointerEvent->Reset();
        }
    }
}

int32_t EventNormalizeHandler::HandleTableToolEvent(libinput_event* event)
{
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("TableTool device does not support");
        return ERROR_UNSUPPORT;
    }
#ifdef OHOS_BUILD_ENABLE_TOUCH
    CHKPR(event, ERROR_NULL_POINTER);
    auto pointerEvent = TouchEventHdr->OnLibInput(event, TouchEventNormalize::DeviceType::TABLET_TOOL);
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    BytraceAdapter::StartBytrace(pointerEvent, BytraceAdapter::TRACE_START);
    nextHandler_->HandleTouchEvent(pointerEvent);
    if (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_UP) {
        pointerEvent->Reset();
    }
#else
    MMI_HILOGW("TableTool device does not support");
#endif // OHOS_BUILD_ENABLE_TOUCH
    return RET_OK;
}

int32_t EventNormalizeHandler::HandleJoystickEvent(libinput_event* event)
{
    CALL_DEBUG_ENTER;
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("Joystick device does not support");
        return ERROR_UNSUPPORT;
    }
#ifdef OHOS_BUILD_ENABLE_JOYSTICK
    CHKPR(event, ERROR_NULL_POINTER);
    auto pointerEvent = TouchEventHdr->OnLibInput(event, TouchEventNormalize::DeviceType::JOYSTICK);
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    BytraceAdapter::StartBytrace(pointerEvent, BytraceAdapter::TRACE_START);
    nextHandler_->HandlePointerEvent(pointerEvent);
#else
    MMI_HILOGW("Joystick device does not support");
#endif // OHOS_BUILD_ENABLE_JOYSTICK
    return RET_OK;
}

int32_t EventNormalizeHandler::HandleSwitchInputEvent(libinput_event* event)
{
    if (nextHandler_ == nullptr) {
        MMI_HILOGW("switch device does not support");
        return ERROR_UNSUPPORT;
    }
#ifdef OHOS_BUILD_ENABLE_SWITCH
    CHKPR(event, ERROR_NULL_POINTER);
    struct libinput_event_switch *swev = libinput_event_get_switch_event(event);
    CHKPR(swev, ERROR_NULL_POINTER);

    enum libinput_switch_state state = libinput_event_switch_get_switch_state(swev);
    auto swEvent = std::make_unique<SwitchEvent>(static_cast<int>(state));
    nextHandler_->HandleSwitchEvent(std::move(swEvent));
#else
    MMI_HILOGW("switch device does not support");
#endif // OHOS_BUILD_ENABLE_SWITCH
    return RET_OK;
}

int32_t EventNormalizeHandler::AddHandleTimer(int32_t timeout)
{
    CALL_DEBUG_ENTER;
    timerId_ = TimerMgr->AddTimer(timeout, 1, [this]() {
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
        auto keyEvent = KeyEventHdr->GetKeyEvent();
        CHKPV(keyEvent);
        CHKPV(nextHandler_);
        nextHandler_->HandleKeyEvent(keyEvent);
        int32_t triggerTime = KeyRepeat->GetIntervalTime(keyEvent->GetDeviceId());
        this->AddHandleTimer(triggerTime);
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    });
    return timerId_;
}
} // namespace MMI
} // namespace OHOS
