/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef MMI_EVENT_DISPATCH_HANDLER_MOCK_H
#define MMI_EVENT_DISPATCH_HANDLER_MOCK_H

#include "gmock/gmock.h"
#include "i_input_event_handler.h"

namespace OHOS {
namespace MMI {
class IEventDispatchHandler : public IInputEventHandler {
public:
    IEventDispatchHandler() = default;
    virtual ~IEventDispatchHandler() = default;
};

class EventDispatchHandler : public IEventDispatchHandler {
public:
    EventDispatchHandler() = default;
    virtual ~EventDispatchHandler() override = default;

    MOCK_METHOD(void, HandleKeyEvent, (const std::shared_ptr<KeyEvent>));
    MOCK_METHOD(void, HandlePointerEvent, (const std::shared_ptr<PointerEvent>));
    MOCK_METHOD(void, HandleTouchEvent, (const std::shared_ptr<PointerEvent>));
    MOCK_METHOD(int32_t, DispatchRawTouchpadPointerEvent, (std::shared_ptr<PointerEvent>, int32_t));
};
} // namespace MMI
} // namespace OHOS
#endif // MMI_EVENT_DISPATCH_HANDLER_MOCK_H