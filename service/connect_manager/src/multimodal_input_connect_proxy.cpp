/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "multimodal_input_connect_proxy.h"
#include "log.h"
#include "message_option.h"
#include "multimodal_input_connect_def_parcel.h"
#include "multimodal_input_connect_define.h"
#include "string_ex.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "MultimodalInputConnectProxy" };
}

// 获取其他设备注册的SA的Proxy
MultimodalInputConnectProxy::MultimodalInputConnectProxy(const sptr<IRemoteObject> &impl) :
    IRemoteProxy<IMultimodalInputConnect>(impl)
{
    MMI_LOGI("MultimodalInputConnectProxy()");
}

MultimodalInputConnectProxy::~MultimodalInputConnectProxy()
{
    MMI_LOGI("~MultimodalInputConnectProxy()");
}

int32_t MultimodalInputConnectProxy::AllocSocketFd(const std::string &programName,
    const int32_t moduleType, int32_t &socketFd)
{
    MMI_LOGD("enter");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option; // (MessageOption::TF_ASYNC);

    if (!data.WriteInterfaceToken(MultimodalInputConnectProxy::GetDescriptor())) {
        MMI_LOGE("Failed to write descriptor");
        return ERR_INVALID_VALUE;
    }

    ConnectDefReqParcel req;
    req.data.moduleId = moduleType;
    req.data.clientName = programName;
    if (!data.WriteParcelable(&req)) {
        MMI_LOGE("Failed to write programName");
        return ERR_INVALID_VALUE;
    }

    int32_t requestResult = Remote()->SendRequest(ALLOC_SOCKET_FD, data, reply, option);
    if (requestResult != NO_ERROR) {
        MMI_LOGE("send request fail, result:%{public}d", requestResult);
        return RET_ERR;
    }

    MMI_LOGE("have recieve message from server");

    int32_t result = reply.ReadInt32();
    MMI_LOGE("result:%{public}d", result);
    if (result != RET_OK) {
        MMI_LOGE("responce return error:%{public}d", result);
        return RET_ERR;
    }
    socketFd = reply.ReadFileDescriptor();
    MMI_LOGE("socketFd:%{public}d", socketFd);
    MMI_LOGD("leave");
    return RET_OK;
}

int32_t MultimodalInputConnectProxy::AddInputEventFilter(sptr<IEventFilter> filter)
{
    MMI_LOGD("enter");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(MultimodalInputConnectProxy::GetDescriptor())) {
        MMI_LOGE("Failed to write descriptor");
        return ERR_INVALID_VALUE;
    }

    if (!data.WriteRemoteObject(filter->AsObject().GetRefPtr())) {
        MMI_LOGE("Failed to write filter");
        return ERR_INVALID_VALUE;
    }

    int32_t requestResult = Remote()->SendRequest(SET_EVENT_POINTER_FILTER, data, reply, option);
    if (requestResult != NO_ERROR) {
        MMI_LOGE("send request fail, result:%{public}d", requestResult);
        return RET_ERR;
    }

    int32_t result = reply.ReadInt32();
    if (result != RET_OK) {
        MMI_LOGE("responce return error:%{public}d", result);
    }
    MMI_LOGD("leave");
    return result;
}
} // namespace MMI
} // namespace OHOS
