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

#include <gtest/gtest.h>

#include "image_source.h"
#include "image_type.h"
#include "image_utils.h"

#include "mmi_log.h"
#include "pointer_event.h"
#include "knuckle_dynamic_drawing_manager.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "KnuckleDynamicDrawingManagerTest"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
constexpr int32_t POINT_SYSTEM_SIZE = 50;
constexpr int32_t MAX_DIVERGENCE_NUM = 10;
constexpr int32_t MAX_POINTER_COLOR = 0xff00ff;
constexpr std::string_view SCREEN_READ_ENABLE { "1" };
} // namespace

class KnuckleDynamicDrawingManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp(void)
    {
        DisplayInfo displayInfo = { .id = 1, .x = 1, .y = 1, .width = 1, .height = 1,
            .dpi = 240, .name = "display", .uniq = "xx" };
        if (knuckleDynamicDrawingMgr == nullptr) {
            knuckleDynamicDrawingMgr = std::make_shared<KnuckleDynamicDrawingManager>();
        }
        knuckleDynamicDrawingMgr->UpdateDisplayInfo(displayInfo);
    }

    std::shared_ptr<Media::PixelMap> DecodeImageToPixelMap(const std::string &imagePath);
private:
    std::shared_ptr<KnuckleDynamicDrawingManager> knuckleDynamicDrawingMgr { nullptr };
};

std::shared_ptr<Media::PixelMap> KnuckleDynamicDrawingManagerTest::DecodeImageToPixelMap(const std::string &imagePath)
{
    CALL_DEBUG_ENTER;
    OHOS::Media::SourceOptions opts;
    uint32_t ret = 0;
    auto imageSource = OHOS::Media::ImageSource::CreateImageSource(imagePath, opts, ret);
    CHKPP(imageSource);
    std::set<std::string> formats;
    ret = imageSource->GetSupportedFormats(formats);
    OHOS::Media::DecodeOptions decodeOpts;
    decodeOpts.desiredSize = {
        .width = 80,
        .height = 80
    };

    decodeOpts.SVGOpts.fillColor = {.isValidColor = false, .color = MAX_POINTER_COLOR};
    decodeOpts.SVGOpts.strokeColor = {.isValidColor = false, .color = MAX_POINTER_COLOR};

    std::shared_ptr<OHOS::Media::PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, ret);
    if (pixelMap == nullptr) {
        MMI_HILOGE("The pixelMap is nullptr");
    }
    return pixelMap;
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_Normal
 * @tc.desc: Test Normal branch of covering KnuckleDynamicDrawHandler function
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest,
    KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_Normal, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetToolType(PointerEvent::TOOL_TYPE_MOUSE);
    pointerEvent->SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    knuckleDynamicDrawMgr.KnuckleDynamicDrawHandler(pointerEvent);
    EXPECT_FALSE(knuckleDynamicDrawMgr.isStop_);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_Abnormal
 * @tc.desc: Test Abnormal branch of covering KnuckleDynamicDrawHandler function
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest,
    KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_Abnormal, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetToolType(PointerEvent::TOOL_TYPE_KNUCKLE);
    pointerEvent->SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    pointerEvent->SetTargetDisplayId(50);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_DOWN);
    knuckleDynamicDrawMgr.KnuckleDynamicDrawHandler(pointerEvent);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_SWIPE_END);
    knuckleDynamicDrawMgr.KnuckleDynamicDrawHandler(pointerEvent);
    EXPECT_TRUE(knuckleDynamicDrawMgr.isDrawing_);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_InitPointerPathPaint
 * @tc.desc: Test Overrides InitPointerPathPaint function branches
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_InitPointerPathPaint, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    knuckleDynamicDrawMgr.glowTraceSystem_ = nullptr;
    knuckleDynamicDrawMgr.InitPointerPathPaint();
    std::string imagePath = "/system/etc/multimodalinput/mouse_icon/Default.svg";
    auto pixelMap = DecodeImageToPixelMap(imagePath);
    ASSERT_NE(pixelMap, nullptr);
    knuckleDynamicDrawMgr.glowTraceSystem_ =
        std::make_shared<KnuckleGlowTraceSystem>(POINT_SYSTEM_SIZE, pixelMap, MAX_DIVERGENCE_NUM);
    knuckleDynamicDrawMgr.InitPointerPathPaint();
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_IsSingleKnuckle
 * @tc.desc: Test Overrides IsSingleKnuckle function branches
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_IsSingleKnuckle, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    item.SetToolType(PointerEvent::TOOL_TYPE_KNUCKLE);
    pointerEvent->SetPointerId(1);
    pointerEvent->AddPointerItem(item);
    ASSERT_TRUE(knuckleDynamicDrawMgr.IsSingleKnuckle(pointerEvent));

    item.SetPointerId(2);
    item.SetToolType(PointerEvent::TOOL_TYPE_TOUCHPAD);
    pointerEvent->SetPointerId(2);
    pointerEvent->AddPointerItem(item);
    knuckleDynamicDrawMgr.canvasNode_ = nullptr;
    ASSERT_FALSE(knuckleDynamicDrawMgr.IsSingleKnuckle(pointerEvent));

    knuckleDynamicDrawMgr.canvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    ASSERT_NE(knuckleDynamicDrawMgr.canvasNode_, nullptr);
    ASSERT_FALSE(knuckleDynamicDrawMgr.IsSingleKnuckle(pointerEvent));
}

/**
 * @tc.name: KnuckleDrawingManagerTest_StartTouchDraw
 * @tc.desc: Test Overrides StartTouchDraw function branches
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_StartTouchDraw, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerId(1);
    knuckleDynamicDrawMgr.StartTouchDraw(pointerEvent);

    knuckleDynamicDrawMgr.canvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    ASSERT_NE(knuckleDynamicDrawMgr.canvasNode_, nullptr);
    knuckleDynamicDrawMgr.displayInfo_.width = 200;
    knuckleDynamicDrawMgr.displayInfo_.height = 200;
    std::string imagePath = "/system/etc/multimodalinput/mouse_icon/Default.svg";
    auto pixelMap = DecodeImageToPixelMap(imagePath);
    knuckleDynamicDrawMgr.glowTraceSystem_ =
        std::make_shared<KnuckleGlowTraceSystem>(POINT_SYSTEM_SIZE, pixelMap, MAX_DIVERGENCE_NUM);
    knuckleDynamicDrawMgr.isDrawing_ = true;
    knuckleDynamicDrawMgr.StartTouchDraw(pointerEvent);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_DrawGraphic
 * @tc.name: KnuckleDrawingManagerTest_DrawGraphic
 * @tc.desc: Test Overrides DrawGraphic function branches
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_DrawGraphic, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    pointerEvent->SetPointerId(1);

    knuckleDynamicDrawMgr.canvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    ASSERT_NE(knuckleDynamicDrawMgr.canvasNode_, nullptr);
    knuckleDynamicDrawMgr.displayInfo_.width = 200;
    knuckleDynamicDrawMgr.displayInfo_.height = 200;
    std::string imagePath = "/system/etc/multimodalinput/mouse_icon/Default.svg";
    auto pixelMap = DecodeImageToPixelMap(imagePath);
    knuckleDynamicDrawMgr.glowTraceSystem_ =
        std::make_shared<KnuckleGlowTraceSystem>(POINT_SYSTEM_SIZE, pixelMap, MAX_DIVERGENCE_NUM);
    knuckleDynamicDrawMgr.isDrawing_ = false;
    ASSERT_EQ(knuckleDynamicDrawMgr.DrawGraphic(pointerEvent), RET_OK);
    knuckleDynamicDrawMgr.isDrawing_ = true;
    ASSERT_EQ(knuckleDynamicDrawMgr.DrawGraphic(pointerEvent), RET_ERR);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_CreateTouchWindow
 * @tc.desc: Test Overrides CreateTouchWindow function branches
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_CreateTouchWindow, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    int32_t displayId = 10;
    knuckleDynamicDrawMgr.surfaceNode_ = nullptr;
    knuckleDynamicDrawMgr.displayInfo_.width = 200;
    knuckleDynamicDrawMgr.displayInfo_.height = 200;
    knuckleDynamicDrawMgr.CreateTouchWindow(displayId);

    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "touch window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    knuckleDynamicDrawMgr.surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_NE(knuckleDynamicDrawMgr.surfaceNode_, nullptr);
    knuckleDynamicDrawMgr.CreateTouchWindow(displayId);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_001
 * @tc.desc: Test KnuckleDynamicDrawHandler
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_001,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);

    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    int32_t displayX = 100;
    int32_t displayY = 100;
    item.SetDisplayX(displayX);
    item.SetDisplayY(displayY);
    item.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetTargetDisplayId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    knuckleDynamicDrawingMgr->KnuckleDynamicDrawHandler(pointerEvent);
    EXPECT_TRUE(knuckleDynamicDrawingMgr->isDrawing_);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_002
 * @tc.desc: Test KnuckleDynamicDrawHandler
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_002,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);

    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    int32_t displayX = 200;
    int32_t displayY = 200;
    item.SetDisplayX(displayX);
    item.SetDisplayY(displayY);
    item.SetToolType(PointerEvent::TOOL_TYPE_KNUCKLE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetTargetDisplayId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    knuckleDynamicDrawingMgr->KnuckleDynamicDrawHandler(pointerEvent);
    EXPECT_EQ(knuckleDynamicDrawingMgr->lastUpTime_, 0);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_003
 * @tc.desc: Test KnuckleDynamicDrawHandler
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_003,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);

    PointerEvent::PointerItem item1;
    item1.SetPointerId(0);
    int32_t displayX = 100;
    int32_t displayY = 200;
    item1.SetDisplayX(displayX);
    item1.SetDisplayY(displayY);
    item1.SetToolType(PointerEvent::TOOL_TYPE_KNUCKLE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetTargetDisplayId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item1);

    PointerEvent::PointerItem item2;
    item2.SetPointerId(1);
    displayX = 200;
    displayY = 200;
    item2.SetDisplayX(displayX);
    item2.SetDisplayY(displayY);
    item2.SetToolType(PointerEvent::TOOL_TYPE_KNUCKLE);
    pointerEvent->AddPointerItem(item2);
    knuckleDynamicDrawingMgr->KnuckleDynamicDrawHandler(pointerEvent);
    EXPECT_TRUE(knuckleDynamicDrawingMgr->isDrawing_);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_004
 * @tc.desc: Test KnuckleDynamicDrawHandler
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_004,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);

    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    int32_t displayX = 200;
    int32_t displayY = 200;
    item.SetDisplayX(displayX);
    item.SetDisplayY(displayY);
    item.SetToolType(PointerEvent::TOOL_TYPE_KNUCKLE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetTargetDisplayId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    knuckleDynamicDrawingMgr->KnuckleDynamicDrawHandler(pointerEvent);
    EXPECT_EQ(knuckleDynamicDrawingMgr->firstDownTime_, 0);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_005
 * @tc.desc: Test KnuckleDynamicDrawHandler
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_005,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);

    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    int32_t displayX = 200;
    int32_t displayY = 200;
    item.SetDisplayX(displayX);
    item.SetDisplayY(displayY);
    item.SetToolType(PointerEvent::TOOL_TYPE_KNUCKLE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetTargetDisplayId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    knuckleDynamicDrawingMgr->KnuckleDynamicDrawHandler(pointerEvent);
    EXPECT_NE(knuckleDynamicDrawingMgr->pointCounter_, 1);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_006
 * @tc.desc: Test KnuckleDynamicDrawHandler
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_KnuckleDynamicDrawHandler_006,
    TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);

    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    int32_t displayX = 200;
    int32_t displayY = 200;
    item.SetDisplayX(displayX);
    item.SetDisplayY(displayY);
    item.SetToolType(PointerEvent::TOOL_TYPE_KNUCKLE);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    pointerEvent->SetSourceType(PointerEvent::SOURCE_TYPE_TOUCHSCREEN);
    pointerEvent->SetTargetDisplayId(0);
    pointerEvent->SetPointerId(0);
    pointerEvent->AddPointerItem(item);
    knuckleDynamicDrawingMgr->KnuckleDynamicDrawHandler(pointerEvent);
    EXPECT_TRUE(knuckleDynamicDrawingMgr->traceControlPoints_.empty());
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_UpdateDisplayInfo_001
 * @tc.desc: Test UpdateDisplayInfo
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_UpdateDisplayInfo_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    DisplayInfo displayInfo = { .id = 1, .x = 1, .y = 1, .width = 1, .height = 1,
        .dpi = 240, .name = "display", .uniq = "xx" };
    knuckleDynamicDrawingMgr->UpdateDisplayInfo(displayInfo);
    EXPECT_EQ(knuckleDynamicDrawingMgr->displayInfo_.width, 1);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_UpdateDisplayInfo_002
 * @tc.desc: Test UpdateDisplayInfo
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_UpdateDisplayInfo_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    DisplayInfo displayInfo;
    knuckleDynamicDrawingMgr->UpdateDisplayInfo(displayInfo);
    EXPECT_EQ(knuckleDynamicDrawingMgr->displayInfo_.width, 0);
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_IsSingleKnuckle_001
 * @tc.desc: Test Overrides IsSingleKnuckle function branches
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_IsSingleKnuckle_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(10);
    item.SetToolType(PointerEvent::TOOL_TYPE_TOUCHPAD);
    pointerEvent->SetPointerId(10);
    pointerEvent->AddPointerItem(item);
    Rosen::Drawing::Point point = Rosen::Drawing::Point();
    knuckleDynamicDrawMgr.traceControlPoints_.push_back(point);
    EXPECT_FALSE(knuckleDynamicDrawMgr.IsSingleKnuckle(pointerEvent));
    knuckleDynamicDrawMgr.traceControlPoints_.clear();
    knuckleDynamicDrawMgr.isRotate_ = true;
    item.SetPointerId(20);
    item.SetToolType(PointerEvent::TOOL_TYPE_KNUCKLE);
    pointerEvent->SetPointerId(20);
    pointerEvent->AddPointerItem(item);
    EXPECT_TRUE(knuckleDynamicDrawMgr.IsSingleKnuckle(pointerEvent));
    pointerEvent->SetPointerId(10);
    knuckleDynamicDrawMgr.isRotate_ = true;
    EXPECT_FALSE(knuckleDynamicDrawMgr.IsSingleKnuckle(pointerEvent));
    EXPECT_FALSE(knuckleDynamicDrawMgr.IsSingleKnuckle(pointerEvent));
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_CheckPointerAction
 * @tc.desc: Test Overrides CheckPointerAction function branches
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_CheckPointerAction, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    knuckleDynamicDrawMgr.canvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    ASSERT_NE(knuckleDynamicDrawMgr.canvasNode_, nullptr);
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "touch window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    knuckleDynamicDrawMgr.surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    ASSERT_NE(knuckleDynamicDrawMgr.surfaceNode_, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(10);
    pointerEvent->AddPointerItem(item);
    item.SetPointerId(20);
    pointerEvent->AddPointerItem(item);
    knuckleDynamicDrawMgr.knuckleDrawMgr_ = std::make_shared<KnuckleDrawingManager>();
    knuckleDynamicDrawMgr.knuckleDrawMgr_->screenReadState_.state = SCREEN_READ_ENABLE;
    EXPECT_FALSE(knuckleDynamicDrawMgr.CheckPointerAction(pointerEvent));
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_CheckPointerAction_001
 * @tc.desc: Test Overrides CheckPointerAction function branches
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_CheckPointerAction_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(10);
    pointerEvent->AddPointerItem(item);
    knuckleDynamicDrawMgr.knuckleDrawMgr_ = std::make_shared<KnuckleDrawingManager>();
    knuckleDynamicDrawMgr.knuckleDrawMgr_->screenReadState_.state = "0";
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    EXPECT_TRUE(knuckleDynamicDrawMgr.CheckPointerAction(pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_UP);
    EXPECT_TRUE(knuckleDynamicDrawMgr.CheckPointerAction(pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_TRUE(knuckleDynamicDrawMgr.CheckPointerAction(pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_DOWN);
    EXPECT_TRUE(knuckleDynamicDrawMgr.CheckPointerAction(pointerEvent));
    knuckleDynamicDrawMgr.isStop_ = true;
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    EXPECT_FALSE(knuckleDynamicDrawMgr.CheckPointerAction(pointerEvent));
    knuckleDynamicDrawMgr.isStop_ = false;
    knuckleDynamicDrawMgr.traceControlPoints_.clear();
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_MOVE);
    EXPECT_FALSE(knuckleDynamicDrawMgr.CheckPointerAction(pointerEvent));
    Rosen::Drawing::Point point = Rosen::Drawing::Point();
    knuckleDynamicDrawMgr.traceControlPoints_.push_back(point);
    EXPECT_TRUE(knuckleDynamicDrawMgr.CheckPointerAction(pointerEvent));
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UNKNOWN);
    EXPECT_FALSE(knuckleDynamicDrawMgr.CheckPointerAction(pointerEvent));
}

/**
 * @tc.name: KnuckleDynamicDrawingManagerTest_ProcessUpAndCancelEvent
 * @tc.desc: Test Overrides ProcessUpAndCancelEvent function branches
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(KnuckleDynamicDrawingManagerTest, KnuckleDynamicDrawingManagerTest_ProcessUpAndCancelEvent, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    KnuckleDynamicDrawingManager knuckleDynamicDrawMgr;
    std::shared_ptr<PointerEvent> pointerEvent = PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    std::string imagePath = "/system/etc/multimodalinput/mouse_icon/Default.svg";
    auto pixelMap = DecodeImageToPixelMap(imagePath);
    knuckleDynamicDrawMgr.glowTraceSystem_ = std::make_shared<KnuckleGlowTraceSystem>(POINT_SYSTEM_SIZE,
        pixelMap, MAX_DIVERGENCE_NUM);
    pointerEvent->SetPointerId(10);
    pointerEvent->SetActionTime(1000);
    EXPECT_NO_FATAL_FAILURE(knuckleDynamicDrawMgr.ProcessUpAndCancelEvent(pointerEvent));
}
} // namespace MMI
} // namespace OHOS