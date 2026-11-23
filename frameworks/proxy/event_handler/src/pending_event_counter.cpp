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

#include "pending_event_counter.h"

#include <unistd.h>
#include <memory>
#include <mutex>

#include "define_multimodal.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "PendingEventCounter"

namespace OHOS {
namespace MMI {

PendingEventCounter::Guard::Guard(PendingEventCounter &counter) : counter_(&counter)
{
    counter.Record();
}

PendingEventCounter::Guard::~Guard()
{
    Release();
}

void PendingEventCounter::Guard::Release()
{
    PendingEventCounter* counter = counter_.exchange(nullptr);
    if (counter != nullptr) {
        counter_->Remove();
    }
}

bool PendingEventCounter::HasPendingEvent()
{
    std::lock_guard<std::mutex> guard(pendingCountMutex_);
    MMI_HILOGD("pid:%{public}d, haspending:%{public}d, count:%{public}u",
        getpid(), pendingCount_ != 0, pendingCount_);
    return pendingCount_ != 0;
}

std::shared_ptr<PendingEventCounter::Guard> PendingEventCounter::CreateGuard()
{
    return std::make_shared<Guard>(*this);
}

void PendingEventCounter::Record()
{
    std::lock_guard<std::mutex> guard(pendingCountMutex_);
    ++pendingCount_;
    MMI_HILOGD("Record pending count, pid:%{public}d, count:%{public}u",
        getpid(), pendingCount_);
}

void PendingEventCounter::Remove()
{
    std::lock_guard<std::mutex> guard(pendingCountMutex_);
    if (pendingCount_ > 0) {
        --pendingCount_;
    }
    MMI_HILOGD("Remove pending count, pid:%{public}d, count:%{public}u",
        getpid(), pendingCount_);
}

} // namespace MMI
} // namespace OHOS
