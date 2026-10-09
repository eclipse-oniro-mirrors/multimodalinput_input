/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "drag_security_manager.h"
#include <algorithm>
#include <chrono>
#include "mmi_log.h"
#include "define_multimodal.h"
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "DragSecurityManager"

namespace OHOS {
namespace MMI {
namespace {
    const int32_t MAX_BASE64_LENGTH { 1024 };
    const int32_t SECURITY_DATA_LEN { 17 };
}

DragSecurityManager& DragSecurityManager::GetInstance()
{
    static DragSecurityManager instance;
    return instance;
}

DragSecurityManager::DragSecurityManager() = default;

uint64_t DragSecurityManager::GetCurrentTimesTampMs() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

int32_t DragSecurityManager::DeliverNonce(const std::string& nonce)
{
    CALL_DEBUG_ENTER;
    if (nonce.empty()) {
        MMI_HILOGE("nonce is empty");
        return RET_ERR;
    }
    nonce_ = nonce;
    return RET_OK;
}

std::string DragSecurityManager::GetNonce()
{
    return nonce_;
}

void DragSecurityManager::ResetNonce()
{
    nonce_ = "";
}

#ifdef OHOS_BUILD_ENABLE_DRAG_SECURITY
void DragSecurityManager::DragSecurityUpdatePointerEvent(std::shared_ptr<PointerEvent> pointerEvent)
{
    CALL_DEBUG_ENTER;
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    uint64_t distributeEventTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    pointerEvent->SetDistributeEventTime(distributeEventTime);
    std::vector<uint8_t> nonceBin = Base64Decode(nonce_);
    std::string signature;
    PointerEvent::PointerItem pointerItem {};
    if (pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
        DragSecurityData dragEventData = {
            .timestampMs = distributeEventTime,
            .coordinateX = pointerItem.GetGlobalX(),
            .coordinateY = pointerItem.GetGlobalY(),
        };
        signature = Base64Encode(GenerateSignature(nonceBin, dragEventData));
    }
    if (signature.empty()) {
        MMI_HILOGE("Signature is empty");
        return;
    }
    pointerEvent->SetSignature(signature);
    ResetNonce();
}

std::vector<uint8_t> DragSecurityManager::GenerateSignature(const std::vector<uint8_t>& nonce,
    const DragSecurityData& dragSecurityData)
{
    CALL_DEBUG_ENTER;
    if (nonce.empty()) {
        MMI_HILOGE("Empty nonce provided");
        return {};
    }
    auto dataStr = SerializeDragEventData(dragSecurityData);
    if (dataStr.empty()) {
        MMI_HILOGE("Failed to serialize event data");
        return {};
    }
    CRYPT_EAL_MacCtx *ctx = CRYPT_EAL_MacNewCtx(CRYPT_MAC_HMAC_SHA384);
    if (ctx == nullptr) {
        MMI_HILOGE("CRYPT_EAL_MacNewCtx failed");
        return {};
    }
    constexpr uint32_t HMAC_SHA384_DIGEST_LEN = 48;
    uint8_t digest[HMAC_SHA384_DIGEST_LEN] = {0};
    uint32_t len = sizeof(digest);
    int32_t ret = CRYPT_EAL_MacInit(ctx, nonce.data(), static_cast<uint32_t>(nonce.size()));
    if (ret == CRYPT_SUCCESS) {
        ret = CRYPT_EAL_MacUpdate(ctx, reinterpret_cast<const uint8_t*>(dataStr.data()),
            static_cast<uint32_t>(dataStr.size()));
    }
    if (ret == CRYPT_SUCCESS) {
        ret = CRYPT_EAL_MacFinal(ctx, digest, &len);
    }
    CRYPT_EAL_MacFreeCtx(ctx);
    if (ret != CRYPT_SUCCESS) {
        MMI_HILOGE("HMAC-SHA384 calculation failed, ret:%{public}d", ret);
        return {};
    }
    return std::vector<uint8_t>(digest, digest + len);
}

std::string DragSecurityManager::SerializeDragEventData(const DragSecurityData& data)
{
    std::ostringstream oss;
    oss.precision(SECURITY_DATA_LEN);
    oss << data.timestampMs << '|' << data.coordinateX << '|' << data.coordinateY;
    return oss.str();
}

std::string DragSecurityManager::Base64Encode(const uint8_t* data, size_t len)
{
    if (!data || (len == 0)) {
        MMI_HILOGE("data or len error");
        return "";
    }
    // CHECK Base64encode formula for calculating length, BSL_BASE64_Encode uses uint32_t length
    if (len > static_cast<size_t>((UINT32_MAX - 4) / 4 * 3)) {
        MMI_HILOGE("Len too large");
        return "";
    }
    // Base64encode formula for calculating length, openhitls needs one more byte than encoded length
    uint32_t outLen = static_cast<uint32_t>(4 * ((len + 2) / 3)) + 1;
    std::vector<char> outBuf(outLen);
    if (BSL_BASE64_Encode(data, static_cast<uint32_t>(len), outBuf.data(), &outLen) != BSL_SUCCESS) {
        MMI_HILOGE("BSL_BASE64_Encode failed");
        return "";
    }
    return std::string(outBuf.data(), outLen);
}
 
std::string DragSecurityManager::Base64Encode(const std::vector<uint8_t>& vec)
{
    return Base64Encode(vec.data(), vec.size());
}
 
std::vector<uint8_t> DragSecurityManager::Base64Decode(const std::string& b64)
{
    if (b64.empty() || (b64.length() > MAX_BASE64_LENGTH)) {
        MMI_HILOGE("B64 is empty or too long");
        return {};
    }
    
    // Base64decode formula for calculating length, openhitls requires dst buffer >= (srcLen+3)/4*3
    uint32_t srcLen = static_cast<uint32_t>(b64.size());
    uint32_t outLen = (srcLen + 3) / 4 * 3;
    std::vector<uint8_t> result(outLen);
    if (BSL_BASE64_Decode(b64.data(), srcLen, result.data(), &outLen) != BSL_SUCCESS) {
        MMI_HILOGE("BSL_BASE64_Decode failed");
        return {};
    }
    result.resize(outLen);
    return result;
}
#endif // OHOS_BUILD_ENABLE_DRAG_SECURITY
} // namespace MMI
} // namespace OHOS