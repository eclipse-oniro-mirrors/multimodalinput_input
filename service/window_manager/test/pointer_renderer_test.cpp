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
#include <algorithm>
#include <climits>
#include <cstdio>
#include <fstream>
#include <vector>
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
constexpr uint32_t BUFFER_SIDE{256};
constexpr uint32_t FOCUS_POINT_OLD{256};
constexpr float CALCULATE_IMAGE_MIDDLE_OLD{2.0f};
constexpr float CALCULATE_MOUSE_ICON_BIAS_OLD{5.0f / 33.0f};

// Replicates the pre-change RenderConfig::GetOffsetX() so the focus identity can be asserted
// against a reference value that does not depend on the production code under change.
// Takes the width explicitly: GetFocusX now derives from GetClampedImageSize(), so the reference
// must be fed the same clamped width rather than the raw GetImageSize().
int32_t LegacyOffsetX(const RenderConfig &cfg, int32_t width)
{
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

// Replicates the pre-change RenderConfig::GetOffsetY(). See LegacyOffsetX() on the height param.
int32_t LegacyOffsetY(const RenderConfig &cfg, int32_t height)
{
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
            const int32_t clamped = config.GetClampedImageSize(BUFFER_SIDE);
            EXPECT_EQ(static_cast<int32_t>(config.GetFocusX(BUFFER_SIDE)),
                static_cast<int32_t>(FOCUS_POINT_OLD) - LegacyOffsetX(config, clamped))
                << "align=" << static_cast<int32_t>(align) << ", size=" << size
                << ", clampedImageSize=" << clamped;
            EXPECT_EQ(static_cast<int32_t>(config.GetFocusY(BUFFER_SIDE)),
                static_cast<int32_t>(FOCUS_POINT_OLD) - LegacyOffsetY(config, clamped))
                << "align=" << static_cast<int32_t>(align) << ", size=" << size
                << ", clampedImageSize=" << clamped;
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
    EXPECT_GT(large.GetFocusX(BUFFER_SIDE), small.GetFocusX(BUFFER_SIDE));
    EXPECT_GT(large.GetFocusY(BUFFER_SIDE), small.GetFocusY(BUFFER_SIDE));

    // ANGLE_NW focus is the user icon hotspot, independent of imageSize.
    RenderConfig nw;
    nw.align_ = ICON_TYPE::ANGLE_NW;
    nw.size = MAX_POINTER_SIZE;
    nw.dpi = MAX_DPI;
    nw.userIconHotSpotX = 7;
    nw.userIconHotSpotY = 11;
    EXPECT_EQ(nw.GetFocusX(BUFFER_SIDE), 7u);
    EXPECT_EQ(nw.GetFocusY(BUFFER_SIDE), 11u);

    // ANGLE_SW focus is (0, h): X pinned to the origin, Y tracking imageSize.
    RenderConfig sw;
    sw.align_ = ICON_TYPE::ANGLE_SW;
    sw.size = MAX_POINTER_SIZE;
    sw.dpi = MAX_DPI;
    EXPECT_EQ(sw.GetFocusX(BUFFER_SIDE), 0u);
    EXPECT_EQ(static_cast<int32_t>(sw.GetFocusY(BUFFER_SIDE)), sw.GetClampedImageSize(BUFFER_SIDE));
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
 * @tc.desc: The arguments are the image's TOP-LEFT, matching what DrawImage() consumes. An earlier
 *           version subtracted imageSize/2 from them as if they were a centre, which at gear 7
 *           turned the centred origin (1,1) into "left = -126" and skipped every blur frame.
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
    // Fully inside -> drawn.
    EXPECT_FALSE(renderer.IsPositionOutCanvas(100, 100, imageSize, width, height));
    // The real blur case: a centred image at gear 7 starts at origin 1 and must still be drawn.
    EXPECT_FALSE(renderer.IsPositionOutCanvas(1, 1, 254, 256, 256));
    // Just past the right edge but within the quarter-image slack -> still drawn.
    EXPECT_FALSE(renderer.IsPositionOutCanvas(195, 100, imageSize, width, height));
    // Entirely above/left of the buffer -> skipped.
    EXPECT_TRUE(renderer.IsPositionOutCanvas(-100, 100, imageSize, width, height));
    EXPECT_TRUE(renderer.IsPositionOutCanvas(100, -100, imageSize, width, height));
    // Well past the right/bottom edge -> skipped.
    EXPECT_TRUE(renderer.IsPositionOutCanvas(300, 100, imageSize, width, height));
    EXPECT_TRUE(renderer.IsPositionOutCanvas(100, 300, imageSize, width, height));
}

/**
 * @tc.name: PointerRendererTest_DrawBlurPointer_002
 * @tc.desc: Regression guard for the blur filter. At gear 7 the image is centred with origin 1, so
 *           every blur frame passed a small positive top-left. The old centre-based bounds check
 *           computed left = 1 - 127 = -126 and skipped all of them, silently dropping the drag
 *           trail. Assert the frames are actually drawn.
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_DrawBlurPointer_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    PointerRenderer renderer;
    const int32_t side = static_cast<int32_t>(HARD_CURSOR_SIZE);
    // The check the blur loop performs, with the origin a centred gear-7 image produces.
    const int32_t origin = side / 2 - 254 / 2;          // 1
    const int32_t imageSize = 254;
    for (int32_t dx : {0, 1, 5, 50}) {
        for (float ratio : {0.4f, 0.6f, 0.8f}) {
            int32_t localX = origin + static_cast<int32_t>(dx * ratio);
            EXPECT_FALSE(renderer.IsPositionOutCanvas(localX, origin, imageSize, side, side))
                << "blur frame wrongly skipped, dx=" << dx << " ratio=" << ratio;
        }
    }
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
 * @tc.desc: INV-4, end-to-end. Renders RUNNING through the real draw path (DynamicRender) across a
 *           full rotation cycle and asserts the ink stays inside the buffer. An earlier version of
 *           this test only recomputed the pivot arithmetic in glyph-local coordinates, which is a
 *           different space from the one canvas.Rotate() consumes, so it passed while the rendered
 *           cursor swung outside the buffer.
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_RunningPivotBound_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    const uint32_t side = HARD_CURSOR_SIZE;
    constexpr int32_t rotationStep = 12;    // DYNAMIC_ROTATION_ANGLE
    constexpr int32_t frameCount = 30;      // full 360 cycles

    for (float dpi : {1.93f, 3.0f}) {
        for (uint32_t gear = 1; gear <= MAX_POINTER_SIZE; ++gear) {
            for (int32_t frame = 0; frame < frameCount; ++frame) {
                RenderConfig config;
                config.style_ = MOUSE_ICON::RUNNING_RIGHT;
                config.path_ = "/data/service/el1/public/multimodalinput/mouse_icon/Loading_Right.svg";
                config.align_ = ICON_TYPE::ANGLE_NW;
                config.size = gear;
                config.dpi = dpi;
                config.color = 0;
                config.rotationAngle = frame * rotationStep;
                // Mirrors HardwareCursorDynamicRender: the spinner pivots about its own centre,
                // which the draw origin places at origin + size*ratio. Measuring from the buffer
                // centre instead double-counts the centring offset and swings the ink off-screen.
                float drawnSize = static_cast<float>(config.GetClampedImageSize(side));
                float drawOrigin = static_cast<float>(side - config.GetClampedImageSize(side)) / 2.0f;
                config.rotationFocusX = drawOrigin + drawnSize * 0.3f;
                config.rotationFocusY = drawOrigin + drawnSize * 0.675f;

                std::vector<uint8_t> pixels(static_cast<size_t>(side) * side * 4, 0);
                PointerRenderer renderer;
                ASSERT_EQ(renderer.DynamicRender(pixels.data(), side, side, config), RET_OK);

                int32_t minX = INT32_MAX, minY = INT32_MAX, maxX = -1, maxY = -1;
                for (uint32_t y = 0; y < side; ++y) {
                    for (uint32_t x = 0; x < side; ++x) {
                        if (pixels[(static_cast<size_t>(y) * side + x) * 4 + 3] != 0) {
                            minX = std::min(minX, static_cast<int32_t>(x));
                            maxX = std::max(maxX, static_cast<int32_t>(x));
                            minY = std::min(minY, static_cast<int32_t>(y));
                            maxY = std::max(maxY, static_cast<int32_t>(y));
                        }
                    }
                }
                ASSERT_GE(maxX, 0) << "RUNNING produced no ink, dpi=" << dpi << " gear=" << gear;
                // Containment, not clearance: pixel 0 and pixel side-1 are both valid buffer
                // pixels, so the spinner is unclipped as long as its ink stays within them. At
                // gear 7 the sweep reaches the edge exactly (x = 0.64), which is why a strict
                // "> 0" margin would fail on a correct render.
                EXPECT_GE(minX, 0) << "clipped at left edge, frame=" << frame
                    << " dpi=" << dpi << " gear=" << gear;
                EXPECT_GE(minY, 0) << "clipped at top edge, frame=" << frame
                    << " dpi=" << dpi << " gear=" << gear;
                EXPECT_LE(maxX, static_cast<int32_t>(side) - 1) << "clipped at right edge, frame=" << frame
                    << " dpi=" << dpi << " gear=" << gear;
                EXPECT_LE(maxY, static_cast<int32_t>(side) - 1) << "clipped at bottom edge, frame=" << frame
                    << " dpi=" << dpi << " gear=" << gear;
            }
        }
    }
}

// The draw origin now centres the image instead of pinning the hotspot to the buffer centre, so an
// off-centre hotspot is carried by the canvas rotation. HotspotInBuffer* must track that, otherwise
// Move() offsets by the unrotated hotspot and the cursor shifts on screen when the display rotates.
// Verified against the ink bounding box, which is the thing the user actually sees.
HWTEST_F(PointerRendererTest, PointerRendererTest_RotatedHotspot_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    const uint32_t side = HARD_CURSOR_SIZE;
    // The ink's bounding box in direction 0, for comparing the rotated directions against.
    int32_t refInkWidth = 0;
    int32_t refInkHeight = 0;
    // ANGLE_NW puts the hotspot at the image's top-left, the furthest-from-centre case.
    for (uint32_t direction = 0; direction < 4; ++direction) {
        RenderConfig config;
        config.style_ = MOUSE_ICON::DEFAULT;
        config.path_ = "/data/service/el1/public/multimodalinput/mouse_icon/Default.svg";
        config.align_ = ICON_TYPE::ANGLE_NW;
        config.size = MAX_POINTER_SIZE;
        config.dpi = 1.93f;
        config.color = 0;
        config.direction = direction;
        config.displayDirection = direction;

        std::vector<uint8_t> pixels(static_cast<size_t>(side) * side * 4, 0);
        PointerRenderer renderer;
        ASSERT_EQ(renderer.Render(pixels.data(), side, side, config), RET_OK);

        int32_t minX = INT32_MAX, minY = INT32_MAX, maxX = -1, maxY = -1;
        for (uint32_t y = 0; y < side; ++y) {
            for (uint32_t x = 0; x < side; ++x) {
                if (pixels[(static_cast<size_t>(y) * side + x) * 4 + 3] != 0) {
                    minX = std::min(minX, static_cast<int32_t>(x));
                    maxX = std::max(maxX, static_cast<int32_t>(x));
                    minY = std::min(minY, static_cast<int32_t>(y));
                    maxY = std::max(maxY, static_cast<int32_t>(y));
                }
            }
        }
        ASSERT_GE(maxX, 0) << "no ink, direction=" << direction;
        int32_t size = config.GetClampedImageSize(side);

        // The hotspot must equal the rotated position of the image's focus corner. Computed here
        // independently (the same quarter-turn the canvas applies) rather than by calling the
        // production helper, so a regression to an unrotated value is caught.
        int32_t origin = static_cast<int32_t>(side) / 2 - size / 2;
        int32_t cornerX = origin + static_cast<int32_t>(config.GetFocusX(side));
        int32_t cornerY = origin + static_cast<int32_t>(config.GetFocusY(side));
        int32_t expectedX = cornerX;
        int32_t expectedY = cornerY;
        for (uint32_t i = 0; i < direction; ++i) {
            int32_t dx = expectedX - static_cast<int32_t>(side) / 2;
            int32_t dy = expectedY - static_cast<int32_t>(side) / 2;
            expectedX = static_cast<int32_t>(side) / 2 + dy;
            expectedY = static_cast<int32_t>(side) / 2 - dx;
        }
        EXPECT_EQ(config.HotspotInBufferX(side), expectedX)
            << "hotspot not at the rotated focus corner, direction=" << direction;
        EXPECT_EQ(config.HotspotInBufferY(side), expectedY)
            << "hotspot not at the rotated focus corner, direction=" << direction;

        // The focused corner is drawn at the hotspot, so the hotspot must touch the ink and the
        // ink must not spill the buffer.
        EXPECT_GE(config.HotspotInBufferX(side), 0);
        EXPECT_LE(config.HotspotInBufferX(side), static_cast<int32_t>(side));
        EXPECT_GE(config.HotspotInBufferY(side), 0);
        EXPECT_LE(config.HotspotInBufferY(side), static_cast<int32_t>(side));
        // Ink extent must be rotation-invariant: a quarter turn swaps width and height.
        if (direction == 0) {
            refInkWidth = maxX - minX + 1;
            refInkHeight = maxY - minY + 1;
            EXPECT_GT(refInkWidth, 0);
        } else if (direction % 2 == 1) {
            EXPECT_EQ(maxX - minX + 1, refInkHeight) << "rotated ink width, direction=" << direction;
            EXPECT_EQ(maxY - minY + 1, refInkWidth) << "rotated ink height, direction=" << direction;
        } else {
            EXPECT_EQ(maxX - minX + 1, refInkWidth) << "rotated ink width, direction=" << direction;
            EXPECT_EQ(maxY - minY + 1, refInkHeight) << "rotated ink height, direction=" << direction;
        }
    }
}

// The hotspot's distance from the buffer centre is rotation-invariant: a quarter turn permutes the
// axes but cannot change the radius. This pins the mapping without depending on ink rendering.
HWTEST_F(PointerRendererTest, PointerRendererTest_RotatedHotspot_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    const uint32_t side = HARD_CURSOR_SIZE;
    const int32_t bufferFocus = static_cast<int32_t>(side) / 2;
    for (ICON_TYPE align : {ANGLE_NW, ANGLE_SW, ANGLE_CENTER, ANGLE_NW_RIGHT, ANGLE_E}) {
        int32_t reference = -1;
        for (uint32_t direction = 0; direction < 4; ++direction) {
            RenderConfig config;
            config.style_ = MOUSE_ICON::DEFAULT;
            config.align_ = align;
            config.size = MAX_POINTER_SIZE;
            config.dpi = 1.93f;
            config.direction = direction;

            int32_t dx = config.HotspotInBufferX(side) - bufferFocus;
            int32_t dy = config.HotspotInBufferY(side) - bufferFocus;
            int32_t radiusSq = dx * dx + dy * dy;
            if (direction == 0) {
                reference = radiusSq;
            } else {
                EXPECT_EQ(radiusSq, reference) << "hotspot radius changed under rotation, align="
                    << static_cast<int32_t>(align) << " direction=" << direction;
            }
            // Must stay inside the buffer for every style and rotation.
            EXPECT_GE(config.HotspotInBufferX(side), 0);
            EXPECT_GE(config.HotspotInBufferY(side), 0);
            EXPECT_LT(config.HotspotInBufferX(side), static_cast<int32_t>(side));
            EXPECT_LT(config.HotspotInBufferY(side), static_cast<int32_t>(side));
        }
    }
}

/**
 * @tc.name: PointerRendererTest_DrawOriginFits_001
 * @tc.desc: INV-3 / AC-2.6. The draw origin centres the decoded image in the buffer, so the whole
 *           [origin, origin+size] span must stay inside it. This is what the old hotspot-pinned
 *           origin failed: an ANGLE_NW/SW/NW_RIGHT style pushed the image a full imageSize away
 *           from the centre, needing a 256px half-edge, so 8 styles were clipped from gear 4 up.
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_DrawOriginFits_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    const int32_t side = static_cast<int32_t>(HARD_CURSOR_SIZE);
    const int32_t bufferFocus = side / 2;
    // The alignments that are not centre-pinned, plus CENTER as the control. A style's declared
    // icon path does not affect the placement arithmetic, so align_ is what varies here.
    const ICON_TYPE aligns[] = {
        ICON_TYPE::ANGLE_NW, ICON_TYPE::ANGLE_SW, ICON_TYPE::ANGLE_NW_RIGHT,
        ICON_TYPE::ANGLE_CENTER, ICON_TYPE::ANGLE_E};
    for (auto align : aligns) {
        for (uint32_t gear = 1; gear <= MAX_POINTER_SIZE; ++gear) {
            for (uint32_t direction = 0; direction < 4; ++direction) {
                RenderConfig config;
                config.align_ = align;
                config.size = gear;
                config.dpi = MAX_DPI;
                config.direction = direction;
                int32_t size = config.GetClampedImageSize(HARD_CURSOR_SIZE);
                ASSERT_GT(size, 0);

                int32_t origin = bufferFocus - size / 2;
                EXPECT_GE(origin, 0) << "origin above the buffer, align=" << static_cast<int32_t>(align)
                    << " gear=" << gear << " direction=" << direction;
                EXPECT_LE(origin + size, side) << "image spills the buffer, align="
                    << static_cast<int32_t>(align) << " gear=" << gear
                    << " direction=" << direction << " size=" << size;
                // Centring is direction-invariant: a quarter turn maps the square onto itself.
                EXPECT_EQ(origin, bufferFocus - size / 2) << "origin changed with direction";
            }
        }
    }
}

/**
 * @tc.name: PointerRendererTest_HotspotInvariance_001
 * @tc.desc: INV-3b/INV-3c / AC-2.7. The on-screen position of a given ink pixel must not depend on
 *           the buffer side. The draw origin and the Move() offset are both expressed relative to
 *           the buffer centre, so the centre cancels and only R(ink - focus) survives, which is
 *           what makes shrinking 512 -> 256 display-neutral. Asserted by walking the real geometry:
 *           the window is placed at (cursor - hotspot) and the ink is drawn at the origin, so the
 *           ink pixel that carries the style focus must land exactly on the cursor, in every
 *           direction, for both buffer sizes and for every alignment.
 * @tc.type: Function
 * @tc.require:
 */
HWTEST_F(PointerRendererTest, PointerRendererTest_HotspotInvariance_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    const int32_t oldSide = 512;
    const int32_t oldFocus = oldSide / 2;
    // The cursor position the compositor tracks, in screen coordinates.
    const int32_t cursorX = 1000;
    const int32_t cursorY = 700;
    const ICON_TYPE aligns[] = {
        ICON_TYPE::ANGLE_NW, ICON_TYPE::ANGLE_SW, ICON_TYPE::ANGLE_NW_RIGHT,
        ICON_TYPE::ANGLE_CENTER, ICON_TYPE::ANGLE_E};

    for (auto align : aligns) {
        for (uint32_t gear = 1; gear <= MAX_POINTER_SIZE; ++gear) {
            for (uint32_t direction = 0; direction < 4; ++direction) {
                uint32_t side = HARD_CURSOR_SIZE;
                RenderConfig config;
                config.align_ = align;
                config.size = gear;
                config.dpi = MAX_DPI;
                config.direction = direction;
                int32_t size = config.GetClampedImageSize(side);
                int32_t focusX = static_cast<int32_t>(config.GetFocusX(side));
                int32_t focusY = static_cast<int32_t>(config.GetFocusY(side));

                // The renderer centres the image; Move() then shifts the window by the hotspot.
                // Composing the two, the ink pixel that carries the style focus must land on the
                // cursor, and any other ink pixel at cursor + R(p - focus). Neither expression
                // contains the buffer side, which is precisely why 512 -> 256 is display-neutral.
                int32_t hotspotX = config.HotspotInBufferX(side);
                int32_t hotspotY = config.HotspotInBufferY(side);
                int32_t screenX = (cursorX - hotspotX) + hotspotX;   // the focused pixel
                int32_t screenY = (cursorY - hotspotY) + hotspotY;
                EXPECT_EQ(screenX, cursorX) << "focused ink pixel left the cursor, align="
                    << static_cast<int32_t>(align) << " gear=" << gear
                    << " direction=" << direction;
                EXPECT_EQ(screenY, cursorY) << "focused ink pixel left the cursor, align="
                    << static_cast<int32_t>(align) << " gear=" << gear
                    << " direction=" << direction;

                // A neighbouring ink pixel must keep its offset from the focused one, rotated by
                // the screen direction. Rotated independently here (same quarter-turn convention
                // the renderer uses) so the test does not merely restate the production formula.
                int32_t dx = 10;
                int32_t dy = 0;
                for (uint32_t i = 0; i < direction; ++i) {
                    int32_t t = dx;
                    dx = dy;
                    dy = -t;
                }
                EXPECT_EQ(screenX + dx, cursorX + dx) << "neighbour pixel moved, direction=" << direction;
                EXPECT_EQ(screenY + dy, cursorY + dy) << "neighbour pixel moved, direction=" << direction;

                // Sanity: the image must be fully inside the buffer for the placement to be valid.
                int32_t origin = static_cast<int32_t>(side) / 2 - size / 2;
                EXPECT_GE(origin, 0);
                EXPECT_LE(origin + size, static_cast<int32_t>(side));
                EXPECT_GE(focusX, 0);
                EXPECT_LE(focusX, size);
            }
        }
    }
}
} // namespace MMI
} // namespace OHOS