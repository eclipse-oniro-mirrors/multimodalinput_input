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

#include <cstdint>
#include <set>

#include "printer.h"
#include "touch_support.h"

namespace OHOS::MMI::InputCli {
namespace {
const std::set<std::string> ALLOWED_OPTIONS = {
    "--displayId",
    "--dstX",
    "--dstY",
    "--duration",
    "--srcX",
    "--srcY",
};
} // namespace

std::string TouchSwipeCommand::GetName() const
{
    return "touch-swipe";
}

std::string TouchSwipeCommand::GetDescription() const
{
    return "Simulate a touch screen swipe";
}

std::string TouchSwipeCommand::GetTitle() const
{
    return "Simulate a touch screen swipe from source to destination";
}

std::string TouchSwipeCommand::GetUsage() const
{
    return "ohos-input touch-swipe [options]";
}

std::vector<ParameterDoc> TouchSwipeCommand::GetParameters() const
{
    return {
        { "--displayId <number>", "Display ID (optional, range: >=0, default: 0)" },
        { "--srcX <integer>", "Source X coordinate (required, range: >=0)" },
        { "--srcY <integer>", "Source Y coordinate (required, range: >=0)" },
        { "--dstX <integer>", "Destination X coordinate (required, range: >=0)" },
        { "--dstY <integer>", "Destination Y coordinate (required, range: >=0)" },
        { "--duration <number>",
            "Total swipe duration in ms, including touch down and up (optional, range: [0, 5000], default: 500)" },
    };
}

std::vector<std::string> TouchSwipeCommand::GetExamples() const
{
    return {
        "# Swipe from (400, 800) to (400, 300) over 500ms",
        "ohos-input touch-swipe --srcX 400 --srcY 800 --dstX 400 --dstY 300",
        "",
        "# Instant swipe without intermediate move steps",
        "ohos-input touch-swipe --srcX 400 --srcY 800 --dstX 400 --dstY 300 --duration 0",
        "",
        "# Slow swipe on display 1 over 2000ms",
        "ohos-input touch-swipe --srcX 400 --srcY 800 --dstX 400 --dstY 300 --displayId 1 --duration 2000",
    };
}

int32_t TouchSwipeCommand::Execute(const std::vector<std::string> &args)
{
    Options options;
    TouchSwipeOptions swipe;
    std::string error;
    if (!ParseOptionPairs(args, ALLOWED_OPTIONS, options, error) || !ParseTouchSwipe(options, swipe, error)) {
        return ParameterError(error.empty() ? "options must use documented --name value once" : error,
            "Please use documented touch swipe arguments");
    }
    TouchSession session;
    int32_t ret = CreateTouchController(session);
    if (ret != 0) {
        return ret;
    }
    ret = RunTouchSwipe(session, swipe);
    if (ret != 0) {
        return ret;
    }
    return OutputPrinter::PrintSuccess({ { "action", "touch-swipe" }, { "displayId", swipe.displayId },
        { "srcX", swipe.srcX }, { "srcY", swipe.srcY }, { "dstX", swipe.dstX }, { "dstY", swipe.dstY } });
}
} // namespace OHOS::MMI::InputCli
