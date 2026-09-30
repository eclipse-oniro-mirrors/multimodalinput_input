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

#include "touch_swipe_command.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <thread>
#include <vector>

#include "error_multimodal.h"
#include "input_manager.h"
#include "option_parser.h"
#include "printer.h"
#include "touch_controller_impl.h"

namespace OHOS::MMI::InputCli {
namespace {
const std::set<std::string> ALLOWED_OPTIONS = { "--displayId", "--srcX", "--srcY", "--dstX", "--dstY", "--duration" };
constexpr int32_t TOUCH_ID = 0;
constexpr int32_t MOVE_INTERVAL_MS = 16;
struct SwipeOptions {
    int32_t displayId { 0 };
    int32_t srcX { 0 };
    int32_t srcY { 0 };
    int32_t dstX { 0 };
    int32_t dstY { 0 };
    int32_t duration { 500 };
};
struct SwipeStep {
    int32_t x;
    int32_t y;
    int32_t delayMs;
};

bool ParseSwipeOptions(const std::vector<std::string> &args, SwipeOptions &options, std::string &error)
{
    Options values;
    if (!ParseOptionPairs(args, ALLOWED_OPTIONS, values, error)) {
        return false;
    }
    const NumberRule durationRule {
        .optionName = "--duration",
        .isRequired = false,
        .defaultValue = 500,
        .minValue = 0,
        .maxValue = 5000,
    };
    return ParseNumber(values, DisplayIdRule("--displayId"), options.displayId, error) &&
        ParseNumber(values, CoordinateRule("--srcX"), options.srcX, error) &&
        ParseNumber(values, CoordinateRule("--srcY"), options.srcY, error) &&
        ParseNumber(values, CoordinateRule("--dstX"), options.dstX, error) &&
        ParseNumber(values, CoordinateRule("--dstY"), options.dstY, error) &&
        ParseNumber(values, durationRule, options.duration, error);
}

std::vector<SwipeStep> BuildSwipeSteps(const SwipeOptions &options)
{
    std::vector<SwipeStep> steps;
    if (options.duration == 0) {
        return steps;
    }
    const int64_t deltaX = static_cast<int64_t>(options.dstX) - options.srcX;
    const int64_t deltaY = static_cast<int64_t>(options.dstY) - options.srcY;
    const int32_t count = (options.duration + MOVE_INTERVAL_MS - 1) / MOVE_INTERVAL_MS;
    steps.reserve(static_cast<std::size_t>(count));
    for (int32_t index = 1; index <= count; ++index) {
        const int32_t delay = index == count ? options.duration - MOVE_INTERVAL_MS * (count - 1)
                                             : MOVE_INTERVAL_MS;
        const int32_t x = static_cast<int32_t>(options.srcX + deltaX * index / count);
        const int32_t y = static_cast<int32_t>(options.srcY + deltaY * index / count);
        steps.push_back({ x, y, delay });
    }
    return steps;
}

int32_t InjectSwipe(TouchControllerImpl &controller, const SwipeOptions &options)
{
    int32_t ret = controller.TouchDown(TOUCH_ID, options.displayId, options.srcX, options.srcY);
    if (ret != 0) {
        return HandleControllerError(ret, "TouchDown");
    }

    int32_t lastX = options.srcX;
    int32_t lastY = options.srcY;
    const bool shouldMove = options.srcX != options.dstX || options.srcY != options.dstY;
    if (!shouldMove) {
        if (options.duration > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(options.duration));
        }
    } else {
        for (const auto &step : BuildSwipeSteps(options)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(step.delayMs));
            ret = controller.TouchMove(TOUCH_ID, options.displayId, step.x, step.y);
            if (ret != 0) {
                (void)controller.TouchUp(TOUCH_ID, options.displayId, lastX, lastY);
                return HandleControllerError(ret, "TouchMove");
            }
            lastX = step.x;
            lastY = step.y;
        }
    }

    ret = controller.TouchUp(TOUCH_ID, options.displayId, options.dstX, options.dstY);
    if (ret != 0) {
        return HandleControllerError(ret, "TouchUp");
    }
    return 0;
}
}

std::string TouchSwipeCommand::GetName() const
{
    return "touch-swipe";
}

std::string TouchSwipeCommand::GetDescription() const
{
    return "Simulate a touch-screen swipe";
}

std::string TouchSwipeCommand::GetTitle() const
{
    return "Swipe between two positions";
}
std::string TouchSwipeCommand::GetUsage() const
{
    return "ohos-input touch-swipe --srcX <int> --srcY <int> --dstX <int> --dstY <int> "
        "[--displayId <int>] [--duration <ms>]";
}
std::vector<ParameterDoc> TouchSwipeCommand::GetParameters() const
{
    return { { "--displayId <int>", "Target display ID (>=0, default: 0)" },
        { "--srcX <int>", "Starting X coordinate (required, >=0)" },
        { "--srcY <int>", "Starting Y coordinate (required, >=0)" },
        { "--dstX <int>", "Ending X coordinate (required, >=0)" },
        { "--dstY <int>", "Ending Y coordinate (required, >=0)" },
        { "--duration <ms>", "Swipe duration in [0, 5000] ms (default: 500)" } };
}
std::vector<std::string> TouchSwipeCommand::GetExamples() const
{
    return { "ohos-input touch-swipe --displayId 0 --srcX 100 --srcY 500 --dstX 900 --dstY 500 --duration 500" };
}

int32_t TouchSwipeCommand::Execute(const std::vector<std::string> &args)
{
    SwipeOptions options;
    std::string error;
    if (!ParseSwipeOptions(args, options, error)) {
        return ParameterError(error, "Please use documented touch swipe arguments");
    }
    std::shared_ptr<TouchControllerImpl> controller;
    auto *inputManager = InputManager::GetInstance();
    if (inputManager == nullptr) {
        return HandleControllerError(ERROR_NULL_POINTER, "InputManager::GetInstance");
    }
    const int32_t createResult = inputManager->CreateTouchController(controller);
    if (createResult != 0 || controller == nullptr) {
        return HandleControllerError(createResult, "CreateTouchController");
    }
    const int32_t ret = InjectSwipe(*controller, options);
    if (ret != 0) {
        return ret;
    }
    return OutputPrinter::PrintSuccess({ { "action", "touch-swipe" }, { "displayId", options.displayId },
        { "srcX", options.srcX }, { "srcY", options.srcY }, { "dstX", options.dstX }, { "dstY", options.dstY },
        { "duration", options.duration } });
}
} // namespace OHOS::MMI::InputCli
