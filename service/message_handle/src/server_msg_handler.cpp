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

#include "server_msg_handler.h"

#include <cinttypes>
#include "ipc_skeleton.h"

#include "ability_manager_client.h"
#include "anr_manager.h"
#include "authorization_dialog.h"
#include "authorize_helper.h"
#include "bytrace_adapter.h"
#include "client_death_handler.h"
#include "display_event_monitor.h"
#include "event_dump.h"
#include "event_interceptor_handler.h"
#include "event_monitor_handler.h"
#include "event_log_helper.h"
#include "hos_key_event.h"
#include "input_device_manager.h"
#include "input_event.h"
#include "input_event_data_transformation.h"
#include "input_event_handler.h"
#include "i_input_windows_manager.h"
#include "i_pointer_drawing_manager.h"
#include "key_event_normalize.h"
#include "key_event_value_transformation.h"
#include "key_subscriber_handler.h"
#include "libinput_adapter.h"
#include "parameters.h"
#include "switch_subscriber_handler.h"
#include "time_cost_chk.h"
#include "touch_drawing_manager.h"

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
const int32_t ROTATE_POLICY = system::GetIntParameter("const.window.device.rotate_policy", 0);
const std::string PRODUCT_TYPE = system::GetParameter("const.product.devicetype", "unknown");
const std::string PRODUCT_TYPE_PC = "2in1";
constexpr int32_t WINDOW_ROTATE { 0 };
constexpr int32_t COMMON_PERMISSION_CHECK_ERROR { 201 };
constexpr int32_t CAST_INPUT_DEVICEID { 0xAAAAAAFF };
} // namespace

void ServerMsgHandler::Init(UDSServer &udsServer)
{
    udsServer_ = &udsServer;
    MsgCallback funs[] = {
        {MmiMessageId::DISPLAY_INFO, [this] (SessionPtr sess, NetPacket &pkt) {
            return this->OnDisplayInfo(sess, pkt); }},
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
        {MmiMessageId::WINDOW_AREA_INFO, [this] (SessionPtr sess, NetPacket &pkt) {
            return this->OnWindowAreaInfo(sess, pkt); }},
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
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
    AUTHORIZE_HELPER->Init(clientDeathHandler_);
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
    LogTracer lt(keyEvent->GetId(), keyEvent->GetEventType(), keyEvent->GetKeyAction());
    if (isNativeInject) {
        int32_t checkReturn = NativeInjectCheck(pid);
        if (checkReturn != RET_OK) {
            return checkReturn;
        }
    }
    int32_t keyIntention = KeyItemsTransKeyIntention(keyEvent->GetKeyItems());
    keyEvent->SetKeyIntention(keyIntention);
    auto inputEventNormalizeHandler = InputHandler->GetEventNormalizeHandler();
    CHKPR(inputEventNormalizeHandler, ERROR_NULL_POINTER);
    inputEventNormalizeHandler->HandleKeyEvent(keyEvent);
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
    const auto &keyEvent = KeyEventHdr->GetKeyEvent();
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    state = keyEvent->GetFunctionKey(funcKey);
    MMI_HILOGD("Get the function key:%{public}d status as %{public}s", funcKey, state ? "open" : "close");
    return RET_OK;
}

int32_t ServerMsgHandler::OnSetFunctionKeyState(int32_t funcKey, bool enable)
{
    CALL_INFO_TRACE;
    auto device = INPUT_DEV_MGR->GetKeyboardDevice();
    CHKPR(device, ERROR_NULL_POINTER);
    if (LibinputAdapter::DeviceLedUpdate(device, funcKey, enable) != RET_OK) {
        MMI_HILOGE("Failed to set the keyboard led");
        return RET_ERR;
    }
    int32_t state = libinput_get_funckey_state(device, funcKey);

    auto keyEvent = KeyEventHdr->GetKeyEvent();
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    int32_t ret = keyEvent->SetFunctionKey(funcKey, state);
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
    bool isNativeInject, bool isShell)
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
    return OnInjectPointerEventExt(pointerEvent, isShell);
}

bool ServerMsgHandler::IsNavigationWindowInjectEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    return pointerEvent->GetZOrder() > 0;
}

int32_t ServerMsgHandler::OnInjectPointerEventExt(const std::shared_ptr<PointerEvent> pointerEvent, bool isShell)
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
            if (isShell) {
                pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SHELL);
            }
            if (!FixTargetWindowId(pointerEvent, pointerEvent->GetPointerAction(), isShell)) {
                return RET_ERR;
            }
            inputEventNormalizeHandler->HandleTouchEvent(pointerEvent);
            if (!pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY) &&
                !(pointerEvent->GetDeviceId() == CAST_INPUT_DEVICEID) &&
                !IsNavigationWindowInjectEvent(pointerEvent)) {
                TOUCH_DRAWING_MGR->TouchDrawHandler(pointerEvent);
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
            inputEventNormalizeHandler->HandlePointerEvent(pointerEvent);
            CHKPR(pointerEvent, ERROR_NULL_POINTER);
            if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY)) {
                break;
            } else if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_HIDE_POINTER)) {
                IPointerDrawingManager::GetInstance()->SetMouseDisplayState(false);
            } else if (((pointerEvent->GetPointerAction() < PointerEvent::POINTER_ACTION_PULL_DOWN) ||
                (pointerEvent->GetPointerAction() > PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW)) &&
                !IPointerDrawingManager::GetInstance()->IsPointerVisible()) {
                IPointerDrawingManager::GetInstance()->SetPointerVisible(getpid(), true, 0, false);
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
int32_t ServerMsgHandler::AccelerateMotion(std::shared_ptr<PointerEvent> pointerEvent)
{
    if (!pointerEvent->HasFlag(InputEvent::EVENT_FLAG_RAW_POINTER_MOVEMENT) ||
        (pointerEvent->GetSourceType() != PointerEvent::SOURCE_TYPE_MOUSE) ||
        ((pointerEvent->GetPointerAction() != PointerEvent::POINTER_ACTION_MOVE) &&
         (pointerEvent->GetPointerAction() != PointerEvent::POINTER_ACTION_PULL_MOVE))) {
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
    if (TOUCH_DRAWING_MGR->IsWindowRotation()) {
        CalculateOffset(displayInfo->direction, offset);
    }
#endif // OHOS_BUILD_EMULATOR
    int32_t ret = RET_OK;

    if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_TOUCHPAD_POINTER)) {
        ret = HandleMotionAccelerateTouchpad(&offset, WIN_MGR->GetMouseIsCaptureMode(),
            &cursorPos.cursorPos.x, &cursorPos.cursorPos.y,
            MouseTransformProcessor::GetTouchpadSpeed(), static_cast<int32_t>(DeviceType::DEVICE_PC));
    } else {
        ret = HandleMotionAccelerateMouse(&offset, WIN_MGR->GetMouseIsCaptureMode(),
            &cursorPos.cursorPos.x, &cursorPos.cursorPos.y,
            MouseTransformProcessor::GetPointerSpeed(), static_cast<int32_t>(DeviceType::DEVICE_PC));
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
    if (!pointerEvent->HasFlag(InputEvent::EVENT_FLAG_RAW_POINTER_MOVEMENT) ||
        (pointerEvent->GetSourceType() != PointerEvent::SOURCE_TYPE_MOUSE)) {
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
    pointerEvent->UpdatePointerItem(pointerEvent->GetPointerId(), pointerItem);
    pointerEvent->SetTargetDisplayId(mouseInfo.displayId);
}

int32_t ServerMsgHandler::SaveTargetWindowId(std::shared_ptr<PointerEvent> pointerEvent, bool isShell)
{
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    if ((pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) &&
        (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_DOWN ||
        pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_HOVER_ENTER)) {
        int32_t pointerId = pointerEvent->GetPointerId();
        PointerEvent::PointerItem pointerItem;
        if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
            MMI_HILOGE("Can't find pointer item, pointer:%{public}d", pointerId);
            return RET_ERR;
        }
        int32_t targetWindowId = pointerEvent->GetTargetWindowId();
        if (isShell) {
            shellTargetWindowIds_[pointerId] = targetWindowId;
        } else if ((pointerEvent->GetDeviceId() == CAST_INPUT_DEVICEID) && (pointerEvent->GetZOrder() > 0)) {
            castTargetWindowIds_[pointerId] = targetWindowId;
        } else if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY)) {
            accessTargetWindowIds_[pointerId] = targetWindowId;
        } else {
            nativeTargetWindowIds_[pointerId] = targetWindowId;
        }
    }
    if ((pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) &&
        (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_UP ||
        pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_HOVER_EXIT)) {
        int32_t pointerId = pointerEvent->GetPointerId();
        if (isShell) {
            shellTargetWindowIds_.erase(pointerId);
        } else if ((pointerEvent->GetDeviceId() == CAST_INPUT_DEVICEID) && (pointerEvent->GetZOrder() > 0)) {
            castTargetWindowIds_.erase(pointerId);
        } else if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY)) {
            accessTargetWindowIds_.erase(pointerId);
        } else {
            nativeTargetWindowIds_.erase(pointerId);
        }
    }
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

#ifdef OHOS_BUILD_ENABLE_TOUCH
bool ServerMsgHandler::FixTargetWindowId(std::shared_ptr<PointerEvent> pointerEvent,
    int32_t action, bool isShell)
{
    int32_t targetWindowId = -1;
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
        MMI_HILOGE("Can't find pointer item, pointer:%{public}d", pointerId);
        return false;
    }
    if (isShell) {
        auto iter = shellTargetWindowIds_.find(pointerEvent->GetPointerId());
        if (iter != shellTargetWindowIds_.end()) {
            targetWindowId = iter->second;
        }
    } else if ((pointerEvent->GetDeviceId() == CAST_INPUT_DEVICEID) && (pointerEvent->GetZOrder() > 0)) {
        pointerEvent->RemovePointerItem(pointerId);
        pointerId += CAST_POINTER_ID;
        pointerItem.SetPointerId(pointerId);
        pointerEvent->UpdatePointerItem(pointerId, pointerItem);
        pointerEvent->SetPointerId(pointerId);
        auto iter = castTargetWindowIds_.find(pointerEvent->GetPointerId());
        if (iter != castTargetWindowIds_.end()) {
            targetWindowId = iter->second;
        }
    } else if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY)) {
        auto iter = accessTargetWindowIds_.find(pointerEvent->GetPointerId());
        if (iter != accessTargetWindowIds_.end()) {
            targetWindowId = iter->second;
        }
    } else {
        pointerEvent->RemovePointerItem(pointerId);
        pointerId += DEFAULT_POINTER_ID;
        pointerItem.SetPointerId(pointerId);
        pointerEvent->UpdatePointerItem(pointerId, pointerItem);
        pointerEvent->SetPointerId(pointerId);
        auto iter = nativeTargetWindowIds_.find(pointerEvent->GetPointerId());
        if (iter != nativeTargetWindowIds_.end()) {
            targetWindowId = iter->second;
        }
    }
    MMI_HILOGD("TargetWindowId:%{public}d %{public}d", pointerEvent->GetTargetWindowId(), targetWindowId);
    return UpdateTouchEvent(pointerEvent, action, targetWindowId);
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
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read display info failed");
        return RET_ERR;
    }
    for (uint32_t i = 0; i < num; i++) {
        WindowInfo extensionInfo;
        pkt >> extensionInfo.id >> extensionInfo.pid >> extensionInfo.uid >> extensionInfo.area
            >> extensionInfo.defaultHotAreas >> extensionInfo.pointerHotAreas >> extensionInfo.agentWindowId
            >> extensionInfo.flags >> extensionInfo.action >> extensionInfo.displayId >> extensionInfo.zOrder
            >> extensionInfo.pointerChangeAreas >> extensionInfo.transform >> extensionInfo.windowInputType
            >> extensionInfo.privacyMode >> extensionInfo.windowType >> extensionInfo.privacyUIFlag
            >> extensionInfo.rectChangeBySystem;
        info.uiExtentionWindowInfo.push_back(extensionInfo);
        if (pkt.ChkRWError()) {
            MMI_HILOGE("Packet read extention window info failed");
            return RET_ERR;
        }
    }
    return RET_OK;
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
    DisplayGroupInfo displayGroupInfo;
    pkt >> displayGroupInfo.width >> displayGroupInfo.height >>
        displayGroupInfo.focusWindowId >> displayGroupInfo.currentUserId;
    uint32_t num = 0;
    pkt >> num;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read display info failed");
        return RET_ERR;
    }
    for (uint32_t i = 0; i < num; i++) {
        WindowInfo info;
        int32_t byteCount = 0;
        pkt >> info.id >> info.pid >> info.uid >> info.area >> info.defaultHotAreas
            >> info.pointerHotAreas >> info.agentWindowId >> info.flags >> info.action
            >> info.displayId >> info.zOrder >> info.pointerChangeAreas >> info.transform
            >> info.windowInputType >> info.privacyMode >> info.windowType >> byteCount;

        OnUiExtentionWindowInfo(pkt, info);
        pkt >> info.rectChangeBySystem;
        displayGroupInfo.windowsInfo.push_back(info);
        if (pkt.ChkRWError()) {
            MMI_HILOGE("Packet read display info failed");
            return RET_ERR;
        }
    }
    pkt >> num;
    for (uint32_t i = 0; i < num; i++) {
        DisplayInfo info;
        pkt >> info.id >> info.x >> info.y >> info.width >> info.height >> info.dpi >> info.name
            >> info.uniq >> info.direction >> info.displayDirection >> info.displayMode >> info.transform;
        displayGroupInfo.displaysInfo.push_back(info);
        if (pkt.ChkRWError()) {
            MMI_HILOGE("Packet read display info failed");
            return RET_ERR;
        }
    }
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read display info failed");
        return RET_ERR;
    }
    WIN_MGR->UpdateDisplayInfoExtIfNeed(displayGroupInfo, true);
    return RET_OK;
}

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
int32_t ServerMsgHandler::OnWindowAreaInfo(SessionPtr sess, NetPacket &pkt)
{
    CALL_DEBUG_ENTER;
    CHKPR(sess, ERROR_NULL_POINTER);
    int32_t temp = 0;
    int32_t pid = 0;
    int32_t windowId = 0;
    pkt >> temp >> pid >> windowId;
    WindowArea area = static_cast<WindowArea>(temp);
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read display info failed");
        return RET_ERR;
    }
    WIN_MGR->SetWindowPointerStyle(area, pid, windowId);
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING

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
    uint32_t num = 0;
    pkt >> num;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read window group info failed");
        return RET_ERR;
    }
    for (uint32_t i = 0; i < num; i++) {
        WindowInfo info;
        pkt >> info.id >> info.pid >> info.uid >> info.area >> info.defaultHotAreas
            >> info.pointerHotAreas >> info.agentWindowId >> info.flags >> info.action
            >> info.displayId >> info.zOrder >> info.pointerChangeAreas >> info.transform
            >> info.windowInputType >> info.privacyMode >> info.windowType;
        OnUiExtentionWindowInfo(pkt, info);
        pkt >> info.rectChangeBySystem;
        windowGroupInfo.windowsInfo.push_back(info);
        if (pkt.ChkRWError()) {
            MMI_HILOGE("Packet read display info failed");
            return RET_ERR;
        }
    }
    WIN_MGR->UpdateWindowInfo(windowGroupInfo);
    return RET_OK;
}

int32_t ServerMsgHandler::RegisterWindowStateErrorCallback(SessionPtr sess, NetPacket &pkt)
{
    CALL_DEBUG_ENTER;
    int32_t pid = sess->GetPid();
    WIN_MGR->SetWindowStateNotifyPid(pid);
    MMI_HILOGI("pid:%{public}d", pid);
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
    uint8_t cfg[num];
    for (uint32_t i = 0; i < num; i++) {
        pkt >> cfg[i];
    }

    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read scinfo config failed");
        return RET_ERR;
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
    MMI_HILOGD("handlerType:%{public}d", handlerType);
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
#endif // OHOS_BUILD_ENABLE_KEYBOARD

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
#endif // OHOS_BUILD_ENABLE_SWITCH

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
    AUTH_DIALOG.ConnectSystemUi();
}

int32_t ServerMsgHandler::OnAuthorize(bool isAuthorize)
{
    CALL_DEBUG_ENTER;
    if (isAuthorize) {
        auto state = AUTHORIZE_HELPER->GetAuthorizeState();
        int32_t authorPid = AUTHORIZE_HELPER->GetAuthorizePid();
        MMI_HILOGE("OnAuthorize  not has authorizing s:%{public}d, authPid:%{public}d",
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
        auto result = AUTHORIZE_HELPER->AddAuthorizeProcess(authorPid,
            [&] (int32_t pid) {
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
            MMI_HILOGW("Authorized pid not callPid.");
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
        MMI_HILOGI("Other processes have been authorized.");
        return COMMON_PERMISSION_CHECK_ERROR;
    }
    return RET_OK;
}
} // namespace MMI
} // namespace OHOS
