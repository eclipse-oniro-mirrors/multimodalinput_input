/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include <cstdio>
#include <fstream>
#include <gmock/gmock.h>

#include "account_manager.h"
#include "cursor_drawing_component.h"
#include "event_filter_handler.h"
#include "fingersense_wrapper.h"
#include "i_pointer_drawing_manager.h"
#include "input_device_manager.h"
#include "input_event_handler.h"
#include "input_windows_manager.h"
#include "mmi_log.h"
#include "mock_input_windows_manager.h"
#include "pointer_drawing_manager.h"
#include "proto.h"
#include "scene_board_judgement.h"
#include "struct_multimodal.h"
#include "uds_server.h"
#include "old_display_info.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "InputWindowsManagerTest"

namespace OHOS {
namespace MMI {
using namespace testing;
using namespace testing::ext;
namespace {
InputWindowsManager *g_instance;
constexpr uint32_t DEFAULT_ICON_COLOR {0xFF};
constexpr int32_t MAX_PIXEL_MAP_WIDTH {600};
constexpr int32_t MAX_PIXEL_MAP_HEIGHT {600};
constexpr int32_t INT32_BYTE {4};
constexpr int32_t NUMBER_TWO {2};
constexpr double HALF_RATIO { 0.5 };
const int32_t ROTATE_POLICY = system::GetIntParameter("const.window.device.rotate_policy", 0);
constexpr int32_t WINDOW_ROTATE { 0 };
#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
constexpr uint32_t WINDOW_NAME_TYPE_SCREENSHOT {1};
constexpr float SCREEN_CAPTURE_WINDOW_ZORDER {8000.0};
constexpr uint32_t CAST_WINDOW_TYPE {2106};
constexpr uint32_t GUIDE_WINDOW_TYPE {2500};
constexpr uint32_t TEST_WINDOW_START {-100};
constexpr uint32_t TEST_WINDOW_END {100000};
#define SCREEN_RECORD_WINDOW_WIDTH  400
#define SCREEN_RECORD_WINDOW_HEIGHT 200
#endif // OHOS_BUILD_ENABLE_VKEYBOARD
} // namespace

#ifdef WIN_MGR
#undef WIN_MGR
#endif
#define WIN_MGR g_instance
#define NUM_100 100
#define NUM_200 200
#define NUM_1 1
#define NUM_2 2
#define NA_GROUP_ID 1000

class InputWindowsManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    static OLD::DisplayGroupInfo CreateDisplayGroupInfo(int groupId, int displayId, int displayId2 = 0)
    {
        OLD::DisplayGroupInfo displayGroupInfo;
        displayGroupInfo.groupId = groupId;
        displayGroupInfo.type = GroupType::GROUP_SPECIAL;
        std::vector<OLD::DisplayInfo> displaysInfo;
        OLD::DisplayInfo display = { .id = displayId };
        displaysInfo.emplace_back(display);
        if (displayId2 != 0) {
            OLD::DisplayInfo display2 = { .id = displayId2 };
            displaysInfo.emplace_back(display2);
        }
        displayGroupInfo.displaysInfo = displaysInfo;
        return displayGroupInfo;
    }
    static std::shared_ptr<Media::PixelMap> CreatePixelMap(int32_t width, int32_t height);
    static SessionPtr CreateSessionPtr();
    static SessionPtr CreateCameraSessionPtr();
    void SetUp(void)
    {
        // 创建displayGroupInfo_
        OLD::DisplayGroupInfo displayGroupInfo;
        displayGroupInfo.focusWindowId = 1;
        uint32_t num = 1;
        for (uint32_t i = 0; i < num; i++) {
            WindowInfo info;
            info.id = 1;
            info.pid = 1;
            info.uid = 1;
            info.area = {1, 1, 1, 1};
            info.defaultHotAreas = {info.area};
            info.pointerHotAreas = {info.area};
            info.agentWindowId = 1;
            info.flags = 1;
            info.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
            info.pointerChangeAreas = {1, 2, 1, 2, 1, 2, 1, 2, 1};
            displayGroupInfo.windowsInfo.push_back(info);
        }
        for (uint32_t i = 0; i < num; i++) {
            OLD::DisplayInfo info;
            info.id = 1;
            info.x = 1;
            info.y = 1;
            info.width = NUMBER_TWO;
            info.height = NUMBER_TWO;
            info.dpi = 240;
            info.name = "pp";
            info.uniq = "pp";
            info.direction = DIRECTION0;
            displayGroupInfo.displaysInfo.push_back(info);
        }
        WIN_MGR->UpdateDisplayInfo(displayGroupInfo);
        preHoverScrollState_ = WIN_MGR->GetHoverScrollState();
    } // void SetUp(void)

    void TearDown(void)
    {
        AccountManager::GetInstance()->AccountManagerUnregister();
        WIN_MGR->SetHoverScrollState(preHoverScrollState_);
    }

private:
    bool preHoverScrollState_ {true};
};

void InputWindowsManagerTest::SetUpTestCase(void)
{
    g_instance = static_cast<InputWindowsManager *>(IInputWindowsManager::GetInstance().get());
}

void FingersenseWrapperTest(int32_t num) {}

std::shared_ptr<Media::PixelMap> InputWindowsManagerTest::CreatePixelMap(int32_t width, int32_t height)
{
    CALL_DEBUG_ENTER;
    if (width <= 0 || width > MAX_PIXEL_MAP_WIDTH || height <= 0 || height > MAX_PIXEL_MAP_HEIGHT) {
        return nullptr;
    }
    Media::InitializationOptions opts;
    opts.size.height = height;
    opts.size.width = width;
    opts.pixelFormat = Media::PixelFormat::BGRA_8888;
    opts.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    opts.scaleMode = Media::ScaleMode::FIT_TARGET_SIZE;

    int32_t colorLen = width * height;
    uint32_t *pixelColors = new (std::nothrow) uint32_t[colorLen];
    CHKPP(pixelColors);
    int32_t colorByteCount = colorLen * INT32_BYTE;
    errno_t ret = memset_s(pixelColors, colorByteCount, DEFAULT_ICON_COLOR, colorByteCount);
    if (ret != EOK) {
        delete[] pixelColors;
        return nullptr;
    }
    std::shared_ptr<Media::PixelMap> pixelMap = Media::PixelMap::Create(pixelColors, colorLen, opts);
    if (pixelMap == nullptr) {
        delete[] pixelColors;
        return nullptr;
    }
    delete[] pixelColors;
    return pixelMap;
}

SessionPtr InputWindowsManagerTest::CreateSessionPtr()
{
    CALL_DEBUG_ENTER;
    std::string programName = "uds_sesion_test";
    int32_t moduleType = 3;
    int32_t fd = -1;
    int32_t uidRoot = 0;
    int32_t pid = 9;
    return std::make_shared<UDSSession>(programName, moduleType, fd, uidRoot, pid);
}

SessionPtr InputWindowsManagerTest::CreateCameraSessionPtr()
{
    CALL_DEBUG_ENTER;
    std::string programName = "uds_sesion_test.camera";
    int32_t moduleType = 3;
        int32_t fd = -1;
    int32_t uidRoot = 0;
    int32_t pid = 9;
    return std::make_shared<UDSSession>(programName, moduleType, fd, uidRoot, pid);
}

/**
 * @tc.name: InputWindowsManagerTest_GetClientFd_001
 * @tc.desc: Test GetClientFd
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetClientFd_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    int32_t idNames = -1;
    ASSERT_EQ(WIN_MGR->GetClientFd(pointerEvent), idNames);
}

/**
 * @tc.name: InputWindowsManagerTest_GetDisplayGroupInfo_001
 * @tc.desc: Test GetDisplayGroupInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetDisplayGroupInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;
    int32_t groupId = 1;
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->GetDisplayGroupInfo(groupId));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTarget_003
 * @tc.desc: Test UpdateTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTarget_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetDeviceId(1);
    keyEvent->SetTargetWindowId(1);
    keyEvent->SetAgentWindowId(1);
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->UpdateTarget(keyEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_HandleKeyEventWindowId_003
 * @tc.desc: Test HandleKeyEventWindowId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandleKeyEventWindowId_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetDeviceId(1);
    keyEvent->SetTargetWindowId(1);
    keyEvent->SetAgentWindowId(1);
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->HandleKeyEventWindowId(keyEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateWindow_002
 * @tc.desc: Test UpdateWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateWindow_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo window;
    window.id = 11;
    window.pid = 1221;
    window.uid = 1;
    window.area = {1, 1, 1, 1};
    window.defaultHotAreas = {window.area};
    window.pointerHotAreas = {window.area};
    window.pointerChangeAreas = {1, 2, 1, 2};
    window.displayId = 0;
    window.agentWindowId = 1;
    window.flags = 1;
    window.action = WINDOW_UPDATE_ACTION::UNKNOWN;
    WIN_MGR->UpdateWindowInfo({0, 11, {window}});
    ASSERT_EQ(WIN_MGR->GetWindowPid(11), -1);
    window.groupId = 2;
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->GetWindowInfoVector(2));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTargetPointer_005
 * @tc.desc: Test UpdateTargetPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTargetPointer_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto pointerEvent = PointerEvent::Create();
    ASSERT_EQ(WIN_MGR->UpdateTargetPointer(pointerEvent), -1);
    WIN_MGR->IsFoldable_ = true;
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    PointerEvent::PointerItem pointerItem;
    pointerEvent->UpdatePointerItem(2, pointerItem);
    WIN_MGR->cancelTouchStatus_ = true;
    int32_t longAxis = 1U << 27U;
    pointerItem.SetLongAxis(longAxis);
    ASSERT_EQ(WIN_MGR->UpdateTargetPointer(pointerEvent), RET_ERR);
}

#ifdef OHOS_BUILD_ENABLE_POINTER
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_006
 * @tc.desc: Test IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    if (Rosen::SceneBoardJudgement::IsSceneBoardEnabled()) {
        ASSERT_EQ(WIN_MGR->IsNeedRefreshLayer(-2), true);
        ASSERT_EQ(WIN_MGR->IsNeedRefreshLayer(-1), true);
        ASSERT_EQ(WIN_MGR->IsNeedRefreshLayer(0), true);
        ASSERT_EQ(WIN_MGR->IsNeedRefreshLayer(1), true);
        ASSERT_EQ(WIN_MGR->IsNeedRefreshLayer(-2), true);
    } else {
        ASSERT_EQ(WIN_MGR->IsNeedRefreshLayer(-2), false);
        ASSERT_EQ(WIN_MGR->IsNeedRefreshLayer(-1), false);
        ASSERT_EQ(WIN_MGR->IsNeedRefreshLayer(0), false);
        ASSERT_EQ(WIN_MGR->IsNeedRefreshLayer(1), false);
        ASSERT_EQ(WIN_MGR->IsNeedRefreshLayer(-2), false);
    }
}

#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING
#endif //OHOS_BUILD_ENABLE_POINTER

/**
 * @tc.name: InputWindowsManagerTest_SetMouseCaptureMode_008
 * @tc.desc: Test SetMouseCaptureMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetMouseCaptureMode_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    bool isCaptureMode = false;
    ASSERT_EQ(WIN_MGR->SetMouseCaptureMode(-1, isCaptureMode), -1);
    ASSERT_EQ(WIN_MGR->SetMouseCaptureMode(1, isCaptureMode), 0);
    isCaptureMode = true;
    ASSERT_EQ(WIN_MGR->SetMouseCaptureMode(1, isCaptureMode), 0);
}

/**
 * @tc.name: InputWindowsManagerTest_DeviceStatusChanged_002
 * @tc.desc: Test DeviceStatusChanged
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DeviceStatusChanged_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    std::string name = "mouse";
    std::string sysUid = "james";
    std::string devStatus = "add";
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->DeviceStatusChanged(2, name, sysUid, devStatus));
    devStatus = "remove";
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->DeviceStatusChanged(2, name, sysUid, devStatus));
}

/**
 * @tc.name: InputWindowsManagerTest_SetDisplayBind_009
 * @tc.desc: Test SetDisplayBind
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetDisplayBind_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    std::string msg = "There is in InputWindowsManagerTest_GetDisplayIdNames_009";
    ASSERT_EQ(WIN_MGR->SetDisplayBind(-1, 1, msg), -1);
}

/**
 * @tc.name: InputWindowsManagerTest_SetDisplayBind_010
 * @tc.desc: Test SetDisplayBind
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetDisplayBind_010, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    std::string msg = "There is in InputWindowsManagerTest_GetDisplayIdNames_009";
    //std::shared_ptr<BindInfos> infos_ = std::make_shared<BindInfos>();
    int32_t deviceId = 2;
    int32_t displayId = 3;
    BindInfo infos_;
    infos_.inputDeviceId_ = 3;
    infos_.inputNodeName_ = 4;
    infos_.inputDeviceName_ = 5;
    infos_.displayId_ = 6;
    infos_.displayName_= "abc";
    ASSERT_EQ(WIN_MGR->SetDisplayBind(deviceId, displayId, msg), RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetHoverScrollState_010
 * @tc.desc: Test SetHoverScrollState
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetHoverScrollState_010, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(WIN_MGR->SetHoverScrollState(false) == RET_OK);
    WIN_MGR->SetHoverScrollState(true);
    ASSERT_TRUE(WIN_MGR->GetHoverScrollState());
}

/**
 * @tc.name: InputWindowsManagerTest_GetHoverScrollState_011
 * @tc.desc: Test GetHoverScrollState
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetHoverScrollState_011, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WIN_MGR->SetHoverScrollState(true);
    ASSERT_TRUE(WIN_MGR->GetHoverScrollState());
}

/**
 * @tc.name: InputWindowsManagerTest_InitMouseDownInfo_001
 * @tc.desc: Test initializing mouse down information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InitMouseDownInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WIN_MGR->InitMouseDownInfo();
    EXPECT_EQ(WIN_MGR->mouseDownInfo_.id, -1);
    EXPECT_EQ(WIN_MGR->mouseDownInfo_.pid, -1);
    EXPECT_TRUE(WIN_MGR->mouseDownInfo_.defaultHotAreas.empty());
    EXPECT_TRUE(WIN_MGR->mouseDownInfo_.pointerHotAreas.empty());
}

/**
 * @tc.name: InputWindowsManagerTest_InitMouseDownInfo_002
 * @tc.desc: Test initializing mouse down information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InitMouseDownInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WIN_MGR->mouseDownInfo_.id = 1;
    WIN_MGR->mouseDownInfo_.pid = 123;
    WIN_MGR->mouseDownInfo_.defaultHotAreas.push_back({0, 0, 100, 100});
    WIN_MGR->InitMouseDownInfo();
    EXPECT_EQ(WIN_MGR->mouseDownInfo_.id, -1);
    EXPECT_EQ(WIN_MGR->mouseDownInfo_.pid, -1);
    EXPECT_TRUE(WIN_MGR->mouseDownInfo_.defaultHotAreas.empty());
    EXPECT_TRUE(WIN_MGR->mouseDownInfo_.pointerHotAreas.empty());
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowGroupInfoByDisplayId_001
 * @tc.desc: Test getting window group information by display ID
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowGroupInfoByDisplayId_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t displayId = -1;
    const std::vector<WindowInfo> &windowGroupInfo = WIN_MGR->GetWindowGroupInfoByDisplayId(displayId);
    EXPECT_EQ(windowGroupInfo.size(), 1);
}

/**
 * @tc.name: InputWindowsManagerTest_GetDisplayId_001
 * @tc.desc: Test getting the display ID
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetDisplayId_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t expectedDisplayId = 1;
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    EXPECT_NE(inputEvent, nullptr);
    inputEvent->SetTargetDisplayId(expectedDisplayId);
    int32_t ret = WIN_MGR->GetDisplayId(inputEvent);
    EXPECT_EQ(ret, expectedDisplayId);
}

/**
 * @tc.name: InputWindowsManagerTest_GetPidAndUpdateTarget_001
 * @tc.desc: Test getting PID and updating the target
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPidAndUpdateTarget_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    EXPECT_NE(keyEvent, nullptr);
    int32_t targetDisplayId = 0;
    keyEvent->SetTargetDisplayId(targetDisplayId);
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->GetPidAndUpdateTarget(keyEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowPid_001
 * @tc.desc: Test getting the process ID of a window
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowPid_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t windowId = 100;
    std::vector<WindowInfo> windowsInfo;
    int32_t ret = WIN_MGR->GetWindowPid(windowId, windowsInfo);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowPid_002
 * @tc.desc: Test getting the process ID of a window
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowPid_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t windowId = 5;
    std::vector<WindowInfo> windowsInfo(3);

    windowsInfo[0].id = 5;
    windowsInfo[0].pid = 6;

    windowsInfo[1].id = 12;
    windowsInfo[1].pid = 13;

    windowsInfo[2].id = 123;
    windowsInfo[2].pid = 124;
    int32_t ret = WIN_MGR->GetWindowPid(windowId,  windowsInfo);
    EXPECT_EQ(ret, 6);
}

/**
 * @tc.name: InputWindowsManagerTest_CheckFocusWindowChange_001
 * @tc.desc: Test checking focus window changes
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckFocusWindowChange_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 123;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->CheckFocusWindowChange(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_CheckFocusWindowChange_002
 * @tc.desc: Test checking focus window changes
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckFocusWindowChange_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    OLD::DisplayGroupInfo displayGroupInfo_;
    displayGroupInfo.focusWindowId = 123;
    displayGroupInfo_.focusWindowId = 456;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->CheckFocusWindowChange(displayGroupInfo));
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->CheckFocusWindowChange(displayGroupInfo_));
}

/**
 * @tc.name: InputWindowsManagerTest_CheckZorderWindowChange_001
 * @tc.desc: Test checking Z-order window changes
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckZorderWindowChange_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::vector<WindowInfo> oldWindowsInfo = {{1}};
    std::vector<WindowInfo> newWindowsInfo = {{2}};
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->CheckZorderWindowChange(oldWindowsInfo, newWindowsInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayIdAndName_001
 * @tc.desc: Test updating display ID and name
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayIdAndName_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayIdAndName());
    assert(WIN_MGR->GetDisplayIdNames().size() == 2);
    assert(WIN_MGR->IsDisplayAdd(1, "A"));
    assert(WIN_MGR->IsDisplayAdd(2, "B"));
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayIdAndName());
    assert(WIN_MGR->GetDisplayIdNames().size() == 2);
    assert(WIN_MGR->IsDisplayAdd(1, "A"));
    assert(WIN_MGR->IsDisplayAdd(3, "C"));
    assert(!WIN_MGR->IsDisplayAdd(2, "B"));
}

/**
 * @tc.name: InputWindowsManagerTest_GetAllUsersDisplays_001
 * @tc.desc: Test GetAllUsersDisplays
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetAllUsersDisplays_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OHOS::MMI::WindowInfo win1 = {.id = NUM_1, .pid = NUM_100};
    OHOS::MMI::WindowInfo win2 = {.id = NUM_2, .pid = NUM_200};
    std::vector<OHOS::MMI::WindowInfo> windowsInfo1 = { win1, win2 };
    std::vector<OHOS::MMI::WindowInfo> windowsInfo2 = { win2 };
    OLD::DisplayGroupInfo group1 = InputWindowsManagerTest::CreateDisplayGroupInfo(NUM_100, NUM_100, NUM_100 + NUM_1);
    group1.windowsInfo = windowsInfo1;
    OLD::DisplayGroupInfo group2 = InputWindowsManagerTest::CreateDisplayGroupInfo(NUM_200, NUM_200);
    group2.windowsInfo = windowsInfo2;

    WIN_MGR->UpdateDisplayInfo(group1);
    WIN_MGR->UpdateDisplayInfo(group2);

    std::vector<OLD::DisplayInfo> displays = WIN_MGR->GetAllUsersDisplays();
    EXPECT_EQ(displays.size(), NUM_1 + NUM_2);
}

/**
 * @tc.name: InputWindowsManagerTest_GetDisplayBindInfo_001
 * @tc.desc: Test getting display binding information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetDisplayBindInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t deviceId = 1;
    int32_t displayId = 2;
    DisplayBindInfos infos;
    std::string msg;
    int32_t ret = WIN_MGR->SetDisplayBind(deviceId, displayId, msg);
    EXPECT_EQ(ret, -1);
    ret = WIN_MGR->GetDisplayBindInfo(infos);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateCaptureMode_001
 * @tc.desc: Test updating capture mode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateCaptureMode_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 123;
    WIN_MGR->UpdateCaptureMode(displayGroupInfo);
    EXPECT_FALSE(WIN_MGR->captureModeInfo_.isCaptureMode);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfoByIncrementalInfo_001
 * @tc.desc: Test updating display information by incremental info
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfoByIncrementalInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;
    WindowInfo window;
    WIN_MGR->UpdateDisplayInfoByIncrementalInfo(window, displayGroupInfo);
    EXPECT_EQ(displayGroupInfo.windowsInfo.size(), 0);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateWindowsInfoPerDisplay_001
 * @tc.desc: Test updating window information for each display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateWindowsInfoPerDisplay_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 2;
    WIN_MGR->UpdateWindowsInfoPerDisplay(displayGroupInfo);
    WindowInfo window1 {1};
    WindowInfo window2 {2};
    displayGroupInfo.windowsInfo.push_back(window1);
    displayGroupInfo.windowsInfo.push_back(window2);
    WIN_MGR->UpdateDisplayInfo(displayGroupInfo);
    ASSERT_EQ(displayGroupInfo.windowsInfo.size(), 2);
    ASSERT_EQ(displayGroupInfo.windowsInfo[0].zOrder, 0);
    ASSERT_EQ(displayGroupInfo.windowsInfo[1].zOrder, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfo_001
 * @tc.desc: Test updating display information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    WindowInfo windowInfo1;
    windowInfo1.zOrder = 1;
    windowInfo1.action = WINDOW_UPDATE_ACTION::ADD_END;
    WindowInfo windowInfo2;
    windowInfo2.zOrder = 2;
    windowInfo2.action = WINDOW_UPDATE_ACTION::ADD_END;
    displayGroupInfo.windowsInfo.push_back(windowInfo1);
    displayGroupInfo.windowsInfo.push_back(windowInfo2);
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayInfo(displayGroupInfo));
}

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)

/**
 * @tc.name: InputWindowsManagerTest_NeedUpdatePointDrawFlag_001
 * @tc.desc: Test whether the point draw flag needs to be updated
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_NeedUpdatePointDrawFlag_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::vector<WindowInfo> windows1;
    EXPECT_FALSE(WIN_MGR->NeedUpdatePointDrawFlag(windows1));
    std::vector<WindowInfo> windows2;
    windows2.push_back(WindowInfo());
    windows2.back().action = OHOS::MMI::WINDOW_UPDATE_ACTION::ADD;
    EXPECT_FALSE(WIN_MGR->NeedUpdatePointDrawFlag(windows2));
    std::vector<WindowInfo> windows3;
    windows3.push_back(WindowInfo());
    windows3.back().action = OHOS::MMI::WINDOW_UPDATE_ACTION::ADD_END;
    EXPECT_TRUE(WIN_MGR->NeedUpdatePointDrawFlag(windows3));
}

#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING

/**
 * @tc.name: InputWindowsManagerTest_DispatchPointer_001
 * @tc.desc: Test dispatching pointer events
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DispatchPointer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_ENTER_WINDOW;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->DispatchPointer(pointerAction));
    pointerAction = PointerEvent::POINTER_ACTION_LEAVE_WINDOW;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->DispatchPointer(pointerAction));
    pointerAction = PointerEvent::POINTER_ACTION_MOVE;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->DispatchPointer(pointerAction));
}

/**
 * @tc.name: InputWindowsManagerTest_NotifyPointerToWindow_001
 * @tc.desc: Test notifying pointer events to window
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_NotifyPointerToWindow_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = nullptr;
    inputWindowsManager.NotifyPointerToWindow();
    EXPECT_EQ(inputWindowsManager.lastWindowInfo_.id, -1);
}

/**
 * @tc.name: InputWindowsManagerTest_PrintWindowInfo_001
 * @tc.desc: Test printing window information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintWindowInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo1;
    windowInfo1.id = 1;
    windowInfo1.pid = 100;
    windowInfo1.uid = 200;
    windowInfo1.area = {0, 0, 800, 600};
    windowInfo1.defaultHotAreas = {
        {10,  10,  100, 100},
        {200, 200, 50,  50 }
    };
    windowInfo1.pointerHotAreas = {
        {30,  30,  150, 150},
        {400, 400, 70,  70 }
    };
    windowInfo1.agentWindowId = 10;
    windowInfo1.flags = 1;
    windowInfo1.displayId = 3;
    windowInfo1.zOrder = 4.0f;
    windowInfo1.pointerChangeAreas = {10, 20, 30};
    windowInfo1.transform = {1.0f, 2.0f, 3.0f};
    WindowInfo windowInfo2;
    windowInfo2.id = 2;
    windowInfo2.pid = 101;
    windowInfo2.uid = 201;
    windowInfo2.area = {800, 600, 1024, 768};
    windowInfo2.defaultHotAreas = {
        {50,  50,  200, 200},
        {600, 600, 100, 100}
    };
    windowInfo2.pointerHotAreas = {
        {70,  70,  250, 250},
        {800, 800, 120, 120}
    };
    windowInfo2.agentWindowId = 20;
    windowInfo2.flags = 2;
    windowInfo2.displayId = 4;
    windowInfo2.zOrder = 5.0f;
    windowInfo2.pointerChangeAreas = {40, 50, 60};
    windowInfo2.transform = {4.0f, 5.0f, 6.0f};
    std::vector<WindowInfo> windowsInfo = {windowInfo1, windowInfo2};
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->PrintWindowInfo(windowsInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_PrintWindowGroupInfo_001
 * @tc.desc: Test printing window group information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintWindowGroupInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowGroupInfo testData;
    testData.focusWindowId = 1;
    testData.displayId = 2;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->PrintWindowGroupInfo(testData));
}

/**
 * @tc.name: InputWindowsManagerTest_PrintDisplayInfo_001
 * @tc.desc: Test printing display information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintDisplayInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    auto it = manager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != manager.displayGroupInfoMap_.end()) {
        it->second.focusWindowId = 1;
        it->second.windowsInfo.push_back(WindowInfo());
        it->second.displaysInfo.push_back(OLD::DisplayInfo());
    }
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->PrintDisplayGroupInfo(it->second));
}

/**
 * @tc.name: InputWindowsManagerTest_GetPhysicalDisplay_001
 * @tc.desc: Test getting physical display information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPhysicalDisplay_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t id = 1;
    const OLD::DisplayInfo *displayInfo = WIN_MGR->GetPhysicalDisplay(id);
    EXPECT_NE(displayInfo, nullptr);
    EXPECT_EQ(displayInfo->id, id);
}

/**
 * @tc.name: InputWindowsManagerTest_GetPhysicalDisplay_002
 * @tc.desc: Test getting physical display information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPhysicalDisplay_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t id = -1;
    const OLD::DisplayInfo *displayInfo = WIN_MGR->GetPhysicalDisplay(id);
    EXPECT_EQ(displayInfo, nullptr);
}

/**
 * @tc.name: InputWindowsManagerTest_FindPhysicalDisplayInfo_001
 * @tc.desc: Test finding physical display information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FindPhysicalDisplayInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    ASSERT_EQ(manager.FindPhysicalDisplayInfo("test"), nullptr);
    OLD::DisplayInfo info1;
    info1.id = 123;
    auto it = manager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != manager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(info1);
    }
    ASSERT_NE(manager.FindPhysicalDisplayInfo("test"), nullptr);
    OLD::DisplayInfo info2;
    info2.id = 456;
    it->second.displaysInfo.push_back(info2);
    ASSERT_NE(manager.FindPhysicalDisplayInfo("test"), nullptr);
    ASSERT_NE(manager.FindPhysicalDisplayInfo("not_matching"), nullptr);
    ASSERT_NE(manager.FindPhysicalDisplayInfo("nonexistent"), nullptr);
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_001
 * @tc.desc: Test rotating the screen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo info;
    PhysicalCoordinate coord;
    info.direction = DIRECTION0;
    coord.x = 10;
    coord.y = 20;
    WIN_MGR->RotateScreen(info, coord);
    EXPECT_NE(coord.x, 100);
    EXPECT_NE(coord.y, 200);
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_002
 * @tc.desc: Test rotating the screen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo info;
    PhysicalCoordinate coord;
    info.direction = DIRECTION90;
    info.width = 800;
    info.height = 600;
    info.validWidth = info.width;
    info.validHeight = info.height;
    coord.x = 10;
    coord.y = 20;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_003
 * @tc.desc: Test rotating the screen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo info;
    PhysicalCoordinate coord;
    info.direction = DIRECTION180;
    info.width = 800;
    info.height = 600;
    info.validWidth = info.width;
    info.validHeight = info.height;
    coord.x = 10;
    coord.y = 20;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_004
 * @tc.desc: Test rotating the screen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo info;
    PhysicalCoordinate coord;
    info.direction = DIRECTION270;
    info.width = 800;
    info.height = 600;
    info.validWidth = info.width;
    info.validHeight = info.height;
    coord.x = 10;
    coord.y = 20;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->RotateScreen(info, coord));
}

#ifdef OHOS_BUILD_ENABLE_POINTER
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_001
 * @tc.desc: Test whether layer refresh is needed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_FALSE(WIN_MGR->IsNeedRefreshLayer(1));
    WIN_MGR->GetWindowInfo(0, 0)->id = 2;
    EXPECT_FALSE(WIN_MGR->IsNeedRefreshLayer(GLOBAL_WINDOW_ID));
    WIN_MGR->GetWindowInfo(0, 0)->id = 3;
    EXPECT_FALSE(WIN_MGR->IsNeedRefreshLayer(1));
}

#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING
#endif //OHOS_BUILD_ENABLE_POINTER

/**
 * @tc.name: InputWindowsManagerTest_OnSessionLost_001
 * @tc.desc: Test handling when session is lost
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_OnSessionLost_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SessionPtr session = std::shared_ptr<UDSSession>();
    WIN_MGR->OnSessionLost(session);
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->GetDisplayGroupInfo());
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePoinerStyle_001
 * @tc.desc: Test updating pointer style
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePoinerStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t pid = 1;
    int32_t windowId = 2;
    PointerStyle pointerStyle;
    int32_t ret = WIN_MGR->UpdatePoinerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, RET_OK);
    pid = -1;
    windowId = -2;
    ret = WIN_MGR->UpdatePoinerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, 401);
    pid = 1;
    windowId = -2;
    ret = WIN_MGR->UpdatePoinerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateSceneBoardPointerStyle_001
 * @tc.desc: Test updating scene board pointer style
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateSceneBoardPointerStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t pid = 1;
    int32_t windowId = 2;
    PointerStyle pointerStyle;
    pointerStyle.id = 3;
    int32_t ret = WIN_MGR->UpdateSceneBoardPointerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, RET_OK);
    pid = -1;
    windowId = -2;
    ret = WIN_MGR->UpdateSceneBoardPointerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_SetPointerStyle_001
 * @tc.desc: Test setting pointer style
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetPointerStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t pid = 1;
    int32_t windowId = GLOBAL_WINDOW_ID;
    PointerStyle pointerStyle;
    pointerStyle.id = 1;
    int32_t ret = WIN_MGR->SetPointerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_EQ(WIN_MGR->globalStyle_.id, pointerStyle.id);
    pid = 1;
    windowId = 2;
    ret = WIN_MGR->SetPointerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, WIN_MGR->UpdatePoinerStyle(pid, windowId, pointerStyle));
    pid = 1;
    windowId = 2;
    ret = WIN_MGR->SetPointerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, WIN_MGR->UpdateSceneBoardPointerStyle(pid, windowId, pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_ClearWindowPointerStyle_001
 * @tc.desc: Test clearing window pointer style
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ClearWindowPointerStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t pid = 123;
    int32_t windowId = 678;
    int32_t ret = WIN_MGR->ClearWindowPointerStyle(pid, windowId);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_GetPointerStyle_001
 * @tc.desc: Test getting pointer style
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPointerStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerStyle style;
    int32_t ret = WIN_MGR->GetPointerStyle(1, GLOBAL_WINDOW_ID, style);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_EQ(style.id, 1);
    ret = WIN_MGR->GetPointerStyle(3, 1, style);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_EQ(style.id, 1);
    ret = WIN_MGR->GetPointerStyle(1, 1, style);
    EXPECT_EQ(ret, RET_OK);
    EXPECT_EQ(style.id, 1);
}

/**
 * @tc.name: InputWindowsManagerTest_IsInHotArea_001
 * @tc.desc: Test whether input is in the hot area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsInHotArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WIN_MGR->InitPointerStyle();
    int32_t x = 10;
    int32_t y = 20;
    std::vector<Rect> rects = {
        {0, 0, 30, 40}
    };
    WindowInfo window;
    bool ret = WIN_MGR->IsInHotArea(x, y, rects, window);
    EXPECT_TRUE(ret);
    x = -10;
    y = 20;
    ret = WIN_MGR->IsInHotArea(x, y, rects, window);
    EXPECT_FALSE(ret);
    x = 10;
    y = -10;
    ret = WIN_MGR->IsInHotArea(x, y, rects, window);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea_001
 * @tc.desc: Test which hot area the input is in
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t x = 50;
    int32_t y = 50;
    std::vector<Rect> rects = {
        {0,   0,   100, 100},
        {100, 100, 200, 200}
    };
    PointerStyle pointerStyle;
    WIN_MGR->InWhichHotArea(x, y, rects, pointerStyle);
    ASSERT_EQ(pointerStyle.id, 6);
    x = 250;
    y = 250;
    WIN_MGR->InWhichHotArea(x, y, rects, pointerStyle);
    ASSERT_EQ(pointerStyle.id, 6);
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustDisplayCoordinate_001
 * @tc.desc: Test adjusting display coordinates
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AdjustDisplayCoordinate_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo displayInfo;
    displayInfo.width = 10;
    displayInfo.height = 20;
    displayInfo.validWidth = displayInfo.width;
    displayInfo.validHeight = displayInfo.height;
    displayInfo.direction = DIRECTION90;
    double physicalX = -5;
    double physicalY = 15;
    WIN_MGR->AdjustDisplayCoordinate(displayInfo, physicalX, physicalY);
    EXPECT_NE(physicalX, 100);
    EXPECT_NE(physicalY, 100);
    displayInfo.width = 10;
    displayInfo.height = 20;
    displayInfo.validWidth = displayInfo.width;
    displayInfo.validHeight = displayInfo.height;
    displayInfo.direction = DIRECTION270;
    physicalX = 15;
    physicalY = 25;
    WIN_MGR->AdjustDisplayCoordinate(displayInfo, physicalX, physicalY);
    EXPECT_NE(physicalX, 100);
    EXPECT_NE(physicalY, 100);
    displayInfo.width = 10;
    displayInfo.height = 20;
    displayInfo.validWidth = displayInfo.width;
    displayInfo.validHeight = displayInfo.height;
    displayInfo.direction = DIRECTION270;
    physicalX = -5;
    physicalY = -15;
    WIN_MGR->AdjustDisplayCoordinate(displayInfo, physicalX, physicalY);
    EXPECT_NE(physicalX, 100);
    EXPECT_NE(physicalY, 100);
}

/**
 * @tc.name: InputWindowsManagerTest_IsTransparentWin
 * @tc.desc: Test IsTransparentWin
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsTransparentWin, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::unique_ptr<Media::PixelMap> pixelMap = nullptr;
    int32_t logicalX = 0;
    int32_t logicalY = 0;
    bool result = WIN_MGR->IsTransparentWin(pixelMap, logicalX, logicalY);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_CheckWindowIdPermissionByPid
 * @tc.desc: Test CheckWindowIdPermissionByPid
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckWindowIdPermissionByPid, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t windowId = 12345;
    int32_t pid = 6789;
    int32_t result = WIN_MGR->CheckWindowIdPermissionByPid(windowId, pid);
    EXPECT_EQ(result, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_IsWindowVisible
 * @tc.desc: Test IsWindowVisible
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsWindowVisible, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t pid = -1;
    bool result = WIN_MGR->IsWindowVisible(pid);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_CoordinateCorrection_001
 * @tc.desc: Test CoordinateCorrection
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CoordinateCorrection_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t width = 100;
    int32_t height = 200;
    int32_t integerX = -1;
    int32_t integerY = 1;
    WIN_MGR->CoordinateCorrection(width, height, integerX, integerY);
    EXPECT_EQ(integerX, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_CoordinateCorrection_002
 * @tc.desc: Test CoordinateCorrection
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CoordinateCorrection_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t width = 100;
    int32_t height = 200;
    int32_t integerX = 150;
    int32_t integerY = 100;
    WIN_MGR->CoordinateCorrection(width, height, integerX, integerY);
    EXPECT_EQ(integerX, 99);
}

/**
 * @tc.name: InputWindowsManagerTest_CoordinateCorrection_003
 * @tc.desc: Test CoordinateCorrection
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CoordinateCorrection_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t width = 100;
    int32_t height = 200;
    int32_t integerX = 1;
    int32_t integerY = -1;
    WIN_MGR->CoordinateCorrection(width, height, integerX, integerY);
    EXPECT_EQ(integerY, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_CoordinateCorrection_004
 * @tc.desc: Test CoordinateCorrection
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CoordinateCorrection_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t width = 100;
    int32_t height = 200;
    int32_t integerX = 100;
    int32_t integerY = 250;
    WIN_MGR->CoordinateCorrection(width, height, integerX, integerY);
    EXPECT_EQ(integerY, 199);
}

/**
 * @tc.name: InputWindowsManagerTest_HandleWindowInputType_001
 * @tc.desc: Test HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandleWindowInputType_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::NORMAL;
    ASSERT_FALSE(WIN_MGR->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_HandleWindowInputType_002
 * @tc.desc: Test HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandleWindowInputType_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::TRANSMIT_ALL;
    ASSERT_FALSE(WIN_MGR->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_HandleWindowInputType_003
 * @tc.desc: Test HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandleWindowInputType_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::ANTI_MISTAKE_TOUCH;
    ASSERT_FALSE(WIN_MGR->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayId_001
 * @tc.desc: Test updating display ID
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayId_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t displayId = 1;
    bool ret = WIN_MGR->UpdateDisplayId(displayId);
    EXPECT_TRUE(ret);
    displayId = 0;
    ret = WIN_MGR->UpdateDisplayId(displayId);
    EXPECT_FALSE(ret);
    displayId = -1;
    ret = WIN_MGR->UpdateDisplayId(displayId);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_SelectWindowInfo_001
 * @tc.desc: Test selecting window information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectWindowInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    pointerEvent->SetPressedKeys({1});
    pointerEvent->SetTargetDisplayId(0);
    pointerEvent->SetTargetWindowId(1);
    std::optional<WindowInfo> result = WIN_MGR->SelectWindowInfo(400, 300, pointerEvent);
    EXPECT_FALSE(result.has_value());
    int32_t ret1 = result->id;
    EXPECT_EQ(ret1, 0);
    int32_t ret2 = result->flags;
    EXPECT_EQ(ret2, 0);
    int32_t ret3 = result->pid;
    EXPECT_EQ(ret3, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_SelectWindowInfo_002
 * @tc.desc: Test selecting window information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectWindowInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    pointerEvent->SetPressedKeys({1});
    pointerEvent->SetTargetDisplayId(0);
    pointerEvent->SetTargetWindowId(1);
    std::optional<WindowInfo> result = WIN_MGR->SelectWindowInfo(-123, -456, pointerEvent);
    EXPECT_FALSE(result.has_value());
    int32_t ret1 = result->id;
    EXPECT_EQ(ret1, 0);
    int32_t ret2 = result->flags;
    EXPECT_EQ(ret2, 0);
    int32_t ret3 = result->pid;
    EXPECT_EQ(ret3, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowInfo_001
 * @tc.desc: Test getting window information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo1 = {1, WindowInfo::FLAG_BIT_UNTOUCHABLE, {}};
    WindowInfo windowInfo2 = {2, 0, {}};
    WIN_MGR->displayGroupInfo_.windowsInfo = {windowInfo1, windowInfo2};
    auto result = WIN_MGR->GetWindowInfo(0, 0);
    EXPECT_FALSE(result.has_value());
    int32_t ret1 = result->id;
    EXPECT_EQ(ret1, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_SelectPointerChangeArea_001
 * @tc.desc: Test selecting pointer change area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectPointerChangeArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.id = 1;
    PointerStyle pointerStyle;
    int32_t logicalX = 0;
    int32_t logicalY = 0;
    bool result = WIN_MGR->SelectPointerChangeArea(windowInfo, pointerStyle, logicalX, logicalY);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_SelectPointerChangeArea_002
 * @tc.desc: Test selecting pointer change area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectPointerChangeArea_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.id = 1;
    PointerStyle pointerStyle;
    int32_t logicalX = -1;
    int32_t logicalY = -2;
    bool result = WIN_MGR->SelectPointerChangeArea(windowInfo, pointerStyle, logicalX, logicalY);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerChangeAreas_001
 * @tc.desc: Test updating pointer change areas
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerChangeAreas_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    WIN_MGR->UpdatePointerChangeAreas(displayGroupInfo);
    EXPECT_TRUE(WIN_MGR->windowsHotAreas_.empty());
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerChangeAreas_002
 * @tc.desc: Test updating pointer change areas
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerChangeAreas_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    WIN_MGR->UpdatePointerChangeAreas();
    WIN_MGR->UpdatePointerChangeAreas(displayGroupInfo);
    EXPECT_NE(WIN_MGR->windowsHotAreas_[1].size(), 8);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTopBottomArea_001
 * @tc.desc: Test updating top-bottom area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTopBottomArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    Rect windowArea = {0, 0, 100, 100};
    std::vector<int32_t> pointerChangeAreas = {10, 20, 30, 40, 50, 60, 70, 80};
    std::vector<Rect> windowHotAreas;
    WIN_MGR->UpdateTopBottomArea(windowArea, pointerChangeAreas, windowHotAreas);
    int32_t ret1 = windowHotAreas.size();
    EXPECT_EQ(ret1, 2);
    int32_t ret2 = windowHotAreas[0].x;
    EXPECT_EQ(ret2, 10);
    int32_t ret3 = windowHotAreas[0].y;
    EXPECT_EQ(ret3, -20);
    int32_t ret4 = windowHotAreas[0].width;
    EXPECT_EQ(ret4, 60);
    int32_t ret5 = windowHotAreas[0].height;
    EXPECT_EQ(ret5, 40);
    int32_t ret6 = windowHotAreas[1].x;
    EXPECT_EQ(ret6, 70);
    int32_t ret7 = windowHotAreas[1].y;
    EXPECT_EQ(ret7, 40);
    int32_t ret8 = windowHotAreas[1].width;
    EXPECT_EQ(ret8, -20);
    int32_t ret9 = windowHotAreas[1].height;
    EXPECT_EQ(ret9, 80);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTopBottomArea_002
 * @tc.desc: Test updating top-bottom area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTopBottomArea_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    Rect windowArea = {0, 0, 100, 100};
    std::vector<int32_t> pointerChangeAreas = {0, 0, 0, 0, 0, 0, 0, 0};
    std::vector<Rect> windowHotAreas;
    WIN_MGR->UpdateTopBottomArea(windowArea, pointerChangeAreas, windowHotAreas);
    int32_t ret1 = windowHotAreas.size();
    EXPECT_EQ(ret1, 2);
    int32_t ret2 = windowHotAreas[0].width;
    EXPECT_EQ(ret2, 0);
    int32_t ret3 = windowHotAreas[0].height;
    EXPECT_EQ(ret3, 0);
    int32_t ret4 = windowHotAreas[1].width;
    EXPECT_EQ(ret4, 0);
    int32_t ret5 = windowHotAreas[1].height;
    EXPECT_EQ(ret5, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateLeftRightArea_001
 * @tc.desc: Test updating left-right area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateLeftRightArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    Rect windowArea = {0, 0, 100, 100};
    std::vector<int32_t> pointerChangeAreas = {10, 20, 30, 40, 50, 60, 70, 80};
    std::vector<Rect> windowHotAreas;
    WIN_MGR->UpdateLeftRightArea(windowArea, pointerChangeAreas, windowHotAreas);
    int32_t ret1 = windowHotAreas.size();
    EXPECT_EQ(ret1, 2);
    int32_t ret2 = windowHotAreas[0].x;
    EXPECT_EQ(ret2, -20);
    int32_t ret3 = windowHotAreas[0].y;
    EXPECT_EQ(ret3, 10);
    int32_t ret4 = windowHotAreas[0].width;
    EXPECT_EQ(ret4, 100);
    int32_t ret5 = windowHotAreas[0].height;
    EXPECT_EQ(ret5, 20);
    int32_t ret6 = windowHotAreas[1].x;
    EXPECT_EQ(ret6, 60);
    int32_t ret7 = windowHotAreas[1].y;
    EXPECT_EQ(ret7, 30);
    int32_t ret8 = windowHotAreas[1].width;
    EXPECT_EQ(ret8, 60);
    int32_t ret9 = windowHotAreas[1].height;
    EXPECT_EQ(ret9, 20);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateLeftRightArea_002
 * @tc.desc: Test updating left-right area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateLeftRightArea_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    Rect windowArea = {0, 0, 100, 100};
    std::vector<int32_t> pointerChangeAreas = {10, 0, 30, 40, 50, 60, 70, 80};
    std::vector<Rect> windowHotAreas;
    WIN_MGR->UpdateLeftRightArea(windowArea, pointerChangeAreas, windowHotAreas);
    int32_t ret1 = windowHotAreas.size();
    EXPECT_EQ(ret1, 2);
    int32_t ret2 = windowHotAreas[0].x;
    EXPECT_EQ(ret2, -20);
    int32_t ret3 = windowHotAreas[0].y;
    EXPECT_EQ(ret3, 10);
    int32_t ret4 = windowHotAreas[0].width;
    EXPECT_EQ(ret4, 100);
    int32_t ret5 = windowHotAreas[0].height;
    EXPECT_EQ(ret5, 20);
    int32_t ret6 = windowHotAreas[1].x;
    EXPECT_EQ(ret6, 60);
    int32_t ret7 = windowHotAreas[1].y;
    EXPECT_EQ(ret7, 30);
    int32_t ret8 = windowHotAreas[1].width;
    EXPECT_EQ(ret8, 60);
    int32_t ret9 = windowHotAreas[1].height;
    EXPECT_EQ(ret9, 20);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateInnerAngleArea_001
 * @tc.desc: Test updating inner angle area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateInnerAngleArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    Rect windowArea;
    windowArea.x = 10;
    windowArea.y = 20;
    windowArea.width = 100;
    windowArea.height = 200;
    std::vector<int32_t> pointerChangeAreas(4, 10);
    std::vector<Rect> windowHotAreas;
    WIN_MGR->UpdateInnerAngleArea(windowArea, pointerChangeAreas, windowHotAreas);
    int32_t ret1 = windowHotAreas.size();
    EXPECT_EQ(ret1, 4);
    int32_t ret2 = windowHotAreas[0].x;
    EXPECT_EQ(ret2, -10);
    int32_t ret3 = windowHotAreas[0].y;
    EXPECT_EQ(ret3, 0);
    int32_t ret4 = windowHotAreas[0].width;
    EXPECT_EQ(ret4, 30);
    int32_t ret5 = windowHotAreas[0].height;
    EXPECT_EQ(ret5, 30);
    int32_t ret6 = windowHotAreas[1].x;
    EXPECT_EQ(ret6, 100);
    int32_t ret7 = windowHotAreas[1].y;
    EXPECT_EQ(ret7, 0);
    int32_t ret8 = windowHotAreas[1].width;
    EXPECT_EQ(ret8, 30);
    int32_t ret9 = windowHotAreas[1].height;
    EXPECT_EQ(ret9, 30);
    int32_t ret10 = windowHotAreas[2].x;
    EXPECT_EQ(ret10, -10);
    int32_t ret11 = windowHotAreas[2].y;
    EXPECT_NE(ret11, 110);
    int32_t ret12 = windowHotAreas[2].width;
    EXPECT_NE(ret12, 21);
    int32_t ret13 = windowHotAreas[2].height;
    EXPECT_EQ(ret13, 32);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerEvent_001
 * @tc.desc: Test updating pointer event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t logicalX = 10;
    int32_t logicalY = 20;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo touchWindow;
    touchWindow.id = 2;
    WIN_MGR->UpdatePointerEvent(logicalX, logicalY, pointerEvent, touchWindow);
    EXPECT_EQ(inputWindowsManager.lastLogicX_, RET_ERR);
    EXPECT_EQ(inputWindowsManager.lastLogicY_, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerEvent_002
 * @tc.desc: Test updating pointer event
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t logicalX = 10;
    int32_t logicalY = 20;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo touchWindow;
    touchWindow.id = 0;
    WIN_MGR->UpdatePointerEvent(logicalX, logicalY, pointerEvent, touchWindow);
    EXPECT_EQ(inputWindowsManager.lastLogicX_, RET_ERR);
    EXPECT_EQ(inputWindowsManager.lastLogicY_, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetHoverScrollState_001
 * @tc.desc: Test setting hover scroll state
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetHoverScrollState_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t result = WIN_MGR->SetHoverScrollState(true);
    EXPECT_EQ(result, 0);
    result = WIN_MGR->SetHoverScrollState(false);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_GetHoverScrollState_001
 * @tc.desc: Test getting hover scroll state
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetHoverScrollState_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = WIN_MGR->GetHoverScrollState();
    EXPECT_TRUE(result);
    result = WIN_MGR->GetHoverScrollState();
    EXPECT_TRUE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateMouseTarget_001
 * @tc.desc: Test updating mouse target
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateMouseTarget_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t result = WIN_MGR->UpdateMouseTarget(pointerEvent);
    WIN_MGR->SetMouseFlag(true);
    WIN_MGR->SetMouseFlag(false);
    auto ret = WIN_MGR->GetMouseFlag();
    EXPECT_FALSE(ret);
    EXPECT_EQ(result, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SkipNavigationWindow_001
 * @tc.desc: Test updating window information for each display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SkipNavigationWindow_001, TestSize.Level1)
{
    WIN_MGR->SetAntiMisTakeStatus(false);
    ASSERT_FALSE(WIN_MGR->SkipNavigationWindow(WindowInputType::ANTI_MISTAKE_TOUCH, PointerEvent::TOOL_TYPE_PEN));
}

/**
 * @tc.name: InputWindowsManagerTest_SkipNavigationWindow_002
 * @tc.desc: Test updating window information for each display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SkipNavigationWindow_002, TestSize.Level1)
{
    WIN_MGR->SetAntiMisTakeStatus(false);
    ASSERT_FALSE(WIN_MGR->SkipNavigationWindow(WindowInputType::NORMAL, PointerEvent::TOOL_TYPE_RUBBER));
}

/**
 * @tc.name: InputWindowsManagerTest_SkipNavigationWindow_003
 * @tc.desc: Test updating window information for each display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SkipNavigationWindow_003, TestSize.Level1)
{
    WIN_MGR->SetAntiMisTakeStatus(false);
    ASSERT_FALSE(WIN_MGR->SkipNavigationWindow(WindowInputType::NORMAL, PointerEvent::TOOL_TYPE_PEN));
}

/**
 * @tc.name: InputWindowsManagerTest_SkipNavigationWindow_004
 * @tc.desc: Test updating window information for each display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SkipNavigationWindow_004, TestSize.Level1)
{
    WIN_MGR->SetAntiMisTakeStatus(false);
    ASSERT_FALSE(WIN_MGR->SkipNavigationWindow(WindowInputType::ANTI_MISTAKE_TOUCH, PointerEvent::TOOL_TYPE_RUBBER));
}

/**
 * @tc.name: InputWindowsManagerTest_SkipNavigationWindow_005
 * @tc.desc: Test updating window information for each display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SkipNavigationWindow_005, TestSize.Level1)
{
    WIN_MGR->SetAntiMisTake(true);
    WIN_MGR->SetAntiMisTakeStatus(false);
    ASSERT_FALSE(WIN_MGR->SkipNavigationWindow(WindowInputType::ANTI_MISTAKE_TOUCH, PointerEvent::TOOL_TYPE_PEN));
}

/**
 * @tc.name: InputWindowsManagerTest_JudgMouseIsDownOrUp_001
 * @tc.desc: This test verifies the functionality of judging whether the mouse button is down or up
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_JudgMouseIsDownOrUp_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WIN_MGR->JudgMouseIsDownOrUp(false);
    EXPECT_FALSE(WIN_MGR->GetMouseFlag());
    WIN_MGR->JudgMouseIsDownOrUp(true);
    EXPECT_FALSE(WIN_MGR->GetMouseFlag());
}

/**
 * @tc.name: InputWindowsManagerTest_SetMouseCaptureMode_001
 * @tc.desc: This test verifies the functionality of setting the mouse capture mode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetMouseCaptureMode_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t windowId = -1;
    bool isCaptureMode = true;
    int32_t result = WIN_MGR->SetMouseCaptureMode(windowId, isCaptureMode);
    EXPECT_EQ(result, RET_ERR);
    windowId = 1;
    isCaptureMode = false;
    result = WIN_MGR->SetMouseCaptureMode(windowId, isCaptureMode);
    EXPECT_EQ(result, RET_OK);
    windowId = 1;
    isCaptureMode = true;
    result = WIN_MGR->SetMouseCaptureMode(windowId, isCaptureMode);
    EXPECT_EQ(result, RET_OK);
    EXPECT_TRUE(WIN_MGR->GetMouseIsCaptureMode());
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedDrawPointer_001
 * @tc.desc: This test verifies the functionality of determining whether to draw the pointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedDrawPointer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    pointerItem.SetDeviceId(1);
    bool result = WIN_MGR->IsNeedDrawPointer(pointerItem);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_SkipAnnotationWindow_001
 * @tc.desc: This test verifies the functionality of determining whether to draw the pointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SkipAnnotationWindow_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    uint32_t flag = WindowInfo::FLAG_BIT_HANDWRITING;
    int32_t toolType = PointerEvent::TOOL_TYPE_FINGER;
    bool result = WIN_MGR->SkipAnnotationWindow(flag, toolType);
    EXPECT_TRUE(result);
    flag = WindowInfo::FLAG_BIT_HANDWRITING;
    toolType = PointerEvent::TOOL_TYPE_PEN;
    result = WIN_MGR->SkipAnnotationWindow(flag, toolType);
    EXPECT_FALSE(result);
    flag = 0;
    toolType = PointerEvent::TOOL_TYPE_FINGER;
    result = WIN_MGR->SkipAnnotationWindow(flag, toolType);
    EXPECT_FALSE(result);
    flag = 0;
    toolType = PointerEvent::TOOL_TYPE_PEN;
    result = WIN_MGR->SkipAnnotationWindow(flag, toolType);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_001
 * @tc.desc: This test verifies the functionality of updating the touch screen target
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto result = WIN_MGR->UpdateTouchScreenTarget(nullptr);
    EXPECT_NE(result, RET_ERR);
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetTargetDisplayId(-1);
    result = WIN_MGR->UpdateTouchScreenTarget(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
    pointerEvent->SetTargetDisplayId(1);
    pointerEvent->SetPointerId(1);
    result = WIN_MGR->UpdateTouchScreenTarget(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_PullEnterLeaveEvent_001
 * @tc.desc: This test verifies the functionality of pulling enter and leave events
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PullEnterLeaveEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t logicalX = 100;
    int32_t logicalY = 200;
    auto pointerEvent = PointerEvent::Create();
    WindowInfo touchWindow;
    WIN_MGR->PullEnterLeaveEvent(logicalX, logicalY, pointerEvent, &touchWindow);
    logicalX = -123;
    logicalY = -456;
    WIN_MGR->PullEnterLeaveEvent(logicalX, logicalY, pointerEvent, &touchWindow);
}

/**
 * @tc.name: InputWindowsManagerTest_DispatchTouch_001
 * @tc.desc: This test verifies the functionality of touch event dispatching
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DispatchTouch_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_PULL_IN_WINDOW;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->DispatchTouch(pointerAction));
    pointerAction = PointerEvent::POINTER_ACTION_DOWN;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->DispatchTouch(pointerAction));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchPadTarget_001
 * @tc.desc: This test verifies the functionality of updating the touchpad target
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchPadTarget_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    int32_t result = WIN_MGR->UpdateTouchPadTarget(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    result = WIN_MGR->UpdateTouchPadTarget(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    result = WIN_MGR->UpdateTouchPadTarget(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    result = WIN_MGR->UpdateTouchPadTarget(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    result = WIN_MGR->UpdateTouchPadTarget(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_TOUCHPAD_ACTIVE);
    result = WIN_MGR->UpdateTouchPadTarget(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
    pointerEvent->SetPointerAction(9999);
    result = WIN_MGR->UpdateTouchPadTarget(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTargetPointer_001
 * @tc.desc: This test verifies the functionality of updating the target pointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTargetPointer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerAction(1);
    int32_t result = WIN_MGR->UpdateTargetPointer(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(1);
    result = WIN_MGR->UpdateTargetPointer(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHPAD);
    pointerEvent->SetPointerAction(1);
    result = WIN_MGR->UpdateTargetPointer(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_JOYSTICK);
    pointerEvent->SetPointerAction(1);
    result = WIN_MGR->UpdateTargetPointer(pointerEvent);
    EXPECT_EQ(result, RET_OK);
    pointerEvent->SetSourceType(999);
    pointerEvent->SetPointerAction(1);
    result = WIN_MGR->UpdateTargetPointer(pointerEvent);
    EXPECT_EQ(result, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_IsInsideDisplay_001
 * @tc.desc: This test verifies the functionality of determining whether it is inside the display area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsInsideDisplay_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo displayInfo;
    displayInfo.width = 1920;
    displayInfo.height = 1080;
    displayInfo.validWidth = displayInfo.width;
    displayInfo.validHeight = displayInfo.height;
    int32_t physicalX = 500;
    int32_t physicalY = 10;
    bool result = WIN_MGR->IsInsideDisplay(displayInfo, physicalX, physicalY);
    EXPECT_TRUE(result);
    physicalX = -10;
    physicalY = 500;
    result = WIN_MGR->IsInsideDisplay(displayInfo, physicalX, physicalY);
    EXPECT_FALSE(result);
    physicalX = 500;
    physicalY = -10;
    result = WIN_MGR->IsInsideDisplay(displayInfo, physicalX, physicalY);
    EXPECT_FALSE(result);
    physicalX = -500;
    physicalY = -10;
    result = WIN_MGR->IsInsideDisplay(displayInfo, physicalX, physicalY);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_FindPhysicalDisplay_001
 * @tc.desc: This test verifies the functionality of finding physical displays
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FindPhysicalDisplay_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo displayInfo = {10, 20};
    double physicalX, physicalY;
    int32_t displayId;
    WIN_MGR->FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId);
    EXPECT_EQ(physicalX, RET_OK);
    EXPECT_EQ(physicalY, RET_OK);
    EXPECT_EQ(displayId, RET_OK);
    displayInfo.x = INT32_MAX;
    displayInfo.y = INT32_MAX;
    WIN_MGR->FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId);
    EXPECT_EQ(physicalX, RET_OK);
    EXPECT_EQ(physicalY, RET_OK);
    EXPECT_EQ(displayId, RET_OK);
    displayInfo.x = 50;
    displayInfo.y = 60;
    WIN_MGR->FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId);
    EXPECT_EQ(physicalX, RET_OK);
    EXPECT_EQ(physicalY, RET_OK);
    EXPECT_EQ(displayId, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_GetWidthAndHeight_001
 * @tc.desc: Test the method for retrieving width and height
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWidthAndHeight_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo displayInfo;
    displayInfo.displayDirection = DIRECTION0;
    displayInfo.direction = DIRECTION0;
    int32_t width = 1920;
    int32_t height = 1080;
    WIN_MGR->GetWidthAndHeight(&displayInfo, width, height);
    EXPECT_EQ(width, RET_OK);
    EXPECT_EQ(height, RET_OK);
    displayInfo.direction = DIRECTION90;
    WIN_MGR->GetWidthAndHeight(&displayInfo, width, height);
    EXPECT_EQ(width, RET_OK);
    EXPECT_EQ(height, RET_OK);
    displayInfo.displayDirection = DIRECTION180;
    displayInfo.direction = DIRECTION0;
    WIN_MGR->GetWidthAndHeight(&displayInfo, width, height);
    EXPECT_EQ(width, RET_OK);
    EXPECT_EQ(height, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ReverseRotateScreen_001
 * @tc.desc: Test the method for reversing screen rotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReverseRotateScreen_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo info;
    Coordinate2D cursorPos;
    info.direction = DIRECTION0;
    info.width = 1920;
    info.height = 1080;
    info.validHeight = info.height;
    info.validWidth = info.width;
    WIN_MGR->ReverseRotateScreen(info, 100.0, 200.0, cursorPos);
    EXPECT_EQ(cursorPos.x, 100);
    EXPECT_EQ(cursorPos.y, 200);
    info.direction = DIRECTION90;
    WIN_MGR->ReverseRotateScreen(info, 100.0, 200.0, cursorPos);
    EXPECT_EQ(cursorPos.x, 200);
    EXPECT_EQ(cursorPos.y, 1819);
    info.direction = DIRECTION180;
    WIN_MGR->ReverseRotateScreen(info, 100.0, 200.0, cursorPos);
    EXPECT_EQ(cursorPos.x, 1819);
    EXPECT_EQ(cursorPos.y, 879);
    info.direction = DIRECTION270;
    WIN_MGR->ReverseRotateScreen(info, 100.0, 200.0, cursorPos);
    EXPECT_EQ(cursorPos.x, 879);
    EXPECT_EQ(cursorPos.y, 100);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateAndAdjustMouseLocation_001
 * @tc.desc: Test the method for updating and adjusting mouse location
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateAndAdjustMouseLocation_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t displayId = 2;
    double x = 100.5;
    double y = 200.5;
    bool isRealData = true;
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    ASSERT_NE(inputEvent, nullptr);
    WIN_MGR->UpdateAndAdjustMouseLocation(displayId, x, y, isRealData);
    auto ret = WIN_MGR->GetDisplayId(inputEvent);
    EXPECT_NE(displayId, ret);
    displayId = -1;
    x = 100.5;
    y = 200.5;
    isRealData = true;
    WIN_MGR->UpdateAndAdjustMouseLocation(displayId, x, y, isRealData);
    ret = WIN_MGR->GetDisplayId(inputEvent);
    EXPECT_NE(displayId, ret);
    displayId = 0;
    x = -100.5;
    y = -200.5;
    isRealData = true;
    WIN_MGR->UpdateAndAdjustMouseLocation(displayId, x, y, isRealData);
    ret = WIN_MGR->GetDisplayId(inputEvent);
    EXPECT_NE(displayId, ret);
    displayId = 0;
    x = 100.5;
    y = 200.5;
    isRealData = false;
    WIN_MGR->UpdateAndAdjustMouseLocation(displayId, x, y, isRealData);
    ret = WIN_MGR->GetDisplayId(inputEvent);
    EXPECT_NE(displayId, ret);
}

/**
 * @tc.name: InputWindowsManagerTest_GetMouseInfo_001
 * @tc.desc: Test the GetMouseInfo method
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetMouseInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    MouseLocation mouseLocation;
    displayGroupInfo.displaysInfo.clear();
    MouseLocation result = WIN_MGR->GetMouseInfo();
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.width = 1920;
    displayInfo.height = 1080;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    mouseLocation.displayId = 0;
    result = WIN_MGR->GetMouseInfo();
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    mouseLocation.displayId = -1;
    MouseLocation expectedResult;
    expectedResult.displayId = 1;
    expectedResult.physicalX = 960;
    expectedResult.physicalY = 540;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->GetMouseInfo());
}

/**
 * @tc.name: InputWindowsManagerTest_GetCursorPos_001
 * @tc.desc: Test the functionality of getting the cursor position
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCursorPos_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    manager.cursorPos_.displayId = -1;
    auto it = manager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != manager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back({0, 800, 600});
    }
    CursorPosition result = WIN_MGR->GetCursorPos();
    EXPECT_EQ(result.displayId, 1);
    manager.cursorPos_.displayId = 1;
    it->second.displaysInfo.push_back({1, 800, 600});
    result = WIN_MGR->GetCursorPos();
    EXPECT_EQ(result.displayId, 1);
}

/**
 * @tc.name: InputWindowsManagerTest_ResetCursorPos_001
 * @tc.desc: Test the functionality of resetting cursor position
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ResetCursorPos_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    auto it = manager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != manager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back({1, 800, 600});
    }
    CursorPosition result = WIN_MGR->ResetCursorPos();
    EXPECT_EQ(result.displayId, 1);
    it->second.displaysInfo.clear();
    result = WIN_MGR->ResetCursorPos();
    EXPECT_EQ(result.displayId, 1);
}

/**
 * @tc.name: InputWindowsManagerTest_AppendExtraData_001
 * @tc.desc: Test the functionality of appending extra data in the input window manager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AppendExtraData_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    ExtraData extraData;
    extraData.appended = true;
    extraData.buffer = std::vector<uint8_t> {1, 2, 3};
    extraData.pointerId = 12345;
    int32_t result = WIN_MGR->AppendExtraData(extraData);
    ASSERT_EQ(result, RET_OK);
    ASSERT_NE(manager.GetExtraData().appended, extraData.appended);
    ASSERT_NE(manager.GetExtraData().buffer, extraData.buffer);
    ASSERT_NE(manager.GetExtraData().pointerId, extraData.pointerId);
}

/**
 * @tc.name: InputWindowsManagerTest_ClearExtraData_001
 * @tc.desc: Test the functionality of clearing extra data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ClearExtraData_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    manager.extraData_.appended = true;
    manager.extraData_.buffer.push_back(1);
    manager.extraData_.sourceType = 0;
    manager.extraData_.pointerId = 1;
    WIN_MGR->ClearExtraData();
    EXPECT_TRUE(manager.extraData_.appended);
    EXPECT_FALSE(manager.extraData_.buffer.empty());
    EXPECT_NE(-1, manager.extraData_.sourceType);
    EXPECT_NE(-1, manager.extraData_.pointerId);
}

/**
 * @tc.name: InputWindowsManagerTest_GetExtraData_001
 * @tc.desc: Test the functionality of getting extra data in the input window manager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetExtraData_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->GetExtraData());
}

/**
 * @tc.name: InputWindowsManagerTest_IsWindowVisible_001
 * @tc.desc: Test the window visibility functionality of the input window manager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsWindowVisible_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t invalidPid = -1;
    bool result = WIN_MGR->IsWindowVisible(invalidPid);
    EXPECT_TRUE(result);
    int32_t visiblePid = 0;
    result = WIN_MGR->IsWindowVisible(visiblePid);
    EXPECT_FALSE(result);
    int32_t invisiblePid = 1;
    result = WIN_MGR->IsWindowVisible(invisiblePid);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerAction_001
 * @tc.desc: Test the update function of pointer action in Input Windows Manager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerAction_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    WIN_MGR->UpdatePointerAction(pointerEvent);
    EXPECT_EQ(pointerEvent->GetPointerAction(), PointerEvent::POINTER_ACTION_PULL_MOVE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    WIN_MGR->UpdatePointerAction(pointerEvent);
    EXPECT_EQ(pointerEvent->GetPointerAction(), PointerEvent::POINTER_ACTION_PULL_UP);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    WIN_MGR->UpdatePointerAction(pointerEvent);
    EXPECT_EQ(pointerEvent->GetPointerAction(), PointerEvent::POINTER_ACTION_PULL_UP);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_ENTER_WINDOW);
    WIN_MGR->UpdatePointerAction(pointerEvent);
    EXPECT_EQ(pointerEvent->GetPointerAction(), PointerEvent::POINTER_ACTION_PULL_IN_WINDOW);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_LEAVE_WINDOW);
    WIN_MGR->UpdatePointerAction(pointerEvent);
    EXPECT_EQ(pointerEvent->GetPointerAction(), PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW);
    pointerEvent->SetPointerAction(100);
    WIN_MGR->UpdatePointerAction(pointerEvent);
    EXPECT_EQ(pointerEvent->GetPointerAction(), 100);
}

/**
 * @tc.name: InputWindowsManagerTest_Dump_001
 * @tc.desc: Test the dump function of the input window manager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_Dump_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t fd = 1;
    std::vector<std::string> args;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->Dump(fd, args));
}

/**
 * @tc.name: InputWindowsManagerTest_TransformWindowXY_001
 * @tc.desc: Test the TransformWindowXY function of the input window manager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TransformWindowXY_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo window;
    double logicX = 10.0;
    double logicY = 20.0;
    std::pair<double, double> result = WIN_MGR->TransformWindowXY(window, logicX, logicY);
    double ret = result.first;
    EXPECT_EQ(ret, logicX);
    double ret1 = result.second;
    EXPECT_EQ(ret1, logicY);
}

/**
 * @tc.name: InputWindowsManagerTest_IsValidZorderWindow_001
 * @tc.desc: Test the validity of the input window manager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsValidZorderWindow_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo window;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->HasFlag(InputEvent::EVENT_FLAG_SIMULATE);
    bool result = WIN_MGR->IsValidZorderWindow(window, pointerEvent);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_HandleWindowInputType_004
 * @tc.desc: Test the functionality of handling window input types in the Input Windows Manage
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandleWindowInputType_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::TRANSMIT_EXCEPT_MOVE;
    ASSERT_FALSE(WIN_MGR->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowAndDisplayInfo_001
 * @tc.desc: Test the function of getting window and display information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowAndDisplayInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t windowId = 1;
    int32_t displayId = 1;
    auto result = WIN_MGR->GetWindowAndDisplayInfo(windowId, displayId);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, windowId);
    windowId = -1;
    displayId = 1;
    result = WIN_MGR->GetWindowAndDisplayInfo(windowId, displayId);
    ASSERT_FALSE(result.has_value());
    windowId = 1;
    displayId = -1;
    result = WIN_MGR->GetWindowAndDisplayInfo(windowId, displayId);
    ASSERT_TRUE(result.has_value());
}

/**
 * @tc.name: InputWindowsManagerTest_GetTargetWindowIds_001
 * @tc.desc: Test the functionality of getting target window IDs
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetTargetWindowIds_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::set<int32_t> windowIds;
    int32_t pointerItemId = 1;
    int32_t windowId = 100;
    int32_t sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    WIN_MGR->AddTargetWindowIds(pointerItemId, sourceType, windowId, 1);
    WIN_MGR->GetTargetWindowIds(pointerItemId, sourceType, windowIds, 1);
    ASSERT_TRUE(!windowIds.empty());
    WIN_MGR->ClearTargetWindowId(pointerItemId, 1);
}

/**
 * @tc.name: InputWindowsManagerTest_AddTargetWindowIds_001
 * @tc.desc: Test the functionality of adding target window IDs
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AddTargetWindowIds_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    int32_t pointerItemId = 1;
    int32_t windowId = 100;
    int32_t sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    WIN_MGR->AddTargetWindowIds(pointerItemId, sourceType, windowId, 1);
    ASSERT_FALSE(manager.targetTouchWinIds_[1].find(pointerItemId) != manager.targetTouchWinIds_[1].end());
    ASSERT_EQ(manager.targetTouchWinIds_[1][pointerItemId].size(), 0);
}

/**
 * @tc.name: InputWindowsManagerTest_AddTargetWindowIds_002
 * @tc.desc: Test the functionality of adding target window IDs
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AddTargetWindowIds_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    int32_t pointerItemId = 2;
    int32_t windowId1 = 200;
    int32_t windowId2 = 201;
    int32_t sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    manager.targetTouchWinIds_[1][pointerItemId] = {windowId1};
    WIN_MGR->AddTargetWindowIds(pointerItemId, sourceType, windowId2, 1);
    ASSERT_TRUE(manager.targetTouchWinIds_[1].find(pointerItemId) != manager.targetTouchWinIds_[1].end());
    ASSERT_EQ(manager.targetTouchWinIds_[1][pointerItemId].size(), 1);
    ASSERT_EQ(*manager.targetTouchWinIds_[1][pointerItemId].begin(), windowId1);
    ASSERT_NE(*manager.targetTouchWinIds_[1][pointerItemId].begin(), windowId2);
}

/**
 * @tc.name: InputWindowsManagerTest_CheckWindowIdPermissionByPid_002
 * @tc.desc: Test the functionality of checking window ID permission by process ID
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckWindowIdPermissionByPid_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t windowId = -123;
    int32_t pid = -456;
    int32_t result = WIN_MGR->CheckWindowIdPermissionByPid(windowId, pid);
    EXPECT_EQ(result, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_IsTransparentWin_001
 * @tc.desc: Test the functionality of transparent windows
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsTransparentWin_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::unique_ptr<Media::PixelMap> pixelMap = nullptr;
    int32_t logicalX = 0;
    int32_t logicalY = 0;
    auto result = WIN_MGR->IsTransparentWin(pixelMap, logicalX, logicalY);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_SetCurrentUser_001
 * @tc.desc: Test the functionality of setting the current user
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetCurrentUser_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t userId = 123;
    auto ret = WIN_MGR->SetCurrentUser(userId);
    EXPECT_EQ(ret, RET_OK);
    userId = -456;
    ret = WIN_MGR->SetCurrentUser(userId);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerAction_01
 * @tc.desc: Test UpdatePointerAction
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerAction_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t action = pointerEvent->GetPointerAction();
    action = PointerEvent::POINTER_ACTION_MOVE;
    WIN_MGR->UpdatePointerAction(pointerEvent);
    ASSERT_NO_FATAL_FAILURE(pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_MOVE));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerAction_02
 * @tc.desc: Test UpdatePointerAction
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerAction_02, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t action = pointerEvent->GetPointerAction();
    action = PointerEvent::POINTER_ACTION_UP;
    WIN_MGR->UpdatePointerAction(pointerEvent);
    ASSERT_NO_FATAL_FAILURE(pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_UP));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerAction_03
 * @tc.desc: Test UpdatePointerAction
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerAction_03, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t action = pointerEvent->GetPointerAction();
    action = PointerEvent::POINTER_ACTION_ENTER_WINDOW;
    WIN_MGR->UpdatePointerAction(pointerEvent);
    ASSERT_NO_FATAL_FAILURE(pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_IN_WINDOW));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerAction_04
 * @tc.desc: Test UpdatePointerAction
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerAction_04, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t action = pointerEvent->GetPointerAction();
    action = PointerEvent::POINTER_ACTION_LEAVE_WINDOW;
    WIN_MGR->UpdatePointerAction(pointerEvent);
    ASSERT_NO_FATAL_FAILURE(pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayId_002
 * @tc.desc: Test UpdateDisplayId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayId_002, TestSize.Level1)
{
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetTargetDisplayId(1);
    auto id = pointerEvent->GetTargetDisplayId();
    ASSERT_TRUE(WIN_MGR->UpdateDisplayId(id));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayId_003
 * @tc.desc: Test UpdateDisplayId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayId_003, TestSize.Level1)
{
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetTargetDisplayId(10);
    auto id = pointerEvent->GetTargetDisplayId();
    ASSERT_FALSE(WIN_MGR->UpdateDisplayId(id));
}

/**
 * @tc.name: InputWindowsManagerTest_Init_001
 * @tc.desc: Test Init
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_Init_001, TestSize.Level1)
{
    UDSServer udsServer;
    WIN_MGR->Init(udsServer);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateCaptureMode_002
 * @tc.desc: Test UpdateCaptureMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateCaptureMode_002, TestSize.Level1)
{
    OLD::DisplayGroupInfo displayGroupInfo;
    OLD::DisplayInfo info;
    info.id = 1;
    info.x = 1;
    info.y = 1;
    info.width = 2;
    info.height = 2;
    info.dpi = 240;
    info.name = "pp";
    info.uniq = "pp";
    info.direction = DIRECTION0;
    displayGroupInfo.displaysInfo.push_back(info);
    WIN_MGR->UpdateCaptureMode(displayGroupInfo);
    ASSERT_FALSE(WIN_MGR->captureModeInfo_.isCaptureMode);
}

/**
 * @tc.name: InputWindowsManagerTest_IsWindowVisible_002
 * @tc.desc: Test IsWindowVisible
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsWindowVisible_002, TestSize.Level1)
{
    int32_t pid = 1000;
    auto ret = WIN_MGR->IsWindowVisible(pid);
    ASSERT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_IsWindowVisible_003
 * @tc.desc: Test IsWindowVisible
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsWindowVisible_003, TestSize.Level1)
{
    int32_t pid = -1;
    auto ret = WIN_MGR->IsWindowVisible(pid);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowGroupInfoByDisplayId
 * @tc.desc: Test GetWindowGroupInfoByDisplayId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowGroupInfoByDisplayId, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    WindowGroupInfo windowGroupInfo;
    int32_t displayId = 1;
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    EXPECT_TRUE(inputWindowsManager.GetWindowGroupInfoByDisplayId(displayId).empty());

    WindowInfo windowInfo;
    displayId = 2;
    windowInfo.id = 1;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    EXPECT_FALSE(!inputWindowsManager.GetWindowGroupInfoByDisplayId(displayId).empty());
}

/**
 * @tc.name: InputWindowsManagerTest_GetClientFd
 * @tc.desc: Test GetClientFd
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetClientFd, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    WindowInfoEX windowInfoEX;
    windowInfoEX.flag = false;
    pointerEvent->SetPointerId(0);
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerEvent->GetPointerId(), windowInfoEX));
    EXPECT_EQ(inputWindowsManager.GetClientFd(pointerEvent), INVALID_FD);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetTargetDisplayId(10);
    pointerEvent->SetTargetWindowId(15);
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = 0;
    windowInfo.pid = 5;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    windowInfo.id = 15;
    windowInfo.pid = 6;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(pointerEvent->GetTargetDisplayId(), windowGroupInfo));
    UDSServer udsServer;
    udsServer.idxPidMap_.insert(std::make_pair(6, 15));
    inputWindowsManager.udsServer_ = &udsServer;
    EXPECT_NE(inputWindowsManager.udsServer_, nullptr);
    EXPECT_NE(inputWindowsManager.GetClientFd(pointerEvent), 15);
    pointerEvent->SetTargetWindowId(20);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_LEAVE_WINDOW);
    EXPECT_EQ(inputWindowsManager.GetClientFd(pointerEvent), INVALID_FD);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    inputWindowsManager.touchItemDownInfos_.clear();
    windowInfoEX.flag = true;
    windowInfoEX.window.agentWindowId = 1;
    pointerEvent->SetPointerId(0);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerEvent->GetPointerId(), windowInfoEX));
    EXPECT_EQ(inputWindowsManager.GetClientFd(pointerEvent), INVALID_FD);
    pointerEvent->SetPointerId(7);
    EXPECT_EQ(inputWindowsManager.GetClientFd(pointerEvent), INVALID_FD);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    inputWindowsManager.mouseDownInfo_.pid = 1;
    inputWindowsManager.mouseDownInfo_.agentWindowId = 6;
    EXPECT_EQ(inputWindowsManager.GetClientFd(pointerEvent), INVALID_FD);
    inputWindowsManager.mouseDownInfo_.pid = -1;
    EXPECT_EQ(inputWindowsManager.GetClientFd(pointerEvent), INVALID_FD);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_JOYSTICK);
    EXPECT_EQ(inputWindowsManager.GetClientFd(pointerEvent), INVALID_FD);
}

/**
 * @tc.name: InputWindowsManagerTest_GetClientFd_002
 * @tc.desc: Test GetClientFd
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetClientFd_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    UDSServer udsServer;
    int32_t udsPid = 20;
    int32_t udsFd = 15;
    udsServer.idxPidMap_.insert(std::make_pair(udsPid, udsFd));
    inputWindowsManager.udsServer_ = &udsServer;
    int32_t windowId = 15;
    EXPECT_NE(inputWindowsManager.udsServer_, nullptr);
    WindowGroupInfo widGroupInfo;
    WindowInfo windowInfo;
    pointerEvent->SetTargetDisplayId(15);
    windowInfo.id = 5;
    windowInfo.pid = 10;
    widGroupInfo.windowsInfo.push_back(windowInfo);
    windowInfo.id = 15;
    windowInfo.pid = 20;
    widGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(pointerEvent->GetTargetDisplayId(), widGroupInfo));
    EXPECT_NE(inputWindowsManager.GetClientFd(pointerEvent, windowId), udsFd);
    windowId = 7;
    EXPECT_EQ(inputWindowsManager.GetClientFd(pointerEvent, windowId), INVALID_FD);
}

/**
 * @tc.name: InputWindowsManagerTest_CheckFocusWindowChange
 * @tc.desc: Test CheckFocusWindowChange
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckFocusWindowChange, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.focusWindowId = -1;
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CheckFocusWindowChange(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_CheckZorderWindowChange
 * @tc.desc: Test CheckZorderWindowChange
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckZorderWindowChange, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::vector<WindowInfo> oldWindowsInfo;
    std::vector<WindowInfo> newWindowsInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CheckZorderWindowChange(oldWindowsInfo, newWindowsInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateCaptureMode
 * @tc.desc: Test UpdateCaptureMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateCaptureMode, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayGroupInfo displayGroupInfo;
    WindowInfo windowInfo;
    inputWindowsManager.captureModeInfo_.isCaptureMode = true;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.focusWindowId = 25;
    }
    displayGroupInfo.focusWindowId = 25;
    windowInfo.id = 10;
    it->second.windowsInfo.push_back(windowInfo);
    windowInfo.id = 11;
    displayGroupInfo.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateCaptureMode(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfoByIncrementalInfo
 * @tc.desc: Test UpdateDisplayInfoByIncrementalInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfoByIncrementalInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    WindowInfo window;
    OLD::DisplayGroupInfo displayGroupInfo;
    WindowInfo windowInfo;
    window.action = WINDOW_UPDATE_ACTION::ADD_END;
    window.id = 5;
    windowInfo.id = 10;
    displayGroupInfo.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayInfoByIncrementalInfo(window, displayGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayInfoByIncrementalInfo(window, displayGroupInfo));
    window.id = 5;
    window.action = WINDOW_UPDATE_ACTION::DEL;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayInfoByIncrementalInfo(window, displayGroupInfo));
    window.action = WINDOW_UPDATE_ACTION::CHANGE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayInfoByIncrementalInfo(window, displayGroupInfo));
    window.id = 10;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayInfoByIncrementalInfo(window, displayGroupInfo));
    window.action = WINDOW_UPDATE_ACTION::UNKNOWN;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayInfoByIncrementalInfo(window, displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayMode
 * @tc.desc: Test UpdateDisplayMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayMode, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo displayInfo;
    displayInfo.displayMode = DisplayMode::FULL;
    inputWindowsManager.displayMode_ = DisplayMode::FULL;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayMode());
    inputWindowsManager.displayMode_ = DisplayMode::MAIN;
    FINGERSENSE_WRAPPER->sendFingerSenseDisplayMode_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayMode());
    inputWindowsManager.displayMode_ = DisplayMode::MAIN;
    FINGERSENSE_WRAPPER->sendFingerSenseDisplayMode_ = FingersenseWrapperTest;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayMode());
}

/**
 * @tc.name: InputWindowsManagerTest_GetPhysicalDisplay
 * @tc.desc: Test GetPhysicalDisplay
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPhysicalDisplay, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t id = 1;
    OLD::DisplayInfo displayInfo;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayInfo.id = 0;
        it->second.displaysInfo.push_back(displayInfo);
    }
    displayInfo.id = 1;
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_NE(inputWindowsManager.GetPhysicalDisplay(id), nullptr);
}

/**
 * @tc.name: InputWindowsManagerTest_FindPhysicalDisplayInfo
 * @tc.desc: Test FindPhysicalDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FindPhysicalDisplayInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo displayInfo;
    std::string uniq = "uniq_test";
    displayInfo.uniq = "uniq_test";
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_NE(inputWindowsManager.FindPhysicalDisplayInfo(uniq), nullptr);
}

/**
 * @tc.name: InputWindowsManagerTest_OnSessionLost
 * @tc.desc: Test OnSessionLost
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_OnSessionLost, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t pointerStylePid = 10;
    int32_t uidRoot = 0;
    std::string programName = "uds_sesion_test";
    int32_t moduleType = 3;
    int32_t pid = 9;
    int32_t fd = -1;
    SessionPtr sess = std::make_shared<UDSSession>(programName, moduleType, fd, uidRoot, pid);
    int32_t windowId = 1;
    PointerStyle pointerStyle;
    pointerStyle.size = 1;
    pointerStyle.color = 2;
    pointerStyle.id = 3;
    std::map<int32_t, PointerStyle> pointerStyleMap;
    pointerStyleMap.insert(std::make_pair(windowId, pointerStyle));
    inputWindowsManager.pointerStyle_.insert(std::make_pair(pointerStylePid, pointerStyleMap));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.OnSessionLost(sess));

    pid = 10;
    SessionPtr session = std::make_shared<UDSSession>(programName, moduleType, fd, uidRoot, pid);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.OnSessionLost(session));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePoinerStyle
 * @tc.desc: Test UpdatePoinerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePoinerStyle, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t pid = 1;
    int32_t windowId = 1;
    PointerStyle pointerStyle;
    pointerStyle.size = 1;
    pointerStyle.color = 2;
    pointerStyle.id = 3;
    std::map<int32_t, PointerStyle> pointerStyleMap;
    pointerStyleMap.insert(std::make_pair(windowId, pointerStyle));
    inputWindowsManager.pointerStyle_.insert(std::make_pair(pid, pointerStyleMap));
    windowId = 2;
    WindowInfo windowInfo;
    windowInfo.id = 3;
    windowInfo.pid = 6;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    windowInfo.id = 2;
    windowInfo.pid = 1;
    it->second.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePoinerStyle(pid, windowId, pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateSceneBoardPointerStyle
 * @tc.desc: Test UpdateSceneBoardPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateSceneBoardPointerStyle, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t pid = 1000;
    int32_t windowId = 987654321;
    bool isUiExtension = true;
    PointerStyle style;
    style.id = 0;
    std::map<int32_t, PointerStyle> pointerStyleMap;
    pointerStyleMap.insert(std::make_pair(windowId, style));
    inputWindowsManager.uiExtensionPointerStyle_.insert(std::make_pair(pid, pointerStyleMap));
    pid = 1001;
    EXPECT_EQ(inputWindowsManager.UpdateSceneBoardPointerStyle(pid, windowId, style, isUiExtension), RET_OK);
    pid = 1000;
    windowId = 123456789;
    EXPECT_EQ(inputWindowsManager.UpdateSceneBoardPointerStyle(pid, windowId, style, isUiExtension), RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_SetGlobalDefaultPointerStyle
 * @tc.desc: Test SetGlobalDefaultPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetGlobalDefaultPointerStyle, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t defaultPointerStyle = 0;
    int32_t cursorCircleStyle = 41;
    int32_t pid = 100;
    int32_t windowId = 1000;
    PointerStyle pointerStyle;
    pointerStyle.id = defaultPointerStyle;
    std::map<int32_t, PointerStyle> pointerStyleMap;
    pointerStyleMap.insert(std::make_pair(windowId, pointerStyle));
    inputWindowsManager.pointerStyle_.insert(std::make_pair(pid, pointerStyleMap));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SetGlobalDefaultPointerStyle());

    pointerStyle.id = cursorCircleStyle;
    inputWindowsManager.pointerStyle_[pid][windowId] = pointerStyle;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SetGlobalDefaultPointerStyle());
}

/**
 * @tc.name: InputWindowsManagerTest_SetPointerStyle
 * @tc.desc: Test SetPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetPointerStyle, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    bool isUiExtension = false;
    int32_t pid = 100;
    int32_t windowId = 1000;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    std::map<int32_t, PointerStyle> pointerStyleMap;
    pointerStyleMap.insert(std::make_pair(windowId, pointerStyle));
    inputWindowsManager.uiExtensionPointerStyle_.insert(std::make_pair(pid, pointerStyleMap));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SetPointerStyle(pid, windowId, pointerStyle, isUiExtension));
}

/**
 * @tc.name: InputWindowsManagerTest_ClearWindowPointerStyle
 * @tc.desc: Test ClearWindowPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ClearWindowPointerStyle, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t pid = 100;
    int32_t windowId = 1000;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    std::map<int32_t, PointerStyle> pointerStyleMap;
    pointerStyleMap.insert(std::make_pair(windowId, pointerStyle));
    inputWindowsManager.pointerStyle_.insert(std::make_pair(pid, pointerStyleMap));
    windowId = 1001;
    EXPECT_EQ(inputWindowsManager.ClearWindowPointerStyle(pid, windowId), RET_OK);
    windowId = 1000;
    EXPECT_EQ(inputWindowsManager.ClearWindowPointerStyle(pid, windowId), RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_GetPointerStyle
 * @tc.desc: Test GetPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPointerStyle, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    bool isUiExtension = true;
    int32_t pid = 100;
    int32_t windowId = 1000;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    std::map<int32_t, PointerStyle> pointerStyleMap;
    pointerStyleMap.insert(std::make_pair(windowId, pointerStyle));
    inputWindowsManager.uiExtensionPointerStyle_.insert(std::make_pair(pid, pointerStyleMap));
    pid = 101;
    EXPECT_EQ(inputWindowsManager.GetPointerStyle(pid, windowId, pointerStyle, isUiExtension), RET_OK);
    pid = 100;
    windowId = 100;
    EXPECT_EQ(inputWindowsManager.GetPointerStyle(pid, windowId, pointerStyle, isUiExtension), RET_OK);
    windowId = 1000;
    EXPECT_EQ(inputWindowsManager.GetPointerStyle(pid, windowId, pointerStyle, isUiExtension), RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayId
 * @tc.desc: Test UpdateDisplayId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayId, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t displayId = -1;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 10;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_TRUE(inputWindowsManager.UpdateDisplayId(displayId));
    EXPECT_TRUE(inputWindowsManager.UpdateDisplayId(displayId));
    displayId = 15;
    EXPECT_FALSE(inputWindowsManager.UpdateDisplayId(displayId));
}

/**
 * @tc.name: InputWindowsManagerTest_SelectWindowInfo
 * @tc.desc: Test SelectWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectWindowInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t desplayId = 10;
    int32_t logicalX = 200;
    int32_t logicalY = 200;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    pointerEvent->SetTargetDisplayId(10);
    WindowInfo windowInfo;
    windowInfo.id = 10;
    WindowGroupInfo windowGroupInfo;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(desplayId, windowGroupInfo));
    inputWindowsManager.firstBtnDownWindowInfo_.first = 10;
    inputWindowsManager.extraData_.appended = false;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_UNKNOWN;
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_ROTATE_BEGIN);
    pointerEvent->SetButtonPressed(1);
    pointerEvent->SetButtonPressed(2);
    EXPECT_EQ(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent), std::nullopt);
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowInfo
 * @tc.desc: Test GetWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t logicalX = 200;
    int32_t logicalY = 200;
    WindowInfo windowInfo;
    windowInfo.flags = 1;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    EXPECT_EQ(inputWindowsManager.GetWindowInfo(logicalX, logicalY), std::nullopt);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateLeftRightArea
 * @tc.desc: Test UpdateLeftRightArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateLeftRightArea, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    Rect windowArea;
    std::vector<int32_t> pointerChangeAreas {5, 0, 5, 0, 5, 0, 5, 0};
    std::vector<Rect> windowHotAreas;
    windowArea.x = 200;
    windowArea.y = 200;
    windowArea.height = 400;
    windowArea.width = 400;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateLeftRightArea(windowArea, pointerChangeAreas, windowHotAreas));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateInnerAngleArea
 * @tc.desc: Test UpdateInnerAngleArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateInnerAngleArea, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    Rect windowArea;
    std::vector<int32_t> pointerChangeAreas {0, 16, 0, 16, 0, 16, 0, 16};
    std::vector<Rect> windowHotAreas;
    windowArea.x = 200;
    windowArea.y = 200;
    windowArea.height = 400;
    windowArea.width = 400;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateInnerAngleArea(windowArea, pointerChangeAreas, windowHotAreas));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerEvent
 * @tc.desc: Test UpdatePointerEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerEvent, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t logicalX = 200;
    int32_t logicalY = 200;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo touchWindow;
    touchWindow.id = 1;
    inputWindowsManager.lastWindowInfo_.id = 1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerEvent(logicalX, logicalY, pointerEvent, touchWindow));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateMouseTarget
 * @tc.desc: Test UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateMouseTarget, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetTargetDisplayId(-1);
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);

    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    pointerEvent->SetPointerId(5);
    PointerEvent::PointerItem item;
    item.SetPointerId(10);
    pointerEvent->AddPointerItem(item);
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_JudgMouseIsDownOrUp
 * @tc.desc: Test JudgMouseIsDownOrUp
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_JudgMouseIsDownOrUp, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    bool dragState = false;
    inputWindowsManager.lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager.lastPointerEvent_, nullptr);
    inputWindowsManager.lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    inputWindowsManager.pointerActionFlag_ = PointerEvent::POINTER_ACTION_BUTTON_UP;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.JudgMouseIsDownOrUp(dragState));

    dragState = true;
    inputWindowsManager.lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    inputWindowsManager.pointerActionFlag_ = PointerEvent::POINTER_ACTION_BUTTON_DOWN;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.JudgMouseIsDownOrUp(dragState));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateJoystickTarget
 * @tc.desc: Test UpdateJoystickTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateJoystickTarget, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.focusWindowId = 150;
    }
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetTargetDisplayId(-1);
    WindowInfo windowInfo;
    windowInfo.id = 150;
    windowInfo.agentWindowId = 200;
    it->second.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateJoystickTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateAndAdjustMouseLocation
 * @tc.desc: Test UpdateAndAdjustMouseLocation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateAndAdjustMouseLocation, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t displayId = 1;
    double x = 200;
    double y = 200;
    bool isRealData = true;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.width = 500;
    displayInfo.height = 500;
    displayInfo.displayDirection = DIRECTION0;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateAndAdjustMouseLocation(displayId, x, y, isRealData));
    isRealData = false;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateAndAdjustMouseLocation(displayId, x, y, isRealData));
}

/**
 * @tc.name: InputWindowsManagerTest_GetMouseInfo
 * @tc.desc: Test GetMouseInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetMouseInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.mouseLocation_.displayId = -1;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.width = 600;
    displayInfo.height = 600;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_EQ(inputWindowsManager.GetMouseInfo().displayId, displayInfo.id);
}

/**
 * @tc.name: InputWindowsManagerTest_GetCursorPos
 * @tc.desc: Test GetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCursorPos, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.cursorPos_.displayId = -1;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.width = 300;
    displayInfo.height = 300;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_NE(inputWindowsManager.GetCursorPos().displayId, displayInfo.id);
}

/**
 * @tc.name: InputWindowsManagerTest_ResetCursorPos
 * @tc.desc: Test ResetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ResetCursorPos, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.width = 300;
    displayInfo.height = 300;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_EQ(inputWindowsManager.ResetCursorPos().displayId, displayInfo.id);
}

/**
 * @tc.name: InputWindowsManagerTest_GetTargetWindowIds
 * @tc.desc: Test GetTargetWindowIds
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetTargetWindowIds, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t pointerItemId = 1;
    int32_t sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    std::set<int32_t> windowIds {1, 2, 3};
    inputWindowsManager.targetTouchWinIds_[1].insert(std::make_pair(pointerItemId, windowIds));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetTargetWindowIds(pointerItemId, sourceType, windowIds, 1));
}

/**
 * @tc.name: InputWindowsManagerTest_CheckWindowIdPermissionByPid_001
 * @tc.desc: Test CheckWindowIdPermissionByPid
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckWindowIdPermissionByPid_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 300;
    int32_t pid = 500;
    WindowInfo windowInfo;
    windowInfo.id = 300;
    windowInfo.pid = 500;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    EXPECT_EQ(inputWindowsManager.CheckWindowIdPermissionByPid(windowId, pid), RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_FindPhysicalDisplay_002
 * @tc.desc: This test verifies the functionality of finding physical displays
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FindPhysicalDisplay_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo displayInfo;
    OLD::DisplayInfo displaysInfo;
    double logicalX = 300;
    double logicalY = 400;
    double physicalX = 100;
    double physicalY = 200;
    int32_t displayId = -1;
    displayInfo.x = INT32_MAX;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId));
    displayInfo.x = 200;
    inputWindowsManager.FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId);
    EXPECT_EQ(logicalX, physicalX + displayInfo.x);
    displayInfo.y = INT32_MAX;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId));
    displayInfo.y = 200;
    inputWindowsManager.FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId);
    EXPECT_EQ(logicalY, physicalY + displayInfo.y);
    displaysInfo.x = 100;
    displaysInfo.width = INT32_MAX;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId));
}

/**
 * @tc.name: InputWindowsManagerTest_FindPhysicalDisplay_003
 * @tc.desc: This test verifies the functionality of finding physical displays
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FindPhysicalDisplay_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo displayInfo;
    OLD::DisplayInfo displaysInfo;
    double logicalX = 300;
    double logicalY = 400;
    double physicalX = 100;
    double physicalY = 200;
    int32_t displayMaxX = 300;
    int32_t displayMaxY = 400;
    int32_t displayId = -1;
    displayInfo.x = 200;
    inputWindowsManager.FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId);
    EXPECT_EQ(logicalX, physicalX + displayInfo.x);
    displayInfo.y = 200;
    inputWindowsManager.FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId);
    EXPECT_EQ(logicalY, physicalY + displayInfo.y);
    displaysInfo.x = 200;
    displaysInfo.width = 100;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    inputWindowsManager.FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId);
    EXPECT_EQ(displayMaxX, displaysInfo.x + displaysInfo.width);
    displaysInfo.y = 100;
    displaysInfo.height = INT32_MAX;
    it->second.displaysInfo.push_back(displaysInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId));
    displaysInfo.y = 200;
    displaysInfo.height = 200;
    it->second.displaysInfo.push_back(displaysInfo);
    inputWindowsManager.FindPhysicalDisplay(displayInfo, physicalX, physicalY, displayId);
    EXPECT_EQ(displayMaxY, displaysInfo.y + displaysInfo.height);
}

/**
 * @tc.name: InputWindowsManagerTest_IsInHotArea
 * @tc.desc: Test IsInHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsInHotArea, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 200;
    int32_t y = 300;
    std::vector<Rect> rects;
    WindowInfo window;
    Rect rect;
    rect.x = 100;
    rect.width = INT32_MAX;
    rects.push_back(rect);
    EXPECT_FALSE(inputWindowsManager.IsInHotArea(x, y, rects, window));
    rects.clear();
    rects = {
        {150, 100, 300, INT32_MAX}
    };
    EXPECT_FALSE(inputWindowsManager.IsInHotArea(x, y, rects, window));
    rects.clear();
    rects = {
        {150, 250, 300, 500}
    };
    EXPECT_TRUE(inputWindowsManager.IsInHotArea(x, y, rects, window));
    x = 100;
    y = 200;
    EXPECT_FALSE(inputWindowsManager.IsInHotArea(x, y, rects, window));
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea
 * @tc.desc: Test InWhichHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 500;
    int32_t y = 800;
    std::vector<Rect> rects;
    PointerStyle pointerStyle;
    rects = {
        {100, 0, INT32_MAX, 0}
    };
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects, pointerStyle));
    rects.clear();
    rects = {
        {150, 100, 300, INT32_MAX}
    };
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects, pointerStyle));
    rects.clear();
    rects = {
        {150, 250, 300, 500}
    };
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects, pointerStyle));
    x = 200;
    y = 300;
    EXPECT_TRUE(inputWindowsManager.InWhichHotArea(x, y, rects, pointerStyle));
    int32_t cycleNum = 7;
    for (int32_t i = 0; i < cycleNum; ++i) {
        rects.insert(rects.begin(), {1000, 1000, 1500, 1500});
        EXPECT_TRUE(inputWindowsManager.InWhichHotArea(x, y, rects, pointerStyle));
    }
}

/**
 * @tc.name: InputWindowsManagerTest_DispatchPointer
 * @tc.desc: Test DispatchPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DispatchPointer, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_ENTER_WINDOW;
    UDSServer udsServer;
    inputWindowsManager.udsServer_ = &udsServer;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointer(pointerAction));
    CursorDrawingComponent::GetInstance().SetMouseDisplayState(true);
    inputWindowsManager.lastPointerEvent_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointer(pointerAction));
    inputWindowsManager.lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(inputWindowsManager.lastPointerEvent_, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    inputWindowsManager.lastPointerEvent_->AddPointerItem(item);
    inputWindowsManager.lastPointerEvent_->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointer(pointerAction));
    inputWindowsManager.lastPointerEvent_->SetPointerId(0);
    inputWindowsManager.lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    WindowInfo windowInfo;
    windowInfo.flags = WindowInfo::FLAG_BIT_HANDWRITING;
    windowInfo.pointerHotAreas.push_back({100, 0, INT32_MAX, 0});
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    inputWindowsManager.lastLogicX_ = 200;
    inputWindowsManager.lastLogicY_ = 200;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointer(pointerAction));
}

/**
 * @tc.name: InputWindowsManagerTest_DispatchPointer_002
 * @tc.desc: Test DispatchPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DispatchPointer_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_ENTER_WINDOW;
    UDSServer udsServer;
    inputWindowsManager.udsServer_ = &udsServer;
    inputWindowsManager.lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(inputWindowsManager.lastPointerEvent_, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDisplayX(100);
    item.SetDisplayY(100);
    inputWindowsManager.lastPointerEvent_->AddPointerItem(item);
    inputWindowsManager.lastPointerEvent_->SetPointerId(0);
    inputWindowsManager.firstBtnDownWindowInfo_.first = 1;
    inputWindowsManager.lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    inputWindowsManager.lastPointerEvent_->SetTargetDisplayId(-1);
    WindowInfo windowInfo;
    windowInfo.id = 1;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    inputWindowsManager.lastWindowInfo_.id = 2;
    inputWindowsManager.lastWindowInfo_.agentWindowId = 2;
    inputWindowsManager.lastWindowInfo_.area.x = 100;
    inputWindowsManager.lastWindowInfo_.area.y = 100;
    inputWindowsManager.lastPointerEvent_->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    inputWindowsManager.lastPointerEvent_->SetDeviceId(5);
    inputWindowsManager.extraData_.appended = true;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    InputHandler->eventFilterHandler_ = std::make_shared<EventFilterHandler>();
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointer(pointerAction));
    inputWindowsManager.lastWindowInfo_.id = 1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointer(pointerAction));
    inputWindowsManager.extraData_.appended = false;
    pointerAction = PointerEvent::POINTER_ACTION_LEAVE_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointer(pointerAction));
}

/**
 * @tc.name: InputWindowsManagerTest_NotifyPointerToWindow
 * @tc.desc: Test NotifyPointerToWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_NotifyPointerToWindow, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    UDSServer udsServer;
    inputWindowsManager.lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager.lastPointerEvent_, nullptr);
    inputWindowsManager.lastLogicX_ = 200;
    inputWindowsManager.lastLogicY_ = 300;
    WindowInfo windowInfo;
    windowInfo.flags = WindowInfo::FLAG_BIT_HANDWRITING;
    windowInfo.pointerHotAreas.push_back({100, 100, INT32_MAX, 100});
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.NotifyPointerToWindow());
    windowInfo.id = 10;
    windowInfo.pointerHotAreas.clear();
    windowInfo.pointerHotAreas.push_back({150, 250, 300, 500});
    it->second.windowsInfo.insert(it->second.windowsInfo.begin(), windowInfo);
    inputWindowsManager.lastWindowInfo_.id = 10;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.NotifyPointerToWindow());
    inputWindowsManager.lastWindowInfo_.id = 20;
    inputWindowsManager.lastWindowInfo_.pid = 50;
    int32_t udsFd = 100;
    udsServer.idxPidMap_.insert(std::make_pair(inputWindowsManager.lastWindowInfo_.pid, udsFd));
    inputWindowsManager.udsServer_ = &udsServer;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.NotifyPointerToWindow());
    inputWindowsManager.udsServer_ = nullptr;
    inputWindowsManager.lastWindowInfo_.id = 30;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.NotifyPointerToWindow());
    windowInfo.id = 50;
    it->second.windowsInfo.push_back(windowInfo);
    inputWindowsManager.lastWindowInfo_.id = 50;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.NotifyPointerToWindow());
}

/**
 * @tc.name: InputWindowsManagerTest_GetMouseInfo_002
 * @tc.desc: Test the function GetMouseInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetMouseInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.mouseLocation_.displayId = -1;
    OLD::DisplayInfo displaysInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetMouseInfo());
    displaysInfo.id = 2;
    displaysInfo.width = 20;
    displaysInfo.height = 30;
    displaysInfo.validWidth = displaysInfo.width;
    displaysInfo.validHeight = displaysInfo.height;
    displaysInfo.name = "name1";
    displaysInfo.uniq = "uniq1";
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    MouseLocation result = inputWindowsManager.GetMouseInfo();
    EXPECT_EQ(result.displayId, 2);
    EXPECT_EQ(result.physicalX, 10);
    EXPECT_EQ(result.physicalY, 15);
}

/**
 * @tc.name: InputWindowsManagerTest_GetCursorPos_002
 * @tc.desc: Test the function GetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCursorPos_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.cursorPos_.displayId = -1;
    OLD::DisplayInfo displaysInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetCursorPos());
    displaysInfo.id = 2;
    displaysInfo.width = 30;
    displaysInfo.height = 40;
    displaysInfo.validWidth = displaysInfo.width;
    displaysInfo.validHeight = displaysInfo.height;
    displaysInfo.name = "name2";
    displaysInfo.uniq = "uniq2";
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    CursorPosition result = inputWindowsManager.GetCursorPos();
    EXPECT_EQ(result.displayId, 2);
    EXPECT_EQ(result.cursorPos.x, 15);
    EXPECT_EQ(result.cursorPos.y, 20);
}

/**
 * @tc.name: InputWindowsManagerTest_ResetCursorPos_002
 * @tc.desc: Test the function ResetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ResetCursorPos_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo displaysInfo;
    CursorPosition result = inputWindowsManager.ResetCursorPos();
    EXPECT_EQ(result.displayId, -1);
    displaysInfo.id = 3;
    displaysInfo.width = 40;
    displaysInfo.height = 50;
    displaysInfo.validWidth = displaysInfo.width;
    displaysInfo.validHeight = displaysInfo.height;
    displaysInfo.name = "name3";
    displaysInfo.uniq = "uniq3";
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    result = inputWindowsManager.ResetCursorPos();
    EXPECT_EQ(result.displayId, 3);
    EXPECT_EQ(result.cursorPos.x, 20);
    EXPECT_EQ(result.cursorPos.y, 25);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayIdAndName_002
 * @tc.desc: Test updating display ID and name
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayIdAndName_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 1;
    displaysInfo.uniq = "abc";
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    inputWindowsManager.bindInfo_.AddDisplay(2, "cde");
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayIdAndName());
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayIdAndName_003
 * @tc.desc: Test updating display ID and name
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayIdAndName_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 1;
    displaysInfo.uniq = "abc";
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    inputWindowsManager.bindInfo_.AddDisplay(1, "abc");
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayIdAndName());
}

/**
 * @tc.name: InputWindowsManagerTest_SendPointerEvent
 * @tc.desc: Test SendPointerEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SendPointerEvent, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_ENTER_WINDOW;
    UDSServer udsServer;
    WindowInfo windowInfo;
    inputWindowsManager.udsServer_ = &udsServer;
    windowInfo.pointerHotAreas.push_back({100, 100, INT32_MAX, 100});
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SendPointerEvent(pointerAction));
    windowInfo.pointerHotAreas.clear();
    windowInfo.pointerHotAreas.push_back({150, 250, 300, 500});
    windowInfo.area.x = 50;
    windowInfo.area.y = 50;
    windowInfo.id = 10;
    windowInfo.pid = 30;
    windowInfo.agentWindowId = 10;
    it->second.windowsInfo.insert(it->second.windowsInfo.begin(), windowInfo);
    inputWindowsManager.mouseLocation_.displayId = 1;
    inputWindowsManager.mouseLocation_.physicalX = 200;
    inputWindowsManager.mouseLocation_.physicalY = 300;
    inputWindowsManager.lastLogicX_ = 150;
    inputWindowsManager.lastLogicY_ = 150;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SendPointerEvent(pointerAction));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateMouseTarget001
 * @tc.desc: Test UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateMouseTarget001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetTargetDisplayId(1);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.x = 300;
    displayInfo.y = 500;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDisplayX(INT32_MAX);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(0);
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);

    pointerEvent->SetPointerId(1);
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);

    item.SetDisplayX(150);
    item.SetDisplayY(INT32_MAX);
    pointerEvent->UpdatePointerItem(pointerEvent->GetPointerId(), item);
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);
    item.SetDisplayX(150);
    item.SetDisplayY(300);
    pointerEvent->UpdatePointerItem(pointerEvent->GetPointerId(), item);

    inputWindowsManager.firstBtnDownWindowInfo_.first = 5;
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = 10;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(pointerEvent->GetTargetDisplayId(), windowGroupInfo));
    inputWindowsManager.mouseDownInfo_.id = -1;
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);

    inputWindowsManager.firstBtnDownWindowInfo_.first = 10;
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    inputWindowsManager.SetHoverScrollState(false);
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateMouseTarget_002
 * @tc.desc: Test UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateMouseTarget_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetTargetDisplayId(1);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.x = 300;
    displayInfo.y = 500;
    displayInfo.width = 100;
    displayInfo.height = 100;
    displayInfo.displayDirection = DIRECTION0;
    displayInfo.direction = DIRECTION180;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDisplayX(150);
    item.SetDisplayY(300);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);
    inputWindowsManager.firstBtnDownWindowInfo_.first = 10;
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = 10;
    windowInfo.pid = 50;
    windowInfo.agentWindowId = 60;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(pointerEvent->GetTargetDisplayId(), windowGroupInfo));
    inputWindowsManager.mouseDownInfo_.id = -1;
    inputWindowsManager.SetHoverScrollState(true);
    std::map<int32_t, PointerStyle> styleMap;
    PointerStyle pointerStyle;
    CursorDrawingComponent::GetInstance().SetMouseDisplayState(false);
    styleMap.insert(std::make_pair(windowInfo.id, pointerStyle));
    inputWindowsManager.uiExtensionPointerStyle_.insert(std::make_pair(windowInfo.pid, styleMap));
    UDSServer udsServer;
    inputWindowsManager.udsServer_ = &udsServer;
    inputWindowsManager.SetUiExtensionInfo(true, 50, 10);
    inputWindowsManager.isDragBorder_ = true;
    inputWindowsManager.dragFlag_ = false;
    inputWindowsManager.captureModeInfo_.isCaptureMode = true;
    inputWindowsManager.captureModeInfo_.windowId = 1;
    inputWindowsManager.extraData_.appended = true;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateMouseTarget_003
 * @tc.desc: Test UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateMouseTarget_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetTargetDisplayId(1);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.x = 300;
    displayInfo.y = 500;
    displayInfo.width = 100;
    displayInfo.height = 100;
    displayInfo.displayDirection = DIRECTION180;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDisplayX(150);
    item.SetDisplayY(300);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);
    inputWindowsManager.firstBtnDownWindowInfo_.first = 10;
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = 10;
    windowInfo.pid = 50;
    windowInfo.agentWindowId = 60;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(pointerEvent->GetTargetDisplayId(), windowGroupInfo));
    inputWindowsManager.mouseDownInfo_.id = -1;
    inputWindowsManager.SetHoverScrollState(true);
    std::map<int32_t, PointerStyle> styleMap;
    PointerStyle pointerStyle;
    CursorDrawingComponent::GetInstance().SetMouseDisplayState(true);
    styleMap.insert(std::make_pair(windowInfo.id, pointerStyle));
    inputWindowsManager.uiExtensionPointerStyle_.insert(std::make_pair(windowInfo.pid, styleMap));
    UDSServer udsServer;
    inputWindowsManager.udsServer_ = &udsServer;
    inputWindowsManager.SetUiExtensionInfo(false, 50, 10);
    inputWindowsManager.isDragBorder_ = false;
    inputWindowsManager.dragFlag_ = true;
    inputWindowsManager.captureModeInfo_.isCaptureMode = false;
    inputWindowsManager.captureModeInfo_.windowId = 10;
    inputWindowsManager.extraData_.appended = false;
    EXPECT_EQ(inputWindowsManager.UpdateMouseTarget(pointerEvent), RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetTargetDisplayId(1);
    pointerEvent->SetPointerId(0);
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.width = 300;
    displayInfo.height = 300;
    displayInfo.displayDirection = DIRECTION0;
    displayInfo.x = INT32_MAX;
    displayInfo.y = 300;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetDisplayX(100);
    item.SetDisplayY(100);
    item.SetDisplayXPos(100);
    item.SetDisplayYPos(100);
    item.SetTargetWindowId(-1);
    pointerEvent->AddPointerItem(item);
    EXPECT_EQ(inputWindowsManager.UpdateTouchScreenTarget(pointerEvent), RET_ERR);

    pointerEvent->SetPointerId(1);
    pointerEvent->bitwise_ = InputEvent::EVENT_FLAG_SIMULATE;
    EXPECT_EQ(inputWindowsManager.UpdateTouchScreenTarget(pointerEvent), RET_ERR);

    pointerEvent->bitwise_ = InputEvent::EVENT_FLAG_NONE;
    it->second.displaysInfo[0].x = 300;
    it->second.displaysInfo[0].y = INT32_MAX;
    EXPECT_EQ(inputWindowsManager.UpdateTouchScreenTarget(pointerEvent), RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_GetPidAndUpdateTarget_002
 * @tc.desc: Test getting PID and updating the target
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPidAndUpdateTarget_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetTargetDisplayId(10);
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.focusWindowId = 52;
    }
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = 2;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    windowInfo.id = 52;
    windowInfo.pid = 100;
    windowInfo.agentWindowId = 65;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(keyEvent->GetTargetDisplayId(), windowGroupInfo));
    ASSERT_NO_FATAL_FAILURE(inputWindowsManager.GetPidAndUpdateTarget(keyEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateAndAdjustMouseLocation_002
 * @tc.desc: Test UpdateAndAdjustMouseLocation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateAndAdjustMouseLocation_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t displayId = 1;
    double x = 350;
    double y = 350;
    bool isRealData = true;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.x = 600;
    displayInfo.y = 600;
    displayInfo.width = 300;
    displayInfo.height = 300;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    displayInfo.id = 2;
    displayInfo.x = 500;
    displayInfo.y = 500;
    displayInfo.width = 600;
    displayInfo.height = 600;
    displayInfo.displayDirection = DIRECTION0;
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateAndAdjustMouseLocation(displayId, x, y, isRealData));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateAndAdjustMouseLocation_003
 * @tc.desc: Test UpdateAndAdjustMouseLocation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateAndAdjustMouseLocation_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t displayId = 1;
    double x = 200;
    double y = 200;
    bool isRealData = false;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.x = 600;
    displayInfo.y = 600;
    displayInfo.width = 400;
    displayInfo.height = 400;
    displayInfo.displayDirection = DIRECTION90;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateAndAdjustMouseLocation(displayId, x, y, isRealData));
    it->second.displaysInfo[0].displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateAndAdjustMouseLocation(displayId, x, y, isRealData));
}

/**
 * @tc.name: InputWindowsManagerTest_HandleWindowInputType_005
 * @tc.desc: Test HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandleWindowInputType_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetToolType(PointerEvent::TOOL_TYPE_MOUSE);
    pointerEvent->SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHPAD);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    WindowInfo window;
    window.windowInputType = WindowInputType::NORMAL;
    EXPECT_FALSE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
    window.windowInputType = WindowInputType::TRANSMIT_ALL;
    EXPECT_TRUE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
    window.windowInputType = WindowInputType::TRANSMIT_EXCEPT_MOVE;
    EXPECT_TRUE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
    window.windowInputType = WindowInputType::ANTI_MISTAKE_TOUCH;
    EXPECT_FALSE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
    window.windowInputType = WindowInputType::TRANSMIT_AXIS_MOVE;
    EXPECT_FALSE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
    window.windowInputType = WindowInputType::TRANSMIT_MOUSE_MOVE;
    EXPECT_FALSE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
    window.windowInputType = WindowInputType::TRANSMIT_LEFT_RIGHT;
    EXPECT_FALSE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
    window.windowInputType = WindowInputType::TRANSMIT_BUTTOM;
    EXPECT_FALSE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
    window.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    EXPECT_FALSE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
    window.windowInputType = WindowInputType::MIX_BUTTOM_ANTI_AXIS_MOVE;
    EXPECT_FALSE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
    window.windowInputType = static_cast<WindowInputType>(100);
    EXPECT_FALSE(inputWindowsMgr.HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_AddTargetWindowIds_003
 * @tc.desc: Test AddTargetWindowIds
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AddTargetWindowIds_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t pointerItemId = 1;
    int32_t sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    int32_t windowId = 50;
    std::set<int32_t> winIds;
    inputWindowsMgr.targetMouseWinIds_.insert(std::make_pair(pointerItemId, winIds));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.AddTargetWindowIds(pointerItemId, sourceType, windowId, 1));

    pointerItemId = 2;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.AddTargetWindowIds(pointerItemId, sourceType, windowId, 1));

    sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    inputWindowsMgr.targetTouchWinIds_[1].insert(std::make_pair(pointerItemId, winIds));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.AddTargetWindowIds(pointerItemId, sourceType, windowId, 1));
}

/**
 * @tc.name: InputWindowsManagerTest_ReverseXY
 * @tc.desc: Test ReverseXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReverseXY, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t x = 100;
    int32_t y = 100;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.ReverseXY(x, y));

    OLD::DisplayInfo displayInfo;
    displayInfo.direction = DIRECTION0;
    displayInfo.width = 200;
    displayInfo.height = 300;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.ReverseXY(x, y));
}

/**
 * @tc.name: InputWindowsManagerTest_SendCancelEventWhenLock
 * @tc.desc: Test SendCancelEventWhenLock
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SendCancelEventWhenLock, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SendCancelEventWhenLock());
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation
 * @tc.desc: Test FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_PrintChangedWindowBySync_001
 * @tc.desc: Test PrintChangedWindowBySync
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintChangedWindowBySync_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    OLD::DisplayGroupInfo newDisplayInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.PrintChangedWindowBySync(newDisplayInfo));
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 50;
    windowInfo.zOrder = 60;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    windowInfo.id = 2;
    newDisplayInfo.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.PrintChangedWindowBySync(newDisplayInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_PrintChangedWindowBySync_002
 * @tc.desc: Test PrintChangedWindowBySync
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintChangedWindowBySync_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    OLD::DisplayGroupInfo newDisplayGroupInfo;
    OLD::DisplayInfo newDisplayInfo;
    newDisplayInfo.direction = DIRECTION0;
    newDisplayInfo.displayDirection = DIRECTION0;
    newDisplayInfo.width = 200;
    newDisplayInfo.height = 300;
    newDisplayGroupInfo.displaysInfo.push_back(newDisplayInfo);
    OLD::DisplayInfo displayInfo;
    displayInfo.direction = DIRECTION0;
    displayInfo.displayDirection = DIRECTION0;
    displayInfo.width = 200;
    displayInfo.height = 300;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.PrintChangedWindowBySync(newDisplayGroupInfo));

    newDisplayInfo.direction = DIRECTION90;
    newDisplayGroupInfo.displaysInfo.clear();
    newDisplayGroupInfo.displaysInfo.push_back(newDisplayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.PrintChangedWindowBySync(newDisplayGroupInfo));

    displayInfo.direction = DIRECTION90;
    auto iter = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (iter != inputWindowsMgr.displayGroupInfoMap_.end()) {
        iter->second.displaysInfo.clear();
        iter->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.PrintChangedWindowBySync(newDisplayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_GetClientFd_003
 * @tc.desc: Test the funcation GetClientFd
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetClientFd_003, TestSize.Level1)
{
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    WindowInfoEX windowInfoEX;
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 5;
    std::vector<WindowInfo> windows;
    windows.push_back(windowInfo);
    windowInfoEX.window = windows[0];
    windowInfoEX.flag = false;
    pointerEvent->pointerId_ = 1;
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(1, windowInfoEX));
    int32_t ret = inputWindowsManager.GetClientFd(pointerEvent);
    EXPECT_EQ(ret, -1);
    windowInfoEX.flag = true;
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    EXPECT_NE(inputEvent, nullptr);
    inputEvent->targetDisplayId_ = 5;
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(10, windowInfoEX));
    ret = inputWindowsManager.GetClientFd(pointerEvent);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name: InputWindowsManagerTest_GetPidAndUpdateTarget_003
 * @tc.desc: Test the funcation GetPidAndUpdateTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPidAndUpdateTarget_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.focusWindowId = 10;
    }
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    EXPECT_NE(inputEvent, nullptr);
    inputEvent->targetDisplayId_ = 10;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetPidAndUpdateTarget(keyEvent));
    inputEvent->targetDisplayId_ = 18;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetPidAndUpdateTarget(keyEvent));
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = 10;
    windowInfo.pid = 11;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    windowInfo.id = 10;
    windowInfo.pid = 11;
    windowInfo.agentWindowId = 12;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(10, windowGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetPidAndUpdateTarget(keyEvent));
}

#ifdef OHOS_BUILD_ENABLE_POINTER
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_002
 * @tc.desc: Test the funcation IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 1;
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    EXPECT_NE(inputEvent, nullptr);
    inputEvent->targetDisplayId_ = -11;
    bool ret = inputWindowsManager.IsNeedRefreshLayer(windowId);
    EXPECT_FALSE(ret);
    inputEvent->targetDisplayId_ = 11;
    EXPECT_FALSE(ret);
}

#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING
#endif //OHOS_BUILD_ENABLE_POINTER

/**
 * @tc.name: InputWindowsManagerTest_SelectWindowInfo_003
 * @tc.desc: Test the funcation SelectWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectWindowInfo_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t logicalX = 200;
    int32_t logicalY = 200;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager.firstBtnDownWindowInfo_.first = -1;
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_BUTTON_DOWN;
    PointerEvent::PointerItem pointerItem;
    pointerItem.targetWindowId_ = 0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerItem.targetWindowId_ = 2;
    WindowInfo windowInfo;
    windowInfo.flags = WindowInfo::FLAG_BIT_UNTOUCHABLE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
    windowInfo.flags = WindowInfo::FLAG_BIT_HANDWRITING;
    pointerEvent->HasFlag(InputEvent::EVENT_FLAG_SIMULATE);
    inputWindowsManager.extraData_.appended = true;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_PULL_UP;
    windowInfo.pointerHotAreas.push_back({150, 250, 300, 500});
    windowInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
    inputWindowsManager.extraData_.appended = false;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_PULL_IN_WINDOW;
    pointerItem.targetWindowId_ = -2;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_PULL_UP;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_AXIS_BEGIN;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_AXIS_UPDATE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_AXIS_END;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerItem.targetWindowId_ = 10;
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    EXPECT_NE(inputEvent, nullptr);
    inputEvent->targetDisplayId_ = 11;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_BUTTON_DOWN;
    inputWindowsManager.firstBtnDownWindowInfo_.first = 1;
    inputEvent->targetDisplayId_ = 1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent));
}

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_CROWN)
/**
 * @tc.name: InputWindowsManagerTest_UpdateCrownTarget_001
 * @tc.desc: Test the funcation UpdateCrownTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateCrownTarget_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    EXPECT_NE(inputEvent, nullptr);
    inputEvent->targetDisplayId_ = -1;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.focusWindowId = -1;
    }
    int32_t ret = inputWindowsManager.UpdateCrownTarget(pointerEvent);
    EXPECT_NE(ret, RET_OK);
    inputEvent->targetDisplayId_ = 5;
    ret = inputWindowsManager.UpdateCrownTarget(pointerEvent);
    EXPECT_NE(ret, RET_OK);
    it->second.focusWindowId = 5;
    ret = inputWindowsManager.UpdateCrownTarget(pointerEvent);
    EXPECT_NE(ret, RET_OK);
}
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_CROWN

/**
 * @tc.name: InputWindowsManagerTest_PrintChangedWindowByEvent_001
 * @tc.desc: Test the funcation PrintChangedWindowByEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintChangedWindowByEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t eventType = 1;
    WindowInfo newWindowInfo;
    newWindowInfo.id = 6;
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 5;
    windowInfo.uid = 1;
    inputWindowsManager.lastMatchedWindow_.insert(std::make_pair(1, windowInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintChangedWindowByEvent(eventType, newWindowInfo));
    eventType = 10;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintChangedWindowByEvent(eventType, newWindowInfo));
    newWindowInfo.id = 1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintChangedWindowByEvent(eventType, newWindowInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_TransformDisplayXY_001
 * @tc.desc: Test the TransformDisplayXY function of the input window manager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TransformDisplayXY_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo info;
    double logicX = 10.0;
    double logicY = 20.0;
    std::pair<double, double> result = WIN_MGR->TransformDisplayXY(info, logicX, logicY);
    double ret = result.first;
    EXPECT_EQ(ret, logicX);
    double ret1 = result.second;
    EXPECT_EQ(ret1, logicY);
}

/**
 * @tc.name: InputWindowsManagerTest_TransformDisplayXY_002
 * @tc.desc: Test the TransformDisplayXY function of the input window manager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TransformDisplayXY_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo info;
    std::vector<float> transform = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    info.transform = transform;
    double logicX = 10.0;
    double logicY = 20.0;
    std::pair<double, double> result = WIN_MGR->TransformDisplayXY(info, logicX, logicY);
    double ret = result.first;
    EXPECT_EQ(ret, logicX);
    double ret1 = result.second;
    EXPECT_EQ(ret1, logicY);
}

/**
 * @tc.name: InputWindowsManagerTest_TransformDisplayXY_003
 * @tc.desc: Test the TransformDisplayXY function of the input window manager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TransformDisplayXY_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo info;
    std::vector<float> transform = {0.5, 0.0, 0.0, 0.0, 0.5, 0.0, 315.0, 690.0, 1.0};
    info.transform = transform;
    double logicX = 10.0;
    double logicY = 20.0;
    std::pair<double, double> result = WIN_MGR->TransformDisplayXY(info, logicX, logicY);
    double ret = result.first;
    EXPECT_EQ(ret, 320);
    double ret1 = result.second;
    EXPECT_EQ(ret1, 700);
}

/**
 * @tc.name: InputWindowsManagerTest_IsValidNavigationWindow_001
 * @tc.desc: Test IsValidNavigationWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsValidNavigationWindow_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    double x = 10.0;
    double y = 20.0;
    Rect rect = {0, 0, 30, 40};
    windowInfo.defaultHotAreas.push_back(rect);
    EXPECT_TRUE(WIN_MGR->IsValidNavigationWindow(windowInfo, x, y));
}

/**
 * @tc.name: InputWindowsManagerTest_IsValidNavigationWindow_002
 * @tc.desc: Test IsValidNavigationWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsValidNavigationWindow_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.windowInputType = WindowInputType::MIX_BUTTOM_ANTI_AXIS_MOVE;
    double x = 10.0;
    double y = 20.0;
    Rect rect = {0, 0, 30, 40};
    windowInfo.defaultHotAreas.push_back(rect);
    EXPECT_TRUE(WIN_MGR->IsValidNavigationWindow(windowInfo, x, y));
}

/**
 * @tc.name: InputWindowsManagerTest_IsValidNavigationWindow_003
 * @tc.desc: Test IsValidNavigationWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsValidNavigationWindow_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.windowInputType = WindowInputType::NORMAL;
    double x = 10.0;
    double y = 20.0;
    Rect rect = {0, 0, 30, 40};
    windowInfo.defaultHotAreas.push_back(rect);
    EXPECT_FALSE(WIN_MGR->IsValidNavigationWindow(windowInfo, x, y));
}

/**
 * @tc.name: InputWindowsManagerTest_IsValidNavigationWindow_004
 * @tc.desc: Test IsValidNavigationWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsValidNavigationWindow_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    double x = -10.0;
    double y = 20.0;
    Rect rect = {0, 0, 30, 40};
    windowInfo.defaultHotAreas.push_back(rect);
    EXPECT_FALSE(WIN_MGR->IsValidNavigationWindow(windowInfo, x, y));
}

/**
 * @tc.name: InputWindowsManagerTest_IsValidNavigationWindow_005
 * @tc.desc: Test IsValidNavigationWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsValidNavigationWindow_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.windowInputType = WindowInputType::TRANSMIT_ALL;
    double x = 10.0;
    double y = -20.0;
    Rect rect = {0, 0, 30, 40};
    windowInfo.defaultHotAreas.push_back(rect);
    EXPECT_FALSE(WIN_MGR->IsValidNavigationWindow(windowInfo, x, y));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTransformDisplayXY_001
 * @tc.desc: Test UpdateTransformDisplayXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTransformDisplayXY_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDisplayXPos(10);
    item.SetDisplayYPos(20);
    pointerEvent->UpdatePointerItem(0, item);
    std::vector<WindowInfo> windowsInfo;
    OLD::DisplayInfo info;
    WIN_MGR->UpdateTransformDisplayXY(pointerEvent, windowsInfo, info);
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
        MMI_HILOG_DISPATCHE("Can't find pointer item, pointer:%{public}d", pointerId);
        return;
    }
    int32_t physicalX = pointerItem.GetDisplayX();
    int32_t physicalY = pointerItem.GetDisplayX();
    EXPECT_EQ(physicalX, 10);
    EXPECT_EQ(physicalY, 20);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTransformDisplayXY_002
 * @tc.desc: Test UpdateTransformDisplayXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTransformDisplayXY_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDisplayXPos(10);
    item.SetDisplayYPos(20);
    pointerEvent->UpdatePointerItem(0, item);
    std::vector<WindowInfo> windowsInfo;
    WindowInfo windowInfo;
    OLD::DisplayInfo info;
    std::vector<float> transform = {0.5, 0.0, 0.0, 0.0, 0.5, 0.0, 315.0, 690.0, 1.0};
    info.transform = transform;
    windowInfo.windowInputType = WindowInputType::MIX_BUTTOM_ANTI_AXIS_MOVE;
    Rect rect = {0, 0, 30, 40};
    windowInfo.defaultHotAreas.push_back(rect);
    windowsInfo.push_back(windowInfo);
    WIN_MGR->UpdateTransformDisplayXY(pointerEvent, windowsInfo, info);
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
        MMI_HILOG_DISPATCHE("Can't find pointer item, pointer:%{public}d", pointerId);
        return;
    }
    int32_t physicalX = pointerItem.GetDisplayX();
    int32_t physicalY = pointerItem.GetDisplayX();
    EXPECT_EQ(physicalX, 10);
    EXPECT_EQ(physicalY, 20);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTransformDisplayXY_003
 * @tc.desc: Test UpdateTransformDisplayXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTransformDisplayXY_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDisplayXPos(10);
    item.SetDisplayYPos(20);
    pointerEvent->UpdatePointerItem(0, item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    std::vector<WindowInfo> windowsInfo;
    WindowInfo windowInfo;
    OLD::DisplayInfo info;
    std::vector<float> transform = {0.5, 0.0, 0.0, 0.0, 0.5, 0.0, 315.0, 690.0, 1.0};
    info.transform = transform;
    windowInfo.windowInputType = WindowInputType::TRANSMIT_ALL;
    Rect rect = {0, 0, 30, 40};
    windowInfo.defaultHotAreas.push_back(rect);
    windowsInfo.push_back(windowInfo);
    WIN_MGR->UpdateTransformDisplayXY(pointerEvent, windowsInfo, info);
    int32_t pointerId = pointerEvent->GetPointerId();
    PointerEvent::PointerItem pointerItem;
    if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) {
        MMI_HILOG_DISPATCHE("Can't find pointer item, pointer:%{public}d", pointerId);
        return;
    }
    int32_t physicalX = pointerItem.GetDisplayX();
    int32_t physicalY = pointerItem.GetDisplayX();
    EXPECT_EQ(physicalX, 320);
    EXPECT_EQ(physicalY, 700);
}

/**
 * @tc.name: InputWindowsManagerTest_GetUIExtentionWindowInfo
 * @tc.desc: Test the funcation GetUIExtentionWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetUIExtentionWindowInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWinMgr;
    WindowInfo windowInfo;
    windowInfo.id = 5;
    std::vector<WindowInfo> uiExtentionWindowInfo;
    int32_t windowId = 10;
    WindowInfo *touchWindow;
    bool isUiExtentionWindow = false;
    uiExtentionWindowInfo.push_back(windowInfo);
    windowInfo.id = 10;
    uiExtentionWindowInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(
        inputWinMgr.GetUIExtentionWindowInfo(uiExtentionWindowInfo, windowId, &touchWindow, isUiExtentionWindow));
}

/**
 * @tc.name: InputWindowsManagerTest_CheckUIExtentionWindowDefaultHotArea
 * @tc.desc: Test the funcation CheckUIExtentionWindowDefaultHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckUIExtentionWindowDefaultHotArea, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWinMgr;
    int32_t logicalX = 150;
    int32_t logicalY = 150;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    std::vector<WindowInfo> windowInfos;
    bool isHotArea = true;
    std::pair<int32_t, int32_t> logicalXY(std::make_pair(logicalX, logicalY));
    WindowInfo windowInfo;
    Rect rect;
    rect.x = INT32_MAX;
    rect.width = 100;
    windowInfo.defaultHotAreas.push_back(rect);
    windowInfos.push_back(windowInfo);
    WindowInfo touchWindow;
    touchWindow.id = 100;
    const WindowInfo *touchWindowInfo = &touchWindow;
    EXPECT_NO_FATAL_FAILURE(inputWinMgr.CheckUIExtentionWindowDefaultHotArea(
        logicalXY, isHotArea, pointerEvent, windowInfos, &touchWindowInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_CheckUIExtentionWindowDefaultHotArea_001
 * @tc.desc: Test the funcation CheckUIExtentionWindowDefaultHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckUIExtentionWindowDefaultHotArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWinMgr;
    int32_t logicalX = 150;
    int32_t logicalY = 150;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    std::vector<WindowInfo> windowInfos;
    bool isHotArea = true;
    std::pair<int32_t, int32_t> logicalXY(std::make_pair(logicalX, logicalY));
    WindowInfo windowInfo;
    Rect rect;
    rect.x = 100;
    rect.y = 100;
    rect.width = 200;
    rect.height = 200;
    windowInfo.defaultHotAreas.push_back(rect);
    windowInfos.push_back(windowInfo);
    WindowInfo touchWindow;
    touchWindow.id = 100;
    const WindowInfo *touchWindowInfo = &touchWindow;
    EXPECT_NO_FATAL_FAILURE(inputWinMgr.CheckUIExtentionWindowDefaultHotArea(
        logicalXY, isHotArea, pointerEvent, windowInfos, &touchWindowInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_IsKeyPressed
 * @tc.desc: Test the funcation IsKeyPressed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsKeyPressed, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWinMgr;
    KeyEvent::KeyItem item;
    int32_t pressedKey = 2024;
    std::vector<KeyEvent::KeyItem> keyItems;
    item.SetKeyCode(2018);
    item.SetPressed(false);
    keyItems.push_back(item);
    EXPECT_FALSE(inputWinMgr.IsKeyPressed(pressedKey, keyItems));
}

/**
 * @tc.name: InputWindowsManagerTest_IsKeyPressed_001
 * @tc.desc: Test the funcation IsKeyPressed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsKeyPressed_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWinMgr;
    KeyEvent::KeyItem item;
    int32_t pressedKey = 2024;
    std::vector<KeyEvent::KeyItem> keyItems;
    item.SetKeyCode(2024);
    item.SetPressed(true);
    keyItems.push_back(item);
    EXPECT_TRUE(inputWinMgr.IsKeyPressed(pressedKey, keyItems));
}

/**
 * @tc.name: InputWindowsManagerTest_IsOnTheWhitelist
 * @tc.desc: Test the funcation IsOnTheWhitelist
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsOnTheWhitelist, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWinMgr;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    KeyEvent::KeyItem item;
    SwitchFocusKey switchFocusKey;
    switchFocusKey.keyCode = 2024;
    switchFocusKey.pressedKey = -1;
    inputWinMgr.vecWhiteList_.push_back(switchFocusKey);
    keyEvent->SetKeyCode(2024);
    item.SetPressed(true);
    item.SetKeyCode(2024);
    keyEvent->AddKeyItem(item);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    EXPECT_TRUE(inputWinMgr.IsOnTheWhitelist(keyEvent));

    inputWinMgr.vecWhiteList_[0].pressedKey = 2024;
    keyEvent->AddKeyItem(item);
    EXPECT_TRUE(inputWinMgr.IsOnTheWhitelist(keyEvent));

    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_UP);
    EXPECT_FALSE(inputWinMgr.IsOnTheWhitelist(keyEvent));

    keyEvent->SetKeyCode(2018);
    EXPECT_FALSE(inputWinMgr.IsOnTheWhitelist(keyEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_IsOnTheWhitelist_001
 * @tc.desc: Test IsOnTheWhitelist
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsOnTheWhitelist_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WIN_MGR->vecWhiteList_ = {{1}, {2}, {3}};
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetKeyCode(4);
    bool ret = WIN_MGR->IsOnTheWhitelist(keyEvent);
    ASSERT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_IsOnTheWhitelist_002
 * @tc.desc: Test IsOnTheWhitelist
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsOnTheWhitelist_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputManager;
    SwitchFocusKey whitelistItem;
    whitelistItem.keyCode = 1;
    whitelistItem.pressedKey = -1;
    inputManager.vecWhiteList_.push_back(whitelistItem);
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetKeyCode(1);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    KeyEvent::KeyItem item;
    item.SetKeyCode(KeyEvent::KEYCODE_HOME);
    keyEvent->AddKeyItem(item);
    bool ret = inputManager.IsOnTheWhitelist(keyEvent);
    ASSERT_TRUE(ret);
    whitelistItem.pressedKey = 2;
    ret = inputManager.IsOnTheWhitelist(keyEvent);
    ASSERT_TRUE(ret);
    whitelistItem.pressedKey = -1;
    item.SetDeviceId(100);
    item.SetKeyCode(KeyEvent::KEYCODE_PAGE_DOWN);
    item.SetDownTime(100);
    keyEvent->AddKeyItem(item);
    ret = inputManager.IsOnTheWhitelist(keyEvent);
    ASSERT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_IsOnTheWhitelist_003
 * @tc.desc: Test IsOnTheWhitelist
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsOnTheWhitelist_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputManager;
    SwitchFocusKey whitelistItem;
    whitelistItem.keyCode = 1;
    whitelistItem.pressedKey = 1;
    inputManager.vecWhiteList_.push_back(whitelistItem);
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetKeyCode(1);
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    KeyEvent::KeyItem item;
    item.SetDeviceId(100);
    item.SetDownTime(100);
    keyEvent->AddKeyItem(item);
    item.pressed_ = true;
    bool ret = WIN_MGR->IsOnTheWhitelist(keyEvent);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_IsKeyPressed_002
 * @tc.desc: Test IsKeyPressed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsKeyPressed_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KeyEvent::KeyItem item;
    item.SetKeyCode(1);
    std::vector<KeyEvent::KeyItem> keyItems;
    keyItems.push_back(item);
    bool ret = WIN_MGR->IsKeyPressed(1, keyItems);
    ASSERT_FALSE(ret);
    ret = WIN_MGR->IsKeyPressed(2, keyItems);
    ASSERT_FALSE(ret);
    item.pressed_ = true;
    keyItems.push_back(item);
    ret = WIN_MGR->IsKeyPressed(1, keyItems);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_CheckUIExtentionWindowPointerHotArea_001
 * @tc.desc: Test CheckUIExtentionWindowPointerHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckUIExtentionWindowPointerHotArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.windowType = 2105;
    windowInfo.area.x = 10;
    windowInfo.area.y = 80;
    windowInfo.area.height = 90;
    std::vector<WindowInfo> windows;
    windows.push_back(windowInfo);
    int32_t windowId = 1;
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->CheckUIExtentionWindowPointerHotArea(15, 20, windows, windowId));
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->CheckUIExtentionWindowPointerHotArea(100, 200, windows, windowId));
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->CheckUIExtentionWindowPointerHotArea(100, 200, {}, windowId));
}

/**
 * @tc.name: InputWindowsManagerTest_GetUIExtentionWindowInfo_001
 * @tc.desc: Test GetUIExtentionWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetUIExtentionWindowInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    std::vector<WindowInfo> windows = {{1}, {2}, {3}};
    WindowInfo *touchWindow = nullptr;
    bool isUiExtentionWindow = false;
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->GetUIExtentionWindowInfo(windows, 2, &touchWindow, isUiExtentionWindow));
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->GetUIExtentionWindowInfo(windows, 4, &touchWindow, isUiExtentionWindow));
    std::vector<WindowInfo> emptyWindows;
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->GetUIExtentionWindowInfo(emptyWindows, 1, &touchWindow, isUiExtentionWindow));
}

/**
 * @tc.name: InputWindowsManagerTest_CheckUIExtentionWindowDefaultHotArea_002
 * @tc.desc: Test CheckUIExtentionWindowDefaultHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckUIExtentionWindowDefaultHotArea_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerId(0);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    bool isHotArea = true;
    InputWindowsManager manager;
    WindowInfo windowInfo;
    windowInfo.windowType = 2105;
    windowInfo.area.x = 10;
    windowInfo.area.y = 100;
    windowInfo.area.height = 200;
    std::vector<WindowInfo> windows;
    windows.push_back(windowInfo);
    WindowInfo touchWindow;
    touchWindow.id = 100;
    const WindowInfo *touchWindowInfo = &touchWindow;
    std::pair<int32_t, int32_t> logicalXY(std::make_pair(15, 25));
    EXPECT_NO_FATAL_FAILURE(
        WIN_MGR->CheckUIExtentionWindowDefaultHotArea(logicalXY, isHotArea, pointerEvent, windows, &touchWindowInfo));
    std::pair<int32_t, int32_t> secondXY(std::make_pair(300, 300));
    EXPECT_NO_FATAL_FAILURE(
        WIN_MGR->CheckUIExtentionWindowDefaultHotArea(secondXY, isHotArea, pointerEvent, windows, &touchWindowInfo));
    EXPECT_NO_FATAL_FAILURE(
        WIN_MGR->CheckUIExtentionWindowDefaultHotArea(logicalXY, isHotArea, pointerEvent, {}, &touchWindowInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_003
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    Rect rect;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(200);
    item.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    rect.x = 300;
    rect.width = 1200;
    rect.y = 300;
    rect.height = 1200;
    for (int32_t i = 0; i < 4; ++i) {
        winInfo.defaultHotAreas.push_back(rect);
    }
    winInfo.id = 0;
    winInfo.flags = 6;
    winInfo.pixelMap = nullptr;
    winGroupInfo.windowsInfo.push_back(winInfo);
    winInfo.id = 1;
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.windowInputType = WindowInputType::NORMAL;
    winInfo.transform.clear();
    winInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    auto pointerId = 150;
    WindowInfoEX winEx;
    winEx.flag = true;
    winEx.window = winInfo;
    inputWindowsMgr.touchItemDownInfos_.insert(std::make_pair(pointerId, winEx));
    auto iter = inputWindowsMgr.touchItemDownInfos_.find(pointerId);
    if (iter != inputWindowsMgr.touchItemDownInfos_.end()) {
        iter->second.window.transform.clear();
        iter->second.window.transform = winInfo.transform;
    }
    winInfo.defaultHotAreas.push_back(rect);
    winGroupInfo.windowsInfo.push_back(winInfo);
    inputWindowsMgr.extraData_.appended = true;
    inputWindowsMgr.extraData_.pointerId = 150;
    inputWindowsMgr.extraData_.sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_004
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    Rect rect;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(-1);
    item.SetToolType(PointerEvent::TOOL_TYPE_RUBBER);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    rect.x = 300;
    rect.width = INT32_MAX;
    winInfo.id = 50;
    winInfo.defaultHotAreas.push_back(rect);
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    rect.x = 300;
    rect.width = 1200;
    rect.y = 300;
    rect.height = 1200;
    winInfo.id = 1;
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.windowInputType = WindowInputType::TRANSMIT_ALL;
    winInfo.defaultHotAreas.clear();
    winInfo.defaultHotAreas.push_back(rect);
    winInfo.transform.clear();
    winInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    auto pointerId = 150;
    WindowInfoEX winEx;
    winEx.flag = true;
    winEx.window = winInfo;
    inputWindowsMgr.touchItemDownInfos_.insert(std::make_pair(pointerId, winEx));
    auto iter = inputWindowsMgr.touchItemDownInfos_.find(pointerId);
    if (iter != inputWindowsMgr.touchItemDownInfos_.end()) {
        iter->second.window.transform.clear();
        iter->second.window.transform = winInfo.transform;
    }
    winGroupInfo.windowsInfo.push_back(winInfo);
    inputWindowsMgr.extraData_.appended = false;
    inputWindowsMgr.extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_IgnoreTouchEvent_001
 * @tc.desc: Test IgnoreTouchEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IgnoreTouchEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->IgnoreTouchEvent(pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->IgnoreTouchEvent(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_ReissueCancelTouchEvent_001
 * @tc.desc: Test ReissueCancelTouchEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReissueCancelTouchEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->ReissueCancelTouchEvent(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_002
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(200);
    item.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    winInfo.flags = 1;
    winGroupInfo.windowsInfo.push_back(winInfo);
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    winGroupInfo.windowsInfo.push_back(winInfo);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    inputWindowsMgr.isOpenAntiMisTakeObserver_ = false;
    inputWindowsMgr.antiMistake_.isOpen = true;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_005
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(200);
    item.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    winInfo.defaultHotAreas = {
        {300, 300, 1000, 1000}
    };
    winGroupInfo.windowsInfo.push_back(winInfo);
    winInfo.defaultHotAreas.clear();
    winInfo.defaultHotAreas = {
        {300, 300, INT32_MAX, INT32_MAX}
    };
    winGroupInfo.windowsInfo.push_back(winInfo);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    inputWindowsMgr.extraData_.appended = true;
    inputWindowsMgr.extraData_.sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    inputWindowsMgr.isOpenAntiMisTakeObserver_ = true;
    inputWindowsMgr.antiMistake_.isOpen = false;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_008
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(-1);
    item.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.windowInputType = WindowInputType::NORMAL;
    winInfo.defaultHotAreas = { {300, 300, 1000, 1000} };
    winInfo.id = 100;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.transform.clear();
    winInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    auto pointerId = 150;
    WindowInfoEX winEx;
    winEx.flag = true;
    winEx.window = winInfo;
    inputWindowsMgr.touchItemDownInfos_.insert(std::make_pair(pointerId, winEx));
    auto iter = inputWindowsMgr.touchItemDownInfos_.find(pointerId);
    if (iter != inputWindowsMgr.touchItemDownInfos_.end()) {
        iter->second.window.transform.clear();
        iter->second.window.transform = winInfo.transform;
    }
    winGroupInfo.windowsInfo.push_back(winInfo);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    inputWindowsMgr.extraData_.appended = false;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_009
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(-1);
    item.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.windowInputType = WindowInputType::TRANSMIT_ALL;
    winInfo.defaultHotAreas = {
        {300, 300, 1000, 1000}
    };
    winInfo.id = -1;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.transform.clear();
    winInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    auto pointerId = 150;
    WindowInfoEX winEx;
    winEx.flag = true;
    winEx.window = winInfo;
    inputWindowsMgr.touchItemDownInfos_.insert(std::make_pair(pointerId, winEx));
    auto iter = inputWindowsMgr.touchItemDownInfos_.find(pointerId);
    if (iter != inputWindowsMgr.touchItemDownInfos_.end()) {
        iter->second.window.transform.clear();
        iter->second.window.transform = winInfo.transform;
    }
    winGroupInfo.windowsInfo.push_back(winInfo);
    winInfo.windowInputType = WindowInputType::NORMAL;
    winGroupInfo.windowsInfo.push_back(winInfo);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    inputWindowsMgr.extraData_.appended = false;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_010
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_010, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(-1);
    item.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.windowInputType = WindowInputType::NORMAL;
    winInfo.defaultHotAreas = {{300, 300, 1000, 1000}};
    winInfo.id = -1;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.transform.clear();
    winInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    auto pointerId = 150;
    WindowInfoEX winEx;
    winEx.flag = true;
    winEx.window = winInfo;
    inputWindowsMgr.touchItemDownInfos_.insert(std::make_pair(pointerId, winEx));
    auto iter = inputWindowsMgr.touchItemDownInfos_.find(pointerId);
    if (iter != inputWindowsMgr.touchItemDownInfos_.end()) {
        iter->second.window.transform.clear();
        iter->second.window.transform = winInfo.transform;
    }
    winGroupInfo.windowsInfo.push_back(winInfo);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    inputWindowsMgr.extraData_.appended = false;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_011
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_011, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    inputWindowsMgr.lastTouchEventOnBackGesture_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsMgr.lastTouchEventOnBackGesture_, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(200);
    item.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.id = 200;
    winInfo.defaultHotAreas = {
        {300, 300, 1000, 1000}
    };
    winInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.transform.clear();
    winInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    auto pointerId = 150;
    WindowInfoEX winEx;
    winEx.flag = true;
    winEx.window = winInfo;
    inputWindowsMgr.touchItemDownInfos_.insert(std::make_pair(pointerId, winEx));
    auto iter = inputWindowsMgr.touchItemDownInfos_.find(pointerId);
    if (iter != inputWindowsMgr.touchItemDownInfos_.end()) {
        iter->second.window.transform.clear();
        iter->second.window.transform = winInfo.transform;
    }
    winGroupInfo.windowsInfo.push_back(winInfo);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    inputWindowsMgr.extraData_.appended = false;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_012
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_012, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    inputWindowsMgr.lastTouchEventOnBackGesture_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsMgr.lastTouchEventOnBackGesture_, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(200);
    item.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.id = 200;
    winInfo.defaultHotAreas = {
        {300, 300, 1000, 1000}
    };
    winInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.transform.clear();
    winInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    auto pointerId = 150;
    WindowInfoEX winEx;
    winEx.flag = true;
    winEx.window = winInfo;
    inputWindowsMgr.touchItemDownInfos_.insert(std::make_pair(pointerId, winEx));
    auto iter = inputWindowsMgr.touchItemDownInfos_.find(pointerId);
    if (iter != inputWindowsMgr.touchItemDownInfos_.end()) {
        iter->second.window.transform.clear();
        iter->second.window.transform = winInfo.transform;
    }
    winGroupInfo.windowsInfo.push_back(winInfo);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    inputWindowsMgr.extraData_.appended = false;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_013
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_013, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    inputWindowsMgr.lastTouchEventOnBackGesture_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsMgr.lastTouchEventOnBackGesture_, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(200);
    item.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.id = 200;
    winInfo.defaultHotAreas = {
        {300, 300, 1000, 1000}
    };
    winInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.transform.clear();
    winInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    auto pointerId = 150;
    WindowInfoEX winEx;
    winEx.flag = true;
    winEx.window = winInfo;
    inputWindowsMgr.touchItemDownInfos_.insert(std::make_pair(pointerId, winEx));
    auto iter = inputWindowsMgr.touchItemDownInfos_.find(pointerId);
    if (iter != inputWindowsMgr.touchItemDownInfos_.end()) {
        iter->second.window.transform.clear();
        iter->second.window.transform = winInfo.transform;
    }
    winGroupInfo.windowsInfo.push_back(winInfo);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    inputWindowsMgr.extraData_.appended = false;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_014
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_014, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    inputWindowsMgr.lastTouchEventOnBackGesture_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsMgr.lastTouchEventOnBackGesture_, nullptr);
    OLD::DisplayInfo displayInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    PointerEvent::PointerItem item;
    displayInfo.id = 100;
    displayInfo.x = 500;
    displayInfo.y = 500;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetPointerId(150);
    item.SetPointerId(150);
    item.SetDisplayXPos(500);
    item.SetDisplayYPos(500);
    item.SetTargetWindowId(200);
    item.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->bitwise_ = 0x00000000;
    winInfo.flags = 0;
    winInfo.pixelMap = nullptr;
    winInfo.id = 200;
    winInfo.defaultHotAreas = {
        {300, 300, 1000, 1000}
    };
    winInfo.windowInputType = WindowInputType::NORMAL;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.transform.clear();
    winInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    auto pointerId = 150;
    WindowInfoEX winEx;
    winEx.flag = true;
    winEx.window = winInfo;
    inputWindowsMgr.touchItemDownInfos_.insert(std::make_pair(pointerId, winEx));
    auto iter = inputWindowsMgr.touchItemDownInfos_.find(pointerId);
    if (iter != inputWindowsMgr.touchItemDownInfos_.end()) {
        iter->second.window.transform.clear();
        iter->second.window.transform = winInfo.transform;
    }
    winGroupInfo.windowsInfo.push_back(winInfo);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    inputWindowsMgr.extraData_.appended = false;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    inputWindowsMgr.windowsPerDisplay_.insert(std::make_pair(100, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTouchScreenTarget_015
 * @tc.desc: This test verifies the functionality of updating the touch screen target
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTouchScreenTarget_015, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);

    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));

    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));

    pointerEvent->RemovePointerItem(item.GetPointerId());
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTouchScreenTarget(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_SendCancelEventWhenLock_001
 * @tc.desc: Test the funcation SendCancelEventWhenLock
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SendCancelEventWhenLock_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    inputWindowsMgr.lastTouchEventOnBackGesture_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsMgr.lastTouchEventOnBackGesture_, nullptr);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SendCancelEventWhenLock());
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SendCancelEventWhenLock());
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SendCancelEventWhenLock());
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SendCancelEventWhenLock());
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfoEX windowInfoEX;
    windowInfoEX.flag = true;
    pointerEvent->SetPointerId(1);
    inputWindowsMgr.touchItemDownInfos_.insert(std::make_pair(pointerEvent->GetPointerId(), windowInfoEX));
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerId(2);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SendCancelEventWhenLock());
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SendCancelEventWhenLock());
}

/**
 * @tc.name: InputWindowsManagerTest_DrawTouchGraphic_001
 * @tc.desc: Test the funcation DrawTouchGraphic
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DrawTouchGraphic_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.DrawTouchGraphic(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_IsMouseDrawing_001
 * @tc.desc: Test the funcation IsMouseDrawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsMouseDrawing_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t currentAction = 10;
    bool ret = inputWindowsMgr.IsMouseDrawing(currentAction);
    ASSERT_FALSE(ret);
    currentAction = 11;
    ret = inputWindowsMgr.IsMouseDrawing(currentAction);
    ASSERT_FALSE(ret);
    currentAction = 15;
    ret = inputWindowsMgr.IsMouseDrawing(currentAction);
    ASSERT_FALSE(ret);
    currentAction = 16;
    ret = inputWindowsMgr.IsMouseDrawing(currentAction);
    ASSERT_FALSE(ret);
    currentAction = 1;
    ret = inputWindowsMgr.IsMouseDrawing(currentAction);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_GetTargetWindowIds_002
 * @tc.desc: Test the funcation GetTargetWindowIds
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetTargetWindowIds_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t pointerItemId = 1;
    int32_t sourceType = 1;
    std::set<int32_t> windowIds {1, 2, 3};
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetTargetWindowIds(pointerItemId, sourceType, windowIds, 1));
    inputWindowsMgr.targetMouseWinIds_.insert(std::make_pair(1, windowIds));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetTargetWindowIds(pointerItemId, sourceType, windowIds, 1));
    sourceType = 2;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetTargetWindowIds(pointerItemId, sourceType, windowIds, 1));
    pointerItemId = 5;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetTargetWindowIds(pointerItemId, sourceType, windowIds, 1));
    inputWindowsMgr.targetMouseWinIds_.insert(std::make_pair(5, windowIds));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetTargetWindowIds(pointerItemId, sourceType, windowIds, 1));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTarget
 * @tc.desc: Test UpdateTouchScreenTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTarget, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<KeyEvent> keyEvent = nullptr;
    inputWindowsMgr.isParseConfig_ = true;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateTarget(keyEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_HandleKeyEventWindowId
 * @tc.desc: Test HandleKeyEventWindowId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandleKeyEventWindowId, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    WindowInfo winInfo;
    keyEvent->SetTargetDisplayId(-1);
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.focusWindowId = 50;
    }
    winInfo.id = 50;
    winInfo.agentWindowId = 100;
    winInfo.privacyMode = SecureFlag::PRIVACY_MODE;
    it->second.windowsInfo.push_back(winInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.HandleKeyEventWindowId(keyEvent));

    it->second.windowsInfo[0].privacyMode = SecureFlag::DEFAULT_MODE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.HandleKeyEventWindowId(keyEvent));

    it->second.focusWindowId = 80;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.HandleKeyEventWindowId(keyEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetDisplayId
 * @tc.desc: Test GetDisplayId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetDisplayId, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    ASSERT_NE(inputEvent, nullptr);
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 100;
    inputEvent->SetTargetDisplayId(-1);
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetDisplayId(inputEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetClientFd_004
 * @tc.desc: Test GetClientFd
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetClientFd_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t windowId = 10;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfo winInfo;
    UDSServer udsServer;
    inputWindowsMgr.udsServer_ = &udsServer;
    pointerEvent->SetTargetDisplayId(-1);
    winInfo.id = 20;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.id = 10;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.pid = 50;
    winInfo.flags = 15;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(winInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetClientFd(pointerEvent, windowId));
    windowId = 100;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetClientFd(pointerEvent, windowId));
}

/**
 * @tc.name: InputWindowsManagerTest_GetPidAndUpdateTarget
 * @tc.desc: Test GetPidAndUpdateTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPidAndUpdateTarget, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<KeyEvent> keyEvent = nullptr;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetPidAndUpdateTarget(keyEvent));

    keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    WindowInfo winInfo;
    winInfo.id = 10;
    winInfo.privacyUIFlag = true;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    keyEvent->SetTargetDisplayId(-1);
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.focusWindowId = 10;
        it->second.windowsInfo.push_back(winInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetPidAndUpdateTarget(keyEvent));

    it->second.windowsInfo[0].uiExtentionWindowInfo[0].privacyUIFlag = false;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetPidAndUpdateTarget(keyEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfoExtIfNeed
 * @tc.desc: Test UpdateDisplayInfoExtIfNeed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfoExtIfNeed, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    OLD::DisplayGroupInfo displayGroupInfo;
    bool needUpdateDisplayExt = true;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.UpdateDisplayInfoExtIfNeed(displayGroupInfo, needUpdateDisplayExt));
}

/**
 * @tc.name: InputWindowsManagerTest_GetPointerStyle_002
 * @tc.desc: Test GetPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPointerStyle_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t pid = 100;
    int32_t windowId = 200;
    PointerStyle pointerStyle;
    bool isUiExtension = false;
    std::map<int32_t, PointerStyle> pointerStyleMap;
    pointerStyleMap.insert(std::make_pair(windowId, pointerStyle));
    inputWindowsMgr.pointerStyle_.insert(std::make_pair(pid, pointerStyleMap));
    EXPECT_EQ(inputWindowsMgr.GetPointerStyle(pid, windowId, pointerStyle, isUiExtension), RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_SelectPointerChangeArea
 * @tc.desc: Test SelectPointerChangeArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectPointerChangeArea, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    WindowInfo windowInfo;
    PointerStyle pointerStyle;
    int32_t logicalX = 300;
    int32_t logicalY = 300;
    std::vector<Rect> areas;
    Rect rect {
        .x = 100,
        .y = 100,
        .width = 1000,
        .height = 1000,
    };
    areas.push_back(rect);
    windowInfo.id = 100;
    inputWindowsMgr.windowsHotAreas_.insert(std::make_pair(100, areas));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SelectPointerChangeArea(windowInfo, pointerStyle, logicalX, logicalY));
}

/**
 * @tc.name: InputWindowsManagerTest_DispatchUIExtentionPointerEvent
 * @tc.desc: Test DispatchUIExtentionPointerEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DispatchUIExtentionPointerEvent, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t logicalX = 300;
    int32_t logicalY = 300;
    WindowInfo winInfo;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetTargetWindowId(100);
    winInfo.id = 10;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.id = 100;
    winInfo.agentWindowId = 200;
    winInfo.uiExtentionWindowInfo.push_back(winInfo);
    winInfo.id = 300;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(winInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.DispatchUIExtentionPointerEvent(logicalX, logicalY, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_PullEnterLeaveEvent
 * @tc.desc: Test PullEnterLeaveEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PullEnterLeaveEvent, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t logicalX = 100;
    int32_t logicalY = 100;
    WindowInfo touchWindow;
    UDSServer udsServer;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    inputWindowsMgr.lastTouchEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsMgr.lastTouchEvent_, nullptr);
    PointerEvent::PointerItem lastPointerItem;
    touchWindow.id = 200;
    inputWindowsMgr.udsServer_ = &udsServer;
    inputWindowsMgr.lastTouchWindowInfo_.id = 100;
    inputWindowsMgr.lastTouchEvent_->SetPointerId(10);
    lastPointerItem.SetPointerId(10);
    inputWindowsMgr.lastTouchEvent_->AddPointerItem(lastPointerItem);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.PullEnterLeaveEvent(logicalX, logicalY, pointerEvent, &touchWindow));
}

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_CROWN)
/**
 * @tc.name: InputWindowsManagerTest_UpdateCrownTarget
 * @tc.desc: Test UpdateCrownTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateCrownTarget, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    WindowInfo winInfo;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetTargetDisplayId(-1);
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.focusWindowId = 100;
    }
    winInfo.id = 200;
    it->second.windowsInfo.push_back(winInfo);
    winInfo.id = 100;
    winInfo.agentWindowId = 500;
    winInfo.privacyMode = SecureFlag::DEFAULT_MODE;
    it->second.windowsInfo.push_back(winInfo);
    EXPECT_EQ(inputWindowsMgr.UpdateCrownTarget(pointerEvent), RET_OK);
}
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_CROWN

/**
 * @tc.name: InputWindowsManagerTest_DrawTouchGraphic
 * @tc.desc: Test DrawTouchGraphic
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DrawTouchGraphic, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetTargetDisplayId(100);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.DrawTouchGraphic(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_ReverseRotateScreen
 * @tc.desc: Test ReverseRotateScreen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReverseRotateScreen, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    OLD::DisplayInfo info;
    double x = 100.5;
    double y = 100.5;
    Coordinate2D cursorPos;
    info.direction = static_cast<Direction>(10);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.ReverseRotateScreen(info, x, y, cursorPos));
}

/**
 * @tc.name: InputWindowsManagerTest_GetTargetWindowIds_003
 * @tc.desc: Test GetTargetWindowIds
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetTargetWindowIds_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t pointerItemId = 100;
    int32_t sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    std::set<int32_t> windowIds;
    inputWindowsMgr.targetMouseWinIds_.insert(std::make_pair(10, windowIds));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetTargetWindowIds(pointerItemId, sourceType, windowIds, 1));
    inputWindowsMgr.targetMouseWinIds_.insert(std::make_pair(pointerItemId, windowIds));
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetTargetWindowIds(pointerItemId, sourceType, windowIds, 1));
    sourceType = PointerEvent::PointerEvent::SOURCE_TYPE_UNKNOWN;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.GetTargetWindowIds(pointerItemId, sourceType, windowIds, 1));
}

/**
 * @tc.name: InputWindowsManagerTest_SetPrivacyModeFlag
 * @tc.desc: Test SetPrivacyModeFlag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetPrivacyModeFlag, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    SecureFlag privacyMode = SecureFlag::PRIVACY_MODE;
    std::shared_ptr<InputEvent> event = InputEvent::Create();
    ASSERT_NE(event, nullptr);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SetPrivacyModeFlag(privacyMode, event));
}

/**
 * @tc.name: InputWindowsManagerTest_ReverseXY_001
 * @tc.desc: Test ReverseXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReverseXY_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t x = 100;
    int32_t y = 100;
    OLD::DisplayInfo displayInfo;
    displayInfo.direction = static_cast<Direction>(-1);
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
        EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.ReverseXY(x, y));
    }
    it->second.displaysInfo[0].direction = static_cast<Direction>(10);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.ReverseXY(x, y));
}

/**
 * @tc.name: InputWindowsManagerTest_SendCancelEventWhenLock_002
 * @tc.desc: Test SendCancelEventWhenLock
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SendCancelEventWhenLock_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    inputWindowsMgr.lastTouchEventOnBackGesture_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsMgr.lastTouchEventOnBackGesture_, nullptr);
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SendCancelEventWhenLock());
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SendCancelEventWhenLock());
    inputWindowsMgr.lastTouchEventOnBackGesture_->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SendCancelEventWhenLock());
}

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING

/**
 * @tc.name: InputWindowsManagerTest_DispatchPointerCancel
 * @tc.desc: Test DispatchPointerCancel
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DispatchPointerCancel, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t displayId = -1;
    WindowInfo winInfo;
    inputWindowsMgr.mouseDownInfo_.id = -1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.DispatchPointerCancel(displayId));
    inputWindowsMgr.mouseDownInfo_.id = 10;
    inputWindowsMgr.extraData_.appended = true;
    inputWindowsMgr.extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.DispatchPointerCancel(displayId));
    inputWindowsMgr.lastPointerEvent_ = nullptr;
    inputWindowsMgr.extraData_.appended = false;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.DispatchPointerCancel(displayId));
    inputWindowsMgr.extraData_.appended = true;
    inputWindowsMgr.extraData_.sourceType = PointerEvent::SOURCE_TYPE_UNKNOWN;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.DispatchPointerCancel(displayId));
    inputWindowsMgr.lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsMgr.lastPointerEvent_, nullptr);
    winInfo.id = 10;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(winInfo);
        EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.DispatchPointerCancel(displayId));
    }
    it->second.windowsInfo.clear();
    winInfo.id = 100;
    it->second.windowsInfo.push_back(winInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.DispatchPointerCancel(displayId));
}

#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

/**
 * @tc.name: InputWindowsManagerTest_GetPidByDisplayIdAndWindowId
 * @tc.desc: Test GetPidByDisplayIdAndWindowId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPidByDisplayIdAndWindowId, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t id = 100;
    WindowInfo winInfo;
    winInfo.id = 100;
    winInfo.pid = 150;
    WindowInfo extWinInfo;
    extWinInfo.id = 1000;
    extWinInfo.pid = 1500;
    WindowInfo exiWin = {.id = 400, .pid = 450};
    extWinInfo.uiExtentionWindowInfo.push_back(exiWin);
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(winInfo);
        it->second.windowsInfo.push_back(extWinInfo);
        it->second.mainDisplayId = 0;
    }
    EXPECT_EQ(inputWindowsMgr.GetPidByDisplayIdAndWindowId(0, id), winInfo.pid);
    EXPECT_EQ(inputWindowsMgr.GetPidByDisplayIdAndWindowId(0, 400), 450);
    id = 300;
    EXPECT_EQ(inputWindowsMgr.GetPidByDisplayIdAndWindowId(0, id), RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_GetMainDisplayId
 * @tc.desc: Test GetMainDisplayId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetMainDisplayId, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    WindowInfo winInfo;
    winInfo.id = 100;
    winInfo.pid = 150;
    auto it = inputWindowsMgr.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsMgr.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(winInfo);
        it->second.mainDisplayId = 100;
    }
    EXPECT_EQ(inputWindowsMgr.GetMainDisplayId(DEFAULT_GROUP_ID), 100);
    EXPECT_EQ(inputWindowsMgr.GetMainDisplayId(NA_GROUP_ID), 0);
}

/**
 * @tc.name: InputWindowsManagerTest_SetPixelMapData
 * @tc.desc: Test SetPixelMapData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetPixelMapData, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t infoId = -1;
    void *pixelMap = nullptr;
    EXPECT_EQ(inputWindowsMgr.SetPixelMapData(infoId, pixelMap), ERR_INVALID_VALUE);
    infoId = 100;
    EXPECT_EQ(inputWindowsMgr.SetPixelMapData(infoId, pixelMap), ERR_INVALID_VALUE);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfo_002
 * @tc.desc: Test updating window & display information for each display in extended screen mode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;

    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 1;
    windowInfo.uid = 1;
    windowInfo.area = {1, 1, 1, 1};
    windowInfo.defaultHotAreas = {windowInfo.area};
    windowInfo.pointerHotAreas = {windowInfo.area};
    windowInfo.agentWindowId = 1;
    windowInfo.flags = 1;
    windowInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    windowInfo.pointerChangeAreas = {1, 2, 1, 2, 1, 2, 1, 2, 1};
    displayGroupInfo.windowsInfo.push_back(windowInfo);

    OLD::DisplayInfo displayInfo1;
    displayInfo1.id = 1;
    displayInfo1.x = 1;
    displayInfo1.y = 1;
    displayInfo1.width = 2;
    displayInfo1.height = 2;
    displayInfo1.dpi = 240;
    displayInfo1.name = "pp";
    displayInfo1.uniq = "pp";
    displayInfo1.direction = DIRECTION0;
    displayGroupInfo.displaysInfo.push_back(displayInfo1);

    OLD::DisplayInfo displayInfo2;
    displayInfo2.id = 2;
    displayInfo2.x = 1;
    displayInfo2.y = 1;
    displayInfo2.width = 2;
    displayInfo2.height = 2;
    displayInfo2.dpi = 240;
    displayInfo2.name = "pp";
    displayInfo2.uniq = "pp";
    displayInfo2.direction = DIRECTION0;
    displayGroupInfo.displaysInfo.push_back(displayInfo2);

    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayInfo(displayGroupInfo));

    displayGroupInfo.displaysInfo.pop_back();
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfo_003
 * @tc.desc: Test updating window & display information for each display in extended screen mode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfo_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;

    OLD::DisplayInfo displayInfo1;
    displayInfo1.id = 1;
    displayInfo1.x = 1;
    displayInfo1.y = 1;
    displayInfo1.width = 2;
    displayInfo1.height = 2;
    displayInfo1.dpi = 240;
    displayInfo1.name = "pp";
    displayInfo1.uniq = "pp";
    displayInfo1.direction = DIRECTION0;
    displayGroupInfo.displaysInfo.push_back(displayInfo1);

    OLD::DisplayInfo displayInfo2;
    displayInfo2.id = 2;
    displayInfo2.x = 1;
    displayInfo2.y = 1;
    displayInfo2.width = 2;
    displayInfo2.height = 2;
    displayInfo2.dpi = 240;
    displayInfo2.name = "pp";
    displayInfo2.uniq = "pp";
    displayInfo2.direction = DIRECTION0;
    displayGroupInfo.displaysInfo.push_back(displayInfo2);

    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfo_004
 * @tc.desc: Test updating window & display information for each display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfo_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;

    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 1;
    windowInfo.uid = 1;
    windowInfo.area = {1, 1, 1, 1};
    windowInfo.defaultHotAreas = {windowInfo.area};
    windowInfo.pointerHotAreas = {windowInfo.area};
    windowInfo.agentWindowId = 1;
    windowInfo.flags = 1;
    windowInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    windowInfo.pointerChangeAreas = {1, 2, 1, 2, 1, 2, 1, 2, 1};
    displayGroupInfo.windowsInfo.push_back(windowInfo);

    OLD::DisplayInfo displayInfo1;
    displayInfo1.id = 1;
    displayInfo1.x = 1;
    displayInfo1.y = 1;
    displayInfo1.width = 2;
    displayInfo1.height = 2;
    displayInfo1.dpi = 240;
    displayInfo1.name = "pp";
    displayInfo1.direction = DIRECTION0;
    displayGroupInfo.displaysInfo.push_back(displayInfo1);

    OLD::DisplayInfo displayInfo2;
    displayInfo2.id = 2;
    displayInfo2.x = 1;
    displayInfo2.y = 1;
    displayInfo2.width = 2;
    displayInfo2.height = 2;
    displayInfo2.dpi = 240;
    displayInfo2.name = "pp";
    displayInfo2.uniq = "pp";
    displayInfo2.direction = DIRECTION0;
    displayGroupInfo.displaysInfo.push_back(displayInfo2);

    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayInfo(displayGroupInfo));

    displayGroupInfo.displaysInfo.erase(displayGroupInfo.displaysInfo.begin());
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfo_005
 * @tc.desc: Test updating window & display information for each display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfo_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;

    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfo_error_userid
 * @tc.desc: Test updating window & display information for each display diff userid
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfo_diff_userid, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;
    displayGroupInfo.currentUserId = 11;

    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateKeyEventDisplayId_001
 * @tc.desc: Test the funcation UpdateKeyEventDisplayId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateKeyEventDisplayId_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<KeyEvent> keyEvent = nullptr;
    int32_t focusWindowId = 1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateKeyEventDisplayId(keyEvent, focusWindowId));

    WindowInfo windowInfo = {.displayId = 1};
    WindowGroupInfo windowGroupInfo = {.focusWindowId = 1, .displayId = 1, .windowsInfo = {windowInfo}};
    inputWindowsManager.windowsPerDisplay_.emplace(std::make_pair(1, windowGroupInfo));
    keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    inputWindowsManager.UpdateKeyEventDisplayId(keyEvent, focusWindowId);
    EXPECT_EQ(keyEvent->GetTargetDisplayId(), -1);

    focusWindowId = 2;
    inputWindowsManager.UpdateKeyEventDisplayId(keyEvent, focusWindowId);
    EXPECT_EQ(keyEvent->GetTargetDisplayId(), -1);

    OLD::DisplayInfo info1 = {.id = 0, .x = 0, .y = 0, .width = 100, .height = 200};
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.emplace_back(info1);
    }
    inputWindowsManager.UpdateKeyEventDisplayId(keyEvent, focusWindowId);
    EXPECT_EQ(keyEvent->GetTargetDisplayId(), 0);
}

/**
 * @tc.name: InputWindowsManagerTest_OnDisplayRemovedOrCombinationChanged_003
 * @tc.desc: Test the funcation OnDisplayRemovedOrCombinationChanged
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_OnDisplayRemovedOrCombinationChanged_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayGroupInfo displayGroupInfo {};
    bool ret = inputWindowsManager.OnDisplayRemovedOrCombinationChanged(displayGroupInfo);
    EXPECT_FALSE(ret);

    OLD::DisplayInfo info1 = {.id = 0, .x = 0, .y = 0, .width = 100, .height = 200};
    OLD::DisplayInfo info2 = {.id = 1, .x = 100, .y = 0, .width = 100, .height = 200};
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo = {info1, info2};
    }
    displayGroupInfo.displaysInfo = {info2};
    ret = inputWindowsManager.OnDisplayRemovedOrCombinationChanged(displayGroupInfo);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateWindowInfo_001
 * @tc.desc: Test the funcation UpdateWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateWindowInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;

    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 1;
    windowInfo.uid = 1;
    windowInfo.area = {1, 1, 1, 1};
    windowInfo.defaultHotAreas = {windowInfo.area};
    windowInfo.pointerHotAreas = {windowInfo.area};
    windowInfo.agentWindowId = 1;
    windowInfo.flags = 1;
    windowInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    windowInfo.pointerChangeAreas = {1, 2, 1, 2, 1, 2, 1, 2, 1};
    windowInfo.action = WINDOW_UPDATE_ACTION::ADD;
    displayGroupInfo.windowsInfo.push_back(windowInfo);

    WINDOW_UPDATE_ACTION ret = WIN_MGR->UpdateWindowInfo(displayGroupInfo);
    ASSERT_EQ(ret, WINDOW_UPDATE_ACTION::ADD);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfo_PointerBackCenter_001
 * @tc.desc: Test moved screen , pointer back screen center
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfo_PointerBackCenter_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;

    OLD::DisplayInfo displayInfo1;
    displayInfo1.id = 1;
    displayInfo1.x = 1;
    displayInfo1.y = 1;
    displayInfo1.width = 2;
    displayInfo1.height = 2;
    displayInfo1.dpi = 240;
    displayInfo1.name = "pp";
    displayInfo1.direction = DIRECTION0;
    displayInfo1.displaySourceMode = OHOS::MMI::DisplaySourceMode::SCREEN_MAIN;

    OLD::DisplayInfo displayInfo2;
    displayInfo2.id = 2;
    displayInfo2.x = 1;
    displayInfo2.y = 1;
    displayInfo2.width = 2;
    displayInfo2.height = 2;
    displayInfo2.dpi = 240;
    displayInfo2.name = "pp";
    displayInfo2.uniq = "pp";
    displayInfo2.direction = DIRECTION0;
    displayInfo2.displaySourceMode = OHOS::MMI::DisplaySourceMode::SCREEN_EXPAND;
    displayGroupInfo.displaysInfo.push_back(displayInfo2);
    displayGroupInfo.displaysInfo.push_back(displayInfo1);

    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayInfo(displayGroupInfo));

    displayGroupInfo.displaysInfo.erase(displayGroupInfo.displaysInfo.begin());
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->UpdateDisplayInfo(displayGroupInfo));
    CursorPosition pointerPos = WIN_MGR->GetCursorPos();
    EXPECT_EQ(pointerPos.displayId, displayInfo1.id);
}

/**
 * @tc.name: InputWindowsManagerTest_IgnoreTouchEvent_002
 * @tc.desc: Test the funcation IgnoreTouchEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IgnoreTouchEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    EXPECT_FALSE(inputWindowsManager.IgnoreTouchEvent(pointerEvent));
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    EXPECT_FALSE(inputWindowsManager.IgnoreTouchEvent(pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    PointerEvent::PointerItem item;
    item.SetPointerId(10);
    item.SetLongAxis(-1);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    EXPECT_FALSE(inputWindowsManager.IgnoreTouchEvent(pointerEvent));
    pointerEvent->SetPointerId(10);
    inputWindowsManager.cancelTouchStatus_ = true;
    EXPECT_TRUE(inputWindowsManager.IgnoreTouchEvent(pointerEvent));
    item.SetLongAxis(100);
    pointerEvent->UpdatePointerItem(10, item);
    EXPECT_FALSE(inputWindowsManager.IgnoreTouchEvent(pointerEvent));
    EXPECT_FALSE(inputWindowsManager.IgnoreTouchEvent(pointerEvent));
    item.SetLongAxis(-1);
    pointerEvent->UpdatePointerItem(10, item);
    EXPECT_TRUE(inputWindowsManager.IgnoreTouchEvent(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_ReissueCancelTouchEvent
 * @tc.desc: Test the funcation ReissueCancelTouchEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReissueCancelTouchEvent, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPressed(false);
    pointerEvent->AddPointerItem(item);
    item.SetPointerId(100);
    item.SetPressed(true);
    pointerEvent->AddPointerItem(item);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ReissueCancelTouchEvent(pointerEvent));
}

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING

/**
 * @tc.name: InputWindowsManagerTest_SetPointerEvent
 * @tc.desc: Test the funcation SetPointerEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetPointerEvent, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_DOWN;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    inputWindowsManager.lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(100);
    item.SetPressed(true);
    item.SetDisplayX(50);
    item.SetDisplayY(50);
    inputWindowsManager.lastPointerEvent_->SetTargetDisplayId(150);
    inputWindowsManager.lastPointerEvent_->SetPointerId(100);
    inputWindowsManager.lastPointerEvent_->AddPointerItem(item);
    inputWindowsManager.lastWindowInfo_.id = 10;
    inputWindowsManager.mouseDownInfo_.id = 100;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SetPointerEvent(pointerAction, pointerEvent));
    item.SetPressed(false);
    inputWindowsManager.lastPointerEvent_->UpdatePointerItem(100, item);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.SetPointerEvent(pointerAction, pointerEvent));
}

#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

/**
 * @tc.name: InputWindowsManagerTest_CheckUIExtentionWindowPointerHotArea
 * @tc.desc: Test the funcation CheckUIExtentionWindowPointerHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckUIExtentionWindowPointerHotArea, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    WindowInfo winInfo;
    Rect rect {
        .x = 100,
        .y = 100,
        .width = 1000,
        .height = 1000,
    };
    int32_t logicalX = 300;
    int32_t logicalY = 300;
    std::vector<WindowInfo> windowInfos;
    int32_t windowId = 10;
    winInfo.id = 20;
    winInfo.pointerHotAreas.push_back(rect);
    windowInfos.push_back(winInfo);
    EXPECT_NO_FATAL_FAILURE(
        inputWindowsManager.CheckUIExtentionWindowPointerHotArea(logicalX, logicalY, windowInfos, windowId));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateTargetPointer
 * @tc.desc: Test the funcation UpdateTargetPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTargetPointer, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t longAxis = 1U << 27U;
    inputWindowsManager.IsFoldable_ = true;
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    PointerEvent::PointerItem item;
    item.SetPointerId(10);
    item.SetLongAxis(longAxis);
    pointerEvent->SetPointerId(10);
    pointerEvent->AddPointerItem(item);
    inputWindowsManager.cancelTouchStatus_ = true;
    EXPECT_EQ(inputWindowsManager.UpdateTargetPointer(pointerEvent), RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_CleanInvalidPiexMap
 * @tc.desc: Test the funcation CleanInvalidPiexMap
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CleanInvalidPiexMap, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 100;
    WindowInfo winInfo;
    winInfo.id = 10;
    std::unique_ptr<Media::PixelMap> pixelMap = nullptr;
    inputWindowsManager.transparentWins_.insert_or_assign(windowId, std::move(pixelMap));
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(winInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CleanInvalidPiexMap());
}

/**
 * @tc.name: InputWindowsManagerTest_CleanInvalidPiexMap_001
 * @tc.desc: Test the funcation CleanInvalidPiexMap
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CleanInvalidPiexMap_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 10;
    WindowInfo winInfo;
    winInfo.id = 10;
    std::unique_ptr<Media::PixelMap> pixelMap = nullptr;
    inputWindowsManager.transparentWins_.insert_or_assign(windowId, std::move(pixelMap));
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(winInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CleanInvalidPiexMap());
}

/**
 * @tc.name: InputWindowsManagerTest_GetCancelEventFlag
 * @tc.desc: Test the funcation GetCancelEventFlag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCancelEventFlag, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfoEX winInfoEx;
    winInfoEx.flag = true;
    int32_t pointerId = 100;
    pointerEvent->SetPointerId(100);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerId, winInfoEx));
    EXPECT_TRUE(inputWindowsManager.GetCancelEventFlag(pointerEvent));

    pointerEvent->SetPointerId(200);
    EXPECT_TRUE(inputWindowsManager.GetCancelEventFlag(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetCancelEventFlag_001
 * @tc.desc: Test the funcation GetCancelEventFlag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCancelEventFlag_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    inputWindowsManager.mouseDownInfo_.pid = 100;
    EXPECT_FALSE(inputWindowsManager.GetCancelEventFlag(pointerEvent));

    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHPAD);
    inputWindowsManager.mouseDownInfo_.pid = -1;
    EXPECT_TRUE(inputWindowsManager.GetCancelEventFlag(pointerEvent));

    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_UNKNOWN);
    EXPECT_FALSE(inputWindowsManager.GetCancelEventFlag(pointerEvent));
}

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING

/**
 * @tc.name: InputWindowsManagerTest_DispatchPointerCancel_001
 * @tc.desc: Test the funcation DispatchPointerCancel
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DispatchPointerCancel_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t displayId = -1;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    inputWindowsManager.lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager.lastPointerEvent_, nullptr);
    inputWindowsManager.mouseDownInfo_.id = 10;
    inputWindowsManager.extraData_.appended = false;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_UNKNOWN;
    inputWindowsManager.firstBtnDownWindowInfo_.first = -1;
    inputWindowsManager.firstBtnDownWindowInfo_.second = 10;
    winInfo.id = 10;
    winGroupInfo.windowsInfo.push_back(winInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(10, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointerCancel(displayId));
}

/**
 * @tc.name: InputWindowsManagerTest_DispatchPointerCancel_002
 * @tc.desc: Test the funcation DispatchPointerCancel
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DispatchPointerCancel_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t displayId = -1;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    inputWindowsManager.lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager.lastPointerEvent_, nullptr);
    inputWindowsManager.mouseDownInfo_.id = 50;
    inputWindowsManager.extraData_.appended = false;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_UNKNOWN;
    inputWindowsManager.firstBtnDownWindowInfo_.first = -1;
    inputWindowsManager.firstBtnDownWindowInfo_.second = 10;
    winInfo.id = 10;
    winGroupInfo.windowsInfo.push_back(winInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(10, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointerCancel(displayId));
}

#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

/**
 * @tc.name: InputWindowsManagerTest_UpdateWindowsInfoPerDisplay
 * @tc.desc: Test the funcation UpdateWindowsInfoPerDisplay
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateWindowsInfoPerDisplay, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayGroupInfo displayGroupInfo;
    WindowGroupInfo winGroupInfo;
    WindowInfo winInfo;
    int32_t displayId = 100;
    winInfo.displayId = 100;
    winInfo.id = 200;
    winInfo.windowType = -1;
    displayGroupInfo.focusWindowId = 300;
    displayGroupInfo.windowsInfo.push_back(winInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, winGroupInfo));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateWindowsInfoPerDisplay(displayGroupInfo));

    winInfo.windowType = static_cast<int32_t>(Rosen::WindowType::WINDOW_TYPE_TRANSPARENT_VIEW);
    displayGroupInfo.windowsInfo.push_back(winInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateWindowsInfoPerDisplay(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_SelectWindowInfo_004
 * @tc.desc: Test SelectWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectWindowInfo_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 150;
    int32_t logicalX = 200;
    int32_t logicalY = 200;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    Rect rect {
        .x = 100,
        .y = 100,
        .width = 1000,
        .height = 1000,
    };
    WindowInfo windowInfo;
    WindowGroupInfo windowGroupInfo;
    pointerEvent->bitwise_ = 0x00000000;
    pointerEvent->SetZOrder(15.5f);
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetTargetWindowId(100);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_UP);
    std::unique_ptr<Media::PixelMap> pixelMap = nullptr;
    windowInfo.id = 150;
    windowInfo.displayId = 300;
    windowInfo.flags = 0;
    windowInfo.pointerHotAreas.push_back(rect);
    windowInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    inputWindowsManager.extraData_.appended = true;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    windowInfo.uiExtentionWindowInfo.push_back(windowInfo);
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    windowInfo.windowInputType = WindowInputType::NORMAL;
    it->second.windowsInfo.push_back(windowInfo);
    inputWindowsManager.firstBtnDownWindowInfo_.first = 150;
    inputWindowsManager.transparentWins_.insert_or_assign(windowId, std::move(pixelMap));
    EXPECT_NE(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent), std::nullopt);
}

/**
 * @tc.name: InputWindowsManagerTest_SelectWindowInfo_005
 * @tc.desc: Test SelectWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectWindowInfo_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 50;
    int32_t logicalX = 200;
    int32_t logicalY = 200;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    Rect rect {
        .x = 100,
        .y = 100,
        .width = 1000,
        .height = 1000,
    };
    WindowInfo windowInfo;
    WindowGroupInfo windowGroupInfo;
    inputWindowsManager.firstBtnDownWindowInfo_.first = -1;
    pointerEvent->SetTargetDisplayId(-1);
    pointerEvent->SetTargetWindowId(-1);
    std::unique_ptr<Media::PixelMap> pixelMap = nullptr;
    windowInfo.id = 150;
    windowInfo.displayId = 300;
    windowInfo.flags = 0;
    windowInfo.pointerHotAreas.push_back(rect);
    windowInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    inputWindowsManager.extraData_.appended = false;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_UNKNOWN;
    windowInfo.uiExtentionWindowInfo.push_back(windowInfo);
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    inputWindowsManager.transparentWins_.insert_or_assign(windowId, std::move(pixelMap));
    EXPECT_EQ(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent), std::nullopt);
    pointerEvent->SetButtonPressed(2024);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_BEGIN);
    EXPECT_EQ(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent), std::nullopt);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    EXPECT_EQ(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent), std::nullopt);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_END);
    EXPECT_EQ(inputWindowsManager.SelectWindowInfo(logicalX, logicalY, pointerEvent), std::nullopt);
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
/**
 * @tc.name: InputWindowsManagerTest_ReissueEvent_001
 * @tc.desc: Verify if (keyEvent->GetKeyAction() != KeyEvent::KEY_ACTION_CANCEL && focusWindowId_ != -1 &&
 * focusWindowId_ != focusWindowId && keyEvent->IsRepeatKey())
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReissueEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto keyEvent = KeyEvent::Create();
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_CANCEL);
    InputWindowsManager inputWindowsManager;
    int32_t focusWindowId = 0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ReissueEvent(keyEvent, focusWindowId));

    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    focusWindowId = -1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ReissueEvent(keyEvent, focusWindowId));

    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    focusWindowId = 0;
    inputWindowsManager.focusWindowId_ = 0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ReissueEvent(keyEvent, focusWindowId));

    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    focusWindowId = 0;
    inputWindowsManager.focusWindowId_ = -1;
    keyEvent->SetRepeatKey(true);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ReissueEvent(keyEvent, focusWindowId));

    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    focusWindowId = 0;
    inputWindowsManager.focusWindowId_ = -1;
    keyEvent->SetRepeatKey(false);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ReissueEvent(keyEvent, focusWindowId));
}

/**
 * @tc.name: InputWindowsManagerTest_ReissueEvent_002
 * @tc.desc: Verify ReissueEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReissueEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_UNKNOWN);
    int32_t focusWindowId = -1;
    inputWindowsManager->focusWindowId_ = 0;

    std::shared_ptr<EventDispatchHandler> handler = std::make_shared<EventDispatchHandler>();
    NiceMock<MockInputWindowsManager> mockInputWindowsManager;
    UDSServer udServer;
    EXPECT_CALL(mockInputWindowsManager, GetEventDispatchHandler).WillRepeatedly(Return(handler));
    EXPECT_CALL(mockInputWindowsManager, GetUDSServer).WillRepeatedly(Return(&udServer));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->ReissueEvent(keyEvent, focusWindowId));
}

/**
 * @tc.name: InputWindowsManagerTest_ReissueEvent_003
 * @tc.desc: Verify ReissueEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReissueEvent_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_UNKNOWN);
    int32_t focusWindowId = -1;
    inputWindowsManager->focusWindowId_ = 0;

    NiceMock<MockInputWindowsManager> mockInputWindowsManager;
    UDSServer udServer;
    EXPECT_CALL(mockInputWindowsManager, GetEventDispatchHandler).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(mockInputWindowsManager, GetUDSServer).WillRepeatedly(Return(&udServer));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->ReissueEvent(keyEvent, focusWindowId));
}

/**
 * @tc.name: InputWindowsManagerTest_ReissueEvent_004
 * @tc.desc: Verify ReissueEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReissueEvent_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_UNKNOWN);
    int32_t focusWindowId = -1;
    inputWindowsManager->focusWindowId_ = 0;

    NiceMock<MockInputWindowsManager> mockInputWindowsManager;
    EXPECT_CALL(mockInputWindowsManager, GetEventDispatchHandler).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(mockInputWindowsManager, GetUDSServer).WillRepeatedly(Return(nullptr));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->ReissueEvent(keyEvent, focusWindowId));
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

/**
 * @tc.name: InputWindowsManagerTest_JudgeCameraInFore_001
 * @tc.desc: Verify if (udsServer_ == nullptr)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_JudgeCameraInFore_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    EXPECT_EQ(inputWindowsManager.JudgeCameraInFore(), false);

    UDSServer udsServer;
    inputWindowsManager.udsServer_ = &udsServer;
    EXPECT_NE(inputWindowsManager.udsServer_, nullptr);
    EXPECT_EQ(inputWindowsManager.JudgeCameraInFore(), false);

    int32_t udsPid = 20;
    int32_t udsFd = 15;
    udsServer.idxPidMap_.insert(std::make_pair(udsPid, udsFd));
    SessionPtr session = CreateSessionPtr();
    udsServer.sessionsMap_.insert(std::make_pair(udsPid, session));

    WindowInfo windowInfo;
    windowInfo.id = 20;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    it->second.focusWindowId = 20;
    EXPECT_EQ(inputWindowsManager.JudgeCameraInFore(), false);
}

/**
 * @tc.name: InputWindowsManagerTest_JudgeCameraInFore_002
 * @tc.desc: JudgeCameraInFore
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_JudgeCameraInFore_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;

    UDSServer udsServer;
    inputWindowsManager.udsServer_ = &udsServer;

    int32_t udsPid = 20;
    int32_t udsFd = 15;
    udsServer.idxPidMap_.insert(std::make_pair(udsPid, udsFd));
    SessionPtr session = CreateCameraSessionPtr();
    udsServer.sessionsMap_.insert(std::make_pair(udsFd, session));

    WindowInfo windowInfo;
    windowInfo.id = 20;
    windowInfo.pid = udsPid;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
        it->second.focusWindowId = 20;
    }
    EXPECT_EQ(inputWindowsManager.JudgeCameraInFore(), true);
}

/**
 * @tc.name: InputWindowsManagerTest_SelectPointerChangeArea_003
 * @tc.desc: Test SelectPointerChangeArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectPointerChangeArea_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    std::vector<Rect> areas;
    Rect rect {
        .x = 100,
        .y = 100,
        .width = 1000,
        .height = 1000,
    };
    areas.push_back(rect);
    inputWindowsMgr.windowsHotAreas_.insert(std::make_pair(100, areas));

    int32_t windowId = 100;
    int32_t logicalX = 300;
    int32_t logicalY = 300;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr.SelectPointerChangeArea(windowId, logicalX, logicalY));
}

/**
 * @tc.name: InputWindowsManagerTest_SelectPointerChangeArea_004
 * @tc.desc: Test SelectPointerChangeArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectPointerChangeArea_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsMgr;
    int32_t windowId = 100;
    int32_t logicalX = 300;
    int32_t logicalY = 300;
    EXPECT_FALSE(inputWindowsMgr.SelectPointerChangeArea(windowId, logicalX, logicalY));
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea_002
 * @tc.desc: Test InWhichHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 500;
    int32_t y = 800;
    std::vector<Rect> rects;
    rects = {
        {100, 0, INT32_MAX, 0}
    };
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects));
    rects.clear();
    rects = {
        {150, 100, 300, INT32_MAX}
    };
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects));
    rects.clear();
    rects = {
        {150, 250, 300, 500}
    };
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects));
    x = 200;
    y = 300;
    EXPECT_TRUE(inputWindowsManager.InWhichHotArea(x, y, rects));
    int32_t cycleNum = 7;
    for (int32_t i = 0; i < cycleNum; ++i) {
        rects.insert(rects.begin(), {1000, 1000, 1500, 1500});
        EXPECT_TRUE(inputWindowsManager.InWhichHotArea(x, y, rects));
    }
}

/**
 * @tc.name: InputWindowsManagerTest_HandleGestureInjection_001
 * @tc.desc: Verify if (!gestureInject)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandleGestureInjection_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    bool gestureInject = false;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.HandleGestureInjection(gestureInject));

    gestureInject = true;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.HandleGestureInjection(gestureInject));
}

/**
 * @tc.name: InputWindowsManagerTest_GetPhysicalDisplay_003
 * @tc.desc: Test GetPhysicalDisplay
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPhysicalDisplay_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t id = 1;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 0;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetPhysicalDisplay(id, displayGroupInfo));

    displayInfo.id = 1;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetPhysicalDisplay(id, displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustFingerFlag_001
 * @tc.desc: Test if (pointerEvent->GetSourceType() != PointerEvent::SOURCE_TYPE_TOUCHSCREEN)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AdjustFingerFlag_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    InputWindowsManager inputWindowsManager;
    EXPECT_FALSE(inputWindowsManager.AdjustFingerFlag(pointerEvent));

    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    EXPECT_FALSE(inputWindowsManager.AdjustFingerFlag(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustFingerFlag_002
 * @tc.desc: Test if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_SHELL))
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AdjustFingerFlag_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    uint32_t flag = 0x00000080;
    pointerEvent->bitwise_ |= flag;
    InputWindowsManager inputWindowsManager;
    EXPECT_FALSE(inputWindowsManager.AdjustFingerFlag(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustFingerFlag_003
 * @tc.desc: Test if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_SHELL))
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AdjustFingerFlag_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    uint32_t flag = 0x00000080;
    pointerEvent->bitwise_ |= flag;
    InputWindowsManager inputWindowsManager;
    EXPECT_FALSE(inputWindowsManager.AdjustFingerFlag(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustFingerFlag_004
 * @tc.desc: Test AdjustFingerFlag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AdjustFingerFlag_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    uint32_t flag = 0x00000100;
    pointerEvent->bitwise_ |= flag;
    InputWindowsManager inputWindowsManager;
    EXPECT_FALSE(inputWindowsManager.AdjustFingerFlag(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetClientFd_007
 * @tc.desc: Test GetClientFd
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetClientFd_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    uint32_t flag = 0x00000100;
    pointerEvent->bitwise_ |= flag;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetClientFd(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetClientFd_008
 * @tc.desc: Test if (iter != touchItemDownInfos_.end() && !(iter->second.flag))
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetClientFd_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);

    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    uint32_t flag = 0x00000100;
    pointerEvent->bitwise_ |= flag;
    WindowInfoEX winInfoEx;
    winInfoEx.flag = true;
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerEvent->GetPointerId(), winInfoEx));

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetClientFd(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetClientFd_009
 * @tc.desc: Test if (iter != touchItemDownInfos_.end() && !(iter->second.flag))
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetClientFd_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);

    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    uint32_t flag = 0x00000100;
    pointerEvent->bitwise_ |= flag;
    WindowInfoEX winInfoEx;
    winInfoEx.flag = false;
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerEvent->GetPointerId(), winInfoEx));

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetClientFd(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetClientFd_010
 * @tc.desc: Test if (uiExtentionWindowInfo.id == pointerEvent->GetTargetWindowId())
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetClientFd_010, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    int32_t id = 1;
    pointerEvent->SetTargetWindowId(id);

    WindowInfo windowInfo1;
    windowInfo1.id = 1;
    windowInfo1.uiExtentionWindowInfo.push_back(windowInfo1);
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo1);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetClientFd(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetClientFd_011
 * @tc.desc: Test if (uiExtentionWindowInfo.id == pointerEvent->GetTargetWindowId())
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetClientFd_011, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    int32_t id = 2;
    pointerEvent->SetTargetWindowId(id);

    WindowInfo windowInfo1;
    windowInfo1.id = 1;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo1);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetClientFd(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_001
 * @tc.desc: Test FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    uint32_t flag = 0x00000100;
    pointerEvent->bitwise_ |= flag;
    InputWindowsManager inputWindowsManager;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_002
 * @tc.desc: Test if (iter == touchItemDownInfos_.end())
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    uint32_t flag = 0x00000100;
    pointerEvent->bitwise_ |= flag;

    InputWindowsManager inputWindowsManager;
    WindowInfoEX winInfoEx;
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerEvent->GetPointerId(), winInfoEx));

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowPid_003
 * @tc.desc: Test if (uiExtentionWindow.id == windowId)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowPid_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 1;
    WindowInfo windowInfo1;
    windowInfo1.id = 2;
    WindowInfo windowInfo2;
    windowInfo2.id = 1;
    windowInfo1.uiExtentionWindowInfo.push_back(windowInfo2);
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo1);
    }

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetWindowPid(windowId));
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowPid_004
 * @tc.desc: Test if (uiExtentionWindow.id == windowId)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowPid_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 1;
    WindowInfo windowInfo1;
    windowInfo1.id = 2;
    WindowInfo windowInfo2;
    windowInfo2.id = 3;
    windowInfo1.uiExtentionWindowInfo.push_back(windowInfo2);
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo1);
    }

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetWindowPid(windowId));
}

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
/**
 * @tc.name: InputWindowsManagerTest_IsWindowRotation_001
 * @tc.desc: Test IsWindowRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsWindowRotation_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(::OHOS::MMI::IInputWindowsManager::GetInstance());
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<OLD::DisplayInfo> physicalDisplayInfo = std::make_shared<OLD::DisplayInfo>();
    ASSERT_NE(physicalDisplayInfo, nullptr);
    physicalDisplayInfo->direction = DIRECTION0;
    physicalDisplayInfo->displayDirection = DIRECTION90;
    auto ret = inputWindowsManager->IsWindowRotation(physicalDisplayInfo.get());
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: InputWindowsManagerTest_IsWindowRotation_002
 * @tc.desc: Test the funcation IsWindowRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsWindowRotation_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(::OHOS::MMI::IInputWindowsManager::GetInstance());
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<OLD::DisplayInfo> physicalDisplayInfo = std::make_shared<OLD::DisplayInfo>();
    ASSERT_NE(physicalDisplayInfo, nullptr);
    bool ret = inputWindowsManager->IsWindowRotation(physicalDisplayInfo.get());
    if (ROTATE_POLICY == WINDOW_ROTATE) {
        ASSERT_EQ(ret, true);
    } else {
        ASSERT_EQ(ret, false);
    }
}
/**
 * @tc.name: InputWindowsManagerTest_ShiftAppPointerEvent_001
 * @tc.desc: Test ShiftAppPointerEvent failed for sourceWindowInfo not exist
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppPointerEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = nullptr;
    int32_t sourceWindowId = 50;
    int32_t targetWindowId = 51;
    ShiftWindowParam param;
    param.sourceWindowId = sourceWindowId;
    param.targetWindowId = targetWindowId;
    bool autoGenDown = true;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    int32_t ret = inputWindowsManager.ShiftAppPointerEvent(param, autoGenDown);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ShiftAppPointerEvent_002
 * @tc.desc: Test ShiftAppPointerEvent failed for targetWindowInfo not exist
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppPointerEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = nullptr;
    int32_t sourceWindowId = 50;
    int32_t targetWindowId = 51;
    ShiftWindowParam param;
    param.sourceWindowId = sourceWindowId;
    param.targetWindowId = targetWindowId;
    bool autoGenDown = true;
    int32_t displayId = 0;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = sourceWindowId;
    windowInfo.displayId = displayId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    int32_t ret = inputWindowsManager.ShiftAppPointerEvent(param, autoGenDown);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ShiftAppPointerEvent_003
 * @tc.desc: Test ShiftAppPointerEvent failed for displayId invalid
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppPointerEvent_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = nullptr;
    int32_t sourceWindowId = 50;
    int32_t targetWindowId = 51;
    ShiftWindowParam param;
    param.sourceWindowId = sourceWindowId;
    param.targetWindowId = targetWindowId;
    bool autoGenDown = true;
    int32_t displayId = -1;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = sourceWindowId;
    windowInfo.displayId = displayId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    int32_t ret = inputWindowsManager.ShiftAppPointerEvent(param, autoGenDown);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ShiftAppPointerEvent_004
 * @tc.desc: Test ShiftAppPointerEvent failed for sourceWindowId untouchtable
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppPointerEvent_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = nullptr;
    int32_t sourceWindowId = 50;
    int32_t targetWindowId = 51;
    ShiftWindowParam param;
    param.sourceWindowId = sourceWindowId;
    param.targetWindowId = targetWindowId;
    bool autoGenDown = true;
    int32_t displayId = 0;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = sourceWindowId;
    windowInfo.displayId = displayId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    windowInfo.flags &= WindowInfo::FLAG_BIT_UNTOUCHABLE;
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    int32_t ret = inputWindowsManager.ShiftAppPointerEvent(param, autoGenDown);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ShiftAppPointerEvent_005
 * @tc.desc: Test ShiftAppPointerEvent failed for sourceWindowId transparent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppPointerEvent_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = nullptr;
    int32_t sourceWindowId = 50;
    int32_t targetWindowId = 51;
    ShiftWindowParam param;
    param.sourceWindowId = sourceWindowId;
    param.targetWindowId = targetWindowId;
    bool autoGenDown = true;
    int32_t displayId = 0;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = sourceWindowId;
    windowInfo.displayId = displayId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    inputWindowsManager.transparentWins_[sourceWindowId];
    int32_t ret = inputWindowsManager.ShiftAppPointerEvent(param, autoGenDown);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ShiftAppPointerEvent_006
 * @tc.desc: Test ShiftAppPointerEvent failed for null lastPointerEvent_
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppPointerEvent_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = nullptr;
    int32_t sourceWindowId = 50;
    int32_t targetWindowId = 51;
    ShiftWindowParam param;
    param.sourceWindowId = sourceWindowId;
    param.targetWindowId = targetWindowId;
    bool autoGenDown = true;
    int32_t displayId = 0;
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = sourceWindowId;
    windowInfo.displayId = displayId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    windowInfo.id = targetWindowId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    int32_t ret = inputWindowsManager.ShiftAppPointerEvent(param, autoGenDown);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ShiftAppPointerEvent_007
 * @tc.desc: Test ShiftAppPointerEvent failed for null lastPointerEvent_
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppPointerEvent_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastTouchEvent_ = nullptr;
    int32_t sourceWindowId = 50;
    int32_t targetWindowId = 51;
    ShiftWindowParam param;
    param.sourceWindowId = sourceWindowId;
    param.targetWindowId = targetWindowId;
    param.sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    bool autoGenDown = true;
    int32_t displayId = 0;
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = sourceWindowId;
    windowInfo.displayId = displayId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    windowInfo.id = targetWindowId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    int32_t ret = inputWindowsManager.ShiftAppPointerEvent(param, autoGenDown);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ShiftAppPointerEvent_008
 * @tc.desc: Test ShiftAppPointerEvent failed for null lastPointerEvent_
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppPointerEvent_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastTouchEvent_ = nullptr;
    int32_t sourceWindowId = 50;
    int32_t targetWindowId = 51;
    int32_t fingerId = -1;
    ShiftWindowParam param;
    param.sourceWindowId = sourceWindowId;
    param.targetWindowId = targetWindowId;
    param.sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    param.fingerId = fingerId;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager.lastTouchEvent_ = pointerEvent;
    bool autoGenDown = true;
    int32_t displayId = 0;
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = sourceWindowId;
    windowInfo.displayId = displayId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    windowInfo.id = targetWindowId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    int32_t ret = inputWindowsManager.ShiftAppPointerEvent(param, autoGenDown);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ShiftAppPointerEvent_009
 * @tc.desc: Test ShiftAppPointerEvent failed for null lastPointerEvent_
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppPointerEvent_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastTouchEvent_ = nullptr;
    int32_t sourceWindowId = 50;
    int32_t targetWindowId = 51;
    int32_t fingerId = 1;
    ShiftWindowParam param;
    param.sourceWindowId = sourceWindowId;
    param.targetWindowId = targetWindowId;
    param.sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    param.fingerId = fingerId;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager.lastTouchEvent_ = pointerEvent;
    bool autoGenDown = true;
    int32_t displayId = 0;
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = sourceWindowId;
    windowInfo.displayId = displayId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    windowInfo.id = targetWindowId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    int32_t ret = inputWindowsManager.ShiftAppPointerEvent(param, autoGenDown);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ShiftAppPointerEvent_010
 * @tc.desc: Test ShiftAppPointerEvent failed for null lastPointerEvent_
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppPointerEvent_010, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastTouchEvent_ = nullptr;
    int32_t sourceWindowId = 50;
    int32_t targetWindowId = 51;
    int32_t fingerId = 0;
    ShiftWindowParam param;
    param.sourceWindowId = sourceWindowId;
    param.targetWindowId = targetWindowId;
    param.sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    param.fingerId = fingerId;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    bool autoGenDown = true;
    int32_t displayId = 0;
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = sourceWindowId;
    windowInfo.displayId = displayId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    windowInfo.id = targetWindowId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    inputWindowsManager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    inputWindowsManager.lastTouchEvent_ = pointerEvent;
    int32_t ret = inputWindowsManager.ShiftAppPointerEvent(param, autoGenDown);
    EXPECT_NE(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ResetPointerPositionIfOutValidDisplay_001
 * @tc.desc: Test if (isOut && isChange)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ResetPointerPositionIfOutValidDisplay_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.width = 1920;
    displayInfo.height = 1080;
    displayGroupInfo.displaysInfo.push_back(displayInfo);

    InputWindowsManager inputWindowsManager;
    inputWindowsManager.cursorPos_.displayId = 1;
    inputWindowsManager.cursorPos_.cursorPos.x = -1;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }

    // isOut = true, isChange = false
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ResetPointerPositionIfOutValidDisplay(displayGroupInfo));

    displayGroupInfo.displaysInfo.clear();
    displayInfo.offsetX = 1;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    // isOut = true, isChange = true
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ResetPointerPositionIfOutValidDisplay(displayGroupInfo));

    inputWindowsManager.cursorPos_.cursorPos.x = 10;
    inputWindowsManager.cursorPos_.cursorPos.y = 10;
    displayGroupInfo.displaysInfo.clear();
    displayInfo.validWidth = 11;
    displayInfo.validHeight = 11;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    // isOut = false, isChange = true
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ResetPointerPositionIfOutValidDisplay(displayGroupInfo));

    displayGroupInfo.displaysInfo.clear();
    displayInfo.offsetX = 0;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    it->second.displaysInfo.clear();
    it->second.displaysInfo.push_back(displayInfo);
    // isOut = false, isChange = false
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ResetPointerPositionIfOutValidDisplay(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_IsPositionOutValidDisplay_001
 * @tc.desc: Test if (!isOut && isPhysicalPos)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsPositionOutValidDisplay_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.width = 1;
    displayInfo.height = 1;

    Coordinate2D position;
    position.x = -1;

    // isOut = true, isChange = false
    bool isPhysicalPos = false;
    InputWindowsManager inputWindowsManager;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.IsPositionOutValidDisplay(position, displayInfo, isPhysicalPos));

    position.x = 1;
    position.y = 1;
    displayInfo.validWidth = 1;
    displayInfo.validHeight = 1;
    // isOut = false, isChange = true, currentDisplay.fixedDirection = DIRECTION0
    isPhysicalPos = true;
    displayInfo.fixedDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.IsPositionOutValidDisplay(position, displayInfo, isPhysicalPos));

    displayInfo.fixedDirection = DIRECTION90;
    // isOut = false, isChange = true, currentDisplay.fixedDirection = DIRECTION90
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.IsPositionOutValidDisplay(position, displayInfo, isPhysicalPos));

    displayInfo.fixedDirection = DIRECTION180;
    // isOut = false, isChange = true, currentDisplay.fixedDirection = DIRECTION180
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.IsPositionOutValidDisplay(position, displayInfo, isPhysicalPos));

    displayInfo.fixedDirection = DIRECTION270;
    // isOut = false, isChange = true, currentDisplay.fixedDirection = DIRECTION270
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.IsPositionOutValidDisplay(position, displayInfo, isPhysicalPos));
}

/**
 * @tc.name: InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_001
 * @tc.desc: Test if (lastPointerEventforGesture_->GetSourceType() != PointerEvent::SOURCE_TYPE_TOUCHSCREEN)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(
    InputWindowsManagerTest, InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.displaysInfo.push_back(displayInfo);

    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    inputWindowsManager.lastPointerEventforGesture_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));

    inputWindowsManager.lastPointerEventforGesture_->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHPAD);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_002
 * @tc.desc: Test if (touchDisplayId == currentDisplay.id && IsValidDisplayChange(currentDisplay))
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(
    InputWindowsManagerTest, InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.displaysInfo.push_back(displayInfo);

    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    inputWindowsManager.lastPointerEventforGesture_ = pointerEvent;
    inputWindowsManager.lastPointerEventforGesture_->SetTargetDisplayId(displayInfo.id);
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }
    // true false
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));

    // false false
    inputWindowsManager.lastPointerEventforGesture_->SetTargetDisplayId(0);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));

    // false true
    displayGroupInfo.displaysInfo.clear();
    displayInfo.offsetX = 1;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    inputWindowsManager.lastPointerEventforGesture_->SetTargetDisplayId(0);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));

    // true true
    inputWindowsManager.lastPointerEventforGesture_->SetTargetDisplayId(displayInfo.id);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_CancelMouseEvent_001
 * @tc.desc: Test if (lastPointerEvent_ == nullptr)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelMouseEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);

    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelMouseEvent());

    inputWindowsManager.lastPointerEvent_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelMouseEvent());
}

/**
 * @tc.name: InputWindowsManagerTest_CancelMouseEvent_002
 * @tc.desc: Test if (lastPointerEvent_->GetSourceType() == PointerEvent::SOURCE_TYPE_MOUSE &&
             !lastPointerEvent_->GetPressedButtons().empty())
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelMouseEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);

    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    // true false
    inputWindowsManager.lastPointerEvent_->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelMouseEvent());

    // true true
    inputWindowsManager.lastPointerEvent_->pressedButtons_.insert(0);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelMouseEvent());

    // false true
    inputWindowsManager.lastPointerEvent_->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelMouseEvent());

    // false false
    inputWindowsManager.lastPointerEvent_->pressedButtons_.clear();
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelMouseEvent());
}

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_001
 * @tc.desc: Test if (lastPointerEvent_->GetPointerAction() != PointerEvent::POINTER_ACTION_DOWN &&
        (lastPointerEvent_->GetPointerAction() == PointerEvent::POINTER_ACTION_BUTTON_UP ||
        lastPointerEvent_->GetPointerAction() == PointerEvent::POINTER_ACTION_PULL_UP ||
        lastPointerEvent_->GetPressedButtons().empty()))
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);

    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    // true false
    inputWindowsManager.lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    inputWindowsManager.lastPointerEvent_->pressedButtons_.insert(0);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerDrawingManagerWindowInfo());

    // true true
    inputWindowsManager.lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerDrawingManagerWindowInfo());
    inputWindowsManager.lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_UP);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerDrawingManagerWindowInfo());
    inputWindowsManager.lastPointerEvent_->pressedButtons_.clear();
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerDrawingManagerWindowInfo());

    // false true
    inputWindowsManager.lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerDrawingManagerWindowInfo());

    // false false
    inputWindowsManager.lastPointerEvent_->pressedButtons_.insert(0);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerDrawingManagerWindowInfo());
}

#endif // defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)

/**
 * @tc.name: InputWindowsManagerTest_PrintHighZorder_001
 * @tc.desc: Test if (!info)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintHighZorder_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.id = 0;
    windowInfo.flags = 2;
    ;
    WindowGroupInfo windowGroupInfo;
    windowGroupInfo.displayId = 0;
    windowGroupInfo.windowsInfo.push_back(windowInfo);

    InputWindowsManager inputWindowsManager;
    inputWindowsManager.windowsPerDisplay_[0] = windowGroupInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintHighZorder(
        windowGroupInfo.windowsInfo, PointerEvent::POINTER_ACTION_AXIS_BEGIN, windowInfo.id, 0, 0));

    inputWindowsManager.windowsPerDisplay_.clear();
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintHighZorder(
        windowGroupInfo.windowsInfo, PointerEvent::POINTER_ACTION_AXIS_BEGIN, windowInfo.id, 0, 0));
}

/**
 * @tc.name: InputWindowsManagerTest_PrintHighZorder_002
 * @tc.desc: Test if (MMI_GNE(windowInfo.zOrder, targetWindow.zOrder) && !windowInfo.flags &&
            pointerAction == PointerEvent::POINTER_ACTION_AXIS_BEGIN &&
            windowInfo.windowInputType != WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE &&
            windowInfo.windowInputType != WindowInputType::MIX_BUTTOM_ANTI_AXIS_MOVE &&
            windowInfo.windowInputType != WindowInputType::TRANSMIT_ALL)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintHighZorder_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.id = 0;
    windowInfo.flags = 2;
    windowInfo.zOrder = 1;
    WindowGroupInfo windowGroupInfo;
    windowGroupInfo.displayId = 0;
    windowGroupInfo.windowsInfo.push_back(windowInfo);

    InputWindowsManager inputWindowsManager;
    inputWindowsManager.windowsPerDisplay_[0] = windowGroupInfo;

    windowGroupInfo.windowsInfo.clear();
    windowInfo.flags = 0;
    windowInfo.zOrder = 10;
    windowInfo.windowInputType = WindowInputType::NORMAL;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintHighZorder(
        windowGroupInfo.windowsInfo, PointerEvent::POINTER_ACTION_AXIS_BEGIN, windowInfo.id, 0, 0));

    windowGroupInfo.windowsInfo.clear();
    windowInfo.windowInputType = WindowInputType::TRANSMIT_ALL;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintHighZorder(
        windowGroupInfo.windowsInfo, PointerEvent::POINTER_ACTION_AXIS_BEGIN, windowInfo.id, 0, 0));

    windowGroupInfo.windowsInfo.clear();
    windowInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintHighZorder(
        windowGroupInfo.windowsInfo, PointerEvent::POINTER_ACTION_AXIS_BEGIN, windowInfo.id, 0, 0));

    windowGroupInfo.windowsInfo.clear();
    windowInfo.windowInputType = WindowInputType::MIX_BUTTOM_ANTI_AXIS_MOVE;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintHighZorder(
        windowGroupInfo.windowsInfo, PointerEvent::POINTER_ACTION_AXIS_BEGIN, windowInfo.id, 0, 0));

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintHighZorder(
        windowGroupInfo.windowsInfo, PointerEvent::POINTER_ACTION_AXIS_UPDATE, windowInfo.id, 0, 0));

    windowGroupInfo.windowsInfo.clear();
    windowInfo.flags = 2;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintHighZorder(
        windowGroupInfo.windowsInfo, PointerEvent::POINTER_ACTION_AXIS_UPDATE, windowInfo.id, 0, 0));
}

/**
 * @tc.name: InputWindowsManagerTest_PrintHighZorder_003
 * @tc.desc: Test if (MMI_GNE(windowInfo.zOrder, targetWindow.zOrder) && !windowInfo.flags &&
            pointerAction == PointerEvent::POINTER_ACTION_AXIS_BEGIN &&
            windowInfo.windowInputType != WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE &&
            windowInfo.windowInputType != WindowInputType::MIX_BUTTOM_ANTI_AXIS_MOVE &&
            windowInfo.windowInputType != WindowInputType::TRANSMIT_ALL)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintHighZorder_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo windowInfo;
    windowInfo.id = 0;
    windowInfo.flags = 2;
    windowInfo.zOrder = 1;
    WindowGroupInfo windowGroupInfo;
    windowGroupInfo.displayId = 0;
    windowGroupInfo.windowsInfo.push_back(windowInfo);

    InputWindowsManager inputWindowsManager;
    inputWindowsManager.windowsPerDisplay_[0] = windowGroupInfo;

    windowGroupInfo.windowsInfo.clear();
    windowInfo.zOrder = 0;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintHighZorder(
        windowGroupInfo.windowsInfo, PointerEvent::POINTER_ACTION_AXIS_UPDATE, windowInfo.id, 0, 0));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateCustomStyle_001
 * @tc.desc: Test if (pointerStyle.id != MOUSE_ICON::DEVELOPER_DEFINED_ICON
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateCustomStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t windowId = 0;
    PointerStyle pointerStyle;
    pointerStyle.id = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    InputWindowsManager inputWindowsManager;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateCustomStyle(windowId, pointerStyle));

    pointerStyle.id = MOUSE_ICON::TRANSPARENT_ICON;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateCustomStyle(windowId, pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateCustomStyle_002
 * @tc.desc: Test if (innerIt.first != windowId && innerIt.second.id == MOUSE_ICON::DEVELOPER_DEFINED_ICON)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateCustomStyle_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t windowId = 0;
    PointerStyle pointerStyle;
    pointerStyle.id = MOUSE_ICON::DEVELOPER_DEFINED_ICON;

    std::map<int32_t, PointerStyle> pointerStyles;
    pointerStyles[1] = pointerStyle;

    InputWindowsManager inputWindowsManager;
    inputWindowsManager.pointerStyle_[0] = pointerStyles;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateCustomStyle(windowId, pointerStyle));

    windowId = 1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateCustomStyle(windowId, pointerStyle));

    windowId = 0;
    PointerStyle pointerStyle1;
    pointerStyle1.id = MOUSE_ICON::TRANSPARENT_ICON;
    pointerStyles[1] = pointerStyle1;
    inputWindowsManager.pointerStyle_[0] = pointerStyles;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateCustomStyle(windowId, pointerStyle));
}

#ifdef OHOS_BUILD_ENABLE_ONE_HAND_MODE
/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerItemInOneHandMode_001
 * @tc.desc: Test if (pointerEvent->HasFlag(InputEvent::EVENT_FLAG_SIMULATE))
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerItemInOneHandMode_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    DisplayInfo displayInfo;
    displayInfo.oneHandY = 10;
    displayInfo.height = 11;

    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetPointerId(0);
    pointerEvent->AddPointerItem(pointerItem);
    pointerEvent->SetPointerId(0);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerItemInOneHandMode(displayInfo, pointerEvent));

    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerItemInOneHandMode(displayInfo, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerItemInOneHandMode_002
 * @tc.desc: Test if (autoToVirtualScreen)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerItemInOneHandMode_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    DisplayInfo displayInfo;
    displayInfo.oneHandY = 10;
    displayInfo.height = 11;

    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetPointerId(0);
    pointerEvent->AddPointerItem(pointerItem);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    pointerEvent->SetAutoToVirtualScreen(true);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerItemInOneHandMode(displayInfo, pointerEvent));

    pointerEvent->SetAutoToVirtualScreen(false);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerItemInOneHandMode(displayInfo, pointerEvent));
}
#endif // OHOS_BUILD_ENABLE_ONE_HAND_MODE

/**
 * @tc.name: InputWindowsManagerTest_ShiftAppMousePointerEvent_001
 * @tc.desc: Test if (!lastPointerEvent_ || !lastPointerEvent_->IsButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT))
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ShiftAppMousePointerEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = nullptr;
    ShiftWindowInfo shiftWindowInfo;
    bool autoGenDown = false;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ShiftAppMousePointerEvent(shiftWindowInfo, autoGenDown));

    inputWindowsManager.lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(inputWindowsManager.lastPointerEvent_, nullptr);
    inputWindowsManager.lastPointerEvent_->SetButtonPressed(PointerEvent::MOUSE_BUTTON_RIGHT);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.ShiftAppMousePointerEvent(shiftWindowInfo, autoGenDown));
}

/**
 * @tc.name: InputWindowsManagerTest_CancelTouch_001
 * @tc.desc: Test if ((iter != touchItemDownInfos_.end()) && iter->second.flag)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelTouch_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t touch = 0;
    WindowInfoEX windowInfoEX;
    windowInfoEX.flag = true;
    inputWindowsManager.touchItemDownInfos_[0] = windowInfoEX;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelTouch(touch));

    windowInfoEX.flag = false;
    inputWindowsManager.touchItemDownInfos_[0] = windowInfoEX;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelTouch(touch));

    touch = 1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelTouch(touch));
}

/**
 * @tc.name: InputWindowsManagerTest_CancelAllTouches_001
 * @tc.desc: Test if (!item.IsPressed())
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelAllTouches_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetPointerId(0);
    pointerItem.SetPressed(false);
    pointerEvent->AddPointerItem(pointerItem);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelAllTouches(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_CancelAllTouches_002
 * @tc.desc: Test if (AdjustFingerFlag(pointerEvent))
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelAllTouches_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetPointerId(0);
    pointerItem.SetPressed(true);
    pointerEvent->AddPointerItem(pointerItem);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);

    WindowInfoEX windowInfoEX;
    windowInfoEX.flag = false;
    inputWindowsManager.touchItemDownInfos_[0] = windowInfoEX;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelAllTouches(pointerEvent));

    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHPAD);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelAllTouches(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_CancelAllTouches_003
 * @tc.desc: Test if (winOpt)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelAllTouches_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetPointerId(0);
    pointerItem.SetPressed(true);
    pointerItem.SetTargetWindowId(0);
    pointerEvent->AddPointerItem(pointerItem);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHPAD);
    pointerEvent->SetTargetDisplayId(-1);

    WindowInfo windowInfo;
    windowInfo.id = 0;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelAllTouches(pointerEvent));
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

#ifdef OHOS_BUILD_ENABLE_ONE_HAND_MODE
/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayXYInOneHandMode_002
 * @tc.desc: Test UpdateDisplayXYInOneHandMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayXYInOneHandMode_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo disPlayInfo = {
        .id = 2,
        .oneHandX = 100,
        .oneHandY = 150,
        .width = 200,
        .name = "test",
    };
    float oneHandScale = 0.8;
    double physicalX = 200;
    double physicalY = 250;
    inputWindowsManager.UpdateDisplayXYInOneHandMode(physicalX, physicalY, disPlayInfo, oneHandScale);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerItemInOneHandMode_003
 * @tc.desc: Test UpdatePointerItemInOneHandMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerItemInOneHandMode_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo disPlayInfo = {
        .id = 2,
        .oneHandX = 100,
        .oneHandY = 150,
        .width = 200,
        .name = "test",
        .height = 250,
        .scalePercent = 150,
    };
    auto pointerEvent = PointerEvent::Create();
    int32_t pointerId = 3;
    pointerEvent->SetPointerId(pointerId);
    PointerEvent::PointerItem pointerItem;
    pointerItem.pointerId_ = pointerId;
    double physicalX = 20.3;
    double physicalY = 30.6;
    pointerItem.SetDisplayXPos(physicalX);
    pointerItem.SetDisplayYPos(physicalY);
    pointerEvent->AddPointerItem(pointerItem);
    ASSERT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerItemInOneHandMode(disPlayInfo, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerItemInOneHandMode_004
 * @tc.desc: Test UpdatePointerItemInOneHandMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerItemInOneHandMode_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo disPlayInfo = {
        .id = 2,
        .oneHandX = 100,
        .oneHandY = 150,
        .width = 200,
        .name = "test",
        .height = 150,
        .scalePercent = 150,
    };
    auto pointerEvent = PointerEvent::Create();
    int32_t pointerId = 3;
    pointerEvent->SetPointerId(pointerId);
    PointerEvent::PointerItem pointerItem;
    pointerItem.pointerId_ = pointerId;
    double physicalX = 20.3;
    double physicalY = 30.6;
    pointerItem.SetDisplayXPos(physicalX);
    pointerItem.SetDisplayYPos(physicalY);
    pointerEvent->AddPointerItem(pointerItem);
    ASSERT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerItemInOneHandMode(disPlayInfo, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerItemInOneHandMode_005
 * @tc.desc: Test UpdatePointerItemInOneHandMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerItemInOneHandMode_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo disPlayInfo = {
        .id = 2,
        .oneHandX = 100,
        .oneHandY = 150,
        .width = 200,
        .name = "test",
        .height = 250,
        .scalePercent = 50,
    };
    auto pointerEvent = PointerEvent::Create();
    int32_t pointerId = 3;
    pointerEvent->SetPointerId(pointerId);
    PointerEvent::PointerItem pointerItem;
    pointerItem.pointerId_ = pointerId;
    double physicalX = 20.3;
    double physicalY = 30.6;
    pointerItem.SetDisplayXPos(physicalX);
    pointerItem.SetDisplayYPos(physicalY);
    pointerEvent->bitwise_ = InputEvent::EVENT_FLAG_SIMULATE;
    pointerEvent->SetAutoToVirtualScreen(true);
    pointerEvent->AddPointerItem(pointerItem);
    ASSERT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerItemInOneHandMode(disPlayInfo, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerItemInOneHandMode_006
 * @tc.desc: Test UpdatePointerItemInOneHandMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerItemInOneHandMode_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo disPlayInfo = {
        .id = 2,
        .oneHandX = 100,
        .oneHandY = 150,
        .width = 200,
        .name = "test",
        .height = 250,
        .scalePercent = 50,
    };
    auto pointerEvent = PointerEvent::Create();
    int32_t pointerId = 3;
    pointerEvent->SetPointerId(pointerId);
    PointerEvent::PointerItem pointerItem;
    pointerItem.pointerId_ = pointerId;
    double physicalX = 20.3;
    double physicalY = 30.6;
    pointerItem.SetDisplayXPos(physicalX);
    pointerItem.SetDisplayYPos(physicalY);
    pointerEvent->bitwise_ = InputEvent::EVENT_FLAG_NONE;
    pointerEvent->SetAutoToVirtualScreen(true);
    pointerEvent->AddPointerItem(pointerItem);
    ASSERT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerItemInOneHandMode(disPlayInfo, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerItemInOneHandMode_007
 * @tc.desc: Test UpdatePointerItemInOneHandMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerItemInOneHandMode_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo disPlayInfo = {
        .id = 2,
        .oneHandX = 100,
        .oneHandY = 150,
        .width = 200,
        .name = "test",
        .height = 250,
        .scalePercent = 50,
    };
    auto pointerEvent = PointerEvent::Create();
    int32_t pointerId = 3;
    pointerEvent->SetPointerId(pointerId);
    PointerEvent::PointerItem pointerItem;
    pointerItem.pointerId_ = pointerId;
    double physicalX = 20.3;
    double physicalY = 30.6;
    pointerItem.SetDisplayXPos(physicalX);
    pointerItem.SetDisplayYPos(physicalY);
    pointerEvent->bitwise_ = InputEvent::EVENT_FLAG_SIMULATE;
    pointerEvent->SetAutoToVirtualScreen(false);
    pointerEvent->AddPointerItem(pointerItem);
    ASSERT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerItemInOneHandMode(disPlayInfo, pointerEvent));
}
#endif // OHOS_BUILD_ENABLE_ONE_HAND_MODE

/**
 * @tc.name: InputWindowsManagerTest_CancelMouseEvent_003
 * @tc.desc: Test CancelMouseEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelMouseEvent_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    inputWindowsManager.extraData_.appended = true;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelMouseEvent());
    inputWindowsManager.extraData_.appended = true;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_UNKNOWN;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelMouseEvent());
    inputWindowsManager.extraData_.appended = false;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelMouseEvent());
    inputWindowsManager.extraData_.appended = false;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_UNKNOWN;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.CancelMouseEvent());
}

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
/**
 * @tc.name: InputWindowsManagerTest_AdjustDragPosition_001
 * @tc.desc: Test AdjustDragPosition
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AdjustDragPosition_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.AdjustDragPosition());
}
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
/**
 * @tc.name: InputWindowsManagerTest_PrintEnterEventInfo_001
 * @tc.desc: Test PrintEnterEventInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintEnterEventInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    InputWindowsManager inputWindowsManager;
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_LEAVE_WINDOW;
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_UNKNOWN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.PrintEnterEventInfo(pointerEvent));
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

#ifdef OHOS_BUILD_ENABLE_TOUCH
/**
 * @tc.name: InputWindowsManagerTest_TriggerTouchUpOnInvalidAreaEntry_001
 * @tc.desc: Test TriggerTouchUpOnInvalidAreaEntry
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TriggerTouchUpOnInvalidAreaEntry_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEventforGesture_ = nullptr;
    int32_t pointerId = -5;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.TriggerTouchUpOnInvalidAreaEntry(pointerId));
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager.lastPointerEventforGesture_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.TriggerTouchUpOnInvalidAreaEntry(pointerId));
    PointerEvent::PointerItem pointerItem;
    pointerItem.pointerId_ = pointerId;
    pointerItem.canceled_ = true;
    pointerItem.pressed_ = true;
    pointerEvent->pointers_.push_back(pointerItem);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.TriggerTouchUpOnInvalidAreaEntry(pointerId));
    pointerItem.canceled_ = true;
    pointerItem.pressed_ = false;
    pointerEvent->pointers_.push_back(pointerItem);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.TriggerTouchUpOnInvalidAreaEntry(pointerId));
    pointerItem.canceled_ = false;
    pointerItem.pressed_ = true;
    pointerEvent->pointers_.push_back(pointerItem);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.TriggerTouchUpOnInvalidAreaEntry(pointerId));
    pointerItem.canceled_ = false;
    pointerItem.pressed_ = false;
    pointerEvent->pointers_.push_back(pointerItem);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.TriggerTouchUpOnInvalidAreaEntry(pointerId));
}
#endif // OHOS_BUILD_ENABLE_TOUCH

#ifdef OHOS_BUILD_ENABLE_ONE_HAND_MODE
/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayXYInOneHandMode_001
 * @tc.desc: Test UpdateDisplayXYInOneHandMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayXYInOneHandMode_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    double physicalX = 6;
    double physicalY = 6;
    float oneHandScale = 1.0;
    OLD::DisplayInfo displayInfo;
    displayInfo.oneHandY = 5;
    displayInfo.oneHandX = 5;
    displayInfo.width = 5;
    EXPECT_NO_FATAL_FAILURE(
        inputWindowsManager.UpdateDisplayXYInOneHandMode(physicalX, physicalY, displayInfo, oneHandScale));
    physicalX = 11;
    physicalY = 6;
    EXPECT_NO_FATAL_FAILURE(
        inputWindowsManager.UpdateDisplayXYInOneHandMode(physicalX, physicalY, displayInfo, oneHandScale));
    physicalX = 4;
    physicalY = 6;
    EXPECT_NO_FATAL_FAILURE(
        inputWindowsManager.UpdateDisplayXYInOneHandMode(physicalX, physicalY, displayInfo, oneHandScale));
    physicalX = 4;
    physicalY = 4;
    EXPECT_NO_FATAL_FAILURE(
        inputWindowsManager.UpdateDisplayXYInOneHandMode(physicalX, physicalY, displayInfo, oneHandScale));
    physicalX = 11;
    physicalY = 4;
    EXPECT_NO_FATAL_FAILURE(
        inputWindowsManager.UpdateDisplayXYInOneHandMode(physicalX, physicalY, displayInfo, oneHandScale));
    physicalX = 6;
    physicalY = 4;
    EXPECT_NO_FATAL_FAILURE(
        inputWindowsManager.UpdateDisplayXYInOneHandMode(physicalX, physicalY, displayInfo, oneHandScale));
}

/**
 * @tc.name: InputWindowsManagerTest_HandleOneHandMode_001
 * @tc.desc: Test HandleOneHandMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandleOneHandMode_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem pointerItem;
    pointerItem.pointerId_ = 2;
    pointerItem.canceled_ = true;
    pointerItem.pressed_ = true;
    OLD::DisplayInfo displayInfo;
    displayInfo.oneHandY = 5;
    displayInfo.oneHandX = 5;
    displayInfo.width = 5;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.HandleOneHandMode(displayInfo, pointerEvent, pointerItem));
}
#endif // OHOS_BUILD_ENABLE_ONE_HAND_MODE

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
/**
 * @tc.name: InputWindowsManagerTest_DrawPointer_001
 * @tc.desc: Test DrawPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DrawPointer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool isDisplayRemoved = true;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DrawPointer(isDisplayRemoved));
}
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING

/**
 * @tc.name: InputWindowsManagerTest_UpdateCaptureMode_003
 * @tc.desc: Test UpdateCaptureMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateCaptureMode_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayGroupInfo displayGroupInfo;
    WindowInfo windowInfo;
    inputWindowsManager->captureModeInfo_.isCaptureMode = true;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.focusWindowId = 25;
    }
    displayGroupInfo.focusWindowId = 26;
    windowInfo.id = 10;
    it->second.windowsInfo.push_back(windowInfo);
    displayGroupInfo.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateCaptureMode(displayGroupInfo));

    inputWindowsManager->captureModeInfo_.isCaptureMode = false;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateCaptureMode(displayGroupInfo));
}

#ifdef OHOS_BUILD_ENABLE_POINTER
/**
 * @tc.name: InputWindowsManagerTest_NotifyPointerToWindow_002
 * @tc.desc: Test NotifyPointerToWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_NotifyPointerToWindow_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    UDSServer udsServer;
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    inputWindowsManager->lastPointerEvent_->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    inputWindowsManager->lastPointerEvent_->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->NotifyPointerToWindow());

    inputWindowsManager->lastPointerEvent_->pressedButtons_.clear();
    inputWindowsManager->lastPointerEvent_->bitwise_ = InputEvent::EVENT_FLAG_SIMULATE;
    PointerEvent::PointerItem pointerItem;
    pointerItem.pointerId_ = 100;
    pointerItem.canceled_ = true;
    pointerItem.pressed_ = true;
    inputWindowsManager->lastPointerEvent_->SetPointerId(pointerItem.pointerId_ + 1);
    inputWindowsManager->lastPointerEvent_->pointers_.push_back(pointerItem);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->NotifyPointerToWindow());

    inputWindowsManager->lastPointerEvent_->SetPointerId(pointerItem.pointerId_);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->NotifyPointerToWindow());
}

/**
 * @tc.name: InputWindowsManagerTest_NotifyPointerToWindow_003
 * @tc.desc: Test NotifyPointerToWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_NotifyPointerToWindow_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    UDSServer udsServer;
    inputWindowsManager.lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager.lastPointerEvent_, nullptr);
    inputWindowsManager.lastLogicX_ = 200;
    inputWindowsManager.lastLogicY_ = 300;
    WindowInfo windowInfo;
    windowInfo.flags = WindowInfo::FLAG_BIT_HANDWRITING;
    windowInfo.zOrder = 5.0f;
    windowInfo.pointerHotAreas.push_back({ 100, 100, 300, 300 });
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    windowInfo.id = 10;
    windowInfo.zOrder = 4.0f;
    inputWindowsManager.lastWindowInfo_ = windowInfo;
    inputWindowsManager.NotifyPointerToWindow();
    EXPECT_TRUE(MMI_GNE(inputWindowsManager.lastWindowInfo_.zOrder, windowInfo.zOrder));

    windowInfo.id = 20;
    windowInfo.zOrder = 3.0f;
    auto iter = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (iter != inputWindowsManager.displayGroupInfoMap_.end()) {
        it->second.windowsInfo.clear();
        iter->second.windowsInfo.push_back(windowInfo);
    }
    inputWindowsManager.NotifyPointerToWindow();
    EXPECT_TRUE(MMI_EQ(inputWindowsManager.lastWindowInfo_.zOrder, windowInfo.zOrder));
}
#endif // OHOS_BUILD_ENABLE_POINTER

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)

/**
 * @tc.name: InputWindowsManagerTest_PointerDrawingManagerOnDisplayInfo_002
 * @tc.desc: Test PointerDrawingManagerOnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PointerDrawingManagerOnDisplayInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayGroupInfo displayGroupInfo;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 521;
    displayInfo.uniq = "uniq_test";
    displayInfo.dpi = 1000;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    displayInfo.dpi++;
    displayInfo.x = 300;
    displayInfo.y = 300;
    inputWindowsManager->lastDpi_ = displayInfo.dpi + 1;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displayInfo);
    }

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_002
 * @tc.desc: Test UpdatePointerDrawingManagerWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdatePointerDrawingManagerWindowInfo());

    OLD::DisplayInfo displaysInfo;
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    inputWindowsManager->lastPointerEvent_->SetButtonPressed(1);
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdatePointerDrawingManagerWindowInfo());
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_003
 * @tc.desc: Test UpdatePointerDrawingManagerWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    OLD::DisplayInfo displaysInfo;
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdatePointerDrawingManagerWindowInfo());
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_004
 * @tc.desc: Test UpdatePointerDrawingManagerWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    OLD::DisplayInfo displaysInfo;
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_UP);
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdatePointerDrawingManagerWindowInfo());
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_005
 * @tc.desc: Test UpdatePointerDrawingManagerWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    OLD::DisplayInfo displaysInfo;
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdatePointerDrawingManagerWindowInfo());
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_006
 * @tc.desc: Test UpdatePointerDrawingManagerWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerDrawingManagerWindowInfo_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    OLD::DisplayInfo displaysInfo;
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdatePointerDrawingManagerWindowInfo());
}

#endif // defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayIdAndName_004
 * @tc.desc: Test updating display ID and name
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayIdAndName_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 1;
    displaysInfo.uniq = "abc";
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    it->second.displaysInfo.push_back(displaysInfo);

    inputWindowsManager->bindInfo_.AddDisplay(1, "abc");
    inputWindowsManager->bindInfo_.AddDisplay(1, "abc");
    inputWindowsManager->bindInfo_.AddDisplay(2, "abc");
    inputWindowsManager->bindInfo_.AddDisplay(1, "aaa");
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateDisplayIdAndName());
}

/* *
 * @tc.name: InputWindowsManagerTest_GetCursorPos_003
 * @tc.desc: Test the function GetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCursorPos_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager->cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.displayId = -1;
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 2;
    displaysInfo.width = 30;
    displaysInfo.height = 40;
    displaysInfo.validWidth = displaysInfo.width;
    displaysInfo.validHeight = displaysInfo.height;
    displaysInfo.name = "name2";
    displaysInfo.uniq = "uniq2";
    auto iter = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (iter != inputWindowsManager->displayGroupInfoMap_.end()) {
        iter->second.displaysInfo.push_back(displaysInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->GetCursorPos());
    EXPECT_NE(cursorPosRef.displayId, displaysInfo.id);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->GetCursorPos());
}

/* *
 * @tc.name: InputWindowsManagerTest_GetCursorPos_004
 * @tc.desc: Test the function GetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCursorPos_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager->cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.displayId = -1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->GetCursorPos());
}

/* *
 * @tc.name: InputWindowsManagerTest_GetCursorPos_006
 * @tc.desc: Test the function GetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCursorPos_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayGroupInfo displayGroupInfo;
    std::map<int32_t, CursorPosition> cursorPosMap_;
    std::map<int32_t, OLD::DisplayGroupInfo> displayGroupInfoMap_;
    OLD::DisplayInfo displaysInfo = {.id = 2, .validWidth = 30, .validHeight = 40,
        .direction = Direction::DIRECTION90, .displayDirection = Direction::DIRECTION0};
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    displayGroupInfo.displaysInfo[0].displaySourceMode = DisplaySourceMode::SCREEN_MAIN;
    cursorPosMap_[0] = {-1, Direction::DIRECTION0, Direction::DIRECTION0, {0, 0}};
    displayGroupInfoMap_[0] = displayGroupInfo;
    inputWindowsManager->cursorPosMap_ = cursorPosMap_;
    inputWindowsManager->displayGroupInfoMap_ = displayGroupInfoMap_;
 
    CursorPosition cursorPosRef;
    cursorPosRef = inputWindowsManager->GetCursorPos();
    EXPECT_EQ(cursorPosRef.displayId, displaysInfo.id);
    EXPECT_EQ(cursorPosRef.direction, displaysInfo.direction);
    EXPECT_EQ(cursorPosRef.displayDirection, displaysInfo.displayDirection);
    EXPECT_EQ(cursorPosRef.cursorPos.x, displaysInfo.validHeight * HALF_RATIO);
    EXPECT_EQ(cursorPosRef.cursorPos.y, displaysInfo.validWidth * HALF_RATIO);
}

/* *
 * @tc.name: InputWindowsManagerTest_GetCursorPos_007
 * @tc.desc: Test the function GetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCursorPos_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    CursorPosition cursorPosRef;
    OLD::DisplayInfo displaysInfo = {.id = 2, .validWidth = 30, .validHeight = 40,
        .direction = Direction::DIRECTION270, .displayDirection = Direction::DIRECTION0};
    auto iter = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (iter != inputWindowsManager->displayGroupInfoMap_.end()) {
        iter->second.displaysInfo.push_back(displaysInfo);
    }
    cursorPosRef = inputWindowsManager->GetCursorPos();
    EXPECT_EQ(cursorPosRef.displayId, displaysInfo.id);
    EXPECT_EQ(cursorPosRef.direction, displaysInfo.direction);
    EXPECT_EQ(cursorPosRef.displayDirection, displaysInfo.displayDirection);
    EXPECT_EQ(cursorPosRef.cursorPos.x, displaysInfo.validHeight * HALF_RATIO);
    EXPECT_EQ(cursorPosRef.cursorPos.y, displaysInfo.validWidth * HALF_RATIO);
}

/* *
 * @tc.name: InputWindowsManagerTest_GetCursorPos_008
 * @tc.desc: Test the function GetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCursorPos_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    CursorPosition cursorPosRef;
    OLD::DisplayInfo displaysInfo = {.id = 2, .validWidth = 30, .validHeight = 40,
        .direction = Direction::DIRECTION90, .displayDirection = Direction::DIRECTION0};
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    cursorPosRef = inputWindowsManager->GetCursorPos(displayGroupInfo);
    EXPECT_EQ(cursorPosRef.displayId, displaysInfo.id);
    EXPECT_EQ(cursorPosRef.direction, displaysInfo.direction);
    EXPECT_EQ(cursorPosRef.displayDirection, displaysInfo.displayDirection);
    EXPECT_EQ(cursorPosRef.cursorPos.x, displaysInfo.validHeight * HALF_RATIO);
    EXPECT_EQ(cursorPosRef.cursorPos.y, displaysInfo.validWidth * HALF_RATIO);
}

/* *
 * @tc.name: InputWindowsManagerTest_GetCursorPos_009
 * @tc.desc: Test the function GetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCursorPos_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    CursorPosition cursorPosRef;
    OLD::DisplayInfo displaysInfo = {.id = 2, .validWidth = 30, .validHeight = 40,
        .direction = Direction::DIRECTION270, .displayDirection = Direction::DIRECTION0};
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    cursorPosRef = inputWindowsManager->GetCursorPos(displayGroupInfo);
    EXPECT_EQ(cursorPosRef.displayId, displaysInfo.id);
    EXPECT_EQ(cursorPosRef.direction, displaysInfo.direction);
    EXPECT_EQ(cursorPosRef.displayDirection, displaysInfo.displayDirection);
    EXPECT_EQ(cursorPosRef.cursorPos.x, displaysInfo.validHeight * HALF_RATIO);
    EXPECT_EQ(cursorPosRef.cursorPos.y, displaysInfo.validWidth * HALF_RATIO);
}

/* *
 * @tc.name: InputWindowsManagerTest_ResetPointerPositionIfOutValidDisplay_002
 * @tc.desc: Test the function ResetPointerPositionIfOutValidDisplay
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ResetPointerPositionIfOutValidDisplay_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayInfo displaysInfo;
    OLD::DisplayGroupInfo displayGroupInfo;
    displaysInfo.id = 2;
    displaysInfo.width = 30;
    displaysInfo.height = 40;
    displaysInfo.validWidth = displaysInfo.width;
    displaysInfo.validHeight = displaysInfo.height;
    displaysInfo.name = "name2";
    displaysInfo.uniq = "uniq2";
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager->cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.displayId = displaysInfo.id;
    cursorPosRef.cursorPos.x = 300;
    cursorPosRef.cursorPos.y = 300;

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->ResetPointerPositionIfOutValidDisplay(displayGroupInfo));
}

/* *
 * @tc.name: InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_003
 * @tc.desc: Test the function CancelTouchScreenEventIfValidDisplayChange
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(
    InputWindowsManagerTest, InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayGroupInfo displayGroupInfo;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_UNKNOWN);
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager->lastPointerEventforGesture_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));

    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 100;
    pointerEvent->SetTargetDisplayId(displaysInfo.id);
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));
}

/* *
 * @tc.name: InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_004
 * @tc.desc: Test the function CancelTouchScreenEventIfValidDisplayChange
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(
    InputWindowsManagerTest, InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayGroupInfo displayGroupInfo;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_UNKNOWN);
    EXPECT_NE(pointerEvent, nullptr);
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 100;
    pointerEvent->SetTargetDisplayId(displaysInfo.id + 1);
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    inputWindowsManager->lastPointerEventforGesture_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));
}

/* *
 * @tc.name: InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_005
 * @tc.desc: Test the function CancelTouchScreenEventIfValidDisplayChange
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(
    InputWindowsManagerTest, InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayInfo displaysInfo;
    OLD::DisplayGroupInfo displayGroupInfo;
    displaysInfo.id = 2;
    displaysInfo.width = 30;
    displaysInfo.height = 40;
    displaysInfo.validWidth = displaysInfo.width;
    displaysInfo.validHeight = displaysInfo.height;
    displaysInfo.name = "name2";
    displaysInfo.uniq = "uniq2";
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_UNKNOWN);
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager->lastPointerEventforGesture_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));
    pointerEvent->SetTargetDisplayId(displaysInfo.id);
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));
}

/* *
 * @tc.name: InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_006
 * @tc.desc: Test the function CancelTouchScreenEventIfValidDisplayChange
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(
    InputWindowsManagerTest, InputWindowsManagerTest_CancelTouchScreenEventIfValidDisplayChange_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayInfo displaysInfo;
    OLD::DisplayGroupInfo displayGroupInfo;
    displaysInfo.id = 2;
    displaysInfo.width = 30;
    displaysInfo.height = 40;
    displaysInfo.validWidth = displaysInfo.width;
    displaysInfo.validHeight = displaysInfo.height;
    displaysInfo.name = "name2";
    displaysInfo.uniq = "uniq2";
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetTargetDisplayId(0);
    EXPECT_NE(pointerEvent, nullptr);
    inputWindowsManager->lastPointerEventforGesture_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelTouchScreenEventIfValidDisplayChange(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfo_006
 * @tc.desc: Test UpdateDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfo_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayGroupInfo displayGroupInfo;
    OLD::DisplayInfo info;
    info.scalePercent = inputWindowsManager->scalePercent_ + 1;
    displayGroupInfo.displaysInfo.emplace_back(info);
    ASSERT_NO_FATAL_FAILURE(inputWindowsManager->UpdateDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateWindowInfo_002
 * @tc.desc: Test the funcation UpdateWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateWindowInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.focusWindowId = 1;

    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 1;
    windowInfo.uid = 1;
    windowInfo.area = {1, 1, 1, 1};
    windowInfo.defaultHotAreas = {windowInfo.area};
    windowInfo.pointerHotAreas = {windowInfo.area};
    windowInfo.agentWindowId = 1;
    windowInfo.flags = 1;
    windowInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    windowInfo.pointerChangeAreas = {1, 2, 1, 2, 1, 2, 1, 2, 1};
    windowInfo.action = WINDOW_UPDATE_ACTION::ADD;
    displayGroupInfo.windowsInfo.push_back(windowInfo);

#ifdef OHOS_BUILD_ENABLE_ANCO
    NiceMock<MockInputWindowsManager> mockInputWindowsManager;
    EXPECT_CALL(mockInputWindowsManager, IsAncoWindow).WillRepeatedly(Return(true));
#endif // OHOS_BUILD_ENABLE_ANCO
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateWindowInfo(displayGroupInfo));

    displayGroupInfo.windowsInfo.clear();
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateWindowInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_PrintWindowNavbar_001
 * @tc.desc: Test the funcation PrintWindowNavbar
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintWindowNavbar_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    WindowInfo windowInfo;
    windowInfo.id = 20;
    windowInfo.windowInputType = WindowInputType::MIX_BUTTOM_ANTI_AXIS_MOVE;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->PrintWindowNavbar());
}

/**
 * @tc.name: InputWindowsManagerTest_PrintWindowNavbar_002
 * @tc.desc: Test the function PrintWindowNavbar
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintWindowNavbar_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    WindowInfo windowInfo;
    windowInfo.id = 21;
    windowInfo.windowInputType = WindowInputType::DUALTRIGGER_TOUCH;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.windowsInfo.push_back(windowInfo);
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->PrintWindowNavbar());
}

/**
 * @tc.name: InputWindowsManagerTest_CancelMouseEvent_004
 * @tc.desc: Test CancelMouseEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelMouseEvent_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);
    pointerEvent->pointers_.clear();
    inputWindowsManager->lastPointerEvent_ = pointerEvent;
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelMouseEvent());
}

/**
 * @tc.name: InputWindowsManagerTest_CancelMouseEvent_005
 * @tc.desc: Test CancelMouseEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelMouseEvent_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetButtonPressed(PointerEvent::MOUSE_BUTTON_LEFT);
    PointerEvent::PointerItem pointerItem;
    pointerItem.pointerId_ = 0;
    pointerItem.canceled_ = true;
    pointerItem.pressed_ = true;
    pointerEvent->pointers_.push_back(pointerItem);
    inputWindowsManager->lastPointerEvent_ = pointerEvent;
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelMouseEvent());
}

/**
 * @tc.name: InputWindowsManagerTest_CancelMouseEvent_006
 * @tc.desc: Test CancelMouseEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelMouseEvent_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->pressedButtons_.clear();
    inputWindowsManager->lastPointerEvent_ = pointerEvent;
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelMouseEvent());
}

/**
 * @tc.name: InputWindowsManagerTest_CancelMouseEvent_007
 * @tc.desc: Test CancelMouseEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CancelMouseEvent_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_UNKNOWN);
    pointerEvent->pressedButtons_.clear();
    inputWindowsManager->lastPointerEvent_ = pointerEvent;
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelMouseEvent());
}

/**
 * @tc.name: InputWindowsManagerTest_DeviceStatusChanged_001
 * @tc.desc: DeviceStatusChanged
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DeviceStatusChanged_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t deviceId = 0;
    std::string name = "mouse";
    std::string sysUid = "";
    std::string devStatus = "add";
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    ASSERT_NO_FATAL_FAILURE(inputWindowsManager->DeviceStatusChanged(deviceId, name, sysUid, devStatus));

    devStatus = "test";
    ASSERT_NO_FATAL_FAILURE(inputWindowsManager->DeviceStatusChanged(deviceId, name, sysUid, devStatus));
}

#ifdef OHOS_BUILD_ENABLE_TOUCH
/**
 * @tc.name: InputWindowsManagerTest_ReissueCancelTouchEvent_002
 * @tc.desc: Test the funcation ReissueCancelTouchEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReissueCancelTouchEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t pointerId = 100;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    inputWindowsManager->extraData_.pointerId = pointerId;
    PointerEvent::PointerItem item;
    item.SetPointerId(pointerId);
    item.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    item.SetPressed(true);
    WindowInfoEX windowInfoEX;
    windowInfoEX.flag = true;
    inputWindowsManager->touchItemDownInfos_.insert(std::make_pair(pointerId, windowInfoEX));
    pointerEvent->AddPointerItem(item);
    EXPECT_EQ(pointerEvent->pointers_.size(), 1);

    std::shared_ptr<EventNormalizeHandler> handler = std::make_shared<EventNormalizeHandler>();
    NiceMock<MockInputWindowsManager> mockInputWindowsManager;
    EXPECT_CALL(mockInputWindowsManager, GetEventNormalizeHandler).WillRepeatedly(Return(handler));

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->ReissueCancelTouchEvent(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_ReissueCancelTouchEvent_003
 * @tc.desc: Test the funcation ReissueCancelTouchEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ReissueCancelTouchEvent_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t pointerId = 100;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_TOUCHSCREEN;
    inputWindowsManager->extraData_.pointerId = pointerId + 1;
    PointerEvent::PointerItem item;
    item.SetPointerId(pointerId);
    item.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    item.SetPressed(true);
    WindowInfoEX windowInfoEX;
    windowInfoEX.flag = true;
    inputWindowsManager->touchItemDownInfos_.insert(std::make_pair(pointerId, windowInfoEX));
    pointerEvent->AddPointerItem(item);
    EXPECT_EQ(pointerEvent->pointers_.size(), 1);

    std::shared_ptr<EventNormalizeHandler> handler = std::make_shared<EventNormalizeHandler>();
    NiceMock<MockInputWindowsManager> mockInputWindowsManager;
    EXPECT_CALL(mockInputWindowsManager, GetEventNormalizeHandler).WillRepeatedly(Return(handler));

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->ReissueCancelTouchEvent(pointerEvent));
}
#endif // OHOS_BUILD_ENABLE_TOUCH

/**
 * @tc.name: InputWindowsManagerTest_CheckAppFocused_001
 * @tc.desc: Test the funcation CheckAppFocused
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CheckAppFocused_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t pid = 100;
    WindowInfo windowInfo;
    windowInfo.pid = pid;
    windowInfo.id = pid;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        it->second.windowsInfo.clear();
        it->second.windowsInfo.emplace_back(windowInfo);
        it->second.focusWindowId = pid;
    }
    EXPECT_TRUE(inputWindowsManager->CheckAppFocused(pid));

    pid++;
    EXPECT_FALSE(inputWindowsManager->CheckAppFocused(pid));

    it->second.focusWindowId = pid + 1;
    EXPECT_FALSE(inputWindowsManager->CheckAppFocused(pid));
}

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
/**
 * @tc.name: InputWindowsManagerTest_CalculateAcrossDirection_001
 * @tc.desc: Test CalculateAcrossDirection
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_CalculateAcrossDirection_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    Vector2D<double> layout;
    OLD::DisplayInfo displayInfo;
    displayInfo.x = 100;
    displayInfo.validWidth = 2147483600;
    auto ret = inputWindowsManager.CalculateAcrossDirection(displayInfo, layout);
    EXPECT_EQ(ret, AcrossDirection::ACROSS_ERROR);
    displayInfo.x = 100;
    displayInfo.validWidth = 100;
    displayInfo.y = 100;
    displayInfo.validHeight = 2147483600;
    ret = inputWindowsManager.CalculateAcrossDirection(displayInfo, layout);
    EXPECT_EQ(ret, AcrossDirection::ACROSS_ERROR);
    displayInfo.x = 100;
    displayInfo.validWidth = 100;
    displayInfo.y = 100;
    displayInfo.validHeight = 100;
    layout.x = 50;
    ret = inputWindowsManager.CalculateAcrossDirection(displayInfo, layout);
    EXPECT_EQ(ret, AcrossDirection::LEFTWARDS);
    displayInfo.x = -2147483648;
    layout.x = -2147483600;
    displayInfo.y = 100;
    layout.y = 50;
    ret = inputWindowsManager.CalculateAcrossDirection(displayInfo, layout);
    EXPECT_EQ(ret, AcrossDirection::UPWARDS);
    displayInfo.y = -2147483648;
    layout.y = -2147483600;
    ret = inputWindowsManager.CalculateAcrossDirection(displayInfo, layout);
    EXPECT_EQ(ret, AcrossDirection::ACROSS_ERROR);
}

/**
 * @tc.name: InputWindowsManagerTest_HandlePullEvent_001
 * @tc.desc: Test HandlePullEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandlePullEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_CANCEL);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.HandlePullEvent(pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_MOVE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.HandlePullEvent(pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.HandlePullEvent(pointerEvent));
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
/**
 * @tc.name: InputWindowsManagerTest_IsPointInsideSpecialWindow_001
 * @tc.desc: Test that IsPointInsideSpecialWindow should return false when there is no window information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsPointInsideSpecialWindow_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->displayGroupInfo_.windowsInfo.clear();
    EXPECT_FALSE(inputWindowsManager->IsPointInsideSpecialWindow(0, 0));
}

/**
 * @tc.name: InputWindowsManagerTest_IsPointInsideSpecialWindow_002
 * @tc.desc: Test that IsPointInsideSpecialWindow should return false when the window type is not guide window
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsPointInsideSpecialWindow_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    WindowInfo windowInfo;
    windowInfo.windowType = GUIDE_WINDOW_TYPE + 1;
    inputWindowsManager->displayGroupInfo_.windowsInfo.push_back(windowInfo);
    EXPECT_FALSE(inputWindowsManager->IsPointInsideSpecialWindow(0, 0));
}

/**
 * @tc.name: InputWindowsManagerTest_IsPointInsideSpecialWindow_003
 * @tc.desc: Test that IsPointInsideSpecialWindow should return false
    when the window type is guide window, but the point is not within it
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsPointInsideSpecialWindow_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo testWindow;
    testWindow.windowType = GUIDE_WINDOW_TYPE;
    Rect rect = {TEST_WINDOW_START, TEST_WINDOW_START, TEST_WINDOW_END, TEST_WINDOW_END};
    testWindow.defaultHotAreas.push_back(rect);

    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->displayGroupInfo_.windowsInfo.push_back(testWindow);

    bool result = inputWindowsManager->IsPointInsideSpecialWindow(TEST_WINDOW_START -1, TEST_WINDOW_START -1);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_IsPointInsideSpecialWindow_004
 * @tc.desc: Test that IsPointInsideSpecialWindow should return false
    when the window name type is xiaoyi voice input window, but the point is not within it
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsPointInsideSpecialWindow_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo testWindow;
    testWindow.windowNameType = 2;
    testWindow.area.x = TEST_WINDOW_START;
    testWindow.area.y = TEST_WINDOW_START;
    testWindow.area.width = TEST_WINDOW_END;
    testWindow.area.height = TEST_WINDOW_END;

    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->displayGroupInfo_.windowsInfo.push_back(testWindow);

    bool result = inputWindowsManager->IsPointInsideSpecialWindow(TEST_WINDOW_START -1, TEST_WINDOW_START -1);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_IsPointInsideWindowArea_001
 * @tc.desc: Test that IsPointInsideWindowArea should return false
    when the point is not within the window area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsPointInsideWindowArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo testWindow;
    testWindow.area.x = TEST_WINDOW_START;
    testWindow.area.y = TEST_WINDOW_START;
    testWindow.area.width = TEST_WINDOW_END;
    testWindow.area.height = TEST_WINDOW_END;

    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->displayGroupInfo_.windowsInfo.push_back(testWindow);

    bool result = inputWindowsManager->IsPointInsideWindowArea(TEST_WINDOW_START -1, TEST_WINDOW_START -1,\
        inputWindowsManager->displayGroupInfo_.windowsInfo[0]);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: InputWindowsManagerTest_IsPointInsideWindowArea_002
 * @tc.desc: Test that IsPointInsideWindowArea should return true
    when the point is within the window area
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsPointInsideWindowArea_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WindowInfo testWindow;
    testWindow.area.x = TEST_WINDOW_START;
    testWindow.area.y = TEST_WINDOW_START;
    testWindow.area.width = TEST_WINDOW_END;
    testWindow.area.height = TEST_WINDOW_END;

    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->displayGroupInfo_.windowsInfo.push_back(testWindow);

    bool result = inputWindowsManager->IsPointInsideWindowArea(0, 0,
        inputWindowsManager->displayGroupInfo_.windowsInfo[0]);
    EXPECT_TRUE(result);
}
/**
 * @tc.name: InputWindowsManagerTest_IsMouseInCastWindow_001
 * @tc.desc: Test that IsMouseInCastWindow should return false when there is no window information
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsMouseInCastWindow_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    inputWindowsManager->displayGroupInfo_.windowsInfo.clear();
    inputWindowsManager->displayGroupInfo_.windowsInfo.clear();
    EXPECT_FALSE(inputWindowsManager->IsMouseInCastWindow());
}

/**
 * @tc.name: InputWindowsManagerTest_IsMouseInCastWindow_002
 * @tc.desc: Test whether IsMouseInCastWindow returns false when there is window information
 * but no window of the CAST_WINDOW_TYPE type.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsMouseInCastWindow_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    WindowInfo windowInfo;
    windowInfo.windowType = CAST_WINDOW_TYPE + 1;
    inputWindowsManager->displayGroupInfo_.windowsInfo.push_back(windowInfo);
    EXPECT_FALSE(inputWindowsManager->IsMouseInCastWindow());
}

/**
 * @tc.name: InputWindowsManagerTest_IsCaptureMode_001
 * @tc.desc: Test case for IsCaptureMode when screenshot window exists
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsCaptureMode_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    WindowInfo screenshotWindow;
    screenshotWindow.windowNameType = WINDOW_NAME_TYPE_SCREENSHOT;
    inputWindowsManager->displayGroupInfo_.windowsInfo.push_back(screenshotWindow);

    EXPECT_FALSE(inputWindowsManager->IsCaptureMode());
}

/**
 * @tc.name: InputWindowsManagerTest_IsCaptureMode_002
 * @tc.desc: Test case for IsCaptureMode when capture window exists and size does not exceed threshold
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsCaptureMode_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    WindowInfo captureWindow;
    captureWindow.zOrder = SCREEN_CAPTURE_WINDOW_ZORDER;
    captureWindow.area.width = SCREEN_RECORD_WINDOW_WIDTH - 1;
    captureWindow.area.height = SCREEN_RECORD_WINDOW_HEIGHT - 1;
    inputWindowsManager->displayGroupInfo_.windowsInfo.push_back(captureWindow);

    EXPECT_FALSE(inputWindowsManager->IsCaptureMode());
}

/**
 * @tc.name: InputWindowsManagerTest_IsCaptureMode_003
 * @tc.desc: Test case for IsCaptureMode when no special windows exist
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsCaptureMode_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    WindowInfo normalWindow;
    normalWindow.windowNameType = WINDOW_NAME_TYPE_SCREENSHOT + 1;
    normalWindow.zOrder = SCREEN_CAPTURE_WINDOW_ZORDER - 1;
    inputWindowsManager->displayGroupInfo_.windowsInfo.push_back(normalWindow);

    EXPECT_FALSE(inputWindowsManager->IsCaptureMode());
}

/**
 * @tc.name: InputWindowsManagerTest_SetFoldState_001
 * @tc.desc: Test SetFoldState
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetFoldState_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    EXPECT_NO_FATAL_FAILURE(manager.SetFoldState());
}

/**
 * @tc.name: InputWindowsManagerTest_DumpDisplayInfo_001
 * @tc.desc: Test DumpDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DumpDisplayInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    int32_t fd = 1;
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 2;
    displaysInfo.width = 30;
    displaysInfo.height = 40;
    displaysInfo.validWidth = displaysInfo.width;
    displaysInfo.validHeight = displaysInfo.height;
    displaysInfo.name = "name2";
    displaysInfo.uniq = "uniq2";
    std::vector<OLD::DisplayInfo> displaysInfos;
    displaysInfos.push_back(displaysInfo);
    EXPECT_NO_FATAL_FAILURE(manager.DumpDisplayInfo(fd, displaysInfos));
}

/**
 * @tc.name: InputWindowsManagerTest_PrintZorderInfo_001
 * @tc.desc: Test PrintZorderInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PrintZorderInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    WindowInfo testWindow;
    testWindow.id = 1;
    testWindow.pid = 100;
    testWindow.uid = 200;
    testWindow.area = {0, 0, 800, 600};
    testWindow.defaultHotAreas = {
        {10,  10,  100, 100},
        {200, 200, 50,  50 }
    };
    testWindow.pointerHotAreas = {
        {30,  30,  150, 150},
        {400, 400, 70,  70 }
    };
    testWindow.agentWindowId = 10;
    testWindow.flags = 1;
    testWindow.displayId = 3;
    testWindow.zOrder = 4.0f;
    testWindow.pointerChangeAreas = {10, 20, 30};
    testWindow.transform = {1.0f, 2.0f, 3.0f};
    std::string windowPrint;
    EXPECT_NO_FATAL_FAILURE(manager.PrintZorderInfo(testWindow, windowPrint));
}

/**
 * @tc.name: InputWindowsManagerTest_ChangeWindowArea_001
 * @tc.desc: Test ChangeWindowArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ChangeWindowArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    WindowInfo testWindow;
    testWindow.id = 1;
    testWindow.pid = 100;
    testWindow.uid = 200;
    testWindow.area = {0, 0, 800, 600};
    testWindow.defaultHotAreas = {
        {10,  10,  100, 100},
        {200, 200, 50,  50 }
    };
    testWindow.pointerHotAreas = {
        {30,  30,  150, 150},
        {400, 400, 70,  70 }
    };
    testWindow.agentWindowId = 10;
    testWindow.flags = 1;
    testWindow.displayId = 3;
    testWindow.zOrder = 4.0f;
    testWindow.pointerChangeAreas = {10, 20, 30};
    testWindow.transform = {1.0f, 2.0f, 3.0f};
    int32_t x = 20;
    int32_t y = 20;
    EXPECT_NO_FATAL_FAILURE(manager.ChangeWindowArea(x, y, testWindow));
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowStateNotifyPid_001
 * @tc.desc: Test GetWindowStateNotifyPid
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowStateNotifyPid_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    EXPECT_NO_FATAL_FAILURE(manager.GetWindowStateNotifyPid());
}

/**
 * @tc.name: InputWindowsManagerTest_ProcessInjectEventGlobalXY_001
 * @tc.desc: Test ProcessInjectEventGlobalXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ProcessInjectEventGlobalXY_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ;
    int32_t useCoordinate = 1;
    EXPECT_NO_FATAL_FAILURE(manager.ProcessInjectEventGlobalXY(pointerEvent, useCoordinate));
}

/**
 * @tc.name: InputWindowsManagerTest_SendBackCenterPointerEevent_001
 * @tc.desc: Test SendBackCenterPointerEevent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SendBackCenterPointerEevent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    CursorPosition cursorPos = manager.GetCursorPos();
    EXPECT_NO_FATAL_FAILURE(manager.SendBackCenterPointerEevent(cursorPos));
}

/**
 * @tc.name: InputWindowsManagerTest_DestroyInstance_001
 * @tc.desc: Test DestroyInstance
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DestroyInstance_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    WIN_MGR->DestroyInstance();
    EXPECT_EQ(WIN_MGR, nullptr);
}

/**
 * @tc.name: InputWindowsManagerTest_GetDisplayMode_001
 * @tc.desc: Test GetDisplayMode
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetDisplayMode_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    auto ret = manager.GetDisplayMode();
    EXPECT_EQ(static_cast<int32_t>(ret), 0);
}

/**
 * @tc.name: InputWindowsManagerTest_GetLaunchDragPid_001
 * @tc.desc: Test GetLaunchDragPid
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetLaunchDragPid_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    auto ret = manager.GetLaunchDragPid();
    EXPECT_EQ(static_cast<int32_t>(ret), 0);
}
#endif // OHOS_BUILD_ENABLE_VKEYBOARD

/**
 * @tc.name: InputWindowsManagerTest_GetWindowAndDisplayInfo_002
 * @tc.desc: Test GetWindowAndDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowAndDisplayInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t windowId = 0;
    int32_t displayId = 0;
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->GetWindowAndDisplayInfo(windowId, displayId));
    auto result = WIN_MGR->GetWindowAndDisplayInfo(windowId, displayId);
    EXPECT_EQ(result->id, windowId);
    windowId = -1;
    displayId = 1;
    result = WIN_MGR->GetWindowAndDisplayInfo(windowId, displayId);
    EXPECT_FALSE(result.has_value());
    windowId = 1;
    displayId = 0;
    result = WIN_MGR->GetWindowAndDisplayInfo(windowId, displayId);
    EXPECT_TRUE(result.has_value());
}

/**
 * @tc.name: InputWindowsManagerTest_GetMouseInfo_003
 * @tc.desc: Test the GetMouseInfo method
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetMouseInfo_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    OLD::DisplayGroupInfo displayGroupInfo;
    MouseLocation mouseLocation;
    displayGroupInfo.displaysInfo.clear();
    MouseLocation result = WIN_MGR->GetMouseInfo();
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 10;
    displayInfo.width = 192;
    displayInfo.height = 108;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    mouseLocation.displayId = 0;
    result = WIN_MGR->GetMouseInfo();
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    mouseLocation.displayId = -1;
    MouseLocation expectedResult;
    expectedResult.displayId = 10;
    expectedResult.physicalX = 60;
    expectedResult.physicalY = 40;
    ASSERT_NO_FATAL_FAILURE(WIN_MGR->GetMouseInfo());
}

/**
 * @tc.name: InputWindowsManagerTest_GetCursorPos_005
 * @tc.desc: Test the functionality of getting the cursor position
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCursorPos_005, TestSize.Level1)
{
    InputWindowsManager manager;
    manager.cursorPosMap_[DEFAULT_GROUP_ID].displayId = -1;
    manager.displayGroupInfo_.displaysInfo.push_back({0, 800, 600});
    CursorPosition result = WIN_MGR->GetCursorPos();
    EXPECT_NE(result.displayId, RET_ERR);
    EXPECT_EQ(result.cursorPos.x, RET_OK);
    EXPECT_EQ(result.cursorPos.y, RET_OK);
    manager.cursorPosMap_[DEFAULT_GROUP_ID].displayId = 1;
    manager.displayGroupInfo_.displaysInfo.push_back({1, 800, 600});
    result = WIN_MGR->GetCursorPos();
    EXPECT_NE(result.displayId, RET_ERR);
    EXPECT_EQ(result.cursorPos.x, RET_OK);
    EXPECT_EQ(result.cursorPos.y, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_ResetCursorPos_003
 * @tc.desc: Test the functionality of resetting cursor position
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ResetCursorPos_003, TestSize.Level1)
{
    InputWindowsManager manager;
    manager.displayGroupInfo_.displaysInfo.push_back({1, 800, 600});
    CursorPosition result = WIN_MGR->ResetCursorPos();
    EXPECT_NE(result.displayId, RET_ERR);
    EXPECT_EQ(result.cursorPos.x, 0);
    EXPECT_EQ(result.cursorPos.y, 0);
    manager.displayGroupInfo_.displaysInfo.clear();
    result = WIN_MGR->ResetCursorPos();
    EXPECT_NE(result.displayId, RET_ERR);
    EXPECT_EQ(result.cursorPos.x, 0);
    EXPECT_EQ(result.cursorPos.y, 0);
}

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)

/**
 * @tc.name: InputWindowsManagerTest_PointerDrawingManagerOnDisplayInfo004
 * @tc.desc: Test PointerDrawingManagerOnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PointerDrawingManagerOnDisplayInfo004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto *pointerDrawingMgr = static_cast<PointerDrawingManager *>(IPointerDrawingManager::GetInstance());
    pointerDrawingMgr->displayInfo_.id = 521;
    OLD::DisplayGroupInfo displayGroupInfo;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 521;
    displayInfo.uniq = "uniq_test";
    displayInfo.dpi = 1000;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    int32_t deviceId = 1;
    InputDeviceManager::InputDeviceInfo inputDeviceInfo;
    inputDeviceInfo.isPointerDevice = true;
    INPUT_DEV_MGR->inputDevice_.insert(std::make_pair(deviceId, inputDeviceInfo));
    WIN_MGR->mouseLocation_.displayId = 10;
    WIN_MGR->mouseLocation_.physicalX = 500;
    WIN_MGR->mouseLocation_.physicalY = 500;
    WIN_MGR->cursorPosMap_[DEFAULT_GROUP_ID].displayId = 10;
    WIN_MGR->cursorPosMap_[DEFAULT_GROUP_ID].cursorPos.x = 300;
    WIN_MGR->cursorPosMap_[DEFAULT_GROUP_ID].cursorPos.y = 300;
    displayInfo.id = 10;
    displayInfo.x = 300;
    displayInfo.y = 300;
    WIN_MGR->displayGroupInfo_.displaysInfo.push_back(displayInfo);
    WIN_MGR->lastPointerEvent_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
    WIN_MGR->lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(WIN_MGR->lastPointerEvent_, nullptr);
    WIN_MGR->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
    WIN_MGR->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_PointerDrawingManagerOnDisplayInfo_001
 * @tc.desc: Test PointerDrawingManagerOnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_PointerDrawingManagerOnDisplayInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto *pointerDrawingMgr = static_cast<PointerDrawingManager *>(IPointerDrawingManager::GetInstance());
    pointerDrawingMgr->displayInfo_.id = 521;
    OLD::DisplayGroupInfo displayGroupInfo;
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 521;
    displayInfo.uniq = "uniq_test";
    displayInfo.dpi = 1000;
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    int32_t deviceId = 1;
    InputDeviceManager::InputDeviceInfo inputDeviceInfo;
    inputDeviceInfo.isPointerDevice = true;
    INPUT_DEV_MGR->inputDevice_.insert(std::make_pair(deviceId, inputDeviceInfo));
    WIN_MGR->mouseLocation_.displayId = 10;
    WIN_MGR->mouseLocation_.physicalX = 500;
    WIN_MGR->mouseLocation_.physicalY = 500;
    WIN_MGR->cursorPosMap_[DEFAULT_GROUP_ID].displayId = 10;
    WIN_MGR->cursorPosMap_[DEFAULT_GROUP_ID].cursorPos.x = 300;
    WIN_MGR->cursorPosMap_[DEFAULT_GROUP_ID].cursorPos.y = 300;
    displayInfo.id = 10;
    displayInfo.x = 300;
    displayInfo.y = 300;
    WIN_MGR->displayGroupInfo_.displaysInfo.push_back(displayInfo);
    WIN_MGR->lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(WIN_MGR->lastPointerEvent_, nullptr);
    WIN_MGR->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_ENTER_WINDOW);
    WIN_MGR->lastPointerEvent_->SetTargetDisplayId(-1);
    WIN_MGR->firstBtnDownWindowInfo_.first = -1;
    WIN_MGR->extraData_.appended = false;
    WIN_MGR->extraData_.sourceType = PointerEvent::SOURCE_TYPE_JOYSTICK;
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 1;
    WIN_MGR->displayGroupInfo_.windowsInfo.push_back(windowInfo);
    WIN_MGR->isDragBorder_ = true;
    WIN_MGR->dragFlag_ = true;
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
    WIN_MGR->isDragBorder_ = false;
    WIN_MGR->dragFlag_ = false;
    WIN_MGR->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
    WIN_MGR->lastPointerEvent_->SetButtonPressed(1);
    WIN_MGR->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    EXPECT_NO_FATAL_FAILURE(WIN_MGR->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
}

#endif // defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayIdAndName_005
 * @tc.desc: Test updating display ID and name
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayIdAndName_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 1;
    displaysInfo.uniq = "abc";
    inputWindowsManager.displayGroupInfo_.displaysInfo.push_back(displaysInfo);
    inputWindowsManager.bindInfo_.AddDisplay(2, "cde");
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayIdAndName());
}

/**
 * @tc.name: InputWindowsManagerTest_HandleWindowPositionChange
 * @tc.desc: Test the funcation HandleWindowPositionChange
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_HandleWindowPositionChange, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    WindowInfoEX winInfoEx;
    int32_t pointerId = 10;
    WindowInfo winInfo;
    inputWindowsManager.lastPointerEventforWindowChange_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager.lastPointerEventforWindowChange_, nullptr);
    winInfo.id = 100;
    winInfoEx.window.id = 50;
    winInfo.rectChangeBySystem = true;
    inputWindowsManager.lastPointerEventforWindowChange_->bitwise_ = 0x00000080;
    inputWindowsManager.displayGroupInfo_.windowsInfo.clear();
    inputWindowsManager.displayGroupInfo_.windowsInfo.push_back(winInfo);
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerId, winInfoEx));
    pointerId = 50;
    winInfoEx.window.id = 100;
    winInfo.rectChangeBySystem = false;
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerId, winInfoEx));
    inputWindowsManager.displayGroupInfo_.windowsInfo.clear();
    inputWindowsManager.displayGroupInfo_.windowsInfo.push_back(winInfo);
    pointerId = 80;
    winInfo.rectChangeBySystem = true;
    inputWindowsManager.displayGroupInfo_.windowsInfo.clear();
    inputWindowsManager.displayGroupInfo_.windowsInfo.push_back(winInfo);
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerId, winInfoEx));
    OLD::DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end())
    {
        displayGroupInfoRef = it->second;
    }
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.HandleWindowPositionChange(displayGroupInfoRef));
}

/**
 * @tc.name: CancelMouseEvent_CancelAction_008
 * @tc.desc: Test CancelMouseEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, CancelMouseEvent_CancelAction_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    inputWindowsManager->lastPointerEvent_ = pointerEvent;
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelMouseEvent());
    EXPECT_EQ(inputWindowsManager->lastPointerEvent_->GetPointerAction(), PointerEvent::POINTER_ACTION_CANCEL);
}

/**
 * @tc.name: CancelMouseEvent_PullCancelAction_009
 * @tc.desc: Test CancelMouseEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, CancelMouseEvent_PullCancelAction_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_CANCEL);
    inputWindowsManager->lastPointerEvent_ = pointerEvent;
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CancelMouseEvent());
    EXPECT_EQ(inputWindowsManager->lastPointerEvent_->GetPointerAction(), PointerEvent::POINTER_ACTION_PULL_CANCEL);
}

/**
 * @tc.name: InputWindowsManagerTest_GetMouseInfo_004
 * @tc.desc: Test the function GetMouseInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetMouseInfo_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.mouseLocation_.displayId = -1;
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 2;
    displaysInfo.width = 20;
    displaysInfo.height = 30;
    displaysInfo.validWidth = displaysInfo.width;
    displaysInfo.validHeight = displaysInfo.height;
    displaysInfo.name = "name1";
    displaysInfo.uniq = "uniq1";
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end())
    {
        it->second.displaysInfo.push_back(displaysInfo);
    }

    inputWindowsManager.mouseLocationMap_.clear();
    EXPECT_EQ(inputWindowsManager.mouseLocationMap_.size(), 0);

    MouseLocation result = inputWindowsManager.GetMouseInfo();
    EXPECT_EQ(result.displayId, -1);
    EXPECT_EQ(result.physicalX, 0);
    EXPECT_EQ(result.physicalY, 0);
}

#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING
/**
 * @tc.name: InputWindowsManagerTest_AdjustDragPosition_002
 * @tc.desc: Test AdjustDragPosition
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AdjustDragPosition_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    InputWindowsManager inputWindowsManager;
    inputWindowsManager.lastPointerEvent_ = pointerEvent;
    inputWindowsManager.lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 1;
    windowInfo.uid = 1;
    windowInfo.area = {1, 1, 1, 1};
    windowInfo.defaultHotAreas = {windowInfo.area};
    windowInfo.pointerHotAreas = {windowInfo.area};
    windowInfo.agentWindowId = 1;
    windowInfo.flags = 1;
    windowInfo.transform = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    windowInfo.pointerChangeAreas = {1, 2, 1, 2, 1, 2, 1, 2, 1};
    windowInfo.action = WINDOW_UPDATE_ACTION::ADD;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end())
    {
        it->second.windowsInfo.push_back(windowInfo);
    }
    inputWindowsManager.axisBeginWindowInfo_ = std::make_optional(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.AdjustDragPosition());
}
#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING

#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
/**
 * @tc.name: InputWindowsManagerTest_ResetCursorPos_004
 * @tc.desc: Test ResetCursorPos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ResetCursorPos_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    OLD::DisplayInfo displaysInfo;
    OLD::DisplayGroupInfo displayGroupInfo;
    displaysInfo.id = 2;
    displaysInfo.width = 30;
    displaysInfo.height = 40;
    displaysInfo.validWidth = displaysInfo.width;
    displaysInfo.validHeight = displaysInfo.height;
    displaysInfo.name = "name2";
    displaysInfo.uniq = "uniq2";
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    EXPECT_EQ(inputWindowsManager.ResetCursorPos(displayGroupInfo).displayId, displaysInfo.id);
}
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH

/**
 * @tc.name: InputWindowsManagerTest_IsPointerOnCenter_001
 * @tc.desc: Test is pointer on center
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsPointerOnCenter_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayInfo displayInfo;
    displayInfo.id = 1;
    displayInfo.validWidth = 1;
    displayInfo.validHeight = 1;
    CursorPosition currentPos;
    currentPos.cursorPos.x = 0.5;
    currentPos.cursorPos.y = 0.5;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsPointerOnCenter(currentPos, displayInfo));
    currentPos.cursorPos.x = 1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsPointerOnCenter(currentPos, displayInfo));
    currentPos.cursorPos.x = 0.5;
    currentPos.cursorPos.y = 1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsPointerOnCenter(currentPos, displayInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_DispatchPointer_003
 * @tc.desc: Test DispatchPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DispatchPointer_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_ENTER_WINDOW;
    UDSServer udsServer;
    inputWindowsManager.udsServer_ = &udsServer;
    inputWindowsManager.lastPointerEvent_ = PointerEvent::Create();
    EXPECT_NE(inputWindowsManager.lastPointerEvent_, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDisplayX(100);
    item.SetDisplayY(100);
    inputWindowsManager.lastPointerEvent_->AddPointerItem(item);
    inputWindowsManager.lastPointerEvent_->SetPointerId(0);
    inputWindowsManager.firstBtnDownWindowInfo_.first = 1;
    inputWindowsManager.lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    inputWindowsManager.lastPointerEvent_->SetTargetDisplayId(-1);
    inputWindowsManager.lastPointerEvent_->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    inputWindowsManager.lastPointerEvent_->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    inputWindowsManager.lastPointerEvent_->HasFlag(InputEvent::EVENT_FLAG_SIMULATE);
    EXPECT_EQ(inputWindowsManager.lastPointerEvent_->GetSourceType(), 1);
    EXPECT_EQ(inputWindowsManager.IsMouseSimulate(), true);
    GlobalCoords globalCoords = inputWindowsManager.DisplayCoords2GlobalCoords({item.GetDisplayX(),
                                                                                item.GetDisplayY()},
                                                                               inputWindowsManager.lastPointerEvent_->GetTargetDisplayId());
    item.SetGlobalX(globalCoords.x);
    item.SetGlobalY(globalCoords.y);
    WindowInfo windowInfo;
    windowInfo.id = 1;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end())
    {
        it->second.windowsInfo.push_back(windowInfo);
    }
    inputWindowsManager.lastWindowInfo_.id = 2;
    inputWindowsManager.lastWindowInfo_.agentWindowId = 2;
    inputWindowsManager.lastWindowInfo_.area.x = 100;
    inputWindowsManager.lastWindowInfo_.area.y = 100;
    inputWindowsManager.lastPointerEvent_->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    inputWindowsManager.lastPointerEvent_->SetDeviceId(5);
    inputWindowsManager.extraData_.appended = true;
    inputWindowsManager.extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    InputHandler->eventFilterHandler_ = std::make_shared<EventFilterHandler>();
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointer(pointerAction));
    inputWindowsManager.lastWindowInfo_.id = 1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointer(pointerAction));
    inputWindowsManager.extraData_.appended = false;
    pointerAction = PointerEvent::POINTER_ACTION_LEAVE_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.DispatchPointer(pointerAction));
}

/**
 * @tc.name: InputWindowsManagerTest_GetDefaultDisplayGroupInfo_001
 * @tc.desc: Test GetDefaultDisplayGroupInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetDefaultDisplayGroupInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;

    OLD::DisplayGroupInfo defaultGroup;
    defaultGroup.type = GroupType::GROUP_DEFAULT;
    InputWindowsManager manager;
    manager.displayGroupInfoMap_[0] = defaultGroup;
    OLD::DisplayGroupInfo result = manager.GetDefaultDisplayGroupInfo();
    EXPECT_EQ(result.type, GroupType::GROUP_DEFAULT);
}

/**
 * @tc.name: InputWindowsManagerTest_GetDefaultDisplayGroupInfo
 * @tc.desc: Test GetDefaultDisplayGroupInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetDefaultDisplayGroupInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.groupId = 0;
    displayGroupInfo.type = GroupType::GROUP_SPECIAL;
    displayGroupInfo.focusWindowId = -1;
    manager.displayGroupInfoMap_[0] = displayGroupInfo;
    auto info = manager.GetDefaultDisplayGroupInfo();
    ASSERT_EQ(info.groupId, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_InitDisplayGroupInfo
 * @tc.desc: Test InitDisplayGroupInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InitDisplayGroupInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    OLD::DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.groupId = 0;
    displayGroupInfo.type = GroupType::GROUP_DEFAULT;
    displayGroupInfo.focusWindowId = -1;
    EXPECT_NO_FATAL_FAILURE(manager.InitDisplayGroupInfo(displayGroupInfo));
    displayGroupInfo.groupId = 1;
    displayGroupInfo.type = GroupType::GROUP_DEFAULT;
    displayGroupInfo.focusWindowId = -1;
    EXPECT_NO_FATAL_FAILURE(manager.InitDisplayGroupInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_ResetPointerPosition_001
 * @tc.desc: Test ResetPointerPosition
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ResetPointerPosition_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputManager;
    OLD::DisplayGroupInfo displayGroupInfo;
    std::map<int32_t, CursorPosition> cursorPosMap_;
    std::map<int32_t, OLD::DisplayGroupInfo> displayGroupInfoMap_;
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 1;
    displaysInfo.x = 0;
    displaysInfo.y = 0;
    displaysInfo.width = 1024;
    displaysInfo.height = 768;
    displaysInfo.dpi = 160;
    displaysInfo.name = "Display1";
    displaysInfo.uniq = "default0";
    displaysInfo.direction = Direction::DIRECTION0;
    displaysInfo.displayDirection = Direction::DIRECTION0;
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    OLD::DisplayInfo displaysInfo2;
    displaysInfo2.id = 2;
    displaysInfo2.x = 0;
    displaysInfo2.y = 0;
    displaysInfo2.width = 1024;
    displaysInfo2.height = 768;
    displaysInfo2.dpi = 160;
    displaysInfo2.name = "Display2";
    displaysInfo2.uniq = "default1";
    displaysInfo2.direction = Direction::DIRECTION0;
    displaysInfo2.displayDirection = Direction::DIRECTION0;
    displayGroupInfo.displaysInfo.push_back(displaysInfo2);
    displayGroupInfo.displaysInfo[0].displaySourceMode = DisplaySourceMode::SCREEN_MAIN;
    displayGroupInfo.displaysInfo[1].displaySourceMode = DisplaySourceMode::SCREEN_MAIN;
    cursorPosMap_[1] = {1, Direction::DIRECTION0, Direction::DIRECTION0, {512, 384}};
    cursorPosMap_[2] = {2, Direction::DIRECTION0, Direction::DIRECTION0, {512, 384}};
    displayGroupInfoMap_[1] = displayGroupInfo;
    displayGroupInfoMap_[2] = displayGroupInfo;
    displayGroupInfo.displaysInfo[0].rsId = 10;
    displayGroupInfo.displaysInfo[0].validWidth = 1024;
    displayGroupInfo.displaysInfo[0].validHeight = 768;
    cursorPosMap_[1].cursorPos.x = 512;
    cursorPosMap_[1].cursorPos.y = 384;
    inputManager.cursorPosMap_ = cursorPosMap_;
    inputManager.displayGroupInfoMap_ = displayGroupInfoMap_;
    EXPECT_NO_FATAL_FAILURE(inputManager.ResetPointerPosition(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowGroupInfoByDisplayIdCopy_001
 * @tc.desc: Test GetWindowGroupInfoByDisplayIdCopy
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowGroupInfoByDisplayIdCopy_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    int32_t displayId = 1;
    WindowGroupInfo windowGroupInfo;
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.displayId = displayId;
    windowGroupInfo.windowsInfo.push_back(windowInfo);
    manager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    auto ret = manager.GetWindowGroupInfoByDisplayIdCopy(displayId);
    EXPECT_FALSE(!ret.empty());
}

/**
 * @tc.name: InputWindowsManagerTest_GetWindowGroupInfoByDisplayIdCopy_002
 * @tc.desc: Test GetWindowGroupInfoByDisplayIdCopy
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWindowGroupInfoByDisplayIdCopy_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    int32_t displayId = 1;
    WindowGroupInfo windowGroupInfo;
    manager.windowsPerDisplay_.insert(std::make_pair(displayId, windowGroupInfo));
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.displayId = displayId;
    manager.displayGroupInfo_.windowsInfo.push_back(windowInfo);
    auto ret = manager.GetWindowGroupInfoByDisplayIdCopy(displayId);
    EXPECT_FALSE(!ret.empty());
}

/**
 * @tc.name: InputWindowsManagerTest_GetCancelEventFlag_002
 * @tc.desc: Test the funcation GetCancelEventFlag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCancelEventFlag_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfoEX winInfoEx;
    winInfoEx.flag = true;
    int32_t pointerId = 100;
    pointerEvent->SetPointerId(100);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->AddFlag(InputEvent::EVENT_TYPE_AXIS);
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerId, winInfoEx));
    EXPECT_TRUE(inputWindowsManager.GetCancelEventFlag(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_GetCancelEventFlag_003
 * @tc.desc: Test the funcation GetCancelEventFlag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetCancelEventFlag_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfoEX winInfoEx;
    winInfoEx.flag = true;
    int32_t pointerId = 100;
    pointerEvent->SetPointerId(100);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_ACCESSIBILITY);
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(pointerId, winInfoEx));
    EXPECT_TRUE(inputWindowsManager.GetCancelEventFlag(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayIdAndName_006
 * @tc.desc: Test updating display ID and name
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayIdAndName_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager = std::make_shared<InputWindowsManager>();
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 1;
    displaysInfo.rsId = 1010;
    displaysInfo.uniq = "abc";
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end())
    {
        it->second.displaysInfo.push_back(displaysInfo);
    }
    it->second.displaysInfo.push_back(displaysInfo);

    inputWindowsManager->bindInfo_.AddDisplay(1, "abc");
    inputWindowsManager->bindInfo_.AddDisplay(1, "abc");
    inputWindowsManager->bindInfo_.AddDisplay(2, "abc");
    inputWindowsManager->bindInfo_.AddDisplay(1, "aaa");
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateDisplayIdAndName());
}

/**
 * @tc.name: InputWindowsManagerTest_ResetPointerPosition_003
 * @tc.desc: Test ResetPointerPosition
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ResetPointerPosition_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputManager;
    OLD::DisplayGroupInfo displayGroupInfo;
    std::map<int32_t, CursorPosition> cursorPosMap_;
    std::map<int32_t, OLD::DisplayGroupInfo> displayGroupInfoMap_;
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 1;
    displaysInfo.x = 0;
    displaysInfo.y = 0;
    displaysInfo.width = 1024;
    displaysInfo.height = 768;
    displaysInfo.dpi = 160;
    displaysInfo.name = "Display1";
    displaysInfo.uniq = "default0";
    displaysInfo.direction = Direction::DIRECTION0;
    displaysInfo.displayDirection = Direction::DIRECTION0;
    displaysInfo.ppi = 160;
    displaysInfo.screenRealWidth = 1024;
    displaysInfo.screenRealHeight = 768;
    displaysInfo.screenRealDPI = 160;
    displaysInfo.validWidth = 1024;
    displaysInfo.validHeight = 768;
    displaysInfo.fixedDirection = Direction::DIRECTION0;
    displaysInfo.physicalWidth = 0;
    displaysInfo.physicalHeight = 0;
    displaysInfo.pointerActiveWidth = 1;
    displaysInfo.pointerActiveHeight = 1;
    displaysInfo.rsId = 1;
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    OLD::DisplayInfo displaysInfo2;
    displaysInfo2.id = 2;
    displaysInfo2.x = 0;
    displaysInfo2.y = 0;
    displaysInfo2.width = 1024;
    displaysInfo2.height = 768;
    displaysInfo2.dpi = 160;
    displaysInfo2.name = "Display2";
    displaysInfo2.uniq = "default1";
    displaysInfo2.direction = Direction::DIRECTION0;
    displaysInfo2.displayDirection = Direction::DIRECTION0;
    displaysInfo2.ppi = 160;
    displaysInfo2.screenRealWidth = 1024;
    displaysInfo2.screenRealHeight = 768;
    displaysInfo2.screenRealDPI = 160;
    displaysInfo2.validWidth = 1024;
    displaysInfo2.validHeight = 768;
    displaysInfo2.fixedDirection = Direction::DIRECTION0;
    displaysInfo2.physicalWidth = 0;
    displaysInfo2.physicalHeight = 0;
    displaysInfo2.pointerActiveWidth = 2;
    displaysInfo2.pointerActiveHeight = 2;
    displaysInfo2.rsId = 2;
    displayGroupInfo.displaysInfo.push_back(displaysInfo2);
    displayGroupInfo.displaysInfo[0].displaySourceMode = DisplaySourceMode::SCREEN_MAIN;
    displayGroupInfo.displaysInfo[1].displaySourceMode = DisplaySourceMode::SCREEN_MAIN;
    cursorPosMap_[1] = {1, Direction::DIRECTION0, Direction::DIRECTION0, {512, 384}};
    cursorPosMap_[2] = {2, Direction::DIRECTION0, Direction::DIRECTION0, {512, 384}};
    displayGroupInfoMap_[1] = displayGroupInfo;
    displayGroupInfoMap_[2] = displayGroupInfo;
    displayGroupInfo.displaysInfo[0].rsId = 1;
    displayGroupInfo.displaysInfo[0].validWidth = 1024;
    displayGroupInfo.displaysInfo[0].validHeight = 768;
    cursorPosMap_[1].cursorPos.x = 513;
    cursorPosMap_[1].cursorPos.y = 384;
    inputManager.cursorPosMap_ = cursorPosMap_;
    inputManager.displayGroupInfoMap_ = displayGroupInfoMap_;
    EXPECT_NO_FATAL_FAILURE(inputManager.ResetPointerPosition(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_ResetPointerPosition_004
 * @tc.desc: Test ResetPointerPosition
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ResetPointerPosition_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputManager;
    OLD::DisplayGroupInfo displayGroupInfo;
    std::map<int32_t, CursorPosition> cursorPosMap_;
    std::map<int32_t, OLD::DisplayGroupInfo> displayGroupInfoMap_;
    OLD::DisplayInfo displaysInfo;
    displaysInfo.id = 1;
    displaysInfo.x = 0;
    displaysInfo.y = 0;
    displaysInfo.width = 1024;
    displaysInfo.height = 768;
    displaysInfo.dpi = 160;
    displaysInfo.name = "Display1";
    displaysInfo.uniq = "default0";
    displaysInfo.direction = Direction::DIRECTION0;
    displaysInfo.displayDirection = Direction::DIRECTION0;
    displaysInfo.ppi = 160;
    displaysInfo.screenRealWidth = 1024;
    displaysInfo.screenRealHeight = 768;
    displaysInfo.screenRealDPI = 160;
    displaysInfo.validWidth = 1024;
    displaysInfo.validHeight = 768;
    displaysInfo.fixedDirection = Direction::DIRECTION0;
    displaysInfo.physicalWidth = 0;
    displaysInfo.physicalHeight = 0;
    displaysInfo.pointerActiveWidth = 1;
    displaysInfo.pointerActiveHeight = 1;
    displaysInfo.rsId = 1;
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    OLD::DisplayInfo displaysInfo2;
    displaysInfo2.id = 2;
    displaysInfo2.x = 0;
    displaysInfo2.y = 0;
    displaysInfo2.width = 1024;
    displaysInfo2.height = 768;
    displaysInfo2.dpi = 160;
    displaysInfo2.name = "Display2";
    displaysInfo2.uniq = "default1";
    displaysInfo2.direction = Direction::DIRECTION0;
    displaysInfo2.displayDirection = Direction::DIRECTION0;
    displaysInfo2.ppi = 160;
    displaysInfo2.screenRealWidth = 1024;
    displaysInfo2.screenRealHeight = 768;
    displaysInfo2.screenRealDPI = 160;
    displaysInfo2.validWidth = 1024;
    displaysInfo2.validHeight = 768;
    displaysInfo2.fixedDirection = Direction::DIRECTION0;
    displaysInfo2.physicalWidth = 0;
    displaysInfo2.physicalHeight = 0;
    displaysInfo2.pointerActiveWidth = 2;
    displaysInfo2.pointerActiveHeight = 2;
    displaysInfo2.rsId = 2;
    displayGroupInfo.displaysInfo.push_back(displaysInfo2);
    displayGroupInfo.displaysInfo[0].displaySourceMode = DisplaySourceMode::SCREEN_MAIN;
    displayGroupInfo.displaysInfo[1].displaySourceMode = DisplaySourceMode::SCREEN_MAIN;
    cursorPosMap_[1] = {1, Direction::DIRECTION0, Direction::DIRECTION0, {512, 384}};
    cursorPosMap_[2] = {2, Direction::DIRECTION0, Direction::DIRECTION0, {512, 384}};
    displayGroupInfoMap_[1] = displayGroupInfo;
    displayGroupInfoMap_[2] = displayGroupInfo;
    displayGroupInfo.displaysInfo[0].rsId = 1;
    displayGroupInfo.displaysInfo[0].validWidth = 1024;
    displayGroupInfo.displaysInfo[0].validHeight = 768;
    cursorPosMap_[1].cursorPos.x = 512;
    cursorPosMap_[1].cursorPos.y = 384;
    inputManager.cursorPosMap_ = cursorPosMap_;
    inputManager.displayGroupInfoMap_ = displayGroupInfoMap_;
    EXPECT_NO_FATAL_FAILURE(inputManager.ResetPointerPosition(displayGroupInfo));
}

class MockPointerEvent : public PointerEvent
{
public:
    MockPointerEvent() : PointerEvent(PointerEvent::GLOBAL_COORDINATE) {}
    MOCK_METHOD(bool, HasFlag, (uint32_t));
    MOCK_METHOD(bool, GetPointerItem, (int32_t, PointerItem &), (const));
    MOCK_METHOD(void, SetTargetDisplayId, (int32_t));
    MOCK_METHOD(void, UpdatePointerItem, (int32_t, PointerItem &));

    class MockPointerItem : public PointerEvent::PointerItem
    {
    public:
        MOCK_METHOD(int32_t, GetPointerId, (), (const));
        MOCK_METHOD(double, GetGlobalX, (), (const));
        MOCK_METHOD(double, GetGlobalY, (), (const));
        MOCK_METHOD(void, SetDisplayX, (int32_t));
        MOCK_METHOD(void, SetDisplayY, (int32_t));
    };
};

/**
 * @tc.name: InputWindowsManagerTest_ProcessInjectEventGlobalXY_009
 * @tc.desc: Test ProcessInjectEventGlobalXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ProcessInjectEventGlobalXY_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager manager;
    OLD::DisplayGroupInfo defaultGroup;
    defaultGroup.type = GroupType::GROUP_DEFAULT;
    defaultGroup.displaysInfo = {{1, 0, 0, 100, 100}, {2, 150, 150, 100, 100}};
    manager.displayGroupInfoMap_[0] = defaultGroup;

    auto mockPointerEvent = std::make_shared<MockPointerEvent>();
    EXPECT_CALL(*mockPointerEvent, HasFlag(InputEvent::EVENT_FLAG_SIMULATE)).WillRepeatedly(Return(false));
    EXPECT_NO_FATAL_FAILURE(manager.ProcessInjectEventGlobalXY(mockPointerEvent, PointerEvent::GLOBAL_COORDINATE));
}

/**
 * @tc.name: InputWindowsManagerTest_ProcessInjectEventGlobalXY_010
 * @tc.desc: Test ProcessInjectEventGlobalXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ProcessInjectEventGlobalXY_010, TestSize.Level1)
{
    CALL_TEST_DEBUG;

    InputWindowsManager manager;
    OLD::DisplayGroupInfo defaultGroup;
    defaultGroup.type = GroupType::GROUP_DEFAULT;
    defaultGroup.displaysInfo = {{1, 0, 0, 100, 100}, {2, 150, 150, 100, 100}};
    manager.displayGroupInfoMap_[0] = defaultGroup;

    auto mockPointerEvent = std::make_shared<MockPointerEvent>();
    EXPECT_CALL(*mockPointerEvent, HasFlag(InputEvent::EVENT_FLAG_SIMULATE)).WillRepeatedly(Return(true));
    EXPECT_NO_FATAL_FAILURE(manager.ProcessInjectEventGlobalXY(mockPointerEvent, 0));
}

/**
 * @tc.name: InputWindowsManagerTest_ProcessInjectEventGlobalXY_011
 * @tc.desc: Test ProcessInjectEventGlobalXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ProcessInjectEventGlobalXY_011, TestSize.Level1)
{
    CALL_TEST_DEBUG;

    InputWindowsManager manager;
    OLD::DisplayGroupInfo defaultGroup;
    defaultGroup.type = GroupType::GROUP_DEFAULT;
    defaultGroup.displaysInfo = {{1, 0, 0, 100, 100}, {2, 150, 150, 100, 100}};
    manager.displayGroupInfoMap_[0] = defaultGroup;

    auto mockPointerEvent = std::make_shared<MockPointerEvent>();
    EXPECT_CALL(*mockPointerEvent, HasFlag(InputEvent::EVENT_FLAG_SIMULATE)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockPointerEvent, GetPointerItem(::testing::_, ::testing::_)).WillRepeatedly(Return(false));
    EXPECT_NO_FATAL_FAILURE(manager.ProcessInjectEventGlobalXY(mockPointerEvent, PointerEvent::GLOBAL_COORDINATE));
}

/**
 * @tc.name: InputWindowsManagerTest_ProcessInjectEventGlobalXY_012
 * @tc.desc: Test ProcessInjectEventGlobalXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ProcessInjectEventGlobalXY_012, TestSize.Level1)
{
    CALL_TEST_DEBUG;

    InputWindowsManager manager;
    OLD::DisplayGroupInfo defaultGroup;
    defaultGroup.type = GroupType::GROUP_DEFAULT;
    defaultGroup.displaysInfo = {{1, 0, 0, 100, 100}, {2, 150, 150, 100, 100}};
    manager.displayGroupInfoMap_[0] = defaultGroup;

    auto mockPointerEvent = std::make_shared<MockPointerEvent>();
    MockPointerEvent::MockPointerItem mockPointerItem;
    EXPECT_CALL(*mockPointerEvent, HasFlag(InputEvent::EVENT_FLAG_SIMULATE)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockPointerEvent, GetPointerItem(::testing::_, ::testing::_))
        .WillRepeatedly(DoAll(SetArgReferee<1>(::testing::ByRef(mockPointerItem)), Return(true)));
    EXPECT_CALL(mockPointerItem, GetGlobalX()).WillRepeatedly(Return(DBL_MAX));
    EXPECT_CALL(mockPointerItem, GetGlobalY()).WillRepeatedly(Return(50.0));
    EXPECT_EQ(mockPointerItem.GetGlobalY(), 50.0);
    EXPECT_EQ(mockPointerItem.GetGlobalX(), DBL_MAX);
    EXPECT_NO_FATAL_FAILURE(manager.ProcessInjectEventGlobalXY(mockPointerEvent, PointerEvent::GLOBAL_COORDINATE));
}

/**
 * @tc.name: InputWindowsManagerTest_ProcessInjectEventGlobalXY_013
 * @tc.desc: Test ProcessInjectEventGlobalXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ProcessInjectEventGlobalXY_013, TestSize.Level1)
{
    CALL_TEST_DEBUG;

    InputWindowsManager manager;
    OLD::DisplayGroupInfo defaultGroup;
    defaultGroup.type = GroupType::GROUP_DEFAULT;
    defaultGroup.displaysInfo = {{1, 0, 0, 100, 100}, {2, 150, 150, 100, 100}};
    manager.displayGroupInfoMap_[0] = defaultGroup;

    auto mockPointerEvent = std::make_shared<MockPointerEvent>();
    MockPointerEvent::MockPointerItem mockPointerItem;
    EXPECT_CALL(*mockPointerEvent, HasFlag(InputEvent::EVENT_FLAG_SIMULATE)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockPointerEvent, GetPointerItem(::testing::_, ::testing::_))
        .WillRepeatedly(DoAll(SetArgReferee<1>(::testing::ByRef(mockPointerItem)), Return(true)));
    EXPECT_CALL(mockPointerItem, GetGlobalX()).WillRepeatedly(Return(50.0));
    EXPECT_CALL(mockPointerItem, GetGlobalY()).WillRepeatedly(Return(50.0));
    EXPECT_EQ(mockPointerItem.GetGlobalY(), 50.0);
    EXPECT_EQ(mockPointerItem.GetGlobalX(), 50.0);
    EXPECT_NO_FATAL_FAILURE(manager.ProcessInjectEventGlobalXY(mockPointerEvent, PointerEvent::GLOBAL_COORDINATE));
}

/**
 * @tc.name: InputWindowsManagerTest_ProcessInjectEventGlobalXY_014
 * @tc.desc: Test ProcessInjectEventGlobalXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_ProcessInjectEventGlobalXY_014, TestSize.Level1)
{
    CALL_TEST_DEBUG;

    InputWindowsManager manager;
    OLD::DisplayGroupInfo defaultGroup;
    defaultGroup.type = GroupType::GROUP_DEFAULT;
    defaultGroup.displaysInfo = {{1, 0, 0, 100, 100}, {2, 150, 150, 100, 100}};
    manager.displayGroupInfoMap_[0] = defaultGroup;

    auto mockPointerEvent = std::make_shared<MockPointerEvent>();
    MockPointerEvent::MockPointerItem mockPointerItem;
    EXPECT_CALL(*mockPointerEvent, HasFlag(InputEvent::EVENT_FLAG_SIMULATE)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockPointerEvent, GetPointerItem(::testing::_, ::testing::_))
        .WillRepeatedly(DoAll(SetArgReferee<1>(::testing::ByRef(mockPointerItem)), Return(true)));
    EXPECT_CALL(mockPointerItem, GetGlobalX()).WillRepeatedly(Return(200.0));
    EXPECT_CALL(mockPointerItem, GetGlobalY()).WillRepeatedly(Return(200.0));
    EXPECT_EQ(mockPointerItem.GetGlobalY(), 200.0);
    EXPECT_EQ(mockPointerItem.GetGlobalX(), 200.0);
    EXPECT_NO_FATAL_FAILURE(manager.ProcessInjectEventGlobalXY(mockPointerEvent, PointerEvent::GLOBAL_COORDINATE));
}
} // namespace MMI
} // namespace OHOS