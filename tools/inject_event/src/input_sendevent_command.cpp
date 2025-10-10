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

#include "input_sendevent_command.h"

#include <cctype>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <regex>
#include <string>
#include <typeinfo>
#include <unistd.h>
#include <linux/input.h>

#include "error_multimodal.h"
#include "proto.h"
#include "util.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "InputSendeventCommand"

namespace OHOS {
namespace MMI {
namespace {
constexpr uint32_t SEND_EVENT_ARGC { 6 };
constexpr uint32_t SEND_EVENT_ARGV_COUNTS { 5 };
constexpr uint32_t SEND_EVENT_DEV_NODE_INDEX { 1 };
constexpr uint32_t SEND_EVENT_TYPE_INDEX { 2 };
constexpr uint32_t SEND_EVENT_CODE_INDEX { 3 };
constexpr uint32_t SEND_EVENT_VALUE_INDEX { 4 };
constexpr uint32_t INPUT_TYPE_LENGTH { 5 };
constexpr uint32_t INPUT_CODE_LENGTH { 5 };
constexpr uint32_t INPUT_VALUE_LENGTH { 11 };
} // namespace

int32_t InputSendeventCommand::HandleSendEventCommand(int32_t argc, char** argv)
{
    InputSendeventCommand command;
    if (!command.SendEventOption(argc, argv)) {
        std::cerr << "please use 'uinput sendevent <device_node> '" << std::endl;
        return RET_ERR;
    };
    int32_t ret = command.RunSendEvent();
    return ret;
}

bool InputSendeventCommand::SendEventOption(int32_t argc, char** argv)
{
    CALL_DEBUG_ENTER;
    if (argc != SEND_EVENT_ARGC) {
        std::cerr << "Wrong number of input parameters" << std::endl;
        return false;
    }
    injectArgvs_.clear();
    injectArgvs_.push_back("sendevent");

    std::string deviceNode = argv[++optind];
    if (!(CheckDevice(deviceNode))) {
        std::cerr << "Input error in device node, deviceNode:" << deviceNode.c_str() << std::endl;
        return false;
    }
    injectArgvs_.push_back(deviceNode);

    std::string inputType = argv[++optind];
    if (!(CheckType(inputType))) {
        std::cerr << "Input error in type, type:" << inputType.c_str() << std::endl;
        return false;
    }
    injectArgvs_.push_back(inputType);

    std::string inputCode = argv[++optind];
    if (!(CheckCode(inputCode))) {
        std::cerr << "Input error in code, code:" << inputCode.c_str() << std::endl;
        return false;
    }
    injectArgvs_.push_back(inputCode);

    std::string inputValue = argv[++optind];
    if (!(CheckValue(inputValue))) {
        std::cerr << "Input error in value, value:" << inputValue.c_str() << std::endl;
        return false;
    }
    injectArgvs_.push_back(inputValue);
    return true;
}

bool InputSendeventCommand::CheckValue(const std::string &inputValue)
{
    if ((inputValue.length()) > INPUT_VALUE_LENGTH) {
        return false;
    }
    bool isValueNumber = regex_match(inputValue, std::regex("-?(\\d+)"));
    if (isValueNumber) {
        auto numberValue = stoll(inputValue);
        if ((numberValue >= INT32_MIN) && (numberValue <= INT32_MAX)) {
            return true;
        }
    }
    return false;
}

bool InputSendeventCommand::CheckCode(const std::string &inputCode)
{
    if ((inputCode.length()) > INPUT_CODE_LENGTH) {
        return false;
    }
    bool isCodeNumber = regex_match(inputCode, std::regex("\\d+"));
    if (isCodeNumber) {
        int32_t numberCode = stoi(inputCode);
        if (numberCode <= UINT16_MAX) {
            return true;
        }
    }
    return false;
}

bool InputSendeventCommand::CheckType(const std::string &inputType)
{
    if ((inputType.length()) > INPUT_TYPE_LENGTH) {
        return false;
    }
    bool isTypeNumber = regex_match(inputType, std::regex("\\d+"));
    if (isTypeNumber) {
        int32_t numberType = stoi(inputType);
        if (numberType <= UINT16_MAX) {
            return true;
        }
    }
    return false;
}

bool InputSendeventCommand::CheckDevice(const std::string &deviceNode)
{
    if (deviceNode.empty()) {
        return false;
    }
    char realPath[PATH_MAX] = {};
    if (realpath(deviceNode.c_str(), realPath) == nullptr) {
        return false;
    }
    return true;
}

int32_t InputSendeventCommand::RunSendEvent()
{
    if (injectArgvs_.size() != SEND_EVENT_ARGV_COUNTS) {
        std::cerr << "please use 'uinput sendevent <device_node> '" << std::endl;
        return RET_ERR;
    }
    std::string deviceNode = injectArgvs_[SEND_EVENT_DEV_NODE_INDEX];
    if (deviceNode.empty()) {
        std::cerr << "Device node does not exist:" << deviceNode.c_str() << std::endl;
        return RET_ERR;
    }
    char realPath[PATH_MAX] = {};
    if (realpath(deviceNode.c_str(), realPath) == nullptr) {
        std::cerr << "Device node path is error, path:" << deviceNode.c_str() << std::endl;
        return RET_ERR;
    }
    int32_t fd = open(realPath, O_RDWR);
    if (fd < 0) {
        std::cerr << "Open device node failed, path:" << deviceNode.c_str() << std::endl;
        return RET_ERR;
    }
    struct timeval tm;
    gettimeofday(&tm, 0);
    struct input_event event = {};
    event.input_event_sec = tm.tv_sec;
    event.input_event_usec = tm.tv_usec;
    event.type = static_cast<uint16_t>(std::stoi(injectArgvs_[SEND_EVENT_TYPE_INDEX]));
    event.code = static_cast<uint16_t>(std::stoi(injectArgvs_[SEND_EVENT_CODE_INDEX]));
    event.value = static_cast<int32_t>(std::stoi(injectArgvs_[SEND_EVENT_VALUE_INDEX]));
    int32_t ret = write(fd, &event, sizeof(event));
    if (ret != sizeof(event)) {
        std::cerr << "Send event to device node failed" << std::endl;
        close(fd);
        return RET_ERR;
    }
    if (fd >= 0) {
        close(fd);
    }
    return RET_OK;
}
} // namespace MMI
} // namespace OHOS