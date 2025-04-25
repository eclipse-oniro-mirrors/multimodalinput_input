/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "input_manager.h"

#include "input_manager_impl.h"
#include "hitrace_meter.h"
#include "pre_monitor_manager.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "InputManager"

namespace OHOS {
namespace MMI {
InputManager *InputManager::instance_ = new (std::nothrow) InputManager();

const std::map<int32_t, int32_t> MOUSE_TO_TOUCH_PARAM_MAP = {
    {PointerEvent::SOURCE_TYPE_MOUSE, PointerEvent::SOURCE_TYPE_TOUCHSCREEN},
    {PointerEvent::POINTER_ACTION_BUTTON_DOWN, PointerEvent::POINTER_ACTION_DOWN},
    {PointerEvent::POINTER_ACTION_BUTTON_UP, PointerEvent::POINTER_ACTION_UP},
    {PointerEvent::TOOL_TYPE_MOUSE, PointerEvent::TOOL_TYPE_FINGER},
    {PointerEvent::MOUSE_BUTTON_LEFT, PointerEvent::POINTER_INITIAL_VALUE}
};

const std::map<int32_t, int32_t> TOUCH_TO_MOUSE_PARAM_MAP = {
    {PointerEvent::POINTER_ACTION_DOWN, PointerEvent::POINTER_ACTION_BUTTON_DOWN},
    {PointerEvent::POINTER_ACTION_UP, PointerEvent::POINTER_ACTION_BUTTON_UP},
    {PointerEvent::TOOL_TYPE_FINGER, PointerEvent::TOOL_TYPE_MOUSE}
};

InputManager *InputManager::GetInstance()
{
    return instance_;
}

int32_t InputManager::GetDisplayBindInfo(DisplayBindInfos &infos)
{
    return InputMgrImpl.GetDisplayBindInfo(infos);
}

int32_t InputManager::GetAllMmiSubscribedEvents(std::map<std::tuple<int32_t, int32_t, std::string>, int32_t> &datas)
{
    return InputMgrImpl.GetAllMmiSubscribedEvents(datas);
}

int32_t InputManager::SetDisplayBind(int32_t deviceId, int32_t displayId, std::string &msg)
{
    return InputMgrImpl.SetDisplayBind(deviceId, displayId, msg);
}

int32_t InputManager::GetWindowPid(int32_t windowId)
{
    return InputMgrImpl.GetWindowPid(windowId);
}

int32_t InputManager::UpdateDisplayInfo(const DisplayGroupInfo &displayGroupInfo)
{
    return InputMgrImpl.UpdateDisplayInfo(displayGroupInfo);
}

int32_t InputManager::UpdateWindowInfo(const WindowGroupInfo &windowGroupInfo)
{
    return InputMgrImpl.UpdateWindowInfo(windowGroupInfo);
}

int32_t InputManager::AddInputEventFilter(std::shared_ptr<IInputEventFilter> filter, int32_t priority,
    uint32_t deviceTags)
{
    return InputMgrImpl.AddInputEventFilter(filter, priority, deviceTags);
}

int32_t InputManager::RemoveInputEventFilter(int32_t filterId)
{
    return InputMgrImpl.RemoveInputEventFilter(filterId);
}

int32_t InputManager::AddInputEventObserver(std::shared_ptr<MMIEventObserver> observer)
{
    return InputMgrImpl.AddInputEventObserver(observer);
}

int32_t InputManager::RemoveInputEventObserver(std::shared_ptr<MMIEventObserver> observer)
{
    return InputMgrImpl.RemoveInputEventObserver(observer);
}

int32_t InputManager::SetWindowInputEventConsumer(std::shared_ptr<IInputEventConsumer> inputEventConsumer)
{
    return InputMgrImpl.SetWindowInputEventConsumer(inputEventConsumer, nullptr);
}

int32_t InputManager::SetWindowInputEventConsumer(std::shared_ptr<IInputEventConsumer> inputEventConsumer,
    std::shared_ptr<AppExecFwk::EventHandler> eventHandler)
{
    CHKPR(eventHandler, RET_ERR);
    return InputMgrImpl.SetWindowInputEventConsumer(inputEventConsumer, eventHandler);
}

int32_t InputManager::SubscribeKeyEvent(std::shared_ptr<KeyOption> keyOption,
    std::function<void(std::shared_ptr<KeyEvent>)> callback)
{
    return InputMgrImpl.SubscribeKeyEvent(keyOption, callback);
}

void InputManager::UnsubscribeKeyEvent(int32_t subscriberId)
{
    InputMgrImpl.UnsubscribeKeyEvent(subscriberId);
}

int32_t InputManager::SubscribeHotkey(std::shared_ptr<KeyOption> keyOption,
    std::function<void(std::shared_ptr<KeyEvent>)> callback)
{
    return InputMgrImpl.SubscribeHotkey(keyOption, callback);
}

void InputManager::UnsubscribeHotkey(int32_t subscriberId)
{
    InputMgrImpl.UnsubscribeHotkey(subscriberId);
}

int32_t InputManager::SubscribeKeyMonitor(const KeyMonitorOption &keyOption,
    std::function<void(std::shared_ptr<KeyEvent>)> callback)
{
    return InputMgrImpl.SubscribeKeyMonitor(keyOption, callback);
}

int32_t InputManager::UnsubscribeKeyMonitor(int32_t subscriberId)
{
    return InputMgrImpl.UnsubscribeKeyMonitor(subscriberId);
}

int32_t InputManager::SubscribeSwitchEvent(std::function<void(std::shared_ptr<SwitchEvent>)> callback,
    SwitchEvent::SwitchType switchType)
{
    return InputMgrImpl.SubscribeSwitchEvent(static_cast<int32_t>(switchType), callback);
}

void InputManager::UnsubscribeSwitchEvent(int32_t subscriberId)
{
    InputMgrImpl.UnsubscribeSwitchEvent(subscriberId);
}

int32_t InputManager::SubscribeTabletProximity(std::function<void(std::shared_ptr<PointerEvent>)> callback)
{
    return InputMgrImpl.SubscribeTabletProximity(callback);
}

void InputManager::UnsubscribetabletProximity(int32_t subscriberId)
{
    InputMgrImpl.UnsubscribetabletProximity(subscriberId);
}

int32_t InputManager::SubscribeLongPressEvent(const LongPressRequest &longPressRequest,
    std::function<void(LongPressEvent)> callback)
{
    return InputMgrImpl.SubscribeLongPressEvent(longPressRequest, callback);
}
 
void InputManager::UnsubscribeLongPressEvent(int32_t subscriberId)
{
    InputMgrImpl.UnsubscribeLongPressEvent(subscriberId);
}

int32_t InputManager::AddMonitor(std::function<void(std::shared_ptr<KeyEvent>)> monitor)
{
    return InputMgrImpl.AddMonitor(monitor);
}

int32_t InputManager::AddMonitor(std::function<void(std::shared_ptr<PointerEvent>)> monitor)
{
    return InputMgrImpl.AddMonitor(monitor);
}

int32_t InputManager::AddMonitor(std::shared_ptr<IInputEventConsumer> monitor, HandleEventType eventType)
{
    return InputMgrImpl.AddMonitor(monitor, eventType);
}

int32_t InputManager::AddMonitor(std::shared_ptr<IInputEventConsumer> monitor, std::vector<int32_t> actionsType)
{
    return InputMgrImpl.AddMonitor(monitor, actionsType);
}

int32_t InputManager::AddPreMonitor(
    std::shared_ptr<IInputEventConsumer> monitor, HandleEventType eventType, std::vector<int32_t> keys)
{
    return PRE_MONITOR_MGR.AddHandler(monitor, eventType, keys);
}

void InputManager::RemovePreMonitor(int32_t monitorId)
{
    PRE_MONITOR_MGR.RemoveHandler(monitorId);
}

void InputManager::RemoveMonitor(int32_t monitorId)
{
    InputMgrImpl.RemoveMonitor(monitorId);
}

int32_t InputManager::AddGestureMonitor(
    std::shared_ptr<IInputEventConsumer> consumer, TouchGestureType type, int32_t fingers)
{
    return InputMgrImpl.AddGestureMonitor(consumer, type, fingers);
}

int32_t InputManager::RemoveGestureMonitor(int32_t monitorId)
{
    return InputMgrImpl.RemoveGestureMonitor(monitorId);
}

void InputManager::MarkConsumed(int32_t monitorId, int32_t eventId)
{
    InputMgrImpl.MarkConsumed(monitorId, eventId);
}

void InputManager::MoveMouse(int32_t offsetX, int32_t offsetY)
{
    InputMgrImpl.MoveMouse(offsetX, offsetY);
}

int32_t InputManager::AddInterceptor(std::shared_ptr<IInputEventConsumer> interceptor)
{
    return InputMgrImpl.AddInterceptor(interceptor);
}

int32_t InputManager::AddInterceptor(std::function<void(std::shared_ptr<KeyEvent>)> interceptor)
{
    return InputMgrImpl.AddInterceptor(interceptor);
}

int32_t InputManager::AddInterceptor(std::shared_ptr<IInputEventConsumer> interceptor, int32_t priority,
    uint32_t deviceTags)
{
    return InputMgrImpl.AddInterceptor(interceptor, priority, deviceTags);
}

void InputManager::RemoveInterceptor(int32_t interceptorId)
{
    InputMgrImpl.RemoveInterceptor(interceptorId);
}

void InputManager::SimulateInputEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    LogTracer lt(keyEvent->GetId(), keyEvent->GetEventType(), keyEvent->GetKeyAction());
    keyEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    InputMgrImpl.SimulateInputEvent(keyEvent);
}

void InputManager::SimulateInputEvent(std::shared_ptr<PointerEvent> pointerEvent, bool isAutoToVirtualScreen)
{
    LogTracer lt(pointerEvent->GetId(), pointerEvent->GetEventType(), pointerEvent->GetPointerAction());
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
#ifdef OHOS_BUILD_ENABLE_ONE_HAND_MODE
    pointerEvent->SetAutoToVirtualScreen(isAutoToVirtualScreen);
#endif // OHOS_BUILD_ENABLE_ONE_HAND_MODE
    PointerEvent::PointerItem pointerItem;
    if (pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
        MMI_HILOGD("isAutoToVirtualScreen=%{public}s, DX=%{private}d, DY=%{private}d, "
            "FDX=%{private}d, FDY=%{private}d",
            isAutoToVirtualScreen ? "true" : "false", pointerItem.GetDisplayX(), pointerItem.GetDisplayY(),
            pointerItem.GetFixedDisplayX(), pointerItem.GetFixedDisplayY());
    } else {
        MMI_HILOGD("isAutoToVirtualScreen=%{public}s", isAutoToVirtualScreen ? "true" : "false");
    }
    InputMgrImpl.SimulateInputEvent(pointerEvent);
}

void InputManager::SimulateInputEvent(std::shared_ptr<PointerEvent> pointerEvent, float zOrder,
    bool isAutoToVirtualScreen)
{
    CHKPV(pointerEvent);
    LogTracer lt(pointerEvent->GetId(), pointerEvent->GetEventType(), pointerEvent->GetPointerAction());
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    pointerEvent->SetZOrder(zOrder);
#ifdef OHOS_BUILD_ENABLE_ONE_HAND_MODE
    pointerEvent->SetAutoToVirtualScreen(isAutoToVirtualScreen);
#endif // OHOS_BUILD_ENABLE_ONE_HAND_MODE
    PointerEvent::PointerItem pointerItem;
    if (pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
        MMI_HILOGD("zOrder=%{public}f, isAutoToVirtualScreen=%{public}s, DX=%{private}d, DY=%{private}d, "
            "FDX=%{private}d, FDY=%{private}d",
            zOrder, isAutoToVirtualScreen ? "true" : "false", pointerItem.GetDisplayX(), pointerItem.GetDisplayY(),
            pointerItem.GetFixedDisplayX(), pointerItem.GetFixedDisplayY());
    } else {
        MMI_HILOGD("zOrder=%{public}f, isAutoToVirtualScreen=%{public}s", zOrder,
            isAutoToVirtualScreen ? "true" : "false");
    }
    InputMgrImpl.SimulateInputEvent(pointerEvent);
}

void InputManager::SimulateTouchPadInputEvent(std::shared_ptr<PointerEvent> pointerEvent,
    const TouchpadCDG &touchpadCDG)
{
    CHKPV(pointerEvent);
    LogTracer lt(pointerEvent->GetId(), pointerEvent->GetEventType(), pointerEvent->GetPointerAction());
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    pointerEvent->SetZOrder(touchpadCDG.zOrder);
    InputMgrImpl.SimulateTouchPadInputEvent(pointerEvent, touchpadCDG);
}

void InputManager::SimulateTouchPadEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    LogTracer lt(pointerEvent->GetId(), pointerEvent->GetEventType(), pointerEvent->GetPointerAction());
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    InputMgrImpl.SimulateTouchPadEvent(pointerEvent);
}

bool InputManager::TransformMouseEventToTouchEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    if (pointerEvent->GetSourceType() != PointerEvent::SOURCE_TYPE_MOUSE) {
        MMI_HILOGD("It's not MouseEvent, don't need to transform");
        return true;
    }

    int32_t result = -1;
    if (MOUSE_TO_TOUCH_PARAM_MAP.count(pointerEvent->GetSourceType()) > 0) {
        result = MOUSE_TO_TOUCH_PARAM_MAP.at(pointerEvent->GetSourceType());
        pointerEvent->SetSourceType(result);
    }

    if (MOUSE_TO_TOUCH_PARAM_MAP.count(pointerEvent->GetPointerAction()) > 0) {
        result = MOUSE_TO_TOUCH_PARAM_MAP.at(pointerEvent->GetPointerAction());
        pointerEvent->SetPointerAction(result);
        pointerEvent->SetOriginPointerAction(result);
    }

    if (MOUSE_TO_TOUCH_PARAM_MAP.count(pointerEvent->GetButtonId()) > 0) {
        result = MOUSE_TO_TOUCH_PARAM_MAP.at(pointerEvent->GetButtonId());
        pointerEvent->SetButtonId(result);
    }

    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
        MMI_HILOGE("Can't find pointer item, pointer:%{public}d", pointerId);
        return false;
    }

    if (MOUSE_TO_TOUCH_PARAM_MAP.count(pointerItem.GetToolType()) > 0) {
        result = MOUSE_TO_TOUCH_PARAM_MAP.at(pointerItem.GetToolType());
        pointerItem.SetToolType(result);
    }

    pointerEvent->UpdatePointerItem(pointerId, pointerItem);
    return true;
}

bool InputManager::TransformTouchEventToMouseEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    if (pointerEvent->GetSourceType() != PointerEvent::SOURCE_TYPE_TOUCHSCREEN) {
        MMI_HILOGD("It's not MouseEvent, don't need to transform");
        return true;
    }

    int32_t result = -1;
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);

    if (TOUCH_TO_MOUSE_PARAM_MAP.count(pointerEvent->GetPointerAction()) > 0) {
        result = TOUCH_TO_MOUSE_PARAM_MAP.at(pointerEvent->GetPointerAction());
        pointerEvent->SetPointerAction(result);
        pointerEvent->SetOriginPointerAction(result);
    }

    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
        MMI_HILOGE("Can't find pointer item, pointer:%{public}d", pointerId);
        return false;
    }

    if (TOUCH_TO_MOUSE_PARAM_MAP.count(pointerItem.GetToolType()) > 0) {
        result = TOUCH_TO_MOUSE_PARAM_MAP.at(pointerItem.GetToolType());
        pointerItem.SetToolType(result);
    }

    if (pointerItem.GetTargetWindowId() > 0) {
        pointerItem.SetTargetWindowId(PointerEvent::POINTER_INITIAL_VALUE);
    }

    pointerEvent->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);

    pointerEvent->UpdatePointerItem(pointerId, pointerItem);
    return true;
}

int32_t InputManager::RegisterDevListener(std::string type, std::shared_ptr<IInputDeviceListener> listener)
{
    return InputMgrImpl.RegisterDevListener(type, listener);
}

int32_t InputManager::UnregisterDevListener(std::string type, std::shared_ptr<IInputDeviceListener> listener)
{
    return InputMgrImpl.UnregisterDevListener(type, listener);
}

int32_t InputManager::GetDeviceIds(std::function<void(std::vector<int32_t>&)> callback)
{
    return InputMgrImpl.GetDeviceIds(callback);
}

int32_t InputManager::GetDevice(int32_t deviceId,
    std::function<void(std::shared_ptr<InputDevice>)> callback)
{
    return InputMgrImpl.GetDevice(deviceId, callback);
}

int32_t InputManager::SupportKeys(int32_t deviceId, std::vector<int32_t> keyCodes,
    std::function<void(std::vector<bool>&)> callback)
{
    return InputMgrImpl.SupportKeys(deviceId, keyCodes, callback);
}

int32_t InputManager::SetMouseScrollRows(int32_t Rows)
{
    return InputMgrImpl.SetMouseScrollRows(Rows);
}

int32_t InputManager::GetMouseScrollRows(int32_t &Rows)
{
    return InputMgrImpl.GetMouseScrollRows(Rows);
}

int32_t InputManager::SetCustomCursor(int32_t windowId, void* pixelMap, int32_t focusX, int32_t focusY)
{
    return InputMgrImpl.SetCustomCursor(windowId, focusX, focusY, pixelMap);
}

int32_t InputManager::SetMouseIcon(int32_t windowId, void* pixelMap)
{
    return InputMgrImpl.SetMouseIcon(windowId, pixelMap);
}

int32_t InputManager::SetPointerSize(int32_t size)
{
    return InputMgrImpl.SetPointerSize(size);
}

int32_t InputManager::GetPointerSize(int32_t &size)
{
    return InputMgrImpl.GetPointerSize(size);
}

int32_t InputManager::GetCursorSurfaceId(uint64_t &surfaceId)
{
    return InputMgrImpl.GetCursorSurfaceId(surfaceId);
}

int32_t InputManager::SetMouseHotSpot(int32_t windowId, int32_t hotSpotX, int32_t hotSpotY)
{
    return InputMgrImpl.SetMouseHotSpot(windowId, hotSpotX, hotSpotY);
}

int32_t InputManager::SetMousePrimaryButton(int32_t primaryButton)
{
    return InputMgrImpl.SetMousePrimaryButton(primaryButton);
}

int32_t InputManager::GetMousePrimaryButton(int32_t &primaryButton)
{
    return InputMgrImpl.GetMousePrimaryButton(primaryButton);
}

int32_t InputManager::SetHoverScrollState(bool state)
{
    return InputMgrImpl.SetHoverScrollState(state);
}

int32_t InputManager::GetHoverScrollState(bool &state)
{
    return InputMgrImpl.GetHoverScrollState(state);
}

int32_t InputManager::SetPointerVisible(bool visible, int32_t priority)
{
    return InputMgrImpl.SetPointerVisible(visible, priority);
}

bool InputManager::IsPointerVisible()
{
    return InputMgrImpl.IsPointerVisible();
}

int32_t InputManager::SetPointerColor(int32_t color)
{
    return InputMgrImpl.SetPointerColor(color);
}

int32_t InputManager::GetPointerColor(int32_t &color)
{
    return InputMgrImpl.GetPointerColor(color);
}

int32_t InputManager::EnableCombineKey(bool enable)
{
    return InputMgrImpl.EnableCombineKey(enable);
}

int32_t InputManager::SetPointerSpeed(int32_t speed)
{
    return InputMgrImpl.SetPointerSpeed(speed);
}

int32_t InputManager::GetPointerSpeed(int32_t &speed)
{
    return InputMgrImpl.GetPointerSpeed(speed);
}

int32_t InputManager::GetKeyboardType(int32_t deviceId, std::function<void(int32_t)> callback)
{
    return InputMgrImpl.GetKeyboardType(deviceId, callback);
}

void InputManager::SetAnrObserver(std::shared_ptr<IAnrObserver> observer)
{
    InputMgrImpl.SetAnrObserver(observer);
}

int32_t InputManager::SetPointerStyle(int32_t windowId, PointerStyle pointerStyle, bool isUiExtension)
{
    return InputMgrImpl.SetPointerStyle(windowId, pointerStyle, isUiExtension);
}

int32_t InputManager::GetPointerStyle(int32_t windowId, PointerStyle &pointerStyle, bool isUiExtension)
{
    return InputMgrImpl.GetPointerStyle(windowId, pointerStyle, isUiExtension);
}

int32_t InputManager::GetFunctionKeyState(int32_t funcKey, bool &state)
{
    return InputMgrImpl.GetFunctionKeyState(funcKey, state);
}

int32_t InputManager::SetFunctionKeyState(int32_t funcKey, bool enable)
{
    return InputMgrImpl.SetFunctionKeyState(funcKey, enable);
}

int32_t InputManager::SetPointerLocation(int32_t x, int32_t y, int32_t displayId)
{
    return InputMgrImpl.SetPointerLocation(x, y, displayId);
}

int32_t InputManager::EnterCaptureMode(int32_t windowId)
{
    return InputMgrImpl.EnterCaptureMode(windowId);
}

int32_t InputManager::LeaveCaptureMode(int32_t windowId)
{
    return InputMgrImpl.LeaveCaptureMode(windowId);
}

int32_t InputManager::AppendExtraData(const ExtraData& extraData)
{
    return InputMgrImpl.AppendExtraData(extraData);
}

int32_t InputManager::EnableInputDevice(bool enable)
{
    return InputMgrImpl.EnableInputDevice(enable);
}

int32_t InputManager::AddVirtualInputDevice(std::shared_ptr<InputDevice> device, int32_t &deviceId)
{
    return InputMgrImpl.AddVirtualInputDevice(device, deviceId);
}

int32_t InputManager::RemoveVirtualInputDevice(int32_t deviceId)
{
    return InputMgrImpl.RemoveVirtualInputDevice(deviceId);
}

int32_t InputManager::SetKeyDownDuration(const std::string& businessId, int32_t delay)
{
    return InputMgrImpl.SetKeyDownDuration(businessId, delay);
}

int32_t InputManager::SetKeyboardRepeatDelay(int32_t delay)
{
    return InputMgrImpl.SetKeyboardRepeatDelay(delay);
}

int32_t InputManager::SetKeyboardRepeatRate(int32_t rate)
{
    return InputMgrImpl.SetKeyboardRepeatRate(rate);
}

int32_t InputManager::GetKeyboardRepeatDelay(std::function<void(int32_t)> callback)
{
    return InputMgrImpl.GetKeyboardRepeatDelay(callback);
}

int32_t InputManager::GetKeyboardRepeatRate(std::function<void(int32_t)> callback)
{
    return InputMgrImpl.GetKeyboardRepeatRate(callback);
}

#ifdef OHOS_BUILD_ENABLE_SECURITY_COMPONENT
void InputManager::SetEnhanceConfig(uint8_t *cfg, uint32_t cfgLen)
{
    InputMgrImpl.SetEnhanceConfig(cfg, cfgLen);
}
#endif // OHOS_BUILD_ENABLE_SECURITY_COMPONENT

int32_t InputManager::SetTouchpadScrollSwitch(bool switchFlag)
{
    return InputMgrImpl.SetTouchpadScrollSwitch(switchFlag);
}

int32_t InputManager::GetTouchpadScrollSwitch(bool &switchFlag)
{
    return InputMgrImpl.GetTouchpadScrollSwitch(switchFlag);
}
int32_t InputManager::SetTouchpadScrollDirection(bool state)
{
    return InputMgrImpl.SetTouchpadScrollDirection(state);
}

int32_t InputManager::GetTouchpadScrollDirection(bool &state)
{
    return InputMgrImpl.GetTouchpadScrollDirection(state);
}
int32_t InputManager::SetTouchpadTapSwitch(bool switchFlag)
{
    return InputMgrImpl.SetTouchpadTapSwitch(switchFlag);
}

int32_t InputManager::GetTouchpadTapSwitch(bool &switchFlag)
{
    return InputMgrImpl.GetTouchpadTapSwitch(switchFlag);
}

int32_t InputManager::SetTouchpadPointerSpeed(int32_t speed)
{
    return InputMgrImpl.SetTouchpadPointerSpeed(speed);
}

int32_t InputManager::GetTouchpadPointerSpeed(int32_t &speed)
{
    return InputMgrImpl.GetTouchpadPointerSpeed(speed);
}

int32_t InputManager::GetTouchpadCDG(TouchpadCDG &touchpadCDG)
{
    return InputMgrImpl.GetTouchpadCDG(touchpadCDG);
}

int32_t InputManager::SetTouchpadPinchSwitch(bool switchFlag)
{
    return InputMgrImpl.SetTouchpadPinchSwitch(switchFlag);
}

int32_t InputManager::GetTouchpadPinchSwitch(bool &switchFlag)
{
    return InputMgrImpl.GetTouchpadPinchSwitch(switchFlag);
}

int32_t InputManager::SetTouchpadSwipeSwitch(bool switchFlag)
{
    return InputMgrImpl.SetTouchpadSwipeSwitch(switchFlag);
}

int32_t InputManager::GetTouchpadSwipeSwitch(bool &switchFlag)
{
    return InputMgrImpl.GetTouchpadSwipeSwitch(switchFlag);
}

int32_t InputManager::SetTouchpadRightClickType(int32_t type)
{
    return InputMgrImpl.SetTouchpadRightClickType(type);
}

int32_t InputManager::GetTouchpadRightClickType(int32_t &type)
{
    return InputMgrImpl.GetTouchpadRightClickType(type);
}

int32_t InputManager::SetTouchpadRotateSwitch(bool rotateSwitch)
{
    return InputMgrImpl.SetTouchpadRotateSwitch(rotateSwitch);
}

int32_t InputManager::GetTouchpadRotateSwitch(bool &rotateSwitch)
{
    return InputMgrImpl.GetTouchpadRotateSwitch(rotateSwitch);
}

int32_t InputManager::SetTouchpadDoubleTapAndDragState(bool switchFlag)
{
    return InputMgrImpl.SetTouchpadDoubleTapAndDragState(switchFlag);
}

int32_t InputManager::GetTouchpadDoubleTapAndDragState(bool &switchFlag)
{
    return InputMgrImpl.GetTouchpadDoubleTapAndDragState(switchFlag);
}

int32_t InputManager::EnableHardwareCursorStats(bool enable)
{
    return InputMgrImpl.EnableHardwareCursorStats(enable);
}

int32_t InputManager::GetHardwareCursorStats(uint32_t &frameCount, uint32_t &vsyncCount)
{
    return InputMgrImpl.GetHardwareCursorStats(frameCount, vsyncCount);
}

int32_t InputManager::GetPointerSnapshot(void *pixelMapPtr)
{
    return InputMgrImpl.GetPointerSnapshot(pixelMapPtr);
}

int32_t InputManager::SetTouchpadScrollRows(int32_t rows)
{
    return InputMgrImpl.SetTouchpadScrollRows(rows);
}

int32_t InputManager::GetTouchpadScrollRows(int32_t &rows)
{
    return InputMgrImpl.GetTouchpadScrollRows(rows);
}

void InputManager::ClearWindowPointerStyle(int32_t pid, int32_t windowId)
{
    InputMgrImpl.ClearWindowPointerStyle(pid, windowId);
}

void InputManager::SetNapStatus(int32_t pid, int32_t uid, std::string bundleName, int32_t napStatus)
{
    InputMgrImpl.SetNapStatus(pid, uid, bundleName, napStatus);
}

int32_t InputManager::SetShieldStatus(int32_t shieldMode, bool isShield)
{
    return InputMgrImpl.SetShieldStatus(shieldMode, isShield);
}

int32_t InputManager::GetShieldStatus(int32_t shieldMode, bool &isShield)
{
    return InputMgrImpl.GetShieldStatus(shieldMode, isShield);
}

void InputManager::AddServiceWatcher(std::shared_ptr<IInputServiceWatcher> watcher)
{
    InputMgrImpl.AddServiceWatcher(watcher);
}

void InputManager::RemoveServiceWatcher(std::shared_ptr<IInputServiceWatcher> watcher)
{
    InputMgrImpl.RemoveServiceWatcher(watcher);
}

int32_t InputManager::MarkProcessed(int32_t eventId, int64_t actionTime, bool enable)
{
    LogTracer lt(eventId, 0, 0);
    if (enable) {
        return InputMgrImpl.MarkProcessed(eventId, actionTime);
    }
    MMI_HILOGD("Skip MarkProcessed eventId:%{public}d", eventId);
    return RET_OK;
}

int32_t InputManager::GetKeyState(std::vector<int32_t> &pressedKeys, std::map<int32_t, int32_t> &specialKeysState)
{
    return InputMgrImpl.GetKeyState(pressedKeys, specialKeysState);
}

void InputManager::Authorize(bool isAuthorize)
{
    InputMgrImpl.Authorize(isAuthorize);
}

int32_t InputManager::HasIrEmitter(bool &hasIrEmitter)
{
    return InputMgrImpl.HasIrEmitter(hasIrEmitter);
}

int32_t InputManager::GetInfraredFrequencies(std::vector<InfraredFrequency>& requencys)
{
    return InputMgrImpl.GetInfraredFrequencies(requencys);
}

int32_t InputManager::TransmitInfrared(int64_t number, std::vector<int64_t>& pattern)
{
    return InputMgrImpl.TransmitInfrared(number, pattern);
}

#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
int32_t InputManager::CreateVKeyboardDevice(sptr<IRemoteObject> &vkeyboardDevice)
{
    return InputMgrImpl.CreateVKeyboardDevice(vkeyboardDevice);
}
#endif // OHOS_BUILD_ENABLE_VKEYBOARD

int32_t InputManager::SetCurrentUser(int32_t userId)
{
    return InputMgrImpl.SetCurrentUser(userId);
}

int32_t InputManager::SetMoveEventFilters(bool flag)
{
    return InputMgrImpl.SetMoveEventFilters(flag);
}

int32_t InputManager::SetTouchpadThreeFingersTapSwitch(bool switchFlag)
{
    return InputMgrImpl.SetTouchpadThreeFingersTapSwitch(switchFlag);
}

int32_t InputManager::GetTouchpadThreeFingersTapSwitch(bool &switchFlag)
{
    return InputMgrImpl.GetTouchpadThreeFingersTapSwitch(switchFlag);
}

int32_t InputManager::GetWinSyncBatchSize(int32_t maxAreasCount, int32_t displayCount)
{
    return InputMgrImpl.GetWinSyncBatchSize(maxAreasCount, displayCount);
}

int32_t InputManager::AncoAddConsumer(std::shared_ptr<IAncoConsumer> consumer)
{
    return InputMgrImpl.AncoAddChannel(consumer);
}

int32_t InputManager::AncoRemoveConsumer(std::shared_ptr<IAncoConsumer> consumer)
{
    return InputMgrImpl.AncoRemoveChannel(consumer);
}

int32_t InputManager::SkipPointerLayer(bool isSkip)
{
    return InputMgrImpl.SkipPointerLayer(isSkip);
}

int32_t InputManager::RegisterWindowStateErrorCallback(std::function<void(int32_t, int32_t)> callback)
{
    return InputMgrImpl.RegisterWindowStateErrorCallback(callback);
}

int32_t InputManager::GetIntervalSinceLastInput(int64_t &timeInterval)
{
    return InputMgrImpl.GetIntervalSinceLastInput(timeInterval);
}

int32_t InputManager::GetAllSystemHotkeys(std::vector<std::unique_ptr<KeyOption>> &keyOptions, int32_t &count)
{
    return InputMgrImpl.GetAllSystemHotkeys(keyOptions, count);
}

int32_t InputManager::ConvertToCapiKeyAction(int32_t keyAction)
{
    return InputMgrImpl.ConvertToCapiKeyAction(keyAction);
}

int32_t InputManager::SetInputDeviceEnabled(int32_t deviceId, bool enable, std::function<void(int32_t)> callback)
{
    return InputMgrImpl.SetInputDeviceEnabled(deviceId, enable, callback);
}

int32_t InputManager::ShiftAppPointerEvent(const ShiftWindowParam &param, bool autoGenDown)
{
    HITRACE_METER_NAME(HITRACE_TAG_MULTIMODALINPUT, "shift pointer event entry");
    return InputMgrImpl.ShiftAppPointerEvent(param, autoGenDown);
}

int32_t InputManager::SetCustomCursor(int32_t windowId, CustomCursor cursor, CursorOptions options)
{
    return InputMgrImpl.SetCustomCursor(windowId, cursor, options);
}

int32_t InputManager::CheckKnuckleEvent(float pointX, float pointY, bool &isKnuckleType)
{
    return InputMgrImpl.CheckKnuckleEvent(pointX, pointY, isKnuckleType);
}

void InputManager::SetMultiWindowScreenId(uint64_t screenId, uint64_t displayNodeScreenId)
{
    InputMgrImpl.SetMultiWindowScreenId(screenId, displayNodeScreenId);
}

int32_t InputManager::SubscribeInputActive(std::shared_ptr<IInputEventConsumer> inputEventConsumer, int64_t interval)
{
    return InputMgrImpl.SubscribeInputActive(inputEventConsumer, interval);
}

void InputManager::UnsubscribeInputActive(int32_t subscribeId)
{
    InputMgrImpl.UnsubscribeInputActive(subscribeId);
}

int32_t InputManager::SetInputDeviceConsumer(const std::vector<std::string>& deviceNames,
    std::shared_ptr<IInputEventConsumer> consumer)
{
    return InputMgrImpl.SetInputDeviceConsumer(deviceNames, consumer);
}
} // namespace MMI
} // namespace OHOS
