/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "event_dispatch_handler.h"

#include "util_ex.h"

namespace OHOS {
namespace MMI {

namespace {
class AssetCheckerStub : public AssetChecker {
public:
    bool HandleEvent(const std::shared_ptr<InputEvent>& event) override
    {
        (void)event;
        return false;
    }

    void Dump(int32_t fd) override
    {
        mprintf(fd, "AssetChecker: not enabled (stub)\n");
    }
};
} // namespace

std::unique_ptr<AssetChecker> AssetChecker::Create()
{
    return std::make_unique<AssetCheckerStub>();
}

} // namespace MMI
} // namespace OHOS
