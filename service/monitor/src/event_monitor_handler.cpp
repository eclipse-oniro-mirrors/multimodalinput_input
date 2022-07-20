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

#include "event_monitor_handler.h"

#include "bytrace_adapter.h"
#include "define_multimodal.h"
#include "input_event_data_transformation.h"
#include "input_event_handler.h"
#include "mmi_log.h"
#include "net_packet.h"
#include "proto.h"
#include "util_ex.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "EventMonitorHandler" };
} // namespace

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
void EventMonitorHandler::HandleKeyEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPV(keyEvent);
    OnHandleEvent(keyEvent);
    CHKPV(nextHandler_);
    nextHandler_->HandleKeyEvent(keyEvent);
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

#ifdef OHOS_BUILD_ENABLE_POINTER
void EventMonitorHandler::HandlePointerEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    if (OnHandleEvent(pointerEvent)) {
        BytraceAdapter::StartBytrace(pointerEvent, BytraceAdapter::TRACE_STOP);
        MMI_HILOGD("Monitor is succeeded");
        return;
    }
    CHKPV(nextHandler_);
    nextHandler_->HandlePointerEvent(pointerEvent);
}
#endif // OHOS_BUILD_ENABLE_POINTER

#ifdef OHOS_BUILD_ENABLE_TOUCH
void EventMonitorHandler::HandleTouchEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    if (OnHandleEvent(pointerEvent)) {
        BytraceAdapter::StartBytrace(pointerEvent, BytraceAdapter::TRACE_STOP);
        MMI_HILOGD("Monitor is succeeded");
        return;
    }
    CHKPV(nextHandler_);
    nextHandler_->HandleTouchEvent(pointerEvent);
}
#endif // OHOS_BUILD_ENABLE_TOUCH

int32_t EventMonitorHandler::AddInputHandler(int32_t handlerId,
    InputHandlerType handlerType, SessionPtr session)
{
    CALL_INFO_TRACE;
    InitSessionLostCallback();
    if (!IsValidHandlerId(handlerId)) {
        MMI_HILOGE("Invalid handler");
        return RET_ERR;
    }
    CHKPR(session, RET_ERR);
    if (handlerType == InputHandlerType::MONITOR) {
        MMI_HILOGD("Register monitor:%{public}d", handlerId);
        SessionHandler mon { handlerId, handlerType, session };
        return monitors_.AddMonitor(mon);
    }
    MMI_HILOGW("Invalid handler type:%{public}d", handlerType);
    return RET_ERR;
}

void EventMonitorHandler::RemoveInputHandler(int32_t handlerId,
    InputHandlerType handlerType, SessionPtr session)
{
    CALL_INFO_TRACE;
    if (handlerType == InputHandlerType::MONITOR) {
        MMI_HILOGD("Unregister monitor:%{public}d", handlerId);
        SessionHandler monitor { handlerId, handlerType, session };
        monitors_.RemoveMonitor(monitor);
    }
}

void EventMonitorHandler::MarkConsumed(int32_t handlerId, int32_t eventId, SessionPtr session)
{
    CALL_INFO_TRACE;
    MMI_HILOGD("Mark consumed state, monitor:%{public}d", handlerId);
    monitors_.MarkConsumed(handlerId, eventId, session);
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
bool EventMonitorHandler::OnHandleEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    MMI_HILOGD("Handle KeyEvent");
    CHKPF(keyEvent);
    if (keyEvent->HasFlag(InputEvent::EVENT_FLAG_NO_MONITOR)) {
        MMI_HILOGD("This event has been tagged as not to be monitored");
    } else {
        if (monitors_.HandleEvent(keyEvent)) {
            MMI_HILOGD("Key event was consumed");
            return true;
        }
    }
    return false;
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
bool EventMonitorHandler::OnHandleEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPF(pointerEvent);
    if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_NO_MONITOR)) {
        MMI_HILOGD("This event has been tagged as not to be monitored");
    } else {
        if (monitors_.HandleEvent(pointerEvent)) {
            MMI_HILOGD("Pointer event was monitor");
            return true;
        }
    }
    MMI_HILOGD("Interception and monitor failed");
    return false;
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

void EventMonitorHandler::InitSessionLostCallback()
{
    if (sessionLostCallbackInitialized_)  {
        return;
    }
    auto udsServerPtr = InputHandler->GetUDSServer();
    CHKPV(udsServerPtr);
    udsServerPtr->AddSessionDeletedCallback(std::bind(
        &EventMonitorHandler::OnSessionLost, this, std::placeholders::_1));
    sessionLostCallbackInitialized_ = true;
    MMI_HILOGD("The callback on session deleted is registered successfully");
}

void EventMonitorHandler::OnSessionLost(SessionPtr session)
{
    monitors_.OnSessionLost(session);
}

void EventMonitorHandler::SessionHandler::SendToClient(std::shared_ptr<KeyEvent> keyEvent) const
{
    CHKPV(keyEvent);
    NetPacket pkt(MmiMessageId::REPORT_KEY_EVENT);
    pkt << id_;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet write key event failed");
        return;
    }
    if (InputEventDataTransformation::KeyEventToNetPacket(keyEvent, pkt) != RET_OK) {
        MMI_HILOGE("Packet key event failed, errCode:%{public}d", STREAM_BUF_WRITE_FAIL);
        return;
    }
    if (!session_->SendMsg(pkt)) {
        MMI_HILOGE("Send message failed, errCode:%{public}d", MSG_SEND_FAIL);
        return;
    }
}

void EventMonitorHandler::SessionHandler::SendToClient(std::shared_ptr<PointerEvent> pointerEvent) const
{
    CHKPV(pointerEvent);
    NetPacket pkt(MmiMessageId::REPORT_POINTER_EVENT);
    MMI_HILOGD("Service SendToClient id:%{public}d,InputHandlerType:%{public}d", id_, handlerType_);
    pkt << id_ << handlerType_;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet write pointer event failed");
        return;
    }
    if (InputEventDataTransformation::Marshalling(pointerEvent, pkt) != RET_OK) {
        MMI_HILOGE("Marshalling pointer event failed, errCode:%{public}d", STREAM_BUF_WRITE_FAIL);
        return;
    }
    if (!session_->SendMsg(pkt)) {
        MMI_HILOGE("Send message failed, errCode:%{public}d", MSG_SEND_FAIL);
        return;
    }
}

int32_t EventMonitorHandler::MonitorCollection::AddMonitor(const SessionHandler& monitor)
{
    if (monitors_.size() >= MAX_N_INPUT_MONITORS) {
        MMI_HILOGE("The number of monitors exceeds the maximum:%{public}zu,monitors,errCode:%{public}d",
                   monitors_.size(), INVALID_MONITOR_MON);
        return RET_ERR;
    }
    auto ret = monitors_.insert(monitor);
    if (ret.second) {
        MMI_HILOGD("Service AddMonitor Success");
    } else {
        MMI_HILOGW("Duplicate monitors");
    }
    return RET_OK;
}

void EventMonitorHandler::MonitorCollection::RemoveMonitor(const SessionHandler& monitor)
{
    std::set<SessionHandler>::const_iterator tItr = monitors_.find(monitor);
    if (tItr != monitors_.end()) {
        monitors_.erase(tItr);
        MMI_HILOGD("Service RemoveMonitor Success");
    }
}

void EventMonitorHandler::MonitorCollection::MarkConsumed(int32_t monitorId, int32_t eventId, SessionPtr session)
{
    if (!HasMonitor(monitorId, session)) {
        MMI_HILOGW("Specified monitor does not exist, monitor:%{public}d", monitorId);
        return;
    }
    if (isMonitorConsumed_) {
        MMI_HILOGW("Event consumed");
        return;
    }
    if ((downEventId_ < 0) || (lastPointerEvent_ == nullptr)) {
        MMI_HILOGI("No touch event or press event without a previous finger is not handled");
        return;
    }
    if (downEventId_ > eventId) {
        MMI_HILOGW("A new process has began %{public}d,%{public}d", downEventId_, eventId);
        return;
    }
    isMonitorConsumed_ = true;
    MMI_HILOGD("Cancel operation");
    auto pointerEvent = std::make_shared<PointerEvent>(*lastPointerEvent_);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    pointerEvent->SetActionTime(GetSysClockTime());
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT | InputEvent::EVENT_FLAG_NO_MONITOR);
#ifdef OHOS_BUILD_ENABLE_TOUCH
    auto inputEventNormalizeHandler = InputHandler->GetInputEventNormalizeHandler();
    CHKPV(inputEventNormalizeHandler);
    inputEventNormalizeHandler->HandleTouchEvent(pointerEvent);
#endif // OHOS_BUILD_ENABLE_TOUCH
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
bool EventMonitorHandler::MonitorCollection::HandleEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPF(keyEvent);
    MMI_HILOGD("There are currently %{public}zu monitors", monitors_.size());
    for (const auto &mon : monitors_) {
        mon.SendToClient(keyEvent);
    }
    return false;
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
bool EventMonitorHandler::MonitorCollection::HandleEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPF(pointerEvent);
#ifdef OHOS_BUILD_ENABLE_TOUCH
    UpdateConsumptionState(pointerEvent);
#endif // OHOS_BUILD_ENABLE_TOUCH
    Monitor(pointerEvent);
    if (pointerEvent->GetSourceType() == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) {
        return isMonitorConsumed_;
    }
    MMI_HILOGI("This is not a touch-screen event");
    return false;
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

bool EventMonitorHandler::MonitorCollection::HasMonitor(int32_t monitorId, SessionPtr session)
{
    SessionHandler monitor { monitorId, InputHandlerType::MONITOR, session };
    return (monitors_.find(monitor) != monitors_.end());
}

#ifdef OHOS_BUILD_ENABLE_TOUCH
void EventMonitorHandler::MonitorCollection::UpdateConsumptionState(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    if (pointerEvent->GetSourceType() != PointerEvent::SOURCE_TYPE_TOUCHSCREEN) {
        return;
    }
    lastPointerEvent_ = pointerEvent;

    if (pointerEvent->GetPointerIds().size() != 1) {
        MMI_HILOGD("First press down and last press up intermediate process");
        return;
    }
    if (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_DOWN) {
        MMI_HILOGD("First press down");
        downEventId_ = pointerEvent->GetId();
        isMonitorConsumed_ = false;
    } else if (pointerEvent->GetPointerAction() == PointerEvent::POINTER_ACTION_UP) {
        MMI_HILOGD("Last press up");
        downEventId_ = -1;
        lastPointerEvent_.reset();
    }
}
#endif // OHOS_BUILD_ENABLE_TOUCH

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
void EventMonitorHandler::MonitorCollection::Monitor(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    MMI_HILOGD("There are currently %{public}zu monitors", monitors_.size());
    for (const auto &monitor : monitors_) {
        monitor.SendToClient(pointerEvent);
    }
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

void EventMonitorHandler::MonitorCollection::OnSessionLost(SessionPtr session)
{
    CALL_INFO_TRACE;
    std::set<SessionHandler>::const_iterator cItr = monitors_.cbegin();
    while (cItr != monitors_.cend()) {
        if (cItr->session_ != session) {
            ++cItr;
        } else {
            cItr = monitors_.erase(cItr);
        }
    }
}
void EventMonitorHandler::Dump(int32_t fd, const std::vector<std::string> &args)
{
    return monitors_.Dump(fd, args);
}

void EventMonitorHandler::MonitorCollection::Dump(int32_t fd, const std::vector<std::string> &args)
{
    CALL_DEBUG_ENTER;
    mprintf(fd, "Monitor information:\t");
    mprintf(fd, "monitors: count=%d", monitors_.size());
    for (const auto &item : monitors_) {
        SessionPtr session = item.session_;
        CHKPV(session);
        mprintf(fd,
                "monitor id:%d | handlerType:%d | Pid:%d | Uid:%d | Fd:%d "
                "| EarliestEventTime:%" PRId64 " | Descript:%s \t",
                item.id_, item.handlerType_, session->GetPid(),
                session->GetUid(), session->GetFd(),
                session->GetEarliestEventTime(), session->GetDescript().c_str());
    }
}
} // namespace MMI
} // namespace OHOS
