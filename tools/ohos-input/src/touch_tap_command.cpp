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

#include "touch_tap_command.h"

#include <chrono>
#include <memory>
#include <set>
#include <thread>

#include "input_manager.h"
#include "option_parser.h"
#include "printer.h"
#include "touch_controller_impl.h"

namespace OHOS::MMI::InputCli {
namespace {
const std::set<std::string> ALLOWED_OPTIONS = { "--displayId", "--x", "--y", "--duration" };
struct TapOptions {
    int32_t displayId { 0 };
    int32_t x { 0 };
    int32_t y { 0 };
    int32_t duration { 100 };
};

bool ParseTapOptions(const std::vector<std::string> &args, TapOptions &options, std::string &error)
{
    Options values;
    if (!ParseOptionPairs(args, ALLOWED_OPTIONS, values, error)) {
        return false;
    }
    const NumberRule durationRule {
        .optionName = "--duration",
        .isRequired = false,
        .defaultValue = 100,
        .minValue = 0,
        .maxValue = 5000,
    };
    return ParseNumber(values, DisplayIdRule("--displayId"), options.displayId, error) &&
        ParseNumber(values, CoordinateRule("--x"), options.x, error) &&
        ParseNumber(values, CoordinateRule("--y"), options.y, error) &&
        ParseNumber(values, durationRule, options.duration, error);
}
constexpr int32_t TOUCH_ID = 0;
}

std::string TouchTapCommand::GetName() const
{
    return "touch-tap";
}

std::string TouchTapCommand::GetDescription() const
{
    return "Simulate a touch-screen tap";
}

std::string TouchTapCommand::GetTitle() const
{
    return "Tap a position on a display";
}

std::string TouchTapCommand::GetUsage() const
{
    return "ohos-input touch-tap --x <int> --y <int> [--displayId <int>] [--duration <ms>]";
}

std::vector<ParameterDoc> TouchTapCommand::GetParameters() const
{
    return { { "--displayId <int>", "Target display ID (>=0, default: 0)" },
        { "--x <int>", "Target X coordinate (required, >=0)" }, { "--y <int>", "Target Y coordinate (required, >=0)" },
        { "--duration <ms>", "Press duration in [0, 5000] ms (default: 100)" } };
}

std::vector<std::string> TouchTapCommand::GetExamples() const
{
    return { "ohos-input touch-tap --displayId 0 --x 500 --y 800 --duration 200" };
}

int32_t TouchTapCommand::Execute(const std::vector<std::string> &args)
{
    TapOptions options;
    std::string error;
    if (!ParseTapOptions(args, options, error)) {
        return ParameterError(error, "Please use documented touch tap arguments");
    }
    std::shared_ptr<TouchControllerImpl> controller;
    const int32_t createResult = InputManager::GetInstance()->CreateTouchController(controller);
    if (createResult != 0 || controller == nullptr) {
        return HandleControllerError(createResult, "CreateTouchController");
    }
    int32_t ret = controller->TouchDown(TOUCH_ID, options.displayId, options.x, options.y);
    if (ret != 0) {
        return HandleControllerError(ret, "TouchDown");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(options.duration));
    ret = controller->TouchUp(TOUCH_ID, options.displayId, options.x, options.y);
    if (ret != 0) {
        return HandleControllerError(ret, "TouchUp");
    }
    return OutputPrinter::PrintSuccess({ { "action", "touch-tap" }, { "displayId", options.displayId },
        { "x", options.x }, { "y", options.y }, { "duration", options.duration } });
}
} // namespace OHOS::MMI::InputCli
