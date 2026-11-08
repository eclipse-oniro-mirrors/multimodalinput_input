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
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/file.h>
#include <unistd.h>

#include "input_manager.h"
#include "pointer_event.h"
#include "struct_multimodal.h"
#include "util.h"

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
constexpr char MOTION_STATE_LOCK_PATH[] = "/data/local/tmp/uinput_motion_state.lock";
constexpr char MOTION_STATE_TMP_PATH[] = "/data/local/tmp/uinput_motion_state.tmp";
constexpr mode_t MOTION_STATE_FILE_MODE { 0666 };
constexpr int32_t MOTION_INVALID_FD { -1 };
constexpr int32_t OPTION_VALUE_STEP { 2 };

struct MotionOptions {
    int32_t x { 0 };
    int32_t y { 0 };
    int32_t fingerId { DEFAULT_FINGER_ID };
    int32_t screenId { DEFAULT_SCREEN_ID };
};

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

bool IsValidFingerState(const FingerState &state)
{
    return (state.fingerId >= MIN_FINGER_ID) && (state.fingerId <= MAX_FINGER_ID) &&
        (state.screenId >= 0) && (state.x >= 0) && (state.y >= 0) && state.isDown;
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

bool ParseOptionArg(const std::string &name, const std::string &value, MotionOptions &options)
{
    int32_t number = 0;
    if (!ParseNumber(value, number)) {
        std::cerr << "invalid number for option " << name << ": " << value << std::endl;
        return false;
    }
    if (name == "-f") {
        options.fingerId = number;
    } else {
        options.screenId = number;
    }
    return true;
}

bool ApplyPositionalArg(const std::string &arg, int32_t &positionSeen, MotionOptions &options)
{
    if (positionSeen >= POSITION_COUNT) {
        std::cerr << "too many positional parameters: " << arg << std::endl;
        PrintUsage();
        return false;
    }
    int32_t coordinate = 0;
    if (!ParseNumber(arg, coordinate)) {
        std::cerr << "invalid coordinate value" << std::endl;
        PrintUsage();
        return false;
    }
    if (positionSeen == 0) {
        options.x = coordinate;
    } else {
        options.y = coordinate;
    }
    ++positionSeen;
    return true;
}

bool ParseOptions(int32_t argc, char** argv, MotionOptions &options)
{
    int32_t positionSeen = 0;
    int32_t i = FIRST_OPTION_INDEX;
    while (i < argc) {
        const std::string arg(argv[i]);
        if ((arg == "-f") || (arg == "-s")) {
            if (i + 1 >= argc) {
                std::cerr << "missing value for option: " << arg << std::endl;
                PrintUsage();
                return false;
            }
            if (!ParseOptionArg(arg, argv[i + 1], options)) {
                return false;
            }
            i += OPTION_VALUE_STEP;
            continue;
        }
        if (!ApplyPositionalArg(arg, positionSeen, options)) {
            return false;
        }
        ++i;
    }
    if (positionSeen < POSITION_COUNT) {
        std::cerr << "too few arguments to function" << std::endl;
        PrintUsage();
        return false;
    }
    return true;
}

bool ParseFingerStateLine(const std::string &line, FingerState &state)
{
    std::istringstream lineStream(line);
    int32_t isDown = 0;
    if (!(lineStream >> state.fingerId >> state.screenId >> state.x >> state.y >> isDown)) {
        return false;
    }
    state.isDown = (isDown != 0);
    int64_t downTime = 0;
    lineStream >> downTime;
    state.downTime = downTime;
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
    MotionOptions options;
    if (!ParseOptions(argc, argv, options)) {
        return MOTION_EXIT_FAILURE;
    }
    if (ValidateParams(options.x, options.y, options.fingerId, options.screenId) != MOTION_EXIT_SUCCESS) {
        return MOTION_EXIT_FAILURE;
    }
    if (subCommand == "motion_down") {
        return command.HandleDown(options.x, options.y, options.fingerId, options.screenId);
    }
    if (subCommand == "motion_move") {
        return command.HandleMove(options.x, options.y, options.fingerId, options.screenId);
    }
    return command.HandleUp(options.x, options.y, options.fingerId, options.screenId);
}

int32_t InputMotionCommand::ValidateParams(int32_t x, int32_t y, int32_t fingerId, int32_t screenId)
{
    if ((fingerId < MIN_FINGER_ID) || (fingerId > MAX_FINGER_ID)) {
        std::cerr << "invalid fingerId, range is 0-9" << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    if ((x < 0) || (y < 0)) {
        std::cerr << "Coordinate value must be greater or equal than 0" << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    if (screenId < 0) {
        std::cerr << "invalid displayId" << std::endl;
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
    int32_t fd = open(MOTION_STATE_LOCK_PATH, O_CREAT | O_RDWR, MOTION_STATE_FILE_MODE);
    if (fd == MOTION_INVALID_FD) {
        std::cerr << "failed to open state lock file: " << MOTION_STATE_LOCK_PATH << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        if (errno == EWOULDBLOCK) {
            std::cerr << "state file is locked by another process, please retry later or run motion_reset"
                << std::endl;
        } else {
            std::cerr << "failed to lock state file: " << MOTION_STATE_LOCK_PATH << ", errno=" << errno << std::endl;
        }
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
    int32_t dataFd = open(MOTION_STATE_FILE_PATH, O_RDONLY);
    if (dataFd == MOTION_INVALID_FD) {
        if (errno == ENOENT) {
            return MOTION_EXIT_SUCCESS;
        }
        std::cerr << "failed to open state file: " << MOTION_STATE_FILE_PATH << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    std::string content;
    char buf[256];
    ssize_t n = 0;
    while ((n = read(dataFd, buf, sizeof(buf))) > 0) {
        content.append(buf, static_cast<size_t>(n));
    }
    if (n < 0) {
        std::cerr << "failed to read state file: " << MOTION_STATE_FILE_PATH << std::endl;
        close(dataFd);
        return MOTION_EXIT_FAILURE;
    }
    close(dataFd);
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) {
            continue;
        }
        FingerState state {};
        if (!ParseFingerStateLine(line, state)) {
            std::cerr << "failed to parse state file: " << MOTION_STATE_FILE_PATH << std::endl;
            return MOTION_EXIT_FAILURE;
        }
        if (!IsValidFingerState(state)) {
            continue;
        }
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
                << (state.isDown ? 1 : 0) << " " << state.downTime << "\n";
        }
    }
    const std::string content = oss.str();
    int32_t tmpFd = open(MOTION_STATE_TMP_PATH, O_CREAT | O_WRONLY | O_TRUNC, MOTION_STATE_FILE_MODE);
    if (tmpFd == MOTION_INVALID_FD) {
        std::cerr << "failed to open temp state file: " << MOTION_STATE_TMP_PATH << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    size_t written = 0;
    while (written < content.size()) {
        ssize_t n = write(tmpFd, content.data() + written, content.size() - written);
        if (n <= 0) {
            std::cerr << "failed to write state file: " << MOTION_STATE_FILE_PATH << std::endl;
            close(tmpFd);
            return MOTION_EXIT_FAILURE;
        }
        written += static_cast<size_t>(n);
    }
    close(tmpFd);
    if (std::rename(MOTION_STATE_TMP_PATH, MOTION_STATE_FILE_PATH) != 0) {
        std::cerr << "failed to replace state file: " << MOTION_STATE_FILE_PATH << std::endl;
        return MOTION_EXIT_FAILURE;
    }
    return MOTION_EXIT_SUCCESS;
}

std::shared_ptr<PointerEvent> InputMotionCommand::BuildPointerEvent(int32_t pointerAction, int32_t x, int32_t y,
    int32_t fingerId, int32_t screenId, const FingerStateMap &stateMap, int64_t actionTime, int64_t downTime)
{
    auto pointerEvent = PointerEvent::Create();
    if (pointerEvent == nullptr) {
        std::cerr << "failed to create pointer event" << std::endl;
        return nullptr;
    }
    bool hasOtherPressedFinger = false;
    auto screenIt = stateMap.find(screenId);
    if (screenIt != stateMap.end()) {
        for (const auto &state : screenIt->second) {
            if (state.fingerId == fingerId) {
                continue;
            }
            hasOtherPressedFinger = true;
            PointerEvent::PointerItem item;
            item.SetPointerId(state.fingerId);
            item.SetOriginPointerId(state.fingerId);
            item.SetDisplayX(state.x);
            item.SetDisplayY(state.y);
            item.SetDisplayXPos(static_cast<double>(state.x));
            item.SetDisplayYPos(static_cast<double>(state.y));
            item.SetRawDisplayX(state.x);
            item.SetRawDisplayY(state.y);
            item.SetDeviceId(VIRTUAL_TOUCH_DEVICE_ID);
            item.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
            item.SetDownTime(state.downTime);
            item.SetPressed(true);
            pointerEvent->AddPointerItem(item);
        }
    }
    PointerEvent::PointerItem currentItem;
    currentItem.SetPointerId(fingerId);
    currentItem.SetOriginPointerId(fingerId);
    currentItem.SetDisplayX(x);
    currentItem.SetDisplayY(y);
    currentItem.SetDisplayXPos(static_cast<double>(x));
    currentItem.SetDisplayYPos(static_cast<double>(y));
    currentItem.SetRawDisplayX(x);
    currentItem.SetRawDisplayY(y);
    currentItem.SetDeviceId(VIRTUAL_TOUCH_DEVICE_ID);
    currentItem.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    currentItem.SetDownTime(downTime);
    currentItem.SetPressed(pointerAction != PointerEvent::POINTER_ACTION_UP);
    pointerEvent->AddPointerItem(currentItem);
    pointerEvent->SetPointerId(fingerId);
    pointerEvent->SetPointerAction(pointerAction);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetTargetDisplayId(screenId);
    pointerEvent->SetActionTime(actionTime);
    if ((pointerAction == PointerEvent::POINTER_ACTION_DOWN) && !hasOtherPressedFinger) {
        pointerEvent->SetActionStartTime(actionTime);
    }
    pointerEvent->SetDeviceId(VIRTUAL_TOUCH_DEVICE_ID);
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_RAW_POINTER_ID);
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
    state.downTime = GetSysClockTime();
    stateMap[screenId].push_back(state);
    if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    std::shared_ptr<PointerEvent> pointerEvent = BuildPointerEvent(PointerEvent::POINTER_ACTION_DOWN, x, y, fingerId,
        screenId, stateMap, GetSysClockTime(), state.downTime);
    if (pointerEvent == nullptr) {
        RollbackState(stateMap, screenId, fingerId);
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    if (InputManager::GetInstance()->SimulateInputEvent(pointerEvent) != RET_OK) {
        std::cerr << "inject event failed" << std::endl;
        RollbackState(stateMap, screenId, fingerId);
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
    int32_t oldX = pressed->x;
    int32_t oldY = pressed->y;
    pressed->x = x;
    pressed->y = y;
    if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    std::shared_ptr<PointerEvent> pointerEvent = BuildPointerEvent(PointerEvent::POINTER_ACTION_MOVE, x, y, fingerId,
        screenId, stateMap, GetSysClockTime(), pressed->downTime);
    if (pointerEvent == nullptr) {
        RollbackMoveState(stateMap, screenId, fingerId, oldX, oldY);
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    if (InputManager::GetInstance()->SimulateInputEvent(pointerEvent) != RET_OK) {
        std::cerr << "inject event failed" << std::endl;
        RollbackMoveState(stateMap, screenId, fingerId, oldX, oldY);
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
    FingerState lifted = *pressed;
    auto screenIt = stateMap.find(screenId);
    auto &fingers = screenIt->second;
    fingers.erase(std::remove_if(fingers.begin(), fingers.end(),
        [fingerId](const FingerState &state) { return state.fingerId == fingerId; }), fingers.end());
    if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    std::shared_ptr<PointerEvent> pointerEvent = BuildPointerEvent(PointerEvent::POINTER_ACTION_UP, x, y, fingerId,
        screenId, stateMap, GetSysClockTime(), lifted.downTime);
    if (pointerEvent == nullptr) {
        RollbackUpState(stateMap, lifted);
        ReleaseLock();
        return MOTION_EXIT_FAILURE;
    }
    if (InputManager::GetInstance()->SimulateInputEvent(pointerEvent) != RET_OK) {
        std::cerr << "inject event failed" << std::endl;
        RollbackUpState(stateMap, lifted);
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
        std::cerr << "failed to load state file: " << MOTION_STATE_FILE_PATH << ", clearing" << std::endl;
        stateMap.clear();
        if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
            ReleaseLock();
            return MOTION_EXIT_FAILURE;
        }
        ReleaseLock();
        return MOTION_EXIT_SUCCESS;
    }
    while (!stateMap.empty()) {
        auto screenIt = stateMap.begin();
        int32_t targetScreenId = screenIt->first;
        auto &fingers = screenIt->second;
        while (!fingers.empty()) {
            FingerState state = fingers.front();
            std::shared_ptr<PointerEvent> pointerEvent = BuildPointerEvent(PointerEvent::POINTER_ACTION_UP, state.x,
                state.y, state.fingerId, targetScreenId, stateMap, GetSysClockTime(), state.downTime);
            if (pointerEvent == nullptr) {
                ReleaseLock();
                return MOTION_EXIT_FAILURE;
            }
            if (InputManager::GetInstance()->SimulateInputEvent(pointerEvent) != RET_OK) {
                std::cerr << "inject event failed" << std::endl;
                ReleaseLock();
                return MOTION_EXIT_FAILURE;
            }
            fingers.erase(fingers.begin());
            if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
                std::cerr << "failed to persist lifted finger: " << MOTION_STATE_FILE_PATH << std::endl;
                ReleaseLock();
                return MOTION_EXIT_FAILURE;
            }
        }
        stateMap.erase(screenIt);
    }
    ReleaseLock();
    return MOTION_EXIT_SUCCESS;
}

void InputMotionCommand::RollbackState(FingerStateMap &stateMap, int32_t screenId, int32_t fingerId)
{
    auto screenIt = stateMap.find(screenId);
    if (screenIt != stateMap.end()) {
        auto &fingers = screenIt->second;
        fingers.erase(std::remove_if(fingers.begin(), fingers.end(),
            [fingerId](const FingerState &state) { return state.fingerId == fingerId; }), fingers.end());
    }
    if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
        std::cerr << "failed to rollback state file: " << MOTION_STATE_FILE_PATH << std::endl;
    }
}

void InputMotionCommand::RollbackMoveState(FingerStateMap &stateMap, int32_t screenId, int32_t fingerId,
    int32_t oldX, int32_t oldY)
{
    FingerState *pressed = FindFingerState(stateMap, screenId, fingerId);
    if (pressed != nullptr) {
        pressed->x = oldX;
        pressed->y = oldY;
    }
    if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
        std::cerr << "failed to rollback state file: " << MOTION_STATE_FILE_PATH << std::endl;
    }
}

void InputMotionCommand::RollbackUpState(FingerStateMap &stateMap, const FingerState &lifted)
{
    stateMap[lifted.screenId].push_back(lifted);
    if (SaveState(stateMap) != MOTION_EXIT_SUCCESS) {
        std::cerr << "failed to rollback state file: " << MOTION_STATE_FILE_PATH << std::endl;
    }
}
} // namespace MMI
} // namespace OHOS
