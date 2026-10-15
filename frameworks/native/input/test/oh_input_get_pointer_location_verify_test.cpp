/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

/**
 * @file oh_input_get_pointer_location_verify_test.cpp
 * @brief On-device verification tests for OH_Input_GetPointerLocation (feature 20260831443646)
 * @desc Verify that non-focused applications can obtain the pointer location on PC products
 * without permission or focus checks, while other products keep the original checks.
 */

#include <cmath>
#include <cstdio>
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "oh_input_manager.h"
#include "input_manager.h"
#include "mmi_log.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "GetPointerLocationVerifyTest"

namespace OHOS {
namespace system {
// Implemented in libbegetutil, see syspara/parameters.h.
std::string GetParameter(const std::string &key, const std::string &def);
} // namespace system
} // namespace OHOS

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;

constexpr const char *DEVICE_TYPE_KEY = "const.product.devicetype";
constexpr const char *PRODUCT_TYPE_PC_VALUE = "2in1";

bool IsPcDevice()
{
    return OHOS::system::GetParameter(DEVICE_TYPE_KEY, "unknown") == PRODUCT_TYPE_PC_VALUE;
}

// Checks AC-1.1 output contract: displayId is a valid display id and coordinates are finite.
void CheckCoordinates(int32_t displayId, double displayX, double displayY)
{
    EXPECT_GE(displayId, 0);
    EXPECT_TRUE(std::isfinite(displayX));
    EXPECT_TRUE(std::isfinite(displayY));
    MMI_HILOGI("Pointer location: displayId: %{public}d, displayX: %{public}f, displayY: %{public}f",
        displayId, displayX, displayY);
}
} // namespace

class GetPointerLocationVerifyTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        MMI_HILOGI("GetPointerLocationVerifyTest SetUpTestCase, deviceType: %{public}s, isPc: %{public}d",
            OHOS::system::GetParameter(DEVICE_TYPE_KEY, "unknown").c_str(), IsPcDevice());
    }

    static void TearDownTestCase(void)
    {
        MMI_HILOGI("GetPointerLocationVerifyTest TearDownTestCase");
    }

    void SetUp() override {}

    void TearDown() override {}
};

/**
 * @tc.name: OH_Input_GetPointerLocation_NullParameter_001
 * @tc.desc: Verify EC-04: null pointer parameters return INPUT_PARAMETER_ERROR(401) and no
 * output parameter is written
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GetPointerLocationVerifyTest, OH_Input_GetPointerLocation_NullParameter_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    constexpr int32_t sentinelId = -100;
    constexpr double sentinelCoord = -100.0;
    int32_t displayId = sentinelId;
    double displayX = sentinelCoord;
    double displayY = sentinelCoord;

    Input_Result ret = OH_Input_GetPointerLocation(nullptr, &displayX, &displayY);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);

    ret = OH_Input_GetPointerLocation(&displayId, nullptr, &displayY);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);

    ret = OH_Input_GetPointerLocation(&displayId, &displayX, nullptr);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);

    ret = OH_Input_GetPointerLocation(nullptr, nullptr, nullptr);
    EXPECT_EQ(ret, INPUT_PARAMETER_ERROR);

    // EC-04: no memory write happens on null parameters
    EXPECT_EQ(displayId, sentinelId);
    EXPECT_EQ(displayX, sentinelCoord);
    EXPECT_EQ(displayY, sentinelCoord);
}

/**
 * @tc.name: OH_Input_GetPointerLocation_NonFocus_NoPermission_001
 * @tc.desc: Verify AC-1.1 (BR-01): on PC products a non-focused application without permission
 * obtains the pointer location (INPUT_SUCCESS with valid coordinates); verify BR-02 (EX-04):
 * non-PC products keep the original checks and still reject the call
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GetPointerLocationVerifyTest, OH_Input_GetPointerLocation_NonFocus_NoPermission_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t displayId = -1;
    double displayX = -1.0;
    double displayY = -1.0;
    // The test process itself is a non-focused caller without INPUT_DEVICE_CONFIGURATOR permission.
    Input_Result ret = OH_Input_GetPointerLocation(&displayId, &displayX, &displayY);
    if (ret == INPUT_DEVICE_NO_POINTER) {
        // AC-1.1 requires a pointer device to be connected; skip on products without one.
        MMI_HILOGI("No pointer device on this product, skip the non-focus assertion");
        return;
    }
    if (IsPcDevice()) {
        // BR-01: token type and focus checks are skipped on PC, EC-02: INPUT_APP_NOT_FOCUSED
        // (3900009) is no longer reported on PC.
        EXPECT_EQ(ret, INPUT_SUCCESS);
        if (ret == INPUT_SUCCESS) {
            CheckCoordinates(displayId, displayX, displayY);
        }
    } else {
        // BR-02 / EX-04: non-PC products keep the original checks; INPUT_APP_NOT_FOCUSED
        // (3900009) is still reported for non-HAP callers.
        MMI_HILOGI("Non-PC device, result: %{public}d (expected INPUT_APP_NOT_FOCUSED: %{public}d)",
            ret, INPUT_APP_NOT_FOCUSED);
        EXPECT_NE(ret, INPUT_SUCCESS);
    }
}

/**
 * @tc.name: OH_Input_GetPointerLocation_NoPointerDevice_002
 * @tc.desc: Verify AC-1.2 (BR-03, EX-01, EC-03): with no pointer device connected the interface
 * returns INPUT_DEVICE_NO_POINTER(3900010) and output parameters are not filled.
 * Run this case with all pointer devices disconnected
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GetPointerLocationVerifyTest, OH_Input_GetPointerLocation_NoPointerDevice_002, TestSize.Level2)
{
    CALL_TEST_DEBUG;
    constexpr int32_t sentinelId = -1;
    constexpr double sentinelCoord = -1.0;
    int32_t displayId = sentinelId;
    double displayX = sentinelCoord;
    double displayY = sentinelCoord;
    Input_Result ret = OH_Input_GetPointerLocation(&displayId, &displayX, &displayY);
    if (ret != INPUT_DEVICE_NO_POINTER) {
        // The device-existence check (HasPointerDeviceIncludingVirtual) is kept by this feature;
        // to exercise AC-1.2 disconnect every pointer device (physical and virtual) and rerun.
        MMI_HILOGI("Pointer device present (ret: %{public}d), AC-1.2 requires all pointer devices "
                  "to be disconnected", ret);
        return;
    }
    EXPECT_EQ(ret, INPUT_DEVICE_NO_POINTER);
    // EC-03 / EX-01: output parameters are not filled on failure.
    EXPECT_EQ(displayId, sentinelId);
    EXPECT_EQ(displayX, sentinelCoord);
    EXPECT_EQ(displayY, sentinelCoord);
}

/**
 * @tc.name: OH_Input_GetPointerLocation_MultiCall_Consistency_003
 * @tc.desc: Verify BR-04: the interface signature and return-value semantics stay unchanged,
 * repeated calls report the same result
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GetPointerLocationVerifyTest, OH_Input_GetPointerLocation_MultiCall_Consistency_003, TestSize.Level2)
{
    CALL_TEST_DEBUG;
    int32_t displayId1 = -1;
    double displayX1 = -1.0;
    double displayY1 = -1.0;
    Input_Result ret1 = OH_Input_GetPointerLocation(&displayId1, &displayX1, &displayY1);

    int32_t displayId2 = -1;
    double displayX2 = -1.0;
    double displayY2 = -1.0;
    Input_Result ret2 = OH_Input_GetPointerLocation(&displayId2, &displayX2, &displayY2);

    EXPECT_EQ(ret1, ret2);
    if (ret1 == INPUT_SUCCESS && ret2 == INPUT_SUCCESS) {
        CheckCoordinates(displayId1, displayX1, displayY1);
        CheckCoordinates(displayId2, displayX2, displayY2);
    }
}
} // namespace MMI
} // namespace OHOS
