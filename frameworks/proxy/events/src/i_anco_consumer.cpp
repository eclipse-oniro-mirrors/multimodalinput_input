/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "i_anco_consumer.h"

#include "mmi_log.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "IAncoConsumer"

namespace OHOS {
namespace MMI {
namespace {
constexpr uint64_t MAX_UNMARSHAL_VECTOR_SIZE { 512 };
}

template<typename T>
static bool MarshalVector(const std::vector<T> &data, Parcel &parcel, bool (*writeOne)(const T &arg, Parcel &parcel))
{
    if (writeOne == nullptr) {
        return false;
    }
    uint64_t nItems = static_cast<uint64_t>(data.size());
    if (!parcel.WriteUint64(nItems)) {
        return false;
    }
    for (uint64_t index = 0; index < nItems; ++index) {
        if (!(*writeOne)(data[index], parcel)) {
            return false;
        }
    }
    return true;
}

template<typename T>
static bool UnmarshalVector(Parcel &parcel, std::vector<T> &data, bool (*readOne)(Parcel &parcel, T &arg))
{
    if (readOne == nullptr) {
        return false;
    }
    uint64_t nItems {};
    if (!parcel.ReadUint64(nItems)) {
        return false;
    }
    if (nItems > MAX_UNMARSHAL_VECTOR_SIZE) {
        MMI_HILOGE("The nItems:%{public}" PRIu64 ", exceeds maximum allowed size:%{public}"
            PRIu64, nItems, MAX_UNMARSHAL_VECTOR_SIZE);
        return false;
    }
    data.resize(nItems);

    for (uint64_t index = 0; index < nItems; ++index) {
        if (!(*readOne)(parcel, data[index])) {
            return false;
        }
    }
    return true;
}

static bool MarshalRect(const Rect &rect, Parcel &parcel)
{
    return (
        parcel.WriteInt32(rect.x) &&
        parcel.WriteInt32(rect.y) &&
        parcel.WriteInt32(rect.width) &&
        parcel.WriteInt32(rect.height)
    );
}

static bool UnmarshalRect(Parcel &parcel, Rect &rect)
{
    return (
        parcel.ReadInt32(rect.x) &&
        parcel.ReadInt32(rect.y) &&
        parcel.ReadInt32(rect.width) &&
        parcel.ReadInt32(rect.height)
    );
}

static bool MarshalWindowInfo(const AncoWindowInfo &windowInfo, Parcel &parcel)
{
    return (
        parcel.WriteInt32(windowInfo.id) &&
        parcel.WriteUint32(windowInfo.flags) &&
        parcel.WriteUint32(static_cast<uint32_t>(windowInfo.action)) &&
        parcel.WriteInt32(windowInfo.displayId) &&
        parcel.WriteFloat(windowInfo.zOrder) &&
        parcel.WriteFloatVector(windowInfo.transform) &&
        MarshalVector(windowInfo.defaultHotAreas, parcel, &MarshalRect) &&
        MarshalVector(windowInfo.ancoExcludedAreas, parcel, &MarshalRect)
    );
}

static bool UnmarshalWindowInfo(Parcel &parcel, AncoWindowInfo &windowInfo)
{
    uint32_t action {};

    bool result = (
        parcel.ReadInt32(windowInfo.id) &&
        parcel.ReadUint32(windowInfo.flags) &&
        parcel.ReadUint32(action) &&
        parcel.ReadInt32(windowInfo.displayId) &&
        parcel.ReadFloat(windowInfo.zOrder) &&
        parcel.ReadFloatVector(&windowInfo.transform) &&
        UnmarshalVector(parcel, windowInfo.defaultHotAreas, &UnmarshalRect) &&
        UnmarshalVector(parcel, windowInfo.ancoExcludedAreas, &UnmarshalRect)
    );
    windowInfo.action = static_cast<WINDOW_UPDATE_ACTION>(action);
    return result;
}

bool AncoWindows::Marshalling(Parcel &out) const
{
    return (
        out.WriteUint32(static_cast<uint32_t>(updateType)) &&
        out.WriteInt32(focusWindowId) &&
        MarshalVector(windows, out, &MarshalWindowInfo)
    );
}

bool AncoWindows::ReadFromParcel(Parcel &in)
{
    uint32_t updateTypeData {};
    bool result = (
        in.ReadUint32(updateTypeData) &&
        in.ReadInt32(focusWindowId) &&
        UnmarshalVector(in, windows, &UnmarshalWindowInfo)
    );
    updateType = static_cast<ANCO_WINDOW_UPDATE_TYPE>(updateTypeData);
    return result;
}

AncoWindows *AncoWindows::Unmarshalling(Parcel &in)
{
    AncoWindows *data = new (std::nothrow) AncoWindows();
    if (data && !data->ReadFromParcel(in)) {
        delete data;
        data = nullptr;
    }
    return data;
}

bool AncoOneHandData::Marshalling(Parcel &parcel) const
{
    return (parcel.WriteInt32(oneHandX) && parcel.WriteInt32(oneHandY) &&
            parcel.WriteInt32(expandHeight) && parcel.WriteInt32(scalePercent));
}

bool AncoOneHandData::ReadFromParcel(Parcel &parcel)
{
    return (parcel.ReadInt32(oneHandX) && parcel.ReadInt32(oneHandY) &&
            parcel.ReadInt32(expandHeight) && parcel.ReadInt32(scalePercent));
}

AncoOneHandData *AncoOneHandData::Unmarshalling(Parcel &parcel)
{
    AncoOneHandData *data = new (std::nothrow) AncoOneHandData();
    if (data && !data->ReadFromParcel(parcel)) {
        delete data;
        data = nullptr;
    }
    return data;
}

bool AncoExcludedKeyEventWindow::Marshalling(Parcel &parcel) const
{
    return parcel.WriteInt32Vector(windowIds);
}

bool AncoExcludedKeyEventWindow::ReadFromParcel(Parcel &parcel)
{
    return parcel.ReadInt32Vector(&windowIds);
}

AncoExcludedKeyEventWindow *AncoExcludedKeyEventWindow::Unmarshalling(Parcel &parcel)
{
    AncoExcludedKeyEventWindow *data = new (std::nothrow) AncoExcludedKeyEventWindow();
    if (data && !data->ReadFromParcel(parcel)) {
        delete data;
        data = nullptr;
    }
    return data;
}

bool AncoTripleFingerSnapshotState::Marshalling(Parcel &parcel) const
{
    return parcel.WriteBool(enabled);
}
 
bool AncoTripleFingerSnapshotState::ReadFromParcel(Parcel &parcel)
{
    return parcel.ReadBool(enabled);
}
 
AncoTripleFingerSnapshotState *AncoTripleFingerSnapshotState::Unmarshalling(Parcel &parcel)
{
    AncoTripleFingerSnapshotState *data = new (std::nothrow) AncoTripleFingerSnapshotState();
    if (data && !data->ReadFromParcel(parcel)) {
        delete data;
        data = nullptr;
    }
    return data;
}

bool CustomMessage::Marshalling(Parcel &parcel) const
{
    if (payload.size() > MAX_CUSTOM_MESSAGE_PAYLOAD_SIZE) {
        MMI_HILOGE("CustomMessage marshal failed, msgType:%{public}u, payloadSize:%{public}zu"
            " exceeds limit:%{public}u", msgType, payload.size(), MAX_CUSTOM_MESSAGE_PAYLOAD_SIZE);
        return false;
    }
    if (!parcel.WriteUint32(msgType)) {
        MMI_HILOGE("Failed to write CustomMessage msgType:%{public}u", msgType);
        return false;
    }
    if (!parcel.WriteUint32(static_cast<uint32_t>(payload.size()))) {
        MMI_HILOGE("Failed to write CustomMessage payloadSize:%{public}u, msgType:%{public}u",
            static_cast<uint32_t>(payload.size()), msgType);
        return false;
    }
    if (payload.empty()) {
        MMI_HILOGD("CustomMessage payload is empty, msgType:%{public}u", msgType);
        return true;
    }
    if (!parcel.WriteBuffer(payload.data(), payload.size())) {
        MMI_HILOGE("Failed to write CustomMessage payload, msgType:%{public}u, payloadSize:%{public}zu",
            msgType, payload.size());
        return false;
    }
    return true;
}

bool CustomMessage::ReadFromParcel(Parcel &parcel)
{
    if (!parcel.ReadUint32(msgType)) {
        MMI_HILOGE("Failed to read CustomMessage msgType");
        return false;
    }
    uint32_t payloadSize {};
    if (!parcel.ReadUint32(payloadSize)) {
        MMI_HILOGE("Failed to read CustomMessage payloadSize, msgType:%{public}u", msgType);
        return false;
    }
    if (payloadSize == 0) {
        MMI_HILOGD("CustomMessage payloadSize is zero, msgType:%{public}u", msgType);
        payload.clear();
        return true;
    }
    if (payloadSize > MAX_CUSTOM_MESSAGE_PAYLOAD_SIZE) {
        MMI_HILOGE("CustomMessage payloadSize:%{public}u exceeds limit:%{public}u, msgType:%{public}u",
            payloadSize, MAX_CUSTOM_MESSAGE_PAYLOAD_SIZE, msgType);
        return false;
    }
    const uint8_t *buffer = parcel.ReadBuffer(payloadSize);
    if (buffer == nullptr) {
        MMI_HILOGE("Failed to read CustomMessage payload, msgType:%{public}u, payloadSize:%{public}u",
            msgType, payloadSize);
        return false;
    }
    payload.assign(buffer, buffer + payloadSize);
    return true;
}

bool AncoDisableState::Marshalling(std::vector<uint8_t> &payload) const
{
    Parcel parcel;
    if (!parcel.WriteBool(disable)) {
        MMI_HILOGE("Failed to write AncoDisableState, disable:%{public}d", disable);
        return false;
    }
    const uint8_t *data = reinterpret_cast<const uint8_t *>(parcel.GetData());
    payload.assign(data, data + parcel.GetDataSize());
    return true;
}

CustomMessage *CustomMessage::Unmarshalling(Parcel &parcel)
{
    CustomMessage *data = new (std::nothrow) CustomMessage();
    if (data && !data->ReadFromParcel(parcel)) {
        delete data;
        data = nullptr;
    }
    return data;
}
} // namespace MMI
} // namespace OHOS