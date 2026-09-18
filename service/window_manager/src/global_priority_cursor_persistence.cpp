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

#include "global_priority_cursor_persistence.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>

#include "mmi_log.h"
#include "pixel_map.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "GlobalPriorityCursorPersistence"

namespace OHOS {
namespace MMI {
namespace {
constexpr const char *GPC_FILE_PATH = "/data/service/el1/public/multimodalinput/global_priority_cursor.dat";
constexpr const char *GPC_MAGIC = "GPC1";
} // namespace

int32_t GlobalPriorityCursorPersistence::Save(const GlobalPriorityCursor &globalCursor)
{
    auto *pixelMap = static_cast<Media::PixelMap *>(globalCursor.cursor.pixelMap);
    if (pixelMap == nullptr) {
        MMI_HILOGE("Global priority cursor pixel map does not exist");
        return RET_ERR;
    }
    Media::ImageInfo imageInfo;
    pixelMap->GetImageInfo(imageInfo);
    std::ofstream ofs(GPC_FILE_PATH, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        MMI_HILOGE("Open global priority cursor file failed");
        return RET_ERR;
    }
    ofs.write(GPC_MAGIC, strlen(GPC_MAGIC));
    ofs.write(reinterpret_cast<const char *>(&globalCursor.pid), sizeof(globalCursor.pid));
    int32_t width = imageInfo.size.width;
    int32_t height = imageInfo.size.height;
    ofs.write(reinterpret_cast<const char *>(&width), sizeof(width));
    ofs.write(reinterpret_cast<const char *>(&height), sizeof(height));
    ofs.write(reinterpret_cast<const char *>(&globalCursor.cursor.focusX), sizeof(globalCursor.cursor.focusX));
    ofs.write(reinterpret_cast<const char *>(&globalCursor.cursor.focusY), sizeof(globalCursor.cursor.focusY));
    const uint8_t *pixels = pixelMap->GetPixels();
    if (pixels == nullptr || pixelMap->GetPixelBytes() <= 0) {
        MMI_HILOGE("Global priority cursor pixel data is invalid");
        ofs.close();
        return RET_ERR;
    }
    ofs.write(reinterpret_cast<const char *>(pixels), pixelMap->GetPixelBytes());
    ofs.close();
    MMI_HILOGD("Save global priority cursor succeeded, pid:%{public}d", globalCursor.pid);
    return RET_OK;
}

int32_t GlobalPriorityCursorPersistence::LoadHeader(std::ifstream &ifs, FileHeader &fileHeader)
{
    ifs.read(reinterpret_cast<char *>(&fileHeader.pid), sizeof(fileHeader.pid));
    ifs.read(reinterpret_cast<char *>(&fileHeader.width), sizeof(fileHeader.width));
    ifs.read(reinterpret_cast<char *>(&fileHeader.height), sizeof(fileHeader.height));
    ifs.read(reinterpret_cast<char *>(&fileHeader.focusX), sizeof(fileHeader.focusX));
    ifs.read(reinterpret_cast<char *>(&fileHeader.focusY), sizeof(fileHeader.focusY));
    if (fileHeader.pid <= 0 || fileHeader.width <= 0 || fileHeader.height <= 0) {
        MMI_HILOGE("Global priority cursor file header is invalid");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t GlobalPriorityCursorPersistence::LoadPixelData(std::ifstream &ifs, const FileHeader &fileHeader,
    GlobalPriorityCursor &globalCursor)
{
    Media::InitializationOptions opts;
    opts.size.width = fileHeader.width;
    opts.size.height = fileHeader.height;
    opts.editable = true;
    auto pixelMap = Media::PixelMap::Create(opts);
    if (pixelMap == nullptr) {
        MMI_HILOGE("Create global priority cursor pixel map failed");
        return RET_ERR;
    }
    std::streamsize dataSize = pixelMap->GetPixelBytes();
    std::string buffer(dataSize, '\0');
    ifs.read(buffer.data(), dataSize);
    ifs.close();
    if (pixelMap->WritePixels(reinterpret_cast<const uint8_t *>(buffer.data()), buffer.size()) != 0) {
        MMI_HILOGE("Write global priority cursor pixel data failed");
        return RET_ERR;
    }
    globalCursor.cursor.pixelMap = pixelMap.release();
    globalCursor.cursor.focusX = fileHeader.focusX;
    globalCursor.cursor.focusY = fileHeader.focusY;
    globalCursor.pid = fileHeader.pid;
    globalCursor.persisted = true;
    MMI_HILOGD("Load global priority cursor succeeded, pid:%{public}d", fileHeader.pid);
    return RET_OK;
}

int32_t GlobalPriorityCursorPersistence::Load(GlobalPriorityCursor &globalCursor)
{
    std::ifstream ifs(GPC_FILE_PATH, std::ios::binary);
    if (!ifs.is_open() || ifs.peek() == std::ifstream::traits_type::eof()) {
        MMI_HILOGD("Global priority cursor file does not exist");
        return RET_ERR;
    }
    char magic[sizeof(GPC_MAGIC)] = { 0 };
    ifs.read(magic, strlen(GPC_MAGIC));
    if (strncmp(magic, GPC_MAGIC, strlen(GPC_MAGIC)) != 0) {
        MMI_HILOGE("Global priority cursor file magic is invalid");
        return RET_ERR;
    }
    FileHeader fileHeader;
    if (LoadHeader(ifs, fileHeader) != RET_OK) {
        return RET_ERR;
    }
    return LoadPixelData(ifs, fileHeader, globalCursor);
}

int32_t GlobalPriorityCursorPersistence::Remove()
{
    if (std::remove(GPC_FILE_PATH) != 0) {
        MMI_HILOGE("Remove global priority cursor file failed, errno:%{public}d", errno);
        return RET_ERR;
    }
    MMI_HILOGD("Remove global priority cursor file succeeded");
    return RET_OK;
}
} // namespace MMI
} // namespace OHOS