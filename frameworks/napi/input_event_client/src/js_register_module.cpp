/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "js_register_module.h"

#include <cinttypes>

#include "input_manager.h"
#include "js_register_util.h"
#include "js_joystick_event.h"
#include "napi_constants.h"
#include "util_napi.h"
#include "util_napi_error.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "JSRegisterModule"

namespace OHOS {
namespace MMI {
namespace {
constexpr int32_t JS_CALLBACK_MOUSE_BUTTON_MIDDLE { 1 };
constexpr int32_t JS_CALLBACK_MOUSE_BUTTON_RIGHT { 2 };

std::map<JsJoystickEvent::Axis, PointerEvent::AxisType> g_joystickAxisType = {
    { JsJoystickEvent::Axis::ABS_X, PointerEvent::AXIS_TYPE_ABS_X },
    { JsJoystickEvent::Axis::ABS_Y, PointerEvent::AXIS_TYPE_ABS_Y },
    { JsJoystickEvent::Axis::ABS_Z, PointerEvent::AXIS_TYPE_ABS_Z },
    { JsJoystickEvent::Axis::ABS_RZ, PointerEvent::AXIS_TYPE_ABS_RZ },
    { JsJoystickEvent::Axis::ABS_GAS, PointerEvent::AXIS_TYPE_ABS_GAS },
    { JsJoystickEvent::Axis::ABS_BRAKE, PointerEvent::AXIS_TYPE_ABS_BRAKE },
    { JsJoystickEvent::Axis::ABS_HAT0X, PointerEvent::AXIS_TYPE_ABS_HAT0X },
    { JsJoystickEvent::Axis::ABS_HAT0Y, PointerEvent::AXIS_TYPE_ABS_HAT0Y },
    { JsJoystickEvent::Axis::ABS_THROTTLE, PointerEvent::AXIS_TYPE_ABS_THROTTLE }
};

std::map<JsJoystickEvent::Button, int32_t> g_joystickButtonType = {
    { JsJoystickEvent::Button::BUTTON_TL2, PointerEvent::JOYSTICK_BUTTON_TL2 },
    { JsJoystickEvent::Button::BUTTON_TR2, PointerEvent::JOYSTICK_BUTTON_TR2 },
    { JsJoystickEvent::Button::BUTTON_TL, PointerEvent::JOYSTICK_BUTTON_TL },
    { JsJoystickEvent::Button::BUTTON_TR, PointerEvent::JOYSTICK_BUTTON_TR },
    { JsJoystickEvent::Button::BUTTON_WEST, PointerEvent::JOYSTICK_BUTTON_WEST },
    { JsJoystickEvent::Button::BUTTON_SOUTH, PointerEvent::JOYSTICK_BUTTON_SOUTH },
    { JsJoystickEvent::Button::BUTTON_NORTH, PointerEvent::JOYSTICK_BUTTON_NORTH },
    { JsJoystickEvent::Button::BUTTON_EAST, PointerEvent::JOYSTICK_BUTTON_EAST },
    { JsJoystickEvent::Button::BUTTON_START, PointerEvent::JOYSTICK_BUTTON_START },
    { JsJoystickEvent::Button::BUTTON_SELECT, PointerEvent::JOYSTICK_BUTTON_SELECT },
    { JsJoystickEvent::Button::BUTTON_HOMEPAGE, PointerEvent::JOYSTICK_BUTTON_HOMEPAGE },
    { JsJoystickEvent::Button::BUTTON_THUMBL, PointerEvent::JOYSTICK_BUTTON_THUMBL },
    { JsJoystickEvent::Button::BUTTON_THUMBR, PointerEvent::JOYSTICK_BUTTON_THUMBR },
    { JsJoystickEvent::Button::BUTTON_TRIGGER, PointerEvent::JOYSTICK_BUTTON_TRIGGER },
    { JsJoystickEvent::Button::BUTTON_THUMB, PointerEvent::JOYSTICK_BUTTON_THUMB },
    { JsJoystickEvent::Button::BUTTON_THUMB2, PointerEvent::JOYSTICK_BUTTON_THUMB2 },
    { JsJoystickEvent::Button::BUTTON_TOP, PointerEvent::JOYSTICK_BUTTON_TOP },
    { JsJoystickEvent::Button::BUTTON_TOP2, PointerEvent::JOYSTICK_BUTTON_TOP2 },
    { JsJoystickEvent::Button::BUTTON_PINKIE, PointerEvent::JOYSTICK_BUTTON_PINKIE },
    { JsJoystickEvent::Button::BUTTON_BASE, PointerEvent::JOYSTICK_BUTTON_BASE },
    { JsJoystickEvent::Button::BUTTON_BASE2, PointerEvent::JOYSTICK_BUTTON_BASE2 },
    { JsJoystickEvent::Button::BUTTON_BASE3, PointerEvent::JOYSTICK_BUTTON_BASE3 },
    { JsJoystickEvent::Button::BUTTON_BASE4, PointerEvent::JOYSTICK_BUTTON_BASE4 },
    { JsJoystickEvent::Button::BUTTON_BASE5, PointerEvent::JOYSTICK_BUTTON_BASE5 },
    { JsJoystickEvent::Button::BUTTON_BASE6, PointerEvent::JOYSTICK_BUTTON_BASE6 },
    { JsJoystickEvent::Button::BUTTON_DEAD, PointerEvent::JOYSTICK_BUTTON_DEAD },
    { JsJoystickEvent::Button::BUTTON_C, PointerEvent::JOYSTICK_BUTTON_C },
    { JsJoystickEvent::Button::BUTTON_Z, PointerEvent::JOYSTICK_BUTTON_Z },
    { JsJoystickEvent::Button::BUTTON_MODE, PointerEvent::JOYSTICK_BUTTON_MODE }
};
} // namespace

static void GetInjectionEventData(napi_env env, std::shared_ptr<KeyEvent> keyEvent, napi_value keyHandle)
{
    keyEvent->SetRepeat(true);
    bool isPressed = false;
    if (GetNamedPropertyBool(env, keyHandle, "isPressed", isPressed) != RET_OK) {
        MMI_HILOGE("Get isPressed failed");
    }
    bool isIntercepted = false;
    if (GetNamedPropertyBool(env, keyHandle, "isIntercepted", isIntercepted) != RET_OK) {
        MMI_HILOGE("Get isIntercepted failed");
    }
    keyEvent->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT);
    int32_t keyDownDuration = 0;
    if (GetNamedPropertyInt32(env, keyHandle, "keyDownDuration", keyDownDuration) != RET_OK) {
        MMI_HILOGE("Get keyDownDuration failed");
    }
    if (keyDownDuration < 0) {
        MMI_HILOGE("keyDownDuration:%{public}d is less 0, can not process", keyDownDuration);
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "keyDownDuration must be greater than or equal to 0");
    }
    int32_t keyCode = 0;
    if (GetNamedPropertyInt32(env, keyHandle, "keyCode", keyCode) != RET_OK) {
        MMI_HILOGE("Get keyCode failed");
    }
    if (keyCode < 0) {
        MMI_HILOGE("keyCode:%{public}d is less 0, can not process", keyCode);
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "keyCode must be greater than or equal to 0");
    }
    keyEvent->SetKeyCode(keyCode);
    auto keyAction = isPressed ? KeyEvent::KEY_ACTION_DOWN : KeyEvent::KEY_ACTION_UP;
    keyEvent->SetKeyAction(keyAction);
    KeyEvent::KeyItem item;
    item.SetKeyCode(keyCode);
    item.SetPressed(isPressed);
    item.SetDownTime(static_cast<int64_t>(keyDownDuration));
    keyEvent->AddKeyItem(item);
    InputManager::GetInstance()->SimulateInputEvent(keyEvent);
    std::this_thread::sleep_for(std::chrono::milliseconds(keyDownDuration));
}

static napi_value InjectEvent(napi_env env, napi_callback_info info)
{
    CALL_DEBUG_ENTER;
    napi_value result = nullptr;
    size_t argc = 1;
    napi_value argv[1] = { 0 };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 1) {
        MMI_HILOGE("Parameter number error");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "parameter number error");
        return nullptr;
    }
    napi_valuetype tmpType = napi_undefined;
    CHKRP(napi_typeof(env, argv[0], &tmpType), TYPEOF);
    if (tmpType != napi_object) {
        MMI_HILOGE("KeyEvent is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "KeyEvent", "object");
        return nullptr;
    }
    napi_value keyHandle = nullptr;
    CHKRP(napi_get_named_property(env, argv[0], "KeyEvent", &keyHandle), GET_NAMED_PROPERTY);
    if (keyHandle == nullptr) {
        MMI_HILOGE("KeyEvent is nullptr");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "KeyEvent not found");
        return nullptr;
    }
    CHKRP(napi_typeof(env, keyHandle, &tmpType), TYPEOF);
    if (tmpType != napi_object) {
        MMI_HILOGE("KeyEvent is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "KeyEvent", "object");
        return nullptr;
    }
    auto keyEvent = KeyEvent::Create();
    CHKPP(keyEvent);
    GetInjectionEventData(env, keyEvent, keyHandle);
    CHKRP(napi_create_int32(env, 0, &result), CREATE_INT32);
    return result;
}

static napi_value InjectKeyEvent(napi_env env, napi_callback_info info)
{
    CALL_DEBUG_ENTER;
    napi_value result = nullptr;
    size_t argc = 1;
    napi_value argv[1] = { 0 };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 1) {
        MMI_HILOGE("Parameter number error");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "parameter number error");
        return nullptr;
    }
    napi_valuetype tmpType = napi_undefined;
    CHKRP(napi_typeof(env, argv[0], &tmpType), TYPEOF);
    if (tmpType != napi_object) {
        MMI_HILOGE("keyEvent is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "keyEvent", "object");
        return nullptr;
    }
    napi_value keyHandle = nullptr;
    CHKRP(napi_get_named_property(env, argv[0], "keyEvent", &keyHandle), GET_NAMED_PROPERTY);
    if (keyHandle == nullptr) {
        MMI_HILOGE("keyEvent is nullptr");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "keyEvent not found");
        return nullptr;
    }
    CHKRP(napi_typeof(env, keyHandle, &tmpType), TYPEOF);
    if (tmpType != napi_object) {
        MMI_HILOGE("keyEvent is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "keyEvent", "object");
        return nullptr;
    }
    auto keyEvent = KeyEvent::Create();
    CHKPP(keyEvent);
    GetInjectionEventData(env, keyEvent, keyHandle);
    CHKRP(napi_create_int32(env, 0, &result), CREATE_INT32);
    return result;
}

static void HandleMouseButton(napi_env env, napi_value mouseHandle, std::shared_ptr<PointerEvent> pointerEvent)
{
    int32_t button = 0;
    if (GetNamedPropertyInt32(env, mouseHandle, "button", button) != RET_OK) {
        MMI_HILOGE("Get button failed");
    }
    if (button < 0) {
        MMI_HILOGE("button:%{public}d is less 0, can not process", button);
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "button must be greater than or equal to 0");
    }

    switch (button) {
        case JS_CALLBACK_MOUSE_BUTTON_MIDDLE:
            button = PointerEvent::MOUSE_BUTTON_MIDDLE;
            break;
        case JS_CALLBACK_MOUSE_BUTTON_RIGHT:
            button = PointerEvent::MOUSE_BUTTON_RIGHT;
            break;
        default:
            MMI_HILOGE("button:%{public}d is unknown", button);
            break;
    }
    pointerEvent->SetButtonId(button);
    pointerEvent->SetButtonPressed(button);
}

static void HandleMouseAction(napi_env env, napi_value mouseHandle,
    std::shared_ptr<PointerEvent> pointerEvent, PointerEvent::PointerItem &item)
{
    int32_t action = 0;
    if (GetNamedPropertyInt32(env, mouseHandle, "action", action) != RET_OK) {
        MMI_HILOGE("Get action failed");
        return;
    }
    switch (action) {
        case JS_CALLBACK_MOUSE_ACTION_MOVE:
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
            break;
        case JS_CALLBACK_MOUSE_ACTION_BUTTON_DOWN:
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
            item.SetPressed(true);
            break;
        case JS_CALLBACK_MOUSE_ACTION_BUTTON_UP:
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
            item.SetPressed(false);
            break;
        case JS_CALLBACK_POINTER_ACTION_DOWN:
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
            item.SetPressed(true);
            break;
        case JS_CALLBACK_POINTER_ACTION_UP:
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
            item.SetPressed(false);
            break;
        default:
            MMI_HILOGE("action:%{public}d is unknown", action);
            break;
    }
    if (action == JS_CALLBACK_MOUSE_ACTION_BUTTON_DOWN || action == JS_CALLBACK_MOUSE_ACTION_BUTTON_UP) {
        HandleMouseButton(env, mouseHandle, pointerEvent);
    }
}

static void HandleMousePropertyInt32(napi_env env, napi_value mouseHandle,
    std::shared_ptr<PointerEvent> pointerEvent, PointerEvent::PointerItem &item)
{
    int32_t screenX = 0;
    if (GetNamedPropertyInt32(env, mouseHandle, "screenX", screenX) != RET_OK) {
        MMI_HILOGE("Get screenX failed");
    }
    int32_t screenY = 0;
    if (GetNamedPropertyInt32(env, mouseHandle, "screenY", screenY) != RET_OK) {
        MMI_HILOGE("Get screenY failed");
    }
    int32_t toolType = 0;
    if (GetNamedPropertyInt32(env, mouseHandle, "toolType", toolType) != RET_OK) {
        MMI_HILOGE("Get toolType failed");
    }
    if (toolType < 0) {
        MMI_HILOGE("toolType:%{public}d is less 0, can not process", toolType);
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "toolType must be greater than or equal to 0");
    }
    pointerEvent->SetSourceType(toolType);
    item.SetPointerId(0);
    item.SetDisplayX(screenX);
    item.SetDisplayY(screenY);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
}

static napi_value InjectMouseEvent(napi_env env, napi_callback_info info)
{
    CALL_DEBUG_ENTER;
    napi_value result = nullptr;
    size_t argc = 1;
    napi_value argv[1] = { 0 };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 1) {
        MMI_HILOGE("Parameter number error");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "parameter number error");
        return nullptr;
    }
    napi_valuetype tmpType = napi_undefined;
    CHKRP(napi_typeof(env, argv[0], &tmpType), TYPEOF);
    if (tmpType != napi_object) {
        MMI_HILOGE("MouseEvent is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "mouseEvent", "object");
        return nullptr;
    }
    napi_value mouseHandle = nullptr;
    CHKRP(napi_get_named_property(env, argv[0], "mouseEvent", &mouseHandle), GET_NAMED_PROPERTY);
    if (mouseHandle == nullptr) {
        MMI_HILOGE("MouseEvent is nullptr");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "MouseEvent not found");
        return nullptr;
    }
    CHKRP(napi_typeof(env, mouseHandle, &tmpType), TYPEOF);
    if (tmpType != napi_object) {
        MMI_HILOGE("MouseEvent is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "mouseEvent", "object");
        return nullptr;
    }
    auto pointerEvent = PointerEvent::Create();
    PointerEvent::PointerItem item;
    CHKPP(pointerEvent);

    HandleMouseAction(env, mouseHandle, pointerEvent, item);
    HandleMousePropertyInt32(env, mouseHandle, pointerEvent, item);
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
    CHKRP(napi_create_int32(env, 0, &result), CREATE_INT32);
    return result;
}

static int32_t HandleTouchAction(napi_env env, napi_value touchHandle,
    std::shared_ptr<PointerEvent> pointerEvent, PointerEvent::PointerItem &item)
{
    int32_t action = 0;
    if (GetNamedPropertyInt32(env, touchHandle, "action", action) != RET_OK) {
        MMI_HILOGE("Get action failed");
        return RET_ERR;
    }
    switch (action) {
        case JS_CALLBACK_TOUCH_ACTION_DOWN:
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
            item.SetPressed(true);
            break;
        case JS_CALLBACK_TOUCH_ACTION_MOVE:
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
            break;
        case JS_CALLBACK_TOUCH_ACTION_UP:
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
            item.SetPressed(false);
            break;
        default:
            MMI_HILOGE("action:%{public}d is unknown", action);
            break;
    }
    return action;
}

static napi_value HandleTouchProperty(napi_env env, napi_value touchHandle)
{
    napi_value touchProperty = nullptr;
    if (napi_get_named_property(env, touchHandle, "touch", &touchProperty) != RET_OK) {
        MMI_HILOGE("Get touch failed");
        return nullptr;
    }
    if (touchProperty == nullptr) {
        MMI_HILOGE("Touch value is null");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Invalid touch");
        return nullptr;
    }
    napi_valuetype tmpType = napi_undefined;
    if (napi_typeof(env, touchProperty, &tmpType) != napi_ok) {
        MMI_HILOGE("Call napi_typeof failed");
        return nullptr;
    }
    if (tmpType != napi_object) {
        MMI_HILOGE("touch is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "touch", "object");
        return nullptr;
    }
    return touchProperty;
}

static void HandleTouchPropertyInt32(napi_env env, napi_value touchHandle,
    std::shared_ptr<PointerEvent> pointerEvent, PointerEvent::PointerItem &item, int32_t action)
{
    int32_t sourceType = 0;
    if (GetNamedPropertyInt32(env, touchHandle, "sourceType", sourceType) != RET_OK) {
        MMI_HILOGE("Get sourceType failed");
    }
    if (sourceType < 0) {
        MMI_HILOGE("sourceType:%{public}d is less 0, can not process", sourceType);
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "sourceType must be greater than or equal to 0");
    }
    if (sourceType == TOUCH_SCREEN || sourceType == PEN) {
        sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    }
    napi_value touchProperty = HandleTouchProperty(env, touchHandle);
    CHKPV(touchProperty);
    int32_t screenX = 0;
    if (GetNamedPropertyInt32(env, touchProperty, "screenX", screenX) != RET_OK) {
        MMI_HILOGE("Get screenX failed");
    }
    int32_t screenY = 0;
    if (GetNamedPropertyInt32(env, touchProperty, "screenY", screenY) != RET_OK) {
        MMI_HILOGE("Get screenY failed");
    }
    int64_t pressedTime;
    if (GetNamedPropertyInt64(env, touchProperty, "pressedTime", pressedTime) != RET_OK) {
        MMI_HILOGE("Get pressed time failed");
    }
    int32_t toolType = 0;
    if (GetNamedPropertyInt32(env, touchProperty, "toolType", toolType) != RET_OK) {
        MMI_HILOGE("Get toolType failed");
    }
    double pressure;
    if (GetNamedPropertyDouble(env, touchProperty, "pressure", pressure) != RET_OK) {
        MMI_HILOGE("Get pressure failed");
    }
    item.SetDisplayX(screenX);
    item.SetDisplayY(screenY);
    item.SetPointerId(0);
    item.SetToolType(toolType);
    item.SetPressure(pressure);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetSourceType(sourceType);
    pointerEvent->SetActionTime(pressedTime);
    if ((action == JS_CALLBACK_TOUCH_ACTION_MOVE) || (action == JS_CALLBACK_TOUCH_ACTION_UP)) {
        pointerEvent->UpdatePointerItem(0, item);
    }
}

static napi_value InjectTouchEvent(napi_env env, napi_callback_info info)
{
    CALL_DEBUG_ENTER;
    napi_value result = nullptr;
    size_t argc = 1;
    napi_value argv[1] = { 0 };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 1) {
        MMI_HILOGE("Parameter number error");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "parameter number error");
        return nullptr;
    }
    napi_valuetype tmpType = napi_undefined;
    CHKRP(napi_typeof(env, argv[0], &tmpType), TYPEOF);
    if (tmpType != napi_object) {
        MMI_HILOGE("TouchEvent is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "touchEvent", "object");
        return nullptr;
    }
    napi_value touchHandle = nullptr;
    CHKRP(napi_get_named_property(env, argv[0], "touchEvent", &touchHandle), GET_NAMED_PROPERTY);
    if (touchHandle == nullptr) {
        MMI_HILOGE("TouchEvent is nullptr");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "TouchEvent not found");
        return nullptr;
    }
    CHKRP(napi_typeof(env, touchHandle, &tmpType), TYPEOF);
    if (tmpType != napi_object) {
        MMI_HILOGE("TouchEvent is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "touchEvent", "object");
        return nullptr;
    }
    auto pointerEvent = PointerEvent::Create();
    PointerEvent::PointerItem item;
    CHKPP(pointerEvent);

    int32_t action = HandleTouchAction(env, touchHandle, pointerEvent, item);
    HandleTouchPropertyInt32(env, touchHandle, pointerEvent, item, action);
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
    CHKRP(napi_create_int32(env, 0, &result), CREATE_INT32);
    return result;
}

static int32_t HandleJoystickButton(napi_env env, napi_value joystickHandle, std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    int32_t button = -1;
    if (GetNamedPropertyInt32(env, joystickHandle, "button", button) != RET_OK || button < 0) {
        MMI_HILOGE("Get button error");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "get button error");
        return RET_ERR;
    }

    for (const auto &item : g_joystickButtonType) {
        if (button == static_cast<int32_t>(item.first)) {
            pointerEvent->SetButtonId(item.second);
            break;
        }
    }
    return RET_OK;
}

static int32_t HandleJoystickAction(napi_env env, napi_value joystickHandle, std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    int32_t action = 0;
    if (GetNamedPropertyInt32(env, joystickHandle, "action", action) != RET_OK) {
        MMI_HILOGE("Get action failed");
        return RET_ERR;
    }
    int32_t buttonId = pointerEvent->GetButtonId();
    if (buttonId < 0) {
        MMI_HILOGE("buttonId:%{public}d is less 0, can not process", buttonId);
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "buttonId must be greater than or equal to 0");
        return RET_ERR;
    }

    switch (action) {
        case static_cast<int32_t>(JsJoystickEvent::Action::BUTTON_DOWN): {
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
            pointerEvent->SetButtonPressed(buttonId);
            break;
        }
        case static_cast<int32_t>(JsJoystickEvent::Action::BUTTON_UP): {
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
            pointerEvent->DeleteReleaseButton(buttonId);
            break;
        }
        case static_cast<int32_t>(JsJoystickEvent::Action::ABS_UPDATE): {
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
            break;
        }
        default: {
            MMI_HILOGE("action:%{public}d is unknown", action);
            break;
        }
    }

    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_JOYSTICK);
    return RET_OK;
}

static int32_t HandleJoystickAxes(napi_env env, napi_value joystickHandle, std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    napi_value axesArray = {};
    pointerEvent->ClearAxisValue();
    if (napi_get_named_property(env, joystickHandle, "axes", &axesArray) != napi_ok) {
        MMI_HILOGE("Call napi_get_named_property failed");
        return RET_ERR;
    }
    CHKPR(axesArray, RET_ERR);

    uint32_t arrayLength = 0;
    if (napi_get_array_length(env, axesArray, &arrayLength) != napi_ok) {
        MMI_HILOGE("Call napi_get_array_length failed");
        return RET_ERR;
    }

    for (uint32_t i = 0; i < arrayLength; i++) {
        napi_value axisObject;
        if (napi_get_element(env, axesArray, i, &axisObject) != napi_ok) {
            MMI_HILOGE("Call napi_get_element failed");
            return RET_ERR;
        }

        int32_t axis = 0;
        if (GetNamedPropertyInt32(env, axisObject, "axis", axis) != RET_OK) {
            MMI_HILOGE("Get axis failed");
            return RET_ERR;
        }
        double axisValue;
        if (GetNamedPropertyDouble(env, axisObject, "value", axisValue) != RET_OK) {
            MMI_HILOGE("Get axisValue failed");
            return RET_ERR;
        }
        for (const auto &item : g_joystickAxisType) {
            if (axis == static_cast<int32_t>(item.first)) {
                pointerEvent->SetAxisValue(item.second, axisValue);
            }
        }
    }
    return RET_OK;
}

static napi_value InjectJoystickEvent(napi_env env, napi_callback_info info)
{
    CALL_DEBUG_ENTER;
    napi_value result = nullptr;
    size_t argc = 1;
    napi_value argv[1] = { 0 };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 1) {
        MMI_HILOGE("Parameter number error");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "parameter number error");
        return nullptr;
    }
    napi_valuetype tmpType = napi_undefined;
    CHKRP(napi_typeof(env, argv[0], &tmpType), TYPEOF);
    if (tmpType != napi_object) {
        MMI_HILOGE("joystickEvent is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "joystickEvent", "object");
        return nullptr;
    }
    napi_value joystickHandle = nullptr;
    CHKRP(napi_get_named_property(env, argv[0], "joystickEvent", &joystickHandle), GET_NAMED_PROPERTY);
    if (joystickHandle == nullptr) {
        MMI_HILOGE("joystickEvent is nullptr");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "joysticEvent not found");
        return nullptr;
    }
    CHKRP(napi_typeof(env, joystickHandle, &tmpType), TYPEOF);
    if (tmpType != napi_object) {
        MMI_HILOGE("joystickEvent is not napi_object");
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "joystickEvent", "object");
        return nullptr;
    }
    auto pointerEvent = PointerEvent::Create();
    CHKPP(pointerEvent);

    HandleJoystickButton(env, joystickHandle, pointerEvent);
    HandleJoystickAction(env, joystickHandle, pointerEvent);
    HandleJoystickAxes(env, joystickHandle, pointerEvent);
    InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
    CHKRP(napi_create_int32(env, 0, &result), CREATE_INT32);
    return result;
}

static napi_value PermitInjection(napi_env env, napi_callback_info info)
{
    CALL_DEBUG_ENTER;
    napi_value result = nullptr;
    size_t argc = 1;
    napi_value argv[1] = { 0 };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 1) {
        MMI_HILOGE("Parameter number error");
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "parameter number error");
        return nullptr;
    }

    bool bResult = false;
    if (!UtilNapi::TypeOf(env, argv[0], napi_boolean)) {
        THROWERR_API9(env, COMMON_PARAMETER_ERROR, "type", "boolean");
        MMI_HILOGE("The first parameter is not boolean");
        return nullptr;
    }

    CHKRP(napi_get_value_bool(env, argv[0], &bResult), GET_VALUE_BOOL);
    MMI_HILOGI("Parameter bResult:%{public}d ok", bResult);
    
    InputManager::GetInstance()->Authorize(bResult);
    CHKRP(napi_create_int32(env, 0, &result), CREATE_INT32);
    return result;
}

EXTERN_C_START
static napi_value MmiInit(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("injectEvent", InjectEvent),
        DECLARE_NAPI_FUNCTION("injectKeyEvent", InjectKeyEvent),
        DECLARE_NAPI_FUNCTION("injectMouseEvent", InjectMouseEvent),
        DECLARE_NAPI_FUNCTION("injectTouchEvent", InjectTouchEvent),
        DECLARE_NAPI_FUNCTION("injectJoystickEvent", InjectJoystickEvent),
        DECLARE_NAPI_FUNCTION("permitInjection", PermitInjection),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}
EXTERN_C_END

static napi_module mmiModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = MmiInit,
    .nm_modname = "multimodalInput.inputEventClient",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&mmiModule);
}
} // namespace MMI
} // namespace OHOS
