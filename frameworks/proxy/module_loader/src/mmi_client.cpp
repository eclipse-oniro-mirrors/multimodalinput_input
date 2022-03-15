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

#include "mmi_client.h"

#include "mmi_log.h"
#include "multimodal_event_handler.h"
#include "multimodal_input_connect_manager.h"
#include "proto.h"
#include "util.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "MMIClient" };
} // namespace

MMIClient::MMIClient() {}

MMIClient::~MMIClient()
{
    CALL_LOG_ENTER;
}

bool MMIClient::SendMessage(const NetPacket &pkt) const
{
    return SendMsg(pkt);
}

bool MMIClient::GetCurrentConnectedStatus() const
{
    return GetConnectedStatus();
}

bool MMIClient::Start(IClientMsgHandlerPtr msgHdl, bool detachMode)
{
    CALL_LOG_ENTER;
    CHKPF(msgHdl);
    EventManager.SetClientHandle(GetPtr());
    if (!(msgHdl->Init())) {
        MMI_LOGE("Message processing initialization failed");
        return false;
    }
    auto callback = msgHdl->GetCallback();
    CHKPF(callback);
    if (!(StartClient(callback, detachMode))) {
        MMI_LOGE("Client startup failed");
        return false;
    }
    return true;
}

void MMIClient::RegisterConnectedFunction(ConnectCallback fun)
{
    funConnected_ = fun;
}

void MMIClient::RegisterDisconnectedFunction(ConnectCallback fun)
{
    funDisconnected_ = fun;
}

void MMIClient::VirtualKeyIn(RawInputEvent virtualKeyEvent)
{
    NetPacket pkt(MmiMessageId::ON_VIRTUAL_KEY);
    pkt << virtualKeyEvent;
    SendMsg(pkt);
}

void MMIClient::ReplyMessageToServer(MmiMessageId idMsg, int64_t clientTime, int64_t endTime) const
{
    NetPacket pkt(MmiMessageId::CHECK_REPLY_MESSAGE);
    pkt << idMsg << clientTime << endTime;
    SendMsg(pkt);
}

void MMIClient::SdkGetMultimodeInputInfo()
{
    TagPackHead tagPackHead = {MmiMessageId::GET_MMI_INFO_REQ, {0}};
    NetPacket pkt(MmiMessageId::GET_MMI_INFO_REQ);
    pkt << tagPackHead;
    SendMsg(pkt);
}

void MMIClient::OnDisconnected()
{
    MMI_LOGD("Disconnected from server, fd:%{public}d", GetFd());
    if (funDisconnected_) {
        funDisconnected_(*this);
    }
    isConnected_ = false;
}

void MMIClient::OnConnected()
{
    MMI_LOGD("Connection to server succeeded, fd:%{public}d", GetFd());
    if (funConnected_) {
        funConnected_(*this);
    }
    isConnected_ = true;
}

int32_t MMIClient::Socket()
{
    CALL_LOG_ENTER;
    int32_t ret = MultimodalInputConnectManager::GetInstance()->
                        AllocSocketPair(IMultimodalInputConnect::CONNECT_MODULE_TYPE_MMI_CLIENT);
    if (ret != RET_OK) {
        MMI_LOGE("UDSSocket::Socket, call MultimodalInputConnectManager::AllocSocketPair return %{public}d", ret);
        return -1;
    }
    fd_ = MultimodalInputConnectManager::GetInstance()->GetClientSocketFdOfAllocedSocketPair();
    if (fd_ == IMultimodalInputConnect::INVALID_SOCKET_FD) {
        MMI_LOGE("UDSSocket::Socket, call MultimodalInputConnectManager::GetClientSocketFdOfAllocedSocketPair"
                 " return invalid fd");
    } else {
        MMI_LOGD("UDSSocket::Socket, call MultimodalInputConnectManager::GetClientSocketFdOfAllocedSocketPair"
                 " return fd:%{public}d", fd_);
    }

    return fd_;
}
} // namespace MMI
} // namespace OHOS
