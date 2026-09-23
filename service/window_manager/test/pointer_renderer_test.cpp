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

#include "config_multimodal.h"
#include "image_source.h"
#include "mmi_log.h"
#include "pointer_renderer.h"
#include "resource_decompress.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "PointerRendererTest"
constexpr int32_t MIN_POINTER_COLOR{0x000000};
constexpr int32_t MAX_POINTER_COLOR{0xFFFFFF};
constexpr int32_t OTHER_POINTER_COLOR{0x171717};
constexpr int32_t MIDDLE_POINTER_COLOR{0x7F7F7F};
constexpr uint32_t MAX_POINTER_SIZE{7};
constexpr float MAX_DPI{1.93f};
// The soft cursor buffer stays at 512; the draw helpers now take the buffer size explicitly.
constexpr uint32_t SOFT_CURSOR_SIZE{512};
// The hard cursor buffer side after TASK-5; kept here so the guard cases and the clamp cases agree.
constexpr uint32_t HARD_CURSOR_SIZE{256};

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
} // namespace
class PointerRendererTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        DecompressToDisk(DEF_MOUSE_ICONS_DAT_PATH, "/data/service/el1/public/multimodalinput/mouse_icon/");
    };
    static void TearDownTestCase(void) {};
    void SetUp(void) {};
};

namespace {
constexpr uint32_t FOCUS_POINT_OLD{256};
constexpr float CALCULATE_IMAGE_MIDDLE_OLD{2.0f};
constexpr float CALCULATE_MOUSE_ICON_BIAS_OLD{5.0f / 33.0f};

// Replicates the pre-change RenderConfig::GetOffsetX() so the focus identity can be asserted
// against a reference value that does not depend on the production code under change.
int32_t LegacyOffsetX(const RenderConfig &cfg)
{
    int32_t width = cfg.GetImageSize();
    switch (cfg.align_) {
        case ANGLE_E:
            return FOCUS_POINT_OLD;
        case ANGLE_S:
        case ANGLE_N:
        case ANGLE_CENTER:
            return static_cast<int32_t>(FOCUS_POINT_OLD - width / CALCULATE_IMAGE_MIDDLE_OLD);
        case ANGLE_W:
        case ANGLE_SE:
        case ANGLE_NE:
            return FOCUS_POINT_OLD - width;
        case ANGLE_SW:
            return FOCUS_POINT_OLD;
        case ANGLE_NW:
            return FOCUS_POINT_OLD - cfg.userIconHotSpotX;
        case ANGLE_NW_RIGHT:
            return FOCUS_POINT_OLD - width * CALCULATE_MOUSE_ICON_BIAS_OLD;
        default:
            return FOCUS_POINT_OLD;
    }
}

// Replicates the pre-change RenderConfig::GetOffsetY().
int32_t LegacyOffsetY(const RenderConfig &cfg)
{
    int32_t height = cfg.GetImageSize();
    switch (cfg.align_) {
        case ANGLE_E:
            return FOCUS_POINT_OLD - height / CALCULATE_IMAGE_MIDDLE_OLD;
        case ANGLE_S:
            return FOCUS_POINT_OLD;
        case ANGLE_W:
        case ANGLE_N:
        case ANGLE_SE:
            return FOCUS_POINT_OLD - height;
        case ANGLE_NE:
            return FOCUS_POINT_OLD;
        case ANGLE_SW:
            return FOCUS_POINT_OLD - height;
        case ANGLE_NW:
            return FOCUS_POINT_OLD - cfg.userIconHotSpotY;
        case ANGLE_CENTER:
            return FOCUS_POINT_OLD - height / CALCULATE_IMAGE_MIDDLE_OLD;
        case ANGLE_NW_RIGHT:
            return FOCUS_POINT_OLD;
        default:
            return FOCUS_POINT_OLD;
    }
}
} // namespace

/**
 * @tc.name: PointerRendererTest_GetFocus_001
 * @tc.desc: Test GetFocusX/GetFocusY satisfy the identity GetFocus == FOCUS_POINT_old - GetOffset_old
 *           for every live align across every pointer size (INV-2)
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_GetFocus_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    const ICON_TYPE liveAligns[] = {
        ICON_TYPE::ANGLE_CENTER, ICON_TYPE::ANGLE_NW, ICON_TYPE::ANGLE_SW, ICON_TYPE::ANGLE_NW_RIGHT};
    for (auto align : liveAligns) {
        for (uint32_t size = 1; size <= MAX_POINTER_SIZE; ++size) {
            RenderConfig config;
            config.align_ = align;
            config.size = size;
            config.dpi = MAX_DPI;
            config.userIconHotSpotX = 3;
            config.userIconHotSpotY = 5;
            ASSERT_GT(config.GetImageSize(), 0);
            EXPECT_EQ(static_cast<int32_t>(config.GetFocusX()),
                static_cast<int32_t>(FOCUS_POINT_OLD) - LegacyOffsetX(config))
                << "align=" << static_cast<int32_t>(align) << ", size=" << size
                << ", imageSize=" << config.GetImageSize();
            EXPECT_EQ(static_cast<int32_t>(config.GetFocusY()),
                static_cast<int32_t>(FOCUS_POINT_OLD) - LegacyOffsetY(config))
                << "align=" << static_cast<int32_t>(align) << ", size=" << size
                << ", imageSize=" << config.GetImageSize();
        }
    }
}

/**
 * @tc.name: PointerRendererTest_GetFocus_002
 * @tc.desc: Test GetFocusX/GetFocusY track imageSize instead of returning a fixed constant,
 *           so a hardcoded implementation cannot pass (INV-2)
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_GetFocus_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    // ANGLE_CENTER focus is w/2, so it must strictly grow with imageSize.
    RenderConfig small;
    small.align_ = ICON_TYPE::ANGLE_CENTER;
    small.size = 1;
    small.dpi = MAX_DPI;
    RenderConfig large;
    large.align_ = ICON_TYPE::ANGLE_CENTER;
    large.size = MAX_POINTER_SIZE;
    large.dpi = MAX_DPI;
    ASSERT_GT(large.GetImageSize(), small.GetImageSize());
    EXPECT_GT(large.GetFocusX(), small.GetFocusX());
    EXPECT_GT(large.GetFocusY(), small.GetFocusY());

    // ANGLE_NW focus is the user icon hotspot, independent of imageSize.
    RenderConfig nw;
    nw.align_ = ICON_TYPE::ANGLE_NW;
    nw.size = MAX_POINTER_SIZE;
    nw.dpi = MAX_DPI;
    nw.userIconHotSpotX = 7;
    nw.userIconHotSpotY = 11;
    EXPECT_EQ(nw.GetFocusX(), 7u);
    EXPECT_EQ(nw.GetFocusY(), 11u);

    // ANGLE_SW focus is (0, h): X pinned to the origin, Y tracking imageSize.
    RenderConfig sw;
    sw.align_ = ICON_TYPE::ANGLE_SW;
    sw.size = MAX_POINTER_SIZE;
    sw.dpi = MAX_DPI;
    EXPECT_EQ(sw.GetFocusX(), 0u);
    EXPECT_EQ(static_cast<int32_t>(sw.GetFocusY()), sw.GetImageSize());
}

/**
 * @tc.name: PointerRendererTest_ScaleToFit_001
 * @tc.desc: Test the only imageSize leak channel (gear 1, dpi > 6.4) is clamped to <= side-1 (INV-6/AC-4)
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_ScaleToFit_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    // Gear 1 has a zero suppression exponent, so imageSize = 40 * dpi grows without bound.
    // ImageSizeUpperBound_002 pins that leak; here we assert the clamp closes it.
    const float leakDpis[] = {6.4f, 8.0f, 12.0f};
    for (auto dpi : leakDpis) {
        RenderConfig config;
        config.align_ = ICON_TYPE::ANGLE_CENTER;
        config.size = 1;
        config.dpi = dpi;
        ASSERT_GE(config.GetImageSize(), static_cast<int32_t>(HARD_CURSOR_SIZE))
            << "dpi=" << dpi << " should be a genuine overflow case";
        int32_t clamped = config.GetClampedImageSize(HARD_CURSOR_SIZE);
        EXPECT_LE(clamped, static_cast<int32_t>(HARD_CURSOR_SIZE) - 1)
            << "dpi=" << dpi << ", raw=" << config.GetImageSize() << ", clamped=" << clamped;
        EXPECT_GT(clamped, 0) << "dpi=" << dpi;
    }
}

/**
 * @tc.name: PointerRendererTest_ScaleToFit_002
 * @tc.desc: Test in-bound imageSize is returned unchanged, so the clamp is not a blanket shrink (INV-6)
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_ScaleToFit_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    // Every live gear at MAX_DPI is already < 256 (ImageSizeUpperBound_001), so the clamp must be a no-op.
    for (uint32_t gear = 1; gear <= MAX_POINTER_SIZE; ++gear) {
        RenderConfig config;
        config.align_ = ICON_TYPE::ANGLE_CENTER;
        config.size = gear;
        config.dpi = MAX_DPI;
        int32_t raw = config.GetImageSize();
        ASSERT_LT(raw, static_cast<int32_t>(HARD_CURSOR_SIZE)) << "gear=" << gear;
        EXPECT_EQ(config.GetClampedImageSize(HARD_CURSOR_SIZE), raw) << "gear=" << gear;
    }
    // The soft-cursor buffer is larger, so a value that overflows 256 may still fit 512 untouched.
    RenderConfig soft;
    soft.align_ = ICON_TYPE::ANGLE_CENTER;
    soft.size = 1;
    soft.dpi = 8.0f;
    ASSERT_EQ(soft.GetImageSize(), 320);
    EXPECT_EQ(soft.GetClampedImageSize(SOFT_CURSOR_SIZE), 320);
    EXPECT_LE(soft.GetClampedImageSize(HARD_CURSOR_SIZE), static_cast<int32_t>(HARD_CURSOR_SIZE) - 1);
}

/**
 * @tc.name: PointerRendererTest_UserIconScale_001
 * @tc.desc: Test UserIconScale
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_UserIconScale_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    uint32_t width = 100;
    uint32_t height = 200;
    config.userIconFollowSystem = true;
    PointerRenderer renderer;
    image_ptr_t ret = renderer.UserIconScale(width, height, config);
    EXPECT_EQ(ret, nullptr);
    config.userIconFollowSystem = false;
    ret = renderer.UserIconScale(width, height, config);
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name: PointerRendererTest_Render_001
 * @tc.desc: Test Render
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_Render_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    uint32_t width = 10;
    uint32_t height = 20;
    uint8_t addr[800] = {10};
    config.style_ = MOUSE_ICON::TRANSPARENT_ICON;
    int32_t ret = renderer.Render(addr, width, height, config);
    EXPECT_EQ(ret, RET_OK);
    config.style_ = MOUSE_ICON::AECH_DEVELOPER_DEFINED_ICON;
    ret = renderer.Render(addr, width, height, config);
    config.direction = 5;
    EXPECT_EQ(ret, RET_ERR);
    config.direction = 0;
    config.style_ = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    ret = renderer.Render(addr, width, height, config);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: PointerRendererTest_Render_002
 * @tc.desc: Test Render
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_Render_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    uint32_t width = 10;
    uint32_t height = 20;
    uint8_t addr[800] = {10};
    config.style_ = MOUSE_ICON::AECH_DEVELOPER_DEFINED_ICON;

    std::string imagePath = "/data/service/el1/public/multimodalinput/mouse_icon/Default.svg";
    OHOS::Media::SourceOptions opts;
    uint32_t ret = 0;
    auto imageSource = OHOS::Media::ImageSource::CreateImageSource(imagePath, opts, ret);
    ASSERT_NE(imageSource, nullptr);
    std::set<std::string> formats;
    ret = imageSource->GetSupportedFormats(formats);
    OHOS::Media::DecodeOptions decodeOpts;
    decodeOpts.desiredSize = {
        .width = 80,
        .height = 80
    };
    decodeOpts.SVGOpts.fillColor = {.isValidColor = false, .color = 0xff00ff};
    decodeOpts.SVGOpts.strokeColor = {.isValidColor = false, .color = 0xff00ff};
    config.userIconPixelMap = imageSource->CreatePixelMap(decodeOpts, ret);
    ASSERT_NE(config.userIconPixelMap, nullptr);
    ret = renderer.Render(addr, width, height, config);
    config.direction = 5;
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: PointerRendererTest_Render_003
 * @tc.desc: Test Render
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_Render_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    uint32_t width = 10;
    uint32_t height = 20;
    uint8_t addr[800] = {10};
    config.style_ = MOUSE_ICON::DEFAULT;
    config.isHard = true;
    config.isBlur = true;
    int32_t ret = renderer.Render(addr, width, height, config);
    EXPECT_EQ(ret, RET_OK);
    config.isBlur = false;
    ret = renderer.Render(addr, width, height, config);
    EXPECT_EQ(ret, RET_ERR);
    config.isHard = false;
    ret = renderer.Render(addr, width, height, config);
    EXPECT_EQ(ret, RET_ERR);
    config.isBlur = true;
    ret = renderer.Render(addr, width, height, config);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: PointerRendererTest_BlurRender_001
 * @tc.desc: Test BlurRender
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_BlurRender_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    uint32_t width = 10;
    uint32_t height = 20;
    uint8_t addr[800] = {10};
    constexpr uint32_t renderStride = 4;
    uint32_t addrSize = width * height * renderStride;
    config.style_ = MOUSE_ICON::DEFAULT;
    config.isBlur = false;
    int32_t ret = renderer.BlurRender(addr, addrSize, width, height, config);
    EXPECT_EQ(ret, RET_ERR);
    config.isBlur = true;
    renderer.defaultInit_ = false;
    ret = renderer.BlurRender(addr, addrSize, width, height, config);
    EXPECT_EQ(renderer.defaultInit_, true);
    EXPECT_EQ(ret, RET_OK);
    renderer.defaultInit_ = true;
    ret = renderer.BlurRender(addr, addrSize, width, height, config);
    EXPECT_EQ(renderer.defaultInit_, true);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: PointerRendererTest_BlurRender_002
 * @tc.desc: Test BlurRender
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_BlurRender_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    uint32_t width = 10;
    uint32_t height = 20;
    uint8_t addr[800] = {10};
    config.isBlur = true;
    constexpr uint32_t renderStride = 4;
    uint32_t addrSize = width * height * renderStride;
    config.style_ = MOUSE_ICON::TRANSPARENT_ICON;
    int32_t ret = renderer.BlurRender(addr, addrSize, width, height, config);
    EXPECT_EQ(ret, RET_OK);
    config.style_ = MOUSE_ICON::AECH_DEVELOPER_DEFINED_ICON;
    ret = renderer.BlurRender(addr, addrSize, width, height, config);
    config.direction = 5;
    EXPECT_EQ(ret, RET_OK);
    config.direction = 0;
    config.style_ = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    ret = renderer.BlurRender(addr, addrSize, width, height, config);
    EXPECT_EQ(ret, RET_OK);
    width = 0;
    height = 0;
    config.direction = 0;
    addrSize = width * height * renderStride;
    config.style_ = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    ret = renderer.BlurRender(addr, addrSize, width, height, config);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: PointerRendererTest_DrawDefaultPointer_001
 * @tc.desc: Test DrawDefaultPointer
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_DrawDefaultPointer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    config.screenId = 0;
    renderer.screenImages_ = {
        {0, {nullptr, nullptr, nullptr, nullptr}}
    };
    renderer.DrawDefaultPointer(SOFT_CURSOR_SIZE, SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(renderer.screenImages_[0][0], nullptr);
    renderer.screenImages_ = {
        {0, {nullptr, nullptr, nullptr}}
    };
    renderer.DrawDefaultPointer(SOFT_CURSOR_SIZE, SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(renderer.screenImages_[0][0], nullptr);
    image_ptr_t img = std::make_shared<OHOS::Rosen::Drawing::Image>();
    renderer.screenImages_ = {
        {0, {img, nullptr, nullptr, nullptr}}
    };
    renderer.DrawDefaultPointer(SOFT_CURSOR_SIZE, SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(renderer.screenImages_[0][0], img);
}

/**
 * @tc.name: PointerRendererTest_DrawBlurPointer_001
 * @tc.desc: Test DrawBlurPointer
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_DrawBlurPointer_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    uint32_t width = 10;
    uint32_t height = 20;
    RenderConfig lastConfig;
    config.screenId = 0;
    renderer.screenImages_ = {
        {0, {nullptr, nullptr, nullptr, nullptr}}
    };
    renderer.DrawBlurPointer(width, height, lastConfig, config);
    EXPECT_EQ(renderer.screenImages_[0][0], nullptr);
    renderer.screenImages_ = {
        {0, {nullptr, nullptr, nullptr}}
    };
    renderer.DrawBlurPointer(width, height, lastConfig, config);
    EXPECT_EQ(renderer.screenImages_[0][0], nullptr);
    image_ptr_t img = std::make_shared<OHOS::Rosen::Drawing::Image>();
    renderer.screenImages_ = {
        {0, {img, img, img, img}}
    };
    renderer.DrawBlurPointer(width, height, lastConfig, config);
    EXPECT_EQ(renderer.screenImages_[0][0], img);
}

/**
 * @tc.name: PointerRendererTest_AdjustDeltaForDirection_001
 * @tc.desc: Test AdjustDeltaForDirection
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_AdjustDeltaForDirection_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    int32_t dx = 10;
    int32_t dy = 5;
    uint32_t direction = 0;
    uint32_t displayDirection = 0;
    renderer.AdjustDeltaForDirection(dx, dy, direction, displayDirection);
    EXPECT_EQ(dx, 10);
    EXPECT_EQ(dy, 5);
    direction = 1;
    renderer.AdjustDeltaForDirection(dx, dy, direction, displayDirection);
    EXPECT_EQ(dx, -5);
    EXPECT_EQ(dy, 10);
    direction = 2;
    dx = 10;
    dy = 5;
    renderer.AdjustDeltaForDirection(dx, dy, direction, displayDirection);
    EXPECT_EQ(dx, -10);
    EXPECT_EQ(dy, -5);
    direction = 3;
    dx = 10;
    dy = 5;
    renderer.AdjustDeltaForDirection(dx, dy, direction, displayDirection);
    EXPECT_EQ(dx, 5);
    EXPECT_EQ(dy, -10);
}

/**
 * @tc.name: PointerRendererTest_IsPositionOutCanvas_001
 * @tc.desc: Test IsPositionOutCanvas
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_IsPositionOutCanvas_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    int32_t imageSize = 20;
    uint32_t width = 200;
    uint32_t height = 200;
    int32_t x = 100;
    int32_t y = 100;
    bool ret = renderer.IsPositionOutCanvas(x, y, imageSize, width, height);
    EXPECT_EQ(ret, false);
    x = -100;
    ret = renderer.IsPositionOutCanvas(x, y, imageSize, width, height);
    EXPECT_EQ(ret, true);
    y = -100;
    ret = renderer.IsPositionOutCanvas(x, y, imageSize, width, height);
    EXPECT_EQ(ret, true);
    x = 100;
    ret = renderer.IsPositionOutCanvas(x, y, imageSize, width, height);
    EXPECT_EQ(ret, true);
    y = 300;
    ret = renderer.IsPositionOutCanvas(x, y, imageSize, width, height);
    EXPECT_EQ(ret, true);
    x = 300;
    ret = renderer.IsPositionOutCanvas(x, y, imageSize, width, height);
    EXPECT_EQ(ret, true);
    y = 100;
    ret = renderer.IsPositionOutCanvas(x, y, imageSize, width, height);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: PointerRendererTest_HasPointerCfg_001
 * @tc.desc: Test HasPointerCfg
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_HasPointerCfg_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    RenderConfig config1;
    PointerRenderer renderer;
    config.style_ = MOUSE_ICON::DEFAULT;
    config1.style_ = MOUSE_ICON::DEFAULT;
    config.screenId = 0;
    config1.screenId = 1;
    bool ret = renderer.HasPointerCfg(config);
    EXPECT_EQ(ret, false);
    renderer.screenConfigs_ = {
        {0, config}
    };
    ret = renderer.HasPointerCfg(config);
    EXPECT_EQ(ret, true);
    config1.style_ = MOUSE_ICON::EAST;
    renderer.screenConfigs_ = {
        {0, config1}
    };
    ret = renderer.HasPointerCfg(config);
    EXPECT_EQ(ret, false);
    config1.style_ = MOUSE_ICON::DEFAULT;
    config1.isBlur = true;
    renderer.screenConfigs_ = {
        {0, config1}
    };
    ret = renderer.HasPointerCfg(config);
    EXPECT_EQ(ret, false);
    config1.isHard = true;
    config1.isBlur = false;
    renderer.screenConfigs_ = {
        {0, config1}
    };
    ret = renderer.HasPointerCfg(config);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: PointerRendererTest_SetPointerCfg_001
 * @tc.desc: Test SetPointerCfg
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_SetPointerCfg_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    config.screenId = 0;
    renderer.SetPointerCfg(config);
    EXPECT_EQ(renderer.screenConfigs_[0], config);
}

/**
 * @tc.name: PointerRendererTest_GetPointerCfg_001
 * @tc.desc: Test GetPointerCfg
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_GetPointerCfg_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    RenderConfig defaultConfig;
    PointerRenderer renderer;
    config.isHard = true;
    defaultConfig.isHard = true;
    config.screenId = 1;
    defaultConfig.screenId = 0;
    auto cfg = renderer.GetPointerCfg(defaultConfig);
    EXPECT_EQ(cfg.screenId, defaultConfig.screenId);
    renderer.screenConfigs_ = {
        {0, config}
    };
    auto cfg1 = renderer.GetPointerCfg(defaultConfig);
    EXPECT_EQ(cfg1.screenId, config.screenId);
}

/**
 * @tc.name: PointerRendererTest_GetPointerImage_001
 * @tc.desc: Test GetPointerImage
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_GetPointerImage_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    config.screenId = 0;
    auto img = renderer.GetPointerImage(config);
    EXPECT_EQ(img.size(), 0);
    renderer.screenImages_ = {
        {0, {nullptr}}
    };
    img = renderer.GetPointerImage(config);
    EXPECT_EQ(img.size(), 1);
}

/**
 * @tc.name: PointerRendererTest_LoadDefaultPointerImage_001
 * @tc.desc: Test LoadDefaultPointerImage
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_LoadDefaultPointerImage_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    config.screenId = 1;
    renderer.screenImages_ = {
        {0, {nullptr, nullptr, nullptr, nullptr}}
    };
    renderer.LoadDefaultPointerImage(SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(renderer.screenImages_[0][0], nullptr);
}

/**
 * @tc.name: PointerRendererTest_ApplyAlpha_001
 * @tc.desc: Test ApplyAlpha
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_ApplyAlpha_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    uint8_t pixel[4] = {255, 255, 255, 255};
    float pecent = 0.5f;
    renderer.ApplyAlpha(pixel, 4, true, pecent);
    EXPECT_EQ(pixel[3], 255);
    EXPECT_EQ(pixel[0], static_cast<uint8_t>(255 * pecent));
    renderer.ApplyAlpha(pixel, 4, false, pecent);
    EXPECT_EQ(pixel[3], 255);
    uint8_t* pixel1 = nullptr;
    ASSERT_NO_FATAL_FAILURE(renderer.ApplyAlpha(pixel1, 0, false, pecent));
}

/**
 * @tc.name: PointerRendererTest_SetAlpha_001
 * @tc.desc: Test SetAlpha
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_SetAlpha001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    pixelmap_ptr_t img = nullptr;
    float pecent = 0.5f;
    ASSERT_NO_FATAL_FAILURE(renderer.SetAlpha(img, pecent));
    img = std::make_shared<OHOS::Media::PixelMap>();
    ASSERT_NO_FATAL_FAILURE(renderer.SetAlpha(img, pecent));
}

/**
 * @tc.name: PointerRendererTest_LoadPointerToCache_001
 * @tc.desc: Test LoadPointerToCache
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_LoadPointerToCache_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    const std::string path = "/data/service/el1/public/multimodalinput/mouse_icon/";
    EXPECT_TRUE(renderer.mouseIcons_.empty());
    std::map<MOUSE_ICON, IconStyle> mouseIcons = {};
    renderer.LoadPointerToCache(mouseIcons);
    EXPECT_TRUE(renderer.mouseIcons_.empty());
    mouseIcons = {
        {DEFAULT, {ANGLE_NW, path + "Default.svg"}},
    };
    renderer.LoadPointerToCache(mouseIcons);
    EXPECT_FALSE(renderer.mouseIcons_.empty());
}

/**
 * @tc.name: PointerRendererTest_GetPointerFromCache_001
 * @tc.desc: Test GetPointerFromCache
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_GetPointerFromCache_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    RenderConfig config;
    std::string svgContent;
    config.style_ = MOUSE_ICON::DEFAULT;
    config.path_ = "/data/service/el1/public/multimodalinput/mouse_icon/Default.svg";
    EXPECT_TRUE(renderer.mouseIcons_.empty());
    bool ret = renderer.GetPointerFromCache(config, svgContent);
    EXPECT_EQ(ret, true);
    EXPECT_FALSE(renderer.mouseIcons_.empty());
    ret = renderer.GetPointerFromCache(config, svgContent);
    EXPECT_EQ(ret, true);
    config.path_ = "test";
    config.style_ = MOUSE_ICON::AECH_DEVELOPER_DEFINED_ICON;
    ret = renderer.GetPointerFromCache(config, svgContent);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: PointerRendererTest_DynamicRender_001
 * @tc.desc: Test DynamicRender
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_DynamicRender_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    uint32_t width = 10;
    uint32_t height = 20;
    uint8_t addr[800] = {10};
    config.style_ = MOUSE_ICON::TRANSPARENT_ICON;
    int32_t ret = renderer.DynamicRender(addr, width, height, config);
    EXPECT_EQ(ret, RET_OK);
    config.style_ = MOUSE_ICON::AECH_DEVELOPER_DEFINED_ICON;
    ret = renderer.DynamicRender(addr, width, height, config);
    config.direction = 5;
    EXPECT_EQ(ret, RET_OK);
    config.direction = 0;
    config.style_ = MOUSE_ICON::DEVELOPER_DEFINED_ICON;
    ret = renderer.DynamicRender(addr, width, height, config);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: PointerRendererTest_DynamicRender_002
 * @tc.desc: Test DynamicRender
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_DynamicRender_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    uint32_t width = 10;
    uint32_t height = 20;
    uint8_t addr[800] = {10};
    config.style_ = MOUSE_ICON::LOADING;
    int32_t ret = renderer.DynamicRender(addr, width, height, config);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: PointerRendererTest_ExtractDrawingImage_001
 * @tc.desc: Test ExtractDrawingImage
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_ExtractDrawingImage_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    pixelmap_ptr_t pixelMap = std::make_shared<OHOS::Media::PixelMap>();
    ASSERT_NE(pixelMap, nullptr);
    Media::ImageInfo imageInfo;
    imageInfo.size.width = 280;
    imageInfo.size.height = 280;
    imageInfo.pixelFormat = Media::PixelFormat::RGB_565;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_UNKNOWN;
    imageInfo.colorSpace = Media::ColorSpace::DISPLAY_P3;
    pixelMap->SetImageInfo(imageInfo);
    image_ptr_t ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
    imageInfo.pixelFormat = Media::PixelFormat::RGBA_8888;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    imageInfo.colorSpace = Media::ColorSpace::LINEAR_SRGB;
    pixelMap->SetImageInfo(imageInfo);
    ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
    imageInfo.pixelFormat = Media::PixelFormat::BGRA_8888;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_PREMUL;
    imageInfo.colorSpace = Media::ColorSpace::SRGB;
    pixelMap->SetImageInfo(imageInfo);
    ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
    imageInfo.pixelFormat = Media::PixelFormat::ALPHA_8;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_UNPREMUL;
    imageInfo.colorSpace = Media::ColorSpace::UNKNOWN;
    pixelMap->SetImageInfo(imageInfo);
    ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
}

/**
 * @tc.name: PointerRendererTest_ExtractDrawingImage_002
 * @tc.desc: Test ExtractDrawingImage
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_ExtractDrawingImage_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    pixelmap_ptr_t pixelMap = std::make_shared<OHOS::Media::PixelMap>();
    ASSERT_NE(pixelMap, nullptr);
    Media::ImageInfo imageInfo;
    imageInfo.size.width = 280;
    imageInfo.size.height = 280;
    imageInfo.pixelFormat = Media::PixelFormat::RGBA_F16;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_UNKNOWN;
    imageInfo.colorSpace = Media::ColorSpace::DISPLAY_P3;
    pixelMap->SetImageInfo(imageInfo);
    image_ptr_t ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
    imageInfo.pixelFormat = Media::PixelFormat::UNKNOWN;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    imageInfo.colorSpace = Media::ColorSpace::LINEAR_SRGB;
    pixelMap->SetImageInfo(imageInfo);
    ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
    imageInfo.pixelFormat = Media::PixelFormat::ARGB_8888;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_PREMUL;
    imageInfo.colorSpace = Media::ColorSpace::SRGB;
    pixelMap->SetImageInfo(imageInfo);
    ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
    imageInfo.pixelFormat = Media::PixelFormat::RGB_888;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_UNPREMUL;
    imageInfo.colorSpace = Media::ColorSpace::UNKNOWN;
    pixelMap->SetImageInfo(imageInfo);
    ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
}

/**
 * @tc.name: PointerRendererTest_ExtractDrawingImage_003
 * @tc.desc: Test ExtractDrawingImage
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_ExtractDrawingImage_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    pixelmap_ptr_t pixelMap = std::make_shared<OHOS::Media::PixelMap>();
    ASSERT_NE(pixelMap, nullptr);
    Media::ImageInfo imageInfo;
    imageInfo.size.width = 280;
    imageInfo.size.height = 280;
    imageInfo.pixelFormat = Media::PixelFormat::NV21;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_UNKNOWN;
    imageInfo.colorSpace = Media::ColorSpace::DISPLAY_P3;
    pixelMap->SetImageInfo(imageInfo);
    image_ptr_t ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
    imageInfo.pixelFormat = Media::PixelFormat::NV12;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    imageInfo.colorSpace = Media::ColorSpace::LINEAR_SRGB;
    pixelMap->SetImageInfo(imageInfo);
    ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
    imageInfo.pixelFormat = Media::PixelFormat::CMYK;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_PREMUL;
    imageInfo.colorSpace = Media::ColorSpace::SRGB;
    pixelMap->SetImageInfo(imageInfo);
    ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
    imageInfo.pixelFormat = Media::PixelFormat::RGBA_1010102;
    imageInfo.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_PREMUL;
    imageInfo.colorSpace = Media::ColorSpace::SRGB;
    pixelMap->SetImageInfo(imageInfo);
    ret = renderer.ExtractDrawingImage(pixelMap);
    ASSERT_EQ(ret, nullptr);
}

/**
 * @tc.name: PointerRendererTest_DrawImage_001
 * @tc.desc: Test DrawImage
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_DrawImage_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    OHOS::Rosen::Drawing::Canvas canvas;
    RenderConfig config;
    config.style_ = MOUSE_ICON::LOADING;
    int32_t ret = renderer.DrawImage(canvas, SOFT_CURSOR_SIZE, SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(ret, RET_ERR);
    config.style_ = MOUSE_ICON::DEFAULT;
    ret = renderer.DrawImage(canvas, SOFT_CURSOR_SIZE, SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: PointerRendererTest_DrawImage_002
 * @tc.desc: Test DrawImage
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_DrawImage_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    OHOS::Rosen::Drawing::Canvas canvas;
    RenderConfig config;
    config.style_ = MOUSE_ICON::LOADING;
    config.path_ = "/data/service/el1/public/multimodalinput/mouse_icon/Default.svg";
    int32_t ret = renderer.DrawImage(canvas, SOFT_CURSOR_SIZE, SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(ret, RET_OK);

    config.style_ = MOUSE_ICON::DEFAULT;
    ret = renderer.DrawImage(canvas, SOFT_CURSOR_SIZE, SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: PointerRendererTest_LoadCursorSvgWithColor_001
 * @tc.desc: Test LoadCursorSvgWithColor
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_LoadCursorSvgWithColor_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    pixelmap_ptr_t ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(ret, nullptr);
    config.color = 0xFFFFFF;
    ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(ret, nullptr);
    config.style_ = CURSOR_COPY;
    config.color = 0x000123;
    ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(ret, nullptr);
    config.color = 0xFFFFFF;
    ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(ret, nullptr);
    config.style_ = HAND_GRABBING;
    config.color = 0xFFFFFF;
    ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(ret, nullptr);
    config.style_ = HAND_GRABBING;
    config.color = 0x000123;
    ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name: PointerRendererTest_LoadCursorSvgWithColor_002
 * @tc.desc: Test LoadCursorSvgWithColor && ChangeSvgCursorColor
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_LoadCursorSvgWithColor_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    config.path_ = "/data/service/el1/public/multimodalinput/mouse_icon/Default.svg";
    config.style_ = MOUSE_ICON::CURSOR_COPY;
    config.color = MAX_POINTER_COLOR;
    pixelmap_ptr_t ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_NE(ret, nullptr);

    config.color = MIN_POINTER_COLOR;
    ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name: PointerRendererTest_LoadCursorSvgWithColor_003
 * @tc.desc: Test LoadCursorSvgWithColor && SetCursorColorBaseOnStyle
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_LoadCursorSvgWithColor_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    PointerRenderer renderer;
    config.path_ = "/data/service/el1/public/multimodalinput/mouse_icon/Default.svg";

    config.style_ = MOUSE_ICON::DEFAULT;
    config.color = MIN_POINTER_COLOR;
    pixelmap_ptr_t ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_NE(ret, nullptr);

    config.style_ = MOUSE_ICON::HAND_GRABBING;
    config.color = MIN_POINTER_COLOR;
    ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_NE(ret, nullptr);

    config.style_ = MOUSE_ICON::HAND_OPEN;
    config.color = MAX_POINTER_COLOR;
    ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_NE(ret, nullptr);

    config.style_ = MOUSE_ICON::HAND_POINTING;
    config.color = OTHER_POINTER_COLOR;
    ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_NE(ret, nullptr);

    config.style_ = MOUSE_ICON::HAND_POINTING;
    config.color = MIDDLE_POINTER_COLOR;
    ret = renderer.LoadCursorSvgWithColor(SOFT_CURSOR_SIZE, config);
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name: PointerRendererTest_AdjustIncreaseRatio_001
 * @tc.desc: Test AdjustIncreaseRatio
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_AdjustIncreaseRatio_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig config;
    float originDpi = 0.0f;
    float ret = config.AdjustIncreaseRatio(originDpi);
    EXPECT_EQ(ret, 1.22f);
    originDpi = 1.9f;
    ret = config.AdjustIncreaseRatio(originDpi);
    EXPECT_EQ(ret, 1.22f);
    originDpi = 2.125f;
    ret = config.AdjustIncreaseRatio(originDpi);
    EXPECT_NE(ret, 1.22f);
}

/**
 * @tc.name: PointerRendererTest_ImageSizeUpperBound_001
 * @tc.desc: Guardrail for INV-1: imageSize stays below the hard cursor buffer side length across
 *           the whole supported domain (size 1..7 x dpi (0, MAX_DPI]). Relaxing MAX_DPI or
 *           MAX_POINTER_SIZE must fail here instead of silently clipping the cursor.
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_ImageSizeUpperBound_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    constexpr int32_t hardCursorBufferSize = static_cast<int32_t>(HARD_CURSOR_SIZE);
    int32_t worstSize = 0;
    float worstDpi = 0.0f;
    uint32_t worstGear = 0;
    for (uint32_t gear = 1; gear <= MAX_POINTER_SIZE; ++gear) {
        for (int32_t step = 1; step <= 193; ++step) {
            RenderConfig config;
            config.align_ = ICON_TYPE::ANGLE_CENTER;
            config.size = gear;
            config.dpi = static_cast<float>(step) / 100.0f;
            int32_t imageSize = config.GetImageSize();
            if (imageSize > worstSize) {
                worstSize = imageSize;
                worstDpi = config.dpi;
                worstGear = gear;
            }
        }
    }
    EXPECT_LT(worstSize, hardCursorBufferSize)
        << "imageSize exceeds the buffer: size=" << worstSize << ", gear=" << worstGear
        << ", dpi=" << worstDpi;
}

/**
 * @tc.name: PointerRendererTest_ImageSizeUpperBound_002
 * @tc.desc: Pin the known leak channel: gear 1 bypasses the MAX_DPI suppression (exponent is 0),
 *           so imageSize = 40 * dpi grows unbounded and reaches 256 at dpi 6.4. Gear 7 is instead
 *           dpi-invariant. This documents why TASK-7 scale-to-fit is required.
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_ImageSizeUpperBound_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    RenderConfig gear1;
    gear1.align_ = ICON_TYPE::ANGLE_CENTER;
    gear1.size = 1;
    gear1.dpi = 6.4f;
    EXPECT_GE(gear1.GetImageSize(), 256) << "gear 1 leak threshold moved away from dpi 6.4";

    // Gear 7 cancels the dpi term entirely, so it stays at the calibrated ceiling.
    RenderConfig gear7Low;
    gear7Low.align_ = ICON_TYPE::ANGLE_CENTER;
    gear7Low.size = MAX_POINTER_SIZE;
    gear7Low.dpi = MAX_DPI;
    RenderConfig gear7High;
    gear7High.align_ = ICON_TYPE::ANGLE_CENTER;
    gear7High.size = MAX_POINTER_SIZE;
    gear7High.dpi = 100.0f;
    EXPECT_EQ(gear7Low.GetImageSize(), gear7High.GetImageSize())
        << "gear 7 is expected to be dpi-invariant";
    EXPECT_LT(gear7Low.GetImageSize(), 256);
}

/**
 * @tc.name: PointerRendererTest_RunningPivotBound_001
 * @tc.desc: Guardrail for INV-4: the RUNNING rotate pivot is glyph-relative, so the swept extent
 *           pivot_y + r_max (0.3003 * imageSize) must stay inside a 256 buffer. Adding the buffer
 *           focus to the pivot would push it out (128 + 254*0.675 = 299.5) and is regression-tested
 *           here because canvas.Rotate() consumes rotationFocus directly.
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_RunningPivotBound_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    constexpr int32_t hardCursorBufferSize = static_cast<int32_t>(HARD_CURSOR_SIZE);
    constexpr float runningXRatio = 0.3f;
    constexpr float runningYRatio = 0.675f;
    constexpr float runningInkRadiusRatio = 0.3003f;
    for (uint32_t gear = 1; gear <= MAX_POINTER_SIZE; ++gear) {
        RenderConfig config;
        config.align_ = ICON_TYPE::ANGLE_NW;
        config.size = gear;
        config.dpi = MAX_DPI;
        float imageSize = static_cast<float>(config.GetImageSize());
        ASSERT_GT(imageSize, 0.0f);
        // Mirrors the hard cursor pivot in HardwareCursorDynamicRender: glyph-relative only.
        float pivotX = imageSize * runningXRatio;
        float pivotY = imageSize * runningYRatio;
        float inkRadius = imageSize * runningInkRadiusRatio;
        EXPECT_GE(pivotX - inkRadius, -1.0f) << "gear=" << gear << " pivotX=" << pivotX;
        EXPECT_LT(pivotY + inkRadius, static_cast<float>(hardCursorBufferSize))
            << "RUNNING sweep overflows the buffer: gear=" << gear << ", imageSize=" << imageSize
            << ", pivotY=" << pivotY << ", inkRadius=" << inkRadius;
    }
}

/**
 * @tc.name: PointerRendererTest_ImgCacheKey_001
 * @tc.desc: Test the decoded-image cache is keyed by buffer side as well as config. Above the
 *           buffer limit two sides share the same unclamped GetImageSize(), so keying on config
 *           alone would let a 320px soft-cursor decode be reused for a 256px hard-cursor buffer
 *           and bypass GetClampedImageSize().
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_ImgCacheKey_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    RenderConfig config;
    config.style_ = MOUSE_ICON::LOADING;
    config.align_ = ICON_TYPE::ANGLE_CENTER;
    config.size = 1;
    config.dpi = 8.0f;   // gear 1 leak channel: unclamped imageSize is 320
    ASSERT_GT(config.GetImageSize(), static_cast<int32_t>(HARD_CURSOR_SIZE));

    image_ptr_t softImg = std::make_shared<OHOS::Rosen::Drawing::Image>();
    image_ptr_t hardImg = std::make_shared<OHOS::Rosen::Drawing::Image>();
    renderer.PushImg(SOFT_CURSOR_SIZE, config, softImg);
    renderer.PushImg(HARD_CURSOR_SIZE, config, hardImg);

    // Same config, different buffer side: each side must get its own entry back.
    EXPECT_EQ(renderer.FindImg(SOFT_CURSOR_SIZE, config), softImg);
    EXPECT_EQ(renderer.FindImg(HARD_CURSOR_SIZE, config), hardImg);
    // An unseen side must miss rather than silently reuse another side's decode.
    EXPECT_EQ(renderer.FindImg(1024, config), nullptr);
}
} // namespace MMI
} // namespace OHOS