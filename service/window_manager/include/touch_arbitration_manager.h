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

#ifndef TOUCH_ARBITRATION_MANAGER_H
#define TOUCH_ARBITRATION_MANAGER_H

#include <algorithm>
#include <map>
#include <memory>

#include "pointer_event.h"
namespace OHOS {
namespace MMI {

class InputWindowsManager;

enum class TouchArbitrationSource {
    REAL_TOUCH,
    VIRTUAL_TOUCH,
    OTHER,
};

enum class TouchArbitrationPointerState {
    ACTIVE,
    CLOSED,
};

struct TouchArbitrationDeviceState {
    TouchArbitrationSource source { TouchArbitrationSource::OTHER };
    std::shared_ptr<PointerEvent> currentEvent { nullptr };
    std::map<int32_t, TouchArbitrationPointerState> pointerStates;

    bool HasActivePointer() const
    {
        return std::any_of(pointerStates.cbegin(), pointerStates.cend(), [](const auto &pointerState) {
            return pointerState.second == TouchArbitrationPointerState::ACTIVE;
        });
    }

    bool IsPointerInState(int32_t pointerId, TouchArbitrationPointerState state) const
    {
        auto pointerIter = pointerStates.find(pointerId);
        return pointerIter != pointerStates.end() && pointerIter->second == state;
    }

    void SetPointerState(int32_t pointerId, TouchArbitrationPointerState state)
    {
        pointerStates[pointerId] = state;
    }

    bool ClearPointerState(int32_t pointerId)
    {
        auto pointerIter = pointerStates.find(pointerId);
        if (pointerIter == pointerStates.end()) {
            return false;
        }
        pointerStates.erase(pointerIter);
        return true;
    }
};

struct DisplayTouchArbitrationState {
    std::map<int32_t, TouchArbitrationDeviceState> devices;
};

class TouchArbitrationManager {
public:
    explicit TouchArbitrationManager(InputWindowsManager *inputWindowsManager);

    TouchArbitrationSource GetTouchArbitrationSource(
        const std::shared_ptr<PointerEvent> &pointerEvent) const;

    void HandleTouchSourceDownArbitration(const std::shared_ptr<PointerEvent> &pointerEvent);
    bool HandleClosedTouchArbitrationStream(const std::shared_ptr<PointerEvent> &pointerEvent);
    void UpdateTouchArbitrationState(const std::shared_ptr<PointerEvent> &pointerEvent);
    void ClearTouchArbitrationStateByDevice(int32_t deviceId);

private:
    bool IsRealTouchEvent(const std::shared_ptr<PointerEvent> &pointerEvent) const;
    bool IsVirtualTouchEvent(const std::shared_ptr<PointerEvent> &pointerEvent) const;
    bool HasConflictingTouchSource(int32_t displayId, TouchArbitrationSource source) const;
    bool IsActiveTouchArbitrationStream(const std::shared_ptr<PointerEvent> &pointerEvent,
        TouchArbitrationSource source) const;
    void MarkClosedPointerItemsCanceled(const std::shared_ptr<PointerEvent> &pointerEvent);
    void ClearTouchArbitrationPointerState(int32_t displayId, TouchArbitrationSource source,
        int32_t pointerId, int32_t deviceId);
    void CancelConflictingTouchSource(int32_t displayId, TouchArbitrationSource source);

    InputWindowsManager *inputWindowsManager_ { nullptr };
    std::map<int32_t, DisplayTouchArbitrationState> states_;
};
} // namespace MMI
} // namespace OHOS
#endif // TOUCH_ARBITRATION_MANAGER_H
