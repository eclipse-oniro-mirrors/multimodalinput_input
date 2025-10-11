/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "mock.h"
#include "define_multimodal.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "OHInputManagerMock"

namespace OHOS {
namespace MMI {

int32_t InputManager::GetPointerLocation(int32_t &displayId, double &displayX, double &displayY)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return RET_ERR;
    }
    return DfsMessageParcel::messageParcel->GetPointerLocation(displayId, displayX, displayY);
}

int32_t InputManager::GetCurrentCursorInfo(bool& visible, PointerStyle& pointerStyle)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return RET_ERR;
    }
    return DfsMessageParcel::messageParcel->GetCurrentCursorInfo(visible, pointerStyle);
}

int32_t InputManager::GetUserDefinedCursorPixelMap(void *pixelMapPtr)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return RET_ERR;
    }
    return DfsMessageParcel::messageParcel->GetUserDefinedCursorPixelMap(pixelMapPtr);
}
} // namespace MMI
} // namespace OHOS