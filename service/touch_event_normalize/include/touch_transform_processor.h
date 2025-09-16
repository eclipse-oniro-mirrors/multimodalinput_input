/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef TOUCH_TRANSFORM_PROCESSOR_H
#define TOUCH_TRANSFORM_PROCESSOR_H

#include "aggregator.h"
#include "fingersense_manager.h"
#include "timer_manager.h"
#include "transform_processor.h"

namespace OHOS {
namespace MMI {
class TouchTransformProcessor final : public TransformProcessor {
public:
    explicit TouchTransformProcessor(int32_t deviceId);
    DISALLOW_COPY_AND_MOVE(TouchTransformProcessor);
    ~TouchTransformProcessor() = default;
    std::shared_ptr<PointerEvent> OnEvent(struct libinput_event *event) override;
    std::shared_ptr<PointerEvent> GetPointerEvent() override { return nullptr; }
    void OnDeviceRemoved() override;

private:
    bool OnEventTouchDown(struct libinput_event *event);
    bool OnEventTouchMotion(struct libinput_event *event);
    bool OnEventTouchUp(struct libinput_event *event);
    bool OnEventTouchCancel(struct libinput_event *event);
    int32_t GetTouchToolType(struct libinput_event_touch *data, struct libinput_device *device);
    int32_t GetTouchToolType(struct libinput_device *device);
    void TransformTouchProperties(TouchType &rawTouch, PointerEvent::PointerItem &pointerItem);
    void NotifyFingersenseProcess(PointerEvent::PointerItem &pointerItem, int32_t &toolType);
    void UpdatePointerItemByTouchInfo(PointerEvent::PointerItem &item, EventTouch &touchInfo);
    void InitToolTypes();
    bool DumpInner();
#ifdef OHOS_BUILD_EXTERNAL_SCREEN
    void AddInvalidAreaDownedEvent(int32_t seatSlot);
    void RemoveInvalidAreaDownedEvent(int32_t seatSlot);
    bool isInvalidAreaDownedEvent(int32_t seatSlot);
#endif // OHOS_BUILD_EXTERNAL_SCREEN
private:
    const int32_t deviceId_ { -1 };
    int32_t processedCount_ { 0 };
    std::shared_ptr<PointerEvent> pointerEvent_ { nullptr };
    std::vector<std::pair<int32_t, int32_t>> vecToolType_;
    std::unordered_map<int32_t, bool> pointerItemCancelMarks_;
    Aggregator aggregator_ {
            [](int32_t intervalMs, int32_t repeatCount, std::function<void()> callback) -> int32_t {
                return TimerMgr->AddTimer(intervalMs, repeatCount, std::move(callback),
                    "TouchTransformProcessor-Aggregator");
            },
            [](int32_t timerId) -> int32_t
            {
                return TimerMgr->ResetTimer(timerId);
            },
            [](int32_t timerId) -> int32_t
            {
                return TimerMgr->RemoveTimer(timerId);
            }
    };
#ifdef OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER
    TouchType rawTouch_;
#endif // OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER
#ifdef OHOS_BUILD_EXTERNAL_SCREEN
    std::list<int32_t> InvalidAreaDownedEvents_;
#endif // OHOS_BUILD_EXTERNAL_SCREEN
};
} // namespace MMI
} // namespace OHOS
#endif // TOUCH_TRANSFORM_PROCESSOR_H