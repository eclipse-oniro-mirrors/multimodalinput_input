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

#include "client_msg_handler.h"

#include <cinttypes>
#include <iostream>
#include <sstream>

#include "bytrace_adapter.h"
#include "event_log_helper.h"
#include "input_device.h"
#include "input_device_impl.h"
#include "input_event_data_transformation.h"
#include "input_handler_manager.h"
#include "input_manager_impl.h"
#include "anr_handler.h"
#ifdef OHOS_BUILD_ENABLE_MONITOR
#include "input_monitor_manager.h"
#endif // OHOS_BUILD_ENABLE_MONITOR
#include "mmi_client.h"
#include "mmi_func_callback.h"
#include "multimodal_event_handler.h"
#include "multimodal_input_connect_manager.h"
#include "napi_constants.h"
#include "proto.h"
#include "time_cost_chk.h"
#include "util.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "ClientMsgHandler" };
} // namespace

void ClientMsgHandler::Init()
{
    MsgCallback funs[] = {
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
        { MmiMessageId::ON_KEY_EVENT, MsgCallbackBind2(&ClientMsgHandler::OnKeyEvent, this) },
        { MmiMessageId::ON_SUBSCRIBE_KEY, std::bind(&ClientMsgHandler::OnSubscribeKeyEventCallback,
            this, std::placeholders::_1, std::placeholders::_2) },
#endif // OHOS_BUILD_ENABLE_KEYBOARD
#ifdef OHOS_BUILD_ENABLE_SWITCH
        { MmiMessageId::ON_SUBSCRIBE_SWITCH, std::bind(&ClientMsgHandler::OnSubscribeSwitchEventCallback,
            this, std::placeholders::_1, std::placeholders::_2) },
#endif // OHOS_BUILD_ENABLE_SWITCH
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
        { MmiMessageId::ON_POINTER_EVENT, MsgCallbackBind2(&ClientMsgHandler::OnPointerEvent, this) },
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
        { MmiMessageId::ADD_INPUT_DEVICE_LISTENER, MsgCallbackBind2(&ClientMsgHandler::OnDevListener, this) },
        { MmiMessageId::NOTICE_ANR, MsgCallbackBind2(&ClientMsgHandler::OnAnr, this) },
#if defined(OHOS_BUILD_ENABLE_KEYBOARD) && (defined(OHOS_BUILD_ENABLE_INTERCEPTOR) || \
    defined(OHOS_BUILD_ENABLE_MONITOR))
        { MmiMessageId::REPORT_KEY_EVENT, MsgCallbackBind2(&ClientMsgHandler::ReportKeyEvent, this) },
#endif // OHOS_BUILD_ENABLE_KEYBOARD
#if (defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)) && \
    (defined(OHOS_BUILD_ENABLE_INTERCEPTOR) || defined(OHOS_BUILD_ENABLE_MONITOR))
        { MmiMessageId::REPORT_POINTER_EVENT, MsgCallbackBind2(&ClientMsgHandler::ReportPointerEvent, this) },
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
        { MmiMessageId::NOTIFY_BUNDLE_NAME, MsgCallbackBind2(&ClientMsgHandler::NotifyBundleName, this) },
    };
    for (auto &it : funs) {
        if (!RegistrationEvent(it)) {
            MMI_HILOGW("Failed to register event errCode:%{public}d", EVENT_REG_FAIL);
            continue;
        }
    }
}

void ClientMsgHandler::InitProcessedCallback()
{
    CALL_DEBUG_ENTER;
    int32_t tokenType = MultimodalInputConnMgr->GetTokenType();
    if (tokenType == TokenType::TOKEN_HAP || tokenType == TokenType::TOKEN_SYSTEM_HAP) {
        MMI_HILOGD("Current session is hap");
        dispatchCallback_ = std::bind(&ClientMsgHandler::OnDispatchEventProcessed, std::placeholders::_1,
            std::placeholders::_2);
    } else if (tokenType == static_cast<int32_t>(TokenType::TOKEN_NATIVE)) {
        MMI_HILOGD("Current session is native");
    } else {
        MMI_HILOGE("Current session is unknown tokenType:%{public}d", tokenType);
    }
}

void ClientMsgHandler::OnMsgHandler(const UDSClient& client, NetPacket& pkt)
{
    auto id = pkt.GetMsgId();
    TimeCostChk chk("ClientMsgHandler::OnMsgHandler", "overtime 300(us)", MAX_OVER_TIME, id);
    auto callback = GetMsgCallback(id);
    if (callback == nullptr) {
        MMI_HILOGE("Unknown msg id:%{public}d", id);
        return;
    }
    auto ret = (*callback)(client, pkt);
    if (ret < 0) {
        MMI_HILOGE("Msg handling failed. id:%{public}d,ret:%{public}d", id, ret);
        return;
    }
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
int32_t ClientMsgHandler::OnKeyEvent(const UDSClient& client, NetPacket& pkt)
{
    auto key = KeyEvent::Create();
    CHKPR(key, ERROR_NULL_POINTER);
    int32_t ret = InputEventDataTransformation::NetPacketToKeyEvent(pkt, key);
    if (ret != RET_OK) {
        MMI_HILOGE("Read netPacket failed");
        return RET_ERR;
    }
    int32_t fd = 0;
    pkt >> fd;
#ifdef OHOS_BUILD_ENABLE_SECURITY_COMPONENT
    if (InputEventDataTransformation::UnmarshallingEnhanceData(pkt, key) != ERR_OK) {
        MMI_HILOGE("Failed to deserialize enhance data key event.");
        return RET_ERR;
    }
#endif // OHOS_BUILD_ENABLE_SECURITY_COMPONENT
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read fd failed");
        return PACKET_READ_FAIL;
    }
    MMI_HILOGD("Key event dispatcher of client, Fd:%{public}d", fd);
    MMI_HILOGI("InputTracking id:%{public}d KeyEvent ReceivedMsg", key->GetId());
    EventLogHelper::PrintEventData(key);
    BytraceAdapter::StartBytrace(key, BytraceAdapter::TRACE_START, BytraceAdapter::KEY_DISPATCH_EVENT);
    key->SetProcessedCallback(dispatchCallback_);
    InputMgrImpl.OnKeyEvent(key);
    key->MarkProcessed();
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

int32_t ClientMsgHandler::NotifyBundleName(const UDSClient& client, NetPacket& pkt)
{
    CALL_DEBUG_ENTER;
    int32_t pid;
    int32_t uid;
    int32_t syncStatus;
    std::string bundleName;
    pkt >> pid >> uid >> bundleName >> syncStatus;
    InputMgrImpl.NotifyBundleName(pid, uid, bundleName, syncStatus);
    MMI_HILOGD("client info in NotifyBundleName is : %{public}d, %{public}d, %{public}s, %{public}d",
        pid, uid, bundleName.c_str(), syncStatus);
    return RET_OK;
}

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
int32_t ClientMsgHandler::OnPointerEvent(const UDSClient& client, NetPacket& pkt)
{
    CALL_DEBUG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    if (InputEventDataTransformation::Unmarshalling(pkt, pointerEvent) != ERR_OK) {
        MMI_HILOGE("Failed to deserialize pointer event.");
        return RET_ERR;
    }
#ifdef OHOS_BUILD_ENABLE_SECURITY_COMPONENT
    if (InputEventDataTransformation::UnmarshallingEnhanceData(pkt, pointerEvent) != ERR_OK) {
        MMI_HILOGE("Failed to deserialize enhance data pointer event.");
        return RET_ERR;
    }
#endif // OHOS_BUILD_ENABLE_SECURITY_COMPONENT
    MMI_HILOGI("InputTracking id:%{public}d PointerEvent ReceivedMsg", pointerEvent->GetId());
    EventLogHelper::PrintEventData(pointerEvent);
    if (PointerEvent::POINTER_ACTION_CANCEL == pointerEvent->GetPointerAction()) {
        MMI_HILOGI("Operation canceled.");
    }
    pointerEvent->SetProcessedCallback(dispatchCallback_);
    BytraceAdapter::StartBytrace(pointerEvent, BytraceAdapter::TRACE_START, BytraceAdapter::POINT_DISPATCH_EVENT);
    InputMgrImpl.OnPointerEvent(pointerEvent);
    if (pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_JOYSTICK) {
        pointerEvent->MarkProcessed();
    }
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
int32_t ClientMsgHandler::OnSubscribeKeyEventCallback(const UDSClient &client, NetPacket &pkt)
{
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    int32_t ret = InputEventDataTransformation::NetPacketToKeyEvent(pkt, keyEvent);
    if (ret != RET_OK) {
        MMI_HILOGE("Read net packet failed");
        return RET_ERR;
    }
    int32_t fd = -1;
    int32_t subscribeId = -1;
    pkt >> fd >> subscribeId;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read fd failed");
        return PACKET_READ_FAIL;
    }
    if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_POWER) {
        MMI_HILOGI("Subscribe:%{public}d,Fd:%{public}d,KeyEvent:%{public}d,"
            "KeyCode:%{public}d,ActionTime:%{public}" PRId64 ",ActionStartTime:%{public}" PRId64 ","
            "Action:%{public}d,KeyAction:%{public}d,EventType:%{public}d,Flag:%{public}u",
        subscribeId, fd, keyEvent->GetId(), keyEvent->GetKeyCode(), keyEvent->GetActionTime(),
        keyEvent->GetActionStartTime(), keyEvent->GetAction(), keyEvent->GetKeyAction(),
        keyEvent->GetEventType(), keyEvent->GetFlag());
    } else {
        MMI_HILOGD("Subscribe:%{public}d,Fd:%{public}d,KeyEvent:%{public}d,"
            "KeyCode:%{public}d,ActionTime:%{public}" PRId64 ",ActionStartTime:%{public}" PRId64 ","
            "Action:%{public}d,KeyAction:%{public}d,EventType:%{public}d,Flag:%{public}u",
        subscribeId, fd, keyEvent->GetId(), keyEvent->GetKeyCode(), keyEvent->GetActionTime(),
        keyEvent->GetActionStartTime(), keyEvent->GetAction(), keyEvent->GetKeyAction(),
        keyEvent->GetEventType(), keyEvent->GetFlag());
    }

    BytraceAdapter::StartBytrace(keyEvent, BytraceAdapter::TRACE_START, BytraceAdapter::KEY_SUBSCRIBE_EVENT);
    return KeyEventInputSubscribeMgr.OnSubscribeKeyEventCallback(keyEvent, subscribeId);
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

#ifdef OHOS_BUILD_ENABLE_SWITCH
int32_t ClientMsgHandler::OnSubscribeSwitchEventCallback(const UDSClient &client, NetPacket &pkt)
{
    std::shared_ptr<SwitchEvent> switchEvent = std::make_shared<SwitchEvent>(0);
    int32_t ret = InputEventDataTransformation::NetPacketToSwitchEvent(pkt, switchEvent);
    if (ret != RET_OK) {
        MMI_HILOGE("Read net packet failed");
        return RET_ERR;
    }
    int32_t fd = -1;
    int32_t subscribeId = -1;
    pkt >> fd >> subscribeId;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read fd failed");
        return PACKET_READ_FAIL;
    }
    return SWITCH_EVENT_INPUT_SUBSCRIBE_MGR.OnSubscribeSwitchEventCallback(switchEvent, subscribeId);
}
#endif

int32_t ClientMsgHandler::OnDevListener(const UDSClient& client, NetPacket& pkt)
{
    CALL_DEBUG_ENTER;
    std::string type;
    int32_t deviceId;
    pkt >> type >> deviceId;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read type failed");
        return RET_ERR;
    }
    InputDevImpl.OnDevListener(deviceId, type);
    return RET_OK;
}

#if defined(OHOS_BUILD_ENABLE_KEYBOARD) && (defined(OHOS_BUILD_ENABLE_INTERCEPTOR) || \
    defined(OHOS_BUILD_ENABLE_MONITOR))
int32_t ClientMsgHandler::ReportKeyEvent(const UDSClient& client, NetPacket& pkt)
{
    CALL_DEBUG_ENTER;
    InputHandlerType handlerType;
    uint32_t deviceTags;
    pkt >> handlerType >> deviceTags;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read handler failed");
        return RET_ERR;
    }
    auto keyEvent = KeyEvent::Create();
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    if (InputEventDataTransformation::NetPacketToKeyEvent(pkt, keyEvent) != ERR_OK) {
        MMI_HILOGE("Failed to deserialize key event.");
        return RET_ERR;
    }
    BytraceAdapter::StartBytrace(keyEvent, BytraceAdapter::TRACE_START, BytraceAdapter::KEY_INTERCEPT_EVENT);
    switch (handlerType) {
        case INTERCEPTOR: {
#ifdef OHOS_BUILD_ENABLE_INTERCEPTOR
            InputInterMgr->OnInputEvent(keyEvent, deviceTags);
#endif // OHOS_BUILD_ENABLE_INTERCEPTOR
            break;
        }
        case MONITOR: {
#ifdef OHOS_BUILD_ENABLE_MONITOR
            IMonitorMgr->OnInputEvent(keyEvent, deviceTags);
#endif // OHOS_BUILD_ENABLE_MONITOR
            break;
        }
        default: {
            MMI_HILOGW("Failed to intercept or monitor on the event");
            break;
        }
    }
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD && OHOS_BUILD_ENABLE_INTERCEPTOR || OHOS_BUILD_ENABLE_MONITOR

#if (defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)) && \
    (defined(OHOS_BUILD_ENABLE_INTERCEPTOR) || defined(OHOS_BUILD_ENABLE_MONITOR))
int32_t ClientMsgHandler::ReportPointerEvent(const UDSClient& client, NetPacket& pkt)
{
    InputHandlerType handlerType;
    uint32_t deviceTags;
    pkt >> handlerType >> deviceTags;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read Pointer data failed");
        return RET_ERR;
    }
    MMI_HILOGD("Client handlerType:%{public}d", handlerType);
    auto pointerEvent = PointerEvent::Create();
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    if (InputEventDataTransformation::Unmarshalling(pkt, pointerEvent) != ERR_OK) {
        MMI_HILOGE("Failed to deserialize pointer event");
        return RET_ERR;
    }
    BytraceAdapter::StartBytrace(pointerEvent, BytraceAdapter::TRACE_START, BytraceAdapter::POINT_INTERCEPT_EVENT);
    switch (handlerType) {
        case INTERCEPTOR: {
#ifdef OHOS_BUILD_ENABLE_INTERCEPTOR
            InputInterMgr->OnInputEvent(pointerEvent, deviceTags);
#endif // OHOS_BUILD_ENABLE_INTERCEPTOR
            break;
        }
        case MONITOR: {
#ifdef OHOS_BUILD_ENABLE_MONITOR
            IMonitorMgr->OnInputEvent(pointerEvent, deviceTags);
#endif // OHOS_BUILD_ENABLE_MONITOR
            break;
        }
        default: {
            MMI_HILOGW("Failed to intercept or monitor on the event");
            break;
        }
    }
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

void ClientMsgHandler::OnDispatchEventProcessed(int32_t eventId, int64_t actionTime)
{
    CALL_DEBUG_ENTER;
    ANRHDL->SetLastProcessedEventId(ANR_DISPATCH, eventId, actionTime);
}

int32_t ClientMsgHandler::OnAnr(const UDSClient& client, NetPacket& pkt)
{
    CALL_DEBUG_ENTER;
    int32_t pid;
    pkt >> pid;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet read data failed");
        return RET_ERR;
    }
    MMI_HILOGI("Client pid:%{public}d", pid);
    InputMgrImpl.OnAnr(pid);
    return RET_OK;
}
} // namespace MMI
} // namespace OHOS
