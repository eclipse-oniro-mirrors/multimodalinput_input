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

#include "event_log_helper.h"
#include "input_manager.h"
#include "js_register_util.h"
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
} // namespace

static void GetInjectionEventData(napi_env env, std::shared_ptr<KeyEvent> keyEvent, napi_value keyHandle)
{
    CHKPV(keyEvent);
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
        if (!EventLogHelper::IsBetaVersion()) {
            MMI_HILOGE("keyCode is less 0, can not process");
        } else {
            MMI_HILOGE("keyCode is less 0, can not process");
        }
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
    int32_t screenId = -1;
    if (GetNamedPropertyInt32(env, touchHandle, "screenId", screenId, false) != RET_OK) {
        MMI_HILOGE("Get screenId failed");
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
    pointerEvent->SetTargetDisplayId(screenId);
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