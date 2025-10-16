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

#ifndef EXPIRATION_CHECKER_H
#define EXPIRATION_CHECKER_H

#include <queue>
#include <shared_mutex>

#include "input_event.h"

namespace OHOS {
namespace MMI {
class ExpirationChecker {
public:
    bool CheckExpiration(int32_t eventId);
    bool CheckValid(const std::shared_ptr<InputEvent> event);
    void UpdateInputEvent(const std::shared_ptr<InputEvent> event);

private:
    void RemoveExpiredEvent();

private:
    struct StashEvent {
        long long timeStampRcvd { 0 };
        int32_t eventId { -1 };
        size_t hashCode { 0 };
    };
    std::deque<StashEvent> stashEvents_; // dispatched events
    std::shared_mutex rwMutex_;
};
} // namespace MMI
} // namespace OHOS
#endif // EXPIRATION_CHECKER_H