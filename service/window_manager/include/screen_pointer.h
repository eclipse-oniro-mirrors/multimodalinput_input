/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef SCREEN_POINTER_H
#define SCREEN_POINTER_H

#include <atomic>
#include <shared_mutex>

#include "screen_info.h"

#include "hardware_cursor_pointer_manager.h"
#include "pointer_renderer.h"
#include "old_display_info.h"
#include "ui/rs_ui_context.h"
#include "ui/rs_ui_director.h"

namespace OHOS::MMI {
using hwcmgr_ptr_t = std::shared_ptr<HardwareCursorPointerManager>;
using handler_ptr_t = std::shared_ptr<OHOS::AppExecFwk::EventHandler>;
using buffer_ptr_t = sptr<OHOS::SurfaceBuffer>;
using screen_info_ptr_t = sptr<OHOS::Rosen::ScreenInfo>;
using mode_t = OHOS::Rosen::ScreenSourceMode;
using rotation_t = OHOS::Rosen::Rotation;

// Side length of the cursor buffers. DEFAULT_CURSOR_SIZE is used when
// const.multimodalinput.support_blur_cursor is false (or absent); BLUR_CURSOR_SIZE is used when
// that property is true. All paths -- the hard-cursor common/default/transparent buffers, the
// soft-cursor placeholder surface buffer, the draw origin, HotspotInBuffer(), and
// Move()/MoveSoft() -- must agree on the same value.
constexpr int32_t DEFAULT_CURSOR_SIZE{256};
constexpr int32_t BLUR_CURSOR_SIZE{512};

// The single source of truth for the choice above. Both the ScreenPointer that allocates the
// buffers and the drawing path that requests a soft placeholder buffer call this rather than
// reading the property themselves, so the property name, its default and the tier mapping exist in
// exactly one place. The property is const.-prefixed (read-only, fixed at boot), so the value is
// resolved once and cached; callers may invoke this per screen or per frame.
int32_t GetCursorBufferSize();

uint32_t GetScreenInfoWidth(const screen_info_ptr_t);
uint32_t GetScreenInfoHeight(const screen_info_ptr_t);

class ScreenPointer final {
public:
    static bool IsROGEnable(uint32_t physicalWidth, uint32_t physicalHeight,
        uint32_t logicalWidth, uint32_t logicalHeight);
    struct ScreenLiteInfo {
        uint32_t width { 0 };
        uint32_t height { 0 };
        rotation_t rotation { rotation_t::ROTATION_0 };
    };
    static ScreenLiteInfo ResolveScreenLiteInfo(const screen_info_ptr_t &si);
public:
    DISALLOW_COPY_AND_MOVE(ScreenPointer);
    ScreenPointer(hwcmgr_ptr_t hwcmgr, handler_ptr_t handler, const OLD::DisplayInfo &di);
    ScreenPointer(hwcmgr_ptr_t hwcmgr, handler_ptr_t handler, screen_info_ptr_t si);
    ~ScreenPointer();

    bool Init(PointerRenderer &render, bool needDrawPointer = true);
    bool InitSurface(bool needDrawPointer);
    void UpdateScreenInfo(screen_info_ptr_t si, bool needDrawPointer);
    bool UpdatePadding(uint32_t sourceScreenWidth, uint32_t sourceScreenHeight);
    void OnDisplayInfo(const OLD::DisplayInfo &di);

    buffer_ptr_t GetDefaultBuffer();
    buffer_ptr_t GetTransparentBuffer();
    buffer_ptr_t GetCommonBuffer();
    buffer_ptr_t RequestBuffer(const RenderConfig &cfg, bool &isCommonBuffer);
    // Publishes the completed frame: atomically commits the hotspot and currentBuffer_ under
    // bufferMutex_ so that Move()/MoveSoft() always read a matched pair.  Must be called after
    // Render() and FlushCache() succeed — never before — to prevent MoveRetry from reading a
    // slot whose pixels haven't been written yet.
    void PublishFrame(const RenderConfig &cfg, buffer_ptr_t buffer);
    buffer_ptr_t GetCurrentBuffer();
    // Caches the hotspot for the given path. isHard selects which copy to update: the two paths
    // run on different threads with their own RenderConfig and must not share one value.
    // bufferSide is the side of the buffer the image was actually drawn into, which is not always
    // DEFAULT_CURSOR_SIZE.
    void UpdateHotspotInBuffer(const RenderConfig &cfg, bool isHard, int32_t bufferSide);

    bool Move(int32_t x, int32_t y);
    bool MoveSoft(int32_t x, int32_t y);
    void CalculatePositionForMirror(int32_t x, int32_t y, int32_t* px, int32_t* py);
    bool SetInvisible();

    uint64_t GetScreenId() const
    {
        return screenId_;
    }

    // Side length of the cursor buffers owned by this ScreenPointer. Callers that compute a
    // rotation pivot must derive it from this rather than a fixed constant, so the pivot stays
    // inside the buffer when the hard cursor buffer shrinks.
    uint32_t GetBufferSize() const;

    // Returns true when the buffer pool was never successfully built, or was cleared by a failed
    // rebuild. Callers that gate re-init on GetBufferSize()==ResolveBufferSide() must also check
    // this: a failed rebuild leaves bufferSize_ at desiredSide (so the tier comparison is equal)
    // while the pool is empty, and without this guard the re-init would never retry.
    bool IsPoolEmpty() const;

    // Resolves the buffer side this screen requires from its current mode and off-screen state.
    // Mirror screens and off-screen-rendering extended screens scale the cursor up (GetScale() > 1),
    // so the upscaled glyph no longer fits a 256 buffer and 512 is required; every other screen
    // follows const.multimodalinput.support_blur_cursor. Evaluated on every mode/display-info change
    // rather than once in Init(), because the off-screen flag and the mode both arrive after
    // construction.
    uint32_t ResolveBufferSide();

    uint32_t GetScreenWidth() const
    {
        return width_;
    }

    uint32_t GetScreenHeight() const
    {
        return height_;
    }

    std::shared_ptr<OHOS::Rosen::RSSurfaceNode> GetSurfaceNode()
    {
        return surfaceNode_;
    }

    void SetDPI(float dpi)
    {
        dpi_ = dpi;
    }

    float GetDPI() const
    {
        return dpi_;
    }

    float GetScale() const
    {
        if (IsMain()) {
            return GetIsCurrentOffScreenRendering() ? offRenderScale_ : scale_;
        } else if (IsExtend()) {
            return offRenderScale_;
        } else if (IsMirror()) {
            return scale_;
        }
        return 1.0f;
    }

    uint32_t GetMode() const
    {
        return static_cast<uint32_t>(mode_);
    }

    bool IsMain() const
    {
        return mode_ == mode_t::SCREEN_MAIN;
    }

    bool IsMirror() const
    {
        return mode_ == mode_t::SCREEN_MIRROR;
    }

    bool IsExtend() const
    {
        return mode_ == mode_t::SCREEN_EXTEND;
    }

    bool GetIsCurrentOffScreenRendering() const
    {
        return isCurrentOffScreenRendering_;
    }

    void SetSourceScreenRotation(const rotation_t sourceScreenRotation)
    {
        sourceScreenRotation_ = sourceScreenRotation;
    }
 
    rotation_t GetSourceScreenRotation()
    {
        return sourceScreenRotation_;
    }

    rotation_t GetRotation()
    {
        return rotation_;
    }

    void SetDirectionAndDisplayDirection(Direction direction, Direction displayDirection);

    Direction GetDirection();

    Direction GetDisplayDirection();

    bool IsPositionOutScreen(int32_t x, int32_t y);

    uint32_t GetBufferId()
    {
        return bufferId_.load(std::memory_order_relaxed);
    }

    void SetMirrorWidth(const uint32_t mirrorWidth)
    {
        mirrorWidth_ = mirrorWidth;
    }

    uint32_t GetMirrorWidth() const
    {
        return mirrorWidth_;
    }

    void SetMirrorHeight(const uint32_t mirrorHeight)
    {
        mirrorHeight_ = mirrorHeight;
    }

    uint32_t GetMirrorHeight() const
    {
        return mirrorHeight_;
    }
    void SetVirtualExtend(bool isVirtualExtend)
    {
        isVirtualExtend_ = isVirtualExtend;
    }

    std::shared_ptr<Rosen::RSUIContext> GetRSUIContext() const
    {
        return rsUIContext_;
    }

    void RsFlushImplicitTransaction();

    void DestroyPointerWindow();

private:
    bool InitSurfaceNode();
    bool FlushSerfaceBuffer();
    void Rotate(rotation_t rotation, int32_t& x, int32_t& y, int32_t width, int32_t height);
    void CalculateHwcPositionForMirror(int32_t& x, int32_t& y);
    void CalculateHwcPositionForExtend(int32_t& x, int32_t& y);
    void CalculateHwcPositionForMain(int32_t& x, int32_t& y);
    bool InitDefaultBuffer(const OHOS::BufferRequestConfig &bufferCfg, PointerRenderer &render);
    bool InitTransparentBuffer(const OHOS::BufferRequestConfig &bufferCfg);
    bool InitCommonBuffer(const OHOS::BufferRequestConfig &bufferCfg);
    buffer_ptr_t CreateSurfaceBuffer(const OHOS::BufferRequestConfig &bufferCfg);
    bool IsDefaultCfg(const RenderConfig &cfg);
    bool InitRSUIContext(uint64_t screenId);

private:
    std::mutex mtx_;

    uint64_t screenId_{0};
    uint32_t width_{0};
    uint32_t height_{0};
    uint32_t mirrorWidth_{0};
    uint32_t mirrorHeight_{0};
    mode_t mode_{mode_t::SCREEN_MAIN};
    std::atomic<rotation_t> rotation_{rotation_t::ROTATION_0};
    float dpi_{1.0f};
    std::atomic<Direction> direction_{DIRECTION0};
    std::atomic<Direction> displayDirection_{DIRECTION0};

    // screen scale and padding info for mirror mode
    float scale_{1.0f};
    int32_t paddingTop_{0};
    int32_t paddingLeft_{0};
    rotation_t sourceScreenRotation_ {rotation_t::ROTATION_0};

    hwcmgr_ptr_t hwcMgr_{nullptr};
    handler_ptr_t handler_{nullptr};

    // RS Layer
    std::shared_ptr<OHOS::Rosen::RSSurfaceNode> surfaceNode_{nullptr};
    std::shared_ptr<OHOS::Rosen::RSCanvasNode> canvasNode_{nullptr};
    std::shared_ptr<OHOS::Rosen::RSUIDirector> rsUIDirector_{nullptr};
    std::shared_ptr<OHOS::Rosen::RSUIContext> rsUIContext_{nullptr};

    RenderConfig defaultCursorCfg_;
    // Resolved once in Init() from const.multimodalinput.support_blur_cursor: BLUR_CURSOR_SIZE when
    // true, DEFAULT_CURSOR_SIZE otherwise. All paths that allocate or size a buffer, compute a
    // hotspot, or position a node must derive their side from this member rather than from the
    // compile-time constant, so they all agree as a set.
    uint32_t bufferSize_ {DEFAULT_CURSOR_SIZE};
    // Hotspot position within the rendered buffer, cached for Move()/MoveSoft(), which receive only
    // a position. Held per path: the two are driven from different threads with their own
    // RenderConfig (the soft path renders at the screen dpi and has its own direction), so a single
    // pair let one path's write reposition the other's cursor -- e.g. at direction 90 the hard and
    // soft hotspots differ by the half-buffer term and the hard cursor would jump by that amount.
    // Initialised to the buffer centre so behaviour before the first render matches the old
    // unconditional half-buffer offset.
    int32_t hotspotHardX_{DEFAULT_CURSOR_SIZE / 2};
    int32_t hotspotHardY_{DEFAULT_CURSOR_SIZE / 2};
    int32_t hotspotSoftX_{DEFAULT_CURSOR_SIZE / 2};
    int32_t hotspotSoftY_{DEFAULT_CURSOR_SIZE / 2};
    buffer_ptr_t transparentBuffer_{nullptr};
    buffer_ptr_t defaultBuffer_{nullptr};
    mutable std::shared_mutex bufferMutex_;
    buffer_ptr_t currentBuffer_{nullptr};
    std::vector<buffer_ptr_t> commonBuffers_;
    std::atomic<uint32_t> bufferId_ {0};

    // isCurrentOffScreenRendering
    bool isCurrentOffScreenRendering_ = false;
    float offRenderScale_{1.0f};
    bool isVirtualExtend_ = false;
};

} // namespace OHOS::MMI

#endif // SCREEN_POINTER_H