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

#include "key_event_value_transformation.h"

#include <map>

#include "hos_key_event.h"
#include "util.h"

#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_DISPATCH
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "KeyEventValueTransformation"

namespace OHOS {
namespace MMI {
namespace {
constexpr uint32_t BIT_SET_INDEX { 16 };
constexpr int32_t INVALID_KEY_CODE { -1 };
constexpr int32_t MAX_KEY_SIZE { 3 };
constexpr int32_t MIN_KEY_SIZE { 1 };
} // namespace

const std::multimap<int32_t, KeyEventValueTransformation> MAP_KEY_EVENT_VALUE_TRANSFORMATION = {
    {11, {"KEY_0", 11, 2000, HOS_KEY_0}},
    {2, {"KEY_1", 2, 2001, HOS_KEY_1}},
    {3, {"KEY_2", 3, 2002, HOS_KEY_2}},
    {4, {"KEY_3", 4, 2003, HOS_KEY_3}},
    {5, {"KEY_4", 5, 2004, HOS_KEY_4}},
    {6, {"KEY_5", 6, 2005, HOS_KEY_5}},
    {7, {"KEY_6", 7, 2006, HOS_KEY_6}},
    {8, {"KEY_7", 8, 2007, HOS_KEY_7}},
    {9, {"KEY_8", 9, 2008, HOS_KEY_8}},
    {10, {"KEY_9", 10, 2009, HOS_KEY_9}},
    {522, {"KEY_STAR", 522, 2010, HOS_KEY_STAR}},
    {523, {"KEY_POUND", 523, 2011, HOS_KEY_POUND}},
    {103, {"KEY_DPAD_UP", 103, 2012, HOS_KEY_DPAD_UP}},
    {108, {"KEY_DPAD_DOWN", 108, 2013, HOS_KEY_DPAD_DOWN}},
    {105, {"KEY_DPAD_LEFT", 105, 2014, HOS_KEY_DPAD_LEFT}},
    {106, {"KEY_DPAD_RIGHT", 106, 2015, HOS_KEY_DPAD_RIGHT}},
    {353, {"KEY_DPAD_CENTER", 353, 2016, HOS_KEY_DPAD_CENTER}},
    {30, {"KEY_A", 30, 2017, HOS_KEY_A}},
    {48, {"KEY_B", 48, 2018, HOS_KEY_B}},
    {46, {"KEY_C", 46, 2019, HOS_KEY_C}},
    {32, {"KEY_D", 32, 2020, HOS_KEY_D}},
    {18, {"KEY_E", 18, 2021, HOS_KEY_E}},
    {33, {"KEY_F", 33, 2022, HOS_KEY_F}},
    {34, {"KEY_G", 34, 2023, HOS_KEY_G}},
    {35, {"KEY_H", 35, 2024, HOS_KEY_H}},
    {23, {"KEY_I", 23, 2025, HOS_KEY_I}},
    {36, {"KEY_J", 36, 2026, HOS_KEY_J}},
    {37, {"KEY_K", 37, 2027, HOS_KEY_K}},
    {38, {"KEY_L", 38, 2028, HOS_KEY_L}},
    {50, {"KEY_M", 50, 2029, HOS_KEY_M}},
    {49, {"KEY_N", 49, 2030, HOS_KEY_N}},
    {24, {"KEY_O", 24, 2031, HOS_KEY_O}},
    {25, {"KEY_P", 25, 2032, HOS_KEY_P}},
    {16, {"KEY_Q", 16, 2033, HOS_KEY_Q}},
    {19, {"KEY_R", 19, 2034, HOS_KEY_R}},
    {31, {"KEY_S", 31, 2035, HOS_KEY_S}},
    {20, {"KEY_T", 20, 2036, HOS_KEY_T}},
    {22, {"KEY_U", 22, 2037, HOS_KEY_U}},
    {47, {"KEY_V", 47, 2038, HOS_KEY_V}},
    {17, {"KEY_W", 17, 2039, HOS_KEY_W}},
    {45, {"KEY_X", 45, 2040, HOS_KEY_X}},
    {21, {"KEY_Y", 21, 2041, HOS_KEY_Y}},
    {44, {"KEY_Z", 44, 2042, HOS_KEY_Z}},
    {51, {"KEY_COMMA", 51, 2043, HOS_KEY_COMMA}},
    {52, {"KEY_PERIOD", 52, 2044, HOS_KEY_PERIOD}},
    {56, {"KEY_ALT_LEFT", 56, 2045, HOS_KEY_ALT_LEFT}},
    {100, {"KEY_ALT_RIGHT", 100, 2046, HOS_KEY_ALT_RIGHT}},
    {42, {"KEY_SHIFT_LEFT", 42, 2047, HOS_KEY_SHIFT_LEFT}},
    {54, {"KEY_SHIFT_RIGHT", 54, 2048, HOS_KEY_SHIFT_RIGHT}},
    {15, {"KEY_TAB", 15, 2049, HOS_KEY_TAB}},
    {57, {"KEY_SPACE", 57, 2050, HOS_KEY_SPACE}},
    {150, {"KEY_EXPLORER", 150, 2052, HOS_KEY_EXPLORER}},
    {155, {"KEY_ENVELOPE", 155, 2053, HOS_KEY_ENVELOPE}},
    {28, {"KEY_ENTER", 28, 2054, HOS_KEY_ENTER}},
    {14, {"KEY_DEL", 14, 2055, HOS_KEY_DEL}},
    {41, {"KEY_GRAVE", 41, 2056, HOS_KEY_GRAVE}},
    {12, {"KEY_MINUS", 12, 2057, HOS_KEY_MINUS}},
    {13, {"KEY_EQUALS", 13, 2058, HOS_KEY_EQUALS}},
    {26, {"KEY_LEFT_BRACKET", 26, 2059, HOS_KEY_LEFT_BRACKET}},
    {27, {"KEY_RIGHT_BRACKET", 27, 2060, HOS_KEY_RIGHT_BRACKET}},
    {43, {"KEY_BACKSLASH", 43, 2061, HOS_KEY_BACKSLASH}},
    {39, {"KEY_SEMICOLON", 39, 2062, HOS_KEY_SEMICOLON}},
    {40, {"KEY_APOSTROPHE", 40, 2063, HOS_KEY_APOSTROPHE}},
    {53, {"KEY_SLASH", 53, 2064, HOS_KEY_SLASH}},
    {139, {"KEY_MENU", 139, 2067, HOS_KEY_MENU}},
    {127, {"KEY_MENU", 127, 2466, HOS_KEY_COMPOSE}},
    {104, {"KEY_PAGE_UP", 104, 2068, HOS_KEY_PAGE_UP}},
    {109, {"KEY_PAGE_DOWN", 109, 2069, HOS_KEY_PAGE_DOWN}},
    {1, {"KEY_ESCAPE", 1, 2070, HOS_KEY_ESCAPE}},
    {111, {"KEY_FORWARD_DEL", 111, 2071, HOS_KEY_FORWARD_DEL}},
    {29, {"KEY_CTRL_LEFT", 29, 2072, HOS_KEY_CTRL_LEFT}},
    {97, {"KEY_CTRL_RIGHT", 97, 2073, HOS_KEY_CTRL_RIGHT}},
    {58, {"KEY_CAPS_LOCK", 58, 2074, HOS_KEY_CAPS_LOCK}},
    {70, {"KEY_SCROLL_LOCK", 70, 2075, HOS_KEY_SCROLL_LOCK}},
    {125, {"KEY_META_LEFT", 125, 2076, HOS_KEY_META_LEFT}},
    {126, {"KEY_META_RIGHT", 126, 2077, HOS_KEY_META_RIGHT}},
    {464, {"KEY_FUNCTION", 464, 2078, HOS_KEY_FUNCTION}},
    {766, {"KEY_FUNCTION", 766, 2078, HOS_KEY_FUNCTION}},
    {99, {"KEY_SYSRQ", 99, 2079, HOS_KEY_SYSRQ}},
    {119, {"KEY_BREAK", 119, 2080, HOS_KEY_BREAK}},
    {102, {"KEY_MOVE_HOME", 102, 2081, HOS_KEY_MOVE_HOME}},
    {107, {"KEY_MOVE_END", 107, 2082, HOS_KEY_MOVE_END}},
    {110, {"KEY_INSERT", 110, 2083, HOS_KEY_INSERT}},
    {159, {"KEY_FORWARD", 159, 2084, HOS_KEY_FORWARD}},
    {207, {"KEY_MEDIA_PLAY", 207, 2085, HOS_KEY_MEDIA_PLAY}},
    {200, {"KEY_PLAY", 200, 2643, HOS_KEY_PLAY}},
    {201, {"KEY_MEDIA_PAUSE", 201, 2086, HOS_KEY_MEDIA_PAUSE}},
    {160, {"KEY_MEDIA_CLOSE", 160, 2087, HOS_KEY_MEDIA_CLOSE}},
    {161, {"KEY_MEDIA_EJECT", 161, 2088, HOS_KEY_MEDIA_EJECT}},
    {597, {"KEY_MEDIA_RECORD", 597, 2089, HOS_KEY_MEDIA_RECORD}},
    {59, {"KEY_F1", 59, 2090, HOS_KEY_F1}},
    {60, {"KEY_F2", 60, 2091, HOS_KEY_F2}},
    {61, {"KEY_F3", 61, 2092, HOS_KEY_F3}},
    {62, {"KEY_F4", 62, 2093, HOS_KEY_F4}},
    {63, {"KEY_F5", 63, 2094, HOS_KEY_F5}},
    {64, {"KEY_F6", 64, 2095, HOS_KEY_F6}},
    {65, {"KEY_F7", 65, 2096, HOS_KEY_F7}},
    {66, {"KEY_F8", 66, 2097, HOS_KEY_F8}},
    {67, {"KEY_F9", 67, 2098, HOS_KEY_F9}},
    {68, {"KEY_F10", 68, 2099, HOS_KEY_F10}},
    {87, {"KEY_F11", 87, 2100, HOS_KEY_F11}},
    {88, {"KEY_F12", 88, 2101, HOS_KEY_F12}},
    {69, {"KEY_NUM_LOCK", 69, 2102, HOS_KEY_NUM_LOCK}},
    {82, {"KEY_NUMPAD_0", 82, 2103, HOS_KEY_NUMPAD_0}},
    {79, {"KEY_NUMPAD_1", 79, 2104, HOS_KEY_NUMPAD_1}},
    {80, {"KEY_NUMPAD_2", 80, 2105, HOS_KEY_NUMPAD_2}},
    {81, {"KEY_NUMPAD_3", 81, 2106, HOS_KEY_NUMPAD_3}},
    {75, {"KEY_NUMPAD_4", 75, 2107, HOS_KEY_NUMPAD_4}},
    {76, {"KEY_NUMPAD_5", 76, 2108, HOS_KEY_NUMPAD_5}},
    {77, {"KEY_NUMPAD_6", 77, 2109, HOS_KEY_NUMPAD_6}},
    {71, {"KEY_NUMPAD_7", 71, 2110, HOS_KEY_NUMPAD_7}},
    {72, {"KEY_NUMPAD_8", 72, 2111, HOS_KEY_NUMPAD_8}},
    {73, {"KEY_NUMPAD_9", 73, 2112, HOS_KEY_NUMPAD_9}},
    {98, {"KEY_NUMPAD_DIVIDE", 98, 2113, HOS_KEY_NUMPAD_DIVIDE}},
    {55, {"KEY_NUMPAD_MULTIPLY", 55, 2114, HOS_KEY_NUMPAD_MULTIPLY}},
    {74, {"KEY_NUMPAD_SUBTRACT", 74, 2115, HOS_KEY_NUMPAD_SUBTRACT}},
    {78, {"KEY_NUMPAD_ADD", 78, 2116, HOS_KEY_NUMPAD_ADD}},
    {83, {"KEY_NUMPAD_DOT", 83, 2117, HOS_KEY_NUMPAD_DOT}},
    {95, {"KEY_NUMPAD_COMMA", 95, 2118, HOS_KEY_NUMPAD_COMMA}},
    {121, {"KEY_NUMPAD_COMMA", 121, 2118, HOS_KEY_NUMPAD_COMMA}},
    {96, {"KEY_NUMPAD_ENTER", 96, 2119, HOS_KEY_NUMPAD_ENTER}},
    {117, {"KEY_NUMPAD_EQUALS", 117, 2120, HOS_KEY_NUMPAD_EQUALS}},
    {179, {"KEY_NUMPAD_LEFT_PAREN", 179, 2121, HOS_KEY_NUMPAD_LEFT_PAREN}},
    {180, {"KEY_NUMPAD_RIGHT_PAREN", 180, 2122, HOS_KEY_NUMPAD_RIGHT_PAREN}},

    {115, {"KEY_VOLUME_UP", 115, 16, HOS_KEY_VOLUME_UP}},
    {114, {"KEY_VOLUME_DOWN", 114, 17, HOS_KEY_VOLUME_DOWN}},
    {116, {"KEY_POWER", 116, 18, HOS_KEY_POWER}},
    {113, {"KEY_VOLUME_MUTE", 113, 22, HOS_KEY_VOLUME_MUTE}},

    {172, {"KEY_HOME", 172, 1, HOS_KEY_HOME}},
    {158, {"KEY_BACK", 158, 2, HOS_KEY_BACK}},
    {640, {"KEY_VIRTUAL_MULTITASK", 640, 2210, HOS_KEY_VIRTUAL_MULTITASK}},

    {304, {"KEY_BUTTON_A", 304, 2301, HOS_KEY_BUTTON_A}},
    {305, {"KEY_BUTTON_B", 305, 2302, HOS_KEY_BUTTON_B}},
    {306, {"KEY_BUTTON_C", 306, 2303, HOS_KEY_BUTTON_C}},
    {307, {"KEY_BUTTON_X", 307, 2304, HOS_KEY_BUTTON_X}},
    {308, {"KEY_BUTTON_Y", 308, 2305, HOS_KEY_BUTTON_Y}},
    {309, {"KEY_BUTTON_Z", 309, 2306, HOS_KEY_BUTTON_Z}},
    {310, {"KEY_BUTTON_L1", 310, 2307, HOS_KEY_BUTTON_L1}},
    {311, {"KEY_BUTTON_R1", 311, 2308, HOS_KEY_BUTTON_R1}},
    {312, {"KEY_BUTTON_L2", 312, 2309, HOS_KEY_BUTTON_L2}},
    {313, {"KEY_BUTTON_R2", 313, 2310, HOS_KEY_BUTTON_R2}},
    {314, {"KEY_BUTTON_SELECT", 314, 2311, HOS_KEY_BUTTON_SELECT}},
    {315, {"KEY_BUTTON_START", 315, 2312, HOS_KEY_BUTTON_START}},
    {316, {"KEY_BUTTON_MODE", 316, 2313, HOS_KEY_BUTTON_MODE}},
    {317, {"KEY_BUTTON_THUMBL", 317, 2314, HOS_KEY_BUTTON_THUMBL}},
    {318, {"KEY_BUTTON_THUMBR", 318, 2315, HOS_KEY_BUTTON_THUMBR}},

    {288, {"KEY_BUTTON_TRIGGER", 288, 2401, HOS_KEY_BUTTON_TRIGGER}},
    {289, {"KEY_BUTTON_THUMB", 289, 2402, HOS_KEY_BUTTON_THUMB}},
    {290, {"KEY_BUTTON_THUMB2", 290, 2403, HOS_KEY_BUTTON_THUMB2}},
    {291, {"KEY_BUTTON_TOP", 291, 2404, HOS_KEY_BUTTON_TOP}},
    {292, {"KEY_BUTTON_TOP2", 292, 2405, HOS_KEY_BUTTON_TOP2}},
    {293, {"KEY_BUTTON_PINKIE", 293, 2406, HOS_KEY_BUTTON_PINKIE}},
    {294, {"KEY_BUTTON_BASE1", 294, 2407, HOS_KEY_BUTTON_BASE1}},
    {295, {"KEY_BUTTON_BASE2", 295, 2408, HOS_KEY_BUTTON_BASE2}},
    {296, {"KEY_BUTTON_BASE3", 296, 2409, HOS_KEY_BUTTON_BASE3}},
    {297, {"KEY_BUTTON_BASE4", 297, 2410, HOS_KEY_BUTTON_BASE4}},
    {298, {"KEY_BUTTON_BASE5", 298, 2411, HOS_KEY_BUTTON_BASE5}},
    {299, {"KEY_BUTTON_BASE6", 299, 2412, HOS_KEY_BUTTON_BASE6}},
    {300, {"KEY_BUTTON_BASE7", 300, 2413, HOS_KEY_BUTTON_BASE7}},
    {301, {"KEY_BUTTON_BASE8", 301, 2414, HOS_KEY_BUTTON_BASE8}},
    {302, {"KEY_BUTTON_BASE9", 302, 2415, HOS_KEY_BUTTON_BASE9}},
    {303, {"KEY_BUTTON_DEAD", 303, 2416, HOS_KEY_BUTTON_DEAD}},

    {330, {"BUTTON_TOUCH", 330, 2500, HOS_BUTTON_TOUCH}},
    {320, {"BUTTON_TOOL_PEN", 320, 2501, HOS_BUTTON_TOOL_PEN}},
    {321, {"BUTTON_TOOL_RUBBER", 321, 2502, HOS_BUTTON_TOOL_RUBBER}},
    {322, {"BUTTON_TOOL_BRUSH", 322, 2503, HOS_BUTTON_TOOL_BRUSH}},
    {323, {"BUTTON_TOOL_PENCIL", 323, 2504, HOS_BUTTON_TOOL_PENCIL}},
    {324, {"BUTTON_TOOL_AIRBRUSH", 324, 2505, HOS_BUTTON_TOOL_AIRBRUSH}},
    {325, {"BUTTON_TOOL_FINGER", 325, 2506, HOS_BUTTON_TOOL_FINGER}},
    {326, {"BUTTON_TOOL_MOUSE", 326, 2507, HOS_BUTTON_TOOL_MOUSE}},
    {327, {"BUTTON_TOOL_LENS", 327, 2508, HOS_BUTTON_TOOL_LENS}},
    {331, {"BUTTON_STYLUS", 331, 2509, HOS_BUTTON_STYLUS}},
    {332, {"BUTTON_STYLUS2", 332, 2510, HOS_BUTTON_STYLUS2}},
    {329, {"BUTTON_STYLUS3", 329, 2511, HOS_BUTTON_STYLUS3}},
    {333, {"BUTTON_TOOL_DOUBLETAP", 333, 2512, HOS_BUTTON_TOOL_DOUBLETAP}},
    {334, {"BUTTON_TOOL_TRIPLETAP", 334, 2513, HOS_BUTTON_TOOL_TRIPLETAP}},
    {335, {"BUTTON_TOOL_QUADTAP", 335, 2514, HOS_BUTTON_TOOL_QUADTAP}},
    {328, {"BUTTON_TOOL_QUINTTAP", 328, 2515, HOS_BUTTON_TOOL_QUINTTAP}},

    {212, {"KEY_CAMERA", 212, 19, HOS_KEY_CAMERA}},
    {225, {"KEY_BRIGHTNESS_UP", 225, 40, HOS_KEY_BRIGHTNESS_UP}},
    {224, {"KEY_BRIGHTNESS_DOWN", 224, 41, HOS_KEY_BRIGHTNESS_DOWN}},
    {355, {"KEY_CLEAR", 355, 5, HOS_KEY_CLEAR}},
    {528, {"KEY_FOCUS", 528, 7, HOS_KEY_FOCUS}},
    {217, {"KEY_SEARCH", 217, 9, HOS_KEY_SEARCH}},
    {164, {"KEY_MEDIA_PLAY_PAUSE", 164, 10, HOS_KEY_MEDIA_PLAY_PAUSE}},
    {166, {"KEY_MEDIA_STOP", 166, 11, HOS_KEY_MEDIA_STOP}},
    {163, {"KEY_MEDIA_NEXT", 163, 12, HOS_KEY_MEDIA_NEXT}},
    {165, {"KEY_MEDIA_PREVIOUS", 165, 13, HOS_KEY_MEDIA_PREVIOUS}},
    {168, {"KEY_MEDIA_REWIND", 168, 14, HOS_KEY_MEDIA_REWIND}},
    {208, {"KEY_MEDIA_FAST_FORWARD", 208, 15, HOS_KEY_MEDIA_FAST_FORWARD}},
    {582, {"KEY_VOICE_ASSISTANT", 582, 20, HOS_KEY_VOICE_ASSISTANT}},
    {240, {"KEY_FN", 240, 0, HOS_KEY_FN}},

    {142, {"KEY_SLEEP", 142, 2600, HOS_KEY_SLEEP}},
    {85, {"KEY_ZENKAKU_HANKAKU", 85, 2601, HOS_KEY_ZENKAKU_HANKAKU}},
    {86, {"KEY_102ND", 86, 2602, HOS_KEY_102ND}},
    {89, {"KEY_RO", 89, 2603, HOS_KEY_RO}},
    {90, {"KEY_KATAKANA", 90, 2604, HOS_KEY_KATAKANA}},
    {91, {"KEY_HIRAGANA", 91, 2605, HOS_KEY_HIRAGANA}},
    {92, {"KEY_HENKAN", 92, 2606, HOS_KEY_HENKAN}},
    {93, {"KEY_KATAKANA_HIRAGANA", 93, 2607, HOS_KEY_KATAKANA_HIRAGANA}},
    {94, {"KEY_MUHENKAN", 94, 2608, HOS_KEY_MUHENKAN}},
    {101, {"KEY_LINEFEED", 101, 2609, HOS_KEY_LINEFEED}},
    {112, {"KEY_MACRO", 112, 2610, HOS_KEY_MACRO}},
    {118, {"KEY_NUMPAD_PLUSMINUS", 118, 2611, HOS_KEY_NUMPAD_PLUSMINUS}},
    {120, {"KEY_SCALE", 120, 2612, HOS_KEY_SCALE}},
    {122, {"KEY_HANGUEL", 122, 2613, HOS_KEY_HANGUEL}},
    {123, {"KEY_HANJA", 123, 2614, HOS_KEY_HANJA}},
    {124, {"KEY_YEN", 124, 2615, HOS_KEY_YEN}},
    {128, {"KEY_STOP", 128, 2616, HOS_KEY_STOP}},
    {129, {"KEY_AGAIN", 129, 2617, HOS_KEY_AGAIN}},
    {130, {"KEY_PROPS", 130, 2618, HOS_KEY_PROPS}},
    {131, {"KEY_UNDO", 131, 2619, HOS_KEY_UNDO}},
    {133, {"KEY_COPY", 133, 2620, HOS_KEY_COPY}},
    {134, {"KEY_OPEN", 134, 2621, HOS_KEY_OPEN}},
    {135, {"KEY_PASTE", 135, 2622, HOS_KEY_PASTE}},
    {136, {"KEY_FIND", 136, 2623, HOS_KEY_FIND}},
    {137, {"KEY_CUT", 137, 2624, HOS_KEY_CUT}},
    {138, {"KEY_HELP", 138, 2625, HOS_KEY_HELP}},
    {140, {"KEY_CALC", 140, 2626, HOS_KEY_CALC}},
    {144, {"KEY_FILE", 144, 2627, HOS_KEY_FILE}},
    {156, {"KEY_BOOKMARKS", 156, 2628, HOS_KEY_BOOKMARKS}},
    {163, {"KEY_NEXTSONG", 163, 12, HOS_KEY_MEDIA_NEXT}},
    {164, {"KEY_PLAYPAUSE", 164, 2630, HOS_KEY_PLAYPAUSE}},
    {165, {"KEY_PREVIOUSSONG", 165, 13, HOS_KEY_MEDIA_PREVIOUS}},
    {166, {"KEY_STOPCD", 166, 2632, HOS_KEY_STOPCD}},
    {169, {"KEY_CALL", 169, 3, HOS_KEY_CALL}},
    {171, {"KEY_CONFIG", 171, 2634, HOS_KEY_CONFIG}},
    {173, {"KEY_REFRESH", 173, 2635, HOS_KEY_REFRESH}},
    {174, {"KEY_EXIT", 174, 2636, HOS_KEY_EXIT}},
    {176, {"KEY_EDIT", 176, 2637, HOS_KEY_EDIT}},
    {177, {"KEY_SCROLLUP", 177, 2638, HOS_KEY_SCROLLUP}},
    {178, {"KEY_SCROLLDOWN", 178, 2639, HOS_KEY_SCROLLDOWN}},
    {181, {"KEY_NEW", 181, 2640, HOS_KEY_NEW}},
    {182, {"KEY_REDO", 182, 2641, HOS_KEY_REDO}},
    {206, {"KEY_CLOSE", 206, 2642, HOS_KEY_CLOSE}},
    {209, {"KEY_BASSBOOST", 209, 2644, HOS_KEY_BASSBOOST}},
    {210, {"KEY_PRINT", 210, 2645, HOS_KEY_PRINT}},
    {216, {"KEY_CHAT", 216, 2646, HOS_KEY_CHAT}},
    {219, {"KEY_FINANCE", 219, 2647, HOS_KEY_FINANCE}},
    {223, {"KEY_CANCEL", 223, 2648, HOS_KEY_CANCEL}},
    {228, {"KEY_KBDILLUM_TOGGLE", 228, 2649, HOS_KEY_KBDILLUM_TOGGLE}},
    {229, {"KEY_KBDILLUM_DOWN", 229, 2650, HOS_KEY_KBDILLUM_DOWN}},
    {230, {"KEY_KBDILLUM_UP", 230, 2651, HOS_KEY_KBDILLUM_UP}},
    {231, {"KEY_SEND", 231, 2652, HOS_KEY_SEND}},
    {232, {"KEY_REPLY", 232, 2653, HOS_KEY_REPLY}},
    {233, {"KEY_FORWARDMAIL", 233, 2654, HOS_KEY_FORWARDMAIL}},
    {234, {"KEY_SAVE", 234, 2655, HOS_KEY_SAVE}},
    {235, {"KEY_DOCUMENTS", 235, 2656, HOS_KEY_DOCUMENTS}},
    {241, {"KEY_VIDEO_NEXT", 241, 2657, HOS_KEY_VIDEO_NEXT}},
    {242, {"KEY_VIDEO_PREV", 242, 2658, HOS_KEY_VIDEO_PREV}},
    {243, {"KEY_BRIGHTNESS_CYCLE", 243, 2659, HOS_KEY_BRIGHTNESS_CYCLE}},
    {244, {"KEY_BRIGHTNESS_ZERO", 244, 2660, HOS_KEY_BRIGHTNESS_ZERO}},
    {245, {"KEY_DISPLAY_OFF", 245, 2661, HOS_KEY_DISPLAY_OFF}},
    {256, {"BTN_MISC", 256, 2662, HOS_BTN_MISC}},
    {354, {"KEY_GOTO", 354, 2663, HOS_KEY_GOTO}},
    {358, {"KEY_INFO", 358, 2664, HOS_KEY_INFO}},
    {362, {"KEY_PROGRAM", 362, 2665, HOS_KEY_PROGRAM}},
    {366, {"KEY_PVR", 366, 2666, HOS_KEY_PVR}},
    {370, {"KEY_SUBTITLE", 370, 2667, HOS_KEY_SUBTITLE}},
    {372, {"KEY_FULL_SCREEN", 372, 2668, HOS_KEY_FULL_SCREEN}},
    {374, {"KEY_KEYBOARD", 374, 2669, HOS_KEY_KEYBOARD}},
    {375, {"KEY_ASPECT_RATIO", 375, 2670, HOS_KEY_ASPECT_RATIO}},
    {376, {"KEY_PC", 376, 2671, HOS_KEY_PC}},
    {377, {"KEY_TV", 377, 2672, HOS_KEY_TV}},
    {378, {"KEY_TV2", 378, 2673, HOS_KEY_TV2}},
    {379, {"KEY_VCR", 379, 2674, HOS_KEY_VCR}},
    {380, {"KEY_VCR2", 380, 2675, HOS_KEY_VCR2}},
    {381, {"KEY_SAT", 381, 2676, HOS_KEY_SAT}},
    {383, {"KEY_CD", 383, 2677, HOS_KEY_CD}},
    {384, {"KEY_TAPE", 384, 2678, HOS_KEY_TAPE}},
    {386, {"KEY_TUNER", 386, 2679, HOS_KEY_TUNER}},
    {387, {"KEY_PLAYER", 387, 2680, HOS_KEY_PLAYER}},
    {389, {"KEY_DVD", 389, 2681, HOS_KEY_DVD}},
    {392, {"KEY_AUDIO", 392, 2682, HOS_KEY_AUDIO}},
    {393, {"KEY_VIDEO", 393, 2683, HOS_KEY_VIDEO}},
    {396, {"KEY_MEMO", 396, 2684, HOS_KEY_MEMO}},
    {397, {"KEY_CALENDAR", 397, 2685, HOS_KEY_CALENDAR}},
    {398, {"KEY_RED", 398, 2686, HOS_KEY_RED}},
    {399, {"KEY_GREEN", 399, 2687, HOS_KEY_GREEN}},
    {400, {"KEY_YELLOW", 400, 2688, HOS_KEY_YELLOW}},
    {401, {"KEY_BLUE", 401, 2689, HOS_KEY_BLUE}},
    {402, {"KEY_CHANNELUP", 402, 2690, HOS_KEY_CHANNELUP}},
    {403, {"KEY_CHANNELDOWN", 403, 2691, HOS_KEY_CHANNELDOWN}},
    {405, {"KEY_LAST", 405, 2692, HOS_KEY_LAST}},
    {408, {"KEY_RESTART", 408, 2693, HOS_KEY_RESTART}},
    {409, {"KEY_SLOW", 409, 2694, HOS_KEY_SLOW}},
    {410, {"KEY_SHUFFLE", 410, 2695, HOS_KEY_SHUFFLE}},
    {416, {"KEY_VIDEOPHONE", 416, 2696, HOS_KEY_VIDEOPHONE}},
    {417, {"KEY_GAMES", 417, 2697, HOS_KEY_GAMES}},
    {418, {"KEY_ZOOMIN", 418, 2698, HOS_KEY_ZOOMIN}},
    {419, {"KEY_ZOOMOUT", 419, 2699, HOS_KEY_ZOOMOUT}},
    {420, {"KEY_ZOOMRESET", 420, 2700, HOS_KEY_ZOOMRESET}},
    {421, {"KEY_WORDPROCESSOR", 421, 2701, HOS_KEY_WORDPROCESSOR}},
    {422, {"KEY_EDITOR", 422, 2702, HOS_KEY_EDITOR}},
    {423, {"KEY_SPREADSHEET", 423, 2703, HOS_KEY_SPREADSHEET}},
    {424, {"KEY_GRAPHICSEDITOR", 424, 2704, HOS_KEY_GRAPHICSEDITOR}},
    {425, {"KEY_PRESENTATION", 425, 2705, HOS_KEY_PRESENTATION}},
    {426, {"KEY_DATABASE", 426, 2706, HOS_KEY_DATABASE}},
    {427, {"KEY_NEWS", 427, 2707, HOS_KEY_NEWS}},
    {428, {"KEY_VOICEMAIL", 428, 2708, HOS_KEY_VOICEMAIL}},
    {429, {"KEY_ADDRESSBOOK", 429, 2709, HOS_KEY_ADDRESSBOOK}},
    {430, {"KEY_MESSENGER", 430, 2710, HOS_KEY_MESSENGER}},
    {431, {"KEY_BRIGHTNESS_TOGGLE", 431, 2711, HOS_KEY_BRIGHTNESS_TOGGLE}},
    {432, {"KEY_SPELLCHECK", 432, 2712, HOS_KEY_SPELLCHECK}},
    {433, {"KEY_COFFEE", 433, 2713, HOS_KEY_COFFEE}},
    {439, {"KEY_MEDIA_REPEAT", 439, 2714, HOS_KEY_MEDIA_REPEAT}},
    {442, {"KEY_IMAGES", 442, 2715, HOS_KEY_IMAGES}},
    {576, {"KEY_BUTTONCONFIG", 576, 2716, HOS_KEY_BUTTONCONFIG}},
    {577, {"KEY_TASKMANAGER", 577, 2717, HOS_KEY_TASKMANAGER}},
    {578, {"KEY_JOURNAL", 578, 2718, HOS_KEY_JOURNAL}},
    {579, {"KEY_CONTROLPANEL", 579, 2719, HOS_KEY_CONTROLPANEL}},
    {580, {"KEY_APPSELECT", 580, 2720, HOS_KEY_APPSELECT}},
    {581, {"KEY_SCREENSAVER", 581, 2721, HOS_KEY_SCREENSAVER}},
    {251, {"KEY_ASSISTANT", 251, 2722, HOS_KEY_ASSISTANT}},
    {584, {"KEY_KBD_LAYOUT_NEXT", 584, 2723, HOS_KEY_KBD_LAYOUT_NEXT}},
    {592, {"KEY_BRIGHTNESS_MIN", 592, 2724, HOS_KEY_BRIGHTNESS_MIN}},
    {593, {"KEY_BRIGHTNESS_MAX", 593, 2725, HOS_KEY_BRIGHTNESS_MAX}},
    {608, {"KEY_KBDINPUTASSIST_PREV", 608, 2726, HOS_KEY_KBDINPUTASSIST_PREV}},
    {609, {"KEY_KBDINPUTASSIST_NEXT", 609, 2727, HOS_KEY_KBDINPUTASSIST_NEXT}},
    {610, {"KEY_KBDINPUTASSIST_PREVGROUP", 610, 2728, HOS_KEY_KBDINPUTASSIST_PREVGROUP}},
    {611, {"KEY_KBDINPUTASSIST_NEXTGROUP", 611, 2729, HOS_KEY_KBDINPUTASSIST_NEXTGROUP}},
    {612, {"KEY_KBDINPUTASSIST_ACCEPT", 612, 2730, HOS_KEY_KBDINPUTASSIST_ACCEPT}},
    {613, {"KEY_KBDINPUTASSIST_CANCEL", 613, 2731, HOS_KEY_KBDINPUTASSIST_CANCEL}},

    {132, {"KEY_FRONT", 132, 2800, HOS_KEY_FRONT}},
    {141, {"KEY_SETUP", 141, 2801, HOS_KEY_SETUP}},
    {143, {"KEY_WAKEUP", 143, 2802, HOS_KEY_WAKEUP}},
    {145, {"KEY_SENDFILE", 145, 2803, HOS_KEY_SENDFILE}},
    {146, {"KEY_DELETEFILE", 146, 2804, HOS_KEY_DELETEFILE}},
    {147, {"KEY_XFER", 147, 2805, HOS_KEY_XFER}},
    {148, {"KEY_PROG1", 148, 2806, HOS_KEY_PROG1}},
    {149, {"KEY_PROG2", 149, 2807, HOS_KEY_PROG2}},
    {151, {"KEY_MSDOS", 151, 2808, HOS_KEY_MSDOS}},
    {152, {"KEY_SCREENLOCK", 152, 2809, HOS_KEY_SCREENLOCK}},
    {153, {"KEY_DIRECTION_ROTATE_DISPLAY", 153, 2810, HOS_KEY_DIRECTION_ROTATE_DISPLAY}},
    {154, {"KEY_CYCLEWINDOWS", 154, 2811, HOS_KEY_CYCLEWINDOWS}},
    {157, {"KEY_COMPUTER", 157, 2812, HOS_KEY_COMPUTER}},
    {162, {"KEY_EJECTCLOSECD", 162, 2813, HOS_KEY_EJECTCLOSECD}},
    {170, {"KEY_ISO", 170, 2814, HOS_KEY_ISO}},
    {175, {"KEY_MOVE", 175, 2815, HOS_KEY_MOVE}},
    {183, {"KEY_F13", 183, 2816, HOS_KEY_F13}},
    {184, {"KEY_F14", 184, 2817, HOS_KEY_F14}},
    {185, {"KEY_F15", 185, 2818, HOS_KEY_F15}},
    {186, {"KEY_F16", 186, 2819, HOS_KEY_F16}},
    {187, {"KEY_F17", 187, 2820, HOS_KEY_F17}},
    {188, {"KEY_F18", 188, 2821, HOS_KEY_F18}},
    {189, {"KEY_F19", 189, 2822, HOS_KEY_F19}},
    {190, {"KEY_F20", 190, 2823, HOS_KEY_F20}},
    {191, {"KEY_F21", 191, 2824, HOS_KEY_F21}},
    {192, {"KEY_F22", 192, 2825, HOS_KEY_F22}},
    {193, {"KEY_F23", 193, 2826, HOS_KEY_F23}},
    {194, {"KEY_F24", 194, 2827, HOS_KEY_F24}},
    {202, {"KEY_PROG3", 202, 2828, HOS_KEY_PROG3}},
    {203, {"KEY_PROG4", 203, 2829, HOS_KEY_PROG4}},
    {204, {"KEY_DASHBOARD", 204, 2830, HOS_KEY_DASHBOARD}},
    {205, {"KEY_SUSPEND", 205, 2831, HOS_KEY_SUSPEND}},
    {211, {"KEY_HP", 211, 2832, HOS_KEY_HP}},
    {249, {"KEY_SOUND", 249, 2833, HOS_KEY_SOUND}},
    {214, {"KEY_QUESTION", 214, 2834, HOS_KEY_QUESTION}},
    {215, {"KEY_AT", 215, 2065, HOS_KEY_AT}},
    {218, {"KEY_CONNECT", 218, 2836, HOS_KEY_CONNECT}},
    {220, {"KEY_SPORT", 220, 2837, HOS_KEY_SPORT}},
    {221, {"KEY_SHOP", 221, 2838, HOS_KEY_SHOP}},
    {222, {"KEY_ALTERASE", 222, 2839, HOS_KEY_ALTERASE}},
    {226, {"KEY_HEADSETHOOK", 226, 6, HOS_KEY_HEADSETHOOK}},
    {595, {"KEY_SWITCHVIDEOMODE", 595, 2841, HOS_KEY_SWITCHVIDEOMODE}},
    {236, {"KEY_BATTERY", 236, 2842, HOS_KEY_BATTERY}},
    {237, {"KEY_BLUETOOTH", 237, 2843, HOS_KEY_BLUETOOTH}},
    {594, {"KEY_WLAN", 594, 2844, HOS_KEY_WLAN}},
    {239, {"KEY_UWB", 239, 2845, HOS_KEY_UWB}},
    {246, {"KEY_WWAN_WIMAX", 246, 2846, HOS_KEY_WWAN_WIMAX}},
    {247, {"KEY_RFKILL", 247, 2847, HOS_KEY_RFKILL}},
    {248, {"KEY_MUTE", 248, 23, HOS_KEY_MUTE}},
    {196, {"KEY_F26", 196, 2848, HOS_KEY_F26}},
    {197, {"KEY_F27", 197, 2849, HOS_KEY_F27}},

    {363, {"KEY_CHANNEL", 363, 3001, HOS_KEY_CHANNEL}},
    {256, {"KEY_BTN_0", 256, 3100, HOS_KEY_BTN_0}},
    {257, {"KEY_BTN_1", 257, 3101, HOS_KEY_BTN_1}},
    {258, {"KEY_BTN_2", 258, 3102, HOS_KEY_BTN_2}},
    {259, {"KEY_BTN_3", 259, 3103, HOS_KEY_BTN_3}},
    {260, {"KEY_BTN_4", 260, 3104, HOS_KEY_BTN_4}},
    {261, {"KEY_BTN_5", 261, 3105, HOS_KEY_BTN_5}},
    {262, {"KEY_BTN_6", 262, 3106, HOS_KEY_BTN_6}},
    {263, {"KEY_BTN_7", 263, 3107, HOS_KEY_BTN_7}},
    {264, {"KEY_BTN_8", 264, 3108, HOS_KEY_BTN_8}},
    {265, {"KEY_BTN_9", 265, 3109, HOS_KEY_BTN_9}},

    {497, {"KEY_BRL_DOT1", 497, 3201, HOS_KEY_BRL_DOT1}},
    {498, {"KEY_BRL_DOT2", 498, 3202, HOS_KEY_BRL_DOT2}},
    {499, {"KEY_BRL_DOT3", 499, 3203, HOS_KEY_BRL_DOT3}},
    {500, {"KEY_BRL_DOT4", 500, 3204, HOS_KEY_BRL_DOT4}},
    {501, {"KEY_BRL_DOT5", 501, 3205, HOS_KEY_BRL_DOT5}},
    {502, {"KEY_BRL_DOT6", 502, 3206, HOS_KEY_BRL_DOT6}},
    {503, {"KEY_BRL_DOT7", 503, 3207, HOS_KEY_BRL_DOT7}},
    {504, {"KEY_BRL_DOT8", 504, 3208, HOS_KEY_BRL_DOT8}},
    {505, {"KEY_BRL_DOT9", 505, 3209, HOS_KEY_BRL_DOT9}},
    {506, {"KEY_BRL_DOT10", 506, 3210, HOS_KEY_BRL_DOT10}},
    {744, {"KEY_ENDCALL", 744, 4, HOS_KEY_ENDCALL}},
    {407, {"KEY_NEXT", 407, 2629, HOS_KEY_NEXT}},
    {412, {"KEY_PREVIOUS", 412, 2631, HOS_KEY_PREVIOUS}},
};

KeyEventValueTransformation TransferKeyValue(int32_t keyValueOfInput)
{
    auto it = MAP_KEY_EVENT_VALUE_TRANSFORMATION.find(keyValueOfInput);
    if (it == MAP_KEY_EVENT_VALUE_TRANSFORMATION.end()) {
        static constexpr int32_t unknownKeyBase = 10000;
        KeyEventValueTransformation unknownKey = {
            "UNKNOWN_KEY", keyValueOfInput, unknownKeyBase + keyValueOfInput, HOS_UNKNOWN_KEY_BASE
        };
        MMI_HILOGE("TransferKeyValue Failed, unknown linux-code:%{public}d,"
                   "UNKNOWN_KEY_BASE:%{public}d", keyValueOfInput, unknownKeyBase);
        return unknownKey;
    }
    return it->second;
}

int32_t InputTransformationKeyValue(int32_t keyCode)
{
    for (const auto &item : MAP_KEY_EVENT_VALUE_TRANSFORMATION) {
        if (item.second.sysKeyValue == keyCode) {
            return item.first;
        }
    }
    return INVALID_KEY_CODE;
}

namespace {
const std::map<int64_t, int32_t> MAP_KEY_INTENTION = {
    {(int64_t)KeyEvent::KEYCODE_DPAD_UP, KeyEvent::INTENTION_UP},
    {(int64_t)KeyEvent::KEYCODE_DPAD_DOWN, KeyEvent::INTENTION_DOWN},
    {(int64_t)KeyEvent::KEYCODE_DPAD_LEFT, KeyEvent::INTENTION_LEFT},
    {(int64_t)KeyEvent::KEYCODE_DPAD_RIGHT, KeyEvent::INTENTION_RIGHT},
    {(int64_t)KeyEvent::KEYCODE_SPACE, KeyEvent::INTENTION_SELECT},
    {(int64_t)KeyEvent::KEYCODE_NUMPAD_ENTER, KeyEvent::INTENTION_SELECT},
    {(int64_t)KeyEvent::KEYCODE_ESCAPE, KeyEvent::INTENTION_ESCAPE},
    {((int64_t)KeyEvent::KEYCODE_ALT_LEFT << 16) + KeyEvent::KEYCODE_DPAD_LEFT, KeyEvent::INTENTION_BACK},
    {((int64_t)KeyEvent::KEYCODE_ALT_LEFT << 16) + KeyEvent::KEYCODE_DPAD_RIGHT, KeyEvent::INTENTION_FORWARD},
    {((int64_t)KeyEvent::KEYCODE_ALT_RIGHT << 16) + KeyEvent::KEYCODE_DPAD_LEFT, KeyEvent::INTENTION_BACK},
    {((int64_t)KeyEvent::KEYCODE_ALT_RIGHT << 16) + KeyEvent::KEYCODE_DPAD_RIGHT, KeyEvent::INTENTION_FORWARD},
    {((int64_t)KeyEvent::KEYCODE_SHIFT_LEFT << 16) + KeyEvent::KEYCODE_F10, KeyEvent::INTENTION_MENU},
    {((int64_t)KeyEvent::KEYCODE_SHIFT_RIGHT << 16) + KeyEvent::KEYCODE_F10, KeyEvent::INTENTION_MENU},
    {(int64_t)KeyEvent::KEYCODE_COMPOSE, KeyEvent::INTENTION_MENU},
    {(int64_t)KeyEvent::KEYCODE_PAGE_UP, KeyEvent::INTENTION_PAGE_UP},
    {(int64_t)KeyEvent::KEYCODE_PAGE_DOWN, KeyEvent::INTENTION_PAGE_DOWN},
    {((int64_t)KeyEvent::KEYCODE_CTRL_LEFT << 16) + KeyEvent::KEYCODE_PLUS, KeyEvent::INTENTION_ZOOM_OUT},
    {((int64_t)KeyEvent::KEYCODE_CTRL_RIGHT << 16) + KeyEvent::KEYCODE_PLUS, KeyEvent::INTENTION_ZOOM_OUT},
    {((int64_t)KeyEvent::KEYCODE_CTRL_LEFT << 16) + KeyEvent::KEYCODE_NUMPAD_ADD, KeyEvent::INTENTION_ZOOM_OUT},
    {((int64_t)KeyEvent::KEYCODE_CTRL_RIGHT << 16) + KeyEvent::KEYCODE_NUMPAD_ADD, KeyEvent::INTENTION_ZOOM_OUT},
    {((int64_t)KeyEvent::KEYCODE_CTRL_LEFT << 16) + KeyEvent::KEYCODE_MINUS, KeyEvent::INTENTION_ZOOM_IN},
    {((int64_t)KeyEvent::KEYCODE_CTRL_RIGHT << 16) + KeyEvent::KEYCODE_MINUS, KeyEvent::INTENTION_ZOOM_IN},
    {((int64_t)KeyEvent::KEYCODE_CTRL_LEFT << 16) + KeyEvent::KEYCODE_NUMPAD_SUBTRACT, KeyEvent::INTENTION_ZOOM_IN},
    {((int64_t)KeyEvent::KEYCODE_CTRL_RIGHT << 16) + KeyEvent::KEYCODE_NUMPAD_SUBTRACT, KeyEvent::INTENTION_ZOOM_IN},
    {(int64_t)KeyEvent::KEYCODE_VOLUME_MUTE, KeyEvent::INTENTION_MEDIA_MUTE},
    {(int64_t)KeyEvent::KEYCODE_MUTE, KeyEvent::INTENTION_MEDIA_MUTE},
    {(int64_t)KeyEvent::KEYCODE_VOLUME_UP, KeyEvent::INTENTION_VOLUTE_UP},
    {(int64_t)KeyEvent::KEYCODE_VOLUME_DOWN, KeyEvent::INTENTION_VOLUTE_DOWN},
    {(int64_t)KeyEvent::KEYCODE_APPSELECT, KeyEvent::INTENTION_SELECT},
    {(int64_t)KeyEvent::KEYCODE_BACK, KeyEvent::INTENTION_BACK},
    {(int64_t)KeyEvent::KEYCODE_MOVE_HOME, KeyEvent::INTENTION_HOME},
};
} // namespace

int32_t KeyItemsTransKeyIntention(const std::vector<KeyEvent::KeyItem> &items)
{
    if (items.size() < MIN_KEY_SIZE || items.size() > MAX_KEY_SIZE) {
        return KeyEvent::INTENTION_UNKNOWN;
    }

    int64_t keyCodes = 0;
    for (const auto &item : items) {
        keyCodes = static_cast<int64_t>(
            (static_cast<uint64_t>(keyCodes) << BIT_SET_INDEX) + (static_cast<uint64_t>(item.GetKeyCode())));
    }
    auto iter = MAP_KEY_INTENTION.find(keyCodes);
    if (iter == MAP_KEY_INTENTION.end()) {
        return KeyEvent::INTENTION_UNKNOWN;
    }
    return iter->second;
}
} // namespace MMI
} // namespace OHOS