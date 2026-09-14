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

#include <cstdint>
#include <set>

#include "printer.h"
#include "touch_support.h"

namespace OHOS::MMI::InputCli {
namespace {
const std::set<std::string> ALLOWED_OPTIONS = {
    "--displayId",
    "--duration",
    "--x",
    "--y",
};
} // namespace

std::string TouchTapCommand::GetName() const
{
    return "touch-tap";
}

std::string TouchTapCommand::GetDescription() const
{
    return "Simulate a touch screen tap";
}

std::string TouchTapCommand::GetTitle() const
{
    return "Simulate a touch screen tap at specified position";
}

std::string TouchTapCommand::GetUsage() const
{
    return "ohos-input touch-tap [options]";
}

std::vector<ParameterDoc> TouchTapCommand::GetParameters() const
{
    return {
        { "--displayId <number>", "Display ID (optional, range: >=0, default: 0)" },
        { "--x <integer>", "Touch X coordinate (required, range: >=0)" },
        { "--y <integer>", "Touch Y coordinate (required, range: >=0)" },
        { "--duration <number>",
            "Duration to hold the touch in ms (optional, range: [0, 5000], 0 means instant, default: 100)" },
    };
}

std::vector<std::string> TouchTapCommand::GetExamples() const
{
    return {
        "# Tap at position (300, 400)",
        "ohos-input touch-tap --x 300 --y 400",
        "",
        "# Long tap at position (300, 400) on display 1 with 800ms hold",
        "ohos-input touch-tap --x 300 --y 400 --displayId 1 --duration 800",
    };
}

int32_t TouchTapCommand::Execute(const std::vector<std::string> &args)
{
    Options options;
    TouchPointOptions point;
    std::string error;
    if (!ParseOptionPairs(args, ALLOWED_OPTIONS, options, error) || !ParseTouchPoint(options, point, error)) {
        return ParameterError(error.empty() ? "options must use documented --name value once" : error,
            "Please use documented touch tap arguments");
    }
    TouchSession session;
    int32_t ret = CreateTouchController(session);
    if (ret != 0) {
        return ret;
    }
    ret = RunTouchTap(session, point);
    if (ret != 0) {
        return ret;
    }
    return OutputPrinter::PrintSuccess({ { "action", "touch-tap" }, { "displayId", point.displayId },
        { "x", point.x }, { "y", point.y }, { "duration", point.duration } });
}
} // namespace OHOS::MMI::InputCli
