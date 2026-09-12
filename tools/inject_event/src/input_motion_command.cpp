/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "input_motion_command.h"

#include <algorithm>
#include <charconv>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/file.h>
#include <unistd.h>

#include "input_manager.h"
#include "pointer_event.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr int32_t MOTION_EXIT_SUCCESS { 0 };
constexpr int32_t MOTION_EXIT_FAILURE { 1 };
constexpr int32_t MIN_FINGER_ID { 0 };
constexpr int32_t MAX_FINGER_ID { 9 };
constexpr int32_t DEFAULT_FINGER_ID { 0 };
constexpr int32_t DEFAULT_SCREEN_ID { 0 };
constexpr int32_t SUB_COMMAND_INDEX { 1 };
constexpr int32_t FIRST_OPTION_INDEX { 2 };
constexpr int32_t POSITION_COUNT { 2 };
constexpr char MOTION_STATE_FILE_PATH[] = "/data/local/tmp/uinput_motion_state";
constexpr mode_t MOTION_STATE_FILE_MODE { 0666 };
constexpr int32_t MOTION_INVALID_FD { -1 };

FingerState *FindFingerState(FingerStateMap &stateMap, int32_t screenId, int32_t fingerId)
{
    auto screenIt = stateMap.find(screenId);
    if (screenIt == stateMap.end()) {
        return nullptr;
    }
    for (auto &state : screenIt->second) {
        if (state.fingerId == fingerId) {
            return &state;
        }
    }
    return nullptr;
}

void PrintUsage()
{
    std::cerr << "usage: uinput motion_down <x> <y> [-f fingerId] [-s screenId]" << std::endl;
    std::cerr << "       uinput motion_move <x> <y> [-f fingerId] [-s screenId]" << std::endl;
    std::cerr << "       uinput motion_up <x> <y> [-f fingerId] [-s screenId]" << std::endl;
    std::cerr << "       uinput motion_reset" << std::endl;
}

bool ParseNumber(const std::string &text, int32_t &value)
{
    if (text.empty()) {
        return false;
    }
    int32_t result = 0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), result);
    if ((ec != std::errc()) || (ptr != text.data() + text.size())) {
        return false;
    }
    value = result;
    return true;
}

bool ParseOptions(int32_t argc, char** argv, int32_t &x, int32_t &y, int32_t &fingerId, int32_t &screenId)
{
    fingerId = DEFAULT_FINGER_ID;
    screenId = DEFAULT_SCREEN_ID;
    int32_t positionSeen = 0;
    for (int32_t i = FIRST_OPTION_INDEX; i < argc; ++i) {
        const std::string arg(argv[i]);
        if ((arg == "-f") || (arg == "-s")) {
            if (i + 1 >= argc) {
                std::cerr << "missing value for option: " << arg << std::endl;
                PrintUsage();
                return false;
            }
            int32_t value = 0;
            if (!ParseNumber(argv[i + 1], value)) {
                std::cerr << "invalid number for option " << arg << ": " << argv[i + 1] << std::endl;
                return false;
            }
            if (arg == "-f") {
                fingerId = value;
            } else {
                screenId = value;
            }
            ++i;
            continue;
        }
        if (positionSeen >= POSITION_COUNT) {
            std::cerr << "too many positional parameters: " << arg << std::endl;
            PrintUsage();
            return false;
        }
        int32_t coordinate = 0;
        if (!ParseNumber(arg, coordinate)) {
            std::cerr << "invalid coordinate: " << arg << std::endl;
            PrintUsage();
            return false;
        }
        if (positionSeen == 0) {
            x = coordinate;
        } else {
            y = coordinate;
        }
        ++positionSeen;
    }
    if (positionSeen < POSITION_COUNT) {
        std::cerr << "missing required parameter: x and y" << std::endl;
        PrintUsage();
        return false;
    }
    return true;
}
} // namespace

int32_t InputMotionCommand::Handle(int32_t argc, char** argv)
{
    if ((argv == nullptr) || (argc <= SUB_COMMAND_INDEX)) {
        PrintUsage();
        return MOTION_EXIT_FAILURE;
    }
    const std::string subCommand(argv[SUB_COMMAND_INDEX]);
    InputMotionCommand command;
    if (subCommand == "motion_reset") {
        return command.HandleReset();
    }
    if ((subCommand != "motion_down") && (subCommand != "motion_move") && (subCommand != "motion_up")) {
        std::cerr << "unknown motion subcommand: " << subCommand << std::endl;
        PrintUsage();
        return MOTION_EXIT_FAILURE;
    }
    int32_t x = 0;
    int32_t y = 0;
    int32_t fingerId = DEFAULT_FINGER_ID;
    int32_t screenId = DEFAULT_SCREEN_ID;
    if (!ParseOptions(argc, argv, x, y, fingerId, screenId)) {
        return MOTION_EXIT_FAILURE;
    }
    if (ValidateParams(x, y, fingerId) != MOTION_EXIT_SUCCESS) {
        return MOTION_EXIT_FAILURE;
    }
    if (subCommand == "motion_down") {
        return command.HandleDown(x, y, fingerId, screenId);
    }
    if (subCommand == "motion_move") {
        return command.HandleMove(x, y, fingerId, screenId);
    }
    return command.HandleUp(x, y, fingerId, screenId);
}

int32_t InputMotionCommand::ValidateParams(int32_t x, int32_t y, int32_t fingerId)
{
    if ((fingerId < MIN_FINGER_ID) || (fingerId > MAX_FINGER_ID)) {
        std::cerr << "invalid fingerId, range is 0-9" << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    if ((x < 0) || (y < 0)) {
        std::cerr << "invalid coordinate, x and y must be non-negative" << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    return MOTION_EXIT_SUCCESS;
}

int32_t InputMotionCommand::AcquireLock()
{
    if (fd_ != MOTION_INVALID_FD) {
        std::cerr << "state file lock is already held" << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    int32_t fd = open(MOTION_STATE_FILE_PATH, O_CREAT | O_RDWR, MOTION_STATE_FILE_MODE);
    if (fd == MOTION_INVALID_FD) {
        std::cerr << "failed to open state file: " << MOTION_STATE_FILE_PATH << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        std::cerr << "state file is locked by another process" << std::endl;
        close(fd);
        return MOTION_EXIT_FAILURE;
    }
    fd_ = fd;
    return MOTION_EXIT_SUCCESS;
}

void InputMotionCommand::ReleaseLock()
{
    if (fd_ == MOTION_INVALID_FD) {
        return;
    }
    flock(fd_, LOCK_UN);
    close(fd_);
    fd_ = MOTION_INVALID_FD;
}

int32_t InputMotionCommand::LoadState(FingerStateMap &stateMap)
{
    stateMap.clear();
    if (fd_ == MOTION_INVALID_FD) {
        std::cerr << "state file is not locked before reading" << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    std::string content;
    char buf[256];
    ssize_t n = 0;
    lseek(fd_, 0, SEEK_SET);
    while ((n = read(fd_, buf, sizeof(buf))) > 0) {
        content.append(buf, static_cast<size_t>(n));
    }
    if (n < 0) {
        std::cerr << "failed to read state file: " << MOTION_STATE_FILE_PATH << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) {
            continue;
        }
        FingerState state {};
        int32_t isDown = 0;
        std::istringstream lineStream(line);
        if (!(lineStream >> state.fingerId >> state.screenId >> state.x >> state.y >> isDown)) {
            std::cerr << "failed to parse state file: " << MOTION_STATE_FILE_PATH << std::endl;
            return MOTION_EXIT_FAILURE;
        }
        state.isDown = (isDown != 0);
        stateMap[state.screenId].push_back(state);
    }
    return MOTION_EXIT_SUCCESS;
}

int32_t InputMotionCommand::SaveState(const FingerStateMap &stateMap)
{
    if (fd_ == MOTION_INVALID_FD) {
        std::cerr << "state file is not locked before writing" << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    std::ostringstream oss;
    for (const auto &entry : stateMap) {
        for (const auto &state : entry.second) {
            oss << state.fingerId << " " << state.screenId << " " << state.x << " " << state.y << " "
                << (state.isDown ? 1 : 0) << "\n";
        }
    }
    const std::string content = oss.str();
    lseek(fd_, 0, SEEK_SET);
    if (ftruncate(fd_, 0) != 0) {
        std::cerr << "failed to truncate state file: " << MOTION_STATE_FILE_PATH << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    size_t written = 0;
    while (written < content.size()) {
        ssize_t n = write(fd_, content.data() + written, content.size() - written);
        if (n <= 0) {
            std::cerr << "failed to write state file: " << MOTION_STATE_FILE_PATH << std::endl;
            return MOTION_EXIT_FAILURE;
        }
        written += static_cast<size_t>(n);
    }
    return MOTION_EXIT_SUCCESS;
}

std::shared_ptr<PointerEvent> InputMotionCommand::BuildPointerEvent(int32_t pointerAction, int32_t x, int32_t y,
    int32_t fingerId, int32_t screenId, const FingerStateMap &stateMap)
{
    auto pointerEvent = PointerEvent::Create();
    if (pointerEvent == nullptr) {
        std::cerr << "failed to create pointer event" << std::endl;
        return nullptr;
    }
    auto screenIt = stateMap.find(screenId);
    if (screenIt != stateMap.end()) {
        for (const auto &state : screenIt->second) {
            if (state.fingerId == fingerId) {
                continue;
            }
            PointerEvent::PointerItem item;
            item.SetPointerId(state.fingerId);
            item.SetDisplayX(state.x);
            item.SetDisplayY(state.y);
            item.SetRawDisplayX(state.x);
            item.SetRawDisplayY(state.y);
            pointerEvent->AddPointerItem(item);
        }
    }
    PointerEvent::PointerItem currentItem;
    currentItem.SetPointerId(fingerId);
    currentItem.SetDisplayX(x);
    currentItem.SetDisplayY(y);
    currentItem.SetRawDisplayX(x);
    currentItem.SetRawDisplayY(y);
    pointerEvent->AddPointerItem(currentItem);
    pointerEvent->SetPointerId(fingerId);
    pointerEvent->SetPointerAction(pointerAction);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetTargetDisplayId(screenId);
    return pointerEvent;
}

int32_t InputMotionCommand::HandleDown(int32_t x, int32_t y, int32_t fingerId, int32_t screenId)
{
    if (AcquireLock() != MOTION_EXIT_SUCCESS) {
        return MOTION_EXIT_FAILURE;
    }
    FingerStateMap stateMap;
    if (LoadState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    FingerState *pressed = FindFingerState(stateMap, screenId, fingerId);
    if ((pressed != nullptr) && (pressed->isDown)) {
        std::cerr << "finger " << fingerId << " is already pressed" << std::endl;
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    FingerState state {};
    state.fingerId = fingerId;
    state.screenId = screenId;
    state.x = x;
    state.y = y;
    state.isDown = true;
    stateMap[screenId].push_back(state);
    std::shared_ptr<PointerEvent> pointerEvent = BuildPointerEvent(PointerEvent::POINTER_ACTION_DOWN, x, y, fingerId,
        screenId, stateMap);
    if (pointerEvent == nullptr) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    if (InputManager::GetInstance()->SimulateInputEvent(pointerEvent) != RET_OK) {
        std::cerr << "inject event failed" << std::endl;
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    ReleaseLock();
    return MOTION_EXIT_SUCCESS;
}

int32_t InputMotionCommand::HandleMove(int32_t x, int32_t y, int32_t fingerId, int32_t screenId)
{
    if (AcquireLock() != MOTION_EXIT_SUCCESS) {
        return MOTION_EXIT_FAILURE;
    }
    FingerStateMap stateMap;
    if (LoadState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    FingerState *pressed = FindFingerState(stateMap, screenId, fingerId);
    if ((pressed == nullptr) || (!pressed->isDown)) {
        std::cerr << "finger " << fingerId << " is not pressed, cannot move" << std::endl;
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    pressed->x = x;
    pressed->y = y;
    std::shared_ptr<PointerEvent> pointerEvent = BuildPointerEvent(PointerEvent::POINTER_ACTION_MOVE, x, y, fingerId,
        screenId, stateMap);
    if (pointerEvent == nullptr) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    if (InputManager::GetInstance()->SimulateInputEvent(pointerEvent) != RET_OK) {
        std::cerr << "inject event failed" << std::endl;
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    ReleaseLock();
    return MOTION_EXIT_SUCCESS;
}

int32_t InputMotionCommand::HandleUp(int32_t x, int32_t y, int32_t fingerId, int32_t screenId)
{
    if (AcquireLock() != MOTION_EXIT_SUCCESS) {
        return MOTION_EXIT_FAILURE;
    }
    FingerStateMap stateMap;
    if (LoadState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    FingerState *pressed = FindFingerState(stateMap, screenId, fingerId);
    if ((pressed == nullptr) || (!pressed->isDown)) {
        std::cerr << "finger " << fingerId << " is not pressed, cannot lift" << std::endl;
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    auto screenIt = stateMap.find(screenId);
    auto &fingers = screenIt->second;
    fingers.erase(std::remove_if(fingers.begin(), fingers.end(),
        [fingerId](const FingerState &state) { return state.fingerId == fingerId; }), fingers.end());
    std::shared_ptr<PointerEvent> pointerEvent = BuildPointerEvent(PointerEvent::POINTER_ACTION_UP, x, y, fingerId,
        screenId, stateMap);
    if (pointerEvent == nullptr) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    if (InputManager::GetInstance()->SimulateInputEvent(pointerEvent) != RET_OK) {
        std::cerr << "inject event failed" << std::endl;
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    ReleaseLock();
    return MOTION_EXIT_SUCCESS;
}

int32_t InputMotionCommand::HandleReset()
{
    if (AcquireLock() != MOTION_EXIT_SUCCESS) {
        return MOTION_EXIT_FAILURE;
    }
    FingerStateMap stateMap;
    if (LoadState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    for (const auto &screenEntry : stateMap) {
        for (const auto &state : screenEntry.second) {
            std::shared_ptr<PointerEvent> pointerEvent = BuildPointerEvent(PointerEvent::POINTER_ACTION_UP, state.x,
                state.y, state.fingerId, screenEntry.first, stateMap);
            if (pointerEvent == nullptr) {
                ReleaseLock();
                return MOTION_EXIT_FAILURE;
            }
            if (InputManager::GetInstance()->SimulateInputEvent(pointerEvent) != RET_OK) {
                std::cerr << "inject event failed" << std::endl;
                ReleaseLock();
                return MOTION_EXIT_FAILURE;
            }
        }
    }
    stateMap.clear();
    if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    ReleaseLock();
    return MOTION_EXIT_SUCCESS;
}
} // namespace MMI
} // namespace OHOS
