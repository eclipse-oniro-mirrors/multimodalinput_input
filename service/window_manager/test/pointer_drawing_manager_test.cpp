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

#include <cstdio>
#include <fstream>

#include <gtest/gtest.h>

#include "image_source.h"
#include "input_windows_manager_mock.h"
#include "knuckle_drawing_manager.h"
#include "libinput_mock.h"
#include "mmi_log.h"
#include "pixel_map.h"
#include "pointer_drawing_manager.h"
#include "pointer_event.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "PointerDrawingManagerTest"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t MOUSE_ICON_SIZE = 64;
constexpr uint32_t DEFAULT_ICON_COLOR { 0xFF };
constexpr int32_t MIDDLE_PIXEL_MAP_WIDTH { 400 };
constexpr int32_t MIDDLE_PIXEL_MAP_HEIGHT { 400 };
constexpr int32_t MAX_PIXEL_MAP_WIDTH { 600 };
constexpr int32_t MAX_PIXEL_MAP_HEIGHT { 600 };
constexpr int32_t INT32_BYTE { 4 };
} // namespace

class PointerDrawingManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    static std::shared_ptr<Media::PixelMap> CreatePixelMap(int32_t width, int32_t height);
    void SetUp(void)
    {}
    void TearDown(void)
    {}

    std::unique_ptr<OHOS::Media::PixelMap> SetMouseIconTest(const std::string iconPath);
private:
};

std::unique_ptr<OHOS::Media::PixelMap> PointerDrawingManagerTest::SetMouseIconTest(const std::string iconPath)
{
    CALL_DEBUG_ENTER;
    OHOS::Media::SourceOptions opts;
    opts.formatHint = "image/svg+xml";
    uint32_t ret = 0;
    auto imageSource = OHOS::Media::ImageSource::CreateImageSource(iconPath, opts, ret);
    CHKPP(imageSource);
    std::set<std::string> formats;
    ret = imageSource->GetSupportedFormats(formats);
    MMI_HILOGD("Get supported format ret:%{public}u", ret);

    OHOS::Media::DecodeOptions decodeOpts;
    decodeOpts.desiredSize = {.width = MOUSE_ICON_SIZE, .height = MOUSE_ICON_SIZE};

    std::unique_ptr<OHOS::Media::PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, ret);
    CHKPL(pixelMap);
    return pixelMap;
}

std::shared_ptr<Media::PixelMap> PointerDrawingManagerTest::CreatePixelMap(int32_t width, int32_t height)
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

/**
 * @tc.name: InputWindowsManagerTest_DrawMovePointer_001
 * @tc.desc: Test the funcation DrawMovePointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawMovePointer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager manager;
    int32_t displayId = 1;
    int32_t physicalX = 2;
    int32_t physicalY = 3;
    PointerStyle pointerStyle;
    Direction direction = DIRECTION0;
    manager.surfaceNode_ = nullptr;
    int32_t ret = manager.DrawMovePointer(displayId, physicalX, physicalY, pointerStyle, direction);
    EXPECT_EQ(ret, RET_ERR);
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    manager.surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_TRUE(manager.surfaceNode_ != nullptr);
    ret = manager.DrawMovePointer(displayId, physicalX, physicalY, pointerStyle, direction);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_DrawCursor_002
 * @tc.desc: Test the funcation DrawCursor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawCursor_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager manager;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    manager.surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_TRUE(manager.surfaceNode_ != nullptr);
    MOUSE_ICON mouseStyle = EAST;
    int32_t ret = manager.DrawCursor(mouseStyle);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_Init_001
 * @tc.desc: Test Init
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_Init_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool isSucess = IPointerDrawingManager::GetInstance()->Init();
    EXPECT_EQ(isSucess, true);
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    IconStyle iconStyle = pointerDrawingManager->GetIconStyle(MOUSE_ICON(MOUSE_ICON::DEFAULT));
    EXPECT_EQ(iconStyle.alignmentWay, 7);
}

/**
 * @tc.name: InputWindowsManagerTest_SetMouseDisplayState_001
 * @tc.desc: Test SetMouseDisplayState
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetMouseDisplayState_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    pointerDrawingManager->SetMouseDisplayState(true);
    bool mouseDisplayState = pointerDrawingManager->GetMouseDisplayState();
    EXPECT_EQ(mouseDisplayState, true);
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerDevice_001
 * @tc.desc: Test UpdatePointerDevice
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_UpdatePointerDevice_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    EXPECT_EQ(pointerDrawingManager->pidInfos_.size(), 0);
    pointerDrawingManager->UpdatePointerDevice(true, true, true);
    EXPECT_EQ(pointerDrawingManager->pidInfos_.size(), 1);
    pointerDrawingManager->UpdatePointerDevice(false, true, true);
    EXPECT_EQ(pointerDrawingManager->pidInfos_.size(), 0);
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustMouseFocusByDirection0_001
 * @tc.desc: Test AdjustMouseFocusByDirection0
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_AdjustMouseFocusByDirection0_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    pointerDrawingManager->imageWidth_ = 50;
    pointerDrawingManager->imageHeight_ = 50;
    pointerDrawingManager->userIconHotSpotX_ = 5;
    pointerDrawingManager->userIconHotSpotY_ = 5;
    int32_t physicalX = 100;
    int32_t physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection0(ANGLE_SW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 50);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection0(ANGLE_CENTER, physicalX, physicalY);
    EXPECT_EQ(physicalX, 75);
    EXPECT_EQ(physicalY, 75);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection0(ANGLE_NW_RIGHT, physicalX, physicalY);
    EXPECT_EQ(physicalX, 95);
    EXPECT_EQ(physicalY, 100);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->userIcon_ = std::make_unique<OHOS::Media::PixelMap>();
    pointerDrawingManager->currentMouseStyle_.id = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    pointerDrawingManager->AdjustMouseFocusByDirection0(ANGLE_NW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 95);
    EXPECT_EQ(physicalY, 95);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection0(ANGLE_E, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 100);
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustMouseFocusByDirection90_001
 * @tc.desc: Test AdjustMouseFocusByDirection90
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_AdjustMouseFocusByDirection90_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    pointerDrawingManager->imageWidth_ = 50;
    pointerDrawingManager->imageHeight_ = 50;
    pointerDrawingManager->userIconHotSpotX_ = 5;
    pointerDrawingManager->userIconHotSpotY_ = 5;
    int32_t physicalX = 100;
    int32_t physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection90(ANGLE_SW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 150);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection90(ANGLE_CENTER, physicalX, physicalY);
    EXPECT_EQ(physicalX, 75);
    EXPECT_EQ(physicalY, 125);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection90(ANGLE_NW_RIGHT, physicalX, physicalY);
    EXPECT_EQ(physicalX, 90);
    EXPECT_EQ(physicalY, 105);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->userIcon_ = std::make_unique<OHOS::Media::PixelMap>();
    pointerDrawingManager->currentMouseStyle_.id = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    pointerDrawingManager->AdjustMouseFocusByDirection90(ANGLE_NW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 95);
    EXPECT_EQ(physicalY, 105);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection90(ANGLE_E, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 100);
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustMouseFocusByDirection180_001
 * @tc.desc: Test AdjustMouseFocusByDirection180
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_AdjustMouseFocusByDirection180_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    pointerDrawingManager->imageWidth_ = 50;
    pointerDrawingManager->imageHeight_ = 50;
    pointerDrawingManager->userIconHotSpotX_ = 5;
    pointerDrawingManager->userIconHotSpotY_ = 5;
    int32_t physicalX = 100;
    int32_t physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection180(ANGLE_SW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 150);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection180(ANGLE_CENTER, physicalX, physicalY);
    EXPECT_EQ(physicalX, 125);
    EXPECT_EQ(physicalY, 125);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection180(ANGLE_NW_RIGHT, physicalX, physicalY);
    EXPECT_EQ(physicalX, 110);
    EXPECT_EQ(physicalY, 105);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->userIcon_ = std::make_unique<OHOS::Media::PixelMap>();
    pointerDrawingManager->currentMouseStyle_.id = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    pointerDrawingManager->AdjustMouseFocusByDirection180(ANGLE_NW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 105);
    EXPECT_EQ(physicalY, 105);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection180(ANGLE_E, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 100);
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustMouseFocusByDirection270_001
 * @tc.desc: Test AdjustMouseFocusByDirection270
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_AdjustMouseFocusByDirection270_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    pointerDrawingManager->imageWidth_ = 50;
    pointerDrawingManager->imageHeight_ = 50;
    pointerDrawingManager->userIconHotSpotX_ = 5;
    pointerDrawingManager->userIconHotSpotY_ = 5;
    int32_t physicalX = 100;
    int32_t physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection270(ANGLE_SW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 50);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection270(ANGLE_CENTER, physicalX, physicalY);
    EXPECT_EQ(physicalX, 125);
    EXPECT_EQ(physicalY, 75);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection270(ANGLE_NW_RIGHT, physicalX, physicalY);
    EXPECT_EQ(physicalX, 110);
    EXPECT_EQ(physicalY, 95);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->userIcon_ = std::make_unique<OHOS::Media::PixelMap>();
    pointerDrawingManager->currentMouseStyle_.id = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    pointerDrawingManager->AdjustMouseFocusByDirection270(ANGLE_NW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 105);
    EXPECT_EQ(physicalY, 95);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->AdjustMouseFocusByDirection270(ANGLE_E, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 100);
}

/**
 * @tc.name: InputWindowsManagerTest_AdjustMouseFocus_001
 * @tc.desc: Test AdjustMouseFocus
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_AdjustMouseFocus_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    pointerDrawingManager->imageWidth_ = 50;
    pointerDrawingManager->imageHeight_ = 50;
    int32_t physicalX = 100;
    int32_t physicalY = 100;
    pointerDrawingManager->RotateDegree(DIRECTION0);
    pointerDrawingManager->AdjustMouseFocus(DIRECTION0, ANGLE_SW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 50);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->RotateDegree(DIRECTION90);
    pointerDrawingManager->AdjustMouseFocus(DIRECTION90, ANGLE_SW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 150);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->RotateDegree(DIRECTION180);
    pointerDrawingManager->AdjustMouseFocus(DIRECTION180, ANGLE_SW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 150);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->RotateDegree(DIRECTION270);
    pointerDrawingManager->AdjustMouseFocus(DIRECTION270, ANGLE_SW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 50);
    physicalX = 100;
    physicalY = 100;
    pointerDrawingManager->RotateDegree(static_cast<Direction>(4));
    pointerDrawingManager->AdjustMouseFocus(static_cast<Direction>(4), ANGLE_SW, physicalX, physicalY);
    EXPECT_EQ(physicalX, 100);
    EXPECT_EQ(physicalY, 100);
}

/**
 * @tc.name: InputWindowsManagerTest_SetPointerColor_001
 * @tc.desc: Test SetPointerColor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerColor_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    pointerDrawingManager->SetPointerColor(-1);
    int32_t color = pointerDrawingManager->GetPointerColor();
    EXPECT_EQ(color, 16777215);
    pointerDrawingManager->SetPointerColor(16777216);
    color = pointerDrawingManager->GetPointerColor();
    EXPECT_EQ(color, 0);
}

/**
 * @tc.name: InputWindowsManagerTest_SetPointerVisible_001
 * @tc.desc: Test SetPointerVisible
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerVisible_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    for (int32_t i = 1; i < 102; i++) {
        pointerDrawingManager->SetPointerVisible(i, false, 0, false);
    }
    bool visible = pointerDrawingManager->GetPointerVisible(1);
    EXPECT_EQ(visible, true);
    pointerDrawingManager->SetPointerVisible(11, true, 0, false);
    visible = pointerDrawingManager->GetPointerVisible(11);
    EXPECT_EQ(visible, true);
}

/**
 * @tc.name: InputWindowsManagerTest_SetPointerStyle_001
 * @tc.desc: Test SetPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    pointerDrawingManager->SetPointerStyle(1, -1, pointerStyle);
    PointerStyle pointerStyleTmp;
    pointerDrawingManager->GetPointerStyle(1, -1, pointerStyleTmp);
    EXPECT_EQ(pointerStyleTmp.id, pointerStyle.id);
}

/**
 * @tc.name: InputWindowsManagerTest_SetPointerSize_001
 * @tc.desc: Test SetPointerSize
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerSize_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    pointerDrawingManager->SetPointerSize(0);
    int32_t pointerSize = pointerDrawingManager->GetPointerSize();
    EXPECT_EQ(pointerSize, 1);
    pointerDrawingManager->SetPointerSize(8);
    pointerSize = pointerDrawingManager->GetPointerSize();
    EXPECT_EQ(pointerSize, 7);
}

/**
 * @tc.name: InputWindowsManagerTest_FixCursorPosition_001
 * @tc.desc: Test FixCursorPosition
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_FixCursorPosition_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    pointerDrawingManager->displayInfo_.displayDirection = DIRECTION0;
    pointerDrawingManager->displayInfo_.direction = DIRECTION0;
    pointerDrawingManager->displayInfo_.width = 500;
    pointerDrawingManager->displayInfo_.height = 1100;
    pointerDrawingManager->imageWidth_ = 48;
    pointerDrawingManager->imageHeight_ = 48;
    int32_t physicalX = 500;
    int32_t physicalY = 1100;
    pointerDrawingManager->FixCursorPosition(physicalX, physicalY);
    EXPECT_EQ(physicalX, 497);
    EXPECT_EQ(physicalY, 1097);
    pointerDrawingManager->displayInfo_.direction = DIRECTION90;
    physicalX = 1100;
    physicalY = 500;
    pointerDrawingManager->FixCursorPosition(physicalX, physicalY);
    EXPECT_EQ(physicalX, 1097);
    EXPECT_EQ(physicalY, 497);
    pointerDrawingManager->displayInfo_.displayDirection = DIRECTION90;
    pointerDrawingManager->displayInfo_.direction = DIRECTION0;
    physicalX = 500;
    physicalY = 1100;
    pointerDrawingManager->FixCursorPosition(physicalX, physicalY);
    EXPECT_EQ(physicalX, 497);
    EXPECT_EQ(physicalY, 1097);
}

/**
 * @tc.name: InputWindowsManagerTest_CreatePointerSwitchObserver_001
 * @tc.desc: Test CreatePointerSwitchObserver
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_CreatePointerSwitchObserver_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    isMagicCursor item;
    item.isShow = true;
    item.name = "test";
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.CreatePointerSwitchObserver(item));
}

/**
 * @tc.name: InputWindowsManagerTest_DrawCursor_001
 * @tc.desc: Test DrawCursor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawCursor_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    MOUSE_ICON mouseStyle = EAST;
    int32_t ret = pointerDrawingManager.DrawCursor(mouseStyle);
    EXPECT_EQ(ret, RET_ERR);
    pointerDrawingManager.surfaceNode_ = nullptr;
    ret = pointerDrawingManager.DrawCursor(mouseStyle);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_DrawLoadingPointerStyle_001
 * @tc.desc: Test DrawLoadingPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawLoadingPointerStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    MOUSE_ICON mouseStyle = EAST;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawLoadingPointerStyle(mouseStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_DrawRunningPointerAnimate_001
 * @tc.desc: Test DrawRunningPointerAnimate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawRunningPointerAnimate_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    pointerDrawingManager.surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    pointerDrawingManager.surfaceNode_->SetFrameGravity(Rosen::Gravity::RESIZE_ASPECT_FILL);
    pointerDrawingManager.surfaceNode_->SetPositionZ(Rosen::RSSurfaceNode::POINTER_WINDOW_POSITION_Z);
    MOUSE_ICON mouseStyle = EAST;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawRunningPointerAnimate(mouseStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_GetLayer_001
 * @tc.desc: Test GetLayer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_GetLayer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    pointerDrawingManager.surfaceNode_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.GetLayer());
}

/**
 * @tc.name: InputWindowsManagerTest_SetMouseIcon_001
 * @tc.desc: Test SetMouseIcon
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetMouseIcon_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    int32_t pid = -1;
    int32_t windowId = 1;
    void* pixelMap = nullptr;
    int32_t ret = pointerDrawingManager.SetMouseIcon(pid, windowId, pixelMap);
    EXPECT_EQ(ret, RET_ERR);
    pid = 1;
    ret = pointerDrawingManager.SetMouseIcon(pid, windowId, pixelMap);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetMouseHotSpot_001
 * @tc.desc: Test SetMouseHotSpot
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetMouseHotSpot_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    int32_t pid = -1;
    int32_t windowId = 1;
    int32_t hotSpotX = 100;
    int32_t hotSpotY = 100;
    int32_t ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    EXPECT_EQ(ret, RET_ERR);
    pid = 1;
    windowId = -1;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    EXPECT_EQ(ret, RET_ERR);
    pid = 1;
    windowId = 1;
    hotSpotX = -1;
    hotSpotY = -1;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    EXPECT_EQ(ret, RET_ERR);
    pid = 1;
    windowId = 1;
    hotSpotX = 100;
    hotSpotY = 100;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_DecodeImageToPixelMap_001
 * @tc.desc: Test DecodeImageToPixelMap
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DecodeImageToPixelMap_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    std::string iconPath = ("/system/etc/multimodalinput/mouse_icon/Loading_Left.svg");
    pointerDrawingManager.tempPointerColor_ = 1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DecodeImageToPixelMap(iconPath));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerVisible_001
 * @tc.desc: Test UpdatePointerVisible
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_UpdatePointerVisible_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    pointerDrawingManager.surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    pointerDrawingManager.mouseDisplayState_ = true;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.UpdatePointerVisible());
    pointerDrawingManager.mouseDisplayState_ = false;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.UpdatePointerVisible());
}

/**
 * @tc.name: InputWindowsManagerTest_IsPointerVisible_001
 * @tc.desc: Test IsPointerVisible
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_IsPointerVisible_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    bool ret = pointerDrawingManager.IsPointerVisible();
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_DeletePointerVisible_001
 * @tc.desc: Test DeletePointerVisible
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DeletePointerVisible_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    int32_t pid = 1;
    PointerDrawingManager::PidInfo info = { .pid = 1, .visible = true };
    pointerDrawingManager.pidInfos_.push_back(info);
    info = { .pid = 2, .visible = true };
    pointerDrawingManager.pidInfos_.push_back(info);
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DeletePointerVisible(pid));
}

/**
 * @tc.name: InputWindowsManagerTest_SetPointerLocation_001
 * @tc.desc: Test SetPointerLocation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerLocation_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    int32_t x = 100;
    int32_t y = 100;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.SetPointerLocation(x, y));
}

/**
 * @tc.name: InputWindowsManagerTest_UpdateDefaultPointerStyle_001
 * @tc.desc: Test UpdateDefaultPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_UpdateDefaultPointerStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    int32_t pid = 1;
    int32_t windowId = 1;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    pointerStyle.color = 0;
    pointerStyle.size = 2;
    int32_t ret = pointerDrawingManager.UpdateDefaultPointerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, RET_OK);
    windowId = -1;
    ret = pointerDrawingManager.UpdateDefaultPointerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_SetPointerStylePreference_001
 * @tc.desc: Test SetPointerStylePreference
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerStylePreference_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    pointerStyle.color = 0;
    pointerStyle.size = 2;
    int32_t ret = pointerDrawingManager.SetPointerStylePreference(pointerStyle);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: InputWindowsManagerTest_CheckPointerStyleParam_001
 * @tc.desc: Test CheckPointerStyleParam
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_CheckPointerStyleParam_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    PointerStyle pointerStyle;
    pointerStyle.id = EAST;
    pointerStyle.color = 0;
    pointerStyle.size = 2;
    int32_t windowId = -2;
    bool ret = pointerDrawingManager.CheckPointerStyleParam(windowId, pointerStyle);
    EXPECT_FALSE(ret);
    windowId = 1;
    ret = pointerDrawingManager.CheckPointerStyleParam(windowId, pointerStyle);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: InputWindowsManagerTest_CheckMouseIconPath_001
 * @tc.desc: Test CheckMouseIconPath
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_CheckMouseIconPath_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.CheckMouseIconPath());
}

/**
 * @tc.name: InputWindowsManagerTest_DrawPixelmap_001
 * @tc.desc: Test DrawPixelmap
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawPixelmap_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    pointerDrawingManager.userIcon_ = std::make_unique<OHOS::Media::PixelMap>();
    OHOS::Rosen::Drawing::Canvas canvas;
    MOUSE_ICON mouseStyle = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPixelmap(canvas, mouseStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_DrawPixelmap_002
 * @tc.desc: Test DrawPixelmap
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawPixelmap_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    pointerDrawingManager.userIcon_ = std::make_unique<OHOS::Media::PixelMap>();
    OHOS::Rosen::Drawing::Canvas canvas;
    MOUSE_ICON mouseStyle = MOUSE_ICON::RUNNING;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPixelmap(canvas, mouseStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_DrawPixelmap_003
 * @tc.desc: Test DrawPixelmap
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawPixelmap_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    pointerDrawingManager.userIcon_ = std::make_unique<OHOS::Media::PixelMap>();
    OHOS::Rosen::Drawing::Canvas canvas;
    MOUSE_ICON mouseStyle = MOUSE_ICON::WEST_EAST;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPixelmap(canvas, mouseStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_SetCustomCursor_001
 * @tc.desc: Test SetCustomCursor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetCustomCursor_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    const std::string iconPath = "/system/etc/multimodalinput/mouse_icon/North_South.svg";
    std::unique_ptr<OHOS::Media::PixelMap> pixelMap = SetMouseIconTest(iconPath);
    ASSERT_NE(pixelMap, nullptr);
    int32_t pid = -1;
    int32_t windowId = 1;
    int32_t focusX = 2;
    int32_t focusY = 3;
    int32_t ret = pointerDrawingManager.SetCustomCursor((void *)pixelMap.get(), pid, windowId, focusX, focusY);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetCustomCursor_002
 * @tc.desc: Test SetCustomCursor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetCustomCursor_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    const std::string iconPath = "/system/etc/multimodalinput/mouse_icon/North_South.svg";
    std::unique_ptr<OHOS::Media::PixelMap> pixelMap = SetMouseIconTest(iconPath);
    ASSERT_NE(pixelMap, nullptr);
    int32_t pid = 1;
    int32_t windowId = -1;
    int32_t focusX = 2;
    int32_t focusY = 3;
    int32_t ret = pointerDrawingManager.SetCustomCursor((void *)pixelMap.get(), pid, windowId, focusX, focusY);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetCustomCursor_003
 * @tc.desc: Test SetCustomCursor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetCustomCursor_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    const std::string iconPath = "/system/etc/multimodalinput/mouse_icon/North_South.svg";
    std::unique_ptr<OHOS::Media::PixelMap> pixelMap = SetMouseIconTest(iconPath);
    ASSERT_NE(pixelMap, nullptr);
    int32_t pid = 1;
    int32_t windowId = 2;
    int32_t focusX = 2;
    int32_t focusY = 3;
    int32_t ret = pointerDrawingManager.SetCustomCursor((void *)pixelMap.get(), pid, windowId, focusX, focusY);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetCustomCursor_004
 * @tc.desc: Test SetCustomCursor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetCustomCursor_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    const std::string iconPath = "/system/etc/multimodalinput/mouse_icon/North_South.svg";
    std::unique_ptr<OHOS::Media::PixelMap> pixelMap = SetMouseIconTest(iconPath);
    ASSERT_NE(pixelMap, nullptr);
    int32_t pid = 2;
    int32_t windowId = 2;
    int32_t focusX = -1;
    int32_t focusY = 3;
    int32_t ret = pointerDrawingManager.SetCustomCursor((void *)pixelMap.get(), pid, windowId, focusX, focusY);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetCustomCursor_005
 * @tc.desc: Test SetCustomCursor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetCustomCursor_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    const std::string iconPath = "/system/etc/multimodalinput/mouse_icon/North_South.svg";
    std::unique_ptr<OHOS::Media::PixelMap> pixelMap = SetMouseIconTest(iconPath);
    ASSERT_NE(pixelMap, nullptr);
    int32_t pid = 2;
    int32_t windowId = 2;
    int32_t focusX = 3;
    int32_t focusY = 4;
    int32_t ret = pointerDrawingManager.SetCustomCursor((void *)pixelMap.get(), pid, windowId, focusX, focusY);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetMouseIcon_002
 * @tc.desc: Test SetMouseIcon
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetMouseIcon_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    const std::string iconPath = "/system/etc/multimodalinput/mouse_icon/North_South.svg";
    std::unique_ptr<OHOS::Media::PixelMap> pixelMap = SetMouseIconTest(iconPath);
    ASSERT_NE(pixelMap, nullptr);
    int32_t pid = -1;
    int32_t windowId = 2;
    int32_t ret = pointerDrawingManager.SetMouseIcon(pid, windowId, (void *)pixelMap.get());
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetMouseIcon_003
 * @tc.desc: Test SetMouseIcon
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetMouseIcon_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    const std::string iconPath = "/system/etc/multimodalinput/mouse_icon/North_South.svg";
    std::unique_ptr<OHOS::Media::PixelMap> pixelMap = SetMouseIconTest(iconPath);
    ASSERT_NE(pixelMap, nullptr);
    int32_t pid = 1;
    int32_t windowId = -2;
    int32_t ret = pointerDrawingManager.SetMouseIcon(pid, windowId, (void *)pixelMap.get());
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetMouseIcon_004
 * @tc.desc: Test SetMouseIcon
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetMouseIcon_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    const std::string iconPath = "/system/etc/multimodalinput/mouse_icon/North_South.svg";
    std::unique_ptr<OHOS::Media::PixelMap> pixelMap = SetMouseIconTest(iconPath);
    ASSERT_NE(pixelMap, nullptr);
    int32_t pid = 1;
    int32_t windowId = 2;
    int32_t ret = pointerDrawingManager.SetMouseIcon(pid, windowId, (void *)pixelMap.get());
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetMouseIcon_005
 * @tc.desc: Test SetMouseIcon
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetMouseIcon_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    const std::string iconPath = "/system/etc/multimodalinput/mouse_icon/North_South.svg";
    std::unique_ptr<OHOS::Media::PixelMap> pixelMap = SetMouseIconTest(iconPath);
    ASSERT_NE(pixelMap, nullptr);
    int32_t pid = 2;
    int32_t windowId = 2;
    int32_t ret = pointerDrawingManager.SetMouseIcon(pid, windowId, (void *)pixelMap.get());
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_SetMouseHotSpot_002
 * @tc.desc: Test SetMouseHotSpot
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetMouseHotSpot_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    int32_t pid = -1;
    int32_t windowId = 2;
    int32_t hotSpotX = 3;
    int32_t hotSpotY = 4;
    int32_t ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    pid = 1;
    windowId = -2;
    hotSpotX = 3;
    hotSpotY = 4;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    pid = 1;
    windowId = 2;
    hotSpotX = 3;
    hotSpotY = 4;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    pid = 2;
    windowId = 2;
    hotSpotX = -3;
    hotSpotY = -4;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    pid = 2;
    windowId = 2;
    hotSpotX = 3;
    hotSpotY = 4;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InputWindowsManagerTest_OnDisplayInfo_001
 * @tc.desc: Test OnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_OnDisplayInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    DisplayInfo displaysInfo;
    DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    displayGroupInfo.focusWindowId = 0;
    displayGroupInfo.width = 0;
    displayGroupInfo.height = 0;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    pointerDrawingManager.surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    pointerDrawingManager.surfaceNode_->SetFrameGravity(Rosen::Gravity::RESIZE_ASPECT_FILL);
    pointerDrawingManager.surfaceNode_->SetPositionZ(Rosen::RSSurfaceNode::POINTER_WINDOW_POSITION_Z);
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.OnDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_OnDisplayInfo_002
 * @tc.desc: Test OnDisplayInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_OnDisplayInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    DisplayInfo displaysInfo;
    DisplayGroupInfo displayGroupInfo;
    displayGroupInfo.displaysInfo.push_back(displaysInfo);
    displayGroupInfo.focusWindowId = 0;
    displayGroupInfo.width = 0;
    displayGroupInfo.height = 0;
    pointerDrawingManager.surfaceNode_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.OnDisplayInfo(displayGroupInfo));
}

/**
 * @tc.name: InputWindowsManagerTest_DrawManager_001
 * @tc.desc: Test DrawManager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawManager_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = true;
    pointerDrawingManager.surfaceNode_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawManager());
}

/**
 * @tc.name: InputWindowsManagerTest_DrawManager_002
 * @tc.desc: Test DrawManager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawManager_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = true;
    pointerDrawingManager.surfaceNode_ = nullptr;
    pointerDrawingManager.displayInfo_.displayDirection = DIRECTION0;
    pointerDrawingManager.lastPhysicalX_ = -1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawManager());
}

/**
 * @tc.name: InputWindowsManagerTest_DrawManager_003
 * @tc.desc: Test DrawManager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawManager_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = true;
    pointerDrawingManager.surfaceNode_ = nullptr;
    pointerDrawingManager.displayInfo_.displayDirection = DIRECTION90;
    pointerDrawingManager.lastPhysicalX_ = 2;
    pointerDrawingManager.lastPhysicalY_ = 2;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawManager());
}

/**
 * @tc.name: InputWindowsManagerTest_DrawManager_004
 * @tc.desc: Test DrawManager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawManager_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    pointerDrawingManager.hasDisplay_ = false;
    pointerDrawingManager.hasPointerDevice_ = true;
    pointerDrawingManager.surfaceNode_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawManager());
}

/**
 * @tc.name: InputWindowsManagerTest_DrawPointerStyle_002
 * @tc.desc: Test DrawPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawPointerStyle_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    pointerStyle.color = 0;
    pointerStyle.size = 2;
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = true;
    pointerDrawingManager.surfaceNode_ = nullptr;
    pointerDrawingManager.displayInfo_.displayDirection = DIRECTION0;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_DrawPointerStyle_003
 * @tc.desc: Test DrawPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawPointerStyle_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    pointerStyle.color = 0;
    pointerStyle.size = 2;
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = true;
    pointerDrawingManager.surfaceNode_ = nullptr;
    pointerDrawingManager.displayInfo_.displayDirection = DIRECTION90;
    pointerDrawingManager.lastPhysicalX_ = -1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_DrawPointerStyle_004
 * @tc.desc: Test DrawPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawPointerStyle_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    pointerStyle.color = 0;
    pointerStyle.size = 2;
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = true;
    pointerDrawingManager.surfaceNode_ = nullptr;
    pointerDrawingManager.displayInfo_.displayDirection = DIRECTION90;
    pointerDrawingManager.lastPhysicalX_ = 2;
    pointerDrawingManager.lastPhysicalY_ = 2;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_DrawPointerStyle_005
 * @tc.desc: Test DrawPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawPointerStyle_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    pointerStyle.color = 0;
    pointerStyle.size = 2;
    pointerDrawingManager.hasDisplay_ = false;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_DrawPointer_001
 * @tc.desc: Test DrawPointer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawPointer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    pointerDrawingManager->DrawPointer(1, 100, 100, pointerStyle, DIRECTION180);
    EXPECT_EQ(pointerDrawingManager->lastDirection_, DIRECTION180);
    pointerDrawingManager->DrawPointer(1, 200, 200, pointerStyle, DIRECTION270);
    EXPECT_EQ(pointerDrawingManager->lastDirection_, DIRECTION270);
}

/**
 * @tc.name: InputWindowsManagerTest_DrawPointerStyle_001
 * @tc.desc: Test DrawPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawPointerStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    PointerStyle pointerStyle;
    pointerStyle.id = EAST;
    pointerStyle.color = 0;
    pointerStyle.size = 2;
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = true;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
    pointerDrawingManager.lastPhysicalX_ = -1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
}

/**
 * @tc.name: InputWindowsManagerTest_SetMouseHotSpot_003
 * @tc.desc: Test SetMouseHotSpot
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetMouseHotSpot_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto winmgrmock = std::make_shared<InputWindowsManagerMock>();
    PointerDrawingManager pointerDrawingManager;
    int32_t pid = 1;
    int32_t windowId = -2;
    EXPECT_CALL(*winmgrmock, CheckWindowIdPermissionByPid).WillRepeatedly(testing::Return(RET_OK));
    int32_t hotSpotX = -1;
    int32_t hotSpotY = 2;
    pointerDrawingManager.userIcon_ = nullptr;
    int32_t ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    hotSpotX = 1;
    hotSpotY = -2;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    hotSpotX = 1;
    hotSpotY = 2;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    pointerDrawingManager.userIcon_ = std::make_unique<OHOS::Media::PixelMap>();
    ASSERT_NE(pointerDrawingManager.userIcon_, nullptr);
    hotSpotX = -1;
    hotSpotY = 2;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    hotSpotX = -1;
    hotSpotY = -2;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    hotSpotX = 1;
    hotSpotY = -2;
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    hotSpotX = 3;
    hotSpotY = 4;
    pointerDrawingManager.userIcon_ = std::make_unique<OHOS::Media::PixelMap>();
    ASSERT_NE(pointerDrawingManager.userIcon_, nullptr);
    ret = pointerDrawingManager.SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
    ASSERT_EQ(ret, RET_ERR);
    testing::Mock::AllowLeak(winmgrmock.get());
}
 
/**
 * @tc.name: InputWindowsManagerTest_SetPointerColor_002
 * @tc.desc: Test SetPointerColor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerColor_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    pointerDrawingManager->surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_TRUE(pointerDrawingManager->surfaceNode_ != nullptr);
    pointerDrawingManager->SetPointerColor(16777216);
    int32_t color = pointerDrawingManager->GetPointerColor();
    EXPECT_EQ(color, RET_OK);
}
 
/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerDevice_002
 * @tc.desc: Test UpdatePointerDevice
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_UpdatePointerDevice_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    EXPECT_EQ(pointerDrawingManager->pidInfos_.size(), 100);
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager->UpdatePointerDevice(true, false, true));
    EXPECT_EQ(pointerDrawingManager->pidInfos_.size(), 100);
    pointerDrawingManager->surfaceNode_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager->UpdatePointerDevice(false, false, true));
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    pointerDrawingManager->surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_TRUE(pointerDrawingManager->surfaceNode_ != nullptr);
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager->UpdatePointerDevice(false, false, true));
}
 
/**
 * @tc.name: InputWindowsManagerTest_DrawManager_005
 * @tc.desc: Test DrawManager
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawManager_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto winmgrmock = std::make_shared<InputWindowsManagerMock>();
    PointerDrawingManager pointerDrawingManager;
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = true;
    pointerDrawingManager.surfaceNode_ = nullptr;
    EXPECT_CALL(*winmgrmock, CheckWindowIdPermissionByPid).WillRepeatedly(testing::Return(RET_ERR));
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawManager());
    EXPECT_CALL(*winmgrmock, CheckWindowIdPermissionByPid).WillRepeatedly(testing::Return(RET_OK));
    pointerDrawingManager.lastPhysicalX_ = -1;
    pointerDrawingManager.lastPhysicalY_ = 1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawManager());
    pointerDrawingManager.lastPhysicalX_ = 1;
    pointerDrawingManager.lastPhysicalY_ = -1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawManager());
    pointerDrawingManager.lastPhysicalX_ = -1;
    pointerDrawingManager.lastPhysicalY_ = -1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawManager());
    EXPECT_CALL(*winmgrmock, CheckWindowIdPermissionByPid).WillRepeatedly(testing::Return(RET_OK));
    pointerDrawingManager.lastPhysicalX_ = 1;
    pointerDrawingManager.lastPhysicalY_ = 1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawManager());
    testing::Mock::AllowLeak(winmgrmock.get());
}
 
/**
 * @tc.name: InputWindowsManagerTest_SetPointerVisible_002
 * @tc.desc: Test SetPointerVisible
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerVisible_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto winmgrmock = std::make_shared<InputWindowsManagerMock>();
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    EXPECT_CALL(*winmgrmock, GetExtraData).WillRepeatedly(testing::Return(ExtraData{true}));
    int32_t pid = 1;
    bool visible = true;
    int32_t priority = 0;
    int32_t ret = pointerDrawingManager->SetPointerVisible(pid, visible, priority, false);
    ASSERT_EQ(ret, RET_OK);
    visible = false;
    priority = 0;
    ret = pointerDrawingManager->SetPointerVisible(pid, visible, priority, false);
    ASSERT_EQ(ret, RET_OK);
    visible = true;
    priority = 1;
    ret = pointerDrawingManager->SetPointerVisible(pid, visible, priority, false);
    ASSERT_EQ(ret, RET_OK);
    visible = false;
    priority = 1;
    ret = pointerDrawingManager->SetPointerVisible(pid, visible, priority, false);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_CALL(*winmgrmock, GetExtraData).WillRepeatedly(testing::Return(ExtraData{false}));
    visible = false;
    priority = 0;
    ret = pointerDrawingManager->SetPointerVisible(pid, visible, priority, false);
    ASSERT_EQ(ret, RET_OK);
    visible = true;
    priority = 1;
    ret = pointerDrawingManager->SetPointerVisible(pid, visible, priority, false);
    ASSERT_EQ(ret, RET_OK);
    visible = false;
    priority = 1;
    ret = pointerDrawingManager->SetPointerVisible(pid, visible, priority, false);
    ASSERT_EQ(ret, RET_OK);
    testing::Mock::AllowLeak(winmgrmock.get());
}
 
/**
 * @tc.name: InputWindowsManagerTest_SetPointerLocation_002
 * @tc.desc: Test SetPointerLocation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerLocation_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    int32_t x = 100;
    int32_t y = 100;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    pointerDrawingManager.surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_TRUE(pointerDrawingManager.surfaceNode_ != nullptr);
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.SetPointerLocation(x, y));
}
 
/**
 * @tc.name: InputWindowsManagerTest_UpdateDefaultPointerStyle_002
 * @tc.desc: Test UpdateDefaultPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_UpdateDefaultPointerStyle_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto winmgrmock = std::make_shared<InputWindowsManagerMock>();
    PointerDrawingManager pointerDrawingManager;
    int32_t pid = 1;
    int32_t windowId = -1;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    pointerStyle.color = 0;
    pointerStyle.size = 2;
    EXPECT_CALL(*winmgrmock, GetPointerStyle).WillRepeatedly(testing::Return(RET_ERR));
    int32_t ret = pointerDrawingManager.UpdateDefaultPointerStyle(pid, windowId, pointerStyle);
    EXPECT_EQ(ret, RET_OK);
    testing::Mock::AllowLeak(winmgrmock.get());
}
 
/**
 * @tc.name: InputWindowsManagerTest_GetPointerStyle_001
 * @tc.desc: Test GetPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_GetPointerStyle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto winmgrmock = std::make_shared<InputWindowsManagerMock>();
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    int32_t pid = 1;
    int32_t windowId = 2;
    bool isUiExtension = true;
    PointerStyle pointerStyle;
    EXPECT_CALL(*winmgrmock, GetPointerStyle).WillRepeatedly(testing::Return(RET_ERR));
    int32_t ret = pointerDrawingManager->GetPointerStyle(pid, windowId, pointerStyle, isUiExtension);
    EXPECT_EQ(ret, RET_OK);
    testing::Mock::AllowLeak(winmgrmock.get());
}
 
/**
 * @tc.name: InputWindowsManagerTest_DrawPointerStyle_006
 * @tc.desc: Test DrawPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_DrawPointerStyle_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawingManager;
    PointerStyle pointerStyle;
    pointerStyle.id = 0;
    pointerStyle.color = 0;
    pointerStyle.size = 2;
    pointerDrawingManager.hasDisplay_ = false;
    pointerDrawingManager.hasPointerDevice_ = true;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = false;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
    pointerDrawingManager.hasDisplay_ = false;
    pointerDrawingManager.hasPointerDevice_ = false;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = true;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    pointerDrawingManager.surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_TRUE(pointerDrawingManager.surfaceNode_ != nullptr);
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
    pointerDrawingManager.hasDisplay_ = true;
    pointerDrawingManager.hasPointerDevice_ = true;
    pointerDrawingManager.surfaceNode_ = nullptr;
    pointerDrawingManager.lastPhysicalX_ = -1;
    pointerDrawingManager.lastPhysicalY_ = 1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
    pointerDrawingManager.lastPhysicalX_ = 1;
    pointerDrawingManager.lastPhysicalY_ = -1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
    pointerDrawingManager.lastPhysicalX_ = -1;
    pointerDrawingManager.lastPhysicalY_ = -1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
    pointerDrawingManager.lastPhysicalX_ = 1;
    pointerDrawingManager.lastPhysicalY_ = 1;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager.DrawPointerStyle(pointerStyle));
}

#ifdef OHOS_BUILD_ENABLE_MAGICCURSOR
/**
 * @tc.name: InputWindowsManagerTest_SetPixelMap
 * @tc.desc: Test SetPixelMap
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPixelMap, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager manager;
    std::shared_ptr<OHOS::Media::PixelMap> pixelMap = nullptr;
    ASSERT_NO_FATAL_FAILURE(manager.SetPixelMap(pixelMap));
}
#endif // OHOS_BUILD_ENABLE_MAGICCURSOR

/**
 * @tc.name: PointerDrawingManagerTest_SwitchPointerStyle
 * @tc.desc: Test SwitchPointerStyle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, PointerDrawingManagerTest_SwitchPointerStyle, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawMgr;
    pointerDrawMgr.lastMouseStyle_.id = 2;
    ASSERT_NO_FATAL_FAILURE(pointerDrawMgr.SwitchPointerStyle());
}

/**
 * @tc.name: PointerDrawingManagerTest_CreateMagicCursorChangeObserver
 * @tc.desc: Test CreateMagicCursorChangeObserver
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, PointerDrawingManagerTest_CreateMagicCursorChangeObserver, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawMgr;
    ASSERT_NO_FATAL_FAILURE(pointerDrawMgr.CreateMagicCursorChangeObserver());
}

/**
 * @tc.name: PointerDrawingManagerTest_UpdateStyleOptions
 * @tc.desc: Test UpdateStyleOptions
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, PointerDrawingManagerTest_UpdateStyleOptions, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawMgr;
    pointerDrawMgr.pid_ = 100;
    ASSERT_NO_FATAL_FAILURE(pointerDrawMgr.UpdateStyleOptions());
}

/**
 * @tc.name: PointerDrawingManagerTest_InitPointerObserver
 * @tc.desc: Test InitPointerObserver
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, PointerDrawingManagerTest_InitPointerObserver, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawMgr;
    pointerDrawMgr.hasInitObserver_ = true;
    ASSERT_NO_FATAL_FAILURE(pointerDrawMgr.InitPointerObserver());

    pointerDrawMgr.hasInitObserver_ = false;
    ASSERT_NO_FATAL_FAILURE(pointerDrawMgr.InitPointerObserver());
}

/**
 * @tc.name: PointerDrawingManagerTest_AdjustMouseFocusByDirection90
 * @tc.desc: Test AdjustMouseFocusByDirection90
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, PointerDrawingManagerTest_AdjustMouseFocusByDirection90, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerDrawingManager pointerDrawMgr;
    ICON_TYPE iconType = ANGLE_SW;
    int32_t physicalX = 500;
    int32_t physicalY = 500;
    ASSERT_NO_FATAL_FAILURE(pointerDrawMgr.AdjustMouseFocusByDirection90(iconType, physicalX, physicalY));
    iconType = ANGLE_CENTER;
    ASSERT_NO_FATAL_FAILURE(pointerDrawMgr.AdjustMouseFocusByDirection90(iconType, physicalX, physicalY));
    iconType = ANGLE_NW_RIGHT;
    ASSERT_NO_FATAL_FAILURE(pointerDrawMgr.AdjustMouseFocusByDirection90(iconType, physicalX, physicalY));
    iconType = ANGLE_NW;
    pointerDrawMgr.userIcon_ = CreatePixelMap(MIDDLE_PIXEL_MAP_WIDTH, MIDDLE_PIXEL_MAP_HEIGHT);
    ASSERT_NE(pointerDrawMgr.userIcon_, nullptr);
    pointerDrawMgr.currentMouseStyle_.id = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    ASSERT_NO_FATAL_FAILURE(pointerDrawMgr.AdjustMouseFocusByDirection90(iconType, physicalX, physicalY));
    pointerDrawMgr.userIcon_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(pointerDrawMgr.AdjustMouseFocusByDirection90(iconType, physicalX, physicalY));
}

/**
 * @tc.name: InputWindowsManagerTest_SetPointerColor_003
 * @tc.desc: Test SetPointerColor
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerColor_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    pointerDrawingManager->surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_TRUE(pointerDrawingManager->surfaceNode_ != nullptr);
    pointerDrawingManager->SetPointerColor(16777216);
    int32_t color = pointerDrawingManager->GetPointerColor();
    EXPECT_EQ(color, RET_OK);
    pointerDrawingManager->surfaceNode_ = nullptr;
    ASSERT_TRUE(pointerDrawingManager->surfaceNode_ == nullptr);
    #ifdef OHOS_BUILD_ENABLE_MAGICCURSOR
    MAGIC_CURSOR->isExistDefaultStyle = false;
    int32_t ret = pointerDrawingManager->SetPointerColor(16777216);
    EXPECT_EQ(ret, RET_OK);
    MAGIC_CURSOR->isExistDefaultStyle = true;
    ret = pointerDrawingManager->SetPointerColor(16777216);
    EXPECT_EQ(ret, RET_OK);
    #endif // OHOS_BUILD_ENABLE_MAGICCURSOR
}
 
/**
 * @tc.name: InputWindowsManagerTest_SetPointerSize_002
 * @tc.desc: Test SetPointerSize
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetPointerSize_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    pointerDrawingManager->SetPointerSize(0);
    int32_t pointerSize = pointerDrawingManager->GetPointerSize();
    EXPECT_EQ(pointerSize, 1);
    pointerDrawingManager->SetPointerSize(8);
    pointerSize = pointerDrawingManager->GetPointerSize();
    EXPECT_EQ(pointerSize, 7);
    pointerDrawingManager->surfaceNode_ = nullptr;
    ASSERT_TRUE(pointerDrawingManager->surfaceNode_ == nullptr);
    pointerDrawingManager->SetPointerSize(5);
    pointerSize = pointerDrawingManager->GetPointerSize();
    EXPECT_EQ(pointerSize, 5);
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    pointerDrawingManager->surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_TRUE(pointerDrawingManager->surfaceNode_ != nullptr);
    #ifdef OHOS_BUILD_ENABLE_MAGICCURSOR
    MAGIC_CURSOR->isExistDefaultStyle = false;
    int32_t ret = pointerDrawingManager->SetPointerSize(5);
    EXPECT_EQ(ret, RET_OK);
    MAGIC_CURSOR->isExistDefaultStyle = true;
    ret = pointerDrawingManager->SetPointerSize(5);
    EXPECT_EQ(ret, RET_OK);
    #endif // OHOS_BUILD_ENABLE_MAGICCURSOR
}
 
/**
 * @tc.name: InputWindowsManagerTest_UpdatePointerDevice_003
 * @tc.desc: Test UpdatePointerDevice
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_UpdatePointerDevice_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<PointerDrawingManager> pointerDrawingManager =
        std::static_pointer_cast<PointerDrawingManager>(IPointerDrawingManager::GetInstance());
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager->UpdatePointerDevice(true, false, true));
    pointerDrawingManager->UpdatePointerDevice(true, true, true);
    pointerDrawingManager->surfaceNode_ = nullptr;
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager->UpdatePointerDevice(false, false, true));
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "pointer window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    pointerDrawingManager->surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_TRUE(pointerDrawingManager->surfaceNode_ != nullptr);
    ASSERT_NO_FATAL_FAILURE(pointerDrawingManager->UpdatePointerDevice(false, false, true));
}
 
/**
 * @tc.name: InputWindowsManagerTest_SetTargetDevice_001
 * @tc.desc: Test SetTargetDevice
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PointerDrawingManagerTest, InputWindowsManagerTest_SetTargetDevice_001, TestSize.Level1)
{
    #ifdef OHOS_BUILD_ENABLE_HARDWARE_CURSOR
    CALL_TEST_DEBUG;
    uint32_t devId = 0;
    hardwareCursorPointerManager_->devId_ = 0;
    hardwareCursorPointerManager_->SetTargetDevice(devId);
    ASSERT_FALSE(hardwareCursorPointerManager_->isEnableState_);
    devId = 10;
    hardwareCursorPointerManager_->SetTargetDevice(devId);
    ASSERT_FALSE(hardwareCursorPointerManager_->isEnableState_);
    #endif // OHOS_BUILD_ENABLE_HARDWARE_CURSOR
}
} // namespace MMI
} // namespace OHOS