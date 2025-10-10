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

#include "key_subscriber_handler.h"

#include "app_state_observer.h"
#include "bytrace_adapter.h"
#ifdef OHOS_BUILD_ENABLE_CALL_MANAGER
#include "call_manager_client.h"
#endif // OHOS_BUILD_ENABLE_CALL_MANAGER
#include "display_event_monitor.h"
#include "device_event_monitor.h"
#include "dfx_hisysevent.h"
#include "display_event_monitor.h"
#include "event_log_helper.h"
#include "input_event_data_transformation.h"
#include "input_event_handler.h"
#include "key_auto_repeat.h"
#ifdef OHOS_BUILD_ENABLE_KEY_PRESSED_HANDLER
#include "key_monitor_manager.h"
#endif // OHOS_BUILD_ENABLE_KEY_PRESSED_HANDLER
#ifdef SHORTCUT_KEY_MANAGER_ENABLED
#include "key_shortcut_manager.h"
#endif // SHORTCUT_KEY_MANAGER_ENABLED
#include "setting_datashare.h"
#include "util_ex.h"
#include "want.h"
#include "tablet_subscriber_handler.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_HANDLER
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "KeySubscriberHandler"

namespace OHOS {
namespace MMI {
namespace {
constexpr uint32_t MAX_PRE_KEY_COUNT { 4 };
constexpr int32_t REMOVE_OBSERVER { -2 };
constexpr int32_t UNOBSERVED { -1 };
constexpr int32_t ACTIVE_EVENT { 2 };
constexpr int32_t NORMAL_CALL { 0 };
constexpr int32_t VOIP_CALL { 1 };
#ifdef OHOS_BUILD_ENABLE_CALL_MANAGER
std::shared_ptr<OHOS::Telephony::CallManagerClient> callManagerClientPtr = nullptr;
#endif // OHOS_BUILD_ENABLE_CALL_MANAGER
const std::string CALL_BEHAVIOR_KEY { "incall_power_button_behavior" };
const std::string SETTINGS_DATA_SYSTEM_URI {
    "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_100?Proxy=true" };
const char* SETTINGS_DATA_EXT_URI {
    "datashare:///com.ohos.USER_SETTINGSDATA_100.DataAbility" };
} // namespace

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
void KeySubscriberHandler::HandleKeyEvent(const std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPV(keyEvent);
    if (OnSubscribeKeyEvent(keyEvent)) {
        if (DISPLAY_MONITOR->GetScreenStatus() == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF) {
            auto monitorHandler = InputHandler->GetMonitorHandler();
            CHKPV(monitorHandler);
            keyEvent->SetFourceMonitorFlag(true);
#ifndef OHOS_BUILD_EMULATOR
            monitorHandler->OnHandleEvent(keyEvent);
#endif // OHOS_BUILD_EMULATOR
            keyEvent->SetFourceMonitorFlag(false);
        }
        if (EventLogHelper::IsBetaVersion() && !keyEvent->HasFlag(InputEvent::EVENT_FLAG_PRIVACY_MODE)) {
            MMI_HILOGD("Subscribe keyEvent filter success:%{private}d", keyEvent->GetKeyCode());
        } else {
            MMI_HILOGD("Subscribe keyEvent filter success:%{private}d", keyEvent->GetKeyCode());
        }
        BytraceAdapter::StartBytrace(keyEvent, BytraceAdapter::KEY_SUBSCRIBE_EVENT);
        DfxHisysevent::ReportKeyEvent("subcriber");

        if ((keyEvent->GetKeyAction() == KeyEvent::KEY_ACTION_DOWN) ||
            (pendingKeys_.find(keyEvent->GetKeyCode()) == pendingKeys_.cend())) {
            return;
        }
        pendingKeys_.erase(keyEvent->GetKeyCode());
        if (keyEvent->GetKeyAction() == KeyEvent::KEY_ACTION_UP) {
            keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_CANCEL);
        }
    } else if (keyEvent->GetKeyAction() == KeyEvent::KEY_ACTION_DOWN) {
        pendingKeys_.emplace(keyEvent->GetKeyCode());
    }
    CHKPV(nextHandler_);
    nextHandler_->HandleKeyEvent(keyEvent);
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

#ifdef OHOS_BUILD_ENABLE_POINTER
void KeySubscriberHandler::HandlePointerEvent(const std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    CHKPV(nextHandler_);
    nextHandler_->HandlePointerEvent(pointerEvent);
}
#endif // OHOS_BUILD_ENABLE_POINTER

#ifdef OHOS_BUILD_ENABLE_TOUCH
void KeySubscriberHandler::HandleTouchEvent(const std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    TABLET_SCRIBER_HANDLER->HandleTabletEvent(pointerEvent);
    CHKPV(nextHandler_);
    nextHandler_->HandleTouchEvent(pointerEvent);
}
#endif // OHOS_BUILD_ENABLE_TOUCH

int32_t KeySubscriberHandler::SubscribeKeyEvent(
    SessionPtr sess, int32_t subscribeId, std::shared_ptr<KeyOption> keyOption)
{
    CALL_DEBUG_ENTER;
    if (subscribeId < 0) {
        MMI_HILOGE("Invalid subscribe");
        return RET_ERR;
    }
    CHKPR(sess, ERROR_NULL_POINTER);
    CHKPR(keyOption, ERROR_NULL_POINTER);
    uint32_t preKeySize = keyOption->GetPreKeys().size();
    if (preKeySize > MAX_PRE_KEY_COUNT) {
        MMI_HILOGE("Leave, preKeySize:%{public}u", preKeySize);
        return RET_ERR;
    }

    for (const auto &keyCode : keyOption->GetPreKeys()) {
        MMI_HILOGD("keyOption->prekey:%{private}d", keyCode);
    }
    MMI_HILOGD("SubscribeId:%{public}d, finalKey:%{private}d,"
        "isFinalKeyDown:%{public}s, finalKeyDownDuration:%{public}d, pid:%{public}d",
        subscribeId, keyOption->GetFinalKey(), keyOption->IsFinalKeyDown() ? "true" : "false",
        keyOption->GetFinalKeyDownDuration(), sess->GetPid());
    DfxHisysevent::ReportSubscribeKeyEvent(subscribeId, keyOption->GetFinalKey(),
        sess->GetProgramName(), sess->GetPid());
    auto subscriber = std::make_shared<Subscriber>(subscribeId, sess, keyOption);
    if (keyGestureMgr_.ShouldIntercept(keyOption)) {
        auto ret = AddKeyGestureSubscriber(subscriber, keyOption);
        if (ret != RET_OK) {
            MMI_HILOGE("AddKeyGestureSubscriber fail, error:%{public}d", ret);
            DfxHisysevent::ReportFailSubscribeKey("SubscribeKeyEvent", sess->GetProgramName(),
                keyOption->GetFinalKey(), DfxHisysevent::KEY_ERROR_CODE::ERROR_RETURN_VALUE);
            return ret;
        }
    } else {
        auto ret = AddSubscriber(subscriber, keyOption, true);
        if (ret != RET_OK) {
            MMI_HILOGE("AddSubscriber fail, error:%{public}d", ret);
            DfxHisysevent::ReportFailSubscribeKey("SubscribeKeyEvent", sess->GetProgramName(),
                keyOption->GetFinalKey(), DfxHisysevent::KEY_ERROR_CODE::ERROR_RETURN_VALUE);
            return ret;
        }
    }
    InitSessionDeleteCallback();
    return RET_OK;
}

int32_t KeySubscriberHandler::UnsubscribeKeyEvent(SessionPtr sess, int32_t subscribeId)
{
    CHKPR(sess, ERROR_NULL_POINTER);
    MMI_HILOGI("SubscribeId:%{public}d, pid:%{public}d", subscribeId, sess->GetPid());
    int32_t ret = RemoveSubscriber(sess, subscribeId, true);
    if (ret != RET_OK) {
        ret = RemoveKeyGestureSubscriber(sess, subscribeId);
    }
    if (ret == RET_ERR) {
        DfxHisysevent::ReportFailSubscribeKey("UnsubscribeKeyEvent", sess->GetProgramName(),
            subscribeId, DfxHisysevent::KEY_ERROR_CODE::ERROR_RETURN_VALUE);
    }
    return ret;
}

int32_t KeySubscriberHandler::RemoveSubscriber(SessionPtr sess, int32_t subscribeId, bool isSystem)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> lock(subscriberMapMutex_);
    for (auto iter = subscriberMap_.begin(); iter != subscriberMap_.end(); iter++) {
        auto &subscribers = iter->second;
        for (auto it = subscribers.begin(); it != subscribers.end(); it++) {
            if ((*it)->id_ == subscribeId && (*it)->sess_ == sess) {
                ClearTimer(*it);
                auto option = (*it)->keyOption_;
                CHKPR(option, ERROR_NULL_POINTER);
#ifdef SHORTCUT_KEY_MANAGER_ENABLED
                if (isSystem) {
                    KEY_SHORTCUT_MGR->UnregisterSystemKey((*it)->shortcutId_);
                } else {
                    KEY_SHORTCUT_MGR->UnregisterHotKey((*it)->shortcutId_);
                }
#endif // SHORTCUT_KEY_MANAGER_ENABLED
                MMI_HILOGI("SubscribeId:%{public}d, finalKey:%{private}d, isFinalKeyDown:%{public}s,"
                    "finalKeyDownDuration:%{public}d, pid:%{public}d", subscribeId, option->GetFinalKey(),
                    option->IsFinalKeyDown() ? "true" : "false", option->GetFinalKeyDownDuration(), sess->GetPid());
                subscribers.erase(it);
                DfxHisysevent::ReportUnSubscribeKeyEvent(subscribeId, option->GetFinalKey(),
                    sess->GetProgramName(), sess->GetPid());
                return RET_OK;
            }
        }
    }
    return RET_ERR;
}

int32_t KeySubscriberHandler::AddKeyGestureSubscriber(
    std::shared_ptr<Subscriber> subscriber, std::shared_ptr<KeyOption> keyOption)
{
    CALL_INFO_TRACE;
    CHKPR(subscriber, RET_ERR);
    CHKPR(subscriber->sess_, RET_ERR);
    subscriber->timerId_ = keyGestureMgr_.AddKeyGesture(subscriber->sess_->GetPid(), keyOption,
        [this, subscriber](std::shared_ptr<KeyEvent> keyEvent) {
            NotifySubscriber(keyEvent, subscriber);
        });
    if (subscriber->timerId_ < 0) {
        MMI_HILOGE("AddKeyGesture fail, error:%{public}d", subscriber->timerId_);
        return RET_ERR;
    }
    MMI_HILOGD("Handler(%{public}d) of key gesture was added", subscriber->timerId_);
    PrintKeyOption(keyOption);
    for (auto &iter : keyGestures_) {
        if (IsEqualKeyOption(keyOption, iter.first)) {
            iter.second.push_back(subscriber);
            return RET_OK;
        }
    }
    keyGestures_[keyOption] = { subscriber };
    return RET_OK;
}

int32_t KeySubscriberHandler::RemoveKeyGestureSubscriber(SessionPtr sess, int32_t subscribeId)
{
    CALL_INFO_TRACE;
    for (auto iter = keyGestures_.begin(); iter != keyGestures_.end(); ++iter) {
        auto &subscribers = iter->second;

        for (auto innerIter = subscribers.begin(); innerIter != subscribers.end(); ++innerIter) {
            auto subscriber = *innerIter;

            if ((subscriber->id_ != subscribeId) || (subscriber->sess_ != sess)) {
                continue;
            }
            MMI_HILOGD("Removing handler(%{public}d) of key gesture", subscriber->timerId_);
            keyGestureMgr_.RemoveKeyGesture(subscriber->timerId_);
            auto option = subscriber->keyOption_;
            MMI_HILOGD("SubscribeId:%{public}d, finalKey:%{private}d, isFinalKeyDown:%{public}s,"
                "finalKeyDownDuration:%{public}d, pid:%{public}d", subscribeId, option->GetFinalKey(),
                option->IsFinalKeyDown() ? "true" : "false", option->GetFinalKeyDownDuration(), sess->GetPid());
            subscribers.erase(innerIter);
            return RET_OK;
        }
    }
    return RET_ERR;
}

#ifdef SHORTCUT_KEY_MANAGER_ENABLED
int32_t KeySubscriberHandler::RegisterSystemKey(std::shared_ptr<KeyOption> option,
    int32_t session, std::function<void(std::shared_ptr<KeyEvent>)> callback)
{
    KeyShortcutManager::SystemShortcutKey sysKey {
        .modifiers = option->GetPreKeys(),
        .finalKey = option->GetFinalKey(),
        .longPressTime = option->GetFinalKeyDownDuration(),
        .triggerType = (option->IsFinalKeyDown() ? KeyShortcutManager::SHORTCUT_TRIGGER_TYPE_DOWN :
                                                   KeyShortcutManager::SHORTCUT_TRIGGER_TYPE_UP),
        .session = session,
        .callback = callback,
    };
    if (KeyShortcutManager::IsModifier(sysKey.finalKey) &&
        !sysKey.modifiers.empty() &&
        std::all_of(sysKey.modifiers.cbegin(), sysKey.modifiers.cend(),
            [](auto key) {
                return KeyShortcutManager::IsModifier(key);
            })) {
        sysKey.modifiers.insert(sysKey.finalKey);
        sysKey.finalKey = KeyShortcutManager::SHORTCUT_PURE_MODIFIERS;
    }
    return KEY_SHORTCUT_MGR->RegisterSystemKey(sysKey);
}

int32_t KeySubscriberHandler::RegisterHotKey(std::shared_ptr<KeyOption> option,
    int32_t session, std::function<void(std::shared_ptr<KeyEvent>)> callback)
{
    KeyShortcutManager::HotKey hotKey {
        .modifiers = option->GetPreKeys(),
        .finalKey = option->GetFinalKey(),
        .longPressTime = option->GetFinalKeyDownDuration(),
        .session = session,
        .callback = callback,
    };
    return KEY_SHORTCUT_MGR->RegisterHotKey(hotKey);
}

void KeySubscriberHandler::UnregisterSystemKey(int32_t shortcutId)
{
    KEY_SHORTCUT_MGR->UnregisterSystemKey(shortcutId);
}

void KeySubscriberHandler::UnregisterHotKey(int32_t shortcutId)
{
    KEY_SHORTCUT_MGR->UnregisterHotKey(shortcutId);
}

void KeySubscriberHandler::DeleteShortcutId(std::shared_ptr<Subscriber> subscriber)
{
    CHKPV(subscriber);
    if (subscriber->isSystem) {
        UnregisterSystemKey(subscriber->shortcutId_);
    } else {
        UnregisterHotKey(subscriber->shortcutId_);
    }
}
#endif // SHORTCUT_KEY_MANAGER_ENABLED

int32_t KeySubscriberHandler::SubscribeHotkey(
    SessionPtr sess, int32_t subscribeId, std::shared_ptr<KeyOption> keyOption)
{
    CHKPR(sess, ERROR_NULL_POINTER);
    CHKPR(keyOption, ERROR_NULL_POINTER);
    uint32_t preKeySize = keyOption->GetPreKeys().size();
    if (preKeySize > MAX_PRE_KEY_COUNT) {
        MMI_HILOGE("Leave, preKeySize:%{public}u", preKeySize);
        return RET_ERR;
    }
    for (const auto &keyCode : keyOption->GetPreKeys()) {
        MMI_HILOGD("keyOption->prekey:%{private}d", keyCode);
    }
    MMI_HILOGI("SubscribeId:%{public}d, finalKey:%{private}d,"
        "isFinalKeyDown:%{public}s, finalKeyDownDuration:%{public}d, pid:%{public}d",
        subscribeId, keyOption->GetFinalKey(), keyOption->IsFinalKeyDown() ? "true" : "false",
        keyOption->GetFinalKeyDownDuration(), sess->GetPid());
    auto subscriber = std::make_shared<Subscriber>(subscribeId, sess, keyOption);
    auto ret = AddSubscriber(subscriber, keyOption, false);
    if (ret != RET_OK) {
        MMI_HILOGE("AddSubscriber fail, error:%{public}d", ret);
        DfxHisysevent::ReportFailSubscribeKey("SubscribeHotkey", sess->GetProgramName(),
            keyOption->GetFinalKey(), DfxHisysevent::KEY_ERROR_CODE::ERROR_RETURN_VALUE);
        return ret;
    }
    InitSessionDeleteCallback();
    return RET_OK;
}

int32_t KeySubscriberHandler::UnsubscribeHotkey(SessionPtr sess, int32_t subscribeId)
{
    CHKPR(sess, ERROR_NULL_POINTER);
    MMI_HILOGI("SubscribeId:%{public}d, pid:%{public}d", subscribeId, sess->GetPid());
    int32_t ret = RemoveSubscriber(sess, subscribeId, false);
    if (ret != RET_OK) {
        MMI_HILOGW("No hot key subscription(%{public}d, No.%{public}d)", sess->GetPid(), subscribeId);
        DfxHisysevent::ReportFailSubscribeKey("UnsubscribeHotkey", sess->GetProgramName(),
            subscribeId, DfxHisysevent::KEY_ERROR_CODE::ERROR_RETURN_VALUE);
    }
    return ret;
}

int32_t KeySubscriberHandler::AddSubscriber(std::shared_ptr<Subscriber> subscriber,
    std::shared_ptr<KeyOption> option, bool isSystem)
{
    CALL_DEBUG_ENTER;
    CHKPR(subscriber, RET_ERR);
    CHKPR(option, RET_ERR);
    PrintKeyOption(option);
#ifdef SHORTCUT_KEY_MANAGER_ENABLED
    CHKPR(subscriber->sess_, RET_ERR);
    if (isSystem) {
        subscriber->isSystem = true;
        subscriber->shortcutId_ = RegisterSystemKey(option, subscriber->sess_->GetPid(),
            [this, subscriber](std::shared_ptr<KeyEvent> keyEvent) {
                NotifySubscriber(keyEvent, subscriber);
            });
    } else {
        subscriber->isSystem = false;
        subscriber->shortcutId_ = RegisterHotKey(option, subscriber->sess_->GetPid(),
            [this, subscriber](std::shared_ptr<KeyEvent> keyEvent) {
                NotifySubscriber(keyEvent, subscriber);
            });
    }
    if (subscriber->shortcutId_ < 0) {
        MMI_HILOGE("Register shortcut fail, error:%{public}d", subscriber->shortcutId_);
        return subscriber->shortcutId_;
    }
#endif // SHORTCUT_KEY_MANAGER_ENABLED
    std::lock_guard<std::mutex> lock(subscriberMapMutex_);
    for (auto &iter : subscriberMap_) {
        if (IsEqualKeyOption(option, iter.first)) {
            MMI_HILOGI("Add subscriber Id:%{public}d, pid:%{public}d", subscriber->id_, subscriber->sess_->GetPid());
            iter.second.push_back(std::move(subscriber));
            MMI_HILOGD("Subscriber size:%{public}zu", iter.second.size());
            return RET_OK;
        }
    }
    MMI_HILOGI("Add subscriber Id:%{public}d", subscriber->id_);
    subscriberMap_[option] = {subscriber};
    return RET_OK;
}

bool KeySubscriberHandler::IsEqualKeyOption(std::shared_ptr<KeyOption> newOption,
    std::shared_ptr<KeyOption> oldOption)
{
    CALL_DEBUG_ENTER;
    CHKPF(newOption);
    CHKPF(oldOption);
    if (!IsEqualPreKeys(newOption->GetPreKeys(), oldOption->GetPreKeys())) {
        MMI_HILOGD("Pre key not match");
        return false;
    }
    if (newOption->GetFinalKey() != oldOption->GetFinalKey()) {
        MMI_HILOGD("Final key not match");
        return false;
    }
    if (newOption->IsFinalKeyDown() != oldOption->IsFinalKeyDown()) {
        MMI_HILOGD("Is final key down not match");
        return false;
    }
    if (newOption->GetFinalKeyDownDuration() != oldOption->GetFinalKeyDownDuration()) {
        MMI_HILOGD("Final key down duration not match");
        return false;
    }
    if (newOption->GetFinalKeyUpDelay() != oldOption->GetFinalKeyUpDelay()) {
        MMI_HILOGD("Is final key up delay not match");
        return false;
    }
    MMI_HILOGD("Key option match");
    return true;
}

void KeySubscriberHandler::GetForegroundPids(std::set<int32_t> &pids)
{
    CALL_DEBUG_ENTER;
    std::vector<AppExecFwk::AppStateData> list = APP_OBSERVER_MGR->GetForegroundAppData();
    for (auto iter = list.begin(); iter != list.end(); iter++) {
        MMI_HILOGD("Foreground process pid:%{public}d", (*iter).pid);
        pids.insert((*iter).pid);
    }
}

int32_t KeySubscriberHandler::EnableCombineKey(bool enable)
{
    enableCombineKey_ = enable;
    MMI_HILOGI("Enable combineKey is successful in subscribe handler, enable:%{public}d", enable);
    return RET_OK;
}

void KeySubscriberHandler::ResetSkipPowerKeyUpFlag()
{
    MMI_HILOGI("Reset needSkipPowerKeyUp when hang up.");
    needSkipPowerKeyUp_ = false;
}

bool KeySubscriberHandler::IsFunctionKey(const std::shared_ptr<KeyEvent> keyEvent)
{
    MMI_HILOGD("Is Funciton Key In");
    if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_BRIGHTNESS_DOWN
        || keyEvent->GetKeyCode() == KeyEvent::KEYCODE_BRIGHTNESS_UP) {
        return true;
    }
    if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_VOLUME_UP
        || keyEvent->GetKeyCode() == KeyEvent::KEYCODE_VOLUME_DOWN
        || keyEvent->GetKeyCode() == KeyEvent::KEYCODE_VOLUME_MUTE) {
        return true;
    }
    if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_MUTE
        || keyEvent->GetKeyCode() == KeyEvent::KEYCODE_SWITCHVIDEOMODE
        || keyEvent->GetKeyCode() == KeyEvent::KEYCODE_MEDIA_RECORD) {
        return true;
    }
    return false;
}

bool KeySubscriberHandler::IsEnableCombineKey(const std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPF(keyEvent);
    if (enableCombineKey_) {
        return true;
    }
    if (IsFunctionKey(keyEvent)) {
        auto items = keyEvent->GetKeyItems();
        return items.size() != 1 ? enableCombineKey_ : true;
    }
    if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_POWER) {
        auto items = keyEvent->GetKeyItems();
        if (items.size() != 1) {
            return enableCombineKey_;
        }
        return true;
    }
    if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_DPAD_RIGHT ||
        keyEvent->GetKeyCode() == KeyEvent::KEYCODE_DPAD_LEFT) {
        if (EventLogHelper::IsBetaVersion() && !keyEvent->HasFlag(InputEvent::EVENT_FLAG_PRIVACY_MODE)) {
            MMI_HILOGD("Subscriber mulit swipe:%{private}d", keyEvent->GetKeyCode());
        } else {
            MMI_HILOGD("Subscriber mulit swipe:%{private}d", keyEvent->GetKeyCode());
        }
        return IsEnableCombineKeySwipe(keyEvent);
    }
    if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_R) {
        return IsEnableCombineKeyRecord(keyEvent);
    }
    if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_L) {
        for (const auto &item : keyEvent->GetKeyItems()) {
            int32_t keyCode = item.GetKeyCode();
            if (keyCode != KeyEvent::KEYCODE_L && keyCode != KeyEvent::KEYCODE_META_LEFT &&
                keyCode != KeyEvent::KEYCODE_META_RIGHT) {
                return enableCombineKey_;
            }
        }
        return true;
    }
    if (!InterceptByVm(keyEvent)) {
        return true;
    }

    return enableCombineKey_;
}

bool KeySubscriberHandler::IsEnableCombineKeySwipe(const std::shared_ptr<KeyEvent> keyEvent)
{
    for (const auto &item : keyEvent->GetKeyItems()) {
        int32_t keyCode = item.GetKeyCode();
        if (keyCode != KeyEvent::KEYCODE_CTRL_LEFT && keyCode != KeyEvent::KEYCODE_META_LEFT &&
            keyCode != KeyEvent::KEYCODE_DPAD_RIGHT && keyCode != KeyEvent::KEYCODE_CTRL_RIGHT &&
            keyCode != KeyEvent::KEYCODE_DPAD_LEFT) {
            return enableCombineKey_;
        }
    }
    return true;
}

bool KeySubscriberHandler::IsEnableCombineKeyRecord(const std::shared_ptr<KeyEvent> keyEvent)
{
    for (const auto &item : keyEvent->GetKeyItems()) {
        int32_t keyCode = item.GetKeyCode();
        if (keyCode != KeyEvent::KEYCODE_SHIFT_LEFT && keyCode != KeyEvent::KEYCODE_META_LEFT &&
            keyCode != KeyEvent::KEYCODE_SHIFT_RIGHT && keyCode != KeyEvent::KEYCODE_META_RIGHT &&
            keyCode != KeyEvent::KEYCODE_R) {
            return enableCombineKey_;
        }
    }
    return true;
}

bool KeySubscriberHandler::InterceptByVm(const std::shared_ptr<KeyEvent> keyEvt)
{
    // logo + leftShift + E is used by sceneboard, do not intercept by vm
    const std::vector<int32_t> LOGO_LEFTSHIFT_E = {
        KeyEvent::KEYCODE_META_LEFT, KeyEvent::KEYCODE_SHIFT_LEFT, KeyEvent::KEYCODE_E};
    size_t waitMatchCnt{LOGO_LEFTSHIFT_E.size()};
    if (keyEvt->GetKeyItems().size() != waitMatchCnt) {
        return true;
    }
    for (auto &&keyItem : keyEvt->GetKeyItems()) {
        for (auto &&k : LOGO_LEFTSHIFT_E) {
            if (keyItem.GetKeyCode() == k) {
                --waitMatchCnt;
                break;
            };
        }
    }
    if (waitMatchCnt == 0) {
        return false;
    }
    return true;
}

void KeySubscriberHandler::PublishKeyPressCommonEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    OHOS::AAFwk::Want want;
    want.SetAction("multimodal.event.MUTE_KEY_PRESS");
    want.SetParam("keyCode", keyEvent->GetKeyCode());
    EventFwk::CommonEventPublishInfo publishInfo;
    std::vector<std::string> permissionVec {"ohos.permission.NOTIFICATION_CONTROLLER"};
    publishInfo.SetSubscriberPermissions(permissionVec);
    EventFwk::CommonEventData commonData {want};
    EventFwk::CommonEventManager::PublishCommonEvent(commonData, publishInfo);
}

#ifdef OHOS_BUILD_ENABLE_CALL_MANAGER
bool KeySubscriberHandler::HandleRingMute(std::shared_ptr<KeyEvent> keyEvent)
{
    CALL_DEBUG_ENTER;
    CHKPF(keyEvent);
    if (keyEvent->GetKeyCode() != KeyEvent::KEYCODE_VOLUME_DOWN &&
        keyEvent->GetKeyCode() != KeyEvent::KEYCODE_VOLUME_UP &&
        keyEvent->GetKeyCode() != KeyEvent::KEYCODE_POWER) {
        MMI_HILOGD("There is no need to set mute");
        return false;
    }
    int32_t ret = -1;
    PublishKeyPressCommonEvent(keyEvent);
    if (DEVICE_MONITOR->GetCallState() == StateType::CALL_STATUS_INCOMING ||
        DEVICE_MONITOR->GetCallState() == StateType::CALL_STATUS_WAITING ||
        DEVICE_MONITOR->GetVoipCallState() == StateType::CALL_STATUS_INCOMING ||
        DEVICE_MONITOR->GetVoipCallState() == StateType::CALL_STATUS_WAITING) {
        if (callManagerClientPtr == nullptr) {
            callManagerClientPtr = DelayedSingleton<OHOS::Telephony::CallManagerClient>::GetInstance();
            if (callManagerClientPtr == nullptr) {
                MMI_HILOGE("CallManager init fail");
                return false;
            }
            auto begin = std::chrono::high_resolution_clock::now();
            callManagerClientPtr->Init(OHOS::TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID);
            auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - begin).count();
#ifdef OHOS_BUILD_ENABLE_DFX_RADAR
            DfxHisysevent::ReportApiCallTimes(ApiDurationStatistics::Api::TELEPHONY_CALL_MGR_INIT, durationMS);
#endif // OHOS_BUILD_ENABLE_DFX_RADAR
        }
        if (!DEVICE_MONITOR->GetHasHandleRingMute()) {
            auto begin = std::chrono::high_resolution_clock::now();
            ret = callManagerClientPtr->MuteRinger();
            auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - begin).count();
#ifdef OHOS_BUILD_ENABLE_DFX_RADAR
            DfxHisysevent::ReportApiCallTimes(ApiDurationStatistics::Api::TELEPHONY_CALL_MGR_MUTE_RINGER, durationMS);
#endif // OHOS_BUILD_ENABLE_DFX_RADAR
            if (ret != ERR_OK) {
                MMI_HILOGE("Set mute fail, ret:%{public}d", ret);
                return false;
            }
#if defined(OHOS_BUILD_ENABLE_DFX_RADAR) && defined(OHOS_BUILD_ENABLE_WATCH)
            DfxHisysevent::ReportCallingMute();
#endif // defined(OHOS_BUILD_ENABLE_DFX_RADAR) && defined(OHOS_BUILD_ENABLE_WATCH)
            MMI_HILOGW("Set mute success");
            DEVICE_MONITOR->SetHasHandleRingMute(true);
            if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_POWER) {
                needSkipPowerKeyUp_ = true;
            }
            return true;
        } else {
            if (keyEvent->GetKeyCode() != KeyEvent::KEYCODE_POWER) {
                MMI_HILOGI("Set mute success, block volumeKey");
                return true;
            }
        }
    }
    return false;
}
#endif // OHOS_BUILD_ENABLE_CALL_MANAGER

bool KeySubscriberHandler::OnSubscribeKeyEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    CALL_DEBUG_ENTER;
    auto powerKeyLogger = [keyEvent] (const std::string &log) {
        if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_POWER) {
            MMI_HILOGI("Power key, action:%{public}d; %{public}s", keyEvent->GetKeyAction(), log.c_str());
        }
    };
    CHKPF(keyEvent);
#ifdef OHOS_BUILD_ENABLE_CALL_MANAGER
    if (HandleRingMute(keyEvent)) {
        MMI_HILOGI("Mute Ring in subscribe keyEvent");
        RemoveSubscriberTimer(keyEvent);
        return true;
    }
    if (HandleCallEnded(keyEvent)) {
        MMI_HILOGI("Call Ended in subscribe keyEvent");
        return true;
    }
#endif // OHOS_BUILD_ENABLE_CALL_MANAGER
    if (!IsEnableCombineKey(keyEvent)) {
        MMI_HILOGI("Combine key is taken over in subscribe keyEvent");
        return false;
    }
    if (keyGestureMgr_.Intercept(keyEvent)) {
        MMI_HILOGD("Key gesture recognized");
        powerKeyLogger("keyGestureMgr::Intercept");
        return true;
    }
#ifdef OHOS_BUILD_ENABLE_KEY_PRESSED_HANDLER
    if (KEY_MONITOR_MGR->Intercept(keyEvent)) {
        MMI_HILOGD("Key monitor intercept (KC:%{private}d, KA:%{public}d)",
            keyEvent->GetKeyCode(), keyEvent->GetKeyAction());
        powerKeyLogger("KEY_MONITOR_MGR::Intercept");
        return true;
    }
#endif // #ifdef OHOS_BUILD_ENABLE_KEY_PRESSED_HANDLER
    if (IsRepeatedKeyEvent(keyEvent)) {
        MMI_HILOGD("Repeat KeyEvent, skip");
        powerKeyLogger("IsRepeatedKeyEvent");
        return true;
    }
    return ProcessKeyEvent(keyEvent);
}

bool KeySubscriberHandler::ProcessKeyEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    keyEvent_ = KeyEvent::Clone(keyEvent);
    int32_t keyAction = keyEvent->GetKeyAction();
    if (EventLogHelper::IsBetaVersion() && !keyEvent->HasFlag(InputEvent::EVENT_FLAG_PRIVACY_MODE)) {
        MMI_HILOGD("code:%{private}d, keyAction:%{public}s", keyEvent->GetKeyCode(),
            KeyEvent::ActionToString(keyAction));
    } else {
        MMI_HILOGD("code:%{private}d, keyAction:%{public}s",
            keyEvent->GetKeyCode(), KeyEvent::ActionToString(keyAction));
    }
    if (needSkipPowerKeyUp_ && keyEvent->GetKeyCode() == KeyEvent::KEYCODE_POWER
        && keyAction == KeyEvent::KEY_ACTION_UP) {
        MMI_HILOGI("Skip power key up");
        needSkipPowerKeyUp_ = false;
        return true;
    }
    for (const auto &keyCode : keyEvent->GetPressedKeys()) {
        if (EventLogHelper::IsBetaVersion() && !keyEvent->HasFlag(InputEvent::EVENT_FLAG_PRIVACY_MODE)) {
            MMI_HILOGD("Pressed:%{private}d", keyCode);
        } else {
            MMI_HILOGD("Pressed:%{private}d", keyCode);
        }
    }
    bool handled = false;
    if (keyAction == KeyEvent::KEY_ACTION_DOWN) {
        handled = HandleKeyDown(keyEvent);
    } else if (keyAction == KeyEvent::KEY_ACTION_UP) {
        hasEventExecuting_ = false;
        handled = HandleKeyUp(keyEvent);
    } else if (keyAction == KeyEvent::KEY_ACTION_CANCEL) {
        hasEventExecuting_ = false;
        DfxHisysevent::ReportKeyEvent("cancel");
        handled = HandleKeyCancel(keyEvent);
    } else {
        MMI_HILOGW("keyAction exception");
    }
    return handled;
}

void KeySubscriberHandler::OnSessionDelete(SessionPtr sess)
{
    CALL_DEBUG_ENTER;
    CHKPV(sess);
    {
        std::lock_guard<std::mutex> lock(subscriberMapMutex_);
        for (auto iter = subscriberMap_.begin(); iter != subscriberMap_.end(); iter++) {
            auto &subscribers = iter->second;
            for (auto it = subscribers.begin(); it != subscribers.end();) {
                if ((*it)->sess_ == sess) {
                    ClearTimer(*it);
#ifdef SHORTCUT_KEY_MANAGER_ENABLED
                    DeleteShortcutId(*it);
#endif // SHORTCUT_KEY_MANAGER_ENABLED
                    subscribers.erase(it++);
                    continue;
                }
                ++it;
            }
        }
    }
    for (auto iter = keyGestures_.begin(); iter != keyGestures_.end();) {
        auto &subscribers = iter->second;
        for (auto inner = subscribers.begin(); inner != subscribers.end();) {
            auto subscriber = *inner;
            if (subscriber->sess_ == sess) {
                MMI_HILOGI("Removing key-gesture handler(%{public}d) on subscriber(%{public}d) dying",
                    subscriber->timerId_, sess->GetPid());
                keyGestureMgr_.RemoveKeyGesture(subscriber->timerId_);
                auto option = subscriber->keyOption_;
                CHKPC(option);
                MMI_HILOGI("SubscribeId:%{public}d, finalKey:%{private}d, isFinalKeyDown:%{public}s,"
                    "finalKeyDownDuration:%{public}d, pid:%{public}d", subscriber->id_, option->GetFinalKey(),
                    option->IsFinalKeyDown() ? "true" : "false", option->GetFinalKeyDownDuration(), sess->GetPid());
                inner = subscribers.erase(inner);
            } else {
                ++inner;
            }
        }
        if (subscribers.empty()) {
            iter = keyGestures_.erase(iter);
        } else {
            ++iter;
        }
    }
}

bool KeySubscriberHandler::IsPreKeysMatch(const std::set<int32_t> &preKeys,
                                          const std::vector<int32_t> &pressedKeys) const
{
    if (preKeys.size() == 0) {
        MMI_HILOGD("The preKeys is empty");
        return true;
    }

    if (preKeys.size() != pressedKeys.size()) {
        MMI_HILOGE("The size of preKeys is not match");
        return false;
    }

    for (const auto &pressedKey : pressedKeys) {
        auto it = std::find(preKeys.begin(), preKeys.end(), pressedKey);
        if (it == preKeys.end()) {
            MMI_HILOGE("Cant't find the pressedKey");
            return false;
        }
    }

    return true;
}

bool KeySubscriberHandler::IsEqualPreKeys(const std::set<int32_t> &preKeys, const std::set<int32_t> &pressedKeys)
{
    if (preKeys.size() != pressedKeys.size()) {
        MMI_HILOGD("Pre key size not equal");
        return false;
    }

    for (const auto &pressedKey : pressedKeys) {
        auto it = std::find(preKeys.begin(), preKeys.end(), pressedKey);
        if (it == preKeys.end()) {
            return false;
        }
    }
    MMI_HILOGD("Equal prekeys");
    return true;
}

bool KeySubscriberHandler::IsMatchForegroundPid(std::list<std::shared_ptr<Subscriber>> subs,
    std::set<int32_t> foregroundPids)
{
    CALL_DEBUG_ENTER;
    isForegroundExits_ = false;
    foregroundPids_.clear();
    for (const auto &item : subs) {
        CHKPF(item);
        auto sess = item->sess_;
        CHKPF(sess);
        if (foregroundPids.find(sess->GetPid()) != foregroundPids.end()) {
            MMI_HILOGD("Subscriber foregroundPid:%{public}d", sess->GetPid());
            foregroundPids_.insert(sess->GetPid());
            isForegroundExits_ = true;
        }
    }
    MMI_HILOGD("The isForegroundExits_:%{public}d, foregroundPids:%{public}zu",
        isForegroundExits_, foregroundPids_.size());
    return isForegroundExits_;
}

void KeySubscriberHandler::NotifyKeyDownSubscriber(const std::shared_ptr<KeyEvent> &keyEvent,
    std::shared_ptr<KeyOption> keyOption, std::list<std::shared_ptr<Subscriber>> &subscribers, bool &handled)
{
    CALL_DEBUG_ENTER;
    CHKPV(keyEvent);
    CHKPV(keyOption);
    MMI_HILOGI("Notify key down subscribers size:%{public}zu", subscribers.size());
    if (keyOption->GetFinalKeyDownDuration() <= 0) {
        NotifyKeyDownRightNow(keyEvent, subscribers,  keyOption->IsRepeat(), handled);
    } else {
        NotifyKeyDownDelay(keyEvent, subscribers, handled);
    }
}

int32_t KeySubscriberHandler::GetHighestPrioritySubscriber(const std::list<std::shared_ptr<Subscriber>> &subscribers)
{
    int highestPriority = std::numeric_limits<int>::min();
    for (const auto &subscriber : subscribers) {
        CHKPC(subscriber);
        CHKPC(subscriber->keyOption_);
        int prio = subscriber->keyOption_->GetPriority();
        if (prio > highestPriority) {
            highestPriority = prio;
        }
    }
    return highestPriority;
}

void KeySubscriberHandler::NotifyKeyDownRightNow(const std::shared_ptr<KeyEvent> &keyEvent,
    std::list<std::shared_ptr<Subscriber>> &subscribers, bool isRepeat, bool &handled)
{
    CALL_DEBUG_ENTER;
    MMI_HILOGI("The subscribe list size is %{public}zu", subscribers.size());
    std::list<std::shared_ptr<Subscriber>> interestedSubscribers;
    for (auto &subscriber : subscribers) {
        CHKPC(subscriber);
        auto sess = subscriber->sess_;
        CHKPC(sess);
        MMI_HILOGI("Notify subscribe conditions, isForegroundExits:%{public}d, code()%{private}d, pid:%{public}d",
            isForegroundExits_, keyEvent->GetKeyCode(), sess->GetPid());
        if (!isForegroundExits_ || keyEvent->GetKeyCode() == KeyEvent::KEYCODE_POWER ||
            foregroundPids_.find(sess->GetPid()) != foregroundPids_.end()) {
            MMI_HILOGD("keyOption->GetFinalKeyDownDuration() <= 0");
            if (!isRepeat && keyEvent->GetKeyCode() == KeyRepeat->GetRepeatKeyCode()) {
                MMI_HILOGI("Subscribers do not need to repeat events");
                handled = true;
                continue;
            }
            interestedSubscribers.push_back(subscriber);
            handled = true;
        }
    }

    int32_t highestPriority = GetHighestPrioritySubscriber(interestedSubscribers);
    for (auto &subscriber : interestedSubscribers) {
        CHKPC(subscriber);
        CHKPC(subscriber->keyOption_);
        if (subscriber->keyOption_->GetPriority() == highestPriority) {
            NotifySubscriber(keyEvent, subscriber);
            MMI_HILOGD("Notified high priority subscriber");
        }
    }
}

void KeySubscriberHandler::NotifyKeyDownDelay(const std::shared_ptr<KeyEvent> &keyEvent,
    std::list<std::shared_ptr<Subscriber>> &subscribers, bool &handled)
{
    CALL_DEBUG_ENTER;
    MMI_HILOGD("The subscribe list size is %{public}zu", subscribers.size());
    std::list<std::shared_ptr<Subscriber>> interestedSubscribers;
    for (auto &subscriber : subscribers) {
        CHKPC(subscriber);
        auto sess = subscriber->sess_;
        CHKPC(sess);
        if (!isForegroundExits_ || keyEvent->GetKeyCode() == KeyEvent::KEYCODE_POWER ||
            foregroundPids_.find(sess->GetPid()) != foregroundPids_.end()) {
            interestedSubscribers.push_back(subscriber);
            handled = true;
        }
    }

    int32_t highestPriority = GetHighestPrioritySubscriber(interestedSubscribers);
    for (auto &subscriber : interestedSubscribers) {
        CHKPC(subscriber);
        CHKPC(subscriber->keyOption_);
        if (subscriber->keyOption_->GetPriority() == highestPriority) {
            MMI_HILOGD("Add timer");
            if (!AddTimer(subscriber, keyEvent)) {
                MMI_HILOGE("Add time failed, subscriberId:%{public}d", subscriber->id_);
            }
        }
    }
}

void KeySubscriberHandler::NotifyKeyUpSubscriber(const std::shared_ptr<KeyEvent> &keyEvent,
    std::list<std::shared_ptr<Subscriber>> subscribers, bool &handled)
{
    CALL_DEBUG_ENTER;
    MMI_HILOGI("Subscribers size:%{public}zu", subscribers.size());
    std::list<std::shared_ptr<Subscriber>> interestedSubscribers;
    for (auto &subscriber : subscribers) {
        CHKPC(subscriber);
        auto sess = subscriber->sess_;
        CHKPC(sess);
        MMI_HILOGI("Notify subscribe conditions, isForegroundExits:%{public}d, code()%{private}d, pid:%{public}d",
            isForegroundExits_, keyEvent->GetKeyCode(), sess->GetPid());
        if (!isForegroundExits_ || foregroundPids_.find(sess->GetPid()) != foregroundPids_.end()) {
            interestedSubscribers.push_back(subscriber);
            handled = true;
        }
    }

    int32_t highestPriority = GetHighestPrioritySubscriber(interestedSubscribers);
    for (auto &subscriber : interestedSubscribers) {
        CHKPC(subscriber);
        CHKPC(subscriber->keyOption_);
        if (subscriber->keyOption_->GetPriority() == highestPriority) {
            HandleKeyUpWithDelay(keyEvent, subscriber);
        }
    }
}

void KeySubscriberHandler::NotifySubscriber(std::shared_ptr<KeyEvent> keyEvent,
    const std::shared_ptr<Subscriber> &subscriber) __attribute__((no_sanitize("cfi")))
{
    CALL_INFO_TRACE;
    CHKPV(keyEvent);
    CHKPV(subscriber);
#ifdef SHORTCUT_KEY_RULES_ENABLED
    if (keyEvent->GetKeyCode() != KeyEvent::KEYCODE_POWER) {
        CHKPV(subscriber->keyOption_);
        KEY_SHORTCUT_MGR->MarkShortcutConsumed(*subscriber->keyOption_);
    }
#endif // SHORTCUT_KEY_RULES_ENABLED
    auto udsServerPtr = InputHandler->GetUDSServer();
    CHKPV(udsServerPtr);
    if (keyEvent->GetKeyCode() == KeyEvent::KEYCODE_POWER) {
        DfxHisysevent::ReportPowerInfo(keyEvent, OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC);
    }
    SubscriberNotifyNap(subscriber);
    NetPacket pkt(MmiMessageId::ON_SUBSCRIBE_KEY);
    InputEventDataTransformation::KeyEventToNetPacket(keyEvent, pkt);
    auto sess = subscriber->sess_;
    CHKPV(sess);
    int32_t fd = sess->GetFd();
    pkt << fd << subscriber->id_;
    if (!EventLogHelper::IsBetaVersion()) {
        MMI_HILOGI("Notify subscriber id:%{public}d, pid:%{public}d", subscriber->id_, sess->GetPid());
    } else {
        MMI_HILOGW("Notify subscriber id:%{public}d, code:%{private}d, pid:%{public}d",
            subscriber->id_, keyEvent->GetKeyCode(), sess->GetPid());
    }
    if (pkt.ChkRWError()) {
        MMI_HILOGE("Packet write dispatch subscriber failed");
        return;
    }
    if (!udsServerPtr->SendMsg(fd, pkt)) {
        MMI_HILOGE("Leave, server dispatch subscriber failed");
        return;
    }
}

bool KeySubscriberHandler::AddTimer(const std::shared_ptr<Subscriber> &subscriber,
                                    const std::shared_ptr<KeyEvent> &keyEvent)
{
    CALL_DEBUG_ENTER;
    CHKPF(keyEvent);
    CHKPF(subscriber);

    if (subscriber->timerId_ >= 0) {
        MMI_HILOGW("Leave, timer already added, it may have been added by injection");
        return true;
    }

    auto &keyOption = subscriber->keyOption_;
    CHKPF(keyOption);
    bool isKeyDown = keyOption->IsFinalKeyDown();
    int32_t duration = isKeyDown ? keyOption->GetFinalKeyDownDuration() : keyOption->GetFinalKeyUpDelay();
    if (duration <= 0) {
        MMI_HILOGE("Leave, duration <= 0");
        return true;
    }

    if (!CloneKeyEvent(keyEvent)) {
        MMI_HILOGE("Leave, cloneKeyEvent failed");
        return false;
    }

    std::weak_ptr<Subscriber> weakSubscriber = subscriber;
    subscriber->timerId_ = TimerMgr->AddTimer(duration, 1, [this, weakSubscriber] () {
        MMI_HILOGD("Timer callback");
        auto subscriber = weakSubscriber.lock();
        CHKPV(subscriber);
        subscriber->timerId_ = -1;
        OnTimer(subscriber);
    }, "KeySubscriberHandler");

    if (subscriber->timerId_ < 0) {
        MMI_HILOGE("Leave, addTimer failed");
        return false;
    }
    subscriber->keyEvent_ = keyEvent_;
    hasEventExecuting_ = true;
    MMI_HILOGD("Leave, add timer success, subscribeId:%{public}d,"
        "duration:%{public}d, timerId:%{public}d",
        subscriber->id_, duration, subscriber->timerId_);
    return true;
}

void KeySubscriberHandler::ClearSubscriberTimer(std::list<std::shared_ptr<Subscriber>> subscribers)
{
    CALL_DEBUG_ENTER;
    MMI_HILOGD("Clear subscriber timer size:%{public}zu", subscribers.size());
    for (auto &subscriber : subscribers) {
        CHKPC(subscriber);
        ClearTimer(subscriber);
    }
}

void KeySubscriberHandler::ClearTimer(const std::shared_ptr<Subscriber> &subscriber)
{
    CALL_DEBUG_ENTER;
    CHKPV(subscriber);

    if (subscriber->timerId_ < 0) {
        MMI_HILOGD("Leave, subscribeId:%{public}d, null timerId < 0", subscriber->id_);
        return;
    }

    TimerMgr->RemoveTimer(subscriber->timerId_);
    auto timerId = subscriber->timerId_;
    subscriber->keyEvent_.reset();
    subscriber->timerId_ = -1;
    hasEventExecuting_ = false;
    MMI_HILOGD("subscribeId:%{public}d, timerId:%{public}d", subscriber->id_, timerId);
}

void KeySubscriberHandler::OnTimer(const std::shared_ptr<Subscriber> subscriber)
{
    CALL_DEBUG_ENTER;
    CHKPV(subscriber);
    subscriber->timerId_ = -1;
    if (subscriber->keyEvent_ == nullptr) {
        MMI_HILOGE("Leave, subscriber->keyEvent is nullptr, subscribeId:%{public}d", subscriber->id_);
        return;
    }

    NotifySubscriber(subscriber->keyEvent_, subscriber);
    subscriber->keyEvent_.reset();
    MMI_HILOGD("subscribeId:%{public}d", subscriber->id_);
}

bool KeySubscriberHandler::InitSessionDeleteCallback()
{
    CALL_DEBUG_ENTER;
    if (callbackInitialized_) {
        MMI_HILOGD("Session delete callback has already been initialized");
        return true;
    }
    auto udsServerPtr = InputHandler->GetUDSServer();
    CHKPF(udsServerPtr);
    std::function<void(SessionPtr)> callback =
        [this] (SessionPtr sess) { return this->OnSessionDelete(sess); };
    udsServerPtr->AddSessionDeletedCallback(callback);
    callbackInitialized_ = true;
    return true;
}

bool KeySubscriberHandler::HandleKeyDown(const std::shared_ptr<KeyEvent> &keyEvent)
{
    CALL_DEBUG_ENTER;
    CHKPF(keyEvent);
#ifdef SHORTCUT_KEY_RULES_ENABLED
    KEY_SHORTCUT_MGR->ResetCheckState();
#endif // SHORTCUT_KEY_RULES_ENABLED
    bool handled = false;
    auto keyCode = keyEvent->GetKeyCode();
    std::vector<int32_t> pressedKeys = keyEvent->GetPressedKeys();
    RemoveKeyCode(keyCode, pressedKeys);
    std::set<int32_t> pids;
    GetForegroundPids(pids);
    MMI_HILOGI("Foreground pid size:%{public}zu", pids.size());
    std::lock_guard<std::mutex> lock(subscriberMapMutex_);
    for (auto &iter : subscriberMap_) {
        auto keyOption = iter.first;
        auto subscribers = iter.second;
        PrintKeyOption(keyOption);
        IsMatchForegroundPid(subscribers, pids);
        CHKPC(keyOption);
        if (!keyOption->IsFinalKeyDown()) {
            MMI_HILOGD("!keyOption->IsFinalKeyDown()");
            continue;
        }
        if (keyCode != keyOption->GetFinalKey()) {
            MMI_HILOGD("code != keyOption->GetFinalKey()");
            ClearSubscriberTimer(subscribers);
            continue;
        }
        if (!IsPreKeysMatch(keyOption->GetPreKeys(), pressedKeys)) {
            MMI_HILOGD("preKeysMatch failed");
            ClearSubscriberTimer(subscribers);
            continue;
        }
        NotifyKeyDownSubscriber(keyEvent, keyOption, subscribers, handled);
    }
    MMI_HILOGI("Handle key down:%{public}s", handled ? "true" : "false");
    return handled;
}

void KeySubscriberHandler::SubscriberNotifyNap(const std::shared_ptr<Subscriber> subscriber)
{
    CALL_DEBUG_ENTER;
    CHKPV(subscriber);
    int32_t state = NapProcess::GetInstance()->GetNapClientPid();
    if (state == REMOVE_OBSERVER || state == UNOBSERVED) {
        MMI_HILOGD("Nap client status:%{public}d", state);
        return;
    }

    auto sess = subscriber->sess_;
    CHKPV(sess);
    OHOS::MMI::NapProcess::NapStatusData napData;
    napData.pid = sess->GetPid();
    napData.uid = sess->GetUid();
    napData.bundleName = sess->GetProgramName();
    if (NapProcess::GetInstance()->IsNeedNotify(napData)) {
        int32_t syncState = ACTIVE_EVENT;
        NapProcess::GetInstance()->AddMmiSubscribedEventData(napData, syncState);
        NapProcess::GetInstance()->NotifyBundleName(napData, syncState);
    }
}

bool KeySubscriberHandler::HandleKeyUp(const std::shared_ptr<KeyEvent> &keyEvent)
{
#ifdef SHORTCUT_KEY_RULES_ENABLED
    if (KEY_SHORTCUT_MGR->HaveShortcutConsumed(keyEvent) || !KEY_SHORTCUT_MGR->IsCheckUpShortcut(keyEvent)) {
        MMI_HILOGI("Subscribe are not notify of key upevent!");
        return false;
    }
#endif // SHORTCUT_KEY_RULES_ENABLED
    bool handled = false;
    auto keyCode = keyEvent->GetKeyCode();
    std::vector<int32_t> pressedKeys = keyEvent->GetPressedKeys();
    RemoveKeyCode(keyCode, pressedKeys);
    std::set<int32_t> pids;
    GetForegroundPids(pids);
    std::lock_guard<std::mutex> lock(subscriberMapMutex_);
    for (auto &iter : subscriberMap_) {
        auto keyOption = iter.first;
        auto subscribers = iter.second;
        PrintKeyOption(keyOption);
        IsMatchForegroundPid(subscribers, pids);
        if (keyOption->IsFinalKeyDown()) {
            MMI_HILOGD("keyOption->IsFinalKeyDown()");
            ClearSubscriberTimer(subscribers);
            continue;
        }
        if (keyCode != keyOption->GetFinalKey()) {
            MMI_HILOGD("code != keyOption->GetFinalKey()");
            continue;
        }
        if (!IsPreKeysMatch(keyOption->GetPreKeys(), pressedKeys)) {
            MMI_HILOGD("PreKeysMatch failed");
            continue;
        }
        auto duration = keyOption->GetFinalKeyDownDuration();
        if (duration <= 0) {
            NotifyKeyUpSubscriber(keyEvent, subscribers, handled);
            continue;
        }
        std::optional<KeyEvent::KeyItem> keyItem = keyEvent->GetKeyItem();
        CHK_KEY_ITEM(keyItem);
        if (keyEvent->GetActionTime() - keyItem->GetDownTime() >= MS2US(duration)) {
            MMI_HILOGE("upTime - downTime >= duration");
            continue;
        }
        NotifyKeyUpSubscriber(keyEvent, subscribers, handled);
    }
    MMI_HILOGI("Handle key up:%{public}s", handled ? "true" : "false");
    return handled;
}

bool KeySubscriberHandler::HandleKeyCancel(const std::shared_ptr<KeyEvent> &keyEvent)
{
    CALL_DEBUG_ENTER;
    CHKPF(keyEvent);
    std::lock_guard<std::mutex> lock(subscriberMapMutex_);
    for (auto &iter : subscriberMap_) {
        auto keyOption = iter.first;
        auto subscribers = iter.second;
        for (auto &subscriber : subscribers) {
            PrintKeyUpLog(subscriber);
            ClearTimer(subscriber);
        }
    }
    return false;
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
bool KeySubscriberHandler::IsKeyEventSubscribed(int32_t keyCode, int32_t trrigerType)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> lock(subscriberMapMutex_);
    for (const auto &iter : subscriberMap_) {
        auto keyOption = iter.first;
        auto subscribers = iter.second;
        CHKPC(keyOption);
        MMI_HILOGD("keyOption->finalKey:%{private}d, keyOption->isFinalKeyDown:%{public}s, "
            "keyOption->finalKeyDownDuration:%{public}d",
            keyOption->GetFinalKey(), keyOption->IsFinalKeyDown() ? "true" : "false",
            keyOption->GetFinalKeyDownDuration());
        int32_t keyAction = KeyEvent::KEY_ACTION_UP;
        if (keyOption->IsFinalKeyDown()) {
            MMI_HILOGD("keyOption is final key down");
            keyAction = KeyEvent::KEY_ACTION_DOWN;
        }
        if (keyCode == keyOption->GetFinalKey() && trrigerType == keyAction && subscribers.size() > 0) {
            MMI_HILOGD("Current key event is subscribed");
            return true;
        }
    }
    return false;
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

bool KeySubscriberHandler::CloneKeyEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPF(keyEvent);
    if (keyEvent_ == nullptr) {
        MMI_HILOGW("keyEvent_ is nullptr");
        keyEvent_ = KeyEvent::Clone(keyEvent);
    }
    CHKPF(keyEvent_);
    return true;
}

void KeySubscriberHandler::RemoveKeyCode(int32_t keyCode, std::vector<int32_t> &keyCodes)
{
    for (auto it = keyCodes.begin(); it != keyCodes.end(); ++it) {
        if (*it == keyCode) {
            keyCodes.erase(it);
            return;
        }
    }
}

bool KeySubscriberHandler::IsRepeatedKeyEvent(std::shared_ptr<KeyEvent> keyEvent)
{
    CHKPF(keyEvent);
    if (!hasEventExecuting_) {
        return false;
    }

    CHKPF(keyEvent_);
    if (keyEvent->GetKeyCode() != keyEvent_->GetKeyCode()) {
        return false;
    }

    if (keyEvent->GetKeyAction() != keyEvent_->GetKeyAction()) {
        return false;
    }

    if (keyEvent->GetKeyItems().size() != keyEvent_->GetKeyItems().size()) {
        return false;
    }

    for (const auto &item : keyEvent->GetKeyItems()) {
        int32_t keyCode = item.GetKeyCode();
        bool findResult = false;
        for (const auto &item1 : keyEvent_->GetKeyItems()) {
            if (keyCode == item1.GetKeyCode()) {
                findResult = true;
                break;
            }
        }
        if (!findResult) {
            return false;
        }
    }
    return true;
}

void KeySubscriberHandler::RemoveSubscriberKeyUpTimer(int32_t keyCode)
{
    {
        std::lock_guard<std::mutex> lock(subscriberMapMutex_);
        for (auto iter = subscriberMap_.begin(); iter != subscriberMap_.end(); iter++) {
            auto &subscribers = iter->second;
            for (auto it = subscribers.begin(); it != subscribers.end(); it++) {
                if (((*it)->timerId_ >= 0) && ((*it)->keyOption_->GetFinalKey() == keyCode)) {
                    ClearTimer(*it);
                }
            }
        }
    }
    keyGestureMgr_.ResetAll();
}

void KeySubscriberHandler::HandleKeyUpWithDelay(std::shared_ptr<KeyEvent> keyEvent,
    const std::shared_ptr<Subscriber> &subscriber)
{
    CHKPV(subscriber);
    CHKPV(subscriber->keyOption_);
    auto keyUpDelay = subscriber->keyOption_->GetFinalKeyUpDelay();
    if (keyUpDelay <= 0) {
        NotifySubscriber(keyEvent, subscriber);
    } else {
        if (!AddTimer(subscriber, keyEvent)) {
            MMI_HILOGE("Leave, add timer failed");
        }
    }
}

void KeySubscriberHandler::PrintKeyOption(const std::shared_ptr<KeyOption> keyOption)
{
    CHKPV(keyOption);
    MMI_HILOGD("keyOption->finalKey:%{private}d,keyOption->isFinalKeyDown:%{public}s, "
        "keyOption->finalKeyDownDuration:%{public}d, keyOption->isRepeat:%{public}s",
        keyOption->GetFinalKey(), keyOption->IsFinalKeyDown() ? "true" : "false",
        keyOption->GetFinalKeyDownDuration(), keyOption->IsRepeat() ? "true" : "false");
    for (const auto &keyCode : keyOption->GetPreKeys()) {
        MMI_HILOGD("keyOption->prekey:%{private}d", keyCode);
    }
}

void KeySubscriberHandler::PrintKeyUpLog(const std::shared_ptr<Subscriber> &subscriber)
{
    CHKPV(subscriber);
    auto &keyOption = subscriber->keyOption_;
    CHKPV(keyOption);
    MMI_HILOGD("subscribeId:%{public}d, keyOption->finalKey:%{private}d,"
        "keyOption->isFinalKeyDown:%{public}s, keyOption->finalKeyDownDuration:%{public}d,"
        "keyOption->finalKeyUpDelay:%{public}d",
        subscriber->id_, keyOption->GetFinalKey(), keyOption->IsFinalKeyDown() ? "true" : "false",
        keyOption->GetFinalKeyDownDuration(), keyOption->GetFinalKeyUpDelay());
    for (const auto &keyCode : keyOption->GetPreKeys()) {
        MMI_HILOGD("keyOption->prekey:%{private}d", keyCode);
    }
}

void KeySubscriberHandler::Dump(int32_t fd, const std::vector<std::string> &args)
{
    CALL_DEBUG_ENTER;
    mprintf(fd, "Subscriber information:\t");
    mprintf(fd, "subscribers: count = %zu", CountSubscribers());
    for (const auto &item : foregroundPids_) {
        mprintf(fd, "Foreground Pids: %d", item);
    }
    mprintf(fd, "enableCombineKey: %s | isForegroundExits: %s | needSkipPowerKeyUp: %s \t",
            enableCombineKey_ ? "true" : "false", isForegroundExits_ ? "true" : "false",
            needSkipPowerKeyUp_ ? "true" : "false");
    {
        std::lock_guard<std::mutex> lock(subscriberMapMutex_);
        DumpSubscribers(fd, subscriberMap_);
    }
    DumpSubscribers(fd, keyGestures_);
}

size_t KeySubscriberHandler::CountSubscribers() const
{
    size_t total { 0 };

    for (auto &item : subscriberMap_) {
        total += item.second.size();
    }
    for (auto &item : keyGestures_) {
        total += item.second.size();
    }
    return total;
}

void KeySubscriberHandler::DumpSubscribers(int32_t fd, const SubscriberCollection &collection) const
{
    for (auto iter = collection.begin(); iter != collection.end(); ++iter) {
        auto &subscribers = iter->second;
        for (auto item = subscribers.begin(); item != subscribers.end(); ++item) {
            DumpSubscriber(fd, *item);
        }
    }
}

void KeySubscriberHandler::DumpSubscriber(int32_t fd, std::shared_ptr<Subscriber> subscriber) const
{
    CHKPV(subscriber);
    SessionPtr session = subscriber->sess_;
    CHKPV(session);
    std::shared_ptr<KeyOption> keyOption = subscriber->keyOption_;
    CHKPV(keyOption);
    std::set<int32_t> preKeys = keyOption->GetPreKeys();
    std::ostringstream sPrekeys;
    if (auto keyIter = preKeys.cbegin(); keyIter != preKeys.cend()) {
        sPrekeys << *keyIter;
        for (++keyIter; keyIter != preKeys.cend(); ++keyIter) {
            sPrekeys << "," << *keyIter;
        }
    }
    mprintf(fd,
            "Subscriber ID:%d | Pid:%d | Uid:%d | Fd:%d | Prekeys:[%s] | FinalKey:%d | "
            "FinalKeyDownDuration:%d | IsFinalKeyDown:%s | IsRepeat:%s | ProgramName:%s",
            subscriber->id_, session->GetPid(), session->GetUid(), session->GetFd(),
            sPrekeys.str().c_str(), keyOption->GetFinalKey(), keyOption->GetFinalKeyDownDuration(),
            keyOption->IsFinalKeyDown() ? "true" : "false", keyOption->IsRepeat() ? "true" : "false",
            session->GetProgramName().c_str());
}

void KeySubscriberHandler::RemoveSubscriberTimer(std::shared_ptr<KeyEvent> keyEvent)
{
    CALL_INFO_TRACE;
    CHKPV(keyEvent);
    auto keyCode = keyEvent->GetKeyCode();
    std::vector<int32_t> pressedKeys = keyEvent->GetPressedKeys();
    RemoveKeyCode(keyCode, pressedKeys);
    std::set<int32_t> pids;
    GetForegroundPids(pids);
    MMI_HILOGI("Foreground pid size:%{public}zu", pids.size());
    std::lock_guard<std::mutex> lock(subscriberMapMutex_);
    for (auto &iter : subscriberMap_) {
        auto keyOption = iter.first;
        auto subscribers = iter.second;
        PrintKeyOption(keyOption);
        IsMatchForegroundPid(subscribers, pids);
        ClearSubscriberTimer(subscribers);
    }
}

#ifdef OHOS_BUILD_ENABLE_CALL_MANAGER
bool KeySubscriberHandler::HandleCallEnded(std::shared_ptr<KeyEvent> keyEvent)
{
    CALL_DEBUG_ENTER;
    CHKPF(keyEvent);
    int32_t callType = DEVICE_MONITOR->GetCallType();
    if (callType == -1) {
        MMI_HILOGE("callType is false");
        return false;
    }
    if (!callBahaviorState_ && callType == GetCallState) {
        MMI_HILOGD("CallBehaviorState is false");
        return false;
    }
    if (callEndKeyUp_ && keyEvent->GetKeyCode() == KeyEvent::KEYCODE_POWER &&
        keyEvent->GetKeyAction() == KeyEvent::KEY_ACTION_UP) {
        callEndKeyUp_ = false;
        return true;
    }
    if (keyEvent->GetKeyCode() != KeyEvent::KEYCODE_POWER ||
        keyEvent->GetKeyAction() != KeyEvent::KEY_ACTION_DOWN) {
        MMI_HILOGE("This key event no need to CallEnded");
        return false;
    }
    if (DISPLAY_MONITOR->GetScreenStatus() != EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON) {
        MMI_HILOGI("The current screen is not on, so not allow end call");
        return false;
    }
    int32_t ret = -1;
    if (callType == VOIP_CALL) {
        ret = DEVICE_MONITOR->GetVoipCallState();
    } else {
        ret = DEVICE_MONITOR->GetCallState();
    }
    MMI_HILOGI("HandleCallEnded, The callState:%{public}d", ret);

    switch (ret) {
        case StateType::CALL_STATUS_HOLDING:
        case StateType::CALL_STATUS_ALERTING:
        case StateType::CALL_STATUS_ANSWERED:
        case StateType::CALL_STATUS_ACTIVE:
        case StateType::CALL_STATUS_DIALING: {
            HangUpCallProcess();
            needSkipPowerKeyUp_ = true;
            callEndKeyUp_ = true;
            return true;
        }
        case StateType::CALL_STATUS_WAITING:
        case StateType::CALL_STATUS_INCOMING: {
            RejectCallProcess();
            needSkipPowerKeyUp_ = true;
            callEndKeyUp_ = true;
            return true;
        }
        case StateType::CALL_STATUS_IDLE:
        case StateType::CALL_STATUS_DISCONNECTING:
        case StateType::CALL_STATUS_DISCONNECTED:
        default: {
            MMI_HILOGE("This state no need to CallEnded");
            return false;
        }
    }
}

void KeySubscriberHandler::HangUpCallProcess()
{
    if (callManagerClientPtr == nullptr) {
        callManagerClientPtr = DelayedSingleton<OHOS::Telephony::CallManagerClient>::GetInstance();
        if (callManagerClientPtr == nullptr) {
            MMI_HILOGE("CallManager init fail");
            return;
        }
        callManagerClientPtr->Init(OHOS::TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID);
    }
    int32_t ret = -1;
    auto begin = std::chrono::high_resolution_clock::now();
    ret = callManagerClientPtr->HangUpCall(0);
    auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::high_resolution_clock::now() - begin).count();
#ifdef OHOS_BUILD_ENABLE_DFX_RADAR
    DfxHisysevent::ReportApiCallTimes(ApiDurationStatistics::Api::TELEPHONY_CALL_MGR_HANG_UP_CALL, durationMS);
#endif // OHOS_BUILD_ENABLE_DFX_RADAR
    if (ret != ERR_OK) {
        MMI_HILOGE("HangUpCall fail, ret:%{public}d", ret);
        return;
    }
    MMI_HILOGI("HangUpCall success");
}

void KeySubscriberHandler::RejectCallProcess()
{
    if (callManagerClientPtr == nullptr) {
        callManagerClientPtr = DelayedSingleton<OHOS::Telephony::CallManagerClient>::GetInstance();
        if (callManagerClientPtr == nullptr) {
            MMI_HILOGE("CallManagerClient GetInstance fail");
            return;
        }
        callManagerClientPtr->Init(OHOS::TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID);
    }
    int32_t ret = -1;
    auto begin = std::chrono::high_resolution_clock::now();
    ret = callManagerClientPtr->RejectCall(0, false, u"");
    auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::high_resolution_clock::now() - begin).count();
#ifdef OHOS_BUILD_ENABLE_DFX_RADAR
    DfxHisysevent::ReportApiCallTimes(ApiDurationStatistics::Api::TELEPHONY_CALL_MGR_REJECT_CALL, durationMS);
#endif // OHOS_BUILD_ENABLE_DFX_RADAR
    if (ret != ERR_OK) {
        MMI_HILOGE("RejectCall fail, ret:%{public}d", ret);
        return;
    }
    MMI_HILOGI("RejectCall success");
}
#endif // OHOS_BUILD_ENABLE_CALL_MANAGER

void KeySubscriberHandler::InitDataShareListener()
{
    SettingObserver::UpdateFunc func = [&](const std::string& key) {
        bool statusValue = false;
        int32_t ret = -1;
        ret = SettingDataShare::GetInstance(MULTIMODAL_INPUT_SERVICE_ID).GetBoolValue(key, statusValue,
            SETTINGS_DATA_SYSTEM_URI);
        if (ret != RET_OK) {
            MMI_HILOGE("Get incall_power_button_behavior state fail:%{public}d", ret);
            return;
        }
        MMI_HILOGE("Get incall_power_button_behavior state success:%{public}d", statusValue);
        callBahaviorState_ = statusValue;
    };

    func(CALL_BEHAVIOR_KEY);

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHKPV(saManager);
    auto remoteObj = saManager->GetSystemAbility(MULTIMODAL_INPUT_SERVICE_ID);
    CHKPV(remoteObj);

    std::pair<int, std::shared_ptr<DataShare::DataShareHelper>> ret =
        DataShare::DataShareHelper::Create(remoteObj, SETTINGS_DATA_SYSTEM_URI, SETTINGS_DATA_EXT_URI);
    MMI_HILOGE("Create data_share helper, ret=%{public}d", ret.first);

    if (ret.first == ERR_OK) {
        std::shared_ptr<DataShare::DataShareHelper> helper = ret.second;
        auto uri = Uri(SETTINGS_DATA_SYSTEM_URI + "&key=" + CALL_BEHAVIOR_KEY);
        sptr<SettingObserver> statusObserver = SettingDataShare::GetInstance(MULTIMODAL_INPUT_SERVICE_ID).
            CreateObserver(CALL_BEHAVIOR_KEY, func);
        CHKPV(statusObserver);
        helper->RegisterObserver(uri, statusObserver);
    }
}
} // namespace MMI
} // namespace OHOS
