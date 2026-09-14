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

#include "touch_support.h"

#include <chrono>
#include <cstdint>
#include <thread>

#include "input_manager.h"
#include "printer.h"

namespace OHOS::MMI::InputCli {
bool ParseTouchPoint(const Options &options, TouchPointOptions &result, std::string &error)
{
    const NumberRule xRule = CoordinateRule("--x");
    const NumberRule yRule = CoordinateRule("--y");
    const NumberRule displayIdRule = DisplayIdRule("--displayId");
    const NumberRule durationRule {
        .optionName = "--duration",
        .isRequired = false,
        .defaultValue = DEFAULT_TOUCH_TAP_DURATION_MS,
        .minValue = 0,
        .maxValue = MAX_TOUCH_DURATION_MS,
    };
    return ParseNumber(options, xRule, result.x, error) &&
        ParseNumber(options, yRule, result.y, error) &&
        ParseNumber(options, displayIdRule, result.displayId, error) &&
        ParseNumber(options, durationRule, result.duration, error);
}

bool ParseTouchSwipe(const Options &options, TouchSwipeOptions &result, std::string &error)
{
    const NumberRule srcXRule = CoordinateRule("--srcX");
    const NumberRule srcYRule = CoordinateRule("--srcY");
    const NumberRule dstXRule = CoordinateRule("--dstX");
    const NumberRule dstYRule = CoordinateRule("--dstY");
    const NumberRule displayIdRule = DisplayIdRule("--displayId");
    const NumberRule durationRule {
        .optionName = "--duration",
        .isRequired = false,
        .defaultValue = DEFAULT_TOUCH_SWIPE_DURATION_MS,
        .minValue = 0,
        .maxValue = MAX_TOUCH_DURATION_MS,
    };
    return ParseNumber(options, srcXRule, result.srcX, error) &&
        ParseNumber(options, srcYRule, result.srcY, error) &&
        ParseNumber(options, dstXRule, result.dstX, error) &&
        ParseNumber(options, dstYRule, result.dstY, error) &&
        ParseNumber(options, displayIdRule, result.displayId, error) &&
        ParseNumber(options, durationRule, result.duration, error);
}

int32_t CreateTouchController(TouchSession &session)
{
    const int32_t ret = InputManager::GetInstance()->CreateTouchController(session.touch);
    if (ret != 0 || session.touch == nullptr) {
        return HandleControllerError(ret, "CreateTouchController");
    }
    return 0;
}

std::vector<DragStep> BuildSwipeSteps(int32_t duration, int32_t srcX, int32_t srcY, int32_t dstX, int32_t dstY)
{
    return BuildDragSteps(duration, DragPath { 0, srcX, srcY, 0, dstX, dstY });
}

int32_t RunTouchTap(const TouchSession &session, const TouchPointOptions &options)
{
    int32_t ret = session.touch->TouchDown(TOUCH_ID, options.displayId, options.x, options.y);
    if (ret != 0) {
        return HandleControllerError(ret, "TouchDown");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(options.duration));
    ret = session.touch->TouchUp(TOUCH_ID, options.displayId, options.x, options.y);
    return ret == 0 ? 0 : HandleControllerError(ret, "TouchUp");
}

int32_t RunTouchSwipe(const TouchSession &session, const TouchSwipeOptions &options)
{
    int32_t ret = session.touch->TouchDown(TOUCH_ID, options.displayId, options.srcX, options.srcY);
    if (ret != 0) {
        return HandleControllerError(ret, "TouchDown");
    }
    const auto steps = BuildSwipeSteps(options.duration, options.srcX, options.srcY, options.dstX, options.dstY);
    for (const auto &step : steps) {
        std::this_thread::sleep_for(std::chrono::milliseconds(step.delayMs));
        if (!step.move) {
            continue;
        }
        ret = session.touch->TouchMove(TOUCH_ID, options.displayId, step.x, step.y);
        if (ret != 0) {
            session.touch->TouchUp(TOUCH_ID, options.displayId, options.dstX, options.dstY);
            return HandleControllerError(ret, "TouchMove");
        }
    }
    ret = session.touch->TouchUp(TOUCH_ID, options.displayId, options.dstX, options.dstY);
    return ret == 0 ? 0 : HandleControllerError(ret, "TouchUp");
}
} // namespace OHOS::MMI::InputCli
