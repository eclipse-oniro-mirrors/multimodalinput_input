/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef INPUT_HANDLER_TYPE_H
#define INPUT_HANDLER_TYPE_H

namespace OHOS {
namespace MMI {
inline constexpr size_t MAX_N_INPUT_HANDLERS { 16 };
inline constexpr size_t MAX_N_INPUT_MONITORS { MAX_N_INPUT_HANDLERS };
inline constexpr size_t MAX_N_INPUT_INTERCEPTORS { MAX_N_INPUT_HANDLERS };
inline constexpr int32_t MIN_HANDLER_ID { 1 };
inline constexpr int32_t MAX_HANDLER_ID { 100000 };
inline constexpr int32_t INVALID_HANDLER_ID { -1 };
inline constexpr int32_t ERROR_EXCEED_MAX_COUNT { -4100001 };
inline constexpr int32_t DEFUALT_INTERCEPTOR_PRIORITY { 500 };
inline constexpr int32_t ALL_FINGER_COUNT = 0;
inline constexpr int32_t THREE_FINGER_COUNT = 3;
inline constexpr int32_t FOUR_FINGER_COUNT = 4;
inline constexpr int32_t MAX_FINGERS_COUNT = 5;

enum InputHandlerType : int32_t {
    NONE,
    INTERCEPTOR,
    MONITOR,
};

enum ScreenCapturePermissionType : uint32_t {
    KNUCKLE_SCREENSHOT = 1 << 0,
    KNUCKLE_SCROLL_SCREENSHOT = 1 << 1,
    KNUCKLE_ENABLE_AI_BASE = 1 << 2,
    KNUCKLE_SCREEN_RECORDING = 1 << 3,
    TOUCHPAD_KNUCKLE_SCREENSHOT = 1 << 4,
    TOUCHPAD_KNUCKLE_SCREEN_RECORDING = 1 << 5,
    SHORTCUT_KEY_SCREENSHOT = 1 << 6,
    SHORTCUT_KEY_SCREEN_RECORDING = 1 << 7,
    DEFAULT_PERMISSIONS = 0XFF,
};

using TouchGestureType = uint32_t;
inline constexpr TouchGestureType TOUCH_GESTURE_TYPE_NONE { 0x0 };
inline constexpr TouchGestureType TOUCH_GESTURE_TYPE_PINCH { 1u };
inline constexpr TouchGestureType TOUCH_GESTURE_TYPE_SWIPE { 1u << 1 };
inline constexpr TouchGestureType TOUCH_GESTURE_TYPE_ALL {
    TOUCH_GESTURE_TYPE_PINCH | TOUCH_GESTURE_TYPE_SWIPE
};

using HandleEventType = uint32_t;
inline constexpr HandleEventType HANDLE_EVENT_TYPE_NONE {0x0};
inline constexpr HandleEventType HANDLE_EVENT_TYPE_KEY {1u};
inline constexpr HandleEventType HANDLE_EVENT_TYPE_POINTER {1u << 1};
inline constexpr HandleEventType HANDLE_EVENT_TYPE_TOUCH_GESTURE {1u << 2};
inline constexpr HandleEventType HANDLE_EVENT_TYPE_TOUCH { 1u << 3 };
inline constexpr HandleEventType HANDLE_EVENT_TYPE_MOUSE { 1u << 4 };
inline constexpr HandleEventType HANDLE_EVENT_TYPE_PINCH { 1u << 5 };
inline constexpr HandleEventType HANDLE_EVENT_TYPE_THREEFINGERSSWIP { 1u << 6 };
inline constexpr HandleEventType HANDLE_EVENT_TYPE_FOURFINGERSSWIP {1u << 7 };
inline constexpr HandleEventType HANDLE_EVENT_TYPE_ROTATE { 1u << 8 };
inline constexpr HandleEventType HANDLE_EVENT_TYPE_THREEFINGERSTAP { 1u << 9 };
inline constexpr HandleEventType HANDLE_EVENT_TYPE_SWIPEINWARD { 1u << 10 };
inline constexpr HandleEventType HANDLE_EVENT_TYPE_PRE_KEY { 1u << 11 };
inline constexpr HandleEventType HANDLE_EVENT_TYPE_ALL {
    HANDLE_EVENT_TYPE_KEY | HANDLE_EVENT_TYPE_POINTER | HANDLE_EVENT_TYPE_TOUCH_GESTURE |
    HANDLE_EVENT_TYPE_TOUCH | HANDLE_EVENT_TYPE_MOUSE | HANDLE_EVENT_TYPE_PINCH |
    HANDLE_EVENT_TYPE_THREEFINGERSSWIP | HANDLE_EVENT_TYPE_FOURFINGERSSWIP |
    HANDLE_EVENT_TYPE_ROTATE | HANDLE_EVENT_TYPE_THREEFINGERSTAP | HANDLE_EVENT_TYPE_SWIPEINWARD |
    HANDLE_EVENT_TYPE_PRE_KEY
};
inline constexpr HandleEventType HANDLE_EVENT_TYPE_KP {HANDLE_EVENT_TYPE_KEY | HANDLE_EVENT_TYPE_POINTER};
inline constexpr HandleEventType HANDLE_EVENT_TYPE_FINGERPRINT {HANDLE_EVENT_TYPE_KEY | HANDLE_EVENT_TYPE_POINTER};
inline constexpr HandleEventType HANDLE_EVENT_TYPE_X_KEY {HANDLE_EVENT_TYPE_KEY | HANDLE_EVENT_TYPE_POINTER};

inline bool IsValidHandlerType(InputHandlerType handlerType)
{
    return ((handlerType == InputHandlerType::INTERCEPTOR) ||
        (handlerType == InputHandlerType::MONITOR));
}

inline bool IsValidHandlerId(int32_t handlerId)
{
    return ((handlerId >= MIN_HANDLER_ID) && (handlerId < MAX_HANDLER_ID));
}
} // namespace MMI
} // namespace OHOS
#endif // INPUT_HANDLER_TYPE_H