/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "input_event_transmission/input_event_builder.h"

#include "display_info.h"
#include "kits/c/wifi_hid2d.h"
#include "res_sched_client.h"
#include "res_type.h"

#include "cooperate_context.h"
#include "cooperate_hisysevent.h"
#include "devicestatus_define.h"
#include "input_event_transmission/input_event_serialization.h"
#include "utility.h"

#undef LOG_TAG
#define LOG_TAG "InputEventBuilder"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {
namespace {
constexpr size_t LOG_PERIOD { 10 };
constexpr int32_t DEFAULT_SCREEN_WIDTH { 512 };
constexpr double MIN_DAMPLING_COEFFICENT { 0.05 };
constexpr double MAX_DAMPLING_COEFFICENT { 1.5 };
constexpr double DEFAULT_DAMPLING_COEFFICIENT { 1.0 };
const std::string WIFI_INTERFACE_NAME { "chba0" };
const int32_t RESTORE_SCENE { 0 };
const int32_t FORBIDDEN_SCENE { 1 };
const int32_t UPPER_SCENE_FPS { 0 };
const int32_t UPPER_SCENE_BW { 0 };
const int32_t MODE_ENABLE { 0 };
const int32_t MODE_DISABLE { 1 };
const int32_t DRIVE_INTERCEPTOR_LATENCY { 5 };
const int32_t INTERCEPTOR_TRANSMISSION_LATENCY { 20 };
const std::string LOW_LATENCY_KEY = "identity";
}

InputEventBuilder::InputEventBuilder(IContext *env)
    : env_(env)
{
    observer_ = std::make_shared<DSoftbusObserver>(*this);
    pointerEvent_ = MMI::PointerEvent::Create();
    keyEvent_ = MMI::KeyEvent::Create();

    for (size_t index = 0, cnt = damplingCoefficients_.size(); index < cnt; ++index) {
        damplingCoefficients_[index] = DEFAULT_DAMPLING_COEFFICIENT;
    }
}

InputEventBuilder::~InputEventBuilder()
{
    Disable();
}

void InputEventBuilder::Enable(Context &context)
{
    CALL_INFO_TRACE;
    CHKPV(env_);
    if (enable_) {
        return;
    }
    enable_ = true;
    xDir_ = 0;
    movement_ = 0;
    freezing_ = (context.CooperateFlag() & COOPERATE_FLAG_FREEZE_CURSOR);
    remoteNetworkId_ = context.Peer();
    localNetworkId_ = context.Local();
    pointerSpeed_ = context.GetPointerSpeed();
    touchPadSpeed_ = context.GetTouchPadSpeed();
    env_->GetDSoftbus().AddObserver(observer_);
    Coordinate cursorPos = context.CursorPosition();
    TurnOffChannelScan();
    isStopByScreenOffOrLock_ = false;
    FI_HILOGI("Cursor transite in (%{private}d, %{private}d)", cursorPos.x, cursorPos.y);
    if (!enable_) {
        CooperateRadarInfo radarInfo {
            .funcName =  __FUNCTION__,
            .bizState = static_cast<int32_t> (BizState::STATE_END),
            .bizStage = static_cast<int32_t> (BizCooperateStage::STATE_INPUT_EVENT_BUILDER_ENABLE),
            .stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL),
            .bizScene = static_cast<int32_t> (BizCooperateScene::SCENE_PASSIVE),
            .errCode = static_cast<int32_t> (CooperateRadarErrCode::INPUT_EVENT_BUILDER_ENABLE_FAILED),
            .hostName = "",
            .localNetId = Utility::DFXRadarAnonymize(context.Local().c_str()),
            .peerNetId = Utility::DFXRadarAnonymize(remoteNetworkId_.c_str())
        };
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
    }
    ExecuteInner();
}

void InputEventBuilder::Disable()
{
    CALL_INFO_TRACE;
    CHKPV(env_);
    if (enable_) {
        enable_ = false;
        env_->GetDSoftbus().RemoveObserver(observer_);
        TurnOnChannelScan();
        ResetPressedEvents();
        isStopByScreenOffOrLock_ = false;
    }
    if ((pointerEventTimer_ >= 0)) {
        env_->GetTimerManager().RemoveTimerAsync(pointerEventTimer_);
        pointerEventTimer_ = -1;
    }
    HandleStopTimer();
}

void InputEventBuilder::Update(Context &context)
{
    remoteNetworkId_ = context.Peer();
    FI_HILOGI("Update peer to \'%{public}s\'", Utility::Anonymize(remoteNetworkId_).c_str());
}

void InputEventBuilder::Freeze()
{
    if (!enable_) {
        return;
    }
    xDir_ = 0;
    movement_ = 0;
    freezing_ = true;
    FI_HILOGI("Freeze remote input from '%{public}s'", Utility::Anonymize(remoteNetworkId_).c_str());
}

void InputEventBuilder::Thaw()
{
    if (!enable_) {
        return;
    }
    freezing_ = false;
    FI_HILOGI("Thaw remote input from '%{public}s'", Utility::Anonymize(remoteNetworkId_).c_str());
}

void InputEventBuilder::SetDamplingCoefficient(uint32_t direction, double coefficient)
{
    coefficient = std::clamp(coefficient, MIN_DAMPLING_COEFFICENT, MAX_DAMPLING_COEFFICENT);
    FI_HILOGI("SetDamplingCoefficient(0x%{public}x, %{public}.3f)", direction, coefficient);
    if ((direction & COORDINATION_DAMPLING_UP) == COORDINATION_DAMPLING_UP) {
        damplingCoefficients_[DamplingDirection::DAMPLING_DIRECTION_UP] = coefficient;
    }
    if ((direction & COORDINATION_DAMPLING_DOWN) == COORDINATION_DAMPLING_DOWN) {
        damplingCoefficients_[DamplingDirection::DAMPLING_DIRECTION_DOWN] = coefficient;
    }
    if ((direction & COORDINATION_DAMPLING_LEFT) == COORDINATION_DAMPLING_LEFT) {
        damplingCoefficients_[DamplingDirection::DAMPLING_DIRECTION_LEFT] = coefficient;
    }
    if ((direction & COORDINATION_DAMPLING_RIGHT) == COORDINATION_DAMPLING_RIGHT) {
        damplingCoefficients_[DamplingDirection::DAMPLING_DIRECTION_RIGHT] = coefficient;
    }
}

void InputEventBuilder::UpdateVirtualDeviceIdMap(const std::unordered_map<int32_t, int32_t> &remote2VirtualIds)
{
    CALL_INFO_TRACE;
    std::unique_lock<std::shared_mutex> lock(lock_);
    remote2VirtualIds_ = remote2VirtualIds;
    for (const auto &elem: remote2VirtualIds_) {
        FI_HILOGI("Remote:%{public}d -> virtual:%{public}d", elem.first, elem.second);
    }
}

double InputEventBuilder::GetDamplingCoefficient(DamplingDirection direction) const
{
    if ((direction >= DamplingDirection::DAMPLING_DIRECTION_UP) &&
        (direction < DamplingDirection::N_DAMPLING_DIRECTIONS)) {
        return damplingCoefficients_[direction];
    }
    return DEFAULT_DAMPLING_COEFFICIENT;
}

bool InputEventBuilder::OnPacket(const std::string &networkId, Msdp::NetPacket &packet)
{
    if (networkId != remoteNetworkId_) {
        FI_HILOGW("Unexpected packet from \'%{public}s\'", Utility::Anonymize(networkId).c_str());
        return false;
    }
    switch (packet.GetMsgId()) {
        case MessageId::DSOFTBUS_INPUT_POINTER_EVENT: {
            OnPointerEvent(packet);
            break;
        }
        case MessageId::DSOFTBUS_INPUT_KEY_EVENT: {
            OnKeyEvent(packet);
            break;
        }
        case MessageId::DSOFTBUS_HEART_BEAT_PACKET: {
            FI_HILOGD("Heart beat received");
            break;
        }
        default: {
            FI_HILOGW("Unexpected message(%{public}d) from \'%{public}s\'",
                static_cast<int32_t>(packet.GetMsgId()), Utility::Anonymize(networkId).c_str());
            return false;
        }
    }
    return true;
}

void InputEventBuilder::OnPointerEvent(Msdp::NetPacket &packet)
{
    int64_t curCrossPlatformTime = Utility::GetSysClockTime();
    CHKPV(pointerEvent_);
    CHKPV(env_);
    if (scanState_) {
        TurnOffChannelScan();
    }
    if ((pointerEventTimer_ >= 0)) {
        env_->GetTimerManager().RemoveTimerAsync(pointerEventTimer_);
        pointerEventTimer_ = -1;
    }
    pointerEvent_->Reset();
    int64_t curInterceptorTime = -1;
    int32_t ret = InputEventSerialization::Unmarshalling(packet, pointerEvent_, curInterceptorTime);
    if (ret != RET_OK) {
        FI_HILOGE("Failed to deserialize pointer event");
        return;
    }
    TagRemoteEvent(pointerEvent_);
    OnNotifyCrossDrag(pointerEvent_);
    FI_HILOGD("PointerEvent(No:%{public}d, Source:%{public}s, Action:%{public}s, rows:%{public}d)",
        pointerEvent_->GetId(), pointerEvent_->DumpSourceType(), pointerEvent_->DumpPointerAction(),
        pointerEvent_->GetScrollRows());
    if (IsActive(pointerEvent_)) {
        CheckLatency(pointerEvent_->GetActionTime(), curInterceptorTime, curCrossPlatformTime, pointerEvent_);
        if (!UpdatePointerEvent()) {
            return;
        }
        env_->GetInput().SimulateInputEvent(pointerEvent_);
    }
    pointerEventTimer_ = env_->GetTimerManager().AddTimerAsync(POINTER_EVENT_TIMEOUT, REPEAT_ONCE, [this]() {
        TurnOnChannelScan();
        pointerEventTimer_ = -1;
    });
}

std::shared_ptr<MMI::PointerEvent> InputEventBuilder::GetPointerEvent()
{
    CALL_INFO_TRACE;
    return pointerEvent_;
}

void InputEventBuilder::CheckLatency(int64_t curDriveActionTime, int64_t curInterceptorTime,
    int64_t curCrossPlatformTime, std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    if (pointerEvent->GetPointerAction() != MMI::PointerEvent::POINTER_ACTION_MOVE ||
        curInterceptorTime == -1) {
        preDriveEventTime_ = -1;
        return;
    }
    if (preDriveEventTime_ < 0) {
        preDriveEventTime_ = curDriveActionTime;
        preInterceptorTime_ = curInterceptorTime;
        preCrossPlatformTime_ = curCrossPlatformTime;
        return;
    }
    driveEventTimeDT_ = curDriveActionTime - preDriveEventTime_;
    cooperateInterceptorTimeDT_ = curInterceptorTime - preInterceptorTime_;
    crossPlatformTimeDT_ = curCrossPlatformTime - preCrossPlatformTime_;
    preDriveEventTime_ = curDriveActionTime;
    preInterceptorTime_ = curInterceptorTime;
    preCrossPlatformTime_ = curCrossPlatformTime;
    TransmissionLatencyRadarInfo radarInfo {
        .funcName = __FUNCTION__,
        .bizState = static_cast<int32_t> (BizState::STATE_END),
        .bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_CLIENT_ON_MESSAGE_RCVD),
        .stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_IDLE),
        .bizScene = static_cast<int32_t> (BizCooperateScene::SCENE_ACTIVE),
        .localNetId = Utility::DFXRadarAnonymize(localNetworkId_.c_str()),
        .peerNetId = Utility::DFXRadarAnonymize(remoteNetworkId_.c_str()),
    };
    int64_t DriveToInterceptorDT = Utility::GetSysClockTimeMilli(cooperateInterceptorTimeDT_ - driveEventTimeDT_);
    int64_t InterceptorToCrossDT = std::abs(Utility::GetSysClockTimeMilli(
        crossPlatformTimeDT_ - cooperateInterceptorTimeDT_));
    if (DriveToInterceptorDT > DRIVE_INTERCEPTOR_LATENCY || InterceptorToCrossDT > INTERCEPTOR_TRANSMISSION_LATENCY) {
        FI_HILOGI("driveEventTimeDT:%{public}" PRId64 ", cooperateInterceptorTimeDT:%{public}" PRId64 ""
            "crossPlatformTimeDT:%{public}" PRId64, driveEventTimeDT_, cooperateInterceptorTimeDT_,
            crossPlatformTimeDT_);
        radarInfo.driveEventTimeDT = driveEventTimeDT_;
        radarInfo.cooperateInterceptorTimeDT = cooperateInterceptorTimeDT_;
        radarInfo.crossPlatformTimeDT = crossPlatformTimeDT_;
        radarInfo.pointerSpeed = pointerSpeed_;
        radarInfo.touchPadSpeed = touchPadSpeed_;
        CooperateRadar::ReportTransmissionLatencyRadarInfo(radarInfo);
    }
}

void InputEventBuilder::OnNotifyCrossDrag(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    auto pointerAction = pointerEvent->GetPointerAction();
    if (pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_IN_WINDOW ||
        pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW) {
        FI_HILOGD("PointerAction:%{public}d, it's pressedButtons is empty, skip", pointerAction);
        return;
    }
    auto pressedButtons = pointerEvent->GetPressedButtons();
    bool isButtonDown = (pressedButtons.find(MMI::PointerEvent::MOUSE_BUTTON_LEFT) != pressedButtons.end());
    FI_HILOGD("PointerAction:%{public}d, isPressed:%{public}s", pointerAction, isButtonDown ? "true" : "false");
    CHKPV(env_);
    env_->GetDragManager().NotifyCrossDrag(isButtonDown);
}

void InputEventBuilder::OnKeyEvent(Msdp::NetPacket &packet)
{
    CHKPV(keyEvent_);
    keyEvent_->Reset();
    int32_t ret = InputEventSerialization::NetPacketToKeyEvent(packet, keyEvent_);
    if (ret != RET_OK) {
        FI_HILOGE("Failed to deserialize key event");
        return;
    }
    UpdateKeyEvent(keyEvent_);
    TagRemoteEvent(keyEvent_);
    FI_HILOGD("KeyEvent(No:%{public}d,Key:%{private}d,Action:%{public}d)",
        keyEvent_->GetId(), keyEvent_->GetKeyCode(), keyEvent_->GetKeyAction());
    keyEvent_->AddFlag(MMI::InputEvent::EVENT_FLAG_SIMULATE);
    env_->GetInput().SimulateInputEvent(keyEvent_);
}

void InputEventBuilder::TurnOffChannelScan()
{
    CALL_INFO_TRACE;
    scanState_ = false;
    if (SetWifiScene(FORBIDDEN_SCENE) != RET_OK) {
        scanState_ = true;
        FI_HILOGE("Forbidden scene failed");
    }
}

void InputEventBuilder::ExecuteInner()
{
    CALL_DEBUG_ENTER;
    // to enable low latency mode: value = 0
    OHOS::ResourceSchedule::ResSchedClient::GetInstance().ReportData(
        OHOS::ResourceSchedule::ResType::RES_TYPE_NETWORK_LATENCY_REQUEST, MODE_ENABLE,
        {{LOW_LATENCY_KEY, FI_PKG_NAME}});
}

void InputEventBuilder::HandleStopTimer()
{
    CALL_DEBUG_ENTER;
    OHOS::ResourceSchedule::ResSchedClient::GetInstance().ReportData(
        OHOS::ResourceSchedule::ResType::RES_TYPE_NETWORK_LATENCY_REQUEST, MODE_DISABLE,
        {{LOW_LATENCY_KEY, FI_PKG_NAME}});
}

void InputEventBuilder::TurnOnChannelScan()
{
    CALL_INFO_TRACE;
    scanState_ = true;
    if (SetWifiScene(RESTORE_SCENE) != RET_OK) {
        scanState_ = false;
        FI_HILOGE("Restore scene failed");
    }
}

int32_t InputEventBuilder::SetWifiScene(unsigned int scene)
{
    CALL_DEBUG_ENTER;
    Hid2dUpperScene upperScene;
    upperScene.scene = scene;
    upperScene.fps = UPPER_SCENE_FPS;
    upperScene.bw = UPPER_SCENE_BW;
    if (Hid2dSetUpperScene(WIFI_INTERFACE_NAME.c_str(), &upperScene) != RET_OK) {
        FI_HILOGE("Set wifi scene failed");
        return RET_ERR;
    }
    return RET_OK;
}

bool InputEventBuilder::UpdatePointerEvent()
{
    if (pointerEvent_->GetSourceType() != MMI::PointerEvent::SOURCE_TYPE_MOUSE) {
        return true;
    }
    if (!DampPointerMotion(pointerEvent_)) {
        FI_HILOGE("DampPointerMotion fail");
        return false;
    }
    pointerEvent_->AddFlag(MMI::InputEvent::EVENT_FLAG_RAW_POINTER_MOVEMENT);
    int64_t time = Utility::GetSysClockTime();
    pointerEvent_->SetActionTime(time);
    pointerEvent_->SetActionStartTime(time);
    pointerEvent_->SetTargetDisplayId(-1);
    pointerEvent_->SetTargetWindowId(-1);
    pointerEvent_->SetAgentWindowId(-1);
    return true;
}

bool InputEventBuilder::DampPointerMotion(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    MMI::PointerEvent::PointerItem item;
    if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item)) {
        FI_HILOGE("Corrupted pointer event");
        return false;
    }
    // Dampling pointer movement.
    // First transition will trigger special effect which would damp pointer movement. We want to
    // damp pointer movement even further than that could be achieved by setting pointer speed.
    // By scaling increment of pointer movement, we want to enlarge the range of pointer speed setting.
    if (item.GetRawDx() > 0) {
        double rawDxRight = rawDxRightRemainder_ + item.GetRawDx() * GetDamplingCoefficient(
            DamplingDirection::DAMPLING_DIRECTION_RIGHT);
        int32_t rawDxIntegerRight = static_cast<int32_t>(rawDxRight);
        rawDxRightRemainder_ = rawDxRight - static_cast<double>(rawDxIntegerRight);
        item.SetRawDx(rawDxIntegerRight);
    } else if (item.GetRawDx() < 0) {
        double rawDxLeft = rawDxLeftRemainder_ + item.GetRawDx() * GetDamplingCoefficient(
            DamplingDirection::DAMPLING_DIRECTION_LEFT);
        int32_t rawDxIntegerLeft = static_cast<int32_t>(rawDxLeft);
        rawDxLeftRemainder_ = rawDxLeft - static_cast<double>(rawDxIntegerLeft);
        item.SetRawDx(rawDxIntegerLeft);
    }
    if (item.GetRawDy() >= 0) {
        item.SetRawDy(static_cast<int32_t>(
            item.GetRawDy() * GetDamplingCoefficient(DamplingDirection::DAMPLING_DIRECTION_DOWN)));
    } else {
        item.SetRawDy(static_cast<int32_t>(
            item.GetRawDy() * GetDamplingCoefficient(DamplingDirection::DAMPLING_DIRECTION_UP)));
    }
    pointerEvent->UpdatePointerItem(pointerEvent->GetPointerId(), item);
    return true;
}

void InputEventBuilder::UpdateKeyEvent(std::shared_ptr<MMI::KeyEvent> keyEvent)
{
    int64_t time = Utility::GetSysClockTime();
    keyEvent->SetActionTime(time);
    keyEvent->SetActionStartTime(time);
}

void InputEventBuilder::TagRemoteEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    std::shared_lock<std::shared_mutex> lock(lock_);
    if (auto deviceId = pointerEvent->GetDeviceId(); remote2VirtualIds_.find(deviceId) != remote2VirtualIds_.end()) {
        pointerEvent->SetDeviceId(remote2VirtualIds_[deviceId]);
    } else {
        pointerEvent->SetDeviceId((deviceId >= 0) ? -(deviceId + 1) : deviceId);
    }
}

void InputEventBuilder::TagRemoteEvent(std::shared_ptr<MMI::KeyEvent> keyEvent)
{
    std::shared_lock<std::shared_mutex> lock(lock_);
    if (auto deviceId = keyEvent->GetDeviceId(); remote2VirtualIds_.find(deviceId) != remote2VirtualIds_.end()) {
        keyEvent->SetDeviceId(remote2VirtualIds_[deviceId]);
    } else {
        keyEvent->SetDeviceId((deviceId >= 0) ? -(deviceId + 1) : deviceId);
    }
}

bool InputEventBuilder::IsActive(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    if (!freezing_) {
        return true;
    }
    if ((pointerEvent->GetSourceType() != MMI::PointerEvent::SOURCE_TYPE_MOUSE) ||
        ((pointerEvent->GetPointerAction() != MMI::PointerEvent::POINTER_ACTION_MOVE) &&
         (pointerEvent->GetPointerAction() != MMI::PointerEvent::POINTER_ACTION_PULL_MOVE))) {
        return true;
    }
    MMI::PointerEvent::PointerItem item;
    if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item)) {
        FI_HILOGE("Corrupted pointer event");
        return false;
    }
    movement_ += item.GetRawDx();
    movement_ = std::clamp(movement_, -DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_WIDTH);
    if (xDir_ == 0) {
        xDir_ = movement_;
    }
    if (((xDir_ > 0) && (movement_ <= 0)) || ((xDir_ < 0) && (movement_ >= 0))) {
        return true;
    }
    if ((nDropped_++ % LOG_PERIOD) == 0) {
        FI_HILOGI("Remote input from '%{public}s' is freezing", Utility::Anonymize(remoteNetworkId_).c_str());
    }
    return false;
}

void InputEventBuilder::ResetPressedEvents()
{
    CHKPV(env_);
    CHKPV(pointerEvent_);
    if (auto pressedButtons = pointerEvent_->GetPressedButtons(); !pressedButtons.empty()) {
        auto dragState = env_->GetDragManager().GetDragState();
        for (auto buttonId : pressedButtons) {
            if (dragState == DragState::START || dragState == DragState::MOTION_DRAGGING) {
                FI_HILOGI("Dragging or cross dragging, skip");
                continue;
            }
            pointerEvent_->SetButtonId(buttonId);
            if (isStopByScreenOffOrLock_) {
                pointerEvent_->SetPointerAction(MMI::PointerEvent::POINTER_ACTION_CANCEL);
            } else {
                pointerEvent_->SetPointerAction(MMI::PointerEvent::POINTER_ACTION_BUTTON_UP);
            }
            env_->GetInput().SimulateInputEvent(pointerEvent_);
            FI_HILOGI("Simulate button-up event, buttonId:%{public}d", buttonId);
        }
        pointerEvent_->Reset();
    }
    CHKPV(keyEvent_);
    if (auto pressedKeys = keyEvent_->GetPressedKeys(); !pressedKeys.empty()) {
        for (auto pressedKey : pressedKeys) {
            keyEvent_->SetKeyCode(pressedKey);
            keyEvent_->SetKeyAction(MMI::KeyEvent::KEY_ACTION_UP);
            auto keyItem = keyEvent_->GetKeyItem(pressedKey);
            if (keyItem.has_value()) {
                keyItem->SetPressed(false);
                keyEvent_->AddReleasedKeyItems(*keyItem);
            } else {
                FI_HILOGE("keyItem is null");
            }
            env_->GetInput().SimulateInputEvent(keyEvent_);
            FI_HILOGI("Simulate key-up event, pressedKey:%{private}d", pressedKey);
        }
        keyEvent_->Reset();
    }
}

void InputEventBuilder::SetStopByScreenOffOrLock(bool stopByScreenOffOrLock)
{
    isStopByScreenOffOrLock_ = stopByScreenOffOrLock;
}
} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
