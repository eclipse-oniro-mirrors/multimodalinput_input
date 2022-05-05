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

#include "get_device_node.h"

using namespace OHOS::MMI;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "GetDeviceNode" };
} // namespace

GetDeviceNode::GetDeviceNode()
{
    InitDeviceInfo();
}

int32_t GetDeviceNode::GetDeviceNodeName(const std::string &targetName, uint16_t devIndex, std::string &deviceNode)
{
    std::vector<std::string> fileStrs = ReadDeviceFile();
    if (fileStrs.size() == 0) {
        MMI_HILOGE("fileStrs is faild");
        return RET_ERR;
    }
    DeviceList deviceList;
    AnalyseDevices(fileStrs, deviceList);
    if (deviceList.empty()) {
        MMI_HILOGE("device list is null");
        return RET_ERR;
    }
    std::string deviceName = deviceList_[targetName];
    auto iter = deviceList.find(deviceName);
    if (iter == deviceList.end()) {
        MMI_HILOGE("faild for find deviceName:%{public}s", deviceName.c_str());
        return RET_ERR;
    }
    size_t targetSize = iter->second.size();
    if (devIndex > targetSize) {
        MMI_HILOGE("faild for devIndex:%{public}d > targetSize:%{public}zu", devIndex, targetSize);
        return RET_ERR;
    }
    std::string nodeRootPath = "/dev/input/";
    deviceNode = nodeRootPath + iter->second[devIndex];
    MMI_HILOGI("%{public}s[%{public}d] --> %{public}s", targetName.c_str(), devIndex,
               deviceNode.c_str());

    return RET_OK;
}

void GetDeviceNode::InitDeviceInfo()
{
    deviceList_["mouse"] = "Virtual Mouse";
    deviceList_["touch"] = "Virtual TouchScreen";
    deviceList_["finger"] = "Virtual Finger";
    deviceList_["pad"] = "Virtual Touchpad";
    deviceList_["pen"] = "Virtual Stylus";
    deviceList_["gamePad"] = "Virtual GamePad";
    deviceList_["joystick"] = "Virtual Joystick";
    deviceList_["remoteControl"] = "Virtual RemoteControl";
    deviceList_["knob model1"] = "Virtual KnobConsumerCtrl";
    deviceList_["knob model2"] = "Virtual Knob";
    deviceList_["knob model3"] = "Virtual KnobMouse";
    deviceList_["keyboard model1"] = "Virtual keyboard";
    deviceList_["keyboard model2"] = "Virtual KeyboardConsumerCtrl";
    deviceList_["keyboard model3"] = "Virtual KeyboardSysCtrl";
    deviceList_["trackball"] = "Virtual Trackball";
    deviceList_["trackpad model1"] = "Virtual TrackPadMouse";
    deviceList_["trackpad model2"] = "Virtual Trackpad";
}

std::vector<std::string> GetDeviceNode::ReadDeviceFile()
{
    std::string openPath = "/proc/bus/input/devices";
    char realPath[PATH_MAX] = {};
    if (realpath(openPath.c_str(), realPath) == nullptr) {
        MMI_HILOGE("path is error, path:%{public}s", openPath.c_str());
        return {};
    }
    FILE* fp = fopen(openPath.c_str(), "r");
    if (fp == nullptr) {
        MMI_HILOGW("open file: %{pulic}s failed", openPath.c_str());
        return {};
    }
    char buf[READ_CMD_BUFF_SIZE] = {};
    std::vector<std::string> deviceStrs;
    while (fgets(buf, sizeof(buf), fp) != nullptr) {
        deviceStrs.push_back(buf);
    }
    if (fclose(fp) != 0) {
        MMI_HILOGW("close file: %{pulic}s failed", openPath.c_str());
    }
    return deviceStrs;
}

void GetDeviceNode::AnalyseDevices(const std::vector<std::string>& deviceStrs, DeviceList& deviceList) const
{
    std::string name;
    for (const auto &item : deviceStrs) {
        if (item.empty()) {
            MMI_HILOGE("device info is none");
            return;
        }
        std::string temp = item.substr(0, 1);
        uint64_t endPos = 0;
        uint64_t startPos = 0;
        if (temp == "N") {
            startPos = item.find("=") + strlen("N:");
            endPos = item.size() - 1;
            name = item.substr(startPos, endPos - startPos - 1);
        }
        if (temp == "H") {
            startPos = item.rfind("event");
            uint64_t eventLength = item.substr(startPos).find_first_of(" ");
            std::string target = item.substr(startPos, eventLength);
            if (!(name.empty())) {
                deviceList[name].push_back(target);
                name.clear();
                target.clear();
            }
        }
    }
}