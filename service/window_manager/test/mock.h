/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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
#ifndef MESSAGE_PARCEL_MOCK_H
#define MESSAGE_PARCEL_MOCK_H

#include <memory>
#include <string>
#include <gmock/gmock.h>

#include "dfx_hisysevent.h"
#include "event_filter_handler.h"
#include "fingersense_wrapper.h"
#include "input_device_manager.h"
#include "input_display_bind_helper.h"
#include "input_event_handler.h"
#include "knuckle_drawing_manager.h"
#include "knuckle_dynamic_drawing_manager.h"
#include "libinput_interface.h"
#include "mouse_event_normalize.h"
#include "multimodal_input_preferences_manager.h"
#include "pointer_drawing_manager.h"
#include "scene_board_judgement.h"
#include "setting_datashare.h"
#include "timer_manager.h"
#include "touch_drawing_manager.h"
#include "uds_server.h"

namespace OHOS {
namespace MMI {
class DfsMessageParcel {
public:
    virtual ~DfsMessageParcel() = default;
public:
    virtual SessionPtr GetSession(int32_t fd) = 0;
    virtual int32_t GetClientFd(int32_t pid) = 0;
    virtual bool HasPointerDevice() = 0;
    virtual std::shared_ptr<InputDevice> GetInputDevice(int32_t deviceId, bool checked) = 0;
    virtual bool GetMouseDisplayState() = 0;
    virtual bool GetBoolValue(const std::string &key, bool defaultValue) = 0;
    virtual bool SendMsg(NetPacket &pkt) = 0;
    virtual bool IsSceneBoardEnabled() = 0;
    virtual bool IsWindowRotation() = 0;
    virtual int32_t GetDisplayId() = 0;
    virtual std::string GetBindDisplayNameByInputDevice(int32_t inputDeviceId) = 0;
    virtual std::optional<WindowInfo> SelectWindowInfo(int32_t logicalX, int32_t logicalY,
        const std::shared_ptr<PointerEvent>& pointerEvent) = 0;
    virtual bool UpdateDisplayId(int32_t& displayId) = 0;
    virtual bool GetHardCursorEnabled() = 0;
    virtual std::vector<int32_t> HandleHardwareCursor(const DisplayInfo *physicalDisplayInfo,
        int32_t physicalX, int32_t physicalY) = 0;
public:
    static inline std::shared_ptr<DfsMessageParcel> messageParcel = nullptr;
};

class MessageParcelMock : public DfsMessageParcel {
public:
    MOCK_METHOD1(GetSession, SessionPtr(int32_t fd));
    MOCK_METHOD1(GetClientFd, int32_t(int32_t pid));
    MOCK_METHOD1(GetBindDisplayNameByInputDevice, std::string(int32_t inputDeviceId));
    MOCK_METHOD0(HasPointerDevice, bool());
    MOCK_METHOD2(GetInputDevice, std::shared_ptr<InputDevice>(int32_t deviceId, bool checked));
    MOCK_METHOD0(GetMouseDisplayState, bool());
    MOCK_METHOD2(GetBoolValue, bool(const std::string &key, bool defaultValue));
    MOCK_METHOD1(SendMsg, bool(NetPacket &pkt));
    MOCK_METHOD0(IsSceneBoardEnabled, bool());
    MOCK_METHOD0(IsWindowRotation, bool());
    MOCK_METHOD0(GetDisplayId, int32_t());
    MOCK_METHOD3(SelectWindowInfo, std::optional<WindowInfo>(int32_t logicalX, int32_t logicalY,
        const std::shared_ptr<PointerEvent>& pointerEvent));
    MOCK_METHOD1(UpdateDisplayId, bool(int32_t& displayId));
    MOCK_METHOD0(GetHardCursorEnabled, bool());
    MOCK_METHOD3(HandleHardwareCursor, std::vector<int32_t>(const DisplayInfo *physicalDisplayInfo,
        int32_t physicalX, int32_t physicalY));
};
} // namespace MMI
} // namespace OHOS
#endif