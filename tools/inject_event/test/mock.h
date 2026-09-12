/*
* Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef MOCK_H
#define MOCK_H

#include <vector>

#include <singleton.h>

#include "define_multimodal.h"
#include "input_manager.h"
#include "pixel_map.h"

namespace OHOS {
namespace MMI {
class MockHandler : public DelayedSingleton<MockHandler> {
public:
    bool mockIsPointerInitRet { false };
    int32_t mockGetCurrentCursorInfoRet { RET_ERR };
    bool mockVisible { false };
    int32_t mockPointerStyleId { 0 };
    int32_t mockGetUserDefinedCursorPixelMapRet { -1 };
    std::shared_ptr<Media::PixelMap> mockPixelMapPtr { nullptr };
    int32_t mockSimulateInputEventRet { RET_OK };
    int32_t simulateInputEventCallCount { 0 };
    std::shared_ptr<PointerEvent> lastSimulatedPointerEvent { nullptr };
    std::vector<std::shared_ptr<PointerEvent>> simulatedPointerEvents {};
};
} // namespace MMI
} // namespace OHOS
#define MOCKHANDLER OHOS::DelayedSingleton<OHOS::MMI::MockHandler>::GetInstance()
#endif