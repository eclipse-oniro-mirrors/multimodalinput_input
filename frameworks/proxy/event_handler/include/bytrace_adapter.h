/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef BYTRACE_ADAPTER_H
#define BYTRACE_ADAPTER_H

#include "key_event.h"
#include "pointer_event.h"

namespace OHOS {
namespace MMI {
class BytraceAdapter final {
public:
    enum TraceBtn {
        TRACE_STOP = 0,
        TRACE_START = 1
    };
    enum HandlerType {
        KEY_INTERCEPT_EVENT = 1,
        KEY_LAUNCH_EVENT = 2,
        KEY_SUBSCRIBE_EVENT = 3,
        KEY_DISPATCH_EVENT = 4,
        POINT_INTERCEPT_EVENT = 5,
        POINT_DISPATCH_EVENT = 6,
        KEY_HOOK_EVENT = 7
    };
    enum EventType {
        START_EVENT = 1,
        LAUNCH_EVENT = 2,
        STOP_EVENT = 3
    };

    enum MMIThreadLoopDepth {
        MMI_THREAD_LOOP_DEPTH_ZERO = 0,
        MMI_THREAD_LOOP_DEPTH_ONE = 1,
        MMI_THREAD_LOOP_DEPTH_TWO = 2,
        MMI_THREAD_LOOP_DEPTH_THREE = 3,
        MMI_THREAD_LOOP_DEPTH_FOUR = 4
    };

    static void StartBytrace(std::shared_ptr<KeyEvent> keyEvent);
    static void StartBytrace(std::shared_ptr<KeyEvent> key, HandlerType handlerType);
    static void StartBytrace(std::shared_ptr<PointerEvent> pointerEvent, TraceBtn traceBtn);
    static void StartBytrace(std::shared_ptr<KeyEvent> keyEvent, TraceBtn traceBtn, HandlerType handlerType);
    static void StartBytrace(std::shared_ptr<PointerEvent> pointerEvent, TraceBtn traceBtn, HandlerType handlerType);
    static void StartBytrace(TraceBtn traceBtn, EventType eventType);

    static void StartIpcServer(uint32_t code);
    static void StopIpcServer();

    static void StartHandleInput(int32_t code);
    static void StopHandleInput();
    static void StartPackageEvent(const std::string &msg);
    static void StopPackageEvent();

    static void StartSocketHandle(int32_t msgId);
    static void StopSocketHandle();

    static void StartDevListener(const std::string& type, int32_t deviceId);
    static void StopDevListener();

    static void StartLaunchAbility(int32_t type, const std::string &bundleName);
    static void StopLaunchAbility();

    static void StartHandleTracker(int32_t pointerId);
    static void StopHandleTracker();

    static void StartConsumer(std::shared_ptr<PointerEvent> pointerEvent);
    static void StartConsumer(std::shared_ptr<KeyEvent> key);
    static void StopConsumer();

    static void StartPostTaskEvent(std::shared_ptr<PointerEvent> pointerEvent);
    static void StartPostTaskEvent(std::shared_ptr<KeyEvent> keyEvent);
    static void StopPostTaskEvent();

    static void StartMarkedTracker(int32_t eventId);
    static void StopMarkedTracker();

    static void StartTouchEvent(int32_t pointerId);
    static void StopTouchEvent();

    static void StartToolType(int32_t toolType);
    static void StopToolType();

    static void StartTouchUp(int32_t pointerId);
    static void StopTouchUp();

    static void StartUpdateDisplayMode(const std::string &modeMsg);
    static void StopUpdateDisplayMode();

    static void StartDataShare(const std::string &key);
    static void StopDataShare();

    static void StartRsSurfaceNode(uint64_t rsId);
    static void StopRsSurfaceNode();

    static void StartFoldState(bool state);
    static void StopFoldState();

    static void StartWindowVisible(int32_t pid);
    static void StopWindowVisible();

    static void StartHardPointerRender(uint32_t width, uint32_t height, uint32_t bufferId, uint32_t screenId,
        int32_t style);
    static void StopHardPointerRender();

    static void StartSoftPointerRender(uint32_t width, uint32_t height, int32_t style);
    static void StopSoftPointerRender();

    static void StartHardPointerMove(uint32_t width, uint32_t height, uint32_t bufferId, uint32_t screenId);
    static void StopHardPointerMove();

    static void MMIServiceTraceStart(int32_t type, const std::string& msg);
    static void MMIServiceTraceStop();

private:
    static std::string GetPointerTraceString(std::shared_ptr<PointerEvent> pointerEvent);
    static std::string GetKeyTraceString(std::shared_ptr<KeyEvent> keyEvent);
};
} // namespace MMI
} // namespace OHOS
#endif // BYTRACE_ADAPTER_H
