/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INNER_POINTER_ITEM_H
#define INNER_POINTER_ITEM_H

#include "pointer_event.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {
struct InnerPointerItem {
    int32_t pointerId { -1 };
    bool pressed { false };
    int32_t displayX { -1 };
    int32_t displayY { -1 };
    int32_t windowX { -1 };
    int32_t windowY { -1 };
    double displayXPos { 0.0 };
    double displayYPos { 0.0 };
    double windowXPos { 0.0 };
    double windowYPos { 0.0 };
    int32_t width { -1 };
    int32_t height { -1 };
    double tiltX { 0.0 };
    double tiltY { 0.0 };
    int32_t toolDisplayX { -1 };
    int32_t toolDisplayY { -1 };
    int32_t toolWindowX { -1 };
    int32_t toolWindowY { -1 };
    int32_t toolWidth { -1 };
    int32_t toolHeight { -1 };
    double pressure { 0.0 };
    int32_t longAxis { -1 };
    int32_t shortAxis { -1 };
    int32_t deviceId { -1 };
    int64_t downTime { -1 };
    int32_t toolType { -1 };
    int32_t targetWindowId { -1 };
    int32_t originPointerId { -1 };
    int32_t rawDx { -1 };
    int32_t rawDy { -1 };

public:
    static void Transform(const MMI::PointerEvent::PointerItem &mmiItem, InnerPointerItem &innerItem);
    static void Transform(const InnerPointerItem &innerItem, MMI::PointerEvent::PointerItem &mmiItem);
};
} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // INNER_POINTER_ITEM_H
