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

#include "server_msg_handler.h"

#include <cinttypes>

#include "anr_manager.h"
#include "event_dump.h"
#include "event_interceptor_handler.h"
#include "event_monitor_handler.h"
#include "hos_key_event.h"
#include "input_device_manager.h"
#include "input_event.h"
#include "input_event_data_transformation.h"
#include "input_event_handler.h"
#include "input_windows_manager.h"
#include "i_pointer_drawing_manager.h"
#include "key_event_normalize.h"
#include "key_subscriber_handler.h"
#include "libinput_adapter.h"
#include "mmi_func_callback.h"
#include "mouse_event_normalize.h"
#include "switch_subscriber_handler.h"
#include "time_cost_chk.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "ServerMsgHandler" };
#ifdef OHOS_BUILD_ENABLE_SECURITY_COMPONENT
constexpr int32_t SECURITY_COMPONENT_SERVICE_ID = 3050;
#endif
} // namespace

void ServerMsgHandler::Init(UDSServer& udsServer)
{
    udsServer_ = &udsServer;
    MsgCallback funs[] = {
        {MmiMessageId::DISPLAY_INFO, MsgCallbackBind2(&ServerMsgHandler::OnDisplayInfo, this)},
        {MmiMessageId::WINDOW_AREA_INFO, MsgCallbackBind2(&ServerMsgHandler::OnWindowAreaInfo, this)},
        {MmiMessageId::WINDOW_INFO, MsgCallbackBind2(&ServerMsgHandler::OnWindowGroupInfo, this)},
#ifdef OHOS_BUILD_ENABLE_SECURITY_COMPONENT
        {MmiMessageId::SCINFO_CONFIG, MsgCallbackBind2(&ServerMsgHandler::OnEnhanceConfig, this)},
#endif // OHOS_BUILD_ENABLE_SECURITY_COMPONENT

    };
    for (auto &it : funs) {
        if (!RegistrationEvent(it)) {
            MMI_HILOGW("Failed to register event errCode:%{public}d", EVENT_REG_FAIL);
            continue;
        }
    }
}

void ServerMsgHandler::OnMsgHandler(SessionPtr sess, NetPacket& pkt)
{
    CHKPV(sess);
    auto id = pkt.GetMsgId();
    TimeCostChk chk("ServerMsgHandler::OnMsgHandler", "overtime 300(us)", MAX_OVER_TIME, id);
    auto callback = GetMsgCallback(id);
    if (callback == nullptr) {
        MMI_HILOGE("Unknown msg id:%{public}d,errCode:%{public}d", id, UNKNOWN_MSG_ID);
        return;
    }
    auto ret = (*callback)(sess, pkt);
    if (ret < 0) {
        MMI_HILOGE("Msg handling failed. id:%{public}d,errCode:%{public}d", id, ret);
    }
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
int32_t ServerMsgHandler::OnInjectKeyEvent(const std::shared_ptr<KeyEvent> keyEvent)
{
    CALL_DEBUG_ENTER;
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    auto inputEventNormalizeHandler = InputHandler->GetEventNormalizeHandler();
    CHKPR(inputEventNormalizeHandler, ERROR_NULL_POINTER);
    inputEventNormalizeHandler->HandleKeyEvent(keyEvent);
    MMI_HILOGD("Inject keyCode:%{public}d, action:%{public}d", keyEvent->GetKeyCode(), keyEvent->GetKeyAction());
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
    auto device = InputDevMgr->GetKeyboardDevice();
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
int32_t ServerMsgHandler::OnInjectPointerEvent(const std::shared_ptr<PointerEvent> pointerEvent)
{
    CALL_DEBUG_ENTER;
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    pointerEvent->UpdateId();
    int32_t action = pointerEvent->GetPointerAction();
    auto source = pointerEvent->GetSourceType();
    switch (source) {
        case PointerEvent::SOURCE_TYPE_TOUCHSCREEN: {
#ifdef OHOS_BUILD_ENABLE_TOUCH
            auto inputEventNormalizeHandler = InputHandler->GetEventNormalizeHandler();
            CHKPR(inputEventNormalizeHandler, ERROR_NULL_POINTER);
            if (!FixTargetWindowId(pointerEvent, action)) {
                return RET_ERR;
            }
            inputEventNormalizeHandler->HandleTouchEvent(pointerEvent);
#endif // OHOS_BUILD_ENABLE_TOUCH
            break;
        }
        case PointerEvent::SOURCE_TYPE_MOUSE:
#ifdef OHOS_BUILD_ENABLE_JOYSTICK
        case PointerEvent::SOURCE_TYPE_JOYSTICK:
#endif // OHOS_BUILD_ENABLE_JOYSTICK
        case PointerEvent::SOURCE_TYPE_TOUCHPAD: {
#ifdef OHOS_BUILD_ENABLE_POINTER
            auto inputEventNormalizeHandler = InputHandler->GetEventNormalizeHandler();
            CHKPR(inputEventNormalizeHandler, ERROR_NULL_POINTER);
            if ((action < PointerEvent::POINTER_ACTION_PULL_DOWN ||
                action > PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW) &&
                !IPointerDrawingManager::GetInstance()->IsPointerVisible()) {
                IPointerDrawingManager::GetInstance()->SetPointerVisible(getpid(), true);
            }
            inputEventNormalizeHandler->HandlePointerEvent(pointerEvent);
#endif // OHOS_BUILD_ENABLE_POINTER
            break;
        }
        default: {
            MMI_HILOGW("Source type is unknown, source:%{public}d", source);
            break;
        }
    }
    if (source == PointerEvent::SOURCE_TYPE_TOUCHSCREEN && action == PointerEvent::POINTER_ACTION_DOWN) {
        int32_t pointerId = pointerEvent->GetPointerId();
        PointerEvent::PointerItem pointerItem;
        if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
            MMI_HILOGE("Can't find pointer item, pointer:%{public}d", pointerId);
            return RET_ERR;
        }
        int32_t targetWindowId = pointerEvent->GetTargetWindowId();
        targetWindowIds_[pointerId] = targetWindowId;
    }
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

#ifdef OHOS_BUILD_ENABLE_TOUCH
bool ServerMsgHandler::FixTargetWindowId(std::shared_ptr<PointerEvent> pointerEvent, int32_t action)
{
    int32_t targetWindowId = -1;
    auto iter = targetWindowIds_.find(pointerEvent->GetPointerId());
    if (iter != targetWindowIds_.end()) {
        targetWindowId = iter->second;
    }
    if (action == PointerEvent::POINTER_ACTION_DOWN || targetWindowId < 0) {
        MMI_HILOGD("Down event or targetWindowId less 0 is not need fix window id");
        return true;
    }
    auto pointerIds = pointerEvent->GetPointerIds();
    if (pointerIds.empty()) {
        MMI_HILOGE("GetPointerIds is empty");
        return false;
    }
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
        MMI_HILOGE("Can't find pointer item, pointer:%{public}d", pointerId);
        return false;
    }
    pointerEvent->SetTargetWindowId(targetWindowId);
    pointerItem.SetTargetWindowId(targetWindowId);
    pointerEvent->UpdatePointerItem(pointerId, pointerItem);
    return true;
}
#endif // OHOS_BUILD_ENABLE_TOUCH

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
    pkt >> displayGroupInfo.width >> displayGroupInfo.height >> displayGroupInfo.focusWindowId;
    uint32_t num = 0;
    pkt >> num;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read display info failed");
        return RET_ERR;
    }
    for (uint32_t i = 0; i < num; i++) {
        WindowInfo info;
        pkt >> info.id >> info.pid >> info.uid >> info.area >> info.defaultHotAreas
            >> info.pointerHotAreas >> info.agentWindowId >> info.flags >> info.action
            >> info.displayId >> info.zOrder >> info.pointerChangeAreas >> info.transform;
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
            >> info.uniq >> info.direction >> info.displayDirection >> info.displayMode;
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
    WinMgr->UpdateDisplayInfoExtIfNeed(displayGroupInfo, true);
    return RET_OK;
}

int32_t ServerMsgHandler::OnWindowAreaInfo(SessionPtr sess, NetPacket &pkt)
{
    CALL_DEBUG_ENTER;
    CHKPR(sess, ERROR_NULL_POINTER);
    int32_t temp;
    int32_t pid;
    int32_t windowId;
    pkt >> temp >> pid >> windowId;
    WindowArea area = static_cast<WindowArea>(temp);
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read display info failed");
        return RET_ERR;
    }
    WinMgr->SetWindowPointerStyle(area, pid, windowId);
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
            >> info.displayId >> info.zOrder >> info.pointerChangeAreas >> info.transform;
        windowGroupInfo.windowsInfo.push_back(info);
        if (pkt.ChkRWError()) {
            MMI_HILOGE("Packet read display info failed");
            return RET_ERR;
        }
    }
    
    WinMgr->UpdateWindowInfo(windowGroupInfo);
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
int32_t ServerMsgHandler::OnSubscribeSwitchEvent(IUdsServer *server, int32_t pid, int32_t subscribeId)
{
    CALL_DEBUG_ENTER;
    CHKPR(server, ERROR_NULL_POINTER);
    auto sess = server->GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    auto subscriberHandler = InputHandler->GetSwitchSubscriberHandler();
    CHKPR(subscriberHandler, ERROR_NULL_POINTER);
    return subscriberHandler->SubscribeSwitchEvent(sess, subscribeId);
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

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
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
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

int32_t ServerMsgHandler::SetShieldStatus(int32_t shieldMode, bool isShield)
{
    return KeyEventHdr->SetShieldStatus(shieldMode, isShield);
}

int32_t ServerMsgHandler::GetShieldStatus(int32_t shieldMode, bool &isShield)
{
    return KeyEventHdr->GetShieldStatus(shieldMode, isShield);
}
} // namespace MMI
} // namespace OHOS
