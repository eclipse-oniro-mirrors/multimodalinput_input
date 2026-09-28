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

#include "pointer_renderer.h"

#include <regex>
#include <unistd.h>
#include "image_source.h"
#include "message_parcel.h"
#include "window_info.h"
#include "util.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_CURSOR
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "PointerRenderer"

constexpr uint32_t RENDER_STRIDE{4};
constexpr int32_t DEVCIE_INDEPENDENT_PIXELS{40};
constexpr float INCREASE_RATIO{1.22f};
constexpr int32_t MIN_POINTER_COLOR{0x000000};
constexpr int32_t MAX_POINTER_COLOR{0xFFFFFF};
constexpr int32_t OTHER_POINTER_COLOR{0x171717};
constexpr float CALCULATE_IMAGE_MIDDLE{2.0f};
constexpr float CALCULATE_MOUSE_ICON_BIAS{ 5.0f / 33.0f };
constexpr float ROTATION_ANGLE90 {90.0f};
constexpr float MAX_DPI {1.93f};   //1.93 is the max dpi for hardware cursor pointer to limit the image size within 256
constexpr float EPSILON = 1e-6;
constexpr uint32_t MAX_POINTER_SIZE {7};
const std::string IMAGE_POINTER_DEFAULT_PATH = "/data/service/el1/public/multimodalinput/mouse_icon/";
constexpr int32_t BLUR_NUM { 3 };
constexpr int32_t BGRA_ALPHA_INDEX { 3 };
constexpr int32_t DIRECTION_NUM { 4 };
constexpr float DEFAULT_OFFSET[] = {0.4f, 0.6f, 0.8f};
constexpr float DEFAULT_BLUR[] = {0.4f, 0.4f, 0.4f};

namespace OHOS::MMI {

// The buffer focus is the centre of the actual buffer, derived from its size rather than a fixed
// constant. Everything that anchors to the buffer centre (rotation pivots, draw origins) must come
// from here so every caller stays pixel-identical whatever side it renders at: the hard cursor and
// the placeholder soft cursor both use DEFAULT_CURSOR_SIZE, while a standalone soft cursor sizes
// its buffer from the glyph (canvasWidth_/canvasHeight_) and so can be larger.
static int32_t BufferFocus(uint32_t side)
{
    return static_cast<int32_t>(side / CALCULATE_IMAGE_MIDDLE);
}

// The side the image is actually drawn at: the built-in SVG path is sized from the gear and dpi,
// but an app-supplied pixelmap keeps its own dimensions unless it would not fit the buffer.
int32_t RenderConfig::GetPlacedImageSize(uint32_t bufferSide) const
{
    if (userIconPixelMap == nullptr) {
        return GetClampedImageSize(bufferSide);
    }
    Media::ImageInfo imageInfo;
    userIconPixelMap->GetImageInfo(imageInfo);
    int32_t iconWidth = imageInfo.size.width;
    int32_t iconHeight = imageInfo.size.height;
    if (iconWidth <= 0 || iconHeight <= 0) {
        MMI_HILOGE("Invalid user icon size, width:%{public}d, height:%{public}d", iconWidth, iconHeight);
        return GetClampedImageSize(bufferSide);
    }
    // A user icon larger than the buffer is scaled down to fit (see UserIconScale); anything that
    // already fits keeps its original size, so a legal 256x256 icon renders at 256, not 255.
    int32_t longSide = (iconWidth > iconHeight ? iconWidth : iconHeight);
    int32_t maxSide = static_cast<int32_t>(bufferSide);
    if (longSide > maxSide) {
        return maxSide;
    }
    return longSide;
}

float RenderConfig::UserIconScaleRatio(uint32_t bufferSide) const
{
    if (userIconPixelMap == nullptr) {
        return 1.0f;
    }
    Media::ImageInfo imageInfo;
    userIconPixelMap->GetImageInfo(imageInfo);
    int32_t iconWidth = imageInfo.size.width;
    int32_t iconHeight = imageInfo.size.height;
    if (iconWidth <= 0 || iconHeight <= 0) {
        return 1.0f;
    }
    int32_t longSide = (iconWidth > iconHeight ? iconWidth : iconHeight);
    int32_t maxSide = static_cast<int32_t>(bufferSide);
    if (longSide <= maxSide) {
        return 1.0f;
    }
    return static_cast<float>(maxSide) / static_cast<float>(longSide);
}

// The draw origin centres the image in the buffer. It deliberately does NOT align the
// style's own focus (GetFocusX/Y) onto the buffer centre: that older scheme let a non-centred
// align (ANGLE_NW's hotspot, ANGLE_SW's (0,h)) push the image almost a full imageSize away from
// the centre, so it needed a 2x buffer and clipped 8 styles once the side shrank to 256.
// Centring the image needs only 1x: imageSize <= side - 1 already guarantees origin >= 0 and
// origin + imageSize <= side, for every align, direction and app-supplied hotspot alike.
// The hotspot then lands at origin + GetFocusX/Y, which ScreenPointer::Move must offset by --
// see HotspotInBufferX/Y.
static int32_t CalcDrawOriginX(uint32_t width, const RenderConfig &cfg)
{
    // Uses the size the image is actually placed at, so an app-supplied pixelmap is centred by its
    // own dimensions rather than by the system gear size.
    return BufferFocus(width) - static_cast<int32_t>(cfg.GetPlacedImageSize(width) / CALCULATE_IMAGE_MIDDLE);
}

static int32_t CalcDrawOriginY(uint32_t height, const RenderConfig &cfg)
{
    return BufferFocus(height) - static_cast<int32_t>(cfg.GetPlacedImageSize(height) / CALCULATE_IMAGE_MIDDLE);
}

int32_t RenderConfig::GetImageSize() const
{
    float increaseRatio = AdjustIncreaseRatio(dpi);
    return pow(increaseRatio, size - 1) * dpi * DEVCIE_INDEPENDENT_PIXELS;
}

int32_t RenderConfig::GetClampedImageSize(uint32_t bufferSide) const
{
    int32_t imageSize = GetImageSize();
    if (bufferSide == 0) {
        MMI_HILOGE("The bufferSide cannot be 0, return unclamped image size");
        return imageSize;
    }
    // Leave one pixel of headroom so the glyph's far edge stays strictly inside the buffer.
    int32_t maxSize = static_cast<int32_t>(bufferSide) - 1;
    if (imageSize <= maxSize) {
        return imageSize;
    }
    // HILOGD, not HILOGW: this runs on every render, and the condition is stable rather than
    // transient, so a warn-level log would repeat once per frame for as long as it holds.
    MMI_HILOGD("ImageSize:%{public}d exceeds buffer:%{public}u, clamp to %{public}d",
        imageSize, bufferSide, maxSize);
    return maxSize;
}

float RenderConfig::AdjustIncreaseRatio(float originDpi) const
{
    if (std::fabs(originDpi) < EPSILON) {
        MMI_HILOGE("The originDpi cannot be 0, return default increase ratio");
        return INCREASE_RATIO;
    }
    if (originDpi <= MAX_DPI) {
        return INCREASE_RATIO;
    } else {
        return INCREASE_RATIO * pow(MAX_DPI / originDpi, float(1.0f / float(MAX_POINTER_SIZE - 1)));
    }
}

std::string RenderConfig::ToString() const
{
    std::ostringstream oss;
    oss << "{style=" << style_ << ", align=" << align_ << ", color=" << color
        << ", size=" << size << ", direction=" << direction
        <<", dpi=" << dpi << ", screenId=" << screenId << ", isBlur=" << isBlur
        << ", isHard=" << isHard << ", ImageSize=" << GetImageSize() << "}";
    return oss.str();
}

// Render()/BlurRender() call canvas.Rotate(-direction * 90, bufferCentre) before drawing, so the
// rotation carries the hotspot along with the image. The old scheme aligned the hotspot onto the
// buffer centre -- the one point a rotation about the centre leaves fixed -- so it needed no
// compensation here, and paid for that with a 2x buffer. Now that the origin centres the image
// instead, an off-centre hotspot does move, and Move() would offset by the unrotated position.
// Rotate the centre-relative offset by the same angle to recover where the hotspot really is.
static void RotateOffsetAboutCentre(uint32_t direction, int32_t &dx, int32_t &dy)
{
    // Quarter turns need no trig. The canvas applies -direction * 90, i.e. the inverse of the
    // +90 mapping in AdjustDeltaForDirection(), so the signs here are that one's transposed.
    int32_t tempDx = dx;
    int32_t tempDy = dy;
    Direction angle = static_cast<Direction>(direction % DIRECTION_NUM);
    if (angle == DIRECTION90) {
        dx = tempDy;
        dy = -tempDx;
    } else if (angle == DIRECTION180) {
        dx = -tempDx;
        dy = -tempDy;
    } else if (angle == DIRECTION270) {
        dx = -tempDy;
        dy = tempDx;
    }
}

// The hotspot's position relative to the buffer centre, after rotation.
static void HotspotOffsetFromCentre(uint32_t bufferSide, const RenderConfig &cfg, int32_t &dx, int32_t &dy)
{
    // Half of the size the image is actually placed at, so an app-supplied icon is centred by its
    // own dimensions rather than by the system gear size.
    int32_t half = static_cast<int32_t>(cfg.GetPlacedImageSize(bufferSide) / CALCULATE_IMAGE_MIDDLE);
    dx = static_cast<int32_t>(cfg.GetFocusX(bufferSide)) - half;
    dy = static_cast<int32_t>(cfg.GetFocusY(bufferSide)) - half;
    RotateOffsetAboutCentre(cfg.direction, dx, dy);
}

int32_t RenderConfig::HotspotInBufferX(uint32_t bufferSide) const
{
    int32_t dx = 0;
    int32_t dy = 0;
    HotspotOffsetFromCentre(bufferSide, *this, dx, dy);
    return BufferFocus(bufferSide) + dx;
}

int32_t RenderConfig::HotspotInBufferY(uint32_t bufferSide) const
{
    int32_t dx = 0;
    int32_t dy = 0;
    HotspotOffsetFromCentre(bufferSide, *this, dx, dy);
    return BufferFocus(bufferSide) + dy;
}

uint32_t RenderConfig::GetFocusX(uint32_t bufferSide) const
{
    // Must use the clamped size: the bitmap is decoded at GetClampedImageSize(), so deriving the
    // focus from the unclamped GetImageSize() would offset the hotspot by the clamp delta.
    int32_t width = this->GetClampedImageSize(bufferSide);
    switch (this->align_) {
        case ANGLE_E:       // fall-though
        case ANGLE_SW:
            return 0;
        case ANGLE_S:       // fall-though
        case ANGLE_N:       // fall-though
        case ANGLE_CENTER:
            // The pre-change offset truncated (buffer focus - width / 2.0f) as a whole, which rounds
            // the focus up for an odd width. Keep that exact rounding so the display stays identical.
            return static_cast<uint32_t>(width - static_cast<int32_t>(width / CALCULATE_IMAGE_MIDDLE));
        case ANGLE_W:       // fall-though
        case ANGLE_SE:      // fall-though
        case ANGLE_NE:
            return static_cast<uint32_t>(width);
        case ANGLE_NW:
            // The app's hotspot is in the icon's own pixel coordinates. When the icon is scaled
            // down to fit the buffer, the hotspot must shrink by the same ratio, or it points at
            // the wrong part of the drawn image.
            return static_cast<uint32_t>(static_cast<float>(this->userIconHotSpotX) *
                this->UserIconScaleRatio(bufferSide));
        case ANGLE_NW_RIGHT:
            // Do NOT simplify to static_cast<uint32_t>(width * CALCULATE_MOUSE_ICON_BIAS): the
            // pre-change expression truncated (focus - width * BIAS) as a whole, which rounds this
            // focus up. A plain multiply truncates instead and lands 1px low at every width tested
            // (w=73 -> 12 vs 11, w=256 -> 39 vs 38). Keep the whole-expression form.
            return static_cast<uint32_t>(width - static_cast<int32_t>(width - width * CALCULATE_MOUSE_ICON_BIAS));
        default:
            MMI_HILOGD("No need calculate focus x");
            return 0;
    }
}

uint32_t RenderConfig::GetFocusY(uint32_t bufferSide) const
{
    // See GetFocusX(): the clamped size is what the bitmap was decoded at.
    int32_t height = this->GetClampedImageSize(bufferSide);
    switch (this->align_) {
        case ANGLE_S:       // fall-though
        case ANGLE_NE:      // fall-though
        case ANGLE_NW_RIGHT:
            return 0;
        case ANGLE_E:       // fall-though
        case ANGLE_CENTER:
            // Same whole-expression truncation as GetFocusX(); see the comment there.
            return static_cast<uint32_t>(height - static_cast<int32_t>(height / CALCULATE_IMAGE_MIDDLE));
        case ANGLE_W:       // fall-though
        case ANGLE_N:       // fall-though
        case ANGLE_SE:      // fall-though
        case ANGLE_SW:
            return static_cast<uint32_t>(height);
        case ANGLE_NW:
            // See GetFocusX(): the hotspot scales with the icon.
            return static_cast<uint32_t>(static_cast<float>(this->userIconHotSpotY) *
                this->UserIconScaleRatio(bufferSide));
        default:
            MMI_HILOGD("No need calculate focus y");
            return 0;
    }
}

image_ptr_t PointerRenderer::UserIconScale(uint32_t width, uint32_t height, const RenderConfig &cfg)
{
    CHKPP(cfg.userIconPixelMap);
    // An app-supplied pixelmap is unbounded external input, so it is bounded here rather than by
    // GetClampedImageSize(), which only constrains the built-in SVG path. The bound is the buffer
    // side itself, not side-1: the draw origin centres the image, so an icon exactly the size of
    // the buffer lands at [0, side) and is fully visible. Trimming a pixel would change the
    // rendering of a legal 256x256 icon, which this change must not do.
    if (width == 0 || height == 0) {
        MMI_HILOGE("Invalid buffer size, width:%{public}u, height:%{public}u", width, height);
        return ExtractDrawingImage(cfg.userIconPixelMap);
    }
    float ratio = cfg.UserIconScaleRatio(width);
    if (ratio >= 1.0f) {
        // Already fits: draw at native size, keeping the app's pixels and hotspot untouched.
        return ExtractDrawingImage(cfg.userIconPixelMap);
    }
    Media::ImageInfo imageInfo;
    cfg.userIconPixelMap->GetImageInfo(imageInfo);
    // Same reasoning as GetClampedImageSize(): this runs per render, and an oversized user icon is
    // a stable condition, so it must not be warn-level.
    MMI_HILOGD("User icon %{public}dx%{public}d exceeds buffer %{public}u, scale by %{public}f",
        imageInfo.size.width, imageInfo.size.height, width, ratio);
    MessageParcel data;
    cfg.userIconPixelMap->Marshalling(data);
    std::shared_ptr<Media::PixelMap> scaled(Media::PixelMap::Unmarshalling(data));
    if (scaled == nullptr) {
        MMI_HILOGE("Copy user icon failed, draw it unscaled");
        return ExtractDrawingImage(cfg.userIconPixelMap);
    }
    scaled->scale(ratio, ratio, Media::AntiAliasingOption::SLR);
    return ExtractDrawingImage(scaled);
}

int32_t PointerRenderer::Render(uint8_t *addr, uint32_t width, uint32_t height, const RenderConfig &cfg)
{
    CHKPR(addr, RET_ERR);
    MMI_HILOGD("Render cfg:%{private}s, width:%{public}u, height:%{public}u", cfg.ToString().c_str(), width, height);

    uint32_t addrSize = width * height * RENDER_STRIDE;
    if (cfg.style_ == MOUSE_ICON::TRANSPARENT_ICON) {
        (void)memset_s(addr, addrSize, 0, addrSize);
        SetPointerCfg(cfg);
        return RET_OK;
    }
    if (cfg.isHard && cfg.isBlur) {
        MMI_HILOGD("Blur Render");
        return BlurRender(addr, addrSize, width, height, cfg);
    }
    SetPointerCfg(cfg);
    // construct bitmap
    OHOS::Rosen::Drawing::Bitmap bitmap;
    OHOS::Rosen::Drawing::BitmapFormat format {
        OHOS::Rosen::Drawing::COLORTYPE_RGBA_8888,
        OHOS::Rosen::Drawing::ALPHATYPE_OPAQUE,
    };
    if (!bitmap.Build(width, height, format)) {
        MMI_HILOGE("Bitmap build failed");
    }

    // construct canvas and bind to bitmap
    OHOS::Rosen::Drawing::Canvas canvas;
    canvas.Bind(bitmap);
    canvas.Clear(OHOS::Rosen::Drawing::Color::COLOR_TRANSPARENT);
    // The focus is the buffer centre, derived from the actual buffer size rather than a fixed
    // constant, so every buffer side stays pixel-identical -- the hard and placeholder soft paths
    // at DEFAULT_CURSOR_SIZE, a standalone soft cursor at its glyph-derived size.
    int32_t focusX = BufferFocus(width);
    int32_t focusY = BufferFocus(height);
    if (cfg.direction) {
        int32_t directionFlag = -1;
        int32_t degree = static_cast<int32_t>(directionFlag * static_cast<int32_t>(cfg.direction) * ROTATION_ANGLE90);
        canvas.Rotate(degree, focusX, focusY);
    }
    image_ptr_t image = nullptr;
    if (cfg.userIconPixelMap == nullptr) {
        image = LoadPointerImage(width, cfg);
    } else {
        image = UserIconScale(width, height, cfg);
    }
    CHKPR(image, RET_ERR);
    // Draw image on canvas: the draw origin places the style's focus onto the buffer focus.
    canvas.DrawImage(*image, CalcDrawOriginX(width, cfg), CalcDrawOriginY(height, cfg),
        Rosen::Drawing::SamplingOptions());
    errno_t ret = memcpy_s(addr, addrSize, bitmap.GetPixels(), bitmap.ComputeByteSize());
    if (ret != EOK) {
        MMI_HILOGE("Memcpy_s failed");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t PointerRenderer::BlurRender(uint8_t *addr, uint32_t addrSize, uint32_t width, uint32_t height,
    const RenderConfig &cfg)
{
    if (!cfg.isBlur) {
        MMI_HILOGD("Blur Render without blur");
        return RET_ERR;
    }
    if (!defaultInit_) {
        if (!defaultBitmap_.Build(width, height, defaultFormat_)) {
            MMI_HILOGE("Default Bitmap build failed");
        }
        defaultCanvas_.Bind(defaultBitmap_);
        LoadDefaultPointerImage(width, cfg);
        SetPointerCfg(cfg);
        defaultCanvas_.Save();
        defaultInit_ = true;
        MMI_HILOGI("Blur Render init success");
    }
    if (!HasPointerCfg(cfg)) {
        MMI_HILOGI("cfg change, render cfg:%{private}s", cfg.ToString().c_str());
        LoadDefaultPointerImage(width, cfg);
        SetPointerCfg(cfg);
    }
    if (cfg.direction >= 0) {
        MMI_HILOGD("rotate diretion: %{public}u", cfg.direction);
        defaultCanvas_.Restore();
        defaultCanvas_.Save();
        int32_t directionFlag = -1;
        int32_t degree = static_cast<int32_t>(directionFlag * static_cast<int32_t>(cfg.direction) * ROTATION_ANGLE90);
        defaultCanvas_.Rotate(degree, BufferFocus(width), BufferFocus(height));
    }
    defaultCanvas_.Clear(OHOS::Rosen::Drawing::Color::COLOR_TRANSPARENT);
    const auto& lastCfg = GetPointerCfg(cfg);
    DrawBlurPointer(width, height, lastCfg, cfg);
    DrawDefaultPointer(width, height, cfg);
    SetPointerCfg(cfg);
    errno_t ret = memcpy_s(addr, addrSize, defaultBitmap_.GetPixels(), addrSize);
    if (ret != EOK) {
        MMI_HILOGE("Memcpy_s failed");
        return RET_ERR;
    }
    return RET_OK;
}

void PointerRenderer::DrawDefaultPointer(uint32_t width, uint32_t height, const RenderConfig &cfg)
{
    const auto& images = GetPointerImage(cfg);
    if (images.size() < BLUR_NUM + 1) {
        MMI_HILOGE("The number of blur images is less than %{public}d", BLUR_NUM);
        return;
    }
    if (!images[0]) {
        MMI_HILOGE("Default pointer image is null");
        return;
    }
    // 512-tier blur draws use the pre-centring origin (BufferFocus - GetFocusX/Y) so that
    // trail positions match the original geometry. The corresponding hotspot for Move() is
    // BufferFocus (not the draw origin): in the original layout origin + styleFocus = BufferFocus,
    // so the hotspot was always at the buffer centre regardless of style.
    // 256-tier uses the centred layout (CalcDrawOriginX/Y) and HotspotInBufferX/Y.
    int32_t originX, originY;
    if (width == static_cast<uint32_t>(BLUR_CURSOR_SIZE)) {
        originX = BufferFocus(width) - static_cast<int32_t>(cfg.GetFocusX(width));
        originY = BufferFocus(height) - static_cast<int32_t>(cfg.GetFocusY(height));
    } else {
        originX = CalcDrawOriginX(width, cfg);
        originY = CalcDrawOriginY(height, cfg);
    }
    defaultCanvas_.DrawImage(*images[0], originX, originY, Rosen::Drawing::SamplingOptions());
}

void PointerRenderer::DrawBlurPointer(uint32_t width, uint32_t height, const RenderConfig &lastCfg,
    const RenderConfig &cfg)
{
    const auto& images = GetPointerImage(cfg);
    if (images.size() < BLUR_NUM + 1) {
        MMI_HILOGE("The number of blur images is less than %{public}d", BLUR_NUM);
        return;
    }
    int32_t dx = lastCfg.x - cfg.x;
    int32_t dy = lastCfg.y - cfg.y;
    AdjustDeltaForDirection(dx, dy, cfg.direction, cfg.displayDirection);
    // 512-tier uses the pre-centring origin (BufferFocus - GetFocusX/Y) to preserve
    // original trail geometry. 256-tier uses CalcDrawOriginX/Y (centred layout).
    // Both tiers use current IsPositionOutCanvas (top-left semantics, slack=imageSize/4).
    int32_t offsetX, offsetY;
    if (width == static_cast<uint32_t>(BLUR_CURSOR_SIZE)) {
        offsetX = BufferFocus(width) - static_cast<int32_t>(cfg.GetFocusX(width));
        offsetY = BufferFocus(height) - static_cast<int32_t>(cfg.GetFocusY(height));
    } else {
        offsetX = CalcDrawOriginX(width, cfg);
        offsetY = CalcDrawOriginY(height, cfg);
    }
    // Clamped, matching what the bitmap was decoded at; the unclamped size would over-report the
    // extent.
    const int32_t imageSize = cfg.GetClampedImageSize(width);
    for (int32_t i = 1; i <= BLUR_NUM; i++) {
        if (!images[i]) {
            MMI_HILOGE("Blur pointer image %{public}d is null", i);
            continue;
        }
        int32_t localX = offsetX + dx * DEFAULT_OFFSET[i - 1];
        int32_t localY = offsetY + dy * DEFAULT_OFFSET[i - 1];
        if (IsPositionOutCanvas(localX, localY, imageSize, width, height)) {
            MMI_HILOGD("The blur image is out of bounds, no need to draw");
            continue;
        }
        defaultCanvas_.DrawImage(*images[i], localX, localY, Rosen::Drawing::SamplingOptions());
    }
}

void PointerRenderer::AdjustDeltaForDirection(int32_t &dx, int32_t &dy, uint32_t direction, uint32_t displayDirection)
{
    Direction displayDir = static_cast<Direction>(displayDirection);
    Direction dir = static_cast<Direction>(direction);
    int32_t tempDx = dx;
    int32_t tempDy = dy;
    Direction angle = static_cast<Direction>((dir - displayDir + DIRECTION_NUM) % DIRECTION_NUM);
    if (angle == DIRECTION90) {
        dx = -tempDy;
        dy = tempDx;
    } else if (angle == DIRECTION180) {
        dx = -tempDx;
        dy = -tempDy;
    } else if (angle == DIRECTION270) {
        dx = tempDy;
        dy = -tempDx;
    }
    MMI_HILOGD("direction:%{private}d, displayDirection:%{private}d, angle:%{private}d",
        static_cast<int32_t>(direction), static_cast<int32_t>(displayDirection), static_cast<int32_t>(angle));
}

// Bounds check for a blur image about to be drawn at (left, top). These coordinates are a
// top-left corner, the same value DrawImage() consumes -- not a centre. An earlier version
// subtracted imageSize/2 from them, which double-counted the offset: at gear 7 the image is
// centred with origin 1, so the check saw left = -126 and skipped every horizontal blur frame.
bool PointerRenderer::IsPositionOutCanvas(int32_t left, int32_t top, int32_t imageSize,
    uint32_t width, uint32_t height)
{
    int32_t right = left + imageSize;
    int32_t bottom = top + imageSize;
    // Allow a quarter of the image to hang off each edge so a partially visible blur still draws.
    int32_t slack = imageSize / 4;
    return right < -slack || left > static_cast<int32_t>(width) + slack ||
            bottom < -slack || top > static_cast<int32_t>(height) + slack;
}

bool PointerRenderer::HasPointerCfg(const RenderConfig &cfg)
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = screenConfigs_.find(cfg.screenId);
    if (it == screenConfigs_.end()) {
        return false;
    }
    if (it->second != cfg || it->second.isHard != cfg.isHard || it->second.isBlur != cfg.isBlur) {
        return false;
    }
    return true;
}

void PointerRenderer::SetPointerCfg(const RenderConfig &cfg)
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    screenConfigs_[cfg.screenId] = cfg;
}

const RenderConfig& PointerRenderer::GetPointerCfg(const RenderConfig &defaultCfg)
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = screenConfigs_.find(defaultCfg.screenId);
    if (it == screenConfigs_.end()) {
        return defaultCfg;
    }
    return it->second.isHard ? it->second : defaultCfg;
}

std::vector<image_ptr_t> PointerRenderer::GetPointerImage(const RenderConfig &cfg)
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = screenImages_.find(cfg.screenId);
    if (it == screenImages_.end()) {
        return {};
    }
    return it->second;
}

void PointerRenderer::LoadDefaultPointerImage(uint32_t bufferSide, const RenderConfig &cfg)
{
    std::vector<image_ptr_t> images;
    images.resize(BLUR_NUM + 1);
    auto pixelmap = LoadCursorSvgWithColor(bufferSide, cfg);
    if (!pixelmap) {
        MMI_HILOGE("Load default cursor svg failed");
        return;
    }
    auto defaultImage = ExtractDrawingImage(pixelmap);
    if (!defaultImage) {
        MMI_HILOGE("Extract default drawing image failed");
        return;
    }
    images[0] = defaultImage;
    for (int32_t i = 1; i <= BLUR_NUM; i++) {
        pixelmap_ptr_t blurPixelMap = LoadCursorSvgWithColor(bufferSide, cfg);
        if (!blurPixelMap) {
            MMI_HILOGE("Load blur cursor svg failed");
            continue;
        }
        SetAlpha(blurPixelMap, DEFAULT_BLUR[i - 1]);
        auto blurImage = ExtractDrawingImage(blurPixelMap);
        if (!blurImage) {
            MMI_HILOGE("Extract blur drawing image failed");
            continue;
        }
        images[i] = blurImage;
    }
    std::lock_guard<std::mutex> lock(cacheMutex_);
    screenImages_[cfg.screenId] = images;
}

void PointerRenderer::ApplyAlpha(uint8_t *pixel, const int32_t len, bool isPixelPremul,
    const float pecent)
{
    if (!pixel) {
        MMI_HILOGE("Invalid pixel");
        return;
    }
    if (!isPixelPremul) {
        MMI_HILOGD("not premultiplied");
        return;
    }
    for (int32_t pixelIndex = 0; pixelIndex < len; pixelIndex++) {
        if (pixelIndex != BGRA_ALPHA_INDEX) {
            pixel[pixelIndex] = static_cast<uint8_t>(pixel[pixelIndex] * pecent);
        }
    }
}

void PointerRenderer::SetAlpha(pixelmap_ptr_t pixelMap, const float pecent)
{
    if (!pixelMap) {
        MMI_HILOGE("Invalid pixelMap");
        return;
    }
    bool isPixelPremul = (pixelMap->GetAlphaType() == Media::AlphaType::IMAGE_ALPHA_TYPE_PREMUL);
    int32_t rowStride = pixelMap->GetRowStride();
    int32_t height = pixelMap->GetHeight();
    int32_t width = pixelMap->GetWidth();
    uint8_t *pixels = static_cast<uint8_t *>(pixelMap->GetWritablePixels());
    if (!pixels) {
        MMI_HILOGE("Invalid pixels");
        return;
    }
    for (int32_t i = 0; i < height; i++) {
        for (int32_t j = 0; j < width; j++) {
            uint8_t *pixel = pixels + i * rowStride + j * RENDER_STRIDE;
            ApplyAlpha(pixel, RENDER_STRIDE, isPixelPremul, pecent);
            pixel[BGRA_ALPHA_INDEX] = static_cast<uint8_t>(pixel[BGRA_ALPHA_INDEX] * pecent);
        }
    }
}

void PointerRenderer::LoadPointerToCache(const std::map<MOUSE_ICON, IconStyle> &mouseIcons)
{
    std::string svgContent;
    bool allLoaded = true;
    for (const auto& [icon, style] : mouseIcons) {
        if (!ReadFile(style.iconPath, svgContent)) {
            MMI_HILOGE("read file failed for icon:%{public}d", icon);
            allLoaded = false;
            continue;
        }
        mouseIcons_[icon] = svgContent;
    }
    MMI_HILOGI("LoadPointerToCache, allLoaded:%{public}d", allLoaded);
}

bool PointerRenderer::GetPointerFromCache(const RenderConfig &cfg, std::string& svgContent)
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = mouseIcons_.find(static_cast<MOUSE_ICON>(cfg.style_));
    if (it != mouseIcons_.end()) {
        svgContent = it->second;
        MMI_HILOGI("Get pointer from cache for style:%{public}d", cfg.style_);
        return true;
    }
    MMI_HILOGI("Pointer style:%{public}d not found in cache", cfg.style_);
    if (ReadFile(cfg.path_, svgContent)) {
        mouseIcons_[static_cast<MOUSE_ICON>(cfg.style_)] = svgContent;
        MMI_HILOGI("Read pointer file success for style:%{public}d", cfg.style_);
        return true;
    }
    MMI_HILOGE("Read pointer file failed for style:%{public}d", cfg.style_);
    return false;
}

image_ptr_t PointerRenderer::LoadPointerImage(uint32_t bufferSide, const RenderConfig &cfg)
{
    auto pixelmap = LoadCursorSvgWithColor(bufferSide, cfg);
    return ExtractDrawingImage(pixelmap);
}

static void ChangeSvgCursorColor(std::string& str, int32_t color)
{
    std::string targetColor = IntToHexRGB(color);
    StringReplace(str, "#000000", targetColor);
    if (color == MAX_POINTER_COLOR) {
        // stroke=\"#FFFFFF" fill="#000000" stroke-linejoin="round" transform="xxx"
        std::regex re("(<path.*)(stroke=\"#[a-fA-F0-9]{6}\")(.*path>)");
        str = std::regex_replace(str, re, "$1stroke=\"#000000\"$3");
    }
}

void SetCursorColorBaseOnStyle(const RenderConfig &cfg, OHOS::Media::DecodeOptions &decodeOpts)
{
    const bool isHandColor =
        (cfg.style_ == HAND_GRABBING) || (cfg.style_ == HAND_OPEN) || (cfg.style_ == HAND_POINTING);
    if (!isHandColor) {
        return;
    }

    if ((cfg.color == MAX_POINTER_COLOR) || (cfg.color == MIN_POINTER_COLOR) || (cfg.color == OTHER_POINTER_COLOR)) {
        decodeOpts.SVGOpts.fillColor = {.isValidColor = true, .color = MAX_POINTER_COLOR};
        decodeOpts.SVGOpts.strokeColor = {.isValidColor = true, .color = MIN_POINTER_COLOR};
        return;
    }

    decodeOpts.SVGOpts.fillColor = {.isValidColor = true, .color = cfg.color};
    if (cfg.color == MAX_POINTER_COLOR) {
        decodeOpts.SVGOpts.strokeColor = {.isValidColor = true, .color = MIN_POINTER_COLOR};
    } else {
        decodeOpts.SVGOpts.strokeColor = {.isValidColor = true, .color = MAX_POINTER_COLOR};
    }
}

pixelmap_ptr_t PointerRenderer::LoadCursorSvgWithColor(uint32_t bufferSide, const RenderConfig &cfg)
{
    std::string svgContent;
    if (!GetPointerFromCache(cfg, svgContent)) {
        MMI_HILOGE("get pointer svg from cache failed");
        return nullptr;
    }

    const bool isPartColor = (cfg.style_ == CURSOR_COPY) || (cfg.style_ == CURSOR_FORBID) || (cfg.style_ == HELP);
    if (isPartColor) {
        ChangeSvgCursorColor(svgContent, cfg.color);
    }
    OHOS::Media::SourceOptions opts;
    std::unique_ptr<std::istream> isp(std::make_unique<std::istringstream>(svgContent));
    uint32_t ret = 0;
    auto imageSource = OHOS::Media::ImageSource::CreateImageSource(std::move(isp), opts, ret);
    if (!imageSource || ret != ERR_OK) {
        MMI_HILOGE("Get ImageSource failed, ret=%{public}u", ret);
    }
    CHKPP(imageSource);

    // Clamp so an unbounded gear-1 dpi cannot decode a glyph larger than the buffer (INV-6).
    int32_t imgSize = cfg.GetClampedImageSize(bufferSide);
    OHOS::Media::DecodeOptions decodeOpts;
    decodeOpts.desiredSize = {.width = imgSize, .height = imgSize};
    if (!isPartColor) {
        decodeOpts.SVGOpts.fillColor = {.isValidColor = true, .color = cfg.color};
        if (cfg.color == MAX_POINTER_COLOR) {
            decodeOpts.SVGOpts.strokeColor = {.isValidColor = true, .color = MIN_POINTER_COLOR};
        } else {
            decodeOpts.SVGOpts.strokeColor = {.isValidColor = true, .color = MAX_POINTER_COLOR};
        }
        SetCursorColorBaseOnStyle(cfg, decodeOpts);
    }

    pixelmap_ptr_t pmap = imageSource->CreatePixelMap(decodeOpts, ret);
    if (ret != ERR_OK) {
        MMI_HILOGE("CreatePixelMap failed, ret=%{public}u", ret);
    }
    return pmap;
}

class PixelMapContext {
public:
    explicit PixelMapContext(pixelmap_ptr_t pixelMap) : pixelMap_(pixelMap) {}
    ~PixelMapContext()
    {
        pixelMap_ = nullptr;
    }

private:
    pixelmap_ptr_t pixelMap_{nullptr};
};

static void PixelMapReleaseProc(const void * /* pixels */, void *context)
{
    PixelMapContext *ctx = static_cast<PixelMapContext *>(context);
    if (ctx != nullptr) {
        delete ctx;
    }
}


static Rosen::Drawing::ColorType PixelFormatToColorType(Media::PixelFormat pixelFormat)
{
    switch (pixelFormat) {
        case Media::PixelFormat::RGB_565:
            return Rosen::Drawing::ColorType::COLORTYPE_RGB_565;
        case Media::PixelFormat::RGBA_8888:
            return Rosen::Drawing::ColorType::COLORTYPE_RGBA_8888;
        case Media::PixelFormat::BGRA_8888:
            return Rosen::Drawing::ColorType::COLORTYPE_BGRA_8888;
        case Media::PixelFormat::ALPHA_8:
            return Rosen::Drawing::ColorType::COLORTYPE_ALPHA_8;
        case Media::PixelFormat::RGBA_F16:
            return Rosen::Drawing::ColorType::COLORTYPE_RGBA_F16;
        case Media::PixelFormat::UNKNOWN:       // fall-though
        case Media::PixelFormat::ARGB_8888:     // fall-though
        case Media::PixelFormat::RGB_888:       // fall-though
        case Media::PixelFormat::NV21:          // fall-though
        case Media::PixelFormat::NV12:          // fall-though
        case Media::PixelFormat::CMYK:          // fall-though
        default:
            return Rosen::Drawing::ColorType::COLORTYPE_UNKNOWN;
    }
}

static Rosen::Drawing::AlphaType AlphaTypeToAlphaType(Media::AlphaType alphaType)
{
    switch (alphaType) {
        case Media::AlphaType::IMAGE_ALPHA_TYPE_UNKNOWN:
            return Rosen::Drawing::AlphaType::ALPHATYPE_UNKNOWN;
        case Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE:
            return Rosen::Drawing::AlphaType::ALPHATYPE_OPAQUE;
        case Media::AlphaType::IMAGE_ALPHA_TYPE_PREMUL:
            return Rosen::Drawing::AlphaType::ALPHATYPE_PREMUL;
        case Media::AlphaType::IMAGE_ALPHA_TYPE_UNPREMUL:
            return Rosen::Drawing::AlphaType::ALPHATYPE_UNPREMUL;
        default:
            return Rosen::Drawing::AlphaType::ALPHATYPE_UNKNOWN;
    }
}

static std::shared_ptr<Rosen::Drawing::ColorSpace> ConvertToColorSpace(Media::ColorSpace colorSpace)
{
    switch (colorSpace) {
        case Media::ColorSpace::DISPLAY_P3:
            return Rosen::Drawing::ColorSpace::CreateRGB(
                Rosen::Drawing::CMSTransferFuncType::SRGB, Rosen::Drawing::CMSMatrixType::DCIP3);
        case Media::ColorSpace::LINEAR_SRGB:
            return Rosen::Drawing::ColorSpace::CreateSRGBLinear();
        case Media::ColorSpace::SRGB:
            return Rosen::Drawing::ColorSpace::CreateSRGB();
        default:
            return Rosen::Drawing::ColorSpace::CreateSRGB();
    }
}

image_ptr_t PointerRenderer::ExtractDrawingImage(pixelmap_ptr_t pixelMap)
{
    CHKPP(pixelMap);
    Media::ImageInfo imageInfo;
    pixelMap->GetImageInfo(imageInfo);
    Rosen::Drawing::ImageInfo drawingImageInfo {
        imageInfo.size.width,
        imageInfo.size.height,
        PixelFormatToColorType(imageInfo.pixelFormat),
        AlphaTypeToAlphaType(imageInfo.alphaType),
        ConvertToColorSpace(imageInfo.colorSpace),
    };
    Rosen::Drawing::Pixmap imagePixmap(drawingImageInfo, reinterpret_cast<const void*>(pixelMap->GetPixels()),
        pixelMap->GetRowStride());
    PixelMapContext *releaseContext = new (std::nothrow) PixelMapContext(pixelMap);
    CHKPP(releaseContext);
    auto image = Rosen::Drawing::Image::MakeFromRaster(imagePixmap, PixelMapReleaseProc, releaseContext);
    if (image == nullptr) {
        MMI_HILOGE("ExtractDrawingImage image fail");
        delete releaseContext;
        releaseContext = nullptr;
    }
    return image;
}

int32_t PointerRenderer::DrawImage(OHOS::Rosen::Drawing::Canvas &canvas, uint32_t width, uint32_t height,
    const RenderConfig &cfg)
{
    if (cfg.style_ == MOUSE_ICON::LOADING) {
        auto loadingImg = FindImg(width, cfg);
        if (loadingImg == nullptr) {
            loadingImg = LoadPointerImage(width, cfg);
            CHKPR(loadingImg, RET_ERR);
            PushImg(width, cfg, loadingImg);
        }
        canvas.Rotate(cfg.rotationAngle, cfg.rotationFocusX, cfg.rotationFocusY);
        canvas.DrawImage(*loadingImg, CalcDrawOriginX(width, cfg), CalcDrawOriginY(height, cfg),
            Rosen::Drawing::SamplingOptions());
    } else {
        RenderConfig runingLCfg = cfg;
        runingLCfg.style_ = MOUSE_ICON::RUNNING_LEFT;
        runingLCfg.align_ = ANGLE_NW;
        runingLCfg.path_ = IMAGE_POINTER_DEFAULT_PATH + "Loading_Left.svg";
        auto runningImgLeft = FindImg(width, runingLCfg);
        if (runningImgLeft == nullptr) {
            runningImgLeft = LoadPointerImage(width, runingLCfg);
            CHKPR(runningImgLeft, RET_ERR);
            PushImg(width, runingLCfg, runningImgLeft);
        }
        CHKPR(runningImgLeft, RET_ERR);
        canvas.DrawImage(*runningImgLeft, CalcDrawOriginX(width, runingLCfg),
            CalcDrawOriginY(height, runingLCfg), Rosen::Drawing::SamplingOptions());

        RenderConfig runingRCfg = cfg;
        runingRCfg.style_ = MOUSE_ICON::RUNNING_RIGHT;
        runingRCfg.align_ = ANGLE_NW;
        runingRCfg.path_ = IMAGE_POINTER_DEFAULT_PATH + "Loading_Right.svg";
        auto runningImgRight = FindImg(width, runingRCfg);
        if (runningImgRight == nullptr) {
            runningImgRight = LoadPointerImage(width, runingRCfg);
            CHKPR(runningImgRight, RET_ERR);
            PushImg(width, runingRCfg, runningImgRight);
        }
        canvas.Rotate(runingRCfg.rotationAngle, runingRCfg.rotationFocusX, runingRCfg.rotationFocusY);
        CHKPR(runningImgRight, RET_ERR);
        canvas.DrawImage(*runningImgRight, CalcDrawOriginX(width, runingRCfg),
            CalcDrawOriginY(height, runingRCfg), Rosen::Drawing::SamplingOptions());
    }
    return RET_OK;
}

int32_t PointerRenderer::DynamicRender(uint8_t *addr, uint32_t width, uint32_t height, const RenderConfig &cfg)
{
    CHKPR(addr, RET_ERR);
    uint32_t addrSize = width * height * RENDER_STRIDE;
    SetPointerCfg(cfg);
    if (cfg.style_ == MOUSE_ICON::TRANSPARENT_ICON) {
        memset_s(addr, addrSize, 0, addrSize);
        return RET_OK;
    }

    OHOS::Rosen::Drawing::Bitmap bitmap;
    OHOS::Rosen::Drawing::BitmapFormat format { OHOS::Rosen::Drawing::COLORTYPE_RGBA_8888,
        OHOS::Rosen::Drawing::ALPHATYPE_OPAQUE };
    if (!bitmap.Build(width, height, format)) {
        MMI_HILOGE("Bitmap build failed");
    }
    OHOS::Rosen::Drawing::Canvas canvas;
    canvas.Bind(bitmap);
    canvas.Clear(OHOS::Rosen::Drawing::Color::COLOR_TRANSPARENT);

    OHOS::Rosen::Drawing::Pen pen;
    pen.SetAntiAlias(true);
    pen.SetColor(OHOS::Rosen::Drawing::Color::COLOR_BLUE);
    OHOS::Rosen::Drawing::scalar penWidth = 1;
    pen.SetWidth(penWidth);
    canvas.AttachPen(pen);

    OHOS::Rosen::Drawing::Brush brush;
    brush.SetColor(Rosen::Drawing::Color::COLOR_TRANSPARENT);
    canvas.DrawBackground(brush);

    if (cfg.direction) {
        int32_t directionFlag = -1;
        int32_t degree = static_cast<int32_t>(directionFlag * static_cast<int32_t>(cfg.direction) * ROTATION_ANGLE90);
        canvas.Rotate(degree, BufferFocus(width), BufferFocus(height));
    }

    if (DrawImage(canvas, width, height, cfg) != RET_OK) {
        return RET_ERR;
    }
    errno_t ret = memcpy_s(addr, addrSize, bitmap.GetPixels(), bitmap.ComputeByteSize());
    if (ret != EOK) {
        return RET_ERR;
    }
    return RET_OK;
}
} // namespace OHOS::MMI