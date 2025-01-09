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

#ifndef INPUT_DEVICE_MANAGER_H
#define INPUT_DEVICE_MANAGER_H

#include <list>
#include <string>

#include "device_config_file_parser.h"
#include "device_observer.h"
#include "input_device.h"
#include "msg_handler.h"
#include "nocopyable.h"
#include "singleton.h"
#include "uds_session.h"
#include "util.h"

namespace OHOS {
namespace MMI {
class InputDeviceManager final : public IDeviceObject {
private:
    struct InputDeviceInfo {
        struct libinput_device *inputDeviceOrigin { nullptr };
        std::string networkIdOrigin;
        bool isRemote { false };
        bool isPointerDevice { false };
        bool isTouchableDevice { false };
        bool enable { false };
        std::string dhid;
        std::string sysUid;
        VendorConfig vendorConfig;
    };

public:
    InputDeviceManager() = default;
    ~InputDeviceManager() = default;
    DISALLOW_COPY_AND_MOVE(InputDeviceManager);

    void OnInputDeviceAdded(struct libinput_device *inputDevice);
    void OnInputDeviceRemoved(struct libinput_device *inputDevice);
    int32_t AddVirtualInputDevice(std::shared_ptr<InputDevice> device, int32_t &deviceId);
    int32_t RemoveVirtualInputDevice(int32_t deviceId);
    std::vector<int32_t> GetInputDeviceIds() const;
    std::shared_ptr<InputDevice> GetInputDevice(int32_t deviceId, bool checked = true) const;
    int32_t SupportKeys(int32_t deviceId, std::vector<int32_t> &keyCodes, std::vector<bool> &keystroke);
    int32_t FindInputDeviceId(struct libinput_device* inputDevice);
    int32_t GetKeyboardBusMode(int32_t deviceId);
    bool GetDeviceConfig(int32_t deviceId, int32_t &KeyboardType);
    int32_t GetDeviceSupportKey(int32_t deviceId, int32_t &keyboardType);
    int32_t GetKeyboardType(int32_t deviceId, int32_t &keyboardType);
    void Attach(std::shared_ptr<IDeviceObserver> observer);
    void Detach(std::shared_ptr<IDeviceObserver> observer);
    void NotifyPointerDevice(bool hasPointerDevice, bool isVisible, bool isHotPlug);
    void AddDevListener(SessionPtr sess);
    void RemoveDevListener(SessionPtr sess);
    void Dump(int32_t fd, const std::vector<std::string> &args);
    void DumpDeviceList(int32_t fd, const std::vector<std::string> &args);
    bool IsRemote(struct libinput_device *inputDevice) const;
    bool IsRemote(int32_t id) const;
    bool IsKeyboardDevice(struct libinput_device* device) const;
    bool IsPointerDevice(struct libinput_device* device) const;
    bool IsTouchDevice(struct libinput_device* device) const;
    struct libinput_device* GetKeyboardDevice() const;
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING
    bool HasPointerDevice();
    bool HasVirtualPointerDevice();
#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING
    bool HasTouchDevice();
    const std::string& GetScreenId(int32_t deviceId) const;
    using inputDeviceCallback = std::function<void(int32_t deviceId, std::string devName, std::string devStatus)>;
    void SetInputStatusChangeCallback(inputDeviceCallback callback);
    VendorConfig GetVendorConfig(int32_t deviceId) const;
    int32_t OnEnableInputDevice(bool enable);
    std::vector<int32_t> GetTouchPadIds();
    struct libinput_device *GetTouchPadDeviceOrigin();

    static std::shared_ptr<InputDeviceManager> GetInstance();

private:
    int32_t ParseDeviceId(struct libinput_device *inputDevice);
    void MakeDeviceInfo(struct libinput_device *inputDevice, struct InputDeviceInfo& info);
    bool IsMatchKeys(struct libinput_device* device, const std::vector<int32_t> &keyCodes) const;
    void ScanPointerDevice();
    void FillInputDevice(std::shared_ptr<InputDevice> inputDevice, libinput_device *deviceOrigin) const;
    std::string GetInputIdentification(struct libinput_device* inputDevice);
    void NotifyDevCallback(int32_t deviceId,  struct InputDeviceInfo inDevice);
    void NotifyDevRemoveCallback(int32_t deviceId,  const InputDeviceInfo &deviceInfo);
    int32_t NotifyMessage(SessionPtr sess, int32_t id, const std::string &type);
    void InitSessionLostCallback();
    void OnSessionLost(SessionPtr session);
    int32_t MakeVirtualDeviceInfo(std::shared_ptr<InputDevice> device, InputDeviceInfo &deviceInfo);
    int32_t GenerateVirtualDeviceId(int32_t &deviceId);
    bool IsPointerDevice(std::shared_ptr<InputDevice> inputDevice) const;
    bool IsTouchableDevice(std::shared_ptr<InputDevice> inputDevice) const;
    bool IsKeyboardDevice(std::shared_ptr<InputDevice> inputDevice) const;
    std::string GetMaskedDeviceId(const std::string& str);
    std::string GetMaskedStr(const std::string& str, size_t reserveLen);

private:
    std::map<int32_t, struct InputDeviceInfo> inputDevice_;
    std::map<int32_t, std::shared_ptr<InputDevice>> virtualInputDevices_;
    std::map<std::string, std::string> inputDeviceScreens_;
    std::list<std::shared_ptr<IDeviceObserver>> observers_;
    std::list<SessionPtr> devListeners_;
    inputDeviceCallback devCallbacks_ { nullptr };
    std::map<int32_t, std::string> displayInputBindInfos_;
    DeviceConfigManagement configManagement_;
    bool sessionLostCallbackInitialized_ { false };

    static std::shared_ptr<InputDeviceManager> instance_;
    static std::mutex mutex_;
};

#define INPUT_DEV_MGR ::OHOS::MMI::InputDeviceManager::GetInstance()
} // namespace MMI
} // namespace OHOS
#endif // INPUT_DEVICE_MANAGER_H
