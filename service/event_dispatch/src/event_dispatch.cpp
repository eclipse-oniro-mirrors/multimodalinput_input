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

#include "event_dispatch.h"

#include <cinttypes>

#include "ability_manager_client.h"
#include "hitrace_meter.h"
#include "input-event-codes.h"
#include "hisysevent.h"

#include "bytrace_adapter.h"
#include "define_interceptor_global.h"
#include "error_multimodal.h"
#include "event_filter_wrap.h"
#include "input_event_data_transformation.h"
#include "input_event_handler.h"
#include "input_event_monitor_manager.h"
#include "input_handler_manager_global.h"
#include "key_event_subscriber.h"
#include "util.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "EventDispatch" };
} // namespace

EventDispatch::EventDispatch() {}

EventDispatch::~EventDispatch() {}

void EventDispatch::OnEventTouchGetPointEventType(const EventTouch& touch,
                                                  const int32_t fingerCount,
                                                  POINT_EVENT_TYPE& pointEventType)
{
    if (fingerCount <= 0 || touch.time <= 0 || touch.seatSlot < 0 || touch.eventType < 0) {
        MMI_HILOGE("The in parameter is error, fingerCount:%{public}d, touch.time:%{public}" PRId64 ","
                   "touch.seatSlot:%{public}d, touch.eventType:%{public}d",
                   fingerCount, touch.time, touch.seatSlot, touch.eventType);
        return;
    }
    if (fingerCount == 1) {
        switch (touch.eventType) {
            case LIBINPUT_EVENT_TOUCH_DOWN: {
                pointEventType = PRIMARY_POINT_DOWN;
                break;
            }
            case LIBINPUT_EVENT_TOUCH_UP: {
                pointEventType = PRIMARY_POINT_UP;
                break;
            }
            case LIBINPUT_EVENT_TOUCH_MOTION: {
                pointEventType = POINT_MOVE;
                break;
            }
            default: {
                break;
            }
        }
    } else {
        switch (touch.eventType) {
            case LIBINPUT_EVENT_TOUCH_DOWN: {
                pointEventType = OTHER_POINT_DOWN;
                break;
            }
            case LIBINPUT_EVENT_TOUCH_UP: {
                pointEventType = OTHER_POINT_UP;
                break;
            }
            case LIBINPUT_EVENT_TOUCH_MOTION: {
                pointEventType = POINT_MOVE;
                break;
            }
            default: {
                break;
            }
        }
    }
}

bool EventDispatch::HandlePointerEventFilter(std::shared_ptr<PointerEvent> point)
{
    return EventFilterWrap::GetInstance().HandlePointerEventFilter(point);
}

int32_t EventDispatch::HandlePointerEvent(std::shared_ptr<PointerEvent> point)
{
    CALL_LOG_ENTER;
    CHKPR(point, ERROR_NULL_POINTER);
    if (HandlePointerEventFilter(point)) {
        MMI_HILOGI("Pointer event Filter succeeded");
        return RET_OK;
    }
    if (InterHdlGl->HandleEvent(point)) {
        BytraceAdapter::StartBytrace(point, BytraceAdapter::TRACE_STOP);
        MMI_HILOGD("Interception is succeeded");
        return RET_OK;
    }
    if (InputHandlerManagerGlobal::GetInstance().HandleEvent(point)) {
        BytraceAdapter::StartBytrace(point, BytraceAdapter::TRACE_STOP);
        MMI_HILOGD("Monitor is succeeded");
        return RET_OK;
    }
    auto fd = WinMgr->UpdateTargetPointer(point);
    if (fd < 0) {
        MMI_HILOGE("The fd less than 0, fd: %{public}d", fd);
        return RET_ERR;
    }
    NetPacket pkt(MmiMessageId::ON_POINTER_EVENT);
    auto udsServer = InputHandler->GetUDSServer();
    if (udsServer == nullptr) {
        MMI_HILOGE("UdsServer is a nullptr");
        return RET_ERR;
    }
    auto pid = udsServer->GetClientPid(fd);
    auto pointerEvent = std::make_shared<PointerEvent>(*point);
    auto pointerIdList = pointerEvent->GetPointersIdList();
    if (pointerIdList.size() > 1) {
        for (const auto& id : pointerIdList) {
            PointerEvent::PointerItem pointeritem;
            if (!pointerEvent->GetPointerItem(id, pointeritem)) {
                MMI_HILOGW("can't find this poinerItem");
                continue;
            }
            auto itemPid = WinMgr->GetWindowPid(pointeritem.GetTargetWindowId());
            if (itemPid >=0 && itemPid != pid) {
                pointerEvent->RemovePointerItem(id);
                MMI_HILOGD("pointerIdList size: %{public}zu", pointerEvent->GetPointersIdList().size());
            }
        }
    }

    InputEventDataTransformation::Marshalling(pointerEvent, pkt);
    BytraceAdapter::StartBytrace(point, BytraceAdapter::TRACE_STOP);
    

    if (!udsServer->SendMsg(fd, pkt)) {
        MMI_HILOGE("Sending structure of EventTouch failed! errCode:%{public}d", MSG_SEND_FAIL);
        return RET_ERR;
    }
    return RET_OK;
}

int32_t EventDispatch::DispatchKeyEventPid(UDSServer& udsServer, std::shared_ptr<KeyEvent> key)
{
    CALL_LOG_ENTER;
    CHKPR(key, PARAM_INPUT_INVALID);
    if (!key->HasFlag(InputEvent::EVENT_FLAG_NO_INTERCEPT)) {
        if (InterHdlGl->HandleEvent(key)) {
            MMI_HILOGD("keyEvent filter find a keyEvent from Original event keyCode: %{puiblic}d",
                key->GetKeyCode());
            BytraceAdapter::StartBytrace(key, BytraceAdapter::KEY_INTERCEPT_EVENT);
            return RET_OK;
        }
    }
    if (IKeyCommandManager::GetInstance()->HandleEvent(key)) {
        MMI_HILOGD("The keyEvent start launch an ability, keyCode:%{public}d", key->GetKeyCode());
        BytraceAdapter::StartBytrace(key, BytraceAdapter::KEY_LAUNCH_EVENT);
        return RET_OK;
    }
    if (KeyEventSubscriber_.SubscribeKeyEvent(key)) {
        MMI_HILOGD("Subscribe keyEvent filter success. keyCode:%{public}d", key->GetKeyCode());
        BytraceAdapter::StartBytrace(key, BytraceAdapter::KEY_SUBSCRIBE_EVENT);
        return RET_OK;
    }
    auto fd = WinMgr->UpdateTarget(key);
    if (fd < 0) {
        MMI_HILOGE("Invalid fd, fd: %{public}d", fd);
        return RET_ERR;
    }
    MMI_HILOGD("event dispatcher of server:KeyEvent:KeyCode:%{public}d,"
               "ActionTime:%{public}" PRId64 ",Action:%{public}d,ActionStartTime:%{public}" PRId64 ","
               "EventType:%{public}d,Flag:%{public}u,"
               "KeyAction:%{public}d,Fd:%{public}d",
               key->GetKeyCode(), key->GetActionTime(), key->GetAction(),
               key->GetActionStartTime(),
               key->GetEventType(),
               key->GetFlag(), key->GetKeyAction(), fd);
    InputHandlerManagerGlobal::GetInstance().HandleEvent(key);

    NetPacket pkt(MmiMessageId::ON_KEYEVENT);
    InputEventDataTransformation::KeyEventToNetPacket(key, pkt);
    BytraceAdapter::StartBytrace(key, BytraceAdapter::KEY_DISPATCH_EVENT);
    pkt << fd;
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet write structure of EventKeyboard failed");
        return RET_ERR;
    }
    if (!udsServer.SendMsg(fd, pkt)) {
        MMI_HILOGE("Sending structure of EventKeyboard failed! errCode:%{public}d", MSG_SEND_FAIL);
        return MSG_SEND_FAIL;
    }
    return RET_OK;
}

int32_t EventDispatch::AddInputEventFilter(sptr<IEventFilter> filter)
{
    return EventFilterWrap::GetInstance().AddInputEventFilter(filter);
}

bool EventDispatch::TriggerANR(int64_t time, SessionPtr sess)
{
    CALL_LOG_ENTER;
    int64_t earlist;
    if (sess->IsEventQueueEmpty()) {
        earlist = time;
    } else {
        earlist = sess->GetEarlistEventTime();
    }

    if (time >= 0) {
        sess->isANRProcess_ = false;
        MMI_HILOGD("the event reports normally");
        return false;
    }

    int32_t ret = OHOS::HiviewDFX::HiSysEvent::Write(
        OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MODAL_INPUT,
        "APPLICATION_BLOCK_INPUT",
        OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
        "PID", sess->GetPid(),
        "UID", sess->GetUid(),
        "PACKAGE_NAME", "",
        "PROCESS_NAME", "",
        "MSG", "User input does not respond");
    if (ret != 0) {
        MMI_HILOGE("HiviewDFX Write failed, HiviewDFX errCode: %{public}d", ret);
    }

    ret = OHOS::AAFwk::AbilityManagerClient::GetInstance()->SendANRProcessID(sess->GetPid());
    if (ret != 0) {
        MMI_HILOGE("AAFwk SendANRProcessID failed, AAFwk errCode: %{public}d", ret);
    }
    return true;
}
} // namespace MMI
} // namespace OHOS
