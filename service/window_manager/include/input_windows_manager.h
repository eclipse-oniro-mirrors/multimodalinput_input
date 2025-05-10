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

#ifndef INPUT_WINDOWS_MANAGER_H
#define INPUT_WINDOWS_MANAGER_H

#include <shared_mutex>
#include "mmi_transform.h"
#include "window_manager_lite.h"

#include "i_input_windows_manager.h"
#include "input_display_bind_helper.h"
#ifndef OHOS_BUILD_ENABLE_WATCH
#include "knuckle_drawing_manager.h"
#include "knuckle_dynamic_drawing_manager.h"
#endif // OHOS_BUILD_ENABLE_WATCH

namespace OHOS {
namespace MMI {
constexpr uint32_t SCREEN_CONTROL_WINDOW_TYPE = 2138;
struct WindowInfoEX {
    WindowInfo window;
    bool flag { false };
};

struct SwitchFocusKey {
    int32_t keyCode { -1 };
    int32_t pressedKey { -1 };
};

enum AcrossDirection : int32_t {
    ACROSS_ERROR = 0,
    UPWARDS = 1,
    DOWNWARDS = 2,
    LEFTWARDS = 3,
    RIGHTWARDS = 4,
};

class InputWindowsManager final : public IInputWindowsManager {
public:
    InputWindowsManager();
    ~InputWindowsManager();
    DISALLOW_COPY_AND_MOVE(InputWindowsManager);

    void Init(UDSServer& udsServer);
    void SetMouseFlag(bool state);
    bool GetMouseFlag();
    bool JudgeCaramaInFore();
#ifdef OHOS_BUILD_ENABLE_POINTER
    void JudgMouseIsDownOrUp(bool dragState);
#endif // OHOS_BUILD_ENABLE_POINTER
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    int32_t GetClientFd(std::shared_ptr<PointerEvent> pointerEvent);
    int32_t GetClientFd(std::shared_ptr<PointerEvent> pointerEvent, int32_t windowId);
    bool AdjustFingerFlag(std::shared_ptr<PointerEvent> pointerEvent);
    void PrintEnterEventInfo(std::shared_ptr<PointerEvent> pointerEvent);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    bool HandleWindowInputType(const WindowInfo &window, std::shared_ptr<PointerEvent> pointerEvent);
    void UpdateCaptureMode(const DisplayGroupInfo &displayGroupInfo);
    bool IsFocusedSession(int32_t session) const;
    void UpdateDisplayInfo(DisplayGroupInfo &displayGroupInfo);
    void UpdateDisplayInfoExtIfNeed(DisplayGroupInfo &displayGroupInfo, bool needUpdateDisplayExt);
    void UpdateWindowInfo(const WindowGroupInfo &windowGroupInfo);
    int32_t ClearWindowPointerStyle(int32_t pid, int32_t windowId);
    void Dump(int32_t fd, const std::vector<std::string> &args);
    void DumpDisplayInfo(int32_t fd);
    int32_t GetWindowPid(int32_t windowId, const std::vector<WindowInfo> &windowsInfo) const;
    int32_t GetWindowPid(int32_t windowId) const;
    int32_t SetMouseCaptureMode(int32_t windowId, bool isCaptureMode);
    bool GetMouseIsCaptureMode() const;
    void DeviceStatusChanged(int32_t deviceId, const std::string &sysUid, const std::string devStatus);
    int32_t GetDisplayBindInfo(DisplayBindInfos &infos);
    int32_t SetDisplayBind(int32_t deviceId, int32_t displayId, std::string &msg);
    int32_t AppendExtraData(const ExtraData& extraData);
    bool IsWindowVisible(int32_t pid);
    void ClearExtraData();
    ExtraData GetExtraData() const;
    const std::vector<WindowInfo> GetWindowGroupInfoByDisplayId(int32_t displayId) const;
    std::pair<double, double> TransformWindowXY(const WindowInfo &window, double logicX, double logicY) const;
    std::pair<double, double> TransformDisplayXY(const DisplayInfo &info, double logicX, double logicY) const;
    int32_t GetCurrentUserId();
    bool GetCancelEventFlag(std::shared_ptr<PointerEvent> pointerEvent);
    void SetFoldState ();
    bool CheckAppFocused(int32_t pid);
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    std::vector<std::pair<int32_t, TargetInfo>> GetPidAndUpdateTarget(std::shared_ptr<KeyEvent> keyEvent);
    void ReissueEvent(std::shared_ptr<KeyEvent> keyEvent, int32_t focusWindowId);
    std::vector<std::pair<int32_t, TargetInfo>> UpdateTarget(std::shared_ptr<KeyEvent> keyEvent);
    bool IsKeyPressed(int32_t pressedKey, std::vector<KeyEvent::KeyItem> &keyItems);
    bool IsOnTheWhitelist(std::shared_ptr<KeyEvent> keyEvent);
    void HandleKeyEventWindowId(std::shared_ptr<KeyEvent> keyEvent);
    int32_t focusWindowId_ { -1 };
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    int32_t CheckWindowIdPermissionByPid(int32_t windowId, int32_t pid);

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    MouseLocation GetMouseInfo();
    CursorPosition GetCursorPos();
    CursorPosition ResetCursorPos();
    void SetGlobalDefaultPointerStyle();
    void UpdateAndAdjustMouseLocation(int32_t& displayId, double& x, double& y, bool isRealData = true);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
#ifdef OHOS_BUILD_ENABLE_POINTER
    const DisplayGroupInfo& GetDisplayGroupInfo();
    std::vector<DisplayInfo> GetDisplayInfoVector() const;
    const std::vector<WindowInfo> GetWindowInfoVector() const;
    int32_t GetFocusWindowId() const;
    int32_t GetLogicalPositionX(int32_t id);
    int32_t GetLogicalPositionY(int32_t id);
    Direction GetLogicalPositionDirection(int32_t id);
    Direction GetPositionDisplayDirection(int32_t id);
    int32_t SetHoverScrollState(bool state);
    bool GetHoverScrollState() const;
    bool SelectPointerChangeArea(int32_t windowId, int32_t logicalX, int32_t logicalY);
#endif // OHOS_BUILD_ENABLE_POINTER
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    int32_t SetPointerStyle(int32_t pid, int32_t windowId, PointerStyle pointerStyle, bool isUiExtension = false);
    int32_t GetPointerStyle(int32_t pid, int32_t windowId, PointerStyle &pointerStyle,
        bool isUiExtension = false) const;
    void SetUiExtensionInfo(bool isUiExtension, int32_t uiExtensionPid, int32_t uiExtensionWindoId);
    void DispatchPointer(int32_t pointerAction, int32_t windowId = -1);
    void SendPointerEvent(int32_t pointerAction);
    bool IsMouseSimulate();
    bool HasMouseHideFlag();
    void UpdatePointerDrawingManagerWindowInfo();
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
#ifdef OHOS_BUILD_ENABLE_POINTER
    PointerStyle GetLastPointerStyle() const;
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING
    bool IsNeedRefreshLayer(int32_t windowId);
#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING
#endif // OHOS_BUILD_ENABLE_POINTER

#ifdef OHOS_BUILD_ENABLE_TOUCH
    void AdjustDisplayCoordinate(const DisplayInfo& displayInfo, double& physicalX, double& physicalY) const;
    bool TouchPointToDisplayPoint(int32_t deviceId, struct libinput_event_touch* touch,
        EventTouch& touchInfo, int32_t& targetDisplayId, bool isNeedClear = false);
#endif // OHOS_BUILD_ENABLE_TOUCH
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    void ReverseRotateScreen(const DisplayInfo& info, const double x, const double y, Coordinate2D& cursorPos) const;
    void ReverseRotateDisplayScreen(const DisplayInfo& info, const double x, const double y,
        Coordinate2D& cursorPos) const;
    void ScreenRotateAdjustDisplayXY(const DisplayInfo& info, PhysicalCoordinate& coord) const;
    void RotateScreen(const DisplayInfo& info, PhysicalCoordinate& coord) const;
    void RotateDisplayScreen(const DisplayInfo& info, PhysicalCoordinate& coord);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
#ifdef OHOS_BUILD_ENABLE_TOUCH
    bool TransformTipPoint(struct libinput_event_tablet_tool* tip, PhysicalCoordinate& coord, int32_t& displayId) const;
    bool CalculateTipPoint(struct libinput_event_tablet_tool* tip,
        int32_t& targetDisplayId, PhysicalCoordinate& coord) const;
    const std::shared_ptr<DisplayInfo> GetDefaultDisplayInfo() const;
    void ReverseXY(int32_t &x, int32_t &y);
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    void FoldScreenRotation(std::shared_ptr<PointerEvent> pointerEvent);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    void SendCancelEventWhenLock();
#endif // OHOS_BUILD_ENABLE_TOUCH

#ifdef OHOS_BUILD_ENABLE_ANCO
    void UpdateWindowInfoExt(const WindowGroupInfo &windowGroupInfo, const DisplayGroupInfo &displayGroupInfo);
    void UpdateOneHandDataExt(const DisplayInfo &displayInfo);
    void UpdateShellWindow(const WindowInfo &window);
    void UpdateDisplayInfoExt(const DisplayGroupInfo &displayGroupInfo);
    bool IsInAncoWindow(const WindowInfo &window, int32_t x, int32_t y) const;
    bool IsAncoWindow(const WindowInfo &window) const;
    bool IsAncoWindowFocus(const WindowInfo &window) const;
    void SimulatePointerExt(std::shared_ptr<PointerEvent> pointerEvent);
    void DumpAncoWindows(std::string& out) const;
    void CleanShellWindowIds();
    bool IsKnuckleOnAncoWindow(std::shared_ptr<PointerEvent> pointerEvent);
    void SendOneHandData(onst DisplayInfo &displayInfo, std::shared_ptr<PointerEvent> &pointerEvent);
#endif // OHOS_BUILD_ENABLE_ANCO

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    bool UpdateDisplayId(int32_t& displayId);
    void DrawTouchGraphic(std::shared_ptr<PointerEvent> pointerEvent);
    int32_t UpdateTargetPointer(std::shared_ptr<PointerEvent> pointerEvent);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    const std::shared_ptr<DisplayInfo> GetPhysicalDisplay(int32_t id) const;
    const std::shared_ptr<DisplayInfo> GetPhysicalDisplay(int32_t id, const DisplayGroupInfo &displayGroupInfo) const;

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    void UpdatePointerChangeAreas();
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    std::optional<WindowInfo> GetWindowAndDisplayInfo(int32_t windowId, int32_t displayId);
    void GetTargetWindowIds(int32_t pointerItemId, int32_t sourceType, std::vector<int32_t> &windowIds);
    void AddTargetWindowIds(int32_t pointerItemId, int32_t sourceType, int32_t windowId);
    void ClearTargetWindowId(int32_t pointerId);
    bool IsTransparentWin(std::unique_ptr<Media::PixelMap> &pixelMap, int32_t logicalX, int32_t logicalY);
    int32_t SetCurrentUser(int32_t userId);
    DisplayMode GetDisplayMode() const;
    void SetWindowStateNotifyPid(int32_t pid);
    int32_t GetWindowStateNotifyPid();
    int32_t GetPidByWindowId(int32_t pid);
#ifdef OHOS_BUILD_ENABLE_ANCO
    int32_t AncoAddChannel(sptr<IAncoChannel> channel);
    int32_t AncoRemoveChannel(sptr<IAncoChannel> channel);
    int32_t SyncKnuckleStatus(bool isKnuckleEnable);
#endif // OHOS_BUILD_ENABLE_ANCO

    int32_t SetPixelMapData(int32_t infoId, void *pixelMap);
    void CleanInvalidPiexMap();
    void HandleWindowPositionChange();
#ifdef OHOS_BUILD_ENABLE_HARDWARE_CURSOR
    bool IsSupported();
#endif // OHOS_BUILD_ENABLE_HARDWARE_CURSOR
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    int32_t ShiftAppPointerEvent(const ShiftWindowParam &param, bool autoGenDown);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
#if defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
    void AttachTouchGestureMgr(std::shared_ptr<TouchGestureManager> touchGestureMgr);
    void CancelAllTouches(std::shared_ptr<PointerEvent> event, bool isDisplayChanged = false);
#endif // defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
#ifdef OHOS_BUILD_ENABLE_TOUCH
    std::shared_ptr<PointerEvent> GetLastPointerEventForGesture() { return lastPointerEventforGesture_; };
#endif // OHOS_BUILD_ENABLE_TOUCH
    void SwitchTouchTracking(bool touchTracking);

private:
    bool NeedTouchTracking(PointerEvent &event) const;
    void ProcessTouchTracking(std::shared_ptr<PointerEvent> event, const WindowInfo &targetWindow);
    bool IgnoreTouchEvent(std::shared_ptr<PointerEvent> pointerEvent);
    void ReissueCancelTouchEvent(std::shared_ptr<PointerEvent> pointerEvent);
    int32_t GetDisplayId(std::shared_ptr<InputEvent> inputEvent) const;
    void PrintHighZorder(const std::vector<WindowInfo> &windowsInfo, int32_t pointerAction,
        int32_t targetWindowId, int32_t logicalX, int32_t logicalY);
    void PrintZorderInfo(const WindowInfo &windowInfo, std::string &window);
    void PrintWindowInfo(const std::vector<WindowInfo> &windowsInfo);
    void PrintDisplayGroupInfo(const DisplayGroupInfo displayGroupInfo);
    void PrintDisplayInfo(const DisplayInfo displayInfo);
    int32_t ConvertToolType(int32_t toolType);
    void PrintWindowGroupInfo(const WindowGroupInfo &windowGroupInfo);
    void PrintWindowNavbar();
    void CheckFocusWindowChange(const DisplayGroupInfo &displayGroupInfo);
    void CheckZorderWindowChange(const std::vector<WindowInfo> &oldWindowsInfo,
        const std::vector<WindowInfo> &newWindowsInfo);
    void UpdateDisplayIdAndName();
    void UpdateCustomStyle(int32_t windowId, PointerStyle pointerStyle);
    void UpdatePointerAction(std::shared_ptr<PointerEvent> pointerEvent);
    bool IsNeedDrawPointer(PointerEvent::PointerItem &pointerItem) const;
    void UpdateDisplayInfoByIncrementalInfo(const WindowInfo &window, DisplayGroupInfo &displayGroupInfo);
    void UpdateWindowsInfoPerDisplay(const DisplayGroupInfo &displayGroupInfo);
    std::pair<int32_t, int32_t> TransformSampleWindowXY(int32_t logicX, int32_t logicY) const;
    bool IsValidZorderWindow(const WindowInfo &window, const std::shared_ptr<PointerEvent>& pointerEvent);
    bool SkipPrivacyProtectionWindow(const std::shared_ptr<PointerEvent>& pointerEvent, const bool &isSkip);
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    void UpdateTopBottomArea(const Rect &windowArea, std::vector<int32_t> &pointerChangeAreas,
        std::vector<Rect> &windowHotAreas);
    void UpdateLeftRightArea(const Rect &windowArea, std::vector<int32_t> &pointerChangeAreas,
        std::vector<Rect> &windowHotAreas);
    void UpdateInnerAngleArea(const Rect &windowArea, std::vector<int32_t> &pointerChangeAreas,
        std::vector<Rect> &windowHotAreas);
    void CoordinateCorrection(int32_t width, int32_t height, int32_t &integerX, int32_t &integerY);
    Direction GetDisplayDirection(const DisplayInfo *displayInfo);
    void GetWidthAndHeight(const DisplayInfo* displayInfo, int32_t &width, int32_t &height, bool isRealData = true);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    void SetPrivacyModeFlag(SecureFlag privacyMode, std::shared_ptr<InputEvent> event);
    void PrintChangedWindowByEvent(int32_t eventType, const WindowInfo &newWindowInfo);
    void PrintChangedWindowBySync(const DisplayGroupInfo &newDisplayInfo);
    bool IsMouseDrawing(int32_t currentAction);
    bool ParseConfig();
    bool ParseJson(const std::string &configFile);
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    void SendUIExtentionPointerEvent(int32_t logicalX, int32_t logicalY,
        const WindowInfo& windowInfo, std::shared_ptr<PointerEvent> pointerEvent);
    void DispatchUIExtentionPointerEvent(int32_t logicalX, int32_t logicalY,
        std::shared_ptr<PointerEvent> pointerEvent);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t UpdateMouseTarget(std::shared_ptr<PointerEvent> pointerEvent);
    void UpdatePointerEvent(int32_t logicalX, int32_t logicalY,
        const std::shared_ptr<PointerEvent>& pointerEvent, const WindowInfo& touchWindow);
    void NotifyPointerToWindow();
    void OnSessionLost(SessionPtr session);
    void InitPointerStyle();
#endif // OHOS_BUILD_ENABLE_POINTER
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    int32_t UpdatePoinerStyle(int32_t pid, int32_t windowId, PointerStyle pointerStyle);
    int32_t UpdateSceneBoardPointerStyle(int32_t pid, int32_t windowId, PointerStyle pointerStyle,
        bool isUiExtension = false);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t UpdateTouchPadTarget(std::shared_ptr<PointerEvent> pointerEvent);
#endif // OHOS_BUILD_ENABLE_POINTER
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    std::optional<WindowInfo> SelectWindowInfo(int32_t logicalX, int32_t logicalY,
        const std::shared_ptr<PointerEvent>& pointerEvent);
    void CheckUIExtentionWindowPointerHotArea(int32_t logicalX, int32_t logicalY,
        const std::vector<WindowInfo>& windowInfos, int32_t& windowId);
    std::optional<WindowInfo> GetWindowInfo(int32_t logicalX, int32_t logicalY);
    bool IsInsideDisplay(const DisplayInfo& displayInfo, double physicalX, double physicalY);
    bool CalculateLayout(const DisplayInfo& displayInfo, const Vector2D<double> &physical, Vector2D<double>& layout);
    void FindPhysicalDisplay(const DisplayInfo& displayInfo, double& physicalX,
        double& physicalY, int32_t& displayId);
    bool AcrossDisplay(const DisplayInfo &displayInfoDes, const DisplayInfo &displayInfoOri, Vector2D<double> &logical,
        Vector2D<double> &layout, const AcrossDirection &acrossDirection);
    AcrossDirection CalculateAcrossDirection(const DisplayInfo &displayInfo, const Vector2D<double> &layout);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    void InitMouseDownInfo();
    bool SelectPointerChangeArea(const WindowInfo &windowInfo, PointerStyle &pointerStyle,
        int32_t logicalX, int32_t logicalY);
    void UpdatePointerChangeAreas(const DisplayGroupInfo &displayGroupInfo);
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING
    void AdjustDisplayRotation();
    void SetPointerEvent(int32_t pointerAction, std::shared_ptr<PointerEvent> pointerEvent);
    void DispatchPointerCancel(int32_t displayId);
    void AdjustDragPosition();
#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
void PointerDrawingManagerOnDisplayInfo(const DisplayGroupInfo &displayGroupInfo, bool isDisplayRemoved = false);
void DrawPointer(bool isDisplayRemoved);
bool NeedUpdatePointDrawFlag(const std::vector<WindowInfo> &windows);
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING

    void UpdateFixedXY(const DisplayInfo& displayInfo, std::shared_ptr<PointerEvent> &pointerEvent);
#ifdef OHOS_BUILD_ENABLE_ONE_HAND_MODE
void UpdatePointerItemInOneHandMode(const DisplayInfo &displayInfo, std::shared_ptr<PointerEvent> &pointerEvent);
void UpdateDisplayXYInOneHandMode(double& physicalX, double& physicalY, const DisplayInfo &displayInfo,
    float oneHandScale);
void HandleOneHandMode(const DisplayInfo &displayInfo, std::shared_ptr<PointerEvent> &pointerEvent,
    PointerEvent::PointerItem &pointerItem);
#endif // OHOS_BUILD_ENABLE_ONE_HAND_MODE

#ifdef OHOS_BUILD_ENABLE_TOUCH
    bool SkipAnnotationWindow(uint32_t flag, int32_t toolType);
    bool SkipNavigationWindow(WindowInputType windowType, int32_t toolType);
    void HandleGestureInjection(bool gestureInject);
    int32_t UpdateTouchScreenTarget(std::shared_ptr<PointerEvent> pointerEvent);
    void UpdateTargetTouchWinIds(const WindowInfo &item, PointerEvent::PointerItem &pointerItem,
        std::shared_ptr<PointerEvent> pointerEvent, int32_t pointerId, int32_t displayId);
    void ClearMismatchTypeWinIds(int32_t pointerId, int32_t displayId);
#endif // OHOS_BUILD_ENABLE_TOUCH

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    bool IsValidNavigationWindow(const WindowInfo& touchWindow, double physicalX, double physicalY);
    bool IsNavigationWindowInjectEvent(std::shared_ptr<PointerEvent> pointerEvent);
    void UpdateTransformDisplayXY(std::shared_ptr<PointerEvent> pointerEvent,
        const std::vector<WindowInfo>& windowsInfo, const DisplayInfo& displayInfo);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
#ifdef OHOS_BUILD_ENABLE_TOUCH
    void PullEnterLeaveEvent(int32_t logicalX, int32_t logicalY,
        const std::shared_ptr<PointerEvent> pointerEvent, const WindowInfo* touchWindow);
    void DispatchTouch(int32_t pointerAction);
    const std::shared_ptr<DisplayInfo> FindPhysicalDisplayInfo(const std::string& uniq) const;
    bool GetPhysicalDisplayCoord(struct libinput_event_touch* touch,
        const DisplayInfo& info, EventTouch& touchInfo, bool isNeedClear = false);
    void TriggerTouchUpOnInvalidAreaEntry(int32_t pointerId);
    void SetAntiMisTake(bool state);
    void SetAntiMisTakeStatus(bool state);
    void CheckUIExtentionWindowDefaultHotArea(std::pair<int32_t, int32_t> logicalXY, bool isHotArea,
        const std::shared_ptr<PointerEvent> pointerEvent, const std::vector<WindowInfo>& windowInfos,
        const WindowInfo** touchWindow);
    void GetUIExtentionWindowInfo(std::vector<WindowInfo> &uiExtentionWindowInfo, int32_t windowId,
        WindowInfo **touchWindow, bool &isUiExtentionWindow);
#endif // OHOS_BUILD_ENABLE_TOUCH

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    bool IsInHotArea(int32_t x, int32_t y, const std::vector<Rect> &rects, const WindowInfo &window) const;
    bool InWhichHotArea(int32_t x, int32_t y, const std::vector<Rect> &rects, PointerStyle &pointerStyle) const;
    bool InWhichHotArea(int32_t x, int32_t y, const std::vector<Rect> &rects) const;
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    template <class T>
    void CreateAntiMisTakeObserver(T& item);
    template <class T>
    void CreatePrivacyProtectionObserver(T& item);

#ifdef OHOS_BUILD_ENABLE_JOYSTICK
    int32_t UpdateJoystickTarget(std::shared_ptr<PointerEvent> pointerEvent);
#endif // OHOS_BUILD_ENABLE_JOYSTICK

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_CROWN)
    int32_t UpdateCrownTarget(std::shared_ptr<PointerEvent> pointerEvent);
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_CROWN
    void UpdateDisplayMode();
    void HandleValidDisplayChange(const DisplayGroupInfo &displayGroupInfo);
    void ResetPointerPositionIfOutValidDisplay(const DisplayGroupInfo &displayGroupInfo);
    void CancelMouseEvent();
    bool IsPositionOutValidDisplay(
        Coordinate2D &position, const DisplayInfo &currentDisplay, bool isPhysicalPos = false);
    void CancelTouchScreenEventIfValidDisplayChange(const DisplayGroupInfo &displayGroupInfo);
    bool IsValidDisplayChange(const DisplayInfo &displayInfo);
#ifdef OHOS_BUILD_ENABLE_HARDWARE_CURSOR
    void UpdateKeyEventDisplayId(std::shared_ptr<KeyEvent> keyEvent, int32_t focusWindowId);
    bool OnDisplayRemovedOrCombiantionChanged(const DisplayGroupInfo &displayGroupInfo);
    void ChangeWindowArea(int32_t x, int32_t y, WindowInfo &windowInfo);
    void ResetPointerPosition(const DisplayGroupInfo &displayGroupInfo);
    int32_t GetMainScreenDisplayInfo(const std::vector<DisplayInfo> &displaysInfo,
        DisplayInfo &mainScreenDisplayInfo) const;
    bool IsPointerOnCenter(const CursorPosition &currentPos, const DisplayInfo &currentDisplay);
    void SendBackCenterPointerEevent(const CursorPosition &cursorPos);
#endif // OHOS_BUILD_ENABLE_HARDWARE_CURSOR
    WINDOW_UPDATE_ACTION UpdateWindowInfo(DisplayGroupInfo &displayGroupInfo);
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    std::optional<WindowInfo> GetWindowInfoById(int32_t windowId) const;
    int32_t ShiftAppMousePointerEvent(const ShiftWindowInfo &shiftWindowInfo, bool autoGenDown);
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
#if defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
    bool CancelTouch(int32_t touch);
#endif // defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
    bool IsPointerActiveRectValid(const DisplayInfo &currentDisplay);
#endif // OHOS_BUILD_ENABLE_VKEYBOARD

private:
    UDSServer* udsServer_ { nullptr };
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    bool isUiExtension_ { false };
    int32_t uiExtensionPid_ { -1 };
    int32_t uiExtensionWindowId_ { -1 };
    std::pair<int32_t, int32_t> firstBtnDownWindowInfo_ {-1, -1};
    std::optional<WindowInfo> axisBeginWindowInfo_ { std::nullopt };
    int32_t lastLogicX_ { -1 };
    int32_t lastLogicY_ { -1 };
    WindowInfo lastWindowInfo_;
    std::shared_ptr<PointerEvent> lastPointerEvent_ { nullptr };
    std::map<int32_t, std::map<int32_t, PointerStyle>> pointerStyle_;
    std::map<int32_t, std::map<int32_t, PointerStyle>> uiExtensionPointerStyle_;
    WindowInfo mouseDownInfo_;
    PointerStyle globalStyle_;
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
#ifdef OHOS_BUILD_ENABLE_TOUCH
    int32_t lastTouchLogicX_ { -1 };
    int32_t lastTouchLogicY_ { -1 };
    WindowInfo lastTouchWindowInfo_;
    std::shared_ptr<PointerEvent> lastTouchEvent_ { nullptr };
    std::shared_ptr<PointerEvent> lastTouchEventOnBackGesture_ { nullptr };
#endif // OHOS_BUILD_ENABLE_TOUCH
#if defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
    std::weak_ptr<TouchGestureManager> touchGestureMgr_;
#endif // defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
    DisplayGroupInfo displayGroupInfoTmp_;
    std::mutex tmpInfoMutex_;
    DisplayGroupInfo displayGroupInfo_;
    mutable std::shared_mutex displayGroupInfoMtx;
    std::map<int32_t, WindowGroupInfo> windowsPerDisplay_;
    PointerStyle lastPointerStyle_ {.id = -1};
    PointerStyle dragPointerStyle_ {.id = -1};
    MouseLocation mouseLocation_ = { -1, -1 };
    CursorPosition cursorPos_ {};
    std::map<int32_t, WindowInfoEX> touchItemDownInfos_;
    std::map<int32_t, std::vector<Rect>> windowsHotAreas_;
    InputDisplayBindHelper bindInfo_;
    struct CaptureModeInfo {
        int32_t windowId { -1 };
        bool isCaptureMode { false };
    } captureModeInfo_;
    ExtraData extraData_;
    int32_t mouseDownEventId_ { -1 };
    bool haveSetObserver_ { false };
    bool dragFlag_ { false };
    bool isDragBorder_ { false };
    bool pointerDrawFlag_ { false };
    DisplayMode displayMode_ { DisplayMode::UNKNOWN };
    struct AntiMisTake {
        std::string switchName;
        bool isOpen { false };
    } antiMistake_;
    bool isOpenAntiMisTakeObserver_ { false };
    struct PrivacyProtection {
        std::string switchName;
        bool isOpen { false };
    } privacyProtection_;
    bool isOpenPrivacyProtectionserver_ { false };
#ifndef OHOS_BUILD_ENABLE_WATCH
    std::shared_ptr<KnuckleDrawingManager> knuckleDrawMgr_ { nullptr };
#endif // OHOS_BUILD_ENABLE_WATCH
    bool mouseFlag_ {false};
    std::map<int32_t, std::vector<int32_t>> targetTouchWinIds_;
    std::map<int32_t, std::vector<int32_t>> targetMouseWinIds_;
    int32_t pointerActionFlag_ { -1 };
    int32_t currentUserId_ { -1 };
#ifndef OHOS_BUILD_ENABLE_WATCH
    std::shared_ptr<KnuckleDynamicDrawingManager> knuckleDynamicDrawingManager_ { nullptr };
#endif // OHOS_BUILD_ENABLE_WATCH
    std::shared_ptr<PointerEvent> lastPointerEventforWindowChange_ { nullptr };
    bool cancelTouchStatus_ { false };
    std::pair<int32_t, Direction> lastDirection_ { -1, static_cast<Direction>(-1) };
    std::map<int32_t, WindowInfo> lastMatchedWindow_;
    std::vector<SwitchFocusKey> vecWhiteList_;
    bool isParseConfig_ { false };
    int32_t windowStateNotifyPid_ { -1 };
    std::map<int32_t, std::unique_ptr<Media::PixelMap>> transparentWins_;
#ifdef OHOS_BUILD_ENABLE_TOUCH
    std::shared_ptr<PointerEvent> lastPointerEventforGesture_ { nullptr };
#endif // OHOS_BUILD_ENABLE_TOUCH
    static std::unordered_map<int32_t, int32_t> convertToolTypeMap_;
    bool IsFoldable_ { false };
    bool touchTracking_ { false };
    int32_t timerId_ { -1 };
    int32_t lastDpi_ { 0 };
    std::shared_ptr<PointerEvent> GetlastPointerEvent();
    std::mutex mtx_;
    std::atomic_bool isHPR_ { false };
    std::mutex oneHandMtx_;
    bool inOneHandMode_ = false;
};
} // namespace MMI
} // namespace OHOS
#endif // INPUT_WINDOWS_MANAGER_H
