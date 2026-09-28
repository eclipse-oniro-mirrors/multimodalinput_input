/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#ifndef POINTER_RENDERER_H
#define POINTER_RENDERER_H

#include "struct_multimodal.h"
#include "ui/rs_canvas_node.h"
#include "ui/rs_surface_node.h"

namespace OHOS::MMI {
using image_ptr_t = std::shared_ptr<Rosen::Drawing::Image>;
using pixelmap_ptr_t = std::shared_ptr<OHOS::Media::PixelMap>;
constexpr int32_t DEFAULT_IMG_SIZE{ 10 };

class RenderConfig {
public:
    MOUSE_ICON style_;
    ICON_TYPE align_;
    std::string path_;
    uint32_t color { 0 };
    uint32_t size { 0 };
    uint32_t direction { 0 };
    uint32_t displayDirection { 0 };
    float dpi { 0 };
    bool isHard { false };
    int32_t rotationAngle { 0 };
    // float so sub-pixel pivot positions (e.g. 128.5 for a 73px glyph centred in a
    // 256 buffer) are preserved through to canvas.Rotate(), which takes float.
    // uint32_t would truncate 128.5 → 128, producing a 1px pivot error doubled at 180°.
    float rotationFocusX { 0.0f };
    float rotationFocusY { 0.0f };
    pixelmap_ptr_t userIconPixelMap { nullptr };
    int32_t userIconHotSpotX { 0 };
    int32_t userIconHotSpotY { 0 };
    bool userIconFollowSystem { false };
    int32_t x { 0 };
    int32_t y { 0 };
    uint64_t screenId { 0 };
    bool isBlur { false };
    int32_t GetImageSize() const;
    // Clamps GetImageSize() to fit a buffer of the given side length, preserving the 1:1 aspect ratio.
    // Gear 1 has a zero suppression exponent, so imageSize = 40 * dpi is unbounded and can exceed the
    // buffer; every other gear is already bounded by MAX_DPI. Takes the side as a parameter so the
    // hard, placeholder-soft and standalone-soft paths share one implementation without any of them
    // hardcoding another's size -- the first two render at DEFAULT_CURSOR_SIZE, the last at a
    // glyph-derived side that can exceed it.
    int32_t GetClampedImageSize(uint32_t bufferSide) const;
    // The side the image is actually drawn at. For built-in SVGs this is GetClampedImageSize();
    // for an app-supplied pixelmap it is the icon's own long side, capped at the buffer side, so
    // placement and the hotspot follow the real dimensions rather than the system gear size.
    int32_t GetPlacedImageSize(uint32_t bufferSide) const;
    // Ratio an app-supplied icon is drawn at relative to its own pixels: 1.0 when it already fits
    // the buffer, otherwise bufferSide/longSide. Placement, the hotspot and the actual scaling all
    // go through this one value so they cannot drift apart.
    float UserIconScaleRatio(uint32_t bufferSide) const;
    float AdjustIncreaseRatio(float dpi) const;
    std::string ToString() const;
    // The style's focus within its own image. Takes the buffer side because the focus must be
    // derived from GetClampedImageSize(), which the bitmap is actually decoded at.
    uint32_t GetFocusX(uint32_t bufferSide) const;
    uint32_t GetFocusY(uint32_t bufferSide) const;
    // Where the hotspot lands inside the buffer, now that the draw origin centres the image rather
    // than aligning the hotspot to the buffer centre. ScreenPointer::Move offsets the cursor
    // position by this instead of by half the buffer side.
    int32_t HotspotInBufferX(uint32_t bufferSide) const;
    int32_t HotspotInBufferY(uint32_t bufferSide) const;

    RenderConfig() = default;
    ~RenderConfig() = default;

    bool operator == (const RenderConfig& rhs) const
    {
        return style_ == rhs.style_ && GetImageSize() == rhs.GetImageSize() && color == rhs.color;
    }

    bool operator != (const RenderConfig& rhs) const
    {
        return style_ != rhs.style_ || GetImageSize() != rhs.GetImageSize() || color != rhs.color;
    }
};

class PointerRenderer {
public:
    PointerRenderer() = default;
    ~PointerRenderer() = default;

    int32_t Render(uint8_t *addr, uint32_t width, uint32_t height, const RenderConfig &cfg);
    int32_t DynamicRender(uint8_t *addr, uint32_t width, uint32_t height, const RenderConfig &cfg);
    image_ptr_t UserIconScale(uint32_t width, uint32_t height, const RenderConfig &cfg);
    void LoadPointerToCache(const std::map<MOUSE_ICON, IconStyle> &mouseIcons);
private:
    int32_t BlurRender(uint8_t *addr, uint32_t addrSize, uint32_t width, uint32_t height,
        const RenderConfig &cfg);
    void DrawDefaultPointer(uint32_t width, uint32_t height, const RenderConfig &cfg);
    void DrawBlurPointer(uint32_t width, uint32_t height, const RenderConfig &lastCfg, const RenderConfig &cfg);
    void AdjustDeltaForDirection(int32_t &dx, int32_t &dy, uint32_t direction, uint32_t displayDirection);
    // (left, top) is the image's top-left in buffer coordinates, matching DrawImage's arguments.
    bool IsPositionOutCanvas(int32_t left, int32_t top, int32_t imageSize, uint32_t width, uint32_t height);
    bool HasPointerCfg(const RenderConfig &cfg);
    void SetPointerCfg(const RenderConfig &cfg);
    const RenderConfig& GetPointerCfg(const RenderConfig &defaultCfg);
    std::vector<image_ptr_t> GetPointerImage(const RenderConfig &cfg);
    void LoadDefaultPointerImage(uint32_t bufferSide, const RenderConfig &cfg);
    void ApplyAlpha(uint8_t *pixel, const int32_t len, bool isPixelPremul, const float pecent);
    void SetAlpha(pixelmap_ptr_t pixelMap, const float pecent);
    image_ptr_t LoadPointerImage(uint32_t bufferSide, const RenderConfig &cfg);
    bool GetPointerFromCache(const RenderConfig &cfg, std::string& svgContent);
    pixelmap_ptr_t LoadCursorSvgWithColor(uint32_t bufferSide, const RenderConfig &cfg);
    image_ptr_t ExtractDrawingImage(pixelmap_ptr_t pixelMap);
    int32_t DrawImage(OHOS::Rosen::Drawing::Canvas &canvas, uint32_t width, uint32_t height,
        const RenderConfig &cfg);
    // The cached image is decoded at GetClampedImageSize(bufferSide), so the buffer side is part of
    // the identity: RenderConfig::operator== compares the *unclamped* image size, and above the
    // buffer limit two sides can share that value while producing differently sized images.
    // Omitting it would let a 320px soft-cursor decode be reused for a 256px hard-cursor buffer,
    // bypassing the clamp.
    std::vector<std::tuple<RenderConfig, uint32_t, image_ptr_t>> imgMaps_;
    std::shared_mutex imgMapsMtx_;
    image_ptr_t FindImg(uint32_t bufferSide, const RenderConfig &cfg)
    {
        std::shared_lock lock(imgMapsMtx_);
        for (auto& data : imgMaps_) {
            if ((std::get<1>(data) == bufferSide) && (std::get<0>(data) == cfg)) {
                return std::get<2>(data);
            }
        }
        return nullptr;
    }
    void PushImg(uint32_t bufferSide, const RenderConfig &cfg, image_ptr_t img)
    {
        std::unique_lock lock(imgMapsMtx_);
        if (imgMaps_.size() >= DEFAULT_IMG_SIZE) {
            imgMaps_.erase(imgMaps_.begin());
        }
        imgMaps_.push_back({cfg, bufferSide, img});
    }
    std::map<MOUSE_ICON, std::string> mouseIcons_;
    mutable std::mutex cacheMutex_;
    std::map<uint64_t, RenderConfig> screenConfigs_;
    bool defaultInit_ { false };
    std::map<uint64_t, std::vector<image_ptr_t>> screenImages_;

    OHOS::Rosen::Drawing::Bitmap defaultBitmap_;
    OHOS::Rosen::Drawing::BitmapFormat defaultFormat_ {
        OHOS::Rosen::Drawing::COLORTYPE_RGBA_8888,
        OHOS::Rosen::Drawing::ALPHATYPE_OPAQUE,
    };
    OHOS::Rosen::Drawing::Canvas defaultCanvas_;
};
} // namespace OHOS::MMI

#endif // POINTER_RENDERER_H