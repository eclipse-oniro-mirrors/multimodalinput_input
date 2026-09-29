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
#include "screen_pointer.h"
#include <numeric>
#include "bytrace_adapter.h"
#include "define_multimodal.h"
#include "transaction/rs_transaction.h"
#include "transaction/rs_interfaces.h"
#include "product_type_parser.h"
#include "product_name_definition.h"
#include "display_manager_lite.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_CURSOR
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "ScreenPointer"

namespace OHOS::MMI {
const char* RS_SURFACE_NODE_NAME{"pointer window"};
const char* POINTER_SIZE{"pointerSize"};
constexpr int32_t RS_NODE_CANVAS_INDEX{-1};
constexpr int32_t DEFAULT_POINTER_SIZE{1};
constexpr int32_t DEVICE_INDEPENDENT_PIXELS{40};
constexpr int32_t BASELINE_DENSITY{160};
constexpr int32_t POINTER_WINDOW_INIT_SIZE{64};
constexpr int32_t BUFFER_RELEASE_WAIT_MS{1000};
constexpr int32_t NUM_TWO{2};
constexpr int32_t DEFAULT_BUFFER_SIZE{8};
constexpr int32_t BUFFER_TIMEOUT{150};
constexpr int32_t STRIDE_ALIGNMENT{8};
constexpr uint32_t RENDER_STRIDE{4};
constexpr uint32_t POINTER_SIZE_DEFAULT { 1 };
constexpr uint32_t POINTER_SIZE_FOLD_PC { 2 };
constexpr int32_t ANGLE_90 { 90 };
constexpr int32_t ANGLE_360 { 360 };

/*
 * 获取当前屏幕宽，该宽不跟随屏幕旋转
 * 使用"hidumper -s 10 -a screen"查看GetModes值
 */
uint32_t GetScreenInfoWidth(screen_info_ptr_t si)
{
    uint32_t width = 0;
    auto modeId = si->GetModeId();
    auto modes = si->GetModes();
    if (modeId < 0 || modeId >= modes.size()) {
        return 0;
    }
    return modes[modeId]->width_;
}

/*
 * 获取当前屏幕高，该高不跟随屏幕旋转
 * 使用"hidumper -s 10 -a screen"查看GetModes值
 */
uint32_t GetScreenInfoHeight(screen_info_ptr_t si)
{
    uint32_t height = 0;
    auto modeId = si->GetModeId();
    auto modes = si->GetModes();
    if (modeId < 0 || modeId >= modes.size()) {
        return 0;
    }
    return modes[modeId]->height_;
}

bool ScreenPointer::IsROGEnable(uint32_t physicalWidth, uint32_t physicalHeight, uint32_t logicalWidth,
    uint32_t logicalHeight)
{
    if (physicalWidth == 0 || physicalHeight == 0 || logicalWidth == 0 || logicalHeight == 0) {
        return false;
    }
    if (physicalWidth == logicalWidth || physicalHeight == logicalHeight) {
        return false;
    }
    uint32_t gcdP = std::gcd(physicalWidth, physicalHeight);
    uint32_t gcdL = std::gcd(logicalWidth, logicalHeight);
    return (physicalWidth * gcdL ==  logicalWidth * gcdP) && (physicalHeight * gcdL == logicalHeight * gcdP);
}

ScreenPointer::ScreenLiteInfo ScreenPointer::ResolveScreenLiteInfo(const screen_info_ptr_t &si)
{
    ScreenLiteInfo result;
    if (si == nullptr) {
        MMI_HILOGE("si is nullptr");
        return result;
    }
    uint32_t physicalWidth = GetScreenInfoWidth(si);
    uint32_t physicalHeight = GetScreenInfoHeight(si);
    result.width = physicalWidth;
    result.height = physicalHeight;
    result.rotation = si->GetRotation();
    const std::string product = OHOS::system::GetParameter("const.build.product", SYS_GET_DEVICE_TYPE_PARAM);
    if (std::find(DEVICE_TYPE_TABLET_A1_VECTOR.begin(), DEVICE_TYPE_TABLET_A1_VECTOR.end(), product) ==
        DEVICE_TYPE_TABLET_A1_VECTOR.end()) {
        return result;
    }
    if (si->GetScreenTypeInfo() != OHOS::Rosen::ScreenTypeInfo::BUILT_IN) {
        MMI_HILOGI("screenId=%{public}" PRIu64 ", not BUILT_IN, use physical", si->GetRsId());
        return result;
    }
    auto displayLite = OHOS::Rosen::DisplayManagerLite::GetInstance().GetDisplayById(si->GetScreenId());
    if (displayLite == nullptr) {
        MMI_HILOGI("screenId=%{public}" PRIu64 ", BUILT_IN but displayLite is nullptr", si->GetRsId());
        return result;
    }
    uint32_t logicalWidth = static_cast<uint32_t>(displayLite->GetWidth());
    uint32_t logicalHeight = static_cast<uint32_t>(displayLite->GetHeight());
    rotation_t logicalRotation = displayLite->GetRotation();
    if (logicalRotation == rotation_t::ROTATION_90 || logicalRotation == rotation_t::ROTATION_270) {
        std::swap(logicalWidth, logicalHeight);
    }
    bool isROG = IsROGEnable(physicalWidth, physicalHeight, logicalWidth, logicalHeight);
    if (isROG) {
        result.width = logicalWidth;
        result.height = logicalHeight;
        result.rotation = logicalRotation;
    }
    MMI_HILOGI("ResolveScreenLiteInfo, rsId=%{public}" PRIu64 ", BUILT_IN, physical=(%{public}u, %{public}u),"
        "logical=(%{public}u, %{public}u), result=(%{public}u, %{public}u), rotation=%{public}u, ROG=%{public}d",
        si->GetRsId(), physicalWidth, physicalHeight, logicalWidth, logicalHeight, result.width, result.height,
        static_cast<uint32_t>(result.rotation), isROG);
    return result;
}

ScreenPointer::ScreenPointer(hwcmgr_ptr_t hwcMgr, handler_ptr_t handler, const OLD::DisplayInfo &di)
    : hwcMgr_(hwcMgr), handler_(handler)
{
    screenId_ = di.rsId;
    width_ = di.width;
    height_ = di.height;
    rotation_ = static_cast<rotation_t>(di.direction);
    if (rotation_ == rotation_t::ROTATION_90 ||
        rotation_ == rotation_t::ROTATION_270) {
        std::swap(width_, height_);
    }
    dpi_ = float(di.dpi) / BASELINE_DENSITY;
    bufferSize_ = static_cast<uint32_t>(GetCursorBufferSize());
    MMI_HILOGI("Construct with DisplayInfo, id=%{public}" PRIu64 ", shape=(%{public}u, %{public}u), mode=%{public}u, "
        "rotation=%{public}u, dpi=%{public}f", screenId_, width_, height_, mode_, rotation_.load(), dpi_);
}

ScreenPointer::ScreenPointer(hwcmgr_ptr_t hwcMgr, handler_ptr_t handler, screen_info_ptr_t si)
    : hwcMgr_(hwcMgr), handler_(handler)
{
    screenId_ = si->GetRsId();
    mode_ = si->GetSourceMode();
    dpi_ = si->GetVirtualPixelRatio();
#ifdef OHOS_BUILD_EXTERNAL_SCREEN
    mirrorWidth_ = si->GetMirrorWidth();
    mirrorHeight_ = si->GetMirrorHeight();
#endif // OHOS_BUILD_EXTERNAL_SCREEN
    auto result = ResolveScreenLiteInfo(si);
    width_ = result.width;
    height_ = result.height;
    rotation_ = result.rotation;
    bufferSize_ = static_cast<uint32_t>(GetCursorBufferSize());
    MMI_HILOGI("Construct with ScreenInfo, id=%{public}" PRIu64 ", shape=(%{public}u, %{public}u), mode=%{public}u, "
        "rotation=%{public}u, dpi=%{public}f", screenId_, width_, height_, mode_, rotation_.load(), dpi_);
}

ScreenPointer::~ScreenPointer()
{
    DestroyPointerWindow();
    MMI_HILOGI("Destruct ScreenPointer, id:%{public}" PRIu64, screenId_);
}

bool ScreenPointer::Init(PointerRenderer &render, bool needDrawPointer)
{
    // Mirror screens and off-screen-rendering extended screens scale the cursor up
    // (GetScale() > 1), so the upscaled glyph no longer fits a 256 buffer. The tier can change
    // after construction -- the off-screen flag and the screen mode both arrive later -- so
    // rebuild the pool whenever the resolved side differs from the one it was built at.
    // InitCommonBuffer only tops the pool up to DEFAULT_BUFFER_SIZE, so without this the eight
    // common buffers would keep their old side while default/transparent were rebuilt, leaving
    // the hotspot (computed from bufferSize_) out of step with the pixels actually drawn.
    uint32_t desiredSide = ResolveBufferSide();
    uint32_t prevBufferSize = bufferSize_;
    if (desiredSide != bufferSize_) {
        MMI_HILOGI("Buffer side change %{public}u -> %{public}u, rebuilding pool",
            bufferSize_, desiredSide);
        {
            std::unique_lock<std::shared_mutex> lock(bufferMutex_);
            currentBuffer_ = nullptr;
            defaultBuffer_ = nullptr;
            transparentBuffer_ = nullptr;
            commonBuffers_.clear();
            // Write bufferSize_ inside the exclusive lock so soft-render thread reads via
            // GetBufferSize() are always consistent with the pool state they observe.
            bufferSize_ = desiredSide;
        }
    }
    if (!InitSurface(needDrawPointer)) {
        MMI_HILOGE("ScreenPointer InitSurface failed");
        bufferSize_ = prevBufferSize;
        return false;
    }

    RenderConfig defaultCursorCfg {
        .style_ = MOUSE_ICON::DEFAULT,
        .align_ = ICON_TYPE::ANGLE_NW,
        .path_ = "/data/service/el1/public/multimodalinput/mouse_icon/Default.svg",
        .color = 0,
        .size = POINTER_SIZE_DEFAULT,
        .direction = Direction::DIRECTION0,
        .dpi = this->GetDPI() * this->GetScale(),
        .isHard = true,
    };
    if (OHOS::system::GetParameter("const.build.product", SYS_GET_DEVICE_TYPE_PARAM) == DEVICE_TYPE_FOLD_PC) {
        defaultCursorCfg.size = POINTER_SIZE_FOLD_PC;
    }
    defaultCursorCfg_ = defaultCursorCfg;

    // Init buffers
    OHOS::BufferRequestConfig bufferCfg = {
        .width = static_cast<int32_t>(bufferSize_),
        .height = static_cast<int32_t>(bufferSize_),
        .strideAlignment = STRIDE_ALIGNMENT,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_HW_COMPOSER
            | BUFFER_USAGE_MEM_MMZ_CACHE,
        .timeout = BUFFER_TIMEOUT,
    };
    if (!InitDefaultBuffer(bufferCfg, render)) {
        MMI_HILOGE("ScreenPointer InitDefaultBuffer failed");
        {
            std::unique_lock<std::shared_mutex> lock(bufferMutex_);
            defaultBuffer_ = nullptr;
            transparentBuffer_ = nullptr;
            commonBuffers_.clear();
            currentBuffer_ = nullptr;
        }
        // Do NOT restore prevBufferSize: leaving bufferSize_=desiredSide ensures the next
        // GetBufferSize()==ResolveBufferSide() comparison is still equal, so IsPoolEmpty()
        // is required alongside that check to detect and retry a failed rebuild.
        return false;
    }
    if (!InitTransparentBuffer(bufferCfg)) {
        MMI_HILOGE("ScreenPointer InitTransparentBuffer failed");
        {
            std::unique_lock<std::shared_mutex> lock(bufferMutex_);
            defaultBuffer_ = nullptr;
            transparentBuffer_ = nullptr;
            commonBuffers_.clear();
            currentBuffer_ = nullptr;
        }
        return false;
    }
    if (!InitCommonBuffer(bufferCfg)) {
        MMI_HILOGE("ScreenPointer InitCommonBuffer failed");
        {
            std::unique_lock<std::shared_mutex> lock(bufferMutex_);
            defaultBuffer_ = nullptr;
            transparentBuffer_ = nullptr;
            commonBuffers_.clear();
            currentBuffer_ = nullptr;
        }
        return false;
    }
    // Advance bufferId_ to seed the round-robin starting point for the first real draw call,
    // but do NOT publish currentBuffer_ here: the common buffers are freshly allocated and have
    // never been rendered into, so making one the current buffer would cause Move() to submit
    // blank pixels until the first DrawHardCursor() completes and calls PublishFrame().
    {
        std::unique_lock<std::shared_mutex> lock(bufferMutex_);
        uint32_t poolSize = static_cast<uint32_t>(commonBuffers_.size());
        if (poolSize > 0) {
            uint32_t newId = bufferId_.load(std::memory_order_relaxed) + 1;
            newId %= poolSize;
            bufferId_.store(newId, std::memory_order_relaxed);
        }
    }
    return true;
}

bool ScreenPointer::InitDefaultBuffer(const OHOS::BufferRequestConfig &bufferCfg, PointerRenderer &render)
{
    buffer_ptr_t buffer = CreateSurfaceBuffer(bufferCfg);
    CHKPF(buffer);
    auto addr = static_cast<uint8_t *>(buffer->GetVirAddr());
    CHKPF(addr);
    int32_t ret = render.Render(addr, buffer->GetWidth(), buffer->GetHeight(), defaultCursorCfg_);
    if (ret != RET_OK) {
        MMI_HILOGE("Render failed, ret:%{public}d.", ret);
        return false;
    }
    std::unique_lock<std::shared_mutex> lock(bufferMutex_);
    defaultBuffer_ = buffer;
    return true;
}

bool ScreenPointer::InitTransparentBuffer(const OHOS::BufferRequestConfig &bufferCfg)
{
    buffer_ptr_t buffer = CreateSurfaceBuffer(bufferCfg);
    CHKPF(buffer);
    auto addr = static_cast<uint8_t *>(buffer->GetVirAddr());
    CHKPF(addr);

    uint32_t addrSize = static_cast<uint32_t>(buffer->GetWidth() * buffer->GetHeight()) * RENDER_STRIDE;
    (void)memset_s(addr, addrSize, 0, addrSize);
    std::unique_lock<std::shared_mutex> lock(bufferMutex_);
    transparentBuffer_ = buffer;
    return true;
}

bool ScreenPointer::InitCommonBuffer(const OHOS::BufferRequestConfig &bufferCfg)
{
    // Allocate all buffers outside the lock (slow allocation), then commit atomically.
    std::vector<buffer_ptr_t> newBuffers;
    newBuffers.reserve(DEFAULT_BUFFER_SIZE);
    for (int32_t i = 0; i < DEFAULT_BUFFER_SIZE; i++) {
        buffer_ptr_t buffer = CreateSurfaceBuffer(bufferCfg);
        CHKPF(buffer);
        newBuffers.push_back(buffer);
    }
    std::unique_lock<std::shared_mutex> lock(bufferMutex_);
    commonBuffers_ = std::move(newBuffers);
    return true;
}

buffer_ptr_t ScreenPointer::GetDefaultBuffer()
{
    std::shared_lock<std::shared_mutex> lock(bufferMutex_);
    return defaultBuffer_;
}

buffer_ptr_t ScreenPointer::GetTransparentBuffer()
{
    std::shared_lock<std::shared_mutex> lock(bufferMutex_);
    return transparentBuffer_;
}

buffer_ptr_t ScreenPointer::GetCommonBuffer()
{
    std::unique_lock<std::shared_mutex> lock(bufferMutex_);
    uint32_t bufferSize = static_cast<uint32_t>(commonBuffers_.size());
    if (bufferSize != DEFAULT_BUFFER_SIZE) {
        MMI_HILOGE("The buffer size is incorrect, size:%{public}u", bufferSize);
        return nullptr;
    }
    uint32_t newId = bufferId_.load(std::memory_order_relaxed) + 1;
    newId %= bufferSize;
    bufferId_.store(newId, std::memory_order_relaxed);
    currentBuffer_ = commonBuffers_[newId];
    return commonBuffers_[newId];
}

void ScreenPointer::UpdateHotspotInBuffer(const RenderConfig &cfg, bool isHard, int32_t bufferSide)
{
    int32_t side = (bufferSide > 0) ? bufferSide : DEFAULT_CURSOR_SIZE;
    int32_t x, y;
    if (side == BLUR_CURSOR_SIZE) {
        // 512-tier: draw origin is BufferFocus - GetFocusX/Y; hotspot = origin + focus = side/2.
        x = side / 2;
        y = side / 2;
    } else {
        x = cfg.HotspotInBufferX(side);
        y = cfg.HotspotInBufferY(side);
    }
    // Written under bufferMutex_ so the hotspot and the buffer it belongs to are published as
    // one frame. Move()/MoveSoft() take the same lock and read both together; without it the
    // retry thread could pair a newly cached hotspot with the previous buffer (a 256-tier
    // DEFAULT -> LOADING switch shifts the hotspot 128 -> 108 and moves the arrow by 20px).
    std::unique_lock<std::shared_mutex> lock(bufferMutex_);
    if (isHard) {
        hotspotHardX_ = x;
        hotspotHardY_ = y;
    } else {
        hotspotSoftX_ = x;
        hotspotSoftY_ = y;
    }
}

buffer_ptr_t ScreenPointer::RequestBuffer(const RenderConfig &cfg, bool &isCommonBuffer)
{
    // Each branch takes the lock once to read bufferSize_ alongside the pool access, ensuring
    // the hotspot is computed from the same tier as the buffer.  The frame is NOT published here
    // — currentBuffer_ and hotspot are committed only after FlushCache succeeds (see PublishFrame).
    if (cfg.style_ == MOUSE_ICON::TRANSPARENT_ICON) {
        MMI_HILOGD("The buffer transparent buffer.");
        isCommonBuffer = false;
        std::shared_lock<std::shared_mutex> lock(bufferMutex_);
        return transparentBuffer_;
    }
    if (IsDefaultCfg(cfg)) {
        MMI_HILOGD("The buffer default buffer.");
        isCommonBuffer = false;
        std::shared_lock<std::shared_mutex> lock(bufferMutex_);
        return defaultBuffer_;
    }
    // Common buffer: size check, id advance under one exclusive lock.
    isCommonBuffer = true;
    std::unique_lock<std::shared_mutex> lock(bufferMutex_);
    uint32_t poolSize = static_cast<uint32_t>(commonBuffers_.size());
    if (poolSize != DEFAULT_BUFFER_SIZE) {
        MMI_HILOGE("The buffer size is incorrect, size:%{public}u", poolSize);
        return nullptr;
    }
    uint32_t newId = bufferId_.load(std::memory_order_relaxed) + 1;
    newId %= poolSize;
    bufferId_.store(newId, std::memory_order_relaxed);
    return commonBuffers_[newId];
}

void ScreenPointer::PublishFrame(const RenderConfig &cfg, buffer_ptr_t buffer)
{
    // Derive the hotspot from the buffer's own side length, not bufferSize_, so a concurrent
    // tier-change rebuild cannot cause a mismatch: if a 256-tier buffer is flushed while a 512
    // rebuild completes, the hotspot is still computed from 256 (the buffer that was drawn into).
    if (!buffer) {
        return;
    }
    int32_t side = buffer->GetWidth();
    if (side <= 0) {
        side = DEFAULT_CURSOR_SIZE;
    }
    std::unique_lock<std::shared_mutex> lock(bufferMutex_);
    hotspotHardX_ = (side == BLUR_CURSOR_SIZE) ? (side / 2) : cfg.HotspotInBufferX(side);
    hotspotHardY_ = (side == BLUR_CURSOR_SIZE) ? (side / 2) : cfg.HotspotInBufferY(side);
    currentBuffer_ = buffer;
}

// Decides whether a frame can be served the one-shot defaultBuffer_, which InitDefaultBuffer renders
// exactly once from defaultCursorCfg_ (DIRECTION0, no blur) and is never redrawn afterwards --
// DrawHardCursor skips Render() for it. Anything this rejects falls through to commonBuffers_ and is
// re-rendered every frame.
//
// RenderConfig::operator== only compares style_, GetImageSize() and color, so each term below has to
// be spelled out; every one of them changes the pixels while leaving operator== equal:
//   - direction: a rotated screen turns the glyph about the buffer focus, so DIRECTION0's cached
//     image is wrong. Rotating away drops to the per-frame path and rotating back re-enters this one.
//   - isBlur: the trail is composed per frame from the pointer's own motion, so a blurred frame can
//     never reuse a cached image. Omitting this term would hand moving cursors the static buffer and
//     the trail would never appear at all.
//   - align_/isHard: change the draw origin and the target buffer pool respectively.
bool ScreenPointer::IsDefaultCfg(const RenderConfig &cfg)
{
    return (cfg == defaultCursorCfg_) && (cfg.direction == defaultCursorCfg_.direction)
        && (cfg.align_ == defaultCursorCfg_.align_) && (cfg.isHard == defaultCursorCfg_.isHard)
        && (cfg.isBlur == defaultCursorCfg_.isBlur);
}

buffer_ptr_t ScreenPointer::GetCurrentBuffer()
{
    std::shared_lock<std::shared_mutex> lock(bufferMutex_);
    return currentBuffer_;
}

buffer_ptr_t ScreenPointer::CreateSurfaceBuffer(const OHOS::BufferRequestConfig &bufferCfg)
{
    buffer_ptr_t buffer = OHOS::SurfaceBuffer::Create();
    if (buffer == nullptr) {
        MMI_HILOGE("SurfaceBuffer Create failed");
        return nullptr;
    }
    OHOS::GSError ret = buffer->Alloc(bufferCfg);
    if (ret != OHOS::GSERROR_OK) {
        MMI_HILOGE("SurfaceBuffer Alloc failed, %{public}s", GSErrorStr(ret).c_str());
        return nullptr;
    }
    return buffer;
}

bool ScreenPointer::InitSurface(bool needDrawPointer)
{
    // Null the old node under bufferMutex_ first, so a concurrent MoveSoft() that snapshots
    // surfaceNode_ under the same lock sees nullptr and returns early rather than using a
    // node that is about to be destroyed.
    {
        std::unique_lock<std::shared_mutex> lock(bufferMutex_);
        surfaceNode_ = nullptr;
    }
    RsFlushImplicitTransaction();

    // create RSUIDirector and RSUIContext
    if (!InitRSUIContext(screenId_)) {
        MMI_HILOGE("Init RSUIContext fail");
        return false;
    }

    // Build the SurfaceNode into a local variable; publish it under bufferMutex_ only after
    // it is fully configured so MoveSoft() never snapshots a half-initialised node.
    std::shared_ptr<Rosen::RSSurfaceNode> newNode;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = RS_SURFACE_NODE_NAME;
    newNode = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, Rosen::RSSurfaceNodeType::CURSOR_NODE,
        true, false, rsUIContext_);
    CHKPF(newNode);
    newNode->SetSkipCheckInMultiInstance(true);
    MMI_HILOGI("SurfaceNode::Create success");

    auto surface = newNode->GetSurface();
    CHKPF(surface);
    surface->SetQueueSize(DEFAULT_BUFFER_SIZE);

    newNode->SetVisible(true);
    newNode->SetSnapshotSkipLayer(true);
    newNode->SetFrameGravity(Rosen::Gravity::TOP_LEFT);
    newNode->SetPositionZ(Rosen::RSSurfaceNode::POINTER_WINDOW_POSITION_Z);
    if (needDrawPointer) {
        newNode->AttachToDisplay(screenId_);
    }
    newNode->SetBounds(0, 0, static_cast<int32_t>(bufferSize_), static_cast<int32_t>(bufferSize_));
    MMI_HILOGI("AttachToDisplay %{public}" PRIu64 " completed, needDrawPointer=%{public}d", screenId_, needDrawPointer);

    canvasNode_ = Rosen::RSCanvasNode::Create(false, false, rsUIContext_);
    CHKPF(canvasNode_);
    canvasNode_->SetBounds(0, 0, static_cast<int32_t>(bufferSize_), static_cast<int32_t>(bufferSize_));
    canvasNode_->SetFrame(0, 0, static_cast<int32_t>(bufferSize_), static_cast<int32_t>(bufferSize_));
#ifndef USE_ROSEN_DRAWING
    canvasNode_->SetBackgroundColor(SK_ColorTRANSPARENT);
#else
    canvasNode_->SetBackgroundColor(Rosen::Drawing::Color::COLOR_TRANSPARENT);
#endif // USE_ROSEN_DRAWING
    canvasNode_->SetCornerRadius(1);
    canvasNode_->SetPositionZ(Rosen::RSSurfaceNode::POINTER_WINDOW_POSITION_Z);
    canvasNode_->SetRotation(float(rotation_.load()));
    newNode->AddChild(canvasNode_, RS_NODE_CANVAS_INDEX);

    // Publish the fully-configured node atomically so MoveSoft() snapshots a complete node.
    {
        std::unique_lock<std::shared_mutex> lock(bufferMutex_);
        surfaceNode_ = std::move(newNode);
    }

    MMI_HILOGI("InitSurface completed");
    return true;
}

void ScreenPointer::UpdateScreenInfo(sptr<OHOS::Rosen::ScreenInfo> si, bool needDrawPointer)
{
    CHKPV(si);
    // Update all screen metadata unconditionally — mode_, width_, height_, dpi_ etc. must
    // reflect the current screen regardless of whether the surface node exists.  If a previous
    // Init() failed and left the node null, the IsPoolEmpty() gate will trigger a retry; that
    // retry must see the correct mode or it will rebuild a 512 pool for a mirror that has since
    // reverted to extend mode.
    screenId_ = si->GetRsId();
    mode_ = si->GetSourceMode();
    dpi_ = si->GetVirtualPixelRatio();
#ifdef OHOS_BUILD_EXTERNAL_SCREEN
    mirrorWidth_ = si->GetMirrorWidth();
    mirrorHeight_ = si->GetMirrorHeight();
#endif // OHOS_BUILD_EXTERNAL_SCREEN
    auto result = ResolveScreenLiteInfo(si);
    width_ = result.width;
    height_ = result.height;
    rotation_ = result.rotation;
    // Node operations only when the node is present.
    auto node = GetSurfaceNode();
    if (needDrawPointer && node != nullptr) {
        node->RemoveFromTree();
        node->AttachToDisplay(screenId_);
    }
    RsFlushImplicitTransaction();
    MMI_HILOGI("Update with ScreenInfo, id=%{public}" PRIu64 ", shape=(%{public}u, %{public}u), mode=%{public}u, "
        "rotation=%{public}u, dpi=%{public}f, (mirrorWidth,mirrotHeight)=(%{public}u, %{public}u), "
        "needDrawPointer=%{public}d",
        screenId_, width_, height_, mode_, rotation_.load(), dpi_, mirrorWidth_, mirrorHeight_, needDrawPointer);
}

void ScreenPointer::OnDisplayInfo(const OLD::DisplayInfo &di)
{
    if (screenId_ != di.rsId) {
        return;
    }

    isCurrentOffScreenRendering_ = di.isCurrentOffScreenRendering;
    dpi_ = float(di.dpi) / BASELINE_DENSITY;
    if (!IsMirror()) {
        rotation_ = static_cast<rotation_t>(di.direction);
    }
    SetDirectionAndDisplayDirection(di.direction, di.displayDirection);
    MMI_HILOGD("Update with DisplayInfo, id=%{public}" PRIu64 ", shape=(%{public}u, %{public}u), mode=%{public}u, "
        "rotation=%{public}u, dpi=%{public}f", screenId_, width_, height_, mode_, rotation_.load(), dpi_);
    if (isCurrentOffScreenRendering_) {
        if (di.width == 0 || di.screenRealWidth == 0 || di.screenRealHeight == 0) {
            MMI_HILOGE("invalid data, width=%{public}u, screenRealWidth=%{public}u, screenRealHeight=%{public}u",
                di.width, di.screenRealWidth, di.screenRealHeight);
            return;
        }

        int32_t realWidth = di.screenRealWidth;
        int32_t realHeight = di.screenRealHeight;
        if ((rotation_ == rotation_t::ROTATION_90 || rotation_ == rotation_t::ROTATION_270)) {
            std::swap(realWidth, realHeight);
        }

        offRenderScale_ = float(realWidth) / di.width;
        MMI_HILOGD("Update with DisplayInfo, screenRealDPI=%{public}d, offRenderScale_=(%{public}f ",
            di.screenRealDPI, offRenderScale_);
    }
}

/*
 * 镜像模式下，计算主屏或镜像屏的屏幕黑边，此接口仅主屏或镜像屏调用
 * 注意：width_、height_不跟随屏幕旋转，sourceScreenWidth、sourceScreenHeight不跟随屏幕旋转
 *
 * 单显示器模式: sourceScreenWidth、sourceScreenHeight为主屏宽高
 * 扩展模式：sourceScreenWidth、sourceScreenHeight为主屏宽高
 * 仅第二屏模式：sourceScreenWidth、sourceScreenHeight为主屏宽高
 * 镜像模式：镜像模式下，主屏可以旋转，镜像屏不旋转
 * （1）满屏显示：主屏
 *      主屏ScreenPointer：sourceScreenWidth、sourceScreenHeight为主屏宽高
 *      镜像屏ScreenPointer: sourceScreenWidth、sourceScreenHeight为主屏宽高
 * （2）满屏显示：镜像屏
 *      主屏ScreenPointer：sourceScreenWidth、sourceScreenHeight为镜像屏宽高
 *      镜像屏ScreenPointer: sourceScreenWidth、sourceScreenHeight为镜像屏宽高
 */
bool ScreenPointer::UpdatePadding(uint32_t sourceScreenWidth, uint32_t sourceScreenHeight)
{
#ifndef OHOS_BUILD_EXTERNAL_SCREEN
    if (!IsMirror()) {
        MMI_HILOGI("UpdatePadding, reset padding, screenId=%{public}" PRIu64 ", scale=%{public}f, "
            "paddingTop_=%{public}u, paddingLeft_=%{public}u", screenId_, scale_, paddingTop_, paddingLeft_);
        scale_ = 1.0;
        paddingTop_ = 0;
        paddingLeft_ = 0;
        return false;
    }
#endif // OHOS_BUILD_EXTERNAL_SCREEN
    if (sourceScreenWidth == 0 || sourceScreenHeight == 0) {
        MMI_HILOGE("Invalid parameters, sourceScreenWidth=%{public}u, sourceScreenHeight=%{public}u",
            sourceScreenWidth, sourceScreenHeight);
        return false;
    }
    // 镜像屏不会旋转，若主屏旋转90度或270度，则(sourceScreenHeight, sourceScreenWidth)为主屏实际分辨率
    if (IsMirror() && (sourceScreenRotation_ == rotation_t::ROTATION_90 ||
        sourceScreenRotation_ == rotation_t::ROTATION_270)) {
        std::swap(sourceScreenWidth, sourceScreenHeight);
    }
    // caculate padding for main screen or mirror screen
    scale_ = fmin(float(width_) / sourceScreenWidth, float(height_) / sourceScreenHeight);
    paddingTop_ = (height_ - sourceScreenHeight * scale_) / NUM_TWO;
    paddingLeft_ = (width_ - sourceScreenWidth * scale_) / NUM_TWO;
#ifdef OHOS_BUILD_EXTERNAL_SCREEN
    if (IsMain()) {
        scale_ = 1.0;
    }
#endif // OHOS_BUILD_EXTERNAL_SCREEN
    MMI_HILOGI("UpdatePadding, screenId=%{public}" PRIu64 ", scale=%{public}f, paddingTop_=%{public}u,"
               " paddingLeft_=%{public}u, curScreen=(%{public}u, %{public}u), sourceScreen=(%{public}u, %{public}u)"
               "sourceScreenRotation=%{public}d",
               screenId_, scale_, paddingTop_, paddingLeft_, width_, height_, sourceScreenWidth,
               sourceScreenHeight, sourceScreenRotation_);
    return true;
}

void ScreenPointer::Rotate(rotation_t rotation, int32_t& x, int32_t& y, int32_t width, int32_t height)
{
    int32_t tmpX = x;
    int32_t tmpY = y;
    if (rotation == rotation_t(DIRECTION90)) {
        x = height - tmpY;
        y = tmpX;
    } else if (rotation == rotation_t(DIRECTION180)) {
        x = width - tmpX;
        y = height - tmpY;
    } else if (rotation == rotation_t(DIRECTION270)) {
        x = tmpY;
        y = width - tmpX;
    }
}

void ScreenPointer::CalculateHwcPositionForMain(int32_t& x, int32_t& y)
{
    if (GetIsCurrentOffScreenRendering()) {
        x = x * offRenderScale_;
        y = y * offRenderScale_;
    }
    Direction direction = static_cast<Direction>((((DIRECTION0 - displayDirection_) *
        ANGLE_90 + ANGLE_360) % ANGLE_360) / ANGLE_90);
#ifdef OHOS_BUILD_EXTERNAL_SCREEN
    int32_t width = static_cast<int32_t>(width_) - paddingLeft_ * NUM_TWO;
    int32_t height = static_cast<int32_t>(height_) - paddingTop_ * NUM_TWO;
#else
    int32_t width = static_cast<int32_t>(width_);
    int32_t height = static_cast<int32_t>(height_);
#endif // OHOS_BUILD_EXTERNAL_SCREEN
    if (displayDirection_ == DIRECTION90 || displayDirection_ == DIRECTION270) {
        std::swap(width, height);
    }
    Rotate(rotation_t(direction), x, y, width, height);
#ifdef OHOS_BUILD_EXTERNAL_SCREEN
    x += paddingLeft_;
    y += paddingTop_;
#endif // OHOS_BUILD_EXTERNAL_SCREEN
}

void ScreenPointer::CalculateHwcPositionForExtend(int32_t& x, int32_t& y)
{
    if (GetIsCurrentOffScreenRendering()) {
        x = x * offRenderScale_;
        y = y * offRenderScale_;
    }
    Direction direction = static_cast<Direction>((((DIRECTION0 - displayDirection_) *
        ANGLE_90 + ANGLE_360) % ANGLE_360) / ANGLE_90);
    int32_t width = static_cast<int32_t>(width_);
    int32_t height = static_cast<int32_t>(height_);
    if (displayDirection_ == DIRECTION90 || displayDirection_ == DIRECTION270) {
        std::swap(width, height);
    }
    Rotate(rotation_t(direction), x, y, width, height);
}

void ScreenPointer::CalculateHwcPositionForMirror(int32_t& x, int32_t& y)
{
    x = x * scale_;
    y = y * scale_;
    Direction direction = static_cast<Direction>((((static_cast<Direction>(sourceScreenRotation_) - displayDirection_) *
        ANGLE_90 + ANGLE_360) % ANGLE_360) / ANGLE_90);
    int32_t width = static_cast<int32_t>(width_) - paddingLeft_ * NUM_TWO;
    int32_t height = static_cast<int32_t>(height_) - paddingTop_ * NUM_TWO;
    if (direction == DIRECTION90 || direction == DIRECTION270) {
        std::swap(width, height);
    }
    Rotate(rotation_t(direction), x, y, width, height);
    x += paddingLeft_;
    y += paddingTop_;
}

int32_t GetCursorBufferSize()
{
    // const. properties are read-only and fixed at boot, so the tier cannot change while the
    // process lives. A function-local static keeps the parameter lookup off the per-frame draw
    // path without needing an ownership story for a global; thread-safe init is guaranteed.
    static const int32_t side = OHOS::system::GetBoolParameter(
        "const.multimodalinput.support_blur_cursor", false) ? BLUR_CURSOR_SIZE : DEFAULT_CURSOR_SIZE;
    return side;
}

uint32_t ScreenPointer::ResolveBufferSide()
{
    // Mirror screens and off-screen-rendering extended screens scale the cursor up
    // (GetScale() > 1) to match the target resolution, so the upscaled glyph no longer fits a
    // 256 buffer: a 200x200 icon at scale 2.0 becomes 400x400. Those two therefore require 512.
    // Every other screen follows the boot-fixed blur switch.
    if (IsMirror() || (IsExtend() && GetIsCurrentOffScreenRendering())) {
        return static_cast<uint32_t>(BLUR_CURSOR_SIZE);
    }
    return static_cast<uint32_t>(GetCursorBufferSize());
}

uint32_t ScreenPointer::GetBufferSize() const
{
    return bufferSize_;
}

bool ScreenPointer::IsPoolEmpty() const
{
    std::shared_lock<std::shared_mutex> lock(bufferMutex_);
    return commonBuffers_.empty();
}

bool ScreenPointer::Move(int32_t x, int32_t y)
{
    if (isVirtualExtend_) {
        MMI_HILOGD("Virtual extend screen move, screenId=%{public}" PRIu64, screenId_);
        return true;
    }
    CHKPF(hwcMgr_);
    switch (mode_) {
        case mode_t::SCREEN_ALONE:
        case mode_t::SCREEN_UNIQUE:{
            MMI_HILOGE("Unsupport screen mode, mode=%{public}u", mode_);
            return false;
        }
        case mode_t::SCREEN_MAIN: {
            CalculateHwcPositionForMain(x, y);
            break;
        }
        case mode_t::SCREEN_EXTEND: {
            CalculateHwcPositionForExtend(x, y);
            break;
        }
        case mode_t::SCREEN_MIRROR: {
            CalculateHwcPositionForMirror(x, y);
            break;
        }
        default: {
            MMI_HILOGE("Invalid screen mode, mode=%{public}u", mode_);
            return false;
        }
    }

    buffer_ptr_t buffer;
    int32_t hotspotX = 0;
    int32_t hotspotY = 0;
    {
        // One frame: the buffer and its matching hotspot are read under the same lock that
        // UpdateHotspotInBuffer() writes them, so the pair can never be crossed.
        std::shared_lock<std::shared_mutex> lock(bufferMutex_);
        buffer = currentBuffer_;
        hotspotX = hotspotHardX_;
        hotspotY = hotspotHardY_;
    }
    CHKPF(buffer);
    auto bh = buffer->GetBufferHandle();
    CHKPF(bh);
    if (IsPositionOutScreen(x, y)) {
        MMI_HILOGE("Position out of screen");
    }
    int32_t px = x - hotspotX;
    int32_t py = y - hotspotY;
    uint32_t bid = bufferId_.load(std::memory_order_relaxed);
    BytraceAdapter::StartHardPointerMove(buffer->GetWidth(), buffer->GetHeight(), bid, screenId_);
    auto ret = hwcMgr_->SetPosition(screenId_, px, py, bh);
    BytraceAdapter::StopHardPointerMove();
    if (ret != RET_OK) {
        MMI_HILOGE("SetPosition failed, screenId=%{public}" PRIu64 ", pos=(%{private}d, %{private}d)",
            screenId_, px, py);
        return false;
    }
    return true;
}

bool ScreenPointer::MoveSoft(int32_t x, int32_t y)
{
    // Snapshot surfaceNode_ under bufferMutex_ so we can't race with InitSurface(), which
    // nulls and replaces it under the same lock.  After the snapshot, the local shared_ptr
    // keeps the node alive even if InitSurface() runs concurrently and replaces the member.
    std::shared_ptr<Rosen::RSSurfaceNode> node;
    {
        std::shared_lock<std::shared_mutex> lock(bufferMutex_);
        node = surfaceNode_;
    }
    if (node == nullptr) {
        MMI_HILOGW("SurfaceNode is null in MoveSoft");
        return false;
    }
    if (rsUIContext_ == nullptr) {
        MMI_HILOGW("RSUIContext is null in MoveSoft");
        return false;
    }
    if (rsUIContext_->GetRSRenderInterface() == nullptr) {
        MMI_HILOGW("RSRenderInterface is null in MoveSoft");
        return false;
    }

    if (IsMirror()) {
        CalculateHwcPositionForMirror(x, y);
    }
    int32_t hotspotX = 0;
    int32_t hotspotY = 0;
    {
        std::shared_lock<std::shared_mutex> lock(bufferMutex_);
        hotspotX = hotspotSoftX_;
        hotspotY = hotspotSoftY_;
    }
    int32_t px = x - hotspotX;
    int32_t py = y - hotspotY;

    if (!IsMirror()) {
        int64_t nodeId = node->GetId();
        rsUIContext_->GetRSRenderInterface()->SetHwcNodeBounds(nodeId, px, py, static_cast<int32_t>(bufferSize_),
            static_cast<int32_t>(bufferSize_));
    } else {
        node->SetBounds(px, py, static_cast<int32_t>(bufferSize_), static_cast<int32_t>(bufferSize_));
    }
    MMI_HILOGD("Pointer Window SetBounds ScreenId:%{public}" PRIu64
        ", surfaceNodeId=%{public}" PRId64 ", pos=(%{private}d, %{private}d)",
        screenId_, node->GetId(), px, py);
    return true;
}

bool ScreenPointer::SetInvisible()
{
    if (isVirtualExtend_) {
        MMI_HILOGD("Virtual extend screen set invisible, screenId=%{public}" PRIu64, screenId_);
        return true;
    }
    CHKPF(hwcMgr_);

    auto buffer = GetTransparentBuffer();
    CHKPF(buffer);
    int32_t width = buffer->GetWidth();
    int32_t height = buffer->GetHeight();
    if ((width < 0) || (height < 0)) {
        MMI_HILOGI("The input data is incorrect, width:%{public}d, height:%{public}d.", width, height);
        return false;
    }
    auto sret = buffer->FlushCache();
    if (sret != RET_OK) {
        MMI_HILOGE("FlushCache ret: %{public}d", sret);
        return sret;
    }
    // Publish the transparent buffer as the current frame so MoveRetry sees a consistent state.
    {
        std::unique_lock<std::shared_mutex> lock(bufferMutex_);
        currentBuffer_ = buffer;
    }

    auto bh = buffer->GetBufferHandle();
    CHKPF(bh);
    auto ret = hwcMgr_->SetPosition(screenId_, 0, 0, bh);
    if (ret != RET_OK) {
        MMI_HILOGE("SetLocation failed, screenId=%{public}" PRIu64 ", loc=(%{private}d, %{private}d)", screenId_, 0, 0);
        return false;
    }
    MMI_HILOGI("SetInvisible success, screenId=%{public}" PRIu64, screenId_);
    return true;
}

bool ScreenPointer::IsPositionOutScreen(int32_t x, int32_t y)
{
    int32_t width = static_cast<int32_t>(width_);
    int32_t height = static_cast<int32_t>(height_);
    if (x < 0 || y < 0 || x > width || y > height) {
        MMI_HILOGE("Position out of screen, x=%{private}d, y=%{private}d, width=%{public}d, height=%{public}d",
            x, y, width, height);
        return true;
    }
    return false;
}

bool ScreenPointer::InitRSUIContext(uint64_t screenId)
{
    bool isInited = (rsUIDirector_ != nullptr && rsUIContext_ != nullptr);
    bool isScreenChanged = (screenId != screenId_);
    if (isInited) {
        if (!isScreenChanged) {
            return true;
        } else {
            RsFlushImplicitTransaction();
        }
    }

    sptr<IRemoteObject> renderToken = Rosen::RSInterfaces::GetInstance().GetConnectToRenderToken(screenId);
    if (renderToken == nullptr) {
        MMI_HILOGE("Get connect to render token fail, screenId=%{public}" PRIu64, screenId);
        return false;
    }

    rsUIDirector_ = Rosen::RSUIDirector::Create(renderToken);
    if (rsUIDirector_ == nullptr) {
        MMI_HILOGE("Create RSUIDirector fail, screenId=%{public}" PRIu64, screenId);
        return false;
    }

    rsUIContext_ = rsUIDirector_->GetRSUIContext();
    if (rsUIContext_ == nullptr) {
        rsUIDirector_ = nullptr;
        MMI_HILOGE("Create RSUIContext fail, screenId=%{public}" PRIu64, screenId);
        return false;
    }
    MMI_HILOGI("Init RSUIContext success, screenId=%{public}" PRIu64, screenId);
    return true;
}

void ScreenPointer::RsFlushImplicitTransaction()
{
    if (rsUIDirector_ != nullptr) {
        rsUIDirector_->SendMessages();
    }
}

void ScreenPointer::DestroyPointerWindow()
{
    // Remove surfaceNode from render tree
    if (surfaceNode_ != nullptr) {
        surfaceNode_->DetachToDisplay(screenId_);
        surfaceNode_ = nullptr;
    }

    RsFlushImplicitTransaction();
    rsUIDirector_ = nullptr;
    rsUIContext_ = nullptr;
    MMI_HILOGI("Destroy pointer window, screenId=%{public}" PRIu64, screenId_);
}

void ScreenPointer::SetDirectionAndDisplayDirection(Direction direction, Direction displayDirection)
{
    if ((direction != DIRECTION0 && direction != DIRECTION90 &&
        direction != DIRECTION180 && direction != DIRECTION270) ||
        (displayDirection != DIRECTION0 && displayDirection != DIRECTION90 &&
        displayDirection != DIRECTION180 && displayDirection != DIRECTION270)) {
        MMI_HILOGE("SetDirection, rsId=%{public}" PRIu64 ", invalid direction=%{public}d, displayDirection=%{public}d",
            screenId_, direction, displayDirection);
        return;
    }

    if (direction_ != direction || displayDirection_ != displayDirection) {
        MMI_HILOGI("SetDirection, rsId=%{public}" PRIu64 ", direction:%{public}d=>%{public}d, "
            "displayDirection:%{public}d=>%{public}d",
            screenId_, direction_.load(), direction, displayDirection_.load(), displayDirection);
        direction_ = direction;
        displayDirection_ = displayDirection;
    }
}

Direction ScreenPointer::GetDirection()
{
    return direction_;
}

Direction ScreenPointer::GetDisplayDirection()
{
    return displayDirection_;
}
} // namespace OHOS::MMI