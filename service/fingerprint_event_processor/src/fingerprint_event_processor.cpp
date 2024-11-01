/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "fingerprint_event_processor.h"

#include "ability_manager_client.h"
#include "event_log_helper.h"
#include "ffrt.h"
#include "input_event_handler.h"
#include "pointer_event.h"
#include "res_sched_client.h"
#include "res_type.h"
#include "setting_datashare.h"
#include "system_ability_definition.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_DISPATCH
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "FingerprintEventProcessor"

namespace OHOS {
namespace MMI {
#ifdef OHOS_BUILD_ENABLE_FINGERPRINT
namespace {
constexpr int32_t KEY_INIT { 0 };
constexpr int32_t KEY_DOWN { 1 };
constexpr int32_t KEY_UP { 2 };
constexpr int32_t POWER_KEY_UP_TIME { 1000 }; // 1000ms
const std::string IS_START_SMART_KEY = "close_fingerprint_nav_event_key";
const std::string IS_SMART_KEY_USE = "close_fingerprint_event_key";
const std::string NEED_SHOW_DIALOG = "1";
const std::string SMART_KEY_IS_OPEN = "1";
const std::string SMART_KEY_IS_CLOSE = "0";
constexpr int32_t IS_SHOW_DIALOG = 1;
constexpr int32_t VOLUME_KEY_UP_TIME { 500 }; // 500ms
}
FingerprintEventProcessor::FingerprintEventProcessor()
{}

FingerprintEventProcessor::~FingerprintEventProcessor()
{}

bool FingerprintEventProcessor::IsFingerprintEvent(struct libinput_event* event)
{
    CALL_DEBUG_ENTER;
    CHKPR(event, false);
    if (!isStartedSmartKey_) {
        StartSmartKeyIfNeeded();
        isStartedSmartKey_ = true;
    }
    if (!isCreatedObserver_) {
        smartKeySwitch_.keyString = IS_START_SMART_KEY;
        CreateStatusConfigObserver(smartKeySwitch_);
        isCreatedObserver_ = true;
    }
    auto device = libinput_event_get_device(event);
    CHKPR(device, false);
    std::string name = libinput_device_get_name(device);
    if (name != FINGERPRINT_SOURCE_KEY && name != FINGERPRINT_SOURCE_POINT) {
        MMI_HILOGD("Not FingerprintEvent");
        return false;
    }
    if (name == FINGERPRINT_SOURCE_KEY) {
        struct libinput_event_keyboard* keyBoard = libinput_event_get_keyboard_event(event);
        CHKPR(keyBoard, false);
        auto key = libinput_event_keyboard_get_key(keyBoard);
        if (key != FINGERPRINT_CODE_DOWN && key != FINGERPRINT_CODE_UP
            && key != FINGERPRINT_CODE_CLICK && key != FINGERPRINT_CODE_RETOUCH
            && key != FINGERPRINT_CODE_CANCEL) {
            MMI_HILOGD("Not FingerprintEvent event");
            return false;
        }
    }
    return true;
}

void FingerprintEventProcessor::SetPowerAndVolumeKeyState(struct libinput_event* event)
{
    CALL_DEBUG_ENTER;
    CHKPV(event);
    auto device = libinput_event_get_device(event);
    CHKPV(device);
    auto data = libinput_event_get_keyboard_event(event);
    CHKPV(data);
    int32_t keyCode = static_cast<int32_t>(libinput_event_keyboard_get_key(data));
    auto iter = keyStateMap_.find(keyCode);
    if (iter == keyStateMap_.end()) {
        MMI_HILOGD("current keycode is not mistouch key, keycode is %{private}d", keyCode);
        return;
    }
    int32_t keyAction = (libinput_event_keyboard_get_key_state(data) == 0) ?
        (KeyEvent::KEY_ACTION_UP) : (KeyEvent::KEY_ACTION_DOWN);
    MMI_HILOGD("current keycode is %{private}d, keyaction is %{private}d", keyCode, keyAction);
    if (keyAction ==  KeyEvent::KEY_ACTION_DOWN) {
        iter->second.first = KEY_DOWN;
        SendFingerprintCancelEvent();
    } else {
        iter->second.first = KEY_UP;
        iter->second.second = std::chrono::steady_clock::now();
    }
}

void FingerprintEventProcessor::SetScreenState(struct libinput_event* event)
{
    CALL_DEBUG_ENTER;
    CHKPV(event);
    auto type = libinput_event_get_type(event);
    MMI_HILOGD("smart key screen state is %{public}d", type);
    switch (type) {
        case LIBINPUT_EVENT_TOUCH_DOWN: {
            screenState_ = true;
            break;
        }
        case LIBINPUT_EVENT_TOUCH_UP: {
            screenState_ = false;
            break;
        }
        default: {
            MMI_HILOGD("Unknown event type, touchType:%{public}d", type);
            return;
        }
    }
    ChangeScreenMissTouchFlag(screenState_, cancelState_);
}

void FingerprintEventProcessor::ChangeScreenMissTouchFlag(bool screen, bool cancel)
{
    int32_t flag = screenMissTouchFlag_ ? 1 : 0;
    MMI_HILOGD("screenMissTouchFlag_ : %{private}d, screen : %{private}d, cancel : %{private}d", flag, screen, screen);
    if (screenMissTouchFlag_ == false) {
        if (screen == true) {
            screenMissTouchFlag_ = true;
            // 上报cancel的逻辑是手指按下屏幕
            SendFingerprintCancelEvent();
            return;
        }
    } else {
        // 手指没有在屏幕，且目前为止收到cancel事件
        if (screen == false && cancel == true) {
            screenMissTouchFlag_ = false;
            return;
        }
    }
}

bool FingerprintEventProcessor::CheckMisTouchState()
{
    if (CheckKeyMisTouchState() || CheckScreenMisTouchState()) {
        return true;
    }
    return false;
}

bool FingerprintEventProcessor::CheckScreenMisTouchState()
{
    int32_t flag = screenMissTouchFlag_ ? 1 : 0;
    MMI_HILOGD("screenMissTouchFlag_ is %{public}d", flag);
    return screenMissTouchFlag_;
}

bool FingerprintEventProcessor::CheckKeyMisTouchState()
{
    CALL_DEBUG_ENTER;
    bool ret = false;
    for (auto &[key, value] : keyStateMap_) {
        auto keystate = value.first;
        MMI_HILOGD("keycode : %{private}d, state : %{public}d", key, value.first);
        if (keystate == KEY_DOWN) {
            ret = true;
        } else if (keystate == KEY_UP) {
            auto currentTime = std::chrono::steady_clock::now();
            auto duration = currentTime - value.second;
            auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            int32_t time = POWER_KEY_UP_TIME;
            if (key != KEY_POWER) {
                time = VOLUME_KEY_UP_TIME;
            }
            if (durationMs < time) {
                MMI_HILOGD("Dont report because time diff < threshold, keycode : %{private}d, state : %{public}d",
                    key, value.first);
                ret = true;
            } else {
                value.first = KEY_INIT;
            }
        }
    }
    MMI_HILOGD("KeyMisTouchState is %{public}d", ret);
    return ret;
}

int32_t FingerprintEventProcessor::SendFingerprintCancelEvent()
{
    CALL_DEBUG_ENTER;
    auto pointerEvent = PointerEvent::Create();
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_FINGERPRINT_CANCEL);
    int64_t time = GetSysClockTime();
    pointerEvent->SetActionTime(time);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_FINGERPRINT);
    pointerEvent->SetPointerId(0);
    EventLogHelper::PrintEventData(pointerEvent, MMI_LOG_HEADER);
    MMI_HILOGD("Fingerprint key:%{public}d", pointerEvent->GetPointerAction());
#if (defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)) && defined(OHOS_BUILD_ENABLE_MONITOR)
    auto eventMonitorHandler_ = InputHandler->GetMonitorHandler();
    if (eventMonitorHandler_ != nullptr) {
        eventMonitorHandler_->OnHandleEvent(pointerEvent);
    }
#endif
    return ERR_OK;
}

int32_t FingerprintEventProcessor::HandleFingerprintEvent(struct libinput_event* event)
{
    CALL_DEBUG_ENTER;
    CHKPR(event, ERROR_NULL_POINTER);
    auto device = libinput_event_get_device(event);
    CHKPR(device, PARAM_INPUT_INVALID);
    std::string name = libinput_device_get_name(device);
    if (name == FINGERPRINT_SOURCE_KEY) {
        return AnalyseKeyEvent(event);
    } else if (name == FINGERPRINT_SOURCE_POINT) {
        ProcessSlideEvent();
        return AnalysePointEvent(event);
    } else {
        MMI_HILOGI("Unknown input device name:%{public}s", name.c_str());
        return PARAM_INPUT_INVALID;
    }
}

int32_t FingerprintEventProcessor::AnalyseKeyEvent(struct libinput_event *event)
{
    CALL_DEBUG_ENTER;
    CHKPR(event, ERROR_NULL_POINTER);
    struct libinput_event_keyboard* keyEvent = libinput_event_get_keyboard_event(event);
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    auto key = libinput_event_keyboard_get_key(keyEvent);
    enum libinput_key_state state = libinput_event_keyboard_get_key_state(keyEvent);
    if (state == LIBINPUT_KEY_STATE_PRESSED) {
        MMI_HILOGI("Dont analyse the press status for %{public}d", key);
        return ERR_OK;
    }
    auto pointerEvent = PointerEvent::Create();
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    isStartedSmartKeyBySlide_ = false;
    switch (key) {
        case FINGERPRINT_CODE_DOWN: {
            cancelState_ = false;
            ChangeScreenMissTouchFlag(screenState_, true);
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_FINGERPRINT_DOWN);
            ReportResSched(ResourceSchedule::ResType::RES_TYPE_CLICK_RECOGNIZE,
                ResourceSchedule::ResType::ClickEventType::TOUCH_EVENT_DOWN);
            break;
        }
        case FINGERPRINT_CODE_CANCEL: {
            cancelState_ = true;
            ChangeScreenMissTouchFlag(screenState_, cancelState_);
            MMI_HILOGI("change cancel state and dont send point event");
            return RET_OK;
        }
        case FINGERPRINT_CODE_UP: {
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_FINGERPRINT_UP);
            ReportResSched(ResourceSchedule::ResType::RES_TYPE_CLICK_RECOGNIZE,
                ResourceSchedule::ResType::ClickEventType::TOUCH_EVENT_UP);
            break;
        }
        case FINGERPRINT_CODE_RETOUCH: {
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_FINGERPRINT_RETOUCH);
            ReportResSched(ResourceSchedule::ResType::RES_TYPE_CLICK_RECOGNIZE,
                ResourceSchedule::ResType::ClickEventType::TOUCH_EVENT_DOWN);
            break;
        }
        case FINGERPRINT_CODE_CLICK: {
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_FINGERPRINT_CLICK);
            ProcessClickEvent();
            break;
        }
        default:
            MMI_HILOGW("Unknown key event:%{public}d", key);
            return UNKNOWN_EVENT;
    }
    int64_t time = GetSysClockTime();
    pointerEvent->SetActionTime(time);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_FINGERPRINT);
    pointerEvent->SetPointerId(0);
    EventLogHelper::PrintEventData(pointerEvent, MMI_LOG_HEADER);
    MMI_HILOGD("Fingerprint key:%{public}d", pointerEvent->GetPointerAction());
    if (CheckMisTouchState()) {
        MMI_HILOGD("in mistouch state, dont report event");
        return ERR_OK;
    }
#if (defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)) && defined(OHOS_BUILD_ENABLE_MONITOR)
    auto eventMonitorHandler_ = InputHandler->GetMonitorHandler();
    if (eventMonitorHandler_ != nullptr) {
        eventMonitorHandler_->OnHandleEvent(pointerEvent);
    }
#endif // (OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH) && OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

int32_t FingerprintEventProcessor::AnalysePointEvent(libinput_event * event)
{
    CALL_DEBUG_ENTER;
    struct libinput_event_pointer* rawPointerEvent = libinput_event_get_pointer_event(event);
    CHKPR(rawPointerEvent, ERROR_NULL_POINTER);
    double ux = libinput_event_pointer_get_dx_unaccelerated(rawPointerEvent);
    double uy = libinput_event_pointer_get_dy_unaccelerated(rawPointerEvent);
    auto pointerEvent = PointerEvent::Create();
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    int64_t time = GetSysClockTime();
    pointerEvent->SetActionTime(time);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_FINGERPRINT_SLIDE);
    pointerEvent->SetFingerprintDistanceX(ux);
    pointerEvent->SetFingerprintDistanceY(uy);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_FINGERPRINT);
    pointerEvent->SetPointerId(0);
    EventLogHelper::PrintEventData(pointerEvent, MMI_LOG_HEADER);
    MMI_HILOGD("Fingerprint key:%{public}d, ux:%f, uy:%f", pointerEvent->GetPointerAction(), ux, uy);
    if (CheckMisTouchState()) {
        MMI_HILOGD("in mistouch state, dont report event");
        return ERR_OK;
    }
#if (defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)) && defined(OHOS_BUILD_ENABLE_MONITOR)
    auto eventMonitorHandler_ = InputHandler->GetMonitorHandler();
    if (eventMonitorHandler_ != nullptr) {
        eventMonitorHandler_->OnHandleEvent(pointerEvent);
    }
#endif // (OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH) && OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

template <class T>
void FingerprintEventProcessor::CreateStatusConfigObserver(T& item)
{
    CALL_DEBUG_ENTER;
    SettingObserver::UpdateFunc updateFunc = [&item](const std::string& key) {
        std::string value = NEED_SHOW_DIALOG;
        auto ret = SettingDataShare::GetInstance(MULTIMODAL_INPUT_SERVICE_ID)
            .GetStringValue(key, value);
        if (ret != RET_OK) {
            MMI_HILOGE("Get value from settings db failed, ret: %{public}d", ret);
            return;
        }
        MMI_HILOGI("Config changed, key: %{public}s, value: %{public}s", key.c_str(), value.c_str());
        item.valueString = value;
    };

    sptr<SettingObserver> statusObserver = SettingDataShare::GetInstance(MULTIMODAL_INPUT_SERVICE_ID)
        .CreateObserver(item.keyString, updateFunc);
    ErrCode ret = SettingDataShare::GetInstance(MULTIMODAL_INPUT_SERVICE_ID).RegisterObserver(statusObserver);
    if (ret != RET_OK) {
        MMI_HILOGE("Register setting observer failed, ret: %{public}d", ret);
        statusObserver = nullptr;
    }

    std::string value = NEED_SHOW_DIALOG;
    ret = SettingDataShare::GetInstance(MULTIMODAL_INPUT_SERVICE_ID)
        .SettingDataShare::GetStringValue(item.keyString, value);
    if (ret != RET_OK) {
        MMI_HILOGE("Get value from settings db failed, ret: %{public}d", ret);
        return;
    }
    MMI_HILOGI("Get value success, key: %{public}s, value: %{public}s", item.keyString.c_str(), value.c_str());
    item.valueString = value;
}

void FingerprintEventProcessor::StartSmartKeyIfNeeded()
{
    std::string isStartSmartKey = SMART_KEY_IS_CLOSE;
    ErrCode ret = SettingDataShare::GetInstance(MULTIMODAL_INPUT_SERVICE_ID)
        .SettingDataShare::GetStringValue(IS_SMART_KEY_USE, isStartSmartKey);
    if (ret != RET_OK) {
        MMI_HILOGE("Get value from settings db failed, ret: %{public}d", ret);
        return;
    }
    if (isStartSmartKey == SMART_KEY_IS_OPEN) {
        MMI_HILOGI("Before start smart-key");
        StartSmartKey(false);
    }
}

void FingerprintEventProcessor::StartSmartKey(bool isShowDialog)
{
    ffrt::submit([isShowDialog] {
        MMI_HILOGI("StartServiceExtAbility start");
        std::shared_ptr<AAFwk::AbilityManagerClient> abmc = AAFwk::AbilityManagerClient::GetInstance();
        CHKPV(abmc);
        const std::string smartKeyBundleName = "";
        const std::string smartKeyAbilityName = "";
        AAFwk::Want want;
        want.SetElementName(smartKeyBundleName, smartKeyAbilityName);
        if (isShowDialog) {
            want.SetParam("isShowDialog", IS_SHOW_DIALOG);
        }

        auto ret = abmc->StartExtensionAbility(want, nullptr, -1, AppExecFwk::ExtensionAbilityType::SERVICE);
        if (ret != RET_OK) {
            MMI_HILOGE("StartExtensionAbility failed, ret: %{public}d", ret);
            return;
        }
        MMI_HILOGI("StartServiceExtAbility finished");
    });
}

void FingerprintEventProcessor::ProcessSlideEvent()
{
    if ((smartKeySwitch_.valueString == NEED_SHOW_DIALOG || smartKeySwitch_.valueString.empty()) &&
        !isStartedSmartKeyBySlide_) {
        isStartedSmartKeyBySlide_ = true;
        StartSmartKey(true);
    }
}

void FingerprintEventProcessor::ProcessClickEvent()
{
    if (smartKeySwitch_.valueString == NEED_SHOW_DIALOG || smartKeySwitch_.valueString.empty()) {
        StartSmartKey(true);
    }
    ReportResSched(ResourceSchedule::ResType::RES_TYPE_CLICK_RECOGNIZE,
        ResourceSchedule::ResType::ClickEventType::TOUCH_EVENT_DOWN);
}

void FingerprintEventProcessor::ReportResSched(uint32_t resType, int64_t value)
{
    std::unordered_map<std::string, std::string> payload { {"msg", ""} };
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(resType, value, payload);
}
#endif // OHOS_BUILD_ENABLE_FINGERPRINT
} // namespace MMI
} // namespace OHOS
