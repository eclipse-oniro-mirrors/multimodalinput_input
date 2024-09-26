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
#include "mock.h"

#include "iremote_broker.h"
#include "i_anco_channel.h"

namespace OHOS {
using namespace OHOS::MMI;
bool PointerEvent::ReadFromParcel(Parcel &in)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadFromParcel(in);
}

bool KeyEvent::ReadFromParcel(Parcel &in)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadFromParcel(in);
}
} // namespace OHOS