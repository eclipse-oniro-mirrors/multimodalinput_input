/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#ifndef MMI_FD_LISTENER_H
#define MMI_FD_LISTENER_H
#include "file_descriptor_listener.h"

namespace OHOS {
namespace MMI {
class MMIFdListener : public AppExecFwk::FileDescriptorListener
{
public:
    MMIFdListener();
    virtual ~MMIFdListener();
    DISALLOW_COPY_AND_MOVE(MMIFdListener);

    virtual void OnReadable(int32_t fd) override;
    virtual void OnWritable(int32_t fd) override;
    virtual void OnShutdown(int32_t fd) override;
    virtual void OnException(int32_t fd) override;
};
} // namespace MMI
} // namespace OHOS
#endif // MMI_FD_LISTENER_H
