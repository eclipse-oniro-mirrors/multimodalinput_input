/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef PENDING_EVENT_COUNTER_H
#define PENDING_EVENT_COUNTER_H

#include <cstdint>
#include <memory>
#include <mutex>

namespace OHOS {
namespace MMI {

class PendingEventCounter final {
public:
    class Guard final {
    public:
        Guard(PendingEventCounter &counter);
        ~Guard();

        void Release();

        Guard(const Guard &) = delete;
        Guard &operator = (const Guard &) = delete;
    
    private:
        std::atomic<PendingEventCounter*> counter_;
    };

    PendingEventCounter() = default;
    ~PendingEventCounter() = default;

    bool HasPendingEvent();
    
    std::shared_ptr<Guard> CreateGuard();

private:
    void Record();
    void Remove();

    std::mutex pendingCountMutex_;
    uint32_t pendingCount_ { 0 };
};

} // namespace MMI
} // namespace OHOS

#endif // PENDING_EVENT_COUNTER_H
