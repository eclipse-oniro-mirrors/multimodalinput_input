/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/input.h>

#include "input_windows_manager.h"
#include "mmi_matrix3.h"
#include "mock.h"
#include "window_info.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
using namespace testing;
const std::string PROGRAM_NAME = "uds_session_test";
constexpr int32_t MODULE_TYPE = 1;
constexpr int32_t UDS_FD = 1;
constexpr int32_t UDS_UID = 100;
constexpr int32_t UDS_PID = 100;
constexpr int32_t CAST_INPUT_DEVICEID { 0xAAAAAAFF };
constexpr int32_t CAST_SCREEN_DEVICEID { 0xAAAAAAFE };
} // namespace

class InputWindowsManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase();
    void SetUp() {}
    void TearDown() {}

    static inline std::shared_ptr<MessageParcelMock> messageParcelMock_ = nullptr;
};

void InputWindowsManagerTest::SetUpTestCase(void)
{
    messageParcelMock_ = std::make_shared<MessageParcelMock>();
    MessageParcelMock::messageParcel = messageParcelMock_;
}
void InputWindowsManagerTest::TearDownTestCase()
{
    IInputWindowsManager::instance_.reset();
    IInputWindowsManager::instance_ = nullptr;
    MessageParcelMock::messageParcel = nullptr;
    messageParcelMock_ = nullptr;
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
/**
 * @tc.name: UpdateTarget_001
 * @tc.desc: Test the function UpdateTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateTarget_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateTarget(keyEvent));
}

/**
 * @tc.name: UpdateTarget_002
 * @tc.desc: Test the function UpdateTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateTarget_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(-1));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    displayGroupInfoRef.focusWindowId = 1;
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 11;
    it->second.windowsInfo.push_back(windowInfo);
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateTarget(keyEvent));
    displayGroupInfoRef.focusWindowId = -1;
    displayGroupInfoRef.windowsInfo.clear();
    inputWindowsManager->udsServer_ = nullptr;
}

/**
 * @tc.name: UpdateTarget_003
 * @tc.desc: Test the function UpdateTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateTarget_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(1));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    displayGroupInfoRef.focusWindowId = 1;
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 11;
    it->second.windowsInfo.push_back(windowInfo);
    std::shared_ptr<KeyEvent> keyEvent = KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateTarget(keyEvent));
    displayGroupInfoRef.focusWindowId = -1;
    displayGroupInfoRef.windowsInfo.clear();
    inputWindowsManager->udsServer_ = nullptr;
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
/**
 * @tc.name: PointerDrawingManagerOnDisplayInfo_001
 * @tc.desc: Test the function PointerDrawingManagerOnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, PointerDrawingManagerOnDisplayInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, HasPointerDevice()).WillOnce(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayGroupInfo displayGroupInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: PointerDrawingManagerOnDisplayInfo_002
 * @tc.desc: Test the function PointerDrawingManagerOnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, PointerDrawingManagerOnDisplayInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, HasPointerDevice()).WillOnce(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    DisplayGroupInfo displayGroupInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->lastPointerEvent_.reset();
    inputWindowsManager->lastPointerEvent_ = nullptr;
}

/**
 * @tc.name: PointerDrawingManagerOnDisplayInfo_003
 * @tc.desc: Test the function PointerDrawingManagerOnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, PointerDrawingManagerOnDisplayInfo_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, HasPointerDevice()).WillOnce(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    inputWindowsManager->lastPointerEvent_->SetButtonPressed(1);
    DisplayGroupInfo displayGroupInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->lastPointerEvent_.reset();
    inputWindowsManager->lastPointerEvent_ = nullptr;
}

/**
 * @tc.name: PointerDrawingManagerOnDisplayInfo_004
 * @tc.desc: Test the function PointerDrawingManagerOnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, PointerDrawingManagerOnDisplayInfo_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, HasPointerDevice()).WillOnce(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    DisplayGroupInfo displayGroupInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->lastPointerEvent_.reset();
    inputWindowsManager->lastPointerEvent_ = nullptr;
}

/**
 * @tc.name: PointerDrawingManagerOnDisplayInfo_005
 * @tc.desc: Test the function PointerDrawingManagerOnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, PointerDrawingManagerOnDisplayInfo_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, HasPointerDevice()).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.x = 8;
    displayInfo.y = 8;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 11;
    windowInfo.transform.push_back(1.1);
    Rect rect;
    rect.x = 5;
    rect.y = 5;
    rect.width = 10;
    rect.height = 10;
    windowInfo.pointerHotAreas.push_back(rect);
    it->second.windowsInfo.push_back(windowInfo);
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    DisplayGroupInfo displayGroupInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
    displayGroupInfoRef.windowsInfo.clear();
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->lastPointerEvent_.reset();
    inputWindowsManager->lastPointerEvent_ = nullptr;
}

/**
 * @tc.name: PointerDrawingManagerOnDisplayInfo_006
 * @tc.desc: Test the function PointerDrawingManagerOnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, PointerDrawingManagerOnDisplayInfo_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, HasPointerDevice()).WillOnce(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.x = 8;
    displayInfo.y = 8;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 11;
    windowInfo.transform.push_back(1.1);
    Rect rect;
    rect.x = 5;
    rect.y = 5;
    rect.width = 10;
    rect.height = 10;
    windowInfo.pointerHotAreas.push_back(rect);
    it->second.windowsInfo.push_back(windowInfo);
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    inputWindowsManager->isDragBorder_ = true;
    inputWindowsManager->dragFlag_ = true;
    DisplayGroupInfo displayGroupInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
    displayGroupInfoRef.displaysInfo.clear();
    displayGroupInfoRef.windowsInfo.clear();
    inputWindowsManager->lastPointerEvent_.reset();
    inputWindowsManager->lastPointerEvent_ = nullptr;
    inputWindowsManager->isDragBorder_ = false;
    inputWindowsManager->dragFlag_ = false;
}

/**
 * @tc.name: PointerDrawingManagerOnDisplayInfo_007
 * @tc.desc: Test the function PointerDrawingManagerOnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, PointerDrawingManagerOnDisplayInfo_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, HasPointerDevice()).WillOnce(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.x = 8;
    displayInfo.y = 8;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 11;
    windowInfo.transform.push_back(1.1);
    Rect rect;
    rect.x = 5;
    rect.y = 5;
    rect.width = 10;
    rect.height = 10;
    windowInfo.pointerHotAreas.push_back(rect);
    it->second.windowsInfo.push_back(windowInfo);
    inputWindowsManager->lastPointerEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastPointerEvent_, nullptr);
    inputWindowsManager->lastPointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    inputWindowsManager->isDragBorder_ = true;
    inputWindowsManager->dragFlag_ = true;
    DisplayGroupInfo displayGroupInfo;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->PointerDrawingManagerOnDisplayInfo(displayGroupInfo));
    displayGroupInfoRef.displaysInfo.clear();
    displayGroupInfoRef.windowsInfo.clear();
    inputWindowsManager->lastPointerEvent_.reset();
    inputWindowsManager->lastPointerEvent_ = nullptr;
    inputWindowsManager->isDragBorder_ = false;
    inputWindowsManager->dragFlag_ = false;
}
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING

/**
 * @tc.name: SendPointerEvent_001
 * @tc.desc: Verify if (pointerAction == PointerEvent::POINTER_ACTION_ENTER_WINDOW ||
 * Rosen::SceneBoardJudgement::IsSceneBoardEnabled())
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, SendPointerEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, SendMsg(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(1));
    SessionPtr session = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    EXPECT_CALL(*messageParcelMock_, GetSession(_)).WillOnce(Return(session));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(false));

    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_UNKNOWN;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SendPointerEvent(pointerAction));
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->extraData_.appended = false;
    inputWindowsManager->extraData_.sourceType = -1;
}

/**
 * @tc.name: SendPointerEvent_002
 * @tc.desc: Verify if (pointerAction == PointerEvent::POINTER_ACTION_ENTER_WINDOW ||
 * Rosen::SceneBoardJudgement::IsSceneBoardEnabled())
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, SendPointerEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, SendMsg(_)).WillOnce(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(1));
    SessionPtr session = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    EXPECT_CALL(*messageParcelMock_, GetSession(_)).WillOnce(Return(session));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(false));

    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_ENTER_WINDOW ;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager->extraData_.appended = false;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SendPointerEvent(pointerAction));
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->extraData_.sourceType = -1;
}

/**
 * @tc.name: SendPointerEvent_003
 * @tc.desc: Verify if (pointerAction == PointerEvent::POINTER_ACTION_ENTER_WINDOW ||
 * Rosen::SceneBoardJudgement::IsSceneBoardEnabled())
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, SendPointerEvent_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, SendMsg(_)).WillOnce(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(1));
    SessionPtr session = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    EXPECT_CALL(*messageParcelMock_, GetSession(_)).WillOnce(Return(session));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(true));

    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_UNKNOWN;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager->extraData_.appended = false;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SendPointerEvent(pointerAction));
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->extraData_.sourceType = -1;
}

/**
 * @tc.name: SendPointerEvent_004
 * @tc.desc: Verify if (!UpdateDisplayId(displayId))
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, SendPointerEvent_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, SendMsg(_)).WillOnce(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(1));
    SessionPtr session = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    EXPECT_CALL(*messageParcelMock_, GetSession(_)).WillOnce(Return(session));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(true));

    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_ENTER_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SendPointerEvent(pointerAction));
    inputWindowsManager->udsServer_ = nullptr;
}

/**
 * @tc.name: SendPointerEvent_005
 * @tc.desc: Verify if (extraData_.appended && extraData_.sourceType == PointerEvent::SOURCE_TYPE_MOUSE)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, SendPointerEvent_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, SendMsg(_)).WillOnce(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(1));
    SessionPtr session = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    EXPECT_CALL(*messageParcelMock_, GetSession(_)).WillOnce(Return(session));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(true));

    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_UNKNOWN;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SendPointerEvent(pointerAction));
    inputWindowsManager->udsServer_ = nullptr;
    inputWindowsManager->extraData_.appended = false;
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->extraData_.sourceType = -1;
}

/**
 * @tc.name: SkipNavigationWindow_001
 * @tc.desc: Test the function SkipNavigationWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, SkipNavigationWindow_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInputType windowType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    int32_t toolType = PointerEvent::TOOL_TYPE_FINGER;
    EXPECT_FALSE(inputWindowsManager->SkipNavigationWindow(windowType, toolType));

    toolType = PointerEvent::TOOL_TYPE_PEN;
    inputWindowsManager->isOpenAntiMisTakeObserver_ = false;
    inputWindowsManager->antiMistake_.isOpen = true;
    EXPECT_TRUE(inputWindowsManager->SkipNavigationWindow(windowType, toolType));

    inputWindowsManager->isOpenAntiMisTakeObserver_ = true;
    inputWindowsManager->antiMistake_.isOpen = false;
    EXPECT_FALSE(inputWindowsManager->SkipNavigationWindow(windowType, toolType));
    inputWindowsManager->isOpenAntiMisTakeObserver_ = false;
}

/**
 * @tc.name: TransformTipPoint_001
 * @tc.desc: Test the function TransformTipPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, TransformTipPoint_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.uniq = "default0";
    displayInfo.direction = DIRECTION90;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    libinput_event_tablet_tool event {};
    PhysicalCoordinate coord;
    int32_t displayId;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TransformTipPoint(&event, coord, displayId));
    displayGroupInfoRef.displaysInfo.clear();
}

/**
 * @tc.name: TransformTipPoint_002
 * @tc.desc: Test the function TransformTipPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, TransformTipPoint_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.uniq = "default0";
    displayInfo.direction = DIRECTION270;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    libinput_event_tablet_tool event {};
    PhysicalCoordinate coord;
    int32_t displayId;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TransformTipPoint(&event, coord, displayId));
    displayGroupInfoRef.displaysInfo.clear();
}

/**
 * @tc.name: TransformTipPoint_003
 * @tc.desc: Test the function TransformTipPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, TransformTipPoint_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.uniq = "default0";
    displayInfo.direction = DIRECTION0;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    libinput_event_tablet_tool event {};
    PhysicalCoordinate coord;
    int32_t displayId;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TransformTipPoint(&event, coord, displayId));
    displayGroupInfoRef.displaysInfo.clear();
}

/**
 * @tc.name: InputWindowsManagerTest_TransformTipPoint_004
 * @tc.desc: Test the function TransformTipPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TransformTipPoint_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    libinput_event_tablet_tool event {};
    Direction direction;
    direction = DIRECTION90;
    PhysicalCoordinate coord;
    coord.x = 5.5;
    coord.y = 3.2;
    int32_t displayId = 2;
    bool ret = inputWindowsManager->TransformTipPoint(&event, coord, displayId);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_TransformTipPoint_005
 * @tc.desc: Test the function TransformTipPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TransformTipPoint_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    libinput_event_tablet_tool event {};
    Direction direction;
    direction = DIRECTION270;
    PhysicalCoordinate coord;
    coord.x = 6.5;
    coord.y = 8.2;
    int32_t displayId = 3;
    bool ret = inputWindowsManager->TransformTipPoint(&event, coord, displayId);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_TransformTipPoint_006
 * @tc.desc: Test the function TransformTipPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TransformTipPoint_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    libinput_event_tablet_tool event {};
    Direction direction;
    direction = DIRECTION0;
    PhysicalCoordinate coord;
    coord.x = 6.5;
    coord.y = 8.2;
    int32_t displayId = 3;
    bool ret = inputWindowsManager->TransformTipPoint(&event, coord, displayId);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_001
 * @tc.desc: Test the function IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t windowId = 2;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(true));

    bool ret = inputWindowsManager->IsNeedRefreshLayer(windowId);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_002
 * @tc.desc: Test the function IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t windowId = 3;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(false));

    int32_t displayId = MouseEventHdr->GetDisplayId();
    EXPECT_FALSE(displayId < 0);

    std::optional<WindowInfo> touchWindow = inputWindowsManager->GetWindowInfo(5, 7);
    bool ret = inputWindowsManager->IsNeedRefreshLayer(windowId);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_003
 * @tc.desc: Test the function IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(false));
    int32_t displayId = MouseEventHdr->GetDisplayId();
    EXPECT_FALSE(displayId < 0);

    std::optional<WindowInfo> touchWindow = inputWindowsManager->GetWindowInfo(6, 8);
    int32_t windowId = GLOBAL_WINDOW_ID;
    bool ret = inputWindowsManager->IsNeedRefreshLayer(windowId);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_004
 * @tc.desc: Test the function IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(false));
    int32_t displayId = MouseEventHdr->GetDisplayId();
    EXPECT_FALSE(displayId < 0);

    std::optional<WindowInfo> touchWindow = inputWindowsManager->GetWindowInfo(2, 3);
    touchWindow = std::nullopt;
    int32_t windowId = GLOBAL_WINDOW_ID;
    bool ret = inputWindowsManager->IsNeedRefreshLayer(windowId);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_005
 * @tc.desc: Test the function IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillOnce(Return(false));
    int32_t displayId = MouseEventHdr->GetDisplayId();
    EXPECT_FALSE(displayId < 0);

    std::optional<WindowInfo> touchWindow = inputWindowsManager->GetWindowInfo(3, 5);
    touchWindow->id = GLOBAL_WINDOW_ID;
    touchWindow->pid = 2;
    touchWindow->uid = 3;
    int32_t windowId = GLOBAL_WINDOW_ID;
    bool ret = inputWindowsManager->IsNeedRefreshLayer(windowId);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_SendUIExtentionPointerEvent_01
 * @tc.desc: Cover if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) branch
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SendUIExtentionPointerEvent_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsMgr =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsMgr, nullptr);

    std::shared_ptr<PointerEvent> pointer = PointerEvent::Create();
    ASSERT_NE(pointer, nullptr);
    pointer->pointerId_ = 2;

    PointerEvent::PointerItem pointerItem;
    pointerItem.SetPointerId(1);
    bool ret = pointer->GetPointerItem(pointer->pointerId_, pointerItem);
    EXPECT_FALSE(ret);

    int32_t logicalX = 300;
    int32_t logicalY = 500;
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 2;
    windowInfo.transform.push_back(1.1);
    EXPECT_FALSE(windowInfo.transform.empty());
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr->SendUIExtentionPointerEvent(logicalX, logicalY, windowInfo, pointer));
}

/**
 * @tc.name: InputWindowsManagerTest_GetPhysicalDisplayCoord_01
 * @tc.desc: Test the function GetPhysicalDisplayCoord
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPhysicalDisplayCoord_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsMgr =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsMgr, nullptr);
    libinput_event_touch *touch = nullptr;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));

    DisplayInfo info;
    info.direction = DIRECTION90;
    info.direction = DIRECTION270;

    EventTouch touchInfo;
    touchInfo.point.x = 125;
    touchInfo.point.y = 300;
    touchInfo.toolRect.point.x = 300;
    touchInfo.toolRect.point.y = 600;
    touchInfo.toolRect.width = 720;
    touchInfo.toolRect.height = 1000;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr->GetPhysicalDisplayCoord(touch, info, touchInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_TouchPointToDisplayPoint_01
 * @tc.desc: Test the function TouchPointToDisplayPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TouchPointToDisplayPoint_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsMgr =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsMgr, nullptr);
    libinput_event_touch *touch = nullptr;
    int32_t deviceId = 1;
    EventTouch touchInfo;
    touchInfo.point.x = 125;
    touchInfo.point.y = 300;
    touchInfo.toolRect.point.x = 300;
    touchInfo.toolRect.point.y = 600;
    touchInfo.toolRect.width = 720;
    touchInfo.toolRect.height = 1000;
    int32_t physicalDisplayId = 2;
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr->TouchPointToDisplayPoint(deviceId, touch, touchInfo, physicalDisplayId));
}

/**
 * @tc.name: CalculateTipPoint_001
 * @tc.desc: Test the function CalculateTipPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, CalculateTipPoint_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    libinput_event_tablet_tool event {};
    PhysicalCoordinate coord;
    int32_t displayId;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CalculateTipPoint(&event, displayId, coord));
}

/**
 * @tc.name: CalculateTipPoint_002
 * @tc.desc: Test the function CalculateTipPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, CalculateTipPoint_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.uniq = "default0";
    displayInfo.direction = DIRECTION0;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    libinput_event_tablet_tool event {};
    PhysicalCoordinate coord;
    int32_t displayId;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->CalculateTipPoint(&event, displayId, coord));
    displayGroupInfoRef.displaysInfo.clear();
}

/**
 * @tc.name: CalculateTipPoint_003
 * @tc.desc: Test the function CalculateTipPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, CalculateTipPoint_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);

    libinput_event_tablet_tool event {};
    int32_t targetDisplayId = 3;
    PhysicalCoordinate coord;
    coord.x = 3.5;
    coord.y = 5.2;
    bool result = inputWindowsManager->TransformTipPoint(&event, coord, targetDisplayId);
    EXPECT_FALSE(result);
    bool ret = inputWindowsManager->CalculateTipPoint(&event, targetDisplayId, coord);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: UpdateMouseTarget_001
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION0;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
}

/**
 * @tc.name: UpdateMouseTarget_002
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION0;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
}

/**
 * @tc.name: UpdateMouseTarget_003
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    inputWindowsManager->mouseDownInfo_.id = 1;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION0;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->mouseDownInfo_.id = -1;
}

/**
 * @tc.name: UpdateMouseTarget_004
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetMouseDisplayState()).WillRepeatedly(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION0;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
}

/**
 * @tc.name: UpdateMouseTarget_005
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetBoolValue(_, _)).WillOnce(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    displayGroupInfoRef.focusWindowId = 1;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION0;
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    displayGroupInfoRef.focusWindowId = -1;
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
}

/**
 * @tc.name: UpdateMouseTarget_006
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetBoolValue(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetMouseDisplayState()).WillRepeatedly(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    displayGroupInfoRef.focusWindowId = -1;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION0;
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
}

/**
 * @tc.name: UpdateMouseTarget_007
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetBoolValue(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*messageParcelMock_, GetMouseDisplayState()).WillRepeatedly(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION0;
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
}

/**
 * @tc.name: UpdateMouseTarget_008
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetBoolValue(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*messageParcelMock_, GetMouseDisplayState()).WillRepeatedly(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION0;
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
}

/**
 * @tc.name: UpdateMouseTarget_009
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetBoolValue(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetMouseDisplayState()).WillRepeatedly(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    inputWindowsManager->isUiExtension_ = true;
    displayGroupInfoRef.focusWindowId = -1;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION0;
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->isUiExtension_ = false;
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
}

/**
 * @tc.name: UpdateMouseTarget_010
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_010, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetMouseDisplayState()).WillRepeatedly(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    inputWindowsManager->dragFlag_ = true;
    inputWindowsManager->isDragBorder_ = true;
    inputWindowsManager->isUiExtension_ = true;
    displayGroupInfoRef.focusWindowId = -1;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION0;
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->dragFlag_ = false;
    inputWindowsManager->isDragBorder_ = false;
    inputWindowsManager->isUiExtension_ = false;
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
}

/**
 * @tc.name: UpdateMouseTarget_011
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_011, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetMouseDisplayState()).WillRepeatedly(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    inputWindowsManager->dragFlag_ = true;
    inputWindowsManager->isDragBorder_ = true;
    inputWindowsManager->isUiExtension_ = false;
    displayGroupInfoRef.focusWindowId = -1;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION90;
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->dragFlag_ = false;
    inputWindowsManager->isDragBorder_ = false;
    inputWindowsManager->isUiExtension_ = false;
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
}

/**
 * @tc.name: UpdateMouseTarget_012
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_012, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetMouseDisplayState()).WillRepeatedly(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    windowInfo.transform.push_back(1.1);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    inputWindowsManager->dragFlag_ = true;
    inputWindowsManager->isDragBorder_ = true;
    inputWindowsManager->isUiExtension_ = false;
    displayGroupInfoRef.focusWindowId = -1;
    inputWindowsManager->captureModeInfo_.isCaptureMode = true;
    inputWindowsManager->captureModeInfo_.windowId = 1;
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION90;
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->dragFlag_ = false;
    inputWindowsManager->isDragBorder_ = false;
    inputWindowsManager->isUiExtension_ = false;
    inputWindowsManager->captureModeInfo_.isCaptureMode = false;
    inputWindowsManager->captureModeInfo_.windowId = -1;
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
    inputWindowsManager->extraData_.appended = false;
    inputWindowsManager->extraData_.sourceType = -1;
}

/**
 * @tc.name: UpdateMouseTarget_013
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_013, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetMouseDisplayState()).WillRepeatedly(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    windowInfo.transform.push_back(1.1);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    inputWindowsManager->dragFlag_ = true;
    inputWindowsManager->isDragBorder_ = true;
    inputWindowsManager->isUiExtension_ = false;
    displayGroupInfoRef.focusWindowId = -1;
    inputWindowsManager->captureModeInfo_.isCaptureMode = true;
    inputWindowsManager->captureModeInfo_.windowId = -1;
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION90;
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_UP);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->dragFlag_ = false;
    inputWindowsManager->isDragBorder_ = false;
    inputWindowsManager->isUiExtension_ = false;
    inputWindowsManager->captureModeInfo_.isCaptureMode = false;
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
    inputWindowsManager->extraData_.appended = false;
    inputWindowsManager->extraData_.sourceType = -1;
}

/**
 * @tc.name: UpdateMouseTarget_014
 * @tc.desc: Test the function UpdateMouseTarget
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, UpdateMouseTarget_014, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetMouseDisplayState()).WillRepeatedly(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    windowInfo.transform.push_back(1.1);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->mouseDownInfo_.id = 1;
    inputWindowsManager->dragFlag_ = true;
    inputWindowsManager->isDragBorder_ = true;
    inputWindowsManager->isUiExtension_ = false;
    displayGroupInfoRef.focusWindowId = -1;
    inputWindowsManager->captureModeInfo_.isCaptureMode = true;
    inputWindowsManager->captureModeInfo_.windowId = -1;
    inputWindowsManager->extraData_.appended = false;
    inputWindowsManager->extraData_.sourceType = -1;
    DisplayInfo displayInfo;
    displayInfo.id = 0;
    displayInfo.displayDirection = DIRECTION90;
    it->second.displaysInfo.push_back(displayInfo);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_UP);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateMouseTarget(pointerEvent));
    displayGroupInfoRef.displaysInfo.clear();
    inputWindowsManager->dragFlag_ = false;
    inputWindowsManager->isDragBorder_ = false;
    inputWindowsManager->isUiExtension_ = false;
    inputWindowsManager->captureModeInfo_.isCaptureMode = false;
    inputWindowsManager->mouseDownInfo_.id = -1;
    inputWindowsManager->udsServer_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
}

/**
 * @tc.name: IsNeedDrawPointer_001
 * @tc.desc: Test the function IsNeedDrawPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsNeedDrawPointer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsNeedDrawPointer(pointerItem));
}

/**
 * @tc.name: IsNeedDrawPointer_002
 * @tc.desc: Test the function IsNeedDrawPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsNeedDrawPointer_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetInputDevice(_, _)).WillOnce(Return(nullptr));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsNeedDrawPointer(pointerItem));
}

/**
 * @tc.name: IsNeedDrawPointer_003
 * @tc.desc: Test the function IsNeedDrawPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsNeedDrawPointer_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputDevice> inputDevice = std::make_shared<InputDevice>();
    inputDevice->SetBus(BUS_USB);
    EXPECT_CALL(*messageParcelMock_, GetInputDevice(_, _)).WillOnce(Return(inputDevice));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsNeedDrawPointer(pointerItem));
}

/**
 * @tc.name: IsNeedDrawPointer_004
 * @tc.desc: Test the function IsNeedDrawPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsNeedDrawPointer_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputDevice> inputDevice = std::make_shared<InputDevice>();
    inputDevice->SetBus(BUS_HIL);
    EXPECT_CALL(*messageParcelMock_, GetInputDevice(_, _)).WillOnce(Return(inputDevice));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    PointerEvent::PointerItem pointerItem;
    pointerItem.SetToolType(PointerEvent::TOOL_TYPE_PEN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsNeedDrawPointer(pointerItem));
}

/**
 * @tc.name: DispatchTouch_001
 * @tc.desc: Test the function DispatchTouch
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DispatchTouch_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->lastTouchEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastTouchEvent_, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 11;
    windowInfo.flags = 1;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    int32_t pointerAction = PointerEvent::POINTER_ACTION_PULL_IN_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DispatchTouch(pointerAction));
    inputWindowsManager->udsServer_ = nullptr;
    inputWindowsManager->lastTouchEvent_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
}

/**
 * @tc.name: DispatchTouch_002
 * @tc.desc: Test the function DispatchTouch
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DispatchTouch_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->lastTouchEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastTouchEvent_, nullptr);
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 11;
    windowInfo.flags = 0;
    windowInfo.transform.push_back(1.1);
    Rect rect;
    rect.x = 5;
    rect.y = 5;
    rect.width = 10;
    rect.height = 10;
    windowInfo.defaultHotAreas.push_back(rect);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    inputWindowsManager->lastTouchLogicX_ = 8;
    inputWindowsManager->lastTouchLogicY_ = 8;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_PULL_IN_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DispatchTouch(pointerAction));
    inputWindowsManager->udsServer_ = nullptr;
    inputWindowsManager->lastTouchEvent_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
    inputWindowsManager->lastTouchLogicX_ = -1;
    inputWindowsManager->lastTouchLogicY_ = -1;
    inputWindowsManager->lastTouchWindowInfo_.id = -1;
    inputWindowsManager->lastTouchWindowInfo_.pid = -1;
    inputWindowsManager->lastTouchWindowInfo_.uid = -1;
    inputWindowsManager->lastTouchWindowInfo_.agentWindowId = -1;
    inputWindowsManager->lastTouchWindowInfo_.area = { 0, 0, 0, 0 };
    inputWindowsManager->lastTouchWindowInfo_.flags = -1;
    inputWindowsManager->lastTouchWindowInfo_.windowType = 0;
}

/**
 * @tc.name: DispatchTouch_003
 * @tc.desc: Test the function DispatchTouch
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DispatchTouch_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(1));
    EXPECT_CALL(*messageParcelMock_, GetSession(_)).WillOnce(Return(nullptr));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->lastTouchEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastTouchEvent_, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    inputWindowsManager->lastTouchEvent_->AddPointerItem(item);
    inputWindowsManager->lastTouchEvent_->SetPointerId(1);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    windowInfo.flags = 0;
    windowInfo.transform.push_back(1.1);
    Rect rect;
    rect.x = 5;
    rect.y = 5;
    rect.width = 10;
    rect.height = 10;
    windowInfo.defaultHotAreas.push_back(rect);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    inputWindowsManager->lastTouchLogicX_ = 8;
    inputWindowsManager->lastTouchLogicY_ = 8;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_PULL_IN_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DispatchTouch(pointerAction));
    inputWindowsManager->udsServer_ = nullptr;
    inputWindowsManager->lastTouchEvent_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
    inputWindowsManager->lastTouchLogicX_ = -1;
    inputWindowsManager->lastTouchLogicY_ = -1;
}

/**
 * @tc.name: DispatchTouch_004
 * @tc.desc: Test the function DispatchTouch
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DispatchTouch_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(1));
    SessionPtr session = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    EXPECT_CALL(*messageParcelMock_, GetSession(_)).WillOnce(Return(session));
    EXPECT_CALL(*messageParcelMock_, SendMsg(_)).WillOnce(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->lastTouchEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastTouchEvent_, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    inputWindowsManager->lastTouchEvent_->AddPointerItem(item);
    inputWindowsManager->lastTouchEvent_->SetPointerId(1);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    windowInfo.flags = 0;
    windowInfo.transform.push_back(1.1);
    Rect rect;
    rect.x = 5;
    rect.y = 5;
    rect.width = 10;
    rect.height = 10;
    windowInfo.defaultHotAreas.push_back(rect);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    inputWindowsManager->lastTouchLogicX_ = 8;
    inputWindowsManager->lastTouchLogicY_ = 8;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DispatchTouch(pointerAction));
    inputWindowsManager->udsServer_ = nullptr;
    inputWindowsManager->lastTouchEvent_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
    inputWindowsManager->lastTouchLogicX_ = -1;
    inputWindowsManager->lastTouchLogicY_ = -1;
}

/**
 * @tc.name: DispatchTouch_005
 * @tc.desc: Test the function DispatchTouch
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DispatchTouch_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillRepeatedly(Return(1));
    SessionPtr session = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    EXPECT_CALL(*messageParcelMock_, GetSession(_)).WillRepeatedly(Return(session));
    EXPECT_CALL(*messageParcelMock_, SendMsg(_)).WillRepeatedly(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    inputWindowsManager->lastTouchEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastTouchEvent_, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    inputWindowsManager->lastTouchEvent_->AddPointerItem(item);
    inputWindowsManager->lastTouchEvent_->SetPointerId(1);
    WindowInfo windowInfo;
    windowInfo.id = -1;
    windowInfo.pid = 11;
    windowInfo.flags = 0;
    windowInfo.transform.push_back(1.1);
    Rect rect;
    rect.x = 5;
    rect.y = 5;
    rect.width = 10;
    rect.height = 10;
    windowInfo.defaultHotAreas.push_back(rect);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    inputWindowsManager->lastTouchLogicX_ = 8;
    inputWindowsManager->lastTouchLogicY_ = 8;
    int32_t pointerAction = PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DispatchTouch(pointerAction));
    inputWindowsManager->udsServer_ = nullptr;
    inputWindowsManager->lastTouchEvent_ = nullptr;
    displayGroupInfoRef.windowsInfo.clear();
    inputWindowsManager->lastTouchLogicX_ = -1;
    inputWindowsManager->lastTouchLogicY_ = -1;
}

/**
 * @tc.name: DispatchTouch_006
 * @tc.desc: Test the function DispatchTouch
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DispatchTouch_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);

    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    ASSERT_NE(inputWindowsManager->udsServer_, nullptr);

    inputWindowsManager->lastTouchEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastTouchEvent_, nullptr);

    int32_t pointerAction = PointerEvent::POINTER_ACTION_PULL_IN_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DispatchTouch(pointerAction));
}

/**
 * @tc.name: DispatchTouch_007
 * @tc.desc: Test the function DispatchTouch
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DispatchTouch_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);

    UDSServer udsServer;
    inputWindowsManager->udsServer_ = &udsServer;
    ASSERT_NE(inputWindowsManager->udsServer_, nullptr);

    inputWindowsManager->lastTouchEvent_ = PointerEvent::Create();
    ASSERT_NE(inputWindowsManager->lastTouchEvent_, nullptr);

    int32_t pointerAction = PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DispatchTouch(pointerAction));
}

/**
 * @tc.name: TransformWindowXY_001
 * @tc.desc: Test the function TransformWindowXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, TransformWindowXY_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.transform = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
    double logicX = 1.1;
    double logicY = 1.1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TransformWindowXY(window, logicX, logicY));
}

/**
 * @tc.name: TransformWindowXY_002
 * @tc.desc: Test the function TransformWindowXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, TransformWindowXY_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.transform = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
    double logicX = 1.1;
    double logicY = 1.1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TransformWindowXY(window, logicX, logicY));
}

/**
 * @tc.name: TransformWindowXY_003
 * @tc.desc: Test the function TransformWindowXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, TransformWindowXY_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);

    WindowInfo window;
    window.transform = { 1.0, 2.0, 3.0 };
    Matrix3f transforms(window.transform);

    EXPECT_TRUE(window.transform.size() == 3);
    bool ret = transforms.IsIdentity();
    EXPECT_FALSE(ret);

    double logicX = 1.1;
    double logicY = 2.1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TransformWindowXY(window, logicX, logicY));
}

/**
 * @tc.name: TransformWindowXY_004
 * @tc.desc: Test the function TransformWindowXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, TransformWindowXY_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);

    WindowInfo window;
    window.transform = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
    Matrix3f transforms(window.transform);

    EXPECT_TRUE(window.transform.size() == 9);
    bool ret = transforms.IsIdentity();
    EXPECT_FALSE(ret);

    double logicX = 3.2;
    double logicY = 5.1;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TransformWindowXY(window, logicX, logicY));
}

/**
 * @tc.name: IsValidZorderWindow_001
 * @tc.desc: Test the function IsValidZorderWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsValidZorderWindow_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_NO_MONITOR);
    pointerEvent->SetZOrder(-6.6);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsValidZorderWindow(window, pointerEvent));
}

/**
 * @tc.name: IsValidZorderWindow_002
 * @tc.desc: Test the function IsValidZorderWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsValidZorderWindow_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_NO_MONITOR);
    pointerEvent->SetZOrder(6.6);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsValidZorderWindow(window, pointerEvent));
}

/**
 * @tc.name: IsValidZorderWindow_003
 * @tc.desc: Test the function IsValidZorderWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsValidZorderWindow_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    pointerEvent->SetZOrder(-6.6);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsValidZorderWindow(window, pointerEvent));
}

/**
 * @tc.name: IsValidZorderWindow_004
 * @tc.desc: Test the function IsValidZorderWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsValidZorderWindow_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.zOrder = 8.8;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    pointerEvent->SetZOrder(6.6);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsValidZorderWindow(window, pointerEvent));
}

/**
 * @tc.name: IsValidZorderWindow_005
 * @tc.desc: Test the function IsValidZorderWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsValidZorderWindow_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.zOrder = 1.1;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->AddFlag(InputEvent::EVENT_FLAG_SIMULATE);
    pointerEvent->SetZOrder(6.6);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->IsValidZorderWindow(window, pointerEvent));
}

/**
 * @tc.name: IsValidZorderWindow_006
 * @tc.desc: Test the function IsValidZorderWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsValidZorderWindow_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);

    uint32_t flag;
    WindowInfo window;
    window.zOrder = 1.1;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    flag = InputEvent::EVENT_FLAG_SIMULATE;
    EXPECT_FALSE(pointerEvent->HasFlag(flag));

    bool ret = inputWindowsManager->IsValidZorderWindow(window, pointerEvent);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: IsValidZorderWindow_007
 * @tc.desc: Test the function IsValidZorderWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, IsValidZorderWindow_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);

    uint32_t flag;
    WindowInfo window;
    window.zOrder = 3.2;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    flag = InputEvent::EVENT_FLAG_TOUCHPAD_POINTER;
    EXPECT_FALSE(pointerEvent->HasFlag(flag));

    bool ret = inputWindowsManager->IsValidZorderWindow(window, pointerEvent);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: HandleWindowInputType_001
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_002
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::NORMAL;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_003
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::TRANSMIT_ALL;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_004
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::TRANSMIT_EXCEPT_MOVE;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_005
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::ANTI_MISTAKE_TOUCH;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_006
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::TRANSMIT_AXIS_MOVE;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_007
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::TRANSMIT_MOUSE_MOVE;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_008
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::TRANSMIT_LEFT_RIGHT;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_009
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::TRANSMIT_BUTTOM;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_010
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_010, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_011
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_011, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = WindowInputType::MIX_BUTTOM_ANTI_AXIS_MOVE;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: HandleWindowInputType_012
 * @tc.desc: Test the function HandleWindowInputType
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, HandleWindowInputType_012, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    WindowInfo window;
    window.windowInputType = static_cast<WindowInputType>(8);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->HandleWindowInputType(window, pointerEvent));
}

/**
 * @tc.name: DrawTouchGraphic_001
 * @tc.desc: Test the function DrawTouchGraphic
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DrawTouchGraphic_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);

    inputWindowsManager->knuckleDrawMgr_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DrawTouchGraphic(pointerEvent));
}

/**
 * @tc.name: DrawTouchGraphic_002
 * @tc.desc: Test the function DrawTouchGraphic
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DrawTouchGraphic_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);

    inputWindowsManager->knuckleDrawMgr_ = std::make_shared<KnuckleDrawingManager>();
    ASSERT_NE(inputWindowsManager->knuckleDrawMgr_, nullptr);
    inputWindowsManager->knuckleDynamicDrawingManager_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DrawTouchGraphic(pointerEvent));
}

/**
 * @tc.name: DrawTouchGraphic_003
 * @tc.desc: Test the function DrawTouchGraphic
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DrawTouchGraphic_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);

    inputWindowsManager->knuckleDrawMgr_ = std::make_shared<KnuckleDrawingManager>();
    ASSERT_NE(inputWindowsManager->knuckleDrawMgr_, nullptr);

    inputWindowsManager->knuckleDynamicDrawingManager_ = std::make_shared<KnuckleDynamicDrawingManager>();
    ASSERT_NE(inputWindowsManager->knuckleDynamicDrawingManager_, nullptr);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DrawTouchGraphic(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_SendUIExtentionPointerEvent
 * @tc.desc: Cover if (!pointerEvent->GetPointerItem(pointerId, pointerItem)) branch
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SendUIExtentionPointerEvent, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsMgr =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsMgr, nullptr);
    std::shared_ptr<PointerEvent> pointer = PointerEvent::Create();
    ASSERT_NE(pointer, nullptr);
    int32_t logicalX = 500;
    int32_t logicalY = 500;
    WindowInfo windowInfo;
    pointer->SetPointerId(0);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    pointer->AddPointerItem(item);
    EXPECT_NO_FATAL_FAILURE(inputWindowsMgr->SendUIExtentionPointerEvent(logicalX, logicalY, windowInfo, pointer));
}

/**
 * @tc.name: InputWindowsManagerTest_GetPhysicalDisplayCoord_001
 * @tc.desc: Test the funcation GetPhysicalDisplayCoord
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetPhysicalDisplayCoord_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    libinput_event_touch touch {};
    DisplayInfo info;
    EventTouch touchInfo;
    info.direction = DIRECTION90;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->GetPhysicalDisplayCoord(&touch, info, touchInfo));
    info.direction = DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->GetPhysicalDisplayCoord(&touch, info, touchInfo));
    info.direction = DIRECTION180;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->GetPhysicalDisplayCoord(&touch, info, touchInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_006
 * @tc.desc: Test the function IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t windowId = -1;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    EXPECT_NE(inputEvent, nullptr);
    inputEvent->targetDisplayId_ = -1;
    bool ret = inputWindowsManager->IsNeedRefreshLayer(windowId);
    EXPECT_FALSE(ret);
    inputEvent->targetDisplayId_ = 2;
    DisplayInfo displayInfo;
    displayInfo.id = 2;
    displayInfo.x = 2;
    displayInfo.y = 3;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    ret = inputWindowsManager->IsNeedRefreshLayer(windowId);
    EXPECT_FALSE(ret);
    windowId = 5;
    ret = inputWindowsManager->IsNeedRefreshLayer(windowId);
    EXPECT_FALSE(ret);
}

/**
@tc.name: InputWindowsManagerTest_UpdateTransformDisplayXY_001
 * @tc.desc: Test the funcation UpdateTransformDisplayXY
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateTransformDisplayXY_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    std::vector<WindowInfo> windowsInfo;
    DisplayInfo displayInfo;
    pointerEvent->pointerId_ = 1;
    PointerEvent::PointerItem item;
    item.pointerId_ = 1;
    pointerEvent->pointers_.push_back(item);
    WindowInfo windowInfo;
    windowInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    item.displayX_ = 10;
    item.displayY_ = 20;
    Rect rect = {0, 0, 30, 40};
    windowInfo.defaultHotAreas.push_back(rect);
    windowsInfo.push_back(windowInfo);
    pointerEvent->bitwise_ = 1;
    pointerEvent->zOrder_ = -1.0f;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateTransformDisplayXY(pointerEvent, windowsInfo, displayInfo));
    pointerEvent->bitwise_ = 0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateTransformDisplayXY(pointerEvent, windowsInfo, displayInfo));
    pointerEvent->bitwise_ = 0x00000200;
    pointerEvent->zOrder_ = 1.0f;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateTransformDisplayXY(pointerEvent, windowsInfo, displayInfo));
    pointerEvent->bitwise_ = 0x00000100;
    pointerEvent->zOrder_ = 1.0f;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->UpdateTransformDisplayXY(pointerEvent, windowsInfo, displayInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_SendUIExtentionPointerEvent_001
 * @tc.desc: Test the funcation SendUIExtentionPointerEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SendUIExtentionPointerEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t logicalX = 100;
    int32_t logicalY = 200;
    WindowInfo windowInfo;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->pointerId_ = 1;
    PointerEvent::PointerItem item;
    item.pointerId_ = -1;
    pointerEvent->pointers_.push_back(item);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SendUIExtentionPointerEvent
        (logicalX, logicalY, windowInfo, pointerEvent));
    item.pointerId_ = 1;
    pointerEvent->pointers_.push_back(item);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SendUIExtentionPointerEvent
        (logicalX, logicalY, windowInfo, pointerEvent));
    windowInfo.id = 1;
    windowInfo.pid = 11;
    windowInfo.transform.push_back(1.1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SendUIExtentionPointerEvent
        (logicalX, logicalY, windowInfo, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_DispatchUIExtentionPointerEvent_001
 * @tc.desc: Test the funcation DispatchUIExtentionPointerEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_DispatchUIExtentionPointerEvent_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t logicalX = 400;
    int32_t logicalY = 600;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    std::shared_ptr<InputEvent> inputEvent = InputEvent::Create();
    EXPECT_NE(inputEvent, nullptr);
    inputEvent->targetDisplayId_ = 2;
    PointerEvent::PointerItem pointerItem;
    pointerItem.targetWindowId_ = 2;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DispatchUIExtentionPointerEvent(logicalX, logicalY, pointerEvent));
    pointerItem.targetWindowId_ = 3;
    WindowInfo windowInfo;
    windowInfo.id = 3;
    windowInfo.uiExtentionWindowInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DispatchUIExtentionPointerEvent(logicalX, logicalY, pointerEvent));
    pointerItem.targetWindowId_ = 6;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DispatchUIExtentionPointerEvent(logicalX, logicalY, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_SelectWindowInfo_001
 * @tc.desc: Test the funcation SelectWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectWindowInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t logicalX = 10;
    int32_t logicalY = 20;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_PULL_UP;
    inputWindowsManager->firstBtnDownWindowInfo_.first = -1;
    PointerEvent::PointerItem pointerItem;
    pointerItem.targetWindowId_ = 2;
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_MOUSE;
    WindowInfo windowInfo;
    windowInfo.pointerHotAreas.push_back({ 0, 0, 30, 40 });
    windowInfo.windowInputType = WindowInputType::MIX_LEFT_RIGHT_ANTI_AXIS_MOVE;
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_UP);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_BEGIN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_IN_WINDOW);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_END);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_LEAVE_WINDOW);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->pressedButtons_.insert(1);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    windowInfo.windowInputType = WindowInputType::TRANSMIT_BUTTOM;
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_PULL_DOWN;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    inputWindowsManager->extraData_.appended = false;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_TOUCHPAD;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_SelectWindowInfo_002
 * @tc.desc: Test the funcation SelectWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectWindowInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t logicalX = 10;
    int32_t logicalY = 20;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_PULL_UP;
    inputWindowsManager->firstBtnDownWindowInfo_.first = -1;
    PointerEvent::PointerItem pointerItem;
    pointerItem.targetWindowId_ = 2;
    inputWindowsManager->extraData_.appended = false;
    inputWindowsManager->extraData_.sourceType = PointerEvent::SOURCE_TYPE_TOUCHPAD;
    WindowInfo windowInfo;
    windowInfo.pointerHotAreas.push_back({ 0, 0, 30, 40 });
    windowInfo.windowInputType = WindowInputType::TRANSMIT_BUTTOM;
    pointerEvent->pressedButtons_.insert(1);
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_PULL_DOWN;
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_UP);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_BEGIN);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_END);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_ENTER_WINDOW);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
    inputWindowsManager->firstBtnDownWindowInfo_.first = 1;
    pointerEvent->pressedButtons_.insert(2);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    inputWindowsManager->extraData_.appended = false;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_TouchPointToDisplayPoint
 * @tc.desc: Test the funcation TouchPointToDisplayPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TouchPointToDisplayPoint, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t deviceId = 10;
    libinput_event_touch touch {};
    EventTouch touchInfo;
    int32_t physicalDisplayId;
    inputWindowsManager->bindInfo_.AddDisplay(2, "abcdefg");
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TouchPointToDisplayPoint
        (deviceId, &touch, touchInfo, physicalDisplayId));
    deviceId = 2;
    DisplayInfo displayInfo;
    displayInfo.width = -1;
    displayInfo.height = 3;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TouchPointToDisplayPoint
        (deviceId, &touch, touchInfo, physicalDisplayId));
    displayInfo.width = 3;
    displayInfo.height = -1;
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TouchPointToDisplayPoint
        (deviceId, &touch, touchInfo, physicalDisplayId));
    displayInfo.width = -5;
    displayInfo.height = -6;
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TouchPointToDisplayPoint
        (deviceId, &touch, touchInfo, physicalDisplayId));
    displayInfo.width = 3;
    displayInfo.height = 2;
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->TouchPointToDisplayPoint
        (deviceId, &touch, touchInfo, physicalDisplayId));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation
 * @tc.desc: Test the function FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfoEX winInfoEx;
    pointerEvent->bitwise_ = 0x00000000;
    pointerEvent->SetPointerId(1);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    inputWindowsManager->touchItemDownInfos_.insert(std::make_pair(2, winInfoEx));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_001
 * @tc.desc: Test the function FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfoEX winInfoEx;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    pointerEvent->bitwise_ = 0x00000000;
    pointerEvent->SetPointerId(1);
    pointerEvent->SetTargetDisplayId(10);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager->touchItemDownInfos_.insert(std::make_pair(1, winInfoEx));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_002
 * @tc.desc: Test the function FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    WindowInfoEX winInfoEx;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    pointerEvent->SetPointerId(1);
    pointerEvent->SetTargetDisplayId(10);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_UNKNOWN);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager->touchItemDownInfos_.insert(std::make_pair(1, winInfoEx));
    inputWindowsManager->lastDirection_ = std::make_pair(10, static_cast<Direction>(-1));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_003
 * @tc.desc: Test the function FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    displayInfo.direction = DIRECTION90;
    item.SetPointerId(2);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetTargetDisplayId(10);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    inputWindowsManager.lastDirection_ = std::make_pair(10, DIRECTION0);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_004
 * @tc.desc: Test the function FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    displayInfo.direction = DIRECTION90;
    item.SetPointerId(1);
    item.SetPressed(false);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetTargetDisplayId(10);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    inputWindowsManager.lastDirection_ = std::make_pair(10, DIRECTION0);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_005
 * @tc.desc: Test the function FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    displayInfo.direction = DIRECTION90;
    item.SetPointerId(1);
    item.SetPressed(true);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetTargetDisplayId(10);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    inputWindowsManager.lastDirection_ = std::make_pair(10, DIRECTION0);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_006
 * @tc.desc: Test the function FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    WindowInfoEX winInfoEx;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    displayInfo.direction = DIRECTION90;
    item.SetPointerId(1);
    item.SetPressed(true);
    pointerEvent->bitwise_ = 0x00000000;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetTargetDisplayId(10);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    inputWindowsManager.lastDirection_ = std::make_pair(10, DIRECTION0);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(1, winInfoEx));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_007
 * @tc.desc: Test the function FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    WindowInfoEX winInfoEx;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    displayInfo.direction = DIRECTION90;
    item.SetPointerId(1);
    item.SetPressed(true);
    pointerEvent->bitwise_ = 0x00000080;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetTargetDisplayId(10);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_UNKNOWN);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    inputWindowsManager.lastDirection_ = std::make_pair(10, DIRECTION0);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(1, winInfoEx));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_008
 * @tc.desc: Test the function FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    WindowInfoEX winInfoEx;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    displayInfo.direction = DIRECTION90;
    item.SetPointerId(1);
    item.SetPressed(true);
    pointerEvent->bitwise_ = 0x00000000;
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetTargetDisplayId(10);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_UNKNOWN);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    inputWindowsManager.lastDirection_ = std::make_pair(10, DIRECTION0);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    inputWindowsManager.touchItemDownInfos_.insert(std::make_pair(1, winInfoEx));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_FoldScreenRotation_009
 * @tc.desc: Test the function FoldScreenRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_FoldScreenRotation_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    DisplayInfo displayInfo;
    displayInfo.id = 10;
    displayInfo.direction = DIRECTION90;
    item.SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetPointerId(1);
    pointerEvent->SetTargetDisplayId(10);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_UNKNOWN);
    inputWindowsManager.lastDirection_ = std::make_pair(10, DIRECTION90);
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.FoldScreenRotation(pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen
 * @tc.desc: Test the function RotateScreen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    DisplayInfo info;
    PhysicalCoordinate coord;
    info.height = 500;
    coord.x = 200;
    info.direction = DIRECTION0;
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager.cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.direction = DIRECTION90;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_001
 * @tc.desc: Test the function RotateScreen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    DisplayInfo info;
    PhysicalCoordinate coord;
    info.width = 500;
    coord.y = 200;
    info.direction = DIRECTION0;
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager.cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.direction = DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_002
 * @tc.desc: Test the function RotateScreen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    DisplayInfo info;
    PhysicalCoordinate coord;
    info.direction = static_cast<Direction>(10);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_003
 * @tc.desc: Verify if (cursorPos_.displayDirection != info.displayDirection && cursorPos_.direction != info.direction)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;
    
    info.direction = DIRECTION0;
    info.displayDirection = DIRECTION0;
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager.cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.direction = DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));

    info.displayDirection = DIRECTION0;
    cursorPosRef.displayDirection = DIRECTION0;
    info.direction = DIRECTION0;
    cursorPosRef.direction = DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));

    info.displayDirection = DIRECTION0;
    cursorPosRef.displayDirection = DIRECTION0;
    info.direction = DIRECTION0;
    cursorPosRef.direction = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_004
 * @tc.desc: Verify if (cursorPos_.direction == Direction::DIRECTION90)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;

    info.displayDirection = DIRECTION0;
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager.cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.displayDirection = DIRECTION0;
    info.direction = DIRECTION0;
    cursorPosRef.direction = DIRECTION0;

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_005
 * @tc.desc: Verify if (cursorPos_.direction == Direction::DIRECTION90)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;

    info.displayDirection = DIRECTION0;
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager.cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.displayDirection = DIRECTION0;
    info.direction = DIRECTION90;
    cursorPosRef.direction = DIRECTION90;
    
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_006
 * @tc.desc: Verify else if (cursorPos_.direction == Direction::DIRECTION270)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;

    info.displayDirection = DIRECTION0;
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager.cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.displayDirection = DIRECTION0;
    info.direction = DIRECTION270;
    inputWindowsManager.cursorPos_.direction = DIRECTION270;
    
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_007
 * @tc.desc: Verify if (direction == DIRECTION90)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;
    info.direction = DIRECTION90;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_008
 * @tc.desc: Verify if (direction == DIRECTION90)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;
    info.direction = DIRECTION90;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_009
 * @tc.desc: Verify if (direction == DIRECTION180)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;
    info.direction = DIRECTION180;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_010
 * @tc.desc: Verify if (direction == DIRECTION270)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_010, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;
    info.direction = DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateScreen_011
 * @tc.desc: Verify if (direction == DIRECTION270)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateScreen_011, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;
    info.direction = DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateDisplayScreen_001
 * @tc.desc: Verify if (displayDirection == DIRECTION0)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateDisplayScreen_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;

    info.direction = DIRECTION0;
    info.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateDisplayScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateDisplayScreen_002
 * @tc.desc: Verify if (displayDirection == DIRECTION90)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateDisplayScreen_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;

    info.direction = DIRECTION90;
    info.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateDisplayScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateDisplayScreen_003
 * @tc.desc: Verify if (displayDirection == DIRECTION90)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateDisplayScreen_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;

    info.direction = DIRECTION90;
    info.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateDisplayScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateDisplayScreen_004
 * @tc.desc: Verify if (displayDirection == DIRECTION180)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateDisplayScreen_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;

    info.direction = DIRECTION180;
    info.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateDisplayScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateDisplayScreen_005
 * @tc.desc: Verify if (displayDirection == DIRECTION180)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateDisplayScreen_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;

    info.direction = DIRECTION180;
    info.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateDisplayScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateDisplayScreen_006
 * @tc.desc: Verify if (displayDirection == DIRECTION270)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateDisplayScreen_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;

    info.direction = DIRECTION270;
    info.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateDisplayScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_RotateDisplayScreen_007
 * @tc.desc: Verify if (displayDirection == DIRECTION270)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_RotateDisplayScreen_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));
    PhysicalCoordinate coord;
    DisplayInfo info;
    InputWindowsManager inputWindowsManager;

    info.direction = DIRECTION270;
    info.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.RotateDisplayScreen(info, coord));
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer
 * @tc.desc: Test the function IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t displayId = -1;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetDisplayId()).WillRepeatedly(Return(displayId));
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 10;
    DisplayInfo displayInfo;
    WindowInfo winInfo;
    displayInfo.id = 100;
    displayInfo.x = 200;
    displayInfo.y = 200;
    winInfo.flags = 0;
    winInfo.id = 10;
    inputWindowsManager.mouseLocation_.displayId = 80;
    inputWindowsManager.mouseLocation_.physicalX = 100;
    inputWindowsManager.mouseLocation_.physicalY = 100;
    winInfo.pointerHotAreas.push_back({ 100, 100, 1000, 1000 });
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    it->second.windowsInfo.push_back(winInfo);
    EXPECT_TRUE(inputWindowsManager.IsNeedRefreshLayer(windowId));
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_007
 * @tc.desc: Test the function IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t displayId = -1;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetDisplayId()).WillRepeatedly(Return(displayId));
    InputWindowsManager inputWindowsManager;
    int32_t windowId = -1;
    DisplayInfo displayInfo;
    WindowInfo winInfo;
    displayInfo.id = 100;
    displayInfo.x = 200;
    displayInfo.y = 200;
    winInfo.flags = 0;
    winInfo.id = 10;
    inputWindowsManager.mouseLocation_.displayId = 80;
    inputWindowsManager.mouseLocation_.physicalX = 100;
    inputWindowsManager.mouseLocation_.physicalY = 100;
    winInfo.pointerHotAreas.push_back({ 100, 100, 1000, 1000 });
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    it->second.windowsInfo.push_back(winInfo);
    EXPECT_TRUE(inputWindowsManager.IsNeedRefreshLayer(windowId));
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_008
 * @tc.desc: Test the function IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t displayId = -1;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetDisplayId()).WillRepeatedly(Return(displayId));
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 50;
    DisplayInfo displayInfo;
    WindowInfo winInfo;
    displayInfo.id = 100;
    displayInfo.x = 200;
    displayInfo.y = 200;
    winInfo.flags = 0;
    winInfo.id = 10;
    inputWindowsManager.mouseLocation_.displayId = 80;
    inputWindowsManager.mouseLocation_.physicalX = 100;
    inputWindowsManager.mouseLocation_.physicalY = 100;
    winInfo.pointerHotAreas.push_back({ 100, 100, 1000, 1000 });
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    it->second.windowsInfo.push_back(winInfo);
    EXPECT_FALSE(inputWindowsManager.IsNeedRefreshLayer(windowId));
}

/**
 * @tc.name: InputWindowsManagerTest_IsNeedRefreshLayer_009
 * @tc.desc: Test the function IsNeedRefreshLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsNeedRefreshLayer_009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t displayId = -1;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, GetDisplayId()).WillRepeatedly(Return(displayId));
    InputWindowsManager inputWindowsManager;
    int32_t windowId = 50;
    DisplayInfo displayInfo;
    WindowInfo winInfo;
    displayInfo.id = 100;
    displayInfo.x = 200;
    displayInfo.y = 200;
    winInfo.flags = 0;
    inputWindowsManager.mouseLocation_.displayId = 80;
    inputWindowsManager.mouseLocation_.physicalX = 100;
    inputWindowsManager.mouseLocation_.physicalY = 100;
    winInfo.pointerHotAreas.push_back({ 100, 100, INT32_MAX, 1000 });
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    it->second.windowsInfo.push_back(winInfo);
    EXPECT_FALSE(inputWindowsManager.IsNeedRefreshLayer(windowId));
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustDisplayRotation
 * @tc.desc: Test the funcation AdjustDisplayRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AdjustDisplayRotation, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    DisplayInfo displayInfo;
    displayInfo.id = 100;
    displayInfo.direction = DIRECTION90;
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager->cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.displayId = 100;
    cursorPosRef.direction = Direction::DIRECTION0;
    DisplayGroupInfo displayGroupInfoRef;
    auto iter = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (iter != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = iter->second;
    }
    iter->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->AdjustDisplayRotation());
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustDisplayRotation_001
 * @tc.desc: Test the funcation AdjustDisplayRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AdjustDisplayRotation_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    DisplayInfo displayInfo;
    displayInfo.id = 100;
    displayInfo.direction = DIRECTION90;
    CursorPosition cursorPosRef;
    auto it = inputWindowsManager->cursorPosMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->cursorPosMap_.end()) {
        cursorPosRef = it->second;
    }
    cursorPosRef.displayId = 100;
    cursorPosRef.direction = Direction::DIRECTION90;
    DisplayGroupInfo displayGroupInfoRef;
    auto iter = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (iter != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = iter->second;
    }
    iter->second.displaysInfo.push_back(displayInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->AdjustDisplayRotation());
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustDisplayRotation_002
 * @tc.desc: Test the funcation AdjustDisplayRotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_AdjustDisplayRotation_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(true));
    InputWindowsManager inputWindowsManager;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.AdjustDisplayRotation());
}

/**
 * @tc.name: InputWindowsManagerTest_TouchPointToDisplayPoint_001
 * @tc.desc: Test the funcation TouchPointToDisplayPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TouchPointToDisplayPoint_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::string uniq = "uniq";
    EXPECT_CALL(*messageParcelMock_, GetBindDisplayNameByInputDevice(_)).WillRepeatedly(Return(uniq));
    InputWindowsManager inputWindowsManager;
    DisplayInfo displayInfo;
    int32_t deviceId = 100;
    libinput_event_touch touch {};
    EventTouch touchInfo;
    int32_t physicalDisplayId = 200;
    displayInfo.id = 100;
    displayInfo.uniq = "uniq";
    displayInfo.width = -1;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_FALSE(inputWindowsManager.TouchPointToDisplayPoint(deviceId, &touch, touchInfo, physicalDisplayId));
}

/**
 * @tc.name: InputWindowsManagerTest_TouchPointToDisplayPoint_002
 * @tc.desc: Test the funcation TouchPointToDisplayPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TouchPointToDisplayPoint_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::string uniq = "uniq";
    EXPECT_CALL(*messageParcelMock_, GetBindDisplayNameByInputDevice(_)).WillRepeatedly(Return(uniq));
    InputWindowsManager inputWindowsManager;
    DisplayInfo displayInfo;
    int32_t deviceId = 100;
    libinput_event_touch touch {};
    EventTouch touchInfo;
    int32_t physicalDisplayId = 200;
    displayInfo.id = 100;
    displayInfo.uniq = "uniq";
    displayInfo.width = 100;
    displayInfo.height = -1;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_FALSE(inputWindowsManager.TouchPointToDisplayPoint(deviceId, &touch, touchInfo, physicalDisplayId));
}

/**
 * @tc.name: InputWindowsManagerTest_TouchPointToDisplayPoint_003
 * @tc.desc: Test the funcation TouchPointToDisplayPoint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_TouchPointToDisplayPoint_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::string uniq = "uniq";
    EXPECT_CALL(*messageParcelMock_, GetBindDisplayNameByInputDevice(_)).WillRepeatedly(Return(uniq));
    InputWindowsManager inputWindowsManager;
    DisplayInfo displayInfo;
    int32_t deviceId = 100;
    libinput_event_touch touch {};
    EventTouch touchInfo;
    int32_t physicalDisplayId = 200;
    displayInfo.id = 100;
    displayInfo.uniq = "uniq";
    displayInfo.width = 100;
    displayInfo.height = 100;
    displayInfo.validWidth = displayInfo.width;
    displayInfo.validHeight = displayInfo.height;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager.displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager.displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);
    EXPECT_FALSE(inputWindowsManager.TouchPointToDisplayPoint(deviceId, &touch, touchInfo, physicalDisplayId));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfo
 * @tc.desc: Test the funcation UpdateDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, HasPointerDevice()).WillRepeatedly(Return(true));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));
    InputWindowsManager inputWindowsManager;
    DisplayGroupInfo displayGroupInfo;
    DisplayInfo displayInfo;
    WindowInfo winInfo;
    winInfo.action = WINDOW_UPDATE_ACTION::UNKNOWN;
    displayGroupInfo.windowsInfo.push_back(winInfo);
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    displayGroupInfo.currentUserId = 100;
    inputWindowsManager.currentUserId_ = 200;
    inputWindowsManager.pointerDrawFlag_ = true;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDisplayInfo_001
 * @tc.desc: Test the funcation UpdateDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdateDisplayInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, HasPointerDevice()).WillRepeatedly(Return(false));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));
    InputWindowsManager inputWindowsManager;
    DisplayGroupInfo displayGroupInfo;
    DisplayInfo displayInfo;
    WindowInfo winInfo;
    winInfo.action = WINDOW_UPDATE_ACTION::UNKNOWN;
    displayGroupInfo.windowsInfo.push_back(winInfo);
    displayGroupInfo.displaysInfo.push_back(displayInfo);
    displayGroupInfo.currentUserId = 100;
    inputWindowsManager.currentUserId_ = 200;
    inputWindowsManager.pointerDrawFlag_ = true;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdateDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_SetPointerStyle
 * @tc.desc: Test the funcation SetPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SetPointerStyle, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    int32_t pid = 100;
    int32_t windowId = 100;
    PointerStyle pointerStyle;
    bool isUiExtension = false;
    EXPECT_NE(inputWindowsManager.SetPointerStyle(pid, windowId, pointerStyle, isUiExtension), RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_IsInHotArea
 * @tc.desc: Test the funcation IsInHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsInHotArea, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 10;
    int32_t y = 300;
    std::vector<Rect> rects = { { 100, 100, 1000, 1000 } };
    WindowInfo window;
    EXPECT_FALSE(inputWindowsManager.IsInHotArea(x, y, rects, window));
}

/**
 * @tc.name: InputWindowsManagerTest_IsInHotArea_001
 * @tc.desc: Test the funcation IsInHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsInHotArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 300;
    int32_t y = 300;
    std::vector<Rect> rects = { { 100, 100, 100, 1000 } };
    WindowInfo window;
    EXPECT_FALSE(inputWindowsManager.IsInHotArea(x, y, rects, window));
}

/**
 * @tc.name: InputWindowsManagerTest_IsInHotArea_002
 * @tc.desc: Test the funcation IsInHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsInHotArea_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 300;
    int32_t y = 10;
    std::vector<Rect> rects = { { 100, 100, 1000, 1000 } };
    WindowInfo window;
    EXPECT_FALSE(inputWindowsManager.IsInHotArea(x, y, rects, window));
}

/**
 * @tc.name: InputWindowsManagerTest_IsInHotArea_003
 * @tc.desc: Test the funcation IsInHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_IsInHotArea_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 300;
    int32_t y = 300;
    std::vector<Rect> rects = { { 100, 100, 100, 1000 } };
    WindowInfo window;
    EXPECT_FALSE(inputWindowsManager.IsInHotArea(x, y, rects, window));
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea
 * @tc.desc: Test the funcation InWhichHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 50;
    int32_t y = 300;
    std::vector<Rect> rects = { { 100, 100, 1000, 1000 } };
    PointerStyle pointerStyle;
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects, pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea_001
 * @tc.desc: Test the funcation InWhichHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 300;
    int32_t y = 300;
    std::vector<Rect> rects = { { 100, 100, 100, 1000 } };
    PointerStyle pointerStyle;
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects, pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea_002
 * @tc.desc: Test the funcation InWhichHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 300;
    int32_t y = 50;
    std::vector<Rect> rects = { { 100, 100, 1000, 1000 } };
    PointerStyle pointerStyle;
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects, pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea_003
 * @tc.desc: Test the funcation InWhichHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 300;
    int32_t y = 300;
    std::vector<Rect> rects = { { 100, 100, 1000, 100 } };
    PointerStyle pointerStyle;
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects, pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerChangeAreas
 * @tc.desc: Test the funcation UpdatePointerChangeAreas
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerChangeAreas, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    DisplayGroupInfo displayGroupInfo;
    WindowInfo winInfo;
    winInfo.id = 100;
    winInfo.area;
    displayGroupInfo.windowsInfo.push_back(winInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerChangeAreas(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerChangeAreas_001
 * @tc.desc: Test the funcation UpdatePointerChangeAreas
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_UpdatePointerChangeAreas_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.UpdatePointerChangeAreas());
}

/**
 * @tc.name: InputWindowsManagerTest_GetWidthAndHeight
 * @tc.desc: Test the funcation GetWidthAndHeight
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_GetWidthAndHeight, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, IsWindowRotation()).WillRepeatedly(Return(true));
    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    InputWindowsManager inputWindowsManager;
    DisplayInfo displayInfo;
    int32_t width = 300;
    int32_t height = 300;
    displayInfo.direction = DIRECTION90;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager.GetWidthAndHeight(&displayInfo, width, height));
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea_004
 * @tc.desc: Test the funcation InWhichHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 50;
    int32_t y = 300;
    std::vector<Rect> rects = { { 100, 100, 1000, 1000 } };
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects));
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea_005
 * @tc.desc: Test the funcation InWhichHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 300;
    int32_t y = 300;
    std::vector<Rect> rects = { { 100, 100, 100, 1000 } };
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects));
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea_006
 * @tc.desc: Test the funcation InWhichHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 300;
    int32_t y = 50;
    std::vector<Rect> rects = { { 100, 100, 1000, 1000 } };
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects));
}

/**
 * @tc.name: InputWindowsManagerTest_InWhichHotArea_007
 * @tc.desc: Test the funcation InWhichHotArea
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_InWhichHotArea_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    InputWindowsManager inputWindowsManager;
    int32_t x = 300;
    int32_t y = 300;
    std::vector<Rect> rects = { { 100, 100, 1000, 100 } };
    EXPECT_FALSE(inputWindowsManager.InWhichHotArea(x, y, rects));
}

/**
 * @tc.name: DrawTouchGraphic_004
 * @tc.desc: Test the function DrawTouchGraphic
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, DrawTouchGraphic_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t displayId = 10;
    pointerEvent->SetTargetDisplayId(displayId);

    DisplayInfo displayInfo;
    displayInfo.id = 10;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);

    inputWindowsManager->knuckleDrawMgr_ = nullptr;
    inputWindowsManager->knuckleDynamicDrawingManager_ = nullptr;

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->DrawTouchGraphic(pointerEvent));
}

/**
 * @tc.name: SetWindowStateNotifyPid_001
 * @tc.desc: Test the function DrawTouchGraphic
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, SetWindowStateNotifyPid_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);

    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(false));
    int32_t pid = 0;
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SetWindowStateNotifyPid(pid));

    EXPECT_CALL(*messageParcelMock_, IsSceneBoardEnabled()).WillRepeatedly(Return(true));
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SetWindowStateNotifyPid(pid));
}

/**
 * @tc.name: InputWindowsManagerTest_SendUIExtentionPointerEvent_002
 * @tc.desc: Test the funcation SendUIExtentionPointerEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SendUIExtentionPointerEvent_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(1));
    SessionPtr session = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    EXPECT_CALL(*messageParcelMock_, GetSession(_)).WillOnce(Return(session));
    EXPECT_CALL(*messageParcelMock_, SendMsg(_)).WillOnce(Return(false));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t logicalX = 100;
    int32_t logicalY = 200;
    
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->pointerId_ = 1;
    PointerEvent::PointerItem item;
    item.pointerId_ = 1;
    pointerEvent->pointers_.push_back(item);
    int32_t displayId = 10;
    pointerEvent->SetTargetDisplayId(displayId);

    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 11;
    windowInfo.transform.push_back(1.1);

    DisplayInfo displayInfo;
    displayInfo.id = 10;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SendUIExtentionPointerEvent
        (logicalX, logicalY, windowInfo, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_SendUIExtentionPointerEvent_003
 * @tc.desc: Test the funcation SendUIExtentionPointerEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SendUIExtentionPointerEvent_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_CALL(*messageParcelMock_, GetClientFd(_)).WillOnce(Return(1));
    SessionPtr session = std::make_shared<UDSSession>(PROGRAM_NAME, MODULE_TYPE, UDS_FD, UDS_UID, UDS_PID);
    EXPECT_CALL(*messageParcelMock_, GetSession(_)).WillOnce(Return(session));
    EXPECT_CALL(*messageParcelMock_, SendMsg(_)).WillOnce(Return(true));
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t logicalX = 100;
    int32_t logicalY = 200;
    
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->pointerId_ = 1;
    PointerEvent::PointerItem item;
    item.pointerId_ = 1;
    pointerEvent->pointers_.push_back(item);
    int32_t displayId = 10;
    pointerEvent->SetTargetDisplayId(displayId);

    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 11;
    windowInfo.transform.push_back(1.1);

    DisplayInfo displayInfo;
    displayInfo.id = 10;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.displaysInfo.push_back(displayInfo);

    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SendUIExtentionPointerEvent
        (logicalX, logicalY, windowInfo, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_SelectWindowInfo_003
 * @tc.desc: Test the funcation SelectWindowInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SelectWindowInfo_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    int32_t logicalX = 10;
    int32_t logicalY = 20;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_DOWN;
    pointerEvent->SetDeviceId(CAST_INPUT_DEVICEID);
    inputWindowsManager->firstBtnDownWindowInfo_.first = -1;
    PointerEvent::PointerItem pointerItem;
    pointerItem.targetWindowId_ = 2;
    inputWindowsManager->extraData_.appended = true;
    inputWindowsManager->extraData_.sourceType = PointerEvent::POINTER_ACTION_DOWN;
    WindowInfo windowInfo;
    windowInfo.id = 1;
    windowInfo.pid = 11;
    windowInfo.isSkipSelfWhenShowOnVirtualScreen = true;
    DisplayGroupInfo displayGroupInfoRef;
    auto it = inputWindowsManager->displayGroupInfoMap_.find(DEFAULT_GROUP_ID);
    if (it != inputWindowsManager->displayGroupInfoMap_.end()) {
        displayGroupInfoRef = it->second;
    }
    it->second.windowsInfo.push_back(windowInfo);
    EXPECT_NO_FATAL_FAILURE(inputWindowsManager->SelectWindowInfo(logicalX, logicalY, pointerEvent));
}

/**
 * @tc.name: InputWindowsManagerTest_SkipPrivacyProtectionWindow
 * @tc.desc: Test the funcation SkipPrivacyProtectionWindow
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputWindowsManagerTest, InputWindowsManagerTest_SkipPrivacyProtectionWindow, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<InputWindowsManager> inputWindowsManager =
        std::static_pointer_cast<InputWindowsManager>(WIN_MGR);
    ASSERT_NE(inputWindowsManager, nullptr);
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);
    pointerEvent->pointerAction_ = PointerEvent::POINTER_ACTION_DOWN;
    pointerEvent->SetDeviceId(CAST_INPUT_DEVICEID);
    inputWindowsManager->firstBtnDownWindowInfo_.first = -1;
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    bool isSkip = true;
    EXPECT_FALSE(inputWindowsManager->SkipPrivacyProtectionWindow(pointerEvent, isSkip));
    inputWindowsManager->privacyProtection_.isOpen = true;
    EXPECT_TRUE(inputWindowsManager->isOpenPrivacyProtectionserver_);
    EXPECT_TRUE(inputWindowsManager->SkipPrivacyProtectionWindow(pointerEvent, isSkip));
    isSkip = false;
    EXPECT_FALSE(inputWindowsManager->SkipPrivacyProtectionWindow(pointerEvent, isSkip));
}
} // namespace MMI
} // namespace OHOS