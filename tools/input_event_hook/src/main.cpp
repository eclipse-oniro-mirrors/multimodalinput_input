/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include "oh_input_manager.h"

namespace {
std::atomic<bool> g_stop { false };
std::atomic<bool> g_consume { false };
std::thread g_flusher;
// 后台刷写线程的冲刷周期（毫秒）
constexpr int32_t FLUSH_INTERVAL_MS { 200 };
// stdout 全缓冲大小（字节）
constexpr int FLUSH_BUFFER_SIZE { 1 << 16 };
constexpr OH_Input_EventType UNSET_EVENT_TYPE { static_cast<OH_Input_EventType>(-1) };


// 后台刷写线程：stdout 为全缓冲（回调线程只写内存缓冲，避免终端 I/O 拖慢回调
// 触发超时禁用），由本线程周期性冲刷，兼顾输出及时性与回调轻快
void FlushLoop()
{
    while (!g_stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(FLUSH_INTERVAL_MS));
        fflush(stdout);
    }
    fflush(stdout);
}

void OnSignal(int32_t)
{
    g_stop.store(true);
}

const char* KeyActionToString(int32_t action)
{
    switch (action) {
        case KEY_ACTION_CANCEL:
            return "CANCEL";
        case KEY_ACTION_DOWN:
            return "DOWN";
        case KEY_ACTION_UP:
            return "UP";
        default:
            return "UNKNOWN";
    }
}

const char* MouseActionToString(int32_t action)
{
    switch (action) {
        case MOUSE_ACTION_CANCEL:
            return "CANCEL";
        case MOUSE_ACTION_MOVE:
            return "MOVE";
        case MOUSE_ACTION_BUTTON_DOWN:
            return "BUTTON_DOWN";
        case MOUSE_ACTION_BUTTON_UP:
            return "BUTTON_UP";
        case MOUSE_ACTION_AXIS_BEGIN:
            return "AXIS_BEGIN";
        case MOUSE_ACTION_AXIS_UPDATE:
            return "AXIS_UPDATE";
        case MOUSE_ACTION_AXIS_END:
            return "AXIS_END";
        default:
            return "UNKNOWN";
    }
}

const char* MouseButtonToString(int32_t button)
{
    switch (button) {
        case MOUSE_BUTTON_NONE:
            return "NONE";
        case MOUSE_BUTTON_LEFT:
            return "LEFT";
        case MOUSE_BUTTON_MIDDLE:
            return "MIDDLE";
        case MOUSE_BUTTON_RIGHT:
            return "RIGHT";
        case MOUSE_BUTTON_FORWARD:
            return "FORWARD";
        case MOUSE_BUTTON_BACK:
            return "BACK";
        default:
            return "UNKNOWN";
    }
}

const char* TouchActionToString(int32_t action)
{
    switch (action) {
        case TOUCH_ACTION_CANCEL:
            return "CANCEL";
        case TOUCH_ACTION_DOWN:
            return "DOWN";
        case TOUCH_ACTION_MOVE:
            return "MOVE";
        case TOUCH_ACTION_UP:
            return "UP";
        default:
            return "UNKNOWN";
    }
}

const char* AxisActionToString(int32_t action)
{
    switch (action) {
        case AXIS_ACTION_CANCEL:
            return "CANCEL";
        case AXIS_ACTION_BEGIN:
            return "BEGIN";
        case AXIS_ACTION_UPDATE:
            return "UPDATE";
        case AXIS_ACTION_END:
            return "END";
        default:
            return "UNKNOWN";
    }
}

void PrintKeyEvent(const Input_KeyEvent* key)
{
    int32_t eventId { -1 };
    OH_Input_GetKeyEventId(key, &eventId);
    printf("[key]   action=%-6s eventId=%d actionTime=%lld windowId=%d displayId=%d\n",
        KeyActionToString(OH_Input_GetKeyEventAction(key)), eventId,
        static_cast<long long>(OH_Input_GetKeyEventActionTime(key)), OH_Input_GetKeyEventWindowId(key),
        OH_Input_GetKeyEventDisplayId(key));
}

void PrintMouseEvent(const Input_MouseEvent* mouse)
{
    printf("[mouse] action=%-10s button=%s actionTime=%lld windowId=%d\n",
        MouseActionToString(OH_Input_GetMouseEventAction(mouse)),
        MouseButtonToString(OH_Input_GetMouseEventButton(mouse)),
        static_cast<long long>(OH_Input_GetMouseEventActionTime(mouse)), OH_Input_GetMouseEventWindowId(mouse));
}

void PrintTouchEvent(const Input_TouchEvent* touch)
{
    printf("[touch] action=%-6s id=%d pressure=%.3f actionTime=%lld windowId=%d displayId=%d\n",
        TouchActionToString(OH_Input_GetTouchEventAction(touch)), OH_Input_GetTouchEventFingerId(touch),
        OH_Input_GetTouchEventPressure(touch), static_cast<long long>(OH_Input_GetTouchEventActionTime(touch)),
        OH_Input_GetTouchEventWindowId(touch), OH_Input_GetTouchEventDisplayId(touch));
}

void PrintAxisEvent(const Input_AxisEvent* axis)
{
    InputEvent_AxisAction action { AXIS_ACTION_CANCEL };
    InputEvent_AxisEventType axisEventType { AXIS_EVENT_TYPE_SCROLL };
    InputEvent_SourceType sourceType { SOURCE_TYPE_MOUSE };
    int64_t actionTime { 0 };
    if (OH_Input_GetAxisEventAction(axis, &action) != INPUT_SUCCESS ||
        OH_Input_GetAxisEventType(axis, &axisEventType) != INPUT_SUCCESS ||
        OH_Input_GetAxisEventSourceType(axis, &sourceType) != INPUT_SUCCESS ||
        OH_Input_GetAxisEventActionTime(axis, &actionTime) != INPUT_SUCCESS) {
        return;
    }
    printf("[axis]  action=%-6s eventType=%d sourceType=%d actionTime=%lld\n",
        AxisActionToString(action), axisEventType, sourceType, static_cast<long long>(actionTime));
}

// Synchronous callback: returning event (non-NULL) passes the event on to the next handler or the
// target application; returning NULL consumes (blocks) it so it never reaches the target application.
// With --consume enabled, NULL is returned.
const OH_Input_Event* OnEvent(OH_Input_EventHandler*, const OH_Input_Event* event)
{
    OH_Input_EventType type { UNSET_EVENT_TYPE };
    if (OH_Input_Event_GetType(event, &type) != INPUT_SUCCESS) {
        printf("[hook] OH_Input_Event_GetType failed\n");
        return event;
    }
    switch (type) {
        case OH_INPUT_EVENT_KEY: {
            const Input_KeyEvent* key { nullptr };
            if (OH_Input_Event_AsKeyEvent(event, &key) == INPUT_SUCCESS && key != nullptr) {
                PrintKeyEvent(key);
            }
            break;
        }
        case OH_INPUT_EVENT_MOUSE: {
            const Input_MouseEvent* mouse { nullptr };
            if (OH_Input_Event_AsMouseEvent(event, &mouse) == INPUT_SUCCESS && mouse != nullptr) {
                PrintMouseEvent(mouse);
            }
            break;
        }
        case OH_INPUT_EVENT_TOUCH: {
            const Input_TouchEvent* touch { nullptr };
            if (OH_Input_Event_AsTouchEvent(event, &touch) == INPUT_SUCCESS && touch != nullptr) {
                PrintTouchEvent(touch);
            }
            break;
        }
        case OH_INPUT_EVENT_AXIS: {
            const Input_AxisEvent* axis { nullptr };
            if (OH_Input_Event_AsAxisEvent(event, &axis) == INPUT_SUCCESS && axis != nullptr) {
                PrintAxisEvent(axis);
            }
            break;
        }
        default:
            printf("[hook] unknown event type=%d\n", type);
            break;
    }
    if (g_consume.load()) {
        printf("[hook] consumed (event dropped, does NOT reach the app)\n");
        return nullptr;
    }
    return event;
}

void OnDisabled(OH_Input_EventHandler*)
{
    printf("[hook] handler disabled (event callback timeout)\n");
    fflush(stdout);
}

void PrintUsage(const char* prog)
{
    printf("Usage: %s [-k] [-m] [-t] [-a] [--all] [-c] [-d SECONDS] [-h]\n"
           "  -k, --key        intercept key events\n"
           "  -m, --mouse      intercept mouse events\n"
           "  -t, --touch      intercept touchscreen events (default)\n"
           "  -a, --axis       intercept axis (mouse wheel) events\n"
           "      --all        intercept all four event types\n"
           "  -c, --consume    consume (drop) matched events instead of passing them through\n"
           "  -d, --duration   run for SECONDS then exit (0 = run until Ctrl-C)\n"
           "  -h, --help       print this help\n",
        prog);
}

void PrintMaskDescription(OH_Input_EventTypeMask mask)
{
    printf("intercepting:");
    if ((mask & OH_INPUT_EVENT_TYPE_MASK_KEY) != 0) {
        printf(" key");
    }
    if ((mask & OH_INPUT_EVENT_TYPE_MASK_MOUSE) != 0) {
        printf(" mouse");
    }
    if ((mask & OH_INPUT_EVENT_TYPE_MASK_TOUCH) != 0) {
        printf(" touch");
    }
    if ((mask & OH_INPUT_EVENT_TYPE_MASK_AXIS) != 0) {
        printf(" axis");
    }
    printf("\n");
}

const char* ErrorCodeToString(int32_t ret)
{
    switch (ret) {
        case INPUT_PERMISSION_DENIED:
            return "INPUT_PERMISSION_DENIED (missing ohos.permission.HOOK_INPUT_EVENT)";
        case INPUT_PARAMETER_ERROR:
            return "INPUT_PARAMETER_ERROR";
        case INPUT_SERVICE_EXCEPTION:
            return "INPUT_SERVICE_EXCEPTION (MMI service abnormal or not running)";
        case INPUT_REPEAT_INTERCEPTOR:
            return "INPUT_REPEAT_INTERCEPTOR (another handler already intercepts the same event type)";
        default:
            return "";
    }
}
} // namespace

int main(int argc, char* argv[])
{
    // Force full buffering on stdout: the callback thread prints synchronously, and per-line writes in
    // line-buffered/terminal mode slow down individual callbacks enough to hit the 1S/1.5S timeout
    // disable; batched output keeps callbacks fast (flushed automatically on normal exit)
    setvbuf(stdout, nullptr, _IOFBF, FLUSH_BUFFER_SIZE);
    OH_Input_EventTypeMask mask { 0 };
    bool anyType { false };
    int32_t durationSec { 0 };

    for (int32_t i = 1; i < argc; ++i) {
        std::string arg { argv[i] };
        if (arg == "-k" || arg == "--key") {
            mask |= OH_INPUT_EVENT_TYPE_MASK_KEY;
            anyType = true;
        } else if (arg == "-m" || arg == "--mouse") {
            mask |= OH_INPUT_EVENT_TYPE_MASK_MOUSE;
            anyType = true;
        } else if (arg == "-t" || arg == "--touch") {
            mask |= OH_INPUT_EVENT_TYPE_MASK_TOUCH;
            anyType = true;
        } else if (arg == "-a" || arg == "--axis") {
            mask |= OH_INPUT_EVENT_TYPE_MASK_AXIS;
            anyType = true;
        } else if (arg == "--all") {
            mask = OH_INPUT_EVENT_TYPE_MASK_KEY | OH_INPUT_EVENT_TYPE_MASK_MOUSE | OH_INPUT_EVENT_TYPE_MASK_TOUCH |
                OH_INPUT_EVENT_TYPE_MASK_AXIS;
            anyType = true;
        } else if (arg == "-c" || arg == "--consume") {
            g_consume.store(true);
        } else if (arg == "-d" || arg == "--duration") {
            if (i + 1 < argc) {
                durationSec = std::atoi(argv[++i]);
            }
        } else if (arg == "-h" || arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else {
            printf("unknown argument: %s\n", arg.c_str());
            PrintUsage(argv[0]);
            return 1;
        }
    }
    if (!anyType) {
        mask = OH_INPUT_EVENT_TYPE_MASK_TOUCH;
    }
    PrintMaskDescription(mask);
    printf("mode: %s\n", g_consume.load() ? "consume (drop matched events)" : "pass-through (events reach the app)");

    OH_Input_EventHandler* handler { nullptr };
    Input_Result ret = OH_Input_EventHandler_Create(mask, OnEvent, OnDisabled, &handler);
    if (ret != INPUT_SUCCESS) {
        printf("OH_Input_EventHandler_Create failed: %d\n", ret);
        return 1;
    }

    ret = OH_Input_EventHandler_Enable(handler, true);
    if (ret != INPUT_SUCCESS) {
        printf("OH_Input_EventHandler_Enable(true) failed: %d %s\n", ret, ErrorCodeToString(ret));
        OH_Input_EventHandler_Destroy(handler);
        return 1;
    }

    bool enabled { false };
    OH_Input_EventHandler_IsEnabled(handler, &enabled);
    printf("handler enabled=%d, waiting for events (Ctrl-C to exit)...\n", enabled ? 1 : 0);
    fflush(stdout);

    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);
    g_flusher = std::thread(FlushLoop);

    auto start = std::chrono::steady_clock::now();
    while (!g_stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(FLUSH_INTERVAL_MS));
        if (durationSec > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= durationSec) {
                break;
            }
        }
    }

    printf("stopping...\n");
    fflush(stdout);
    g_stop.store(true);
    if (g_flusher.joinable()) {
        g_flusher.join();
    }
    OH_Input_EventHandler_Enable(handler, false);
    OH_Input_EventHandler_Destroy(handler);
    return 0;
}
