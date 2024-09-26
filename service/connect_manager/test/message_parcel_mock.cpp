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
#include "message_parcel_mock.h"

#include "iremote_broker.h"

#include "bytrace_adapter.h"

namespace OHOS {
using namespace OHOS::MMI;

Parcelable::Parcelable() : Parcelable(false)
{}

Parcelable::Parcelable(bool asRemote)
{
    asRemote_ = asRemote;
    behavior_ = 0;
}

int32_t PermissionHelper::GetTokenType()
{
    return 0;
}

void BytraceAdapter::StartIpcServer(uint32_t code) {}

void BytraceAdapter::StopIpcServer() {}

bool MessageParcel::WriteInterfaceToken(std::u16string name)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteInterfaceToken(name);
}

std::u16string MessageParcel::ReadInterfaceToken()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return u"";
    }
    return DfsMessageParcel::messageParcel->ReadInterfaceToken();
}

bool Parcel::WriteInt32(int32_t value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteInt32(value);
}

int32_t Parcel::ReadInt32()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return 0;
    }
    return DfsMessageParcel::messageParcel->ReadInt32();
}

bool Parcel::ReadInt32(int32_t &value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadInt32(value);
}

bool Parcel::WriteRemoteObject(const Parcelable *object)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteRemoteObject(object);
}

bool MessageParcel::WriteRemoteObject(const sptr<IRemoteObject> &object)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteRemoteObject(object);
}

sptr<IRemoteObject> MessageParcel::ReadRemoteObject()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return nullptr;
    }
    return DfsMessageParcel::messageParcel->ReadRemoteObject();
}

bool Parcel::ReadBool()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadBool();
}

bool Parcel::ReadBool(bool &value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadBool(value);
}

bool Parcel::WriteBool(bool value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteBool(value);
}

bool Parcel::WriteString(const std::string &value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteString(value);
}

bool Parcel::WriteCString(const char *value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteCString(value);
}

const std::string Parcel::ReadString()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return "";
    }
    return DfsMessageParcel::messageParcel->ReadString();
}

bool Parcel::ReadString(std::string &value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadString(value);
}

bool Parcel::ReadStringVector(std::vector<std::string> *value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadStringVector(value);
}

bool MessageParcel::WriteFileDescriptor(int fd)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteFileDescriptor(fd);
}

int MessageParcel::ReadFileDescriptor()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return 0;
    }
    return DfsMessageParcel::messageParcel->ReadFileDescriptor();
}

bool Parcel::ReadUint32(uint32_t &value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadUint32(value);
}

bool Parcel::WriteUint64(uint64_t value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteUint64(value);
}

bool Parcel::WriteUint16(uint16_t value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteUint16(value);
}

bool Parcel::WriteUint32(uint32_t value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteUint32(value);
}

bool Parcel::ReadUint64(uint64_t &value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadUint64(value);
}

bool PermissionHelper::VerifySystemApp()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->VerifySystemApp();
}

bool PermissionHelper::CheckMouseCursor()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->CheckMouseCursor();
}

bool PermissionHelper::CheckInputEventFilter()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->CheckInputEventFilter();
}

bool PermissionHelper::CheckInterceptor()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->CheckInterceptor();
}

bool PermissionHelper::CheckMonitor()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->CheckMonitor();
}

bool PermissionHelper::CheckDispatchControl()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->CheckDispatchControl();
}

bool PermissionHelper::CheckInfraredEmmit()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->CheckInfraredEmmit();
}

bool PermissionHelper::CheckAuthorize()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->CheckAuthorize();
}

bool Parcel::WriteBoolVector(const std::vector<bool> &val)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteBoolVector(val);
}

bool Parcel::WriteInt32Vector(const std::vector<int32_t> &val)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->WriteInt32Vector(val);
}

int64_t Parcel::ReadInt64()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return 0;
    }
    return DfsMessageParcel::messageParcel->ReadInt64();
}

bool Parcel::ReadInt64(int64_t &value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadInt64(value);
}

float Parcel::ReadFloat()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return 0.0f;
    }
    return DfsMessageParcel::messageParcel->ReadFloat();
}

bool Parcel::ReadFloat(float &value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadFloat(value);
}

double Parcel::ReadDouble()
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return 0.0;
    }
    return DfsMessageParcel::messageParcel->ReadDouble();
}

bool Parcel::ReadDouble(double &value)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return DfsMessageParcel::messageParcel->ReadDouble(value);
}

Media::PixelMap *Media::PixelMap::Unmarshalling(Parcel &parcel)
{
    if (DfsMessageParcel::messageParcel == nullptr) {
        return nullptr;
    }
    return DfsMessageParcel::messageParcel->Unmarshalling(parcel);
}
} // namespace OHOS