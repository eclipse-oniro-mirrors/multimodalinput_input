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

#ifndef KEY_EVENT_NORMALIZE_H
#define KEY_EVENT_NORMALIZE_H

#include "singleton.h"

#include "i_input_windows_manager.h"
#include "key_event.h"
#include "util.h"

namespace OHOS {
namespace MMI {
class KeyEventNormalize final {
    enum class VolumeSwapConfig {
        NO_CONFIG,
        NO_VOLUME_SWAP,
        SWAP_ON_FOLD,
    };

    struct InputProductConfig {
        VolumeSwapConfig volumeSwap_ { VolumeSwapConfig::NO_CONFIG };
    };
    DECLARE_DELAYED_SINGLETON(KeyEventNormalize);

public:
    DISALLOW_COPY_AND_MOVE(KeyEventNormalize);
    std::shared_ptr<KeyEvent> GetKeyEvent();
    void Init();
    int32_t Normalize(libinput_event *event, std::shared_ptr<KeyEvent> keyEvent);
    void SyncLedStateFromKeyEvent(struct libinput_device* device);
    void ResetKeyEvent(struct libinput_device* device);
    int32_t SetShieldStatus(int32_t shieldMode, bool isShield);
    int32_t GetShieldStatus(int32_t shieldMode, bool &isShield);
    void SetCurrentShieldMode(int32_t shieldMode);
    int32_t GetCurrentShieldMode();
    bool IsScreenFold();
    void ModifierkeyEventNormalize(const std::shared_ptr<KeyEvent>& keyEvent);
    void SetKeyStatusRecord(bool enable, int32_t timeout);

private:
    void ReadProductConfig(InputProductConfig &config) const;
    void ReadProductConfig(const std::string &cfgPath, InputProductConfig &config) const;
    void CheckProductParam(InputProductConfig &productConfig) const;
    int32_t TransformVolumeKey(struct libinput_device *dev, int32_t keyCode, int32_t keyAction) const;
    void HandleKeyAction(struct libinput_device* device, KeyEvent::KeyItem &item, std::shared_ptr<KeyEvent> keyEvent);
    bool HandleModifierKeyAction(const std::shared_ptr<KeyEvent>& keyEvent);
    bool HandleModifierKeyDown(const std::shared_ptr<KeyEvent>& keyEvent);
    bool HandleModifierKeyUp(const std::shared_ptr<KeyEvent>& keyEvent);
    void SyncSwitchFunctionKeyState(const std::shared_ptr<KeyEvent>& keyEvent, int32_t funckey);
    int32_t KeyEventAutoUp(const std::shared_ptr<KeyEvent>& keyEvent, int32_t timeout);

private:
    std::shared_ptr<KeyEvent> keyEvent_ { nullptr };
    std::map<int32_t, bool> shieldStatus_ {
        {SHIELD_MODE::FACTORY_MODE, false},
        {SHIELD_MODE::OOBE_MODE, false},
    };
    int32_t lastShieldMode_ { -1 };
    std::mutex mtx_;
    bool keyStatusRecordSwitch_ { false };
    int32_t keyStatusRecordTimeout_ { 10000 };
};
#define KeyEventHdr ::OHOS::DelayedSingleton<KeyEventNormalize>::GetInstance()
} // namespace MMI
} // namespace OHOS
#endif // KEY_EVENT_NORMALIZE_H