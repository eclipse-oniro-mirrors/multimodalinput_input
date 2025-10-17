/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef POINTER_DRAWING_MANAGER_H
#define POINTER_DRAWING_MANAGER_H

#include "transaction/rs_transaction.h"
#include "transaction/rs_interfaces.h"

#include "common_event_manager.h"
#include "device_observer.h"
#include "hardware_cursor_pointer_manager.h"
#include "dm_common.h"
#include "screen_manager_lite.h"
#include "i_pointer_drawing_manager.h"
#include "mouse_event_normalize.h"
#include "screen_pointer.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr int32_t DEFAULT_FRAME_RATE { 30 };
constexpr int32_t INVALID_DISPLAY_ID { -1 };
constexpr float HARDWARE_CANVAS_SIZE { 512.0f };
} // namespace

struct isMagicCursor {
    std::string name;
    bool isShow { false };
};

struct PixelMapReleaseContext {
    explicit PixelMapReleaseContext(std::shared_ptr<Media::PixelMap> pixelMap) : pixelMap_(pixelMap) {}

    ~PixelMapReleaseContext()
    {
        pixelMap_ = nullptr;
    }

private:
    std::shared_ptr<Media::PixelMap> pixelMap_ { nullptr };
};

class ScreenModeChangeListener final : public OHOS::Rosen::ScreenManagerLite::IScreenModeChangeListener {
public:
    using callback_t = std::function<void(const std::vector<sptr<OHOS::Rosen::ScreenInfo>> &)>;
    explicit ScreenModeChangeListener(callback_t func): callback_(func) {}
    virtual ~ScreenModeChangeListener() = default;

    void NotifyScreenModeChange(const std::vector<sptr<OHOS::Rosen::ScreenInfo>> &screens) override
    {
        return callback_(screens);
    }

private:
    callback_t callback_;
};

class DelegateInterface;
class PointerDrawingManager final : public IPointerDrawingManager, public IDeviceObserver {
public:
    PointerDrawingManager();
    DISALLOW_COPY_AND_MOVE(PointerDrawingManager);
    ~PointerDrawingManager();
    void DrawPointer(uint64_t rsId, int32_t physicalX, int32_t physicalY,
        const PointerStyle pointerStyle, Direction direction) override;
    void UpdateDisplayInfo(const OLD::DisplayInfo& displayInfo) override;
    void OnDisplayInfo(const OLD::DisplayGroupInfo& displayGroupInfo) override;
    void OnWindowInfo(const WinInfo &info) override;
    void UpdatePointerDevice(bool hasPointerDevice, bool isPointerVisible, bool isHotPlug) override;
    bool Init() override;
    int32_t SetPointerColor(int32_t color) override;
    int32_t GetPointerColor() override;
    void DeletePointerVisible(int32_t pid) override;
    int32_t SetPointerVisible(int32_t pid, bool visible, int32_t priority, bool isHap) override;
    bool GetPointerVisible(int32_t pid) override;
    int32_t SetPointerStyle(int32_t pid, int32_t windowId, PointerStyle pointerStyle,
        bool isUiExtension = false) override;
    int32_t ClearWindowPointerStyle(int32_t pid, int32_t windowId) override;
    int32_t GetPointerStyle(int32_t pid, int32_t windowId, PointerStyle &pointerStyle,
        bool isUiExtension = false) override;
    int32_t SetPointerSize(int32_t size) override;
    int32_t GetPointerSize() override;
    void GetPointerImageSize(int32_t &width, int32_t &height) override;
    int32_t GetCursorSurfaceId(uint64_t &surfaceId) override;
    void DrawPointerStyle(const PointerStyle& pointerStyle) override;
    bool IsPointerVisible() override;
    void SetPointerLocation(int32_t x, int32_t y, uint64_t rsId) override;
    void AdjustMouseFocus(Direction direction, ICON_TYPE iconType, int32_t &physicalX, int32_t &physicalY);
    void SetMouseDisplayState(bool state) override;
    bool GetMouseDisplayState() const override;
    int32_t SetCustomCursor(CursorPixelMap curPixelMap,
        int32_t pid, int32_t windowId, int32_t focusX, int32_t focusY) override;
    int32_t SetCustomCursor(int32_t pid, int32_t windowId, CustomCursor cursor, CursorOptions options) override;
    int32_t SetMouseIcon(int32_t pid, int32_t windowId, CursorPixelMap curPixelMap) override;
    int32_t SetMouseHotSpot(int32_t pid, int32_t windowId, int32_t hotSpotX, int32_t hotSpotY) override;
    PointerStyle GetLastMouseStyle() override;
    const std::map<MOUSE_ICON, IconStyle>& GetMouseIconPath() override;
    IconStyle GetIconStyle(const MOUSE_ICON mouseStyle) override;
    bool HasMagicCursor();
    int32_t DrawCursor(const MOUSE_ICON mouseStyle);
    int32_t SwitchPointerStyle() override;
    void DrawMovePointer(uint64_t rsId, int32_t physicalX, int32_t physicalY) override;
    std::vector<std::vector<std::string>> GetDisplayInfo(OLD::DisplayInfo &di);
    void Dump(int32_t fd, const std::vector<std::string> &args) override;
    void AttachToDisplay();
    int32_t EnableHardwareCursorStats(int32_t pid, bool enable) override;
    int32_t GetHardwareCursorStats(int32_t pid, uint32_t &frameCount, uint32_t &vsyncCount) override;
    void SubscribeScreenModeChange() override;
    void RegisterDisplayStatusReceiver() override;
    OLD::DisplayInfo GetCurrentDisplayInfo() override;
#ifdef OHOS_BUILD_ENABLE_MAGICCURSOR
    int32_t GetPointerSnapshot(void *pixelMapPtr) override;
#endif // OHOS_BUILD_ENABLE_MAGICCURSOR
    void InitPointerCallback() override;
    void InitPointerObserver() override;
    void OnSessionLost(int32_t pid) override;
    int32_t SkipPointerLayer(bool isSkip) override;
    void SetDelegateProxy(std::shared_ptr<DelegateInterface> proxy) override
    {
        delegateProxy_ = proxy;
    }
    std::shared_ptr<DelegateInterface> GetDelegateProxy() override
    {
        return delegateProxy_;
    }
    void DestroyPointerWindow() override;
    void DrawScreenCenterPointer(const PointerStyle& pointerStyle) override;
    void OnScreenModeChange(const std::vector<sptr<OHOS::Rosen::ScreenInfo>> &screens);
    int32_t UpdateMouseLayer(const PointerStyle& pointerStyle, int32_t physicalX, int32_t physicalY) override;
#ifndef OHOS_BUILD_ENABLE_WATCH
    void NotifyPointerEventToRS(int32_t pointAction, int32_t pointCnt) override;
#endif // OHOS_BUILD_ENABLE_WATCH
    void RsRemoteDiedCallback();
    int32_t GetCurrentCursorInfo(bool& visible, PointerStyle& pointerStyle) override;
    int32_t GetUserDefinedCursorPixelMap(void *pixelMapPtr) override;
    void RecordCursorIdAndImageAddress();
    void RecordCursorVisibleStatus(bool status);

private:
    struct PixelMapInfo {
        std::shared_ptr<OHOS::Media::PixelMap> pixelMap { nullptr };
        int32_t imageWidth { 0 };
        int32_t imageHeight { 0 };
        int32_t pointerColor { 0 };
    };
    IconStyle GetIconType(MOUSE_ICON mouseIcon);
    void GetPreferenceKey(std::string &name);
    void DrawLoadingPointerStyle(const MOUSE_ICON mouseStyle);
    void DrawRunningPointerAnimate(const MOUSE_ICON mouseStyle);
    void CreatePointerWindow(uint64_t rsId, int32_t physicalX, int32_t physicalY, Direction direction);
    int32_t CreatePointerWindowForScreenPointer(uint64_t rsId, int32_t physicalX, int32_t physicalY);
    int32_t CreatePointerWindowForNoScreenPointer(int32_t physicalX, int32_t physicalY);
    sptr<OHOS::Surface> GetLayer();
    sptr<OHOS::SurfaceBuffer> GetSurfaceBuffer(sptr<OHOS::Surface> layer);
    sptr<OHOS::SurfaceBuffer> RetryGetSurfaceBuffer(sptr<OHOS::Surface> layer);
    void DoDraw(uint8_t *addr, uint32_t width, uint32_t height, const MOUSE_ICON mouseStyle = MOUSE_ICON::DEFAULT);
    void DrawPixelmap(OHOS::Rosen::Drawing::Canvas &canvas, const MOUSE_ICON mouseStyle);
    void DrawManager();
    void FixCursorPosition(int32_t &physicalX, int32_t &physicalY);
    std::shared_ptr<OHOS::Media::PixelMap> LoadCursorSvgWithColor(MOUSE_ICON type, int32_t color);
    std::shared_ptr<OHOS::Media::PixelMap> DecodeImageToPixelMap(MOUSE_ICON type);
    void UpdatePointerVisible();
    int32_t UpdateDefaultPointerStyle(int32_t pid, int32_t windowId, PointerStyle style, bool isUiExtension = false);
    void CheckMouseIconPath();
    void InitDefaultMouseIconPath() override;
    void InitStyle();
    int32_t InitLayer(const MOUSE_ICON mouseStyle);
    int32_t SetPointerStylePreference(PointerStyle pointerStyle);
    void UpdateMouseStyle();
    int32_t UpdateCursorProperty(CursorPixelMap curPixelMap, const int32_t &focusX, const int32_t &focusY);
    int32_t UpdateCursorProperty(CustomCursor cursor);
    void RotateDegree(Direction direction);
    int32_t DrawMovePointer(uint64_t rsId, int32_t physicalX, int32_t physicalY,
        PointerStyle pointerStyle, Direction direction);
    void AdjustMouseFocusByDirection0(ICON_TYPE iconType, int32_t &physicalX, int32_t &physicalY);
    void AdjustMouseFocusByDirection90(ICON_TYPE iconType, int32_t &physicalX, int32_t &physicalY);
    void AdjustMouseFocusByDirection180(ICON_TYPE iconType, int32_t &physicalX, int32_t &physicalY);
    void AdjustMouseFocusByDirection270(ICON_TYPE iconType, int32_t &physicalX, int32_t &physicalY);
    void CreateMagicCursorChangeObserver();
    int32_t CreatePointerSwitchObserver(isMagicCursor& item);
    void UpdateStyleOptions();
    int32_t GetIndependentPixels();
    bool IsWindowRotation(const OLD::DisplayInfo *displayInfo);
    Direction GetDisplayDirection(const OLD::DisplayInfo *displayInfo);
    bool CheckPointerStyleParam(int32_t windowId, PointerStyle pointerStyle);
    std::map<MOUSE_ICON, IconStyle>& GetMouseIcons();
    void UpdateIconPath(const MOUSE_ICON mouseStyle, std::string iconPath);
    std::shared_ptr<Rosen::Drawing::ColorSpace> ConvertToColorSpace(Media::ColorSpace colorSpace);
    Rosen::Drawing::ColorType PixelFormatToColorType(Media::PixelFormat pixelFormat);
    Rosen::Drawing::AlphaType AlphaTypeToAlphaType(Media::AlphaType alphaType);
    std::shared_ptr<Rosen::Drawing::Image> ExtractDrawingImage(std::shared_ptr<Media::PixelMap> pixelMap);
    void DrawImage(OHOS::Rosen::Drawing::Canvas &canvas, MOUSE_ICON mouseStyle);
    int32_t ReloadPixelMaps(std::map<MOUSE_ICON, PixelMapInfo>& mousePixelMap, int32_t pointerColor);
    void InitPixelMaps();
#ifdef OHOS_BUILD_ENABLE_MAGICCURSOR
    void SetPixelMap(std::shared_ptr<OHOS::Media::PixelMap> pixelMap);
#endif // OHOS_BUILD_ENABLE_MAGICCURSOR
    void ForceClearPointerVisiableStatus() override;
    int32_t UpdateSurfaceNodeBounds(int32_t physicalX, int32_t physicalY);
    void CreateCanvasNode();
    bool SetCursorLocation(int32_t physicalX, int32_t physicalY, ICON_TYPE iconType);
    void SetHardwareCursorPosition(int32_t physicalX, int32_t physicalY,
        PointerStyle pointerStyle);
    std::shared_ptr<OHOS::Media::PixelMap> GetUserIconCopy(bool setSurfaceNode = true);
    ICON_TYPE MouseIcon2IconType(MOUSE_ICON m);
    void SetSurfaceNodeBounds();
    int32_t DrawNewDpiPointer() override;
    bool SetDynamicHardWareCursorLocation(int32_t physicalX, int32_t physicalY, MOUSE_ICON mouseStyle);
    void RenderThreadLoop();
    void SoftCursorRenderThreadLoop();
    void MoveRetryThreadLoop();
    int32_t RequestNextVSync();
    void OnVsync(uint64_t timestamp);
    void PostTask(std::function<void()> task);
    void PostSoftCursorTask(std::function<void()> task);
    void PostMoveRetryTask(std::function<void()> task);
    int32_t FlushBuffer();
    int32_t GetSurfaceInformation();
    void UpdateBindDisplayId(uint64_t rsId);
    void PostTaskRSLocation(int32_t physicalX, int32_t physicalY, std::shared_ptr<Rosen::RSSurfaceNode> surfaceNode);
    int32_t InitVsync(MOUSE_ICON mouseStyle);
    void DumpScreenInfo(std::ostringstream& oss);
    int32_t DrawSoftCursor(std::shared_ptr<Rosen::RSSurfaceNode> sn, const RenderConfig &cfg);
    int32_t DrawHardCursor(std::shared_ptr<ScreenPointer> sp, const RenderConfig &cfg);
    std::vector<std::shared_ptr<ScreenPointer>> GetMirrorScreenPointers();
    std::shared_ptr<ScreenPointer> GetScreenPointer(uint64_t screenId);
    void CreateRenderConfig(RenderConfig& cfg, std::shared_ptr<ScreenPointer> sp, MOUSE_ICON mouseStyle, bool isHard);
    Direction CalculateRenderDirection(const bool isHard, const bool isWindowRotation);
    void SoftwareCursorRender(MOUSE_ICON mouseStyle);
    void HardwareCursorRender(MOUSE_ICON mouseStyle);
    void SoftwareCursorMove(int32_t x, int32_t y, ICON_TYPE align);
    void SoftwareCursorMoveAsync(int32_t x, int32_t y, ICON_TYPE align);
    void MoveRetryAsync(int32_t x, int32_t y, ICON_TYPE align);
    void ResetMoveRetryTimer();
    int32_t HardwareCursorMove(int32_t x, int32_t y, ICON_TYPE align);
    void HideHardwareCursors();
    int32_t GetMainScreenDisplayInfo(const OLD::DisplayGroupInfo &displayGroupInfo,
        OLD::DisplayInfo &mainScreenDisplayInfo) const;
    int32_t DrawDynamicHardwareCursor(std::shared_ptr<ScreenPointer> sp, const RenderConfig &cfg);
    int32_t DrawDynamicSoftCursor(std::shared_ptr<Rosen::RSSurfaceNode> sn, const RenderConfig &cfg);
    void HardwareCursorDynamicRender(MOUSE_ICON mouseStyle);
    void SoftwareCursorDynamicRender(MOUSE_ICON mouseStyle);
    void UpdateMirrorScreens(std::shared_ptr<ScreenPointer> sp, OLD::DisplayInfo displayInfo);
    void AttachAllSurfaceNode() override;
    void DetachAllSurfaceNode() override;
    int32_t CheckHwcReady() override;
    std::shared_ptr<Rosen::RSSurfaceNode> GetSurfaceNode();
    void SetSurfaceNode(std::shared_ptr<Rosen::RSSurfaceNode> ptr);
    void AdjustMouseFocusToSoftRenderOrigin(Direction direction, const MOUSE_ICON pointerStyle, int32_t &physicalX,
        int32_t &physicalY);
    bool GetHardCursorEnabled() override;
private:
    struct PidInfo {
        int32_t pid { 0 };
        bool visible { false };
    };
    bool hasDisplay_ { false };
    OLD::DisplayInfo displayInfo_ {};
    bool hasPointerDevice_ { false };
    int32_t lastPhysicalX_ { -1 };
    int32_t lastPhysicalY_ { -1 };
    PointerStyle lastMouseStyle_ {};
    PointerStyle currentMouseStyle_ {};
    PointerStyle lastDrawPointerStyle_ {};
    int32_t pid_ { 0 };
    int32_t windowId_ { 0 };
    int32_t imageWidth_ { 0 };
    int32_t imageHeight_ { 0 };
    int32_t canvasWidth_ = 64;
    int32_t canvasHeight_ = 64;
    int32_t cursorWidth_ { 0 };
    int32_t cursorHeight_ { 0 };
    std::map<MOUSE_ICON, IconStyle> mouseIcons_;
    std::list<PidInfo> pidInfos_;
    std::list<PidInfo> hapPidInfos_;
    bool mouseDisplayState_ { false };
    bool mouseIconUpdate_ { false };
    bool offRenderScaleUpdate_ { false };
    std::shared_ptr<OHOS::Media::PixelMap> userIcon_ { nullptr };
    uint64_t screenId_ { 0 };
    std::mutex surfaceNodeMutex_;
    std::shared_ptr<Rosen::RSSurfaceNode> surfaceNode_;
    std::shared_ptr<Rosen::RSCanvasNode> canvasNode_;
    int32_t userIconHotSpotX_ { 0 };
    int32_t userIconHotSpotY_ { 0 };
    int32_t tempPointerColor_ { -1 };
    int32_t originSetColor_ { -1 };
    Direction lastDirection_ { DIRECTION0 };
    Direction currentDirection_ { DIRECTION0 };
    isMagicCursor hasMagicCursor_;
    bool hasInitObserver_ { false };
    int32_t frameCount_ { DEFAULT_FRAME_RATE };
    int32_t currentFrame_ { 0 };
    uint64_t displayId_ { 0 };
    std::shared_ptr<AppExecFwk::EventRunner> runner_ { nullptr };
    std::shared_ptr<AppExecFwk::EventHandler> handler_ { nullptr };
    std::shared_ptr<Rosen::VSyncReceiver> receiver_ { nullptr };
    std::atomic<bool> isRenderRuning_{ false };
    std::unique_ptr<std::thread> renderThread_ { nullptr };
    bool isInit_ { false };
    std::mutex mtx_;
    std::recursive_mutex rec_mtx_;
    std::shared_ptr<HardwareCursorPointerManager> hardwareCursorPointerManager_ { nullptr };
    sptr<ScreenModeChangeListener> screenModeChangeListener_ { nullptr };
    std::unordered_map<uint64_t, std::shared_ptr<ScreenPointer>> screenPointers_;
    PointerRenderer pointerRenderer_;
    bool userIconFollowSystem_ { false };
    std::shared_ptr<AppExecFwk::EventRunner> softCursorRunner_ { nullptr };
    std::shared_ptr<AppExecFwk::EventHandler> softCursorHander_ { nullptr };
    std::unique_ptr<std::thread> softCursorRenderThread_ { nullptr };
    std::shared_ptr<AppExecFwk::EventRunner> moveRetryRunner_ { nullptr };
    std::shared_ptr<AppExecFwk::EventHandler> moveRetryHander_ { nullptr };
    std::unique_ptr<std::thread> moveRetryThread_ { nullptr };
    int32_t moveRetryTimerId_ { -1 };
    int32_t moveRetryCount_ { 0 };
    float hardwareCanvasSize_ { HARDWARE_CANVAS_SIZE };
#ifdef OHOS_BUILD_ENABLE_MAGICCURSOR
    std::shared_ptr<OHOS::Media::PixelMap> pixelMap_ { nullptr };
#endif // OHOS_BUILD_ENABLE_MAGICCURSOR
    std::shared_ptr<DelegateInterface> delegateProxy_ { nullptr };
    uint64_t lastDisplayId_ { 0 };
    std::map<MOUSE_ICON, PixelMapInfo> mousePixelMap_;
    std::mutex mousePixelMapMutex_;
    int32_t initLoadingAndLoadingRightPixelTimerId_ { -1 };
    int releaseFence_ { -1 };
    bool followSystem_ { false };
    int32_t focusX_ { 0 };
    int32_t focusY_ { 0 };
    std::atomic<bool> initEventhandlerFlag_ { false };
    std::atomic<bool> initDisplayStatusReceiverFlag_ { false };
    std::shared_ptr<PointerDrawingManager> self_ { nullptr };
    Rosen::OnRemoteDiedCallback OnRemoteDiedCallback_ { nullptr };
    std::shared_ptr<EventFwk::CommonEventSubscriber> commonEventSubscriber_ { nullptr };
};
} // namespace MMI
} // namespace OHOS
#endif // POINTER_DRAWING_MANAGER_H
