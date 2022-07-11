/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "input_windows_manager.h"

#include <cstdlib>
#include <cstdio>

#include "i_pointer_drawing_manager.h"
#include "util.h"
#include "util_ex.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, MMI_LOG_DOMAIN, "InputWindowsManager"};
} // namespace

InputWindowsManager::InputWindowsManager() {}

InputWindowsManager::~InputWindowsManager() {}

bool InputWindowsManager::Init(UDSServer& udsServer)
{
    udsServer_ = &udsServer;
    return true;
}

int32_t InputWindowsManager::UpdateTarget(std::shared_ptr<InputEvent> inputEvent)
{
    CHKPR(inputEvent, ERROR_NULL_POINTER);
    CALL_LOG_ENTER;
    int32_t pid = GetPidAndUpdateTarget(inputEvent);
    if (pid <= 0) {
        MMI_HILOGE("Invalid pid");
        return RET_ERR;
    }
    int32_t fd = udsServer_->GetClientFd(pid);
    if (fd < 0) {
        MMI_HILOGE("Invalid fd");
        return RET_ERR;
    }
    return fd;
}

int32_t InputWindowsManager::GetDisplayId(std::shared_ptr<InputEvent> inputEvent) const
{
    int32_t displayId = inputEvent->GetTargetDisplayId();
    if (displayId < 0) {
        MMI_HILOGD("target display is -1");
        if (logicalDisplays_.empty()) {
            return displayId;
        }
        displayId = logicalDisplays_[0].id;
        inputEvent->SetTargetDisplayId(displayId);
    }
    return displayId;
}

int32_t InputWindowsManager::GetWindowPid(int32_t windowId)
{
    for (const auto& logicalDisplayItem : logicalDisplays_) {
        for (const auto& windowInfo : logicalDisplayItem.windowsInfo) {
            if (windowInfo.id == windowId) {
                return windowInfo.pid;
            }
        }
    }
    return RET_ERR;
}

int32_t InputWindowsManager::GetPidAndUpdateTarget(std::shared_ptr<InputEvent> inputEvent) const
{
    CALL_LOG_ENTER;
    CHKPR(inputEvent, ERROR_NULL_POINTER);
    const int32_t targetDisplayId = GetDisplayId(inputEvent);
    if (targetDisplayId < 0) {
        MMI_HILOGE("No display is available.");
        return RET_ERR;
    }
    for (const auto &item : logicalDisplays_) {
        if (item.id != targetDisplayId) {
            continue;
        }
        MMI_HILOGD("target display:%{public}d", targetDisplayId);
        auto it = windowInfos_.find(item.focusWindowId);
        if (it == windowInfos_.end()) {
            MMI_HILOGE("can't find window info, focuswindowId:%{public}d", item.focusWindowId);
            return RET_ERR;
        }
        inputEvent->SetTargetWindowId(item.focusWindowId);
        inputEvent->SetAgentWindowId(it->second.agentWindowId);
        MMI_HILOGD("pid:%{public}d", it->second.pid);
        return it->second.pid;
    }

    MMI_HILOGE("can't find logical display,target display:%{public}d", targetDisplayId);
    return RET_ERR;
}

void InputWindowsManager::UpdateDisplayInfo(const std::vector<PhysicalDisplayInfo> &physicalDisplays,
    const std::vector<LogicalDisplayInfo> &logicalDisplays)
{
    CALL_LOG_ENTER;
    physicalDisplays_ = physicalDisplays;
    logicalDisplays_ = logicalDisplays;
    windowInfos_.clear();
    for (const auto &item : logicalDisplays) {
        for (const auto &window : item.windowsInfo) {
            auto iter = windowInfos_.insert(std::pair<int32_t, WindowInfo>(window.id, window));
            if (!iter.second) {
                MMI_HILOGE("Insert value failed, Window:%{public}d", window.id);
            }
        }
    }
    if (!logicalDisplays.empty()) {
        IPointerDrawingManager::GetInstance()->OnDisplayInfo(logicalDisplays[0].id,
            logicalDisplays[0].width, logicalDisplays_[0].height);
    }
    PrintDisplayInfo();
}

void InputWindowsManager::PrintDisplayInfo()
{
    MMI_HILOGD("physicalDisplays,num:%{public}zu", physicalDisplays_.size());
    for (const auto &item : physicalDisplays_) {
        MMI_HILOGD("PhysicalDisplays,id:%{public}d,leftDisplay:%{public}d,upDisplay:%{public}d,"
            "topLeftX:%{public}d,topLeftY:%{public}d,width:%{public}d,height:%{public}d,name:%{public}s,"
            "seatId:%{public}s,seatName:%{public}s,logicWidth:%{public}d,logicHeight:%{public}d,"
            "direction:%{public}d",
            item.id, item.leftDisplayId, item.upDisplayId,
            item.topLeftX, item.topLeftY, item.width,
            item.height, item.name.c_str(), item.seatId.c_str(),
            item.seatName.c_str(), item.logicWidth, item.logicHeight, item.direction);
    }

    MMI_HILOGD("logicalDisplays,num:%{public}zu", logicalDisplays_.size());
    for (const auto &item : logicalDisplays_) {
        MMI_HILOGD("logicalDisplays, id:%{public}d,topLeftX:%{public}d,topLeftY:%{public}d,"
            "width:%{public}d,height:%{public}d,name:%{public}s,"
            "seatId:%{public}s,seatName:%{public}s,focusWindowId:%{public}d,window num:%{public}zu",
            item.id, item.topLeftX, item.topLeftY,
            item.width, item.height, item.name.c_str(),
            item.seatId.c_str(), item.seatName.c_str(), item.focusWindowId,
            item.windowsInfo.size());
    }

    MMI_HILOGD("window info,num:%{public}zu", windowInfos_.size());
    for (const auto &item : windowInfos_) {
        MMI_HILOGD("window:%{public}d,pid:%{public}d,uid:%{public}d,hotZoneTopLeftX:%{public}d,"
            "hotZoneTopLeftY:%{public}d,hotZoneWidth:%{public}d,hotZoneHeight:%{public}d,display:%{public}d,"
            "agentWindowId:%{public}d,winTopLeftX:%{public}d,winTopLeftY:%{public}d,flags:%{public}d",
            item.second.id, item.second.pid, item.second.uid, item.second.hotZoneTopLeftX,
            item.second.hotZoneTopLeftY, item.second.hotZoneWidth, item.second.hotZoneHeight,
            item.second.displayId, item.second.agentWindowId, item.second.winTopLeftX, item.second.winTopLeftY,
            item.second.flags);
    }
}

const PhysicalDisplayInfo* InputWindowsManager::GetPhysicalDisplay(int32_t id) const
{
    for (auto &it : physicalDisplays_) {
        if (it.id == id) {
            return &it;
        }
    }
    MMI_HILOGW("Failed to obtain physical(%{public}d) display", id);
    return nullptr;
}

const PhysicalDisplayInfo* InputWindowsManager::FindPhysicalDisplayInfo(
    const std::string& seatId, const std::string& seatName) const
{
    for (auto &it : physicalDisplays_) {
        if (it.seatId == seatId && it.seatName == seatName) {
            return &it;
        }
    }
    MMI_HILOGE("Failed to search for Physical,seat:%{public}s,name:%{public}s", seatId.c_str(), seatName.c_str());
    return nullptr;
}

void InputWindowsManager::RotateTouchScreen(const PhysicalDisplayInfo* info, LogicalCoordinate& coord) const
{
    CHKPV(info);
    const Direction direction = info->direction;

    if (direction == Direction0) {
        MMI_HILOGD("direction is Direction0");
        return;
    }
    if (direction == Direction90) {
        MMI_HILOGD("direction is Direction90");
        int32_t temp = coord.x;
        coord.x = info->logicHeight - coord.y;
        coord.y = temp;
        MMI_HILOGD("logicalX:%{public}d, logicalY:%{public}d", coord.x, coord.y);
        return;
    }
    if (direction == Direction180) {
        MMI_HILOGD("direction is Direction180");
        coord.x = info->logicWidth - coord.x;
        coord.y = info->logicHeight - coord.y;
        return;
    }
    if (direction == Direction270) {
        MMI_HILOGD("direction is Direction270");
        int32_t temp = coord.y;
        coord.y = info->logicWidth - coord.x;
        coord.x = temp;
    }
}

bool InputWindowsManager::TransformDisplayPoint(struct libinput_event_touch* touch, EventTouch& touchInfo)
{
    CHKPF(touch);
    auto info = FindPhysicalDisplayInfo("seat0", "default0");
    CHKPF(info);

    if ((info->width <= 0) || (info->height <= 0) || (info->logicWidth <= 0) || (info->logicHeight <= 0)) {
        MMI_HILOGE("Get DisplayInfo is error");
        return false;
    }

    PhysicalCoordinate touchPhysCoord {
        .x = libinput_event_touch_get_x_transformed(touch, info->width) + info->topLeftX,
        .y = libinput_event_touch_get_y_transformed(touch, info->height) + info->topLeftY
    };
    LogicalCoordinate touchLogicalCoord;
    if (!Physical2Logical(info, touchPhysCoord, touchLogicalCoord)) {
        MMI_HILOGE("Physical2Logical failed");
        return false;
    }

    PhysicalCoordinate toolPhysCoord {
        .x = libinput_event_touch_get_tool_x_transformed(touch, info->width) + info->topLeftX,
        .y = libinput_event_touch_get_tool_y_transformed(touch, info->height) + info->topLeftY
    };
    LogicalCoordinate toolLogicalCoord;
    if (!Physical2Logical(info, toolPhysCoord, toolLogicalCoord)) {
        MMI_HILOGE("Physical2Logical failed");
        return false;
    }

    auto width = libinput_event_touch_get_tool_width_transformed(touch, info->width);
    auto height = libinput_event_touch_get_tool_height_transformed(touch, info->height);

    touchInfo.point = touchLogicalCoord;
    touchInfo.toolRect = {
        .point = toolLogicalCoord,
        .width = static_cast<int32_t>(width),
        .height = static_cast<int32_t>(height)
    };
    return true;
}

bool InputWindowsManager::TouchMotionPointToDisplayPoint(struct libinput_event_touch* touch,
    int32_t targetDisplayId, EventTouch& touchInfo)
{
    CHKPF(touch);
    if (!TransformDisplayPoint(touch, touchInfo)) {
        MMI_HILOGE("TransformDisplayPoint failed");
        return false;
    }
    for (const auto &display : logicalDisplays_) {
        if (targetDisplayId == display.id) {
            touchInfo.point.x -= display.topLeftX;
            touchInfo.point.y -= display.topLeftY;
            AdjustGlobalCoordinate(display, touchInfo.point);
            touchInfo.toolRect.point.x -= display.topLeftX;
            touchInfo.toolRect.point.y -= display.topLeftY;
            AdjustGlobalCoordinate(display, touchInfo.toolRect.point);
            MMI_HILOGD("Motion targetDisplay:%{public}d, displayX:%{public}d, displayY:%{public}d, "
                       "toolDisplayX:%{public}d, toolDisplayY:%{public}d ",
                       targetDisplayId, touchInfo.point.x, touchInfo.point.y,
                       touchInfo.toolRect.point.x, touchInfo.toolRect.point.y);
            return true;
        }
    }
    return false;
}

bool InputWindowsManager::TouchDownPointToDisplayPoint(struct libinput_event_touch* touch,
    EventTouch& touchInfo, int32_t& logicalDisplayId)
{
    CHKPF(touch);
    if (!TransformDisplayPoint(touch, touchInfo)) {
        MMI_HILOGE("TransformDisplayPoint failed");
        return false;
    }
    for (const auto &display : logicalDisplays_) {
        if ((touchInfo.point.x < display.topLeftX) || (touchInfo.point.x > display.topLeftX + display.width)) {
            continue;
        }
        if ((touchInfo.point.y < display.topLeftY) || (touchInfo.point.y > display.topLeftY + display.height)) {
            continue;
        }
        logicalDisplayId = display.id;
        touchInfo.point.x -= display.topLeftX;
        touchInfo.point.y -= display.topLeftY;
        AdjustGlobalCoordinate(display, touchInfo.point);
        touchInfo.toolRect.point.x -= display.topLeftX;
        touchInfo.toolRect.point.y -= display.topLeftY;
        AdjustGlobalCoordinate(display, touchInfo.toolRect.point);
        MMI_HILOGD("Down tlogicalDisplay:%{public}d, displayX:%{public}d, displayY:%{public}d, "
                   "toolDisplayX:%{public}d, toolDisplayY:%{public}d ",
                   logicalDisplayId, touchInfo.point.x, touchInfo.point.y,
                   touchInfo.toolRect.point.x, touchInfo.toolRect.point.y);
        return true;
    }
    return false;
}

bool InputWindowsManager::Physical2Logical(const PhysicalDisplayInfo* displayInfo,
    const PhysicalCoordinate& phys, LogicalCoordinate& logical) const
{
    CHKPF(displayInfo);
    LogicalCoordinate tCoord {
        .x = (displayInfo->width > 0 ?
            static_cast<int32_t>(phys.x * displayInfo->logicWidth / displayInfo->width) : 0),
        .y = (displayInfo->height > 0 ?
            static_cast<int32_t>(phys.y * displayInfo->logicHeight / displayInfo->height) : 0)
    };
    RotateTouchScreen(displayInfo, tCoord);
    const Direction direction = displayInfo->direction;

    for (const PhysicalDisplayInfo* tDisplay = displayInfo; tDisplay->leftDisplayId >= 0;) {
        tDisplay = GetPhysicalDisplay(tDisplay->leftDisplayId);
        if (tDisplay == nullptr) {
            break;
        }
        if (direction == Direction0 || direction == Direction180) {
            tCoord.x += tDisplay->logicWidth;
        } else if (direction == Direction90 || direction == Direction270) {
            tCoord.x += tDisplay->logicHeight;
        }
    }
    for (const PhysicalDisplayInfo* tDisplay = displayInfo; tDisplay->upDisplayId >= 0;) {
        tDisplay = GetPhysicalDisplay(tDisplay->upDisplayId);
        if (tDisplay == nullptr) {
            break;
        }
        if (direction == Direction0 || direction == Direction180) {
            tCoord.y += tDisplay->logicHeight;
        } else if (direction == Direction90 || direction == Direction270) {
            tCoord.y += tDisplay->logicWidth;
        }
    }
    logical = tCoord;
    MMI_HILOGD("logicalX:%{public}d, logicalY:%{public}d", logical.x, logical.y);
    return true;
}

bool InputWindowsManager::TransformTipPoint(struct libinput_event_tablet_tool* tip, LogicalCoordinate& coord) const
{
    CHKPF(tip);
    auto displayInfo = FindPhysicalDisplayInfo("seat0", "default0");
    CHKPF(displayInfo);
    MMI_HILOGD("PhysicalDisplay.width:%{public}d, PhysicalDisplay.height:%{public}d, "
               "PhysicalDisplay.topLeftX:%{public}d, PhysicalDisplay.topLeftY:%{public}d",
               displayInfo->width, displayInfo->height, displayInfo->topLeftX, displayInfo->topLeftY);
    PhysicalCoordinate phys {
        .x = libinput_event_tablet_tool_get_x_transformed(tip, displayInfo->width),
        .y = libinput_event_tablet_tool_get_y_transformed(tip, displayInfo->height)
    };
    MMI_HILOGD("physicalX:%{public}f, physicalY:%{public}f", phys.x, phys.y);
    return Physical2Logical(displayInfo, phys, coord);
}

bool InputWindowsManager::CalculateTipPoint(struct libinput_event_tablet_tool* tip,
    int32_t& targetDisplayId, LogicalCoordinate& coord) const
{
    CHKPF(tip);
    LogicalCoordinate tCoord;
    if (!TransformTipPoint(tip, tCoord)) {
        return false;
    }
    for (const auto& displayInfo : logicalDisplays_) {
        if (targetDisplayId < 0) {
            if ((tCoord.x < displayInfo.topLeftX) ||
                (tCoord.x > displayInfo.topLeftX + displayInfo.width) ||
                (tCoord.y < displayInfo.topLeftY) ||
                (tCoord.y > displayInfo.topLeftY + displayInfo.height)) {
                continue;
            }
            targetDisplayId = displayInfo.id;
        } else {
            if (targetDisplayId != displayInfo.id) {
                continue;
            }
        }
        tCoord.x -= displayInfo.topLeftX;
        tCoord.y -= displayInfo.topLeftY;
        AdjustGlobalCoordinate(displayInfo, tCoord);
        coord = tCoord;
        MMI_HILOGD("targetDisplay:%{public}d, displayX:%{public}d, displayY:%{public}d ",
            targetDisplayId, coord.x, coord.y);
        return true;
    }
    return false;
}

const std::vector<LogicalDisplayInfo>& InputWindowsManager::GetLogicalDisplayInfo() const
{
    return logicalDisplays_;
}

bool InputWindowsManager::IsInsideWindow(int32_t x, int32_t y, const WindowInfo &info) const
{
    return (x >= info.hotZoneTopLeftX) && (x <= (info.hotZoneTopLeftX + info.hotZoneWidth)) &&
        (y >= info.hotZoneTopLeftY) && (y <= (info.hotZoneTopLeftY + info.hotZoneHeight));
}

void InputWindowsManager::AdjustGlobalCoordinate(
    const LogicalDisplayInfo& displayInfo, LogicalCoordinate& coord) const
{
    if (coord.x <= 0) {
        coord.x = 0;
    }
    if (coord.x >= displayInfo.width && displayInfo.width > 0) {
        coord.x = displayInfo.width - 1;
    }
    if (coord.y <= 0) {
        coord.y = 0;
    }
    if (coord.y >= displayInfo.height && displayInfo.height > 0) {
        coord.y = displayInfo.height - 1;
    }
}

bool InputWindowsManager::UpdataDisplayId(int32_t& displayId)
{
    if (logicalDisplays_.empty()) {
        MMI_HILOGE("logicalDisplays_is empty");
        return false;
    }
    if (displayId < 0) {
        displayId = logicalDisplays_[0].id;
        return true;
    }
    for (const auto &item : logicalDisplays_) {
        if (item.id == displayId) {
            return true;
        }
    }
    return false;
}

LogicalDisplayInfo* InputWindowsManager::GetLogicalDisplayId(int32_t displayId)
{
    for (auto &it : logicalDisplays_) {
        if (it.id == displayId) {
            return &it;
        }
    }
    return nullptr;
}

void InputWindowsManager::SelectWindowInfo(const int32_t& globalX, const int32_t& globalY,
    const std::shared_ptr<PointerEvent>& pointerEvent, LogicalDisplayInfo* const logicalDisplayInfo,
    WindowInfo*& touchWindow)
{
    int32_t action = pointerEvent->GetPointerAction();
    if ((firstBtnDownWindowId_ == -1) ||
        ((action == PointerEvent::POINTER_ACTION_BUTTON_DOWN) && (pointerEvent->GetPressedButtons().size() == 1)) ||
        ((action == PointerEvent::POINTER_ACTION_MOVE) && (pointerEvent->GetPressedButtons().empty()))) {
        int32_t targetWindowId = pointerEvent->GetTargetWindowId();
        for (const auto& item : logicalDisplayInfo->windowsInfo) {
            if ((item.flags & WindowInfo::FLAG_BIT_UNTOUCHABLE) == WindowInfo::FLAG_BIT_UNTOUCHABLE) {
                MMI_HILOGD("Skip the untouchable window to continue searching, "
                           "window:%{public}d, flags:%{public}d", item.id, item.flags);
                continue;
            } else if ((targetWindowId < 0) && (IsInsideWindow(globalX, globalY, item))) {
                firstBtnDownWindowId_ = item.id;
                MMI_HILOGW("find out the dispatch window of this pointerevent when the targetWindowId "
                           "hasn't been setted up yet, window:%{public}d", firstBtnDownWindowId_);
                break;
            } else if ((targetWindowId >= 0) && (targetWindowId == item.id)) {
                firstBtnDownWindowId_ = targetWindowId;
                MMI_HILOGW("find out the dispatch window of this pointerevent when the targetWindowId "
                           "has been setted up already, window:%{public}d", firstBtnDownWindowId_);
                break;
            } else {
                MMI_HILOGW("Continue searching for the dispatch window of this pointerevent");
            }
        }
    }
    for (auto &item : logicalDisplayInfo->windowsInfo) {
        if (item.id == firstBtnDownWindowId_) {
            touchWindow = const_cast<WindowInfo*>(&item);
            break;
        }
    }
}

int32_t InputWindowsManager::UpdateMouseTarget(std::shared_ptr<PointerEvent> pointerEvent)
{
    CALL_LOG_ENTER;
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    auto displayId = pointerEvent->GetTargetDisplayId();
    if (!UpdataDisplayId(displayId)) {
        MMI_HILOGE("This display:%{public}d is not exist", displayId);
        return RET_ERR;
    }
    pointerEvent->SetTargetDisplayId(displayId);

    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
        MMI_HILOGE("Can't find pointer item, pointer:%{public}d", pointerId);
        return RET_ERR;
    }
    LogicalDisplayInfo *logicalDisplayInfo = GetLogicalDisplayId(displayId);
    CHKPR(logicalDisplayInfo, ERROR_NULL_POINTER);
    int32_t globalX = pointerItem.GetGlobalX();
    int32_t globalY = pointerItem.GetGlobalY();
    IPointerDrawingManager::GetInstance()->DrawPointer(displayId, globalX, globalY);
    WindowInfo* touchWindow = nullptr;
    SelectWindowInfo(globalX, globalY, pointerEvent, logicalDisplayInfo, touchWindow);
    if (touchWindow == nullptr) {
        MMI_HILOGE("touchWindow is nullptr, targetWindow:%{public}d", pointerEvent->GetTargetWindowId());
        return RET_ERR;
    }
    pointerEvent->SetTargetWindowId(touchWindow->id);
    pointerEvent->SetAgentWindowId(touchWindow->agentWindowId);
    int32_t localX = globalX - touchWindow->winTopLeftX;
    int32_t localY = globalY - touchWindow->winTopLeftY;
    pointerItem.SetLocalX(localX);
    pointerItem.SetLocalY(localY);
    pointerEvent->UpdatePointerItem(pointerId, pointerItem);
    CHKPR(udsServer_, ERROR_NULL_POINTER);
    auto fd = udsServer_->GetClientFd(touchWindow->pid);

    MMI_HILOGD("fd:%{public}d,pid:%{public}d,id:%{public}d,agentWindowId:%{public}d,"
               "globalX:%{public}d,globalY:%{public}d,localX:%{public}d,localY:%{public}d",
               fd, touchWindow->pid, touchWindow->id, touchWindow->agentWindowId,
               globalX, globalY, localX, localY);
    return fd;
}

int32_t InputWindowsManager::UpdateTouchScreenTarget(std::shared_ptr<PointerEvent> pointerEvent)
{
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    auto displayId = pointerEvent->GetTargetDisplayId();
    if (!UpdataDisplayId(displayId)) {
        MMI_HILOGE("This display is not exist");
        return RET_ERR;
    }
    pointerEvent->SetTargetDisplayId(displayId);

    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
        MMI_HILOGE("Can't find pointer item, pointer:%{public}d", pointerId);
        return RET_ERR;
    }
    MMI_HILOGD("display:%{public}d", displayId);
    LogicalDisplayInfo *logicalDisplayInfo = GetLogicalDisplayId(displayId);
    CHKPR(logicalDisplayInfo, ERROR_NULL_POINTER);
    LogicalCoordinate tCoord;
    tCoord.x = pointerItem.GetGlobalX();
    tCoord.y = pointerItem.GetGlobalY();
    AdjustGlobalCoordinate(*logicalDisplayInfo, tCoord);
    auto targetWindowId = pointerItem.GetTargetWindowId();
    WindowInfo *touchWindow = nullptr;
    for (auto& item : logicalDisplayInfo->windowsInfo) {
        if ((item.flags & WindowInfo::FLAG_BIT_UNTOUCHABLE) == WindowInfo::FLAG_BIT_UNTOUCHABLE) {
            MMI_HILOGD("Skip the untouchable window to continue searching, "
                       "window:%{public}d, flags:%{public}d", item.id, item.flags);
            continue;
        }
        if (targetWindowId < 0) {
            if (IsInsideWindow(tCoord.x, tCoord.y, item)) {
                touchWindow = &item;
                break;
            }
        } else {
            if (targetWindowId == item.id) {
                touchWindow = &item;
                break;
            }
        }
    }
    if (touchWindow == nullptr) {
        MMI_HILOGE("touchWindow is nullptr, targetWindow:%{public}d, globalX:%{public}d, globalY:%{public}d",
            targetWindowId, tCoord.x, tCoord.y);
        return RET_ERR;
    }

    pointerEvent->SetTargetWindowId(touchWindow->id);
    pointerEvent->SetAgentWindowId(touchWindow->agentWindowId);
    int32_t localX = tCoord.x - touchWindow->winTopLeftX;
    int32_t localY = tCoord.y - touchWindow->winTopLeftY;
    pointerItem.SetGlobalX(tCoord.x);
    pointerItem.SetGlobalY(tCoord.y);
    pointerItem.SetLocalX(localX);
    pointerItem.SetLocalY(localY);
    pointerItem.SetToolLocalX(pointerItem.GetToolGlobalX() - touchWindow->winTopLeftX);
    pointerItem.SetToolLocalY(pointerItem.GetToolGlobalY() - touchWindow->winTopLeftY);
    pointerItem.SetTargetWindowId(touchWindow->id);
    pointerEvent->UpdatePointerItem(pointerId, pointerItem);
    auto fd = udsServer_->GetClientFd(touchWindow->pid);
    MMI_HILOGD("pid:%{public}d,fd:%{public}d,globalX01:%{public}d,"
               "globalY01:%{public}d,localX:%{public}d,localY:%{public}d,"
               "TargetWindowId:%{public}d,AgentWindowId:%{public}d",
               touchWindow->pid, fd, tCoord.x, tCoord.y, localX, localY,
               pointerEvent->GetTargetWindowId(), pointerEvent->GetAgentWindowId());
    return fd;
}

int32_t InputWindowsManager::UpdateTouchPadTarget(std::shared_ptr<PointerEvent> pointerEvent)
{
    CALL_LOG_ENTER;
    return RET_ERR;
}

int32_t InputWindowsManager::UpdateTargetPointer(std::shared_ptr<PointerEvent> pointerEvent)
{
    CALL_LOG_ENTER;
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    auto source = pointerEvent->GetSourceType();
    switch (source) {
        case PointerEvent::SOURCE_TYPE_TOUCHSCREEN: {
            return UpdateTouchScreenTarget(pointerEvent);
        }
        case PointerEvent::SOURCE_TYPE_MOUSE: {
            return UpdateMouseTarget(pointerEvent);
        }
        case PointerEvent::SOURCE_TYPE_TOUCHPAD: {
            return UpdateTouchPadTarget(pointerEvent);
        }
        default: {
            MMI_HILOGW("Source type is unknown, source:%{public}d", source);
            break;
        }
    }
    MMI_HILOGE("Source is not of the correct type, source:%{public}d", source);
    return RET_ERR;
}

void InputWindowsManager::UpdateAndAdjustMouseLoction(double& x, double& y)
{
    const std::vector<LogicalDisplayInfo> logicalDisplayInfo = GetLogicalDisplayInfo();
    if (logicalDisplayInfo.empty()) {
        MMI_HILOGE("logicalDisplayInfo is empty");
        return;
    }
    int32_t width = 0;
    int32_t height = 0;
    for (const auto &item : logicalDisplayInfo) {
        width += item.width;
        height += item.height;
    }
    int32_t integerX = static_cast<int32_t>(x);
    int32_t integerY = static_cast<int32_t>(y);
    if (integerX >= width && width > 0) {
        x = static_cast<double>(width);
        mouseLoction_.globalX = width - 1;
    } else if (integerX < 0) {
        x = 0;
        mouseLoction_.globalX = 0;
    } else {
        mouseLoction_.globalX = integerX;
    }
    if (integerY >= height && height > 0) {
        y = static_cast<double>(height);
        mouseLoction_.globalY = height - 1;
    } else if (integerY < 0) {
        y = 0;
        mouseLoction_.globalY = 0;
    } else {
        mouseLoction_.globalY = integerY;
    }
    MMI_HILOGD("Mouse Data: globalX:%{public}d,globalY:%{public}d", mouseLoction_.globalX, mouseLoction_.globalY);
}

MouseLocation InputWindowsManager::GetMouseInfo()
{
    if (mouseLoction_.globalX == -1 || mouseLoction_.globalY == -1) {
        if (!logicalDisplays_.empty()) {
            mouseLoction_.globalX = logicalDisplays_[0].width / 2;
            mouseLoction_.globalY = logicalDisplays_[0].height / 2;
        }
    }
    return mouseLoction_;
}
} // namespace MMI
} // namespace OHOS