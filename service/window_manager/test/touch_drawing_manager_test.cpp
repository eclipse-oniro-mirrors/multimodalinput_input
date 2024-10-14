/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "mmi_log.h"
#include "pointer_event.h"
#ifndef USE_ROSEN_DRAWING
#define USE_ROSEN_DRAWING
#endif
#include "touch_drawing_manager.h"
#include "window_info.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "TouchDrawingManagerTest"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
} // namespace
class TouchDrawingManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp(void)
    {
        DisplayInfo info;
        info.id = 1;
        info.x =1;
        info.y = 1;
        info.width = 1;
        info.height = 1;
        int32_t displayDpi = 240;
        info.dpi = displayDpi;
        info.name = "xx";
        info.uniq = "xx";
        info.direction = DIRECTION0;
        TOUCH_DRAWING_MGR->UpdateDisplayInfo(info);
    }
};

/**
 * @tc.name: TouchDrawingManagerTest_GetOriginalTouchScreenCoordinates_001
 * @tc.desc: Test GetOriginalTouchScreenCoordinates
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_GetOriginalTouchScreenCoordinates_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t width = 100;
    int32_t height = 200;
    int32_t physicalX = 50;
    int32_t physicalY = 60;
    TOUCH_DRAWING_MGR->GetOriginalTouchScreenCoordinates(DIRECTION0, width, height, physicalX, physicalY);
    EXPECT_EQ(physicalX, 50);
    EXPECT_EQ(physicalY, 60);
}

/**
 * @tc.name: TouchDrawingManagerTest_GetOriginalTouchScreenCoordinates_002
 * @tc.desc: Test GetOriginalTouchScreenCoordinates
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_GetOriginalTouchScreenCoordinates_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t width = 100;
    int32_t height = 200;
    int32_t physicalX = 50;
    int32_t physicalY = 60;
    TOUCH_DRAWING_MGR->GetOriginalTouchScreenCoordinates(DIRECTION90, width, height, physicalX, physicalY);
    EXPECT_EQ(physicalX, 60);
    EXPECT_EQ(physicalY, 50);
}

/**
 * @tc.name: TouchDrawingManagerTest_GetOriginalTouchScreenCoordinates_003
 * @tc.desc: Test GetOriginalTouchScreenCoordinates
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_GetOriginalTouchScreenCoordinates_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t width = 100;
    int32_t height = 200;
    int32_t physicalX = 50;
    int32_t physicalY = 60;
    TOUCH_DRAWING_MGR->GetOriginalTouchScreenCoordinates(DIRECTION180, width, height, physicalX, physicalY);
    EXPECT_EQ(physicalX, 50);
    EXPECT_EQ(physicalY, 140);
}

/**
 * @tc.name: TouchDrawingManagerTest_GetOriginalTouchScreenCoordinates_004
 * @tc.desc: Test GetOriginalTouchScreenCoordinates
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_GetOriginalTouchScreenCoordinates_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t width = 100;
    int32_t height = 200;
    int32_t physicalX = 50;
    int32_t physicalY = 60;
    TOUCH_DRAWING_MGR->GetOriginalTouchScreenCoordinates(DIRECTION270, width, height, physicalX, physicalY);
    EXPECT_EQ(physicalX, 140);
    EXPECT_EQ(physicalY, 50);
}

/**
 * @tc.name: TouchDrawingManagerTest_IsValidAction_001
 * @tc.desc: Test is valid action
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_IsValidAction_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TouchDrawingManager manager;
    bool ret = manager.IsValidAction(PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_TRUE(ret);
    ret = manager.IsValidAction(PointerEvent::POINTER_ACTION_PULL_DOWN);
    EXPECT_TRUE(ret);
    ret = manager.IsValidAction(PointerEvent::POINTER_ACTION_MOVE);
    EXPECT_TRUE(ret);
    ret = manager.IsValidAction(PointerEvent::POINTER_ACTION_PULL_MOVE);
    EXPECT_TRUE(ret);
    ret = manager.IsValidAction(PointerEvent::POINTER_ACTION_UP);
    EXPECT_TRUE(ret);
    ret = manager.IsValidAction(PointerEvent::POINTER_ACTION_PULL_UP);
    EXPECT_TRUE(ret);
    ret = manager.IsValidAction(100);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawPointerPositionHandler_001
 * @tc.desc: Test DrawPointerPositionHandler
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawPointerPositionHandler_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawPointerPositionHandler());
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawPointerPositionHandler_002
 * @tc.desc: Test DrawPointerPositionHandler
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawPointerPositionHandler_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    EXPECT_NE(pointerEvent, nullptr);
    TOUCH_DRAWING_MGR->pointerEvent_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawPointerPositionHandler());
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawTracker_001
 * @tc.desc: Test DrawTracker
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawTracker_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t x = 10;
    int32_t y = 10;
    int32_t pointerId = 0;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawTracker(x, y, pointerId));
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawTracker_002
 * @tc.desc: Test DrawTracker
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawTracker_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t x = 11;
    int32_t y = 11;
    int32_t pointerId = 5;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawTracker(x, y, pointerId));
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawCrosshairs_001
 * @tc.desc: Test DrawCrosshairs
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawCrosshairs_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t x = 11;
    int32_t y = 11;
    if (TOUCH_DRAWING_MGR->crosshairCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->crosshairCanvasNode_ = Rosen::RSCanvasNode::Create();
    }
    ASSERT_NE(TOUCH_DRAWING_MGR->crosshairCanvasNode_, nullptr);
    auto canvas = static_cast<TouchDrawingManager::RosenCanvas *>
        (TOUCH_DRAWING_MGR->crosshairCanvasNode_->BeginRecording(TOUCH_DRAWING_MGR->displayInfo_.width,
        TOUCH_DRAWING_MGR->displayInfo_.height));
    ASSERT_NE(canvas, nullptr);
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawCrosshairs(canvas, x, y));
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawCrosshairs_002
 * @tc.desc: Test DrawCrosshairs
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawCrosshairs_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t x = 11;
    int32_t y = 11;
    if (TOUCH_DRAWING_MGR->crosshairCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->crosshairCanvasNode_ = Rosen::RSCanvasNode::Create();
    }
    auto canvas = static_cast<TouchDrawingManager::RosenCanvas *>
        (TOUCH_DRAWING_MGR->crosshairCanvasNode_->BeginRecording(TOUCH_DRAWING_MGR->displayInfo_.width,
        TOUCH_DRAWING_MGR->displayInfo_.height));
    ASSERT_NE(canvas, nullptr);
    TOUCH_DRAWING_MGR->displayInfo_.direction = DIRECTION90;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawCrosshairs(canvas, x, y));
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawCrosshairs_003
 * @tc.desc: Test DrawCrosshairs
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawCrosshairs_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t x = 11;
    int32_t y = 11;
    if (TOUCH_DRAWING_MGR->crosshairCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->crosshairCanvasNode_ = Rosen::RSCanvasNode::Create();
    }
    auto canvas = static_cast<TouchDrawingManager::RosenCanvas *>
        (TOUCH_DRAWING_MGR->crosshairCanvasNode_->BeginRecording(TOUCH_DRAWING_MGR->displayInfo_.width,
        TOUCH_DRAWING_MGR->displayInfo_.height));
    ASSERT_NE(canvas, nullptr);
    TOUCH_DRAWING_MGR->displayInfo_.direction = DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawCrosshairs(canvas, x, y));
}

/**
 * @tc.name: TouchDrawingManagerTest_UpdatePointerPosition_001
 * @tc.desc: Test UpdatePointerPosition
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdatePointerPosition_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetPointerId(5);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    EXPECT_NE(pointerEvent, nullptr);
    TOUCH_DRAWING_MGR->pointerEvent_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->UpdatePointerPosition());
}

/**
 * @tc.name: TouchDrawingManagerTest_UpdatePointerPosition_002
 * @tc.desc: Test UpdatePointerPosition
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdatePointerPosition_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetPointerId(5);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    EXPECT_NE(pointerEvent, nullptr);
    TOUCH_DRAWING_MGR->pointerEvent_ = pointerEvent;
    TOUCH_DRAWING_MGR->currentPointerId_ = 5;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->UpdatePointerPosition());
}

/**
 * @tc.name: TouchDrawingManagerTest_UpdatePointerPosition_003
 * @tc.desc: Test UpdatePointerPosition
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdatePointerPosition_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetPointerId(0);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_NE(pointerEvent, nullptr);
    TOUCH_DRAWING_MGR->pointerEvent_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->UpdatePointerPosition());
}

/**
 * @tc.name: TouchDrawingManagerTest_UpdatePointerPosition_004
 * @tc.desc: Test UpdatePointerPosition
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdatePointerPosition_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetPointerId(0);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_NE(pointerEvent, nullptr);
    TOUCH_DRAWING_MGR->pointerEvent_ = pointerEvent;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->UpdatePointerPosition());
}

/**
 * @tc.name: TouchDrawingManagerTest_UpdatePointerPosition_005
 * @tc.desc: Test UpdatePointerPosition
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdatePointerPosition_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto pointerEvent = PointerEvent::Create();
    pointerEvent->SetPointerId(0);
    pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    EXPECT_NE(pointerEvent, nullptr);
    TOUCH_DRAWING_MGR->pointerEvent_ = pointerEvent;
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetPressed(true);
    TOUCH_DRAWING_MGR->lastPointerItem_.emplace_back(item);
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->UpdatePointerPosition());
}

/**
 * @tc.name: TouchDrawingManagerTest_ClearTracker_001
 * @tc.desc: Test ClearTracker
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_ClearTracker_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->trackerCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->trackerCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    }
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->ClearTracker());
}

/**
 * @tc.name: TouchDrawingManagerTest_ClearTracker_002
 * @tc.desc: Test ClearTracker
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_ClearTracker_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->trackerCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->trackerCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    }
    TOUCH_DRAWING_MGR->lastPointerItem_.clear();
    TOUCH_DRAWING_MGR->isDownAction_ = false;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->ClearTracker());
}

/**
 * @tc.name: TouchDrawingManagerTest_ClearTracker_003
 * @tc.desc: Test ClearTracker
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_ClearTracker_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->trackerCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->trackerCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    }
    PointerEvent::PointerItem item;
    item.SetPointerId(0);
    item.SetDisplayY(200);
    TOUCH_DRAWING_MGR->lastPointerItem_.emplace_back(item);
    TOUCH_DRAWING_MGR->isDownAction_ = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->ClearTracker());
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawLabels_001
 * @tc.desc: Test DrawLabels
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawLabels_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->isDownAction_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.direction = DIRECTION90;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawLabels());
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawLabels_002
 * @tc.desc: Test DrawLabels
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawLabels_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->labelsCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->labelsCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    }
    TOUCH_DRAWING_MGR->isDownAction_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.direction = DIRECTION180;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawLabels());
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawLabels_003
 * @tc.desc: Test DrawLabels
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawLabels_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->labelsCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->labelsCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    }
    TOUCH_DRAWING_MGR->isDownAction_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.direction = DIRECTION270;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawLabels());
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawLabels_004
 * @tc.desc: Test DrawLabels
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawLabels_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->labelsCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->labelsCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    }
    TOUCH_DRAWING_MGR->isDownAction_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.direction = DIRECTION270;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawLabels());
}

/**
 * @tc.name: TouchDrawingManagerTest_UpdateLabels_002
 * @tc.desc: Test UpdateLabels
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdateLabels_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->labelsCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->labelsCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    }
    TOUCH_DRAWING_MGR->pointerMode_.isShow = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->UpdateLabels());
}

/**
 * @tc.name: TouchDrawingManagerTest_CreateObserver_001
 * @tc.desc: Test CreateObserver
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_CreateObserver_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->hasBubbleObserver_ = false;
    TOUCH_DRAWING_MGR->hasPointerObserver_ = false;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->CreateObserver());
}

/**
 * @tc.name: TouchDrawingManagerTest_CreateObserver_002
 * @tc.desc: Test CreateObserver
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_CreateObserver_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->hasBubbleObserver_ = true;
    TOUCH_DRAWING_MGR->hasPointerObserver_ = false;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->CreateObserver());
}

/**
 * @tc.name: TouchDrawingManagerTest_CreateObserver_003
 * @tc.desc: Test CreateObserver
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_CreateObserver_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->hasBubbleObserver_ = false;
    TOUCH_DRAWING_MGR->hasPointerObserver_ = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->CreateObserver());
}

/**
 * @tc.name: TouchDrawingManagerTest_CreateObserver_004
 * @tc.desc: Test CreateObserver
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_CreateObserver_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->hasBubbleObserver_ = true;
    TOUCH_DRAWING_MGR->hasPointerObserver_ = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->CreateObserver());
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawRectItem_001
 * @tc.desc: Test DrawRectItem
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawRectItem_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TouchDrawingManager::RosenCanvas *canvas = nullptr;
    std::string text;
    Rosen::Drawing::Rect rect {};
    Rosen::Drawing::Color color {};
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawRectItem(canvas, text, rect, color));
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawRectItem_002
 * @tc.desc: Test DrawRectItem
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawRectItem_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->labelsCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->labelsCanvasNode_ = Rosen::RSCanvasNode::Create();
    }
    auto canvas = static_cast<TouchDrawingManager::RosenCanvas *>
        (TOUCH_DRAWING_MGR->labelsCanvasNode_->BeginRecording(TOUCH_DRAWING_MGR->displayInfo_.width,
        TOUCH_DRAWING_MGR->displayInfo_.height));
    ASSERT_NE(canvas, nullptr);
    std::string text = "test";
    Rosen::Drawing::Rect rect { 1, 1, 10, 10 };
    Rosen::Drawing::Color color = Rosen::Drawing::Color::ColorQuadSetARGB(192, 255, 255, 255);
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DrawRectItem(canvas, text, rect, color));
    TOUCH_DRAWING_MGR->labelsCanvasNode_->FinishRecording();
    Rosen::RSTransaction::FlushImplicitTransaction();
}

/**
 * @tc.name: TouchDrawingManagerTest_CreateTouchWindow_002
 * @tc.desc: Test CreateTouchWindow
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_CreateTouchWindow_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "touch window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    TOUCH_DRAWING_MGR->surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->CreateTouchWindow());
}

/**
 * @tc.name: TouchDrawingManagerTest_DestoryTouchWindow_001
 * @tc.desc: Test DestoryTouchWindow
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DestoryTouchWindow_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = true;
    TOUCH_DRAWING_MGR->pointerMode_.isShow = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DestoryTouchWindow());
}

/**
 * @tc.name: TouchDrawingManagerTest_DestoryTouchWindow_002
 * @tc.desc: Test DestoryTouchWindow
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DestoryTouchWindow_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = false;
    TOUCH_DRAWING_MGR->pointerMode_.isShow = false;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DestoryTouchWindow());
}

/**
 * @tc.name: TouchDrawingManagerTest_DestoryTouchWindow_003
 * @tc.desc: Test DestoryTouchWindow
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DestoryTouchWindow_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = false;
    TOUCH_DRAWING_MGR->pointerMode_.isShow = false;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "touch window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    TOUCH_DRAWING_MGR->surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->DestoryTouchWindow());
}
/**
 * @tc.name: TouchDrawingManagerTest_UpdateLastPointerItem_001
 * @tc.desc: Test UpdateLastPointerItem
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdateLastPointerItem_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerEvent::PointerItem item;
    item.SetPressed(false);
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->UpdateLastPointerItem(item));
}

/**
 * @tc.name: TouchDrawingManagerTest_UpdateLastPointerItem_002
 * @tc.desc: Test UpdateLastPointerItem
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdateLastPointerItem_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerEvent::PointerItem item;
    item.SetPressed(true);
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->UpdateLastPointerItem(item));
}

/**
 * @tc.name: TouchDrawingManagerTest_UpdateBubbleData_001
 * @tc.desc: Test UpdateBubbleData
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdateBubbleData_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->UpdateBubbleData());
}

/**
 * @tc.name: TouchDrawingManagerTest_UpdateBubbleData_002
 * @tc.desc: Test UpdateBubbleData
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdateBubbleData_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = false;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->UpdateBubbleData());
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationScreen_001
 * @tc.desc: Test RotationScreen
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationScreen_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->isChangedRotation_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION0;
    TOUCH_DRAWING_MGR->pointerMode_.isShow = true;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationScreen());
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationScreen_002
 * @tc.desc: Test RotationScreen
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationScreen_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->isChangedRotation_ = false;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION0;
    TOUCH_DRAWING_MGR->pointerMode_.isShow = true;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationScreen());
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationScreen_003
 * @tc.desc: Test RotationScreen
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationScreen_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->isChangedRotation_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION90;
    TOUCH_DRAWING_MGR->pointerMode_.isShow = true;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationScreen());
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationScreen_004
 * @tc.desc: Test RotationScreen
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationScreen_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->isChangedRotation_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION0;
    TOUCH_DRAWING_MGR->pointerMode_.isShow = false;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationScreen());
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationScreen_005
 * @tc.desc: Test RotationScreen
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationScreen_005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->isChangedRotation_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION0;
    TOUCH_DRAWING_MGR->pointerMode_.isShow = true;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = false;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationScreen());
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationScreen_006
 * @tc.desc: Test RotationScreen
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationScreen_006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->isChangedRotation_ = false;
    TOUCH_DRAWING_MGR->isChangedMode_ = true;
    TOUCH_DRAWING_MGR->pointerMode_.isShow = true;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationScreen());
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationScreen_007
 * @tc.desc: Test RotationScreen
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationScreen_007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->isChangedRotation_ = false;
    TOUCH_DRAWING_MGR->isChangedMode_ = true;
    TOUCH_DRAWING_MGR->pointerMode_.isShow = false;
    TOUCH_DRAWING_MGR->bubbleMode_.isShow = false;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationScreen());
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationScreen_008
 * @tc.desc: Test RotationScreen
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationScreen_008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->isChangedRotation_ = false;
    TOUCH_DRAWING_MGR->isChangedMode_ = false;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationScreen());
}

/**
 * @tc.name: TouchDrawingManagerTest_AddCanvasNode_001
 * @tc.desc: Test AddCanvasNode
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_AddCanvasNode_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "touch window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    TOUCH_DRAWING_MGR->surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);

    std::shared_ptr<Rosen::RSCanvasNode> canvasNode = nullptr;
    bool isTrackerNode = true;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->AddCanvasNode(canvasNode, isTrackerNode));
}

/**
 * @tc.name: TouchDrawingManagerTest_AddCanvasNode_002
 * @tc.desc: Test AddCanvasNode
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_AddCanvasNode_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "touch window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    TOUCH_DRAWING_MGR->surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);

    std::shared_ptr<Rosen::RSCanvasNode> canvasNode = nullptr;
    bool isTrackerNode = false;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->AddCanvasNode(canvasNode, isTrackerNode));
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationCanvasNode_001
 * @tc.desc: Test RotationCanvasNode
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationCanvasNode_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<Rosen::RSCanvasNode> canvasNode = Rosen::RSCanvasNode::Create();
    ASSERT_NE(canvasNode, nullptr);
    TOUCH_DRAWING_MGR->displayInfo_.direction = Direction::DIRECTION90;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationCanvasNode(canvasNode));
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationCanvasNode_002
 * @tc.desc: Test RotationCanvasNode
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationCanvasNode_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<Rosen::RSCanvasNode> canvasNode = Rosen::RSCanvasNode::Create();
    ASSERT_NE(canvasNode, nullptr);
    TOUCH_DRAWING_MGR->displayInfo_.direction = Direction::DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationCanvasNode(canvasNode));
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationCanvasNode_003
 * @tc.desc: Test RotationCanvasNode
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationCanvasNode_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<Rosen::RSCanvasNode> canvasNode = Rosen::RSCanvasNode::Create();
    ASSERT_NE(canvasNode, nullptr);
    TOUCH_DRAWING_MGR->displayInfo_.direction = Direction::DIRECTION180;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationCanvasNode(canvasNode));
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationCanvasNode_004
 * @tc.desc: Test RotationCanvasNode
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationCanvasNode_004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<Rosen::RSCanvasNode> canvasNode = Rosen::RSCanvasNode::Create();
    ASSERT_NE(canvasNode, nullptr);
    TOUCH_DRAWING_MGR->displayInfo_.direction = Direction::DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RotationCanvasNode(canvasNode));
}

/**
 * @tc.name: TouchDrawingManagerTest_RemovePointerPosition_001
 * @tc.desc: Test RemovePointerPosition
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RemovePointerPosition_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "touch window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    TOUCH_DRAWING_MGR->surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->RemovePointerPosition());
}

/**
 * @tc.name: TouchDrawingManagerTest_Snapshot_001
 * @tc.desc: Test Snapshot
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_Snapshot_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->labelsCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->labelsCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    }
    TOUCH_DRAWING_MGR->isChangedRotation_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.direction = DIRECTION90;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->Snapshot());
}

/**
 * @tc.name: TouchDrawingManagerTest_Snapshot_002
 * @tc.desc: Test Snapshot
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_Snapshot_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->labelsCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->labelsCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    }
    TOUCH_DRAWING_MGR->isChangedRotation_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.direction = DIRECTION180;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->Snapshot());
}

/**
 * @tc.name: TouchDrawingManagerTest_Snapshot_003
 * @tc.desc: Test Snapshot
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_Snapshot_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (TOUCH_DRAWING_MGR->labelsCanvasNode_ == nullptr) {
        TOUCH_DRAWING_MGR->labelsCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    }
    TOUCH_DRAWING_MGR->isChangedRotation_ = true;
    TOUCH_DRAWING_MGR->displayInfo_.direction = DIRECTION270;
    TOUCH_DRAWING_MGR->displayInfo_.displayDirection = DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(TOUCH_DRAWING_MGR->Snapshot());
}

/**
 * @tc.name: TouchDrawingManagerTest_InitLabels_001
 * @tc.desc: Test InitLabels
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_InitLabels_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TOUCH_DRAWING_MGR->InitLabels();
    EXPECT_EQ(TOUCH_DRAWING_MGR->isFirstDownAction_, true);
    EXPECT_EQ(TOUCH_DRAWING_MGR->isDownAction_, true);
    EXPECT_EQ(TOUCH_DRAWING_MGR->maxPointerCount_, 0);
}

/**
 * @tc.name: TouchDrawingManagerTest_ResetCanvasNode_001
 * @tc.desc: Test ResetCanvasNode
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_ResetCanvasNode_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto canvasNode = Rosen::RSCanvasDrawingNode::Create();
    TOUCH_DRAWING_MGR->ResetCanvasNode(canvasNode);
}

/**
 * @tc.name: TouchDrawingManagerTest_AddCanvasNode
 * @tc.desc: Test AddCanvasNode
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_AddCanvasNode, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TouchDrawingManager touchDrawingMgr;
    Rosen::RSSurfaceNodeConfig surfaceNodeConfig;
    surfaceNodeConfig.SurfaceNodeName = "touch window";
    Rosen::RSSurfaceNodeType surfaceNodeType = Rosen::RSSurfaceNodeType::SELF_DRAWING_WINDOW_NODE;
    touchDrawingMgr.surfaceNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, surfaceNodeType);
    std::shared_ptr<Rosen::RSCanvasNode> canvasNode = Rosen::RSCanvasNode::Create();
    ASSERT_NE(canvasNode, nullptr);
    bool isTrackerNode = true;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.AddCanvasNode(canvasNode, isTrackerNode));

    canvasNode = nullptr;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.AddCanvasNode(canvasNode, isTrackerNode));
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationCanvasNode
 * @tc.desc: Test RotationCanvasNode
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationCanvasNode, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TouchDrawingManager touchDrawingMgr;
    std::shared_ptr<Rosen::RSCanvasNode> canvasNode = Rosen::RSCanvasNode::Create();
    ASSERT_NE(canvasNode, nullptr);
    touchDrawingMgr.displayInfo_.direction = Direction::DIRECTION90;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.RotationCanvasNode(canvasNode));
    touchDrawingMgr.displayInfo_.direction = Direction::DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.RotationCanvasNode(canvasNode));
    touchDrawingMgr.displayInfo_.direction = Direction::DIRECTION180;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.RotationCanvasNode(canvasNode));
    touchDrawingMgr.displayInfo_.direction = Direction::DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.RotationCanvasNode(canvasNode));
}

/**
 * @tc.name: TouchDrawingManagerTest_UpdateDisplayInfo
 * @tc.desc: Test UpdateDisplayInfo
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_UpdateDisplayInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TouchDrawingManager touchDrawingMgr;
    DisplayInfo displayInfo;
    displayInfo.direction = Direction::DIRECTION0;
    touchDrawingMgr.displayInfo_.direction = Direction::DIRECTION0;
    displayInfo.width = 700;
    displayInfo.height = 500;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.UpdateDisplayInfo(displayInfo));

    displayInfo.direction = Direction::DIRECTION180;
    touchDrawingMgr.displayInfo_.direction = Direction::DIRECTION180;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.UpdateDisplayInfo(displayInfo));

    displayInfo.direction = Direction::DIRECTION270;
    touchDrawingMgr.displayInfo_.direction = Direction::DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.UpdateDisplayInfo(displayInfo));
}

/**
 * @tc.name: TouchDrawingManagerTest_RotationCanvas
 * @tc.desc: Test RotationCanvas
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_RotationCanvas, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TouchDrawingManager touchDrawingMgr;
    int32_t width = 300;
    int32_t height = 100;
    Direction direction = Direction::DIRECTION90;
    touchDrawingMgr.labelsCanvasNode_ = Rosen::RSCanvasDrawingNode::Create();
    auto canvas = static_cast<TouchDrawingManager::RosenCanvas *>
        (touchDrawingMgr.labelsCanvasNode_->BeginRecording(width, height));
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.RotationCanvas(canvas, direction));
    direction = Direction::DIRECTION180;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.RotationCanvas(canvas, direction));
    direction = Direction::DIRECTION270;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.RotationCanvas(canvas, direction));
    direction = Direction::DIRECTION0;
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.RotationCanvas(canvas, direction));
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawBubble
 * @tc.desc: Test DrawBubble
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawBubble, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TouchDrawingManager touchDrawingMgr;
    touchDrawingMgr.bubbleCanvasNode_ = Rosen::RSCanvasNode::Create();
    touchDrawingMgr.pointerEvent_ = PointerEvent::Create();
    ASSERT_NE(touchDrawingMgr.pointerEvent_, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    touchDrawingMgr.pointerEvent_->SetPointerId(1);
    touchDrawingMgr.pointerEvent_->AddPointerItem(item);
    touchDrawingMgr.pointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_UP);
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.DrawBubble());
    touchDrawingMgr.pointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_PULL_UP);
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.DrawBubble());
    touchDrawingMgr.pointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_CANCEL);
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.DrawBubble());
}

/**
 * @tc.name: TouchDrawingManagerTest_DrawBubble_003
 * @tc.desc: Test DrawBubble
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_DrawBubble_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TouchDrawingManager touchDrawingMgr;
    touchDrawingMgr.bubbleCanvasNode_ = Rosen::RSCanvasNode::Create();
    touchDrawingMgr.pointerEvent_ = PointerEvent::Create();
    ASSERT_NE(touchDrawingMgr.pointerEvent_, nullptr);
    PointerEvent::PointerItem item;
    item.SetPointerId(1);
    touchDrawingMgr.pointerEvent_->AddPointerItem(item);
    touchDrawingMgr.pointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_DOWN);
    item.SetPointerId(2);
    touchDrawingMgr.pointerEvent_->SetPointerId(2);
    touchDrawingMgr.pointerEvent_->AddPointerItem(item);
    EXPECT_NO_FATAL_FAILURE(touchDrawingMgr.DrawBubble());
}

/**
 * @tc.name: TouchDrawingManagerTest_CalcDrawCoordinate_001
 * @tc.desc: Test CalcDrawCoordinate
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_CalcDrawCoordinate_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TouchDrawingManager touchDrawingMgr;
    DisplayInfo displayInfo;
    PointerEvent::PointerItem pointerItem;
    int32_t physicalX = 1;
    int32_t physicalY = 1;
    pointerItem.SetRawDisplayX(physicalX);
    pointerItem.SetRawDisplayY(physicalY);
    auto retPair = touchDrawingMgr.CalcDrawCoordinate(displayInfo, pointerItem);
    EXPECT_EQ(retPair.first, 1);
    EXPECT_EQ(retPair.second, 1);
}

/**
 * @tc.name: TouchDrawingManagerTest_CalcDrawCoordinate_002
 * @tc.desc: Test CalcDrawCoordinate
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(TouchDrawingManagerTest, TouchDrawingManagerTest_CalcDrawCoordinate_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    TouchDrawingManager touchDrawingMgr;
    DisplayInfo displayInfo = {
        .id = 0, .x = 0, .y = 0, .width = 100, .height = 200, .dpi = 240,
        .transform = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}
    };
    PointerEvent::PointerItem pointerItem;
    int32_t physicalX = 10;
    int32_t physicalY = 10;
    pointerItem.SetRawDisplayX(physicalX);
    pointerItem.SetRawDisplayY(physicalY);
    auto retPair = touchDrawingMgr.CalcDrawCoordinate(displayInfo, pointerItem);
    EXPECT_EQ(retPair.first, 0);
    EXPECT_EQ(retPair.second, 0);
}
} // namespace MMI
} // namespace OHOS
