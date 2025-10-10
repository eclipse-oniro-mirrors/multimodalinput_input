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

#ifndef MMI_I_INPUT_EVENT_HANDLER_MOCK_H
#define MMI_I_INPUT_EVENT_HANDLER_MOCK_H
#include "axis_event.h"
#include "key_event.h"
#include "pointer_event.h"
#include "switch_event.h"

namespace OHOS {
namespace MMI {
class IInputEventHandler {
public:
    struct IInputEventConsumer {
    public:
        IInputEventConsumer() = default;
        virtual ~IInputEventConsumer() = default;

        virtual void OnInputEvent(InputHandlerType type, std::shared_ptr<KeyEvent> event) const = 0;
        virtual void OnInputEvent(InputHandlerType type, std::shared_ptr<PointerEvent> event) const = 0;
        virtual void OnInputEvent(InputHandlerType type, std::shared_ptr<AxisEvent> event) const = 0;
    };

    IInputEventHandler() = default;
    virtual ~IInputEventHandler() = default;

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    virtual void HandleKeyEvent(const std::shared_ptr<KeyEvent> keyEvent) = 0;
#endif // OHOS_BUILD_ENABLE_KEYBOARD
#ifdef OHOS_BUILD_ENABLE_POINTER
    virtual void HandlePointerEvent(const std::shared_ptr<PointerEvent> pointerEvent) = 0;
#endif // OHOS_BUILD_ENABLE_POINTER
#ifdef OHOS_BUILD_ENABLE_TOUCH
    virtual void HandleTouchEvent(const std::shared_ptr<PointerEvent> pointerEvent) = 0;
#endif // OHOS_BUILD_ENABLE_TOUCH
#ifdef OHOS_BUILD_ENABLE_SWITCH
    virtual void HandleSwitchEvent(const std::shared_ptr<SwitchEvent> switchEvent)
    {
        if (nextHandler_ != nullptr) {
            nextHandler_->HandleSwitchEvent(switchEvent);
        }
    }
#endif // OHOS_BUILD_ENABLE_SWITCH

protected:
    std::shared_ptr<IInputEventHandler> nextHandler_ { nullptr };
};
} // namespace MMI
} // namespace OHOS
#endif // MMI_I_INPUT_EVENT_HANDLER_MOCK_H