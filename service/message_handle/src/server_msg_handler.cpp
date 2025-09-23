/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "server_msg_handler.h"

#include "anr_manager.h"
#include "app_mgr_client.h"
#include "authorization_dialog.h"
#include "authorize_helper.h"
#include "bytrace_adapter.h"
#include "cursor_drawing_component.h"
#ifdef OHOS_BUILD_ENABLE_DFX_RADAR
#include "dfx_hisysevent.h"
#endif // OHOS_BUILD_ENABLE_DFX_RADAR
#include "display_event_monitor.h"
#include "event_log_helper.h"
#include "input_device_manager.h"
#include "input_event_handler.h"
#ifdef OHOS_BUILD_ENABLE_KEY_PRESSED_HANDLER
#include "key_monitor_manager.h"
#endif // OHOS_BUILD_ENABLE_KEY_PRESSED_HANDLER
#ifdef SHORTCUT_KEY_MANAGER_ENABLED
#include "key_shortcut_manager.h"
#endif // SHORTCUT_KEY_MANAGER_ENABLED
#include "long_press_subscriber_handler.h"
#include "libinput_adapter.h"
#include "pointer_device_manager.h"
#include "time_cost_chk.h"
#ifdef OHOS_BUILD_ENABLE_TOUCH_DRAWING
#include "touch_drawing_manager.h"
#endif // #ifdef OHOS_BUILD_ENABLE_TOUCH_DRAWING
#include "util.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_SERVER
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "ServerMsgHandler"

namespace OHOS {
namespace MMI {
namespace {
#ifdef OHOS_BUILD_ENABLE_SECURITY_COMPONENT
constexpr int32_t SECURITY_COMPONENT_SERVICE_ID = 3050;
#endif // OHOS_BUILD_ENABLE_SECURITY_COMPONENT
constexpr int32_t SEND_NOTICE_OVERTIME { 5 };
[[ maybe_unused ]] constexpr int32_t DEFAULT_POINTER_ID { 10000 };
[[ maybe_unused ]] constexpr int32_t CAST_POINTER_ID { 5000 };
const std::string PRODUCT_TYPE = system::GetParameter("const.product.devicetype", "unknown");
const std::string PRODUCT_TYPE_PC = "2in1";
[[ maybe_unused ]] constexpr int32_t WINDOW_ROTATE { 0 };
constexpr int32_t COMMON_PERMISSION_CHECK_ERROR { 201 };
constexpr int32_t CAST_INPUT_DEVICEID { 0xAAAAAAFF };
constexpr int32_t CAST_SCREEN_DEVICEID { 0xAAAAAAFE };
constexpr int32_t ANGLE_90 { 90 };
constexpr int32_t ANGLE_360 { 360 };
constexpr int32_t ERR_DEVICE_NOT_EXIST { 3900002 };
constexpr int32_t ERR_NON_INPUT_APPLICATION { 3900003 };
constexpr int32_t SIMULATE_EVENT_START_ID { 10000 };
constexpr float MM_TO_INCH { 25.4f };
constexpr int32_t SCREEN_DIAGONAL_0 { 0 };
constexpr int32_t SCREEN_DIAGONAL_8 { 8 };
constexpr int32_t SCREEN_DIAGONAL_18 { 18 };
constexpr int32_t SCREEN_DIAGONAL_27 { 27 };
constexpr int32_t SCREEN_DIAGONAL_55 { 55 };
constexpr float FACTOR_0 { 1.0f };
constexpr float FACTOR_8 { 0.7f };
constexpr float FACTOR_18 { 1.0f };
constexpr float FACTOR_27 { 1.2f };
constexpr float FACTOR_55 { 1.6f };
constexpr float FACTOR_MAX { 2.4f };
constexpr int64_t QUERY_AUTHORIZE_MAX_INTERVAL_TIME { 3000 };
constexpr uint32_t MAX_ENHANCE_CONFIG_SIZE { 1000 };
} // namespace

void ServerMsgHandler::Init(UDSServer &udsServer)
{
    udsServer_ = &udsServer;
    MsgCallback funs[] = {
        {MmiMessageId::DISPLAY_INFO, [this] (SessionPtr sess, NetPacket &pkt) {
            return this->OnDisplayInfo(sess, pkt); }},
        {MmiMessageId::WINDOW_INFO, [this] (SessionPtr sess, NetPacket &pkt) {
            return this->OnWindowGroupInfo(sess, pkt); }},
        {MmiMessageId::WINDOW_STATE_ERROR_CALLBACK, [this] (SessionPtr sess, NetPacket &pkt) {
            return this->RegisterWindowStateErrorCallback(sess, pkt); }},
#ifdef OHOS_BUILD_ENABLE_SECURITY_COMPONENT
        {MmiMessageId::SCINFO_CONFIG, [this] (SessionPtr sess, NetPacket &pkt) {
            return this->OnEnhanceConfig(sess, pkt); }},
#endif // OHOS_BUILD_ENABLE_SECURITY_COMPONENT

    };
    for (auto &it : funs) {
        if (!RegistrationEvent(it)) {
            MMI_HILOGW("Failed to register event errCode:%{public}d", EVENT_REG_FAIL);
            continue;
        }
    }
    AUTHORIZE_HELPER->Init(&clientDeathHandler_);
}

void ServerMsgHandler::OnMsgHandler(SessionPtr sess, NetPacket& pkt)
{
    CHKPV(sess);
    auto id = pkt.GetMsgId();
    TimeCostChk chk("ServerMsgHandler::OnMsgHandler", "overtime 300(us)", MAX_OVER_TIME, id);
    BytraceAdapter::StartSocketHandle(static_cast<int32_t>(id));
    auto callback = GetMsgCallback(id);
    if (callback == nullptr) {
        MMI_HILOGE("Unknown msg id:%{public}d,errCode:%{public}d", id, UNKNOWN_MSG_ID);
        return;
    }
    auto ret = (*callback)(sess, pkt);
    BytraceAdapter::StopSocketHandle();
    if (ret < 0) {
        MMI_HILOGE("Msg handling failed. id:%{public}d,errCode:%{public}d", id, ret);
    }
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
int32_t ServerMsgHandler::OnInjectKeyEvent(const std::shared_ptr<KeyEvent> keyEvent, int32_t pid, bool isNativeInject)
{
    CALL_DEBUG_ENTER;
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    keyEvent->UpdateId();
    LogTracer lt(keyEvent->GetId(), keyEvent->GetEventType(), keyEvent->GetKeyAction());
    if (isNativeInject) {
        int32_t checkReturn = NativeInjectCheck(pid);
        if (checkReturn != RET_OK) {
            return checkReturn;
        }
    }
    keyEvent->SetKeyIntention(KeyItemsTransKeyIntention(keyEvent->GetKeyItems()));
    auto inputEventNormalizeHandler = InputHandler->GetEventNormalizeHandler();
    CHKPR(inputEventNormalizeHandler, ERROR_NULL_POINTER);
    inputEventNormalizeHandler->HandleKeyEvent(keyEvent);
#ifdef SHORTCUT_KEY_RULES_ENABLED
    KEY_SHORTCUT_MGR->UpdateShortcutConsumed(keyEvent);
#endif // SHORTCUT_KEY_RULES_ENABLED
    if (EventLogHelper::IsBetaVersion() && !keyEvent->HasFlag(InputEvent::EVENT_FLAG_PRIVACY_MODE)) {
        MMI_HILOGD("Inject keyCode:%{private}d, action:%{public}d", keyEvent->GetKeyCode(), keyEvent->GetKeyAction());
    } else {
        MMI_HILOGD("Inject keyCode:%{private}d, action:%{public}d", keyEvent->GetKeyCode(), keyEvent->GetKeyAction());
    }
    return RET_OK;
}

int32_t ServerMsgHandler::OnGetFunctionKeyState(int32_t funcKey, bool &state)
{
    CALL_INFO_TRACE;
    bool hasVirtualKeyboard = false;
#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
    hasVirtualKeyboard = INPUT_DEV_MGR->HasVirtualKeyboardDevice();
#endif // OHOS_BUILD_ENABLE_VKEYBOARD
    std::vector<struct libinput_device*> input_device;
    INPUT_DEV_MGR->GetMultiKeyboardDevice(input_device);
    if (input_device.size() == 0 && !hasVirtualKeyboard) {
        MMI_HILOGW("No keyboard device is currently available");
        return ERR_DEVICE_NOT_EXIST;
    }
    const auto &keyEvent = KeyEventHdr->GetKeyEvent();
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    state = keyEvent->GetFunctionKey(funcKey);
    MMI_HILOGI("Get function key:%{public}d status as %{public}s", funcKey, state ? "open" : "close");
    return RET_OK;
}

int32_t ServerMsgHandler::OnSetFunctionKeyState(int32_t pid, int32_t funcKey, bool enable)
{
    CALL_INFO_TRACE;
    AppExecFwk::RunningProcessInfo processInfo;
    auto appMgrClient = DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance();
    CHKPR(appMgrClient, ERROR_NULL_POINTER);
    auto begin = std::chrono::high_resolution_clock::now();
    appMgrClient->GetRunningProcessInfoByPid(pid, processInfo);
    auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - begin).count();
#ifdef OHOS_BUILD_ENABLE_DFX_RADAR
    DfxHisysevent::ReportApiCallTimes(ApiDurationStatistics::Api::GET_RUNNING_PROCESS_INFO_BY_PID, durationMS);
#endif // OHOS_BUILD_ENABLE_DFX_RADAR
    if (processInfo.extensionType_ != AppExecFwk::ExtensionAbilityType::INPUTMETHOD) {
        MMI_HILOGW("It is prohibited for non-input applications");
        return ERR_NON_INPUT_APPLICATION;
    }
    bool hasVirtualKeyboard = false;
#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
    hasVirtualKeyboard = INPUT_DEV_MGR->HasVirtualKeyboardDevice();
#endif // OHOS_BUILD_ENABLE_VKEYBOARD
    std::vector<struct libinput_device*> input_device;
    int32_t DeviceId = -1;
    INPUT_DEV_MGR->GetMultiKeyboardDevice(input_device);
    if (input_device.size() == 0 && !hasVirtualKeyboard) {
        MMI_HILOGW("No keyboard device is currently available");
        return ERR_DEVICE_NOT_EXIST;
    }
    auto keyEvent = KeyEventHdr->GetKeyEvent();
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    bool checkState = keyEvent->GetFunctionKey(funcKey);
    MMI_HILOGI("checkState:%{public}d, enable:%{public}d", checkState, enable);
    if (checkState == enable) {
        return RET_OK;
    }
#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
    if (funcKey == KeyEvent::CAPS_LOCK_FUNCTION_KEY) {
        // set vkeyboard caps state with separate API.
        MMI_HILOGI("Set vkb func state old:%{public}d, new:%{public}d", checkState, enable);
        libinput_toggle_caps_key();
    }
#endif // OHOS_BUILD_ENABLE_VKEYBOARD
    for (auto it = input_device.begin(); it != input_device.end(); ++it) {
        auto device = (*it);
        DeviceId = INPUT_DEV_MGR->FindInputDeviceId(device);
        if (LibinputAdapter::DeviceLedUpdate(device, funcKey, enable) != RET_OK) {
            MMI_HILOGE("Failed to set the keyboard led, device id %{public}d", DeviceId);
        }
        int32_t state = libinput_get_funckey_state(device, funcKey);
        if (state != enable) {
            MMI_HILOGE("Failed to enable the function key, device id %{public}d", DeviceId);
        }
    }
    int32_t ret = keyEvent->SetFunctionKey(funcKey, enable);
    if (ret != funcKey) {
        MMI_HILOGE("Failed to enable the function key");
        return RET_ERR;
    }
    MMI_HILOGD("Update function key:%{public}d succeed", funcKey);
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
int32_t ServerMsgHandler::OnInjectPointerEvent(const std::shared_ptr<PointerEvent> pointerEvent, int32_t pid,
    bool isNativeInject, bool isShell, int32_t useCoordinate)
{
    CALL_DEBUG_ENTER;
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    LogTracer lt(pointerEvent->GetId(), pointerEvent->GetEventType(), pointerEvent->GetPointerAction());
    if (isNativeInject) {
        int32_t checkReturn = NativeInjectCheck(pid);
        if (checkReturn != RET_OK) {
            return checkReturn;
        }
    }
    return OnInjectPointerEventExt(pointerEvent, isShell, useCoordinate);
}

int32_t ServerMsgHandler::OnInjectTouchPadEvent(const std::shared_ptr<PointerEvent> pointerEvent, int32_t pid,
    const TouchpadCDG &touchpadCDG, bool isNativeInject, bool isShell)
{
    CALL_DEBUG_ENTER;
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    LogTracer lt(pointerEvent->GetId(), pointerEvent->GetEventType(), pointerEvent->GetPointerAction());
    if (isNativeInject) {
        int32_t checkReturn = NativeInjectCheck(pid);
        if (checkReturn != RET_OK) {
            return checkReturn;
        }
    }
    return OnInjectTouchPadEventExt(pointerEvent, touchpadCDG, isShell);
}

bool ServerMsgHandler::IsNavigationWindowInjectEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    return pointerEvent->GetZOrder() > 0;
}

int32_t ServerMsgHandler::OnInjectTouchPadEventExt(const std::shared_ptr<PointerEvent> pointerEvent,
    const TouchpadCDG &touchpadCDG, bool isShell)
{
    CALL_DEBUG_ENTER;
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    EndLogTraceId(pointerEvent->GetId());
    pointerEvent->UpdateId();
    LogTracer lt(pointerEvent->GetId(), pointerEvent->GetEventType(), pointerEvent->GetPointerAction());
    auto inputEventNormalizeHandler = InputHandler->GetEventNormalizeHandler();
    CHKPR(inputEventNormalizeHandler, ERROR_NULL_POINTER);
    if (pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_MOUSE) {
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
        int32_t ret = AccelerateMotionTouchpad(pointerEvent, touchpadCDG);
        if (ret != RET_OK) {
            MMI_HILOGE("Failed to accelerate motion, error:%{public}d", ret);
            return ret;
        }
        UpdatePointerEvent(pointerEvent);
        inputEventNormalizeHandler->HandlePointerEvent(pointerEvent);
        CHKPR(pointerEvent, ERROR_NULL_POINTER);
        pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY);
        if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_HIDE_POINTER)) {
            if (POINTER_DEV_MGR.isInit) {
                CursorDrawingComponent::GetInstance().SetMouseDisplayState(false);
            }
        } else if ((pointerEvent->GetPointerAction() < PointerEvent::POINTER_ACTION_PULL_DOWN) ||
            (pointerEvent->GetPointerAction() > PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW)) {
            if (POINTER_DEV_MGR.isInit && !CursorDrawingComponent::GetInstance().IsPointerVisible()) {
                CursorDrawingComponent::GetInstance().SetPointerVisible(getpid(), true, 0, false);
            }
        }
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    } else {
            MMI_HILOGW("Source types are not Touchpad, source:%{public}d", pointerEvent->GetSourceType());
    }
    return SaveTargetWindowId(pointerEvent, isShell);
}

void ServerMsgHandler::DealGesturePointers(std::shared_ptr<PointerEvent> pointerEvent)
{
    if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY)) {
        return;
    }
    MMI_HILOGI("Check : current PointerEvent's info :Id=>%{public}d, pointerId=>%{public}d",
        pointerEvent->GetId(), pointerEvent->GetPointerId());
    std::shared_ptr<PointerEvent> touchEvent = WIN_MGR->GetLastPointerEventForGesture();
    CHKPV(touchEvent);
    std::list<PointerEvent::PointerItem> lastPointerItems = touchEvent->GetAllPointerItems();
    std::list<PointerEvent::PointerItem> currenPointerItems = pointerEvent->GetAllPointerItems();
    for (auto &item : lastPointerItems) {
        if (!item.IsPressed()) {
            continue;
        }
        auto iter = currenPointerItems.begin();
        for (; iter != currenPointerItems.end(); iter++) {
            if (item.GetOriginPointerId() == iter->GetOriginPointerId()) {
                break;
            }
        }
        if (iter == currenPointerItems.end()) {
            pointerEvent->AddPointerItem(item);
            MMI_HILOGD("Check : add Item : pointerId=>%{public}d, OriginPointerId=>%{public}d",
                item.GetPointerId(), item.GetOriginPointerId());
        }
    }
}

int32_t ServerMsgHandler::OnInjectPointerEventExt(const std::shared_ptr<PointerEvent> pointerEvent, bool isShell,
    int32_t useCoordinate)
{
    CALL_DEBUG_ENTER;
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    EndLogTraceId(pointerEvent->GetId());
    pointerEvent->UpdateId();
    LogTracer lt(pointerEvent->GetId(), pointerEvent->GetEventType(), pointerEvent->GetPointerAction());
    auto inputEventNormalizeHandler = InputHandler->GetEventNormalizeHandler();
    CHKPR(inputEventNormalizeHandler, ERROR_NULL_POINTER);
    switch (pointerEvent->GetSourceType()) {
        case PointerEvent::SOURCE_TYPE_TOUCHSCREEN: {
#ifdef OHOS_BUILD_ENABLE_TOUCH
            if (!FixTargetWindowId(pointerEvent, pointerEvent->GetPointerAction(), isShell)) {
                return RET_ERR;
            }
            DealGesturePointers(pointerEvent);
            WIN_MGR->ProcessInjectEventGlobalXY(pointerEvent, useCoordinate);
            inputEventNormalizeHandler->HandleTouchEvent(pointerEvent);
            if (!pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY) &&
                !(IsCastInject(pointerEvent->GetDeviceId())) &&
                !IsNavigationWindowInjectEvent(pointerEvent)) {
#ifdef OHOS_BUILD_ENABLE_TOUCH_DRAWING
                TOUCH_DRAWING_MGR->TouchDrawHandler(pointerEvent);
#endif // #ifdef OHOS_BUILD_ENABLE_TOUCH_DRAWING
            }
#endif // OHOS_BUILD_ENABLE_TOUCH
            break;
        }
        case PointerEvent::SOURCE_TYPE_MOUSE:
#ifdef OHOS_BUILD_ENABLE_JOYSTICK
        case PointerEvent::SOURCE_TYPE_JOYSTICK:
#endif // OHOS_BUILD_ENABLE_JOYSTICK
        case PointerEvent::SOURCE_TYPE_TOUCHPAD: {
#ifdef OHOS_BUILD_ENABLE_POINTER
            int32_t ret = AccelerateMotion(pointerEvent);
            if (ret != RET_OK) {
                MMI_HILOGE("Failed to accelerate motion, error:%{public}d", ret);
                return ret;
            }
            UpdatePointerEvent(pointerEvent);
            WIN_MGR->ProcessInjectEventGlobalXY(pointerEvent, useCoordinate);
            inputEventNormalizeHandler->HandlePointerEvent(pointerEvent);
            CHKPR(pointerEvent, ERROR_NULL_POINTER);
            if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY)) {
                break;
            } else if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_HIDE_POINTER)) {
                CursorDrawingComponent::GetInstance().SetMouseDisplayState(false);
            } else if (((pointerEvent->GetPointerAction() < PointerEvent::POINTER_ACTION_PULL_DOWN) ||
                (pointerEvent->GetPointerAction() > PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW)) &&
                !CursorDrawingComponent::GetInstance().IsPointerVisible()) {
                CursorDrawingComponent::GetInstance().SetPointerVisible(getpid(), true, 0, false);
            }
#endif // OHOS_BUILD_ENABLE_POINTER
            break;
        }
        default: {
            MMI_HILOGW("Source type is unknown, source:%{public}d", pointerEvent->GetSourceType());
            break;
        }
    }
    return SaveTargetWindowId(pointerEvent, isShell);
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

#ifdef OHOS_BUILD_ENABLE_POINTER
float ServerMsgHandler::ScreenFactor(const int32_t diagonalInch)
{
    if (diagonalInch <= SCREEN_DIAGONAL_0) {
        return FACTOR_0;
    } else if (diagonalInch < SCREEN_DIAGONAL_8) {
        return FACTOR_8;
    } else if (diagonalInch < SCREEN_DIAGONAL_18) {
        return FACTOR_18;
    } else if (diagonalInch < SCREEN_DIAGONAL_27) {
        return FACTOR_27;
    } else if (diagonalInch < SCREEN_DIAGONAL_55) {
        return FACTOR_55;
    } else {
        return FACTOR_MAX;
    }
}

int32_t ServerMsgHandler::AccelerateMotion(std::shared_ptr<PointerEvent> pointerEvent)
{
    if (!pointerEvent->HasFlag(InputEvent::EVENT_FLAG_RAW_POINTER_MOVEMENT) ||
        (pointerEvent->GetSourceType() != PointerEvent::SOURCE_TYPE_MOUSE) ||
        ((pointerEvent->GetPointerAction() != PointerEvent::POINTER_ACTION_MOVE) &&
        (pointerEvent->GetPointerAction() != PointerEvent::POINTER_ACTION_PULL_MOVE) &&
        (pointerEvent->GetPointerAction() != PointerEvent::POINTER_ACTION_BUTTON_DOWN))) {
            return RET_OK;
    }
    PointerEvent::PointerItem pointerItem {};
    if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
        MMI_HILOGE("Pointer event is corrupted");
        return RET_ERR;
    }
    CursorPosition cursorPos = WIN_MGR->GetCursorPos();
    if (cursorPos.displayId < 0) {
        MMI_HILOGE("No display");
        return RET_ERR;
    }
    Offset offset {
        .dx = pointerItem.GetRawDx(),
        .dy = pointerItem.GetRawDy(),
    };
    auto displayInfo = WIN_MGR->GetPhysicalDisplay(cursorPos.displayId);
    CHKPR(displayInfo, ERROR_NULL_POINTER);
#ifndef OHOS_BUILD_EMULATOR
    Direction displayDirection = WIN_MGR->GetDisplayDirection(displayInfo);
    CalculateOffset(displayDirection, offset);
#endif // OHOS_BUILD_EMULATOR
    int32_t ret = RET_OK;
    if (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        WIN_MGR->UpdateAndAdjustMouseLocation(cursorPos.displayId, cursorPos.cursorPos.x, cursorPos.cursorPos.y);
        return RET_OK;
    }
    if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_TOUCHPAD_POINTER) &&
        pointerEvent->HasFlag(InputEvent::EVENT_FLAG_VIRTUAL_TOUCHPAD_POINTER)) {
        ret = HandleMotionAccelerateTouchpad(&offset, WIN_MGR->GetMouseIsCaptureMode(),
            &cursorPos.cursorPos.x, &cursorPos.cursorPos.y,
            MouseTransformProcessor::GetTouchpadSpeed(), static_cast<int32_t>(DeviceType::DEVICE_FOLD_PC_VIRT));
    } else if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_TOUCHPAD_POINTER)) {
        ret = HandleMotionAccelerateTouchpad(&offset, WIN_MGR->GetMouseIsCaptureMode(),
            &cursorPos.cursorPos.x, &cursorPos.cursorPos.y,
            MouseTransformProcessor::GetTouchpadSpeed(), static_cast<int32_t>(DeviceType::DEVICE_PC));
    } else {
        uint64_t deltaTime = 0;
#ifdef OHOS_BUILD_MOUSE_REPORTING_RATE
        static uint64_t preTime = -1;
        uint64_t currentTime = static_cast<uint64_t>(pointerEvent->GetActionTime());
        preTime = fmin(preTime, currentTime);
        deltaTime = (currentTime - preTime);
        preTime = currentTime;
#endif // OHOS_BUILD_MOUSE_REPORTING_RATE
        if (displayInfo->ppi != 0) {
            int32_t diagonalMm = static_cast<int32_t>(sqrt((displayInfo->physicalWidth * displayInfo->physicalWidth) +
            (displayInfo->physicalHeight * displayInfo->physicalHeight)));
            int32_t diagonalInch = static_cast<int32_t>(diagonalMm / MM_TO_INCH);
            float factor = ScreenFactor(diagonalInch);
            ret = HandleMotionDynamicAccelerateMouse(&offset, WIN_MGR->GetMouseIsCaptureMode(),
            &cursorPos.cursorPos.x, &cursorPos.cursorPos.y, MouseTransformProcessor::GetPointerSpeed(),
            deltaTime, static_cast<double>(displayInfo->ppi), static_cast<double>(factor));
        } else {
            ret = HandleMotionAccelerateMouse(&offset, WIN_MGR->GetMouseIsCaptureMode(),
            &cursorPos.cursorPos.x, &cursorPos.cursorPos.y,
            MouseTransformProcessor::GetPointerSpeed(), static_cast<int32_t>(DeviceType::DEVICE_PC));
        }
    }
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to accelerate pointer motion, error:%{public}d", ret);
        return ret;
    }
    WIN_MGR->UpdateAndAdjustMouseLocation(cursorPos.displayId, cursorPos.cursorPos.x, cursorPos.cursorPos.y);
    if (EventLogHelper::IsBetaVersion() && !pointerEvent->HasFlag(InputEvent::EVENT_FLAG_PRIVACY_MODE)) {
        MMI_HILOGD("Cursor move to (x:%.2f, y:%.2f, DisplayId:%d)",
            cursorPos.cursorPos.x, cursorPos.cursorPos.y, cursorPos.displayId);
    } else {
        MMI_HILOGD("Cursor move to (x:%.2f, y:%.2f, DisplayId:%d)",
            cursorPos.cursorPos.x, cursorPos.cursorPos.y, cursorPos.displayId);
    }
    return RET_OK;
}

int32_t ServerMsgHandler::AccelerateMotionTouchpad(std::shared_ptr<PointerEvent> pointerEvent,
    const TouchpadCDG &touchpadCDG)
{
    PointerEvent::PointerItem pointerItem {};
    if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
        MMI_HILOGE("Pointer event is corrupted");
        return RET_ERR;
    }
    CursorPosition cursorPos = WIN_MGR->GetCursorPos();
    if (cursorPos.displayId < 0) {
        MMI_HILOGE("No display");
        return RET_ERR;
    }
    Offset offset {
        .dx = pointerItem.GetRawDx(),
        .dy = pointerItem.GetRawDy(),
    };
    auto displayInfo = WIN_MGR->GetPhysicalDisplay(cursorPos.displayId);
    CHKPR(displayInfo, ERROR_NULL_POINTER);
#ifndef OHOS_BUILD_EMULATOR
    Direction displayDirection = static_cast<Direction>((
        ((displayInfo->direction - displayInfo->displayDirection) * ANGLE_90 + ANGLE_360) % ANGLE_360) / ANGLE_90);
    CalculateOffset(displayDirection, offset);
#endif // OHOS_BUILD_EMULATOR
    int32_t ret = RET_OK;

#ifdef OHOS_BUILD_MOUSE_REPORTING_RATE
    MMI_HILOGE("Hidumper before HandleMotionDynamicAccelerateTouchpad");
    static uint64_t preTime = -1;
    uint64_t currentTime = static_cast<uint64_t>(pointerEvent->GetActionTime());
    preTime = fmin(preTime, currentTime);
    uint64_t deltaTime = (currentTime - preTime);
    MMI_HILOGE("DeltaTime before HandleMotionDynamicAccelerateTouchpad: %{public}PRId64 ms", deltaTime);

    double displaySize = sqrt(pow(displayInfo->width, 2) + pow(displayInfo->height, 2));
    double touchpadSize = touchpadCDG.size;
    double touchpadPPi = touchpadCDG.ppi;
    int32_t touchpadSpeed = touchpadCDG.speed;
    int32_t frequency = touchpadCDG.frequency;
    if (touchpadSize <= 0 || touchpadPPi <= 0 || touchpadSpeed <= 0 || frequency <= 0) {
        MMI_HILOGE("touchpadSize, touchpadPPi or touchpadSpeed are invalid,
            touchpadSize:%{public}lf, touchpadPPi:%{public}lf, touchpadSpeed:%{public}d, frequency:%{public}d",
            touchpadSize, touchpadPPi, touchpadSpeed, frequency);
        return RET_ERR;
    }
    if (pointerEvent->GetPointerAction() == POINTER_ACTION_MOVE) {
        ret = HandleMotionDynamicAccelerateTouchpad(&offset, WIN_MGR->GetMouseIsCaptureMode(), &cursorPos.cursorPos.x,
            &cursorPos.cursorPos.y, touchpadSpeed, displaySize, touchpadSize, touchpadPPi, frequency);
    }
    MMI_HILOGE("DeltaTime after HandleMotionDynamicAccelerateTouchpad: %{public}PRId64 ms", deltaTime);
    MMI_HILOGE("Hidumper after HandleMotionDynamicAccelerateTouchpad");
    preTime = currentTime;
#else
    ret = HandleMotionAccelerateTouchpad(&offset, WIN_MGR->GetMouseIsCaptureMode(),
        &cursorPos.cursorPos.x, &cursorPos.cursorPos.y,
        MouseTransformProcessor::GetTouchpadSpeed(), static_cast<int32_t>(DeviceType::DEVICE_PC));
#endif // OHOS_BUILD_MOUSE_REPORTING_RATE
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to accelerate pointer motion, error:%{public}d", ret);
        return ret;
    }
    WIN_MGR->UpdateAndAdjustMouseLocation(cursorPos.displayId, cursorPos.cursorPos.x, cursorPos.cursorPos.y);
    if (EventLogHelper::IsBetaVersion() && !pointerEvent->HasFlag(InputEvent::EVENT_FLAG_PRIVACY_MODE)) {
        MMI_HILOGD("Cursor move to (x:%.2f, y:%.2f, DisplayId:%d)",
            cursorPos.cursorPos.x, cursorPos.cursorPos.y, cursorPos.displayId);
    } else {
        MMI_HILOGD("Cursor move to (x:%.2f, y:%.2f, DisplayId:%d)",
            cursorPos.cursorPos.x, cursorPos.cursorPos.y, cursorPos.displayId);
    }
    return RET_OK;
}

void ServerMsgHandler::CalculateOffset(Direction direction, Offset &offset)
{
    std::negate<double> neg;
    if (direction == DIRECTION90) {
        double tmp = offset.dx;
        offset.dx = offset.dy;
        offset.dy = neg(tmp);
    } else if (direction == DIRECTION180) {
        offset.dx = neg(offset.dx);
        offset.dy = neg(offset.dy);
    } else if (direction == DIRECTION270) {
        double tmp = offset.dx;
        offset.dx = neg(offset.dy);
        offset.dy = tmp;
    }
}
#endif // OHOS_BUILD_ENABLE_POINTER

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
void ServerMsgHandler::UpdatePointerEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    if ((!pointerEvent->HasFlag(InputEvent::EVENT_FLAG_RAW_POINTER_MOVEMENT) ||
        (pointerEvent->GetSourceType() != PointerEvent::SOURCE_TYPE_MOUSE)) &&
        (pointerEvent->GetPointerAction() != PointerEvent::POINTER_ACTION_TOUCHPAD_ACTIVE)) {
        return;
    }
    PointerEvent::PointerItem pointerItem {};
    if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
        MMI_HILOGE("Pointer event is corrupted");
        return;
    }
    auto mouseInfo = WIN_MGR->GetMouseInfo();
    pointerItem.SetDisplayX(mouseInfo.physicalX);
    pointerItem.SetDisplayY(mouseInfo.physicalY);
    pointerItem.SetDisplayXPos(mouseInfo.physicalX);
    pointerItem.SetDisplayYPos(mouseInfo.physicalY);
    pointerEvent->UpdatePointerItem(pointerEvent->GetPointerId(), pointerItem);
    pointerEvent->SetTargetDisplayId(mouseInfo.displayId);
}

int32_t ServerMsgHandler::SaveTargetWindowId(std::shared_ptr<PointerEvent> pointerEvent, bool isShell)
{
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    InjectionTouch touch{
        .displayId_ = pointerEvent->GetTargetDisplayId(), .pointerId_ = pointerEvent->GetPointerId()};
    if ((pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) &&
        (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_DOWN ||
        pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_HOVER_ENTER)) {
        int32_t targetWindowId = pointerEvent->GetTargetWindowId();
        if (isShell) {
            shellTargetWindowIds_[touch] = targetWindowId;
        } else if (IsCastInject(pointerEvent->GetDeviceId()) && (pointerEvent->GetZOrder() > 0)) {
            castTargetWindowIds_[touch] = targetWindowId;
        } else if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY)) {
            accessTargetWindowIds_[touch] = targetWindowId;
        } else {
            nativeTargetWindowIds_[touch] = targetWindowId;
        }
        MMI_HILOGD("Update displayId:%{public}d, pointerId:%{public}d, targetWindowId:%{public}d",
            touch.displayId_, touch.pointerId_, targetWindowId);
    }
    if ((pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) &&
        (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_UP ||
        pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_HOVER_EXIT)) {
        if (isShell) {
            shellTargetWindowIds_.erase(touch);
        } else if (IsCastInject(pointerEvent->GetDeviceId()) && (pointerEvent->GetZOrder() > 0)) {
            castTargetWindowIds_.erase(touch);
        } else if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY)) {
            accessTargetWindowIds_.erase(touch);
        } else {
            nativeTargetWindowIds_.erase(touch);
        }
        MMI_HILOGD("Erase displayId:%{public}d, pointerId:%{public}d",
            touch.displayId_, touch.pointerId_);
    }
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

#ifdef OHOS_BUILD_ENABLE_TOUCH
bool ServerMsgHandler::FixTargetWindowId(std::shared_ptr<PointerEvent> pointerEvent,
    int32_t action, bool isShell)
{
    int32_t targetWindowId = -1;
    if (isShell) {
        targetWindowId = FixTargetWindowId(pointerEvent, shellTargetWindowIds_);
    } else if ((IsCastInject(pointerEvent->GetDeviceId())) && (pointerEvent->GetZOrder() > 0)) {
        targetWindowId = FixTargetWindowId(pointerEvent, castTargetWindowIds_, true, CAST_POINTER_ID);
    } else if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY)) {
        targetWindowId = FixTargetWindowId(pointerEvent, accessTargetWindowIds_);
    } else {
        targetWindowId = FixTargetWindowId(pointerEvent, nativeTargetWindowIds_, true, DEFAULT_POINTER_ID);
    }
    MMI_HILOGD("TargetWindowId:%{public}d %{public}d", pointerEvent->GetTargetWindowId(), targetWindowId);
    return UpdateTouchEvent(pointerEvent, action, targetWindowId);
}

int32_t ServerMsgHandler::FixTargetWindowId(std::shared_ptr<PointerEvent> pointerEvent,
    const std::map<InjectionTouch, int32_t>& targetWindowIdMap, bool bNeedResetPointerId, int32_t diffPointerId)
{
    CHKPR(pointerEvent, RET_ERR);
    std::list<PointerEvent::PointerItem> pointerItems = pointerEvent->GetAllPointerItems();
    if (bNeedResetPointerId) {
        if (diffPointerId <= 0) {
            MMI_HILOGE("Parameter diffPointerId error, diffPointerId:%{public}d", diffPointerId);
            return RET_ERR;
        }
        pointerEvent->RemoveAllPointerItems();
        for (auto &pointerItem : pointerItems) {
            int32_t pointId = pointerItem.GetPointerId();
            pointId += diffPointerId;
            pointerItem.SetPointerId(pointId);
            pointerEvent->AddPointerItem(pointerItem);
        }
        if (pointerEvent->GetPointerId() <= INT32_MAX - diffPointerId) {
            pointerEvent->SetPointerId(pointerEvent->GetPointerId() + diffPointerId);
        }
    }
    auto displayId = pointerEvent->GetTargetDisplayId();
    if (displayId < 0 && !WIN_MGR->UpdateDisplayId(displayId)) {
        MMI_HILOG_DISPATCHE("This display is not existent");
        return RET_ERR;
    }
    InjectionTouch touch{
        .displayId_ = displayId, .pointerId_ = pointerEvent->GetPointerId()};
    auto iter = targetWindowIdMap.find(touch);
    if (iter != targetWindowIdMap.end()) {
        return iter->second;
    }
    return RET_ERR;
}

bool ServerMsgHandler::UpdateTouchEvent(std::shared_ptr<PointerEvent> pointerEvent,
    int32_t action, int32_t targetWindowId)
{
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
        MMI_HILOGE("Can't find pointer item, pointer:%{public}d", pointerId);
        return false;
    }
    if (action == PointerEvent::POINTER_ACTION_HOVER_ENTER ||
        action == PointerEvent::POINTER_ACTION_DOWN || targetWindowId < 0) {
        MMI_HILOGD("Down event or targetWindowId less 0 is not need fix window id");
        return true;
    }
    auto pointerIds = pointerEvent->GetPointerIds();
    if (pointerIds.empty()) {
        MMI_HILOGE("GetPointerIds is empty");
        return false;
    }

    pointerEvent->SetTargetWindowId(targetWindowId);
    pointerItem.SetTargetWindowId(targetWindowId);
    pointerEvent->UpdatePointerItem(pointerId, pointerItem);
    return true;
}
#endif // OHOS_BUILD_ENABLE_TOUCH

int32_t ServerMsgHandler::OnUiExtentionWindowInfo(NetPacket &pkt, WindowInfo& info)
{
    uint32_t num = 0;
    pkt >> num;
    CHKRWER(pkt, RET_ERR);
    CHKUPPER(num, MAX_UI_EXTENSION_SIZE, RET_ERR);
    for (uint32_t i = 0; i < num; i++) {
        WindowInfo extensionInfo;
        pkt >> extensionInfo.id >> extensionInfo.pid >> extensionInfo.uid >> extensionInfo.area
            >> extensionInfo.defaultHotAreas >> extensionInfo.pointerHotAreas >> extensionInfo.agentWindowId
            >> extensionInfo.flags >> extensionInfo.action >> extensionInfo.displayId >> extensionInfo.groupId
            >> extensionInfo.zOrder >> extensionInfo.pointerChangeAreas >> extensionInfo.transform
            >> extensionInfo.windowInputType >> extensionInfo.privacyMode >> extensionInfo.windowType
            >> extensionInfo.privacyUIFlag >> extensionInfo.rectChangeBySystem
            >> extensionInfo.isSkipSelfWhenShowOnVirtualScreen
            >> extensionInfo.windowNameType >> extensionInfo.agentPid;
        CHKRWER(pkt, RET_ERR);
        info.uiExtentionWindowInfo.push_back(extensionInfo);
    }
    return RET_OK;
}

bool ServerMsgHandler::IsCastInject(int32_t deviceid)
{
    return (deviceid == CAST_INPUT_DEVICEID || deviceid == CAST_SCREEN_DEVICEID);
}

int32_t ServerMsgHandler::OnDisplayInfo(SessionPtr sess, NetPacket &pkt)
{
    CALL_DEBUG_ENTER;
    CHKPR(sess, ERROR_NULL_POINTER);
    int32_t tokenType = sess->GetTokenType();
    if (tokenType != TokenType::TOKEN_NATIVE && tokenType != TokenType::TOKEN_SHELL &&
        tokenType !=TokenType::TOKEN_SYSTEM_HAP) {
        MMI_HILOGW("Not native or systemapp skip, pid:%{public}d tokenType:%{public}d", sess->GetPid(), tokenType);
        return RET_ERR;
    }
    UserScreenInfo userScreenInfo;
    oldDisplayGroupInfos_.clear();
    pkt >> userScreenInfo.userId >> userScreenInfo.userState;
    if (ReadScreensInfo(pkt, userScreenInfo) != RET_OK) {
        return RET_ERR;
    }
    if (ReadDisplayGroupsInfo(pkt, userScreenInfo) != RET_OK) {
        return RET_ERR;
    }
    if (!ChangeToOld(userScreenInfo)) {
        return RET_ERR;
    }
    Printf(userScreenInfo);
    for (auto &displayGroupInfo : oldDisplayGroupInfos_) {
        WIN_MGR->UpdateDisplayInfoExtIfNeed(displayGroupInfo, true);
    }
    return RET_OK;
}

int32_t ServerMsgHandler::ReadScreensInfo(NetPacket &pkt, UserScreenInfo &userScreenInfo)
{
    uint32_t num = 0;
    pkt >> num;
    CHKRWER(pkt, RET_ERR);
    CHKUPPER(num, MAX_SCREEN_SIZE, RET_ERR);
    for (uint32_t i = 0; i < num; i++) {
        ScreenInfo info;
        pkt >> info.id >> info.uniqueId >> info.screenType >> info.width >> info.height >> info.physicalWidth
            >> info.physicalHeight >> info.tpDirection >> info.dpi >> info.ppi >> info.rotation;
        CHKRWER(pkt, RET_ERR);
        userScreenInfo.screens.push_back(info);
    }
    return RET_OK;
}

int32_t ServerMsgHandler::ReadDisplayGroupsInfo(NetPacket &pkt, UserScreenInfo &userScreenInfo)
{
    uint32_t num = 0;
    pkt >> num;
    CHKRWER(pkt, RET_ERR);
    CHKUPPER(num, MAX_DISPLAY_GROUP_SIZE, RET_ERR);
    for (uint32_t i = 0; i < num; i++) {
        DisplayGroupInfo info;
        OLD::DisplayGroupInfo oldInfo;
        pkt >> info.id >> info.name >> info.type >> info.mainDisplayId >> info.focusWindowId;
        CHKRWER(pkt, RET_ERR);
        if (ReadDisplaysInfo(pkt, info) != RET_OK) {
            return RET_ERR;
        }
        if (ReadWindowsInfo(pkt, info, oldInfo) != RET_OK) {
            return RET_ERR;
        }
        userScreenInfo.displayGroups.push_back(info);
        oldDisplayGroupInfos_.push_back(oldInfo);
    }
    return RET_OK;
}

int32_t ServerMsgHandler::ReadDisplaysInfo(NetPacket &pkt, DisplayGroupInfo &displayGroupInfo)
{
    uint32_t num = 0;
    pkt >> num;
    CHKRWER(pkt, RET_ERR);
    CHKUPPER(num, MAX_DISPLAY_SIZE, RET_ERR);
    for (uint32_t i = 0; i < num; i++) {
        DisplayInfo info;
        pkt >> info.id >> info.x >> info.y >> info.width >> info.height >> info.dpi >> info.name
            >> info.direction >> info.displayDirection >> info.displayMode >> info.transform
            >> info.scalePercent >> info.expandHeight >> info.isCurrentOffScreenRendering
            >> info.displaySourceMode >> info.oneHandX >> info.oneHandY >> info.screenArea >> info.rsId
            >> info.offsetX >> info.offsetY >> info.pointerActiveWidth >> info.pointerActiveHeight
            >> info.deviceRotation >> info.rotationCorrection;
        CHKRWER(pkt, RET_ERR);
        displayGroupInfo.displaysInfo.push_back(info);
    }
    return RET_OK;
}

int32_t ServerMsgHandler::OnWindowGroupInfo(SessionPtr sess, NetPacket &pkt)
{
    CALL_DEBUG_ENTER;
    CHKPR(sess, ERROR_NULL_POINTER);
    int32_t tokenType = sess->GetTokenType();
    if (tokenType != TokenType::TOKEN_NATIVE && tokenType != TokenType::TOKEN_SHELL &&
        tokenType !=TokenType::TOKEN_SYSTEM_HAP) {
        MMI_HILOGW("Not native or systemapp skip, pid:%{public}d tokenType:%{public}d", sess->GetPid(), tokenType);
        return RET_ERR;
    }
    WindowGroupInfo windowGroupInfo;
    pkt >> windowGroupInfo.focusWindowId >> windowGroupInfo.displayId;
    CHKRWER(pkt, RET_ERR);
    uint32_t num = 0;
    pkt >> num;
    CHKRWER(pkt, RET_ERR);
    CHKUPPER(num, MAX_WINDOW_GROUP_INFO_SIZE, RET_ERR);
    for (uint32_t i = 0; i < num; i++) {
        WindowInfo info;
        pkt >> info.id >> info.pid >> info.uid >> info.area >> info.defaultHotAreas
            >> info.pointerHotAreas >> info.agentWindowId >> info.flags >> info.action
            >> info.displayId >> info.groupId >> info.zOrder >> info.pointerChangeAreas >> info.transform
            >> info.windowInputType >> info.privacyMode >> info.windowType >> info.isSkipSelfWhenShowOnVirtualScreen
            >> info.windowNameType >> info.agentPid;
        CHKRWER(pkt, RET_ERR);
        OnUiExtentionWindowInfo(pkt, info);
        pkt >> info.rectChangeBySystem;
        CHKRWER(pkt, RET_ERR);
        windowGroupInfo.windowsInfo.push_back(info);
    }
    WIN_MGR->UpdateWindowInfo(windowGroupInfo);
    return RET_OK;
}

int32_t ServerMsgHandler::RegisterWindowStateErrorCallback(SessionPtr sess, NetPacket &pkt)
{
    CALL_DEBUG_ENTER;
    CHKPR(sess, ERROR_NULL_POINTER);
    int32_t tokenType = sess->GetTokenType();
    if (tokenType != TokenType::TOKEN_NATIVE && tokenType != TokenType::TOKEN_SHELL &&
        tokenType !=TokenType::TOKEN_SYSTEM_HAP) {
        MMI_HILOGW("Not native or systemapp skip, pid:%{public}d tokenType:%{public}d", sess->GetPid(), tokenType);
        return RET_ERR;
    }
    int32_t pid = sess->GetPid();
    WIN_MGR->SetWindowStateNotifyPid(pid);
    MMI_HILOGI("The pid:%{public}d", pid);
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_SECURITY_COMPONENT
int32_t ServerMsgHandler::OnEnhanceConfig(SessionPtr sess, NetPacket &pkt)
{
    CHKPR(sess, ERROR_NULL_POINTER);
    int32_t userId = sess->GetUid();
    if (userId != SECURITY_COMPONENT_SERVICE_ID) {
        MMI_HILOGE("Session is not security component service");
        return RET_ERR;
    }
    uint32_t num = 0;
    pkt >> num;
    CHKRWER(pkt, RET_ERR);
    CHKUPPER(num, MAX_ENHANCE_CONFIG_SIZE, RET_ERR);
    uint8_t cfg[num];
    for (uint32_t i = 0; i < num; i++) {
        pkt >> cfg[i];
        CHKRWER(pkt, RET_ERR);
    }
    int32_t result = Security::SecurityComponent::SecCompEnhanceKit::SetEnhanceCfg(cfg, num);
    if (result != 0) {
        return RET_ERR;
    }
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_SECURITY_COMPONENT
#if defined(OHOS_BUILD_ENABLE_INTERCEPTOR) || defined(OHOS_BUILD_ENABLE_MONITOR)
int32_t ServerMsgHandler::OnAddInputHandler(SessionPtr sess, InputHandlerType handlerType,
    HandleEventType eventType, int32_t priority, uint32_t deviceTags)
{
    CHKPR(sess, ERROR_NULL_POINTER);
    MMI_HILOGD("The handlerType:%{public}d", handlerType);
#ifdef OHOS_BUILD_ENABLE_INTERCEPTOR
    if (handlerType == InputHandlerType::INTERCEPTOR) {
        auto interceptorHandler = InputHandler->GetInterceptorHandler();
        CHKPR(interceptorHandler, ERROR_NULL_POINTER);
        return interceptorHandler->AddInputHandler(handlerType, eventType, priority, deviceTags, sess);
    }
#endif // OHOS_BUILD_ENABLE_INTERCEPTOR
#ifdef OHOS_BUILD_ENABLE_MONITOR
    if (handlerType == InputHandlerType::MONITOR) {
        auto monitorHandler = InputHandler->GetMonitorHandler();
        CHKPR(monitorHandler, ERROR_NULL_POINTER);
        return monitorHandler->AddInputHandler(handlerType, eventType, sess);
    }
#endif // OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

int32_t ServerMsgHandler::OnRemoveInputHandler(SessionPtr sess, InputHandlerType handlerType,
    HandleEventType eventType, int32_t priority, uint32_t deviceTags)
{
    CHKPR(sess, ERROR_NULL_POINTER);
    MMI_HILOGD("OnRemoveInputHandler handlerType:%{public}d eventType:%{public}u", handlerType, eventType);
#ifdef OHOS_BUILD_ENABLE_INTERCEPTOR
    if (handlerType == InputHandlerType::INTERCEPTOR) {
        auto interceptorHandler = InputHandler->GetInterceptorHandler();
        CHKPR(interceptorHandler, ERROR_NULL_POINTER);
        interceptorHandler->RemoveInputHandler(handlerType, eventType, priority, deviceTags, sess);
    }
#endif // OHOS_BUILD_ENABLE_INTERCEPTOR
#ifdef OHOS_BUILD_ENABLE_MONITOR
    if (handlerType == InputHandlerType::MONITOR) {
        auto monitorHandler = InputHandler->GetMonitorHandler();
        CHKPR(monitorHandler, ERROR_NULL_POINTER);
        monitorHandler->RemoveInputHandler(handlerType, eventType, sess);
        ANRMgr->RemoveTimersByType(sess, ANR_MONITOR);
    }
#endif // OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

#endif // OHOS_BUILD_ENABLE_INTERCEPTOR || OHOS_BUILD_ENABLE_MONITOR

int32_t ServerMsgHandler::OnAddGestureMonitor(SessionPtr sess, InputHandlerType handlerType,
    HandleEventType eventType, TouchGestureType gestureType, int32_t fingers)
{
#ifdef OHOS_BUILD_ENABLE_MONITOR
    if (handlerType == InputHandlerType::MONITOR) {
        auto monitorHandler = InputHandler->GetMonitorHandler();
        CHKPR(monitorHandler, ERROR_NULL_POINTER);
        return monitorHandler->AddInputHandler(handlerType, eventType, sess, gestureType, fingers);
    }
#endif // OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

int32_t ServerMsgHandler::OnRemoveGestureMonitor(SessionPtr sess, InputHandlerType handlerType,
    HandleEventType eventType, TouchGestureType gestureType, int32_t fingers)
{
#ifdef OHOS_BUILD_ENABLE_MONITOR
    if (handlerType == InputHandlerType::MONITOR) {
        CHKPR(sess, ERROR_NULL_POINTER);
        auto monitorHandler = InputHandler->GetMonitorHandler();
        CHKPR(monitorHandler, ERROR_NULL_POINTER);
        monitorHandler->RemoveInputHandler(handlerType, eventType, sess, gestureType, fingers);
    }
#endif // OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_MONITOR
int32_t ServerMsgHandler::OnMarkConsumed(SessionPtr sess, int32_t eventId)
{
    CHKPR(sess, ERROR_NULL_POINTER);
    auto monitorHandler = InputHandler->GetMonitorHandler();
    CHKPR(monitorHandler, ERROR_NULL_POINTER);
    monitorHandler->MarkConsumed(eventId, sess);
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_MONITOR

int32_t ServerMsgHandler::ReadWindowsInfo(NetPacket &pkt, DisplayGroupInfo &displayGroupInfo,
    OLD::DisplayGroupInfo &oldDisplayGroupInfo)
{
    uint32_t num = 0;
    pkt >> num;
    CHKRWER(pkt, RET_ERR);
    CHKUPPER(num, MAX_WINDOWS_SIZE, RET_ERR);
    for (uint32_t i = 0; i < num; i++) {
            WindowInfo info;
            int32_t byteCount = 0;
            pkt >> info.id >> info.pid >> info.uid >> info.area >> info.defaultHotAreas
                >> info.pointerHotAreas >> info.agentWindowId >> info.flags >> info.action
                >> info.displayId >> info.groupId >> info.zOrder >> info.pointerChangeAreas >> info.transform
                >> info.windowInputType >> info.privacyMode >> info.windowType
                >> info.isSkipSelfWhenShowOnVirtualScreen >> info.windowNameType >> info.agentPid >> byteCount;
            CHKRWER(pkt, RET_ERR);
            OnUiExtentionWindowInfo(pkt, info);
            pkt >> info.rectChangeBySystem;
            CHKRWER(pkt, RET_ERR);
            displayGroupInfo.windowsInfo.push_back(info);
            oldDisplayGroupInfo.windowsInfo.push_back(info);
        }
    return RET_OK;
}

bool ServerMsgHandler::ChangeToOld(const UserScreenInfo& userScreenInfo)
{
    if (userScreenInfo.displayGroups.size() != oldDisplayGroupInfos_.size()) {
        MMI_HILOGE("ChangeToOld size inconsistent, new size:%{public}zu, old size:%{public}zu",
            userScreenInfo.displayGroups.size(), oldDisplayGroupInfos_.size());
        return false;
    }
    size_t num = 0;
    for (auto &displayGroupInfo : userScreenInfo.displayGroups) {
        if (num >= oldDisplayGroupInfos_.size()) {
            MMI_HILOGE("num:%{public}zu", num);
            break;
        }
        oldDisplayGroupInfos_[num].groupId = displayGroupInfo.id;
        oldDisplayGroupInfos_[num].type = displayGroupInfo.type;
        oldDisplayGroupInfos_[num].mainDisplayId = displayGroupInfo.mainDisplayId;
        oldDisplayGroupInfos_[num].focusWindowId = displayGroupInfo.focusWindowId;
        oldDisplayGroupInfos_[num].currentUserId = userScreenInfo.userId;
        oldDisplayGroupInfos_[num].userState = userScreenInfo.userState;
        ChangeToOld(num, displayGroupInfo.displaysInfo, userScreenInfo.screens);
        num++;
    }
    return true;
}

void ServerMsgHandler::ChangeToOld(size_t num, const std::vector<DisplayInfo>& displaysInfo,
    const std::vector<ScreenInfo>& screens)
{
    for (auto &display : displaysInfo) {
        OLD::DisplayInfo oldDisplay;
        oldDisplay = {
            .id = display.id,
            .x = display.x,
            .y = display.y,
            .dpi = display.dpi,
            .name = display.name,
            .direction = display.direction,
            .displayDirection = display.displayDirection,
            .displayMode = display.displayMode,
            .transform = display.transform,
            .offsetX = display.offsetX,
            .offsetY = display.offsetY,
            .scalePercent = display.scalePercent,
            .expandHeight = display.expandHeight,
            .isCurrentOffScreenRendering = display.isCurrentOffScreenRendering,
            .displaySourceMode = display.displaySourceMode,
            .oneHandX = display.oneHandX,
            .oneHandY = display.oneHandY,
            .validWidth = display.width,
            .validHeight = display.height,
            .pointerActiveWidth = display.pointerActiveWidth,
            .pointerActiveHeight = display.pointerActiveHeight,
            .rsId = display.rsId,
            .deviceRotation = display.deviceRotation,
            .rotationCorrection = display.rotationCorrection
        };
        for (auto &screen : screens) {
            if (screen.id == display.screenArea.id) {
                oldDisplay.uniq = screen.uniqueId;
                oldDisplay.ppi = screen.ppi;
                oldDisplay.screenRealWidth = screen.width;
                oldDisplay.screenRealHeight = screen.height;
                oldDisplay.screenRealDPI = screen.dpi;
                oldDisplay.fixedDirection = screen.tpDirection;
                oldDisplay.physicalWidth = screen.physicalWidth;
                oldDisplay.physicalHeight = screen.physicalHeight;
                if (display.screenArea.area.width > 0 && display.screenArea.area.height > 0) {
                    if (oldDisplay.direction == Direction::DIRECTION0
                        || oldDisplay.direction == Direction::DIRECTION180) {
                        oldDisplay.width = static_cast<int32_t>(static_cast<double>(display.width) /
                            static_cast<double>(display.screenArea.area.width) * static_cast<double>(screen.width));
                        oldDisplay.height = static_cast<int32_t>(static_cast<double>(display.height) /
                            static_cast<double>(display.screenArea.area.height) * static_cast<double>(screen.height));
                    }
                    if (oldDisplay.direction == Direction::DIRECTION90
                        || oldDisplay.direction == Direction::DIRECTION270) {
                        oldDisplay.width = static_cast<int32_t>(static_cast<double>(display.width) /
                            static_cast<double>(display.screenArea.area.height) *static_cast<double>(screen.height));
                        oldDisplay.height = static_cast<int32_t>(static_cast<double>(display.height) /
                            static_cast<double>(display.screenArea.area.width) * static_cast<double>(screen.width));
                    }
                }
                break;
            }
        }
        if (num >= oldDisplayGroupInfos_.size()) {
            MMI_HILOGE("num:%{public}zu", num);
            break;
        }
        oldDisplayGroupInfos_[num].displaysInfo.emplace_back(oldDisplay);
    }
}

void ServerMsgHandler::Printf(const UserScreenInfo& userScreenInfo)
{
    MMI_HILOGD("userScreenInfo:{%{private}d:%{public}d}", userScreenInfo.userId, userScreenInfo.userState);
    size_t num = 0;
    for (const auto &item : userScreenInfo.screens) {
        MMI_HILOGD("screen%{public}zu, id:%{public}d, screenType:%{public}d, width:%{public}d, "
                   "height:%{public}d, physicalWidth:%{public}d, physicalHeight:%{public}d, tpDirection:%{public}d, "
                   "dpi%{public}d, ppi%{public}d, rotation%{public}d", num, item.id,
                   item.screenType, item.width, item.height, item.physicalWidth, item.physicalHeight, item.tpDirection,
                   item.dpi, item.ppi, item.rotation);
        num++;
    }
    num = 0;
    for (const auto &item : userScreenInfo.displayGroups) {
        MMI_HILOGD("displayGroups%{public}zu,id:%{public}d,type:%{public}d, mainDisplayId:%{public}d,"
            "focusWindowId:%{public}d",
            num, item.id, item.type, item.mainDisplayId, item.focusWindowId);
        size_t numDisplayInfo = 0;
        for (const auto &itemDisplay : item.displaysInfo) {
            MMI_HILOGD("displays%{public}zu,id:%{public}d,x:%{private}d,y:%{private}d,width:%{public}d,"
                "height:%{public}d,dpi:%{public}d,direction:%{public}d,displayDirection:%{public}d,"
                "displayMode:%{public}d,scalePercent:%{public}d, expandHeight:%{public}d,"
                "isCurrentOffScreenRendering:%{public}d,displaySourceMode:%{public}d,oneHandX:%{private}d,"
                "oneHandY:%{private}d, screenArea:{%{private}d:{%{private}d,%{public}d,%{public}d,%{public}d},"
                "rsId:%{public}" PRIu64 "},offsetX:%{private}d,offsetY:%{private}d,pointerActiveWidth:%{public}d,"
                "pointerActiveHeight:%{public}d,deviceRotation:%{public}d,rotationCorrection:%{public}d,transform:",
                numDisplayInfo, itemDisplay.id, itemDisplay.x, itemDisplay.y, itemDisplay.width, itemDisplay.height,
                itemDisplay.dpi, itemDisplay.direction, itemDisplay.displayDirection,
                itemDisplay.displayMode, itemDisplay.scalePercent, itemDisplay.expandHeight,
                itemDisplay.isCurrentOffScreenRendering, itemDisplay.displaySourceMode, itemDisplay.oneHandX,
                itemDisplay.oneHandY, itemDisplay.screenArea.id, itemDisplay.screenArea.area.x,
                itemDisplay.screenArea.area.y, itemDisplay.screenArea.area.width, itemDisplay.screenArea.area.height,
                itemDisplay.rsId, itemDisplay.offsetX, itemDisplay.offsetY,
                itemDisplay.pointerActiveWidth, itemDisplay.pointerActiveHeight,
                itemDisplay.deviceRotation, itemDisplay.rotationCorrection);
            for (auto& transform : itemDisplay.transform) {
                MMI_HILOGD("%{public}f,", transform);
            }
            numDisplayInfo++;
        }
        for (const auto &itemWindow : item.windowsInfo) {
            MMI_HILOGD(
                "windows,id:%{public}d,pid:%{public}d,agentPid:%{public}d,displayId:%{public}d,groupId:%{public}d",
                itemWindow.id, itemWindow.pid, itemWindow.agentPid, itemWindow.displayId, itemWindow.groupId);
        }
        num++;
    }
}
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
int32_t ServerMsgHandler::OnMoveMouse(int32_t offsetX, int32_t offsetY)
{
    CALL_DEBUG_ENTER;
    if (MouseEventHdr->NormalizeMoveMouse(offsetX, offsetY)) {
        auto pointerEvent = MouseEventHdr->GetPointerEvent();
        CHKPR(pointerEvent, ERROR_NULL_POINTER);
        auto inputEventNormalizeHandler = InputHandler->GetEventNormalizeHandler();
        CHKPR(inputEventNormalizeHandler, ERROR_NULL_POINTER);
        inputEventNormalizeHandler->HandlePointerEvent(pointerEvent);
        MMI_HILOGD("Mouse movement message processed successfully");
    }
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
int32_t ServerMsgHandler::OnSubscribeKeyEvent(IUdsServer *server, int32_t pid,
    int32_t subscribeId, const std::shared_ptr<KeyOption> option)
{
    CALL_DEBUG_ENTER;
    CHKPR(server, ERROR_NULL_POINTER);
    auto sess = server->GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    auto subscriberHandler = InputHandler->GetSubscriberHandler();
    CHKPR(subscriberHandler, ERROR_NULL_POINTER);
    return subscriberHandler->SubscribeKeyEvent(sess, subscribeId, option);
}

int32_t ServerMsgHandler::OnUnsubscribeKeyEvent(IUdsServer *server, int32_t pid, int32_t subscribeId)
{
    CALL_DEBUG_ENTER;
    CHKPR(server, ERROR_NULL_POINTER);
    auto sess = server->GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    auto subscriberHandler = InputHandler->GetSubscriberHandler();
    CHKPR(subscriberHandler, ERROR_NULL_POINTER);
    return subscriberHandler->UnsubscribeKeyEvent(sess, subscribeId);
}

int32_t ServerMsgHandler::OnSubscribeHotkey(IUdsServer *server, int32_t pid,
    int32_t subscribeId, const std::shared_ptr<KeyOption> option)
{
#ifdef SHORTCUT_KEY_MANAGER_ENABLED
    CALL_DEBUG_ENTER;
    CHKPR(server, ERROR_NULL_POINTER);
    auto sess = server->GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    auto subscriberHandler = InputHandler->GetSubscriberHandler();
    CHKPR(subscriberHandler, ERROR_NULL_POINTER);
    return subscriberHandler->SubscribeHotkey(sess, subscribeId, option);
#else
    MMI_HILOGI("OnSubscribeHotkey function does not support");
    return ERROR_UNSUPPORT;
#endif // SHORTCUT_KEY_MANAGER_ENABLED
}

int32_t ServerMsgHandler::OnUnsubscribeHotkey(IUdsServer *server, int32_t pid, int32_t subscribeId)
{
#ifdef SHORTCUT_KEY_MANAGER_ENABLED
    CALL_DEBUG_ENTER;
    CHKPR(server, ERROR_NULL_POINTER);
    auto sess = server->GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    auto subscriberHandler = InputHandler->GetSubscriberHandler();
    CHKPR(subscriberHandler, ERROR_NULL_POINTER);
    return subscriberHandler->UnsubscribeHotkey(sess, subscribeId);
#else
    MMI_HILOGI("OnUnsubscribeHotkey function does not support");
    return ERROR_UNSUPPORT;
#endif // SHORTCUT_KEY_MANAGER_ENABLED
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

#ifdef OHOS_BUILD_ENABLE_KEY_PRESSED_HANDLER
int32_t ServerMsgHandler::SubscribeKeyMonitor(int32_t session, const KeyMonitorOption &keyOption)
{
    if ((PRODUCT_TYPE != "phone") && (PRODUCT_TYPE != "tablet") && (PRODUCT_TYPE != "2in1")) {
        MMI_HILOGW("Does not support subscription of key monitor on %{public}s", PRODUCT_TYPE.c_str());
        return -CAPABILITY_NOT_SUPPORTED;
    }
    KeyMonitorManager::Monitor monitor {
        .session_ = session,
        .key_ = keyOption.GetKey(),
        .action_ = keyOption.GetAction(),
        .isRepeat_ = keyOption.IsRepeat(),
    };
    return KEY_MONITOR_MGR->AddMonitor(monitor);
}

int32_t ServerMsgHandler::UnsubscribeKeyMonitor(int32_t session, const KeyMonitorOption &keyOption)
{
    if ((PRODUCT_TYPE != "phone") && (PRODUCT_TYPE != "tablet") && (PRODUCT_TYPE != "2in1")) {
        MMI_HILOGW("Does not support subscription of key monitor on %{public}s", PRODUCT_TYPE.c_str());
        return -CAPABILITY_NOT_SUPPORTED;
    }
    KeyMonitorManager::Monitor monitor {
        .session_ = session,
        .key_ = keyOption.GetKey(),
        .action_ = keyOption.GetAction(),
        .isRepeat_ = keyOption.IsRepeat(),
    };
    KEY_MONITOR_MGR->RemoveMonitor(monitor);
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_KEY_PRESSED_HANDLER

#ifdef OHOS_BUILD_ENABLE_SWITCH
int32_t ServerMsgHandler::OnSubscribeSwitchEvent(
    IUdsServer *server, int32_t pid, int32_t subscribeId, int32_t switchType)
{
    CALL_DEBUG_ENTER;
    CHKPR(server, ERROR_NULL_POINTER);
    auto sess = server->GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    auto subscriberHandler = InputHandler->GetSwitchSubscriberHandler();
    CHKPR(subscriberHandler, ERROR_NULL_POINTER);
    return subscriberHandler->SubscribeSwitchEvent(sess, subscribeId, switchType);
}

int32_t ServerMsgHandler::OnUnsubscribeSwitchEvent(IUdsServer *server, int32_t pid, int32_t subscribeId)
{
    CALL_DEBUG_ENTER;
    CHKPR(server, ERROR_NULL_POINTER);
    auto sess = server->GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    auto subscriberHandler = InputHandler->GetSwitchSubscriberHandler();
    CHKPR(subscriberHandler, ERROR_NULL_POINTER);
    return subscriberHandler->UnsubscribeSwitchEvent(sess, subscribeId);
}

int32_t ServerMsgHandler::OnQuerySwitchStatus(int32_t switchType, int32_t& state)
{
    CALL_DEBUG_ENTER;
    auto subscriberHandler = InputHandler->GetSwitchSubscriberHandler();
    CHKPR(subscriberHandler, ERROR_NULL_POINTER);
    return subscriberHandler->QuerySwitchStatus(switchType, state);
}
#endif // OHOS_BUILD_ENABLE_SWITCH

int32_t ServerMsgHandler::OnSubscribeLongPressEvent(IUdsServer *server, int32_t pid, int32_t subscribeId,
    const LongPressRequest &longPressRequest)
{
    CALL_DEBUG_ENTER;
    CHKPR(server, ERROR_NULL_POINTER);
    auto sess = server->GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    return LONG_PRESS_EVENT_HANDLER->SubscribeLongPressEvent(sess, subscribeId, longPressRequest);
}

int32_t ServerMsgHandler::OnUnsubscribeLongPressEvent(IUdsServer *server, int32_t pid, int32_t subscribeId)
{
    CALL_DEBUG_ENTER;
    CHKPR(server, ERROR_NULL_POINTER);
    auto sess = server->GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    return LONG_PRESS_EVENT_HANDLER->UnsubscribeLongPressEvent(sess, subscribeId);
}

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH) || defined(OHOS_BUILD_ENABLE_KEYBOARD)
int32_t ServerMsgHandler::AddInputEventFilter(sptr<IEventFilter> filter,
    int32_t filterId, int32_t priority, uint32_t deviceTags, int32_t clientPid)
{
    auto filterHandler = InputHandler->GetFilterHandler();
    CHKPR(filterHandler, ERROR_NULL_POINTER);
    return filterHandler->AddInputEventFilter(filter, filterId, priority, deviceTags, clientPid);
}

int32_t ServerMsgHandler::RemoveInputEventFilter(int32_t clientPid, int32_t filterId)
{
    auto filterHandler = InputHandler->GetFilterHandler();
    CHKPR(filterHandler, ERROR_NULL_POINTER);
    return filterHandler->RemoveInputEventFilter(clientPid, filterId);
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH || OHOS_BUILD_ENABLE_KEYBOARD

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
int32_t ServerMsgHandler::SetShieldStatus(int32_t shieldMode, bool isShield)
{
    return KeyEventHdr->SetShieldStatus(shieldMode, isShield);
}

int32_t ServerMsgHandler::GetShieldStatus(int32_t shieldMode, bool &isShield)
{
    return KeyEventHdr->GetShieldStatus(shieldMode, isShield);
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

void ServerMsgHandler::LaunchAbility()
{
    CALL_DEBUG_ENTER;
#ifndef OHOS_BUILD_ENABLE_WATCH
    AUTH_DIALOG.ConnectSystemUi();
#endif // OHOS_BUILD_ENABLE_WATCH
}

int32_t ServerMsgHandler::OnAuthorize(bool isAuthorize)
{
    CALL_DEBUG_ENTER;
    if (isAuthorize) {
        auto state = AUTHORIZE_HELPER->GetAuthorizeState();
        int32_t authorPid = AUTHORIZE_HELPER->GetAuthorizePid();
        MMI_HILOGE("OnAuthorize not has authorizing s:%{public}d, authPid:%{public}d",
            state, authorPid);
        if (state == AuthorizeState::STATE_UNAUTHORIZE) {
            MMI_HILOGE("Current not has authorizing");
            return ERR_OK;
        }
        if (state == AuthorizeState::STATE_UNAUTHORIZE) {
            MMI_HILOGE("The injection permission has been granted. authPid:%{public}d ", authorPid);
            return ERR_OK;
        }
        InjectNoticeInfo noticeInfo;
        noticeInfo.pid = authorPid;
        AddInjectNotice(noticeInfo);
        auto result = AUTHORIZE_HELPER->AddAuthorizeProcess(authorPid, [&] (int32_t pid) {
                CloseInjectNotice(pid);
        });
        if (result != RET_OK) {
            MMI_HILOGI("Authorize process failed, pid:%{public}d", authorPid);
        }
        MMI_HILOGD("Agree to apply injection,pid:%{public}d", authorPid);
        return ERR_OK;
    }

    auto state = AUTHORIZE_HELPER->GetAuthorizeState();
    int32_t curAuthPid = AUTHORIZE_HELPER->GetAuthorizePid();
    MMI_HILOGD("Reject application injection,s:%{public}d, authPid:%{public}d",
        state, curAuthPid);
    if (state != AuthorizeState::STATE_UNAUTHORIZE) {
        MMI_HILOGI("Cancel injection right,pid:%{public}d", curAuthPid);
        AUTHORIZE_HELPER->CancelAuthorize(curAuthPid);
        if (state == AuthorizeState::STATE_SELECTION_AUTHORIZE) {
            AUTH_DIALOG.CloseDialog();
        } else {
            CloseInjectNotice(AUTHORIZE_HELPER->GetAuthorizePid());
        }
    }
    return ERR_OK;
}

int32_t ServerMsgHandler::OnCancelInjection(int32_t callPid)
{
    CALL_DEBUG_ENTER;
    auto state = AUTHORIZE_HELPER->GetAuthorizeState();
    int32_t curAuthPid = AUTHORIZE_HELPER->GetAuthorizePid();
    MMI_HILOGD("Cancel application injection,s:%{public}d, authPid:%{public}d",
        state, curAuthPid);
    if (state != AuthorizeState::STATE_UNAUTHORIZE) {
        if (callPid != curAuthPid) {
            MMI_HILOGW("Authorized pid not callPid");
            return COMMON_PERMISSION_CHECK_ERROR;
        }
        AUTHORIZE_HELPER->CancelAuthorize(curAuthPid);
        if (state == AuthorizeState::STATE_SELECTION_AUTHORIZE) {
            AUTH_DIALOG.CloseDialog();
        } else {
            CloseInjectNotice(AUTHORIZE_HELPER->GetAuthorizePid());
        }
    }
    return ERR_OK;
}

int32_t ServerMsgHandler::RequestInjection(const int32_t callingPid, int32_t &status, int32_t &reqId)
{
    CALL_DEBUG_ENTER;
    if (!IsPC()) {
        return ERROR_DEVICE_NOT_SUPPORTED;
    }
    auto ret = QueryAuthorizedStatus(callingPid, status);
    MMI_HILOGD("QueryAuthorizedStatus,%{public}d,%{public}d,%{public}d",
        ret, callingPid, status);
    if (ret != ERR_OK) {
        MMI_HILOGE("QueryAuthorizedStatus,%{public}d,%{public}d", ret, callingPid);
        return ret;
    }
    if (static_cast<AUTHORIZE_QUERY_STATE>(status) != AUTHORIZE_QUERY_STATE::UNAUTHORIZED) {
        return ERR_OK;
    }
    if (CheckForRequestInjectionFrequentAccess(callingPid, QUERY_AUTHORIZE_MAX_INTERVAL_TIME)) {
        return ERROR_OPERATION_FREQUENT;
    }
    LaunchAbility();
    AuthorizeExitCallback fnCallback = [&] (int32_t pid) {
        MMI_HILOGI("User not authorized to inject %{public}d", pid);
        AUTH_DIALOG.CloseDialog();
    };
    reqId = GetRequestInjectionCallbackReqId();
    return AUTHORIZE_HELPER->AddAuthorizeProcess(callingPid, fnCallback, reqId);
}

int32_t ServerMsgHandler::QueryAuthorizedStatus(const int32_t callingPid, int32_t &status)
{
    CALL_DEBUG_ENTER;
    auto state = AUTHORIZE_HELPER->GetAuthorizeState();
    if (state == AuthorizeState::STATE_UNAUTHORIZE) {
        status = static_cast<int32_t>(AUTHORIZE_QUERY_STATE::UNAUTHORIZED);
        return ERR_OK;
    }
    int32_t curAuthPid = AUTHORIZE_HELPER->GetAuthorizePid();
    if (curAuthPid == callingPid) {
        if (state == AuthorizeState::STATE_SELECTION_AUTHORIZE) {
            status = static_cast<int32_t>(AUTHORIZE_QUERY_STATE::CURRENT_PID_IN_AUTHORIZATION_SELECTION);
        } else {
            status = static_cast<int32_t>(AUTHORIZE_QUERY_STATE::CURRENT_PID_AUTHORIZED);
        }
        return ERR_OK;
    }
    if (state == AuthorizeState::STATE_SELECTION_AUTHORIZE) {
        status = static_cast<int32_t>(AUTHORIZE_QUERY_STATE::OTHER_PID_IN_AUTHORIZATION_SELECTION);
    } else {
        status = static_cast<int32_t>(AUTHORIZE_QUERY_STATE::OTHER_PID_AUTHORIZED);
    }
    return ERR_OK;
}

bool ServerMsgHandler::IsPC() const
{
    return PRODUCT_TYPE == PRODUCT_TYPE_PC;
}

int32_t ServerMsgHandler::GetRequestInjectionCallbackReqId()
{
    static std::atomic<int32_t> reqId(1);
    if (reqId >= INT32_MAX - 1) {
        reqId = 1;
        return reqId;
    }
    return reqId.fetch_add(1);
}

bool ServerMsgHandler::CheckForRequestInjectionFrequentAccess(int32_t callingPid, int64_t interval)
{
    CALL_DEBUG_ENTER;
    int64_t curTimestamp = GetMillisTime();
    for (auto it = mapQueryAuthorizeLastTimestamp_.begin(); it != mapQueryAuthorizeLastTimestamp_.end();) {
        if (curTimestamp - it->second >= interval) {
            MMI_HILOGD("requestInjection %{public}" PRId64",%{public}" PRId64",%{public}" PRId64"",
            curTimestamp, it->second, curTimestamp - it->second);
            mapQueryAuthorizeLastTimestamp_.erase(it++);
        } else {
            it++;
        }
    }
    auto itFind = mapQueryAuthorizeLastTimestamp_.find(callingPid);
    if (itFind == mapQueryAuthorizeLastTimestamp_.end()) {
        MMI_HILOGD("requestInjection %{public}d", callingPid);
        mapQueryAuthorizeLastTimestamp_.insert(std::make_pair(callingPid, curTimestamp));
        return false;
    }
    return true;
}

void ServerMsgHandler::SetWindowInfo(int32_t infoId, WindowInfo &info)
{
    CALL_DEBUG_ENTER;
    if (transparentWins_.find(infoId) == transparentWins_.end()) {
        MMI_HILOGE("The infoId is Invalid, infoId:%{public}d", infoId);
        return;
    }
    info.pixelMap = transparentWins_[infoId].get();
}

int32_t ServerMsgHandler::SetPixelMapData(int32_t infoId, void *pixelMap) __attribute__((no_sanitize("cfi")))
{
    CALL_DEBUG_ENTER;
    if (infoId < 0 || pixelMap == nullptr) {
        MMI_HILOGE("The infoId is invalid or pixelMap is nullptr");
        return ERR_INVALID_VALUE;
    }

    WIN_MGR->SetPixelMapData(infoId, pixelMap);
    return RET_OK;
}

bool ServerMsgHandler::InitInjectNoticeSource()
{
    CALL_DEBUG_ENTER;
    MMI_HILOGD("Init InjectNoticeSource enter");
    if (injectNotice_ == nullptr) {
        injectNotice_ = std::make_shared<InjectNoticeManager>();
    }
    MMI_HILOGD("Injectnotice StartNoticeAbility end");
    if (!injectNotice_->IsAbilityStart()) {
        MMI_HILOGD("Injectnotice StartNoticeAbility begin");
        bool isStart = injectNotice_->StartNoticeAbility();
        if (!isStart) {
            MMI_HILOGE("Injectnotice StartNoticeAbility isStart:%{public}d", isStart);
            return false;
        }
        MMI_HILOGD("Injectnotice StartNoticeAbility end");
    }
    auto connection = injectNotice_->GetConnection();
    CHKPF(connection);
    if (!connection->IsConnected()) {
        MMI_HILOGD("Injectnotice ConnectNoticeSrv begin");
        bool isConnect = injectNotice_->ConnectNoticeSrv();
        if (!isConnect) {
            MMI_HILOGD("Injectnotice ConnectNoticeSrv isConnect:%{public}d", isConnect);
            return false;
        }
        MMI_HILOGD("Injectnotice ConnectNoticeSrv end");
    }
    MMI_HILOGD("Injectnotice InitInjectNoticeSource end");
    return true;
}

bool ServerMsgHandler::AddInjectNotice(const InjectNoticeInfo &noticeInfo)
{
    CALL_DEBUG_ENTER;
    bool isInit = InitInjectNoticeSource();
    if (!isInit) {
        MMI_HILOGE("InitinjectNotice_ Source error");
        return false;
    }
    MMI_HILOGD("SendNotice submit begin");
    ffrt::submit([this, noticeInfo] {
        MMI_HILOGD("SendNotice submit enter");
        CHKPV(injectNotice_);
        auto pConnect = injectNotice_->GetConnection();
        CHKPV(pConnect);
        int32_t timeSecond = 0;
        while (timeSecond <= SEND_NOTICE_OVERTIME) {
            bool isConnect = pConnect->IsConnected();
            MMI_HILOGD("SendNotice %{public}d", isConnect);
            if (isConnect) {
                MMI_HILOGD("SendNotice begin");
                pConnect->SendNotice(noticeInfo);
                break;
            }
            timeSecond += 1;
            sleep(1);
        }
        MMI_HILOGD("SendNotice submit leave");
    });
    return true;
}

bool ServerMsgHandler::CloseInjectNotice(int32_t pid)
{
    CALL_DEBUG_ENTER;
    bool isInit = InitInjectNoticeSource();
    if (!isInit) {
        MMI_HILOGE("InitinjectNotice_ Source error");
        return false;
    }
    MMI_HILOGD("CloseNotice submit begin");
    InjectNoticeInfo noticeInfo;
    noticeInfo.pid = pid;
    ffrt::submit([this, noticeInfo] {
        MMI_HILOGD("CloseNotice submit enter");
        CHKPV(injectNotice_);
        auto pConnect = injectNotice_->GetConnection();
        CHKPV(pConnect);
        int32_t timeSecond = 0;
        while (timeSecond <= SEND_NOTICE_OVERTIME) {
            bool isConnect = pConnect->IsConnected();
            MMI_HILOGD("CloseNotice %{public}d", isConnect);
            if (isConnect) {
                MMI_HILOGD("CloseNotice begin");
                pConnect->CancelNotice(noticeInfo);
                break;
            }
            timeSecond += 1;
            sleep(1);
        }
        MMI_HILOGD("CloseNotice submit leave");
    });
    return true;
}

int32_t ServerMsgHandler::OnTransferBinderClientSrv(const sptr<IRemoteObject> &binderClientObject, int32_t pid)
{
    CALL_DEBUG_ENTER;
    bool bRet = clientDeathHandler_.RegisterClientDeathRecipient(binderClientObject, pid);
    if (!bRet) {
        MMI_HILOGE("Failed to registerClientDeathRecipient");
        return RET_ERR;
    }
    return ERR_OK;
}

int32_t ServerMsgHandler::NativeInjectCheck(int32_t pid)
{
    CALL_DEBUG_ENTER;
    if (PRODUCT_TYPE != PRODUCT_TYPE_PC) {
        MMI_HILOGW("Current device has no permission");
        return COMMON_PERMISSION_CHECK_ERROR;
    }
    bool screenLocked = DISPLAY_MONITOR->GetScreenLocked();
    if (screenLocked) {
        MMI_HILOGW("Screen locked, no permission");
        return COMMON_PERMISSION_CHECK_ERROR;
    }
    if (pid <= 0) {
        MMI_HILOGW("Invalid process id pid:%{public}d", pid);
        return COMMON_PERMISSION_CHECK_ERROR;
    }
    auto state = AUTHORIZE_HELPER->GetAuthorizeState();
    MMI_HILOGI("The process is already being processed,s:%{public}d,pid:%{public}d,inputPid:%{public}d",
        state, AUTHORIZE_HELPER->GetAuthorizePid(), pid);
    if (state == AuthorizeState::STATE_UNAUTHORIZE) {
        LaunchAbility();
        AuthorizeExitCallback fnCallback = [&] (int32_t pid) {
            MMI_HILOGI("User not authorized to inject pid:%{public}d", pid);
            AUTH_DIALOG.CloseDialog();
        };
        AUTHORIZE_HELPER->AddAuthorizeProcess(pid, fnCallback);
        return COMMON_PERMISSION_CHECK_ERROR;
    }

    if (state == AuthorizeState::STATE_SELECTION_AUTHORIZE) {
        if (pid == AUTHORIZE_HELPER->GetAuthorizePid()) {
            MMI_HILOGI("The current PID is waiting for user authorization");
        } else {
            MMI_HILOGI("Another PID is waiting for user authorization");
        }
        return COMMON_PERMISSION_CHECK_ERROR;
    }

    // Currently, a process is authorized.state is AuthorizeState::STATE_AUTHORIZE
    if (pid != AUTHORIZE_HELPER->GetAuthorizePid()) {
        MMI_HILOGI("Other processes have been authorized");
        return COMMON_PERMISSION_CHECK_ERROR;
    }
    return RET_OK;
}
} // namespace MMI
} // namespace OHOS
