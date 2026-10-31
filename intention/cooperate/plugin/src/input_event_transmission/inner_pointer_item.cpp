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

#include "input_event_transmission/inner_pointer_item.h"

#undef LOG_TAG
#define LOG_TAG "InnerPointerItem"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {

void InnerPointerItem::Transform(const MMI::PointerEvent::PointerItem &mmiItem, InnerPointerItem &innerItem)
{
    innerItem.pointerId = mmiItem.GetPointerId();
    innerItem.pressed = mmiItem.IsPressed();
    innerItem.displayX = mmiItem.GetDisplayX();
    innerItem.displayY = mmiItem.GetDisplayY();
    innerItem.windowX = mmiItem.GetWindowX();
    innerItem.windowY = mmiItem.GetWindowY();
    innerItem.displayXPos = mmiItem.GetDisplayXPos();
    innerItem.displayYPos = mmiItem.GetDisplayYPos();
    innerItem.windowXPos = mmiItem.GetWindowXPos();
    innerItem.windowYPos = mmiItem.GetWindowYPos();
    innerItem.width = mmiItem.GetWidth();
    innerItem.height = mmiItem.GetHeight();
    innerItem.tiltX = mmiItem.GetTiltX();
    innerItem.tiltY = mmiItem.GetTiltY();
    innerItem.toolDisplayX = mmiItem.GetToolDisplayX();
    innerItem.toolDisplayY = mmiItem.GetToolDisplayY();
    innerItem.toolWindowX = mmiItem.GetToolWindowX();
    innerItem.toolWindowY = mmiItem.GetToolWindowY();
    innerItem.toolWidth = mmiItem.GetToolWidth();
    innerItem.toolHeight = mmiItem.GetToolHeight();
    innerItem.pressure = mmiItem.GetPressure();
    innerItem.longAxis = mmiItem.GetLongAxis();
    innerItem.shortAxis = mmiItem.GetShortAxis();
    innerItem.deviceId = mmiItem.GetDeviceId();
    innerItem.downTime = mmiItem.GetDownTime();
    innerItem.toolType = mmiItem.GetToolType();
    innerItem.targetWindowId = mmiItem.GetTargetWindowId();
    innerItem.originPointerId = mmiItem.GetOriginPointerId();
    innerItem.rawDx = mmiItem.GetRawDx();
    innerItem.rawDy = mmiItem.GetRawDy();
}

void InnerPointerItem::Transform(const InnerPointerItem &innerItem, MMI::PointerEvent::PointerItem &mmiItem)
{
    mmiItem.SetPointerId(innerItem.pointerId);
    mmiItem.SetPressed(innerItem.pressed);
    mmiItem.SetDisplayX(innerItem.displayX);
    mmiItem.SetDisplayY(innerItem.displayY);
    mmiItem.SetWindowX(innerItem.windowX);
    mmiItem.SetWindowY(innerItem.windowY);
    mmiItem.SetDisplayXPos(innerItem.displayXPos);
    mmiItem.SetDisplayYPos(innerItem.displayYPos);
    mmiItem.SetWindowXPos(innerItem.windowXPos);
    mmiItem.SetWindowYPos(innerItem.windowYPos);
    mmiItem.SetWidth(innerItem.width);
    mmiItem.SetHeight(innerItem.height);
    mmiItem.SetTiltX(innerItem.tiltX);
    mmiItem.SetTiltY(innerItem.tiltY);
    mmiItem.SetToolDisplayX(innerItem.toolDisplayX);
    mmiItem.SetToolDisplayY(innerItem.toolDisplayY);
    mmiItem.SetToolWindowX(innerItem.toolWindowX);
    mmiItem.SetToolWindowY(innerItem.toolWindowY);
    mmiItem.SetToolWidth(innerItem.toolWidth);
    mmiItem.SetToolHeight(innerItem.toolHeight);
    mmiItem.SetPressure(innerItem.pressure);
    mmiItem.SetLongAxis(innerItem.longAxis);
    mmiItem.SetShortAxis(innerItem.shortAxis);
    mmiItem.SetDeviceId(innerItem.deviceId);
    mmiItem.SetDownTime(innerItem.downTime);
    mmiItem.SetToolType(innerItem.toolType);
    mmiItem.SetTargetWindowId(innerItem.targetWindowId);
    mmiItem.SetOriginPointerId(innerItem.originPointerId);
    mmiItem.SetRawDx(innerItem.rawDx);
    mmiItem.SetRawDy(innerItem.rawDy);
}

} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
