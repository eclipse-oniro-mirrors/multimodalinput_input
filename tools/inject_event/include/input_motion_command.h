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

#ifndef INPUT_MOTION_COMMAND_H
#define INPUT_MOTION_COMMAND_H

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "nocopyable.h"

namespace OHOS {
namespace MMI {
class PointerEvent;

struct FingerState {
    int32_t fingerId;
    int32_t screenId;
    int32_t x;
    int32_t y;
    bool isDown;
    int64_t downTime;
};

using FingerStateMap = std::map<int32_t, std::vector<FingerState>>;

class InputMotionCommand {
public:
    InputMotionCommand() = default;
    ~InputMotionCommand() = default;
    DISALLOW_COPY_AND_MOVE(InputMotionCommand);

    static int32_t Handle(int32_t argc, char** argv);

    int32_t AcquireLock();
    void ReleaseLock();
    int32_t LoadState(FingerStateMap &stateMap);
    int32_t SaveState(const FingerStateMap &stateMap);
private:
    static int32_t ValidateParams(int32_t x, int32_t y, int32_t fingerId);
    static std::shared_ptr<PointerEvent> BuildPointerEvent(int32_t pointerAction, int32_t x, int32_t y,
        int32_t fingerId, int32_t screenId, const FingerStateMap &stateMap, int64_t actionTime, int64_t downTime);
    void RollbackState(FingerStateMap &stateMap, int32_t screenId, int32_t fingerId);
    void RollbackMoveState(FingerStateMap &stateMap, int32_t screenId, int32_t fingerId, int32_t oldX, int32_t oldY);
    void RollbackUpState(FingerStateMap &stateMap, const FingerState &lifted);
    int32_t HandleDown(int32_t x, int32_t y, int32_t fingerId, int32_t screenId);
    int32_t HandleMove(int32_t x, int32_t y, int32_t fingerId, int32_t screenId);
    int32_t HandleUp(int32_t x, int32_t y, int32_t fingerId, int32_t screenId);
    int32_t HandleReset();
private:
    int32_t fd_ { -1 };
};
} // namespace MMI
} // namespace OHOS
#endif // INPUT_MOTION_COMMAND_H
