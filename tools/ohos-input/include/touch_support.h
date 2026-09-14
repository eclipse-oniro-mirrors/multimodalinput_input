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

#ifndef OHOS_INPUT_TOUCH_SUPPORT_H
#define OHOS_INPUT_TOUCH_SUPPORT_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "mouse_drag_command.h"
#include "option_parser.h"
#include "touch_controller_impl.h"

namespace OHOS::MMI::InputCli {
inline constexpr int32_t DEFAULT_TOUCH_TAP_DURATION_MS = 100;
inline constexpr int32_t DEFAULT_TOUCH_SWIPE_DURATION_MS = 500;
inline constexpr int32_t MAX_TOUCH_DURATION_MS = 5000;
inline constexpr int32_t TOUCH_ID = 0;

struct TouchPointOptions {
    int32_t displayId { 0 };
    int32_t x { 0 };
    int32_t y { 0 };
    int32_t duration { 0 };
};

struct TouchSwipeOptions {
    int32_t displayId { 0 };
    int32_t srcX { 0 };
    int32_t srcY { 0 };
    int32_t dstX { 0 };
    int32_t dstY { 0 };
    int32_t duration { 0 };
};

struct TouchSession {
    std::shared_ptr<TouchControllerImpl> touch;
};

bool ParseTouchPoint(const Options &options, TouchPointOptions &result, std::string &error);
bool ParseTouchSwipe(const Options &options, TouchSwipeOptions &result, std::string &error);
int32_t CreateTouchController(TouchSession &session);
std::vector<DragStep> BuildSwipeSteps(int32_t duration, int32_t srcX, int32_t srcY, int32_t dstX, int32_t dstY);
int32_t RunTouchTap(const TouchSession &session, const TouchPointOptions &options);
int32_t RunTouchSwipe(const TouchSession &session, const TouchSwipeOptions &options);
} // namespace OHOS::MMI::InputCli

#endif
