/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "stubgetpointersnapshot_fuzzer.h"

#include "singleton.h"

#include "mmi_service.h"
#include "multimodal_input_connect_stub.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "StubGetPointerSnapshotFuzzTest"

namespace OHOS {
namespace MMI {
const std::u16string FORMMGR_INTERFACE_TOKEN { u"ohos.multimodalinput.IConnectManager" };

#ifdef OHOS_BUILD_ENABLE_MAGICCURSOR
bool StubGetPointerSnapshotFuzzTest(const uint8_t *data, size_t size)
{
    MessageParcel datas;
    if (!datas.WriteInterfaceToken(FORMMGR_INTERFACE_TOKEN) ||
        !datas.WriteBuffer(data, size) || !datas.RewindRead(0)) {
        return false;
    }
    MessageParcel reply;
    MessageOption option;
    MMIService::GetInstance()->state_ = ServiceRunningState::STATE_RUNNING;
    MMIService::GetInstance()->OnRemoteRequest(
        static_cast<uint32_t>(MultimodalinputConnectInterfaceCode::GET_POINTER_SNAPSHOT), datas, reply, option);
    return true;
}
#endif // OHOS_BUILD_ENABLE_MAGICCURSOR
} // MMI
} // OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }

#ifdef OHOS_BUILD_ENABLE_MAGICCURSOR
    OHOS::MMI::StubGetPointerSnapshotFuzzTest(data, size);
#endif // OHOS_BUILD_ENABLE_MAGICCURSOR
    return 0;
}