/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef MESSAGE_PARCEL_MOCK_H
#define MESSAGE_PARCEL_MOCK_H
 
#include <gmock/gmock.h>
 
#include "libinput.h"
#include "mouse_transform_processor.h"
 
namespace OHOS {
namespace MMI {
class DfsMessageParcel {
public:
    virtual ~DfsMessageParcel() = default;
public:
    virtual uint32_t libinput_event_pointer_get_finger_count() = 0;
    virtual uint32_t libinput_event_pointer_get_button_area() = 0;
    virtual bool IsSceneBoardEnabled() = 0;
public:
    static inline std::shared_ptr<DfsMessageParcel> messageParcel = nullptr;
};

class MessageParcelMock : public DfsMessageParcel {
public:
    MOCK_METHOD0(libinput_event_pointer_get_finger_count, uint32_t());
    MOCK_METHOD0(libinput_event_pointer_get_button_area, uint32_t());
    MOCK_METHOD0(IsSceneBoardEnabled, bool());
};
} // namespace MMI
} // namespace OHOS
#endif