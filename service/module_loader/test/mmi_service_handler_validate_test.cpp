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

#include <gtest/gtest.h>

#include "mmi_log.h"
#include "mmi_service.h"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
} // namespace

class MmiServiceHandlerValidateTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: MmiServiceHandlerValidateTest_CheckInputHandlerVaild_NotRunning_001
 * @tc.desc: Test CheckInputHandlerVaild returns NOT_RUNNING when service state is not running
 * @tc.type: FUNC
 * @tc.require: FEAT-INPUT-RECORDING
 */
HWTEST_F(MmiServiceHandlerValidateTest,
    MmiServiceHandlerValidateTest_CheckInputHandlerVaild_NotRunning_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    int32_t ret = mmiService.CheckInputHandlerVaild(InputHandlerType::MONITOR, true);
    EXPECT_EQ(ret, MMISERVICE_NOT_RUNNING);
}

/**
 * @tc.name: MmiServiceHandlerValidateTest_CheckInputHandlerVaild_RecordingChannel_001
 * @tc.desc: Test CheckInputHandlerVaild returns OK via recording channel for C API monitor
 * @tc.type: FUNC
 * @tc.require: FEAT-INPUT-RECORDING
 */
HWTEST_F(MmiServiceHandlerValidateTest,
    MmiServiceHandlerValidateTest_CheckInputHandlerVaild_RecordingChannel_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_RUNNING;
    int32_t ret = mmiService.CheckInputHandlerVaild(InputHandlerType::MONITOR, true);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: MmiServiceHandlerValidateTest_CheckInputHandlerVaild_CapiDefault_001
 * @tc.desc: Test CheckInputHandlerVaild keeps legacy path when isCapi is false
 * @tc.type: FUNC
 * @tc.require: FEAT-INPUT-RECORDING
 */
HWTEST_F(MmiServiceHandlerValidateTest,
    MmiServiceHandlerValidateTest_CheckInputHandlerVaild_CapiDefault_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_RUNNING;
    int32_t ret = mmiService.CheckInputHandlerVaild(InputHandlerType::MONITOR, false);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: MmiServiceHandlerValidateTest_CheckInputHandlerVaild_InterceptorCapi_001
 * @tc.desc: Test CheckInputHandlerVaild ignores isCapi for interceptor type
 * @tc.type: FUNC
 * @tc.require: FEAT-INPUT-RECORDING
 */
HWTEST_F(MmiServiceHandlerValidateTest,
    MmiServiceHandlerValidateTest_CheckInputHandlerVaild_InterceptorCapi_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    MMIService mmiService;
    mmiService.state_ = ServiceRunningState::STATE_RUNNING;
    int32_t ret = mmiService.CheckInputHandlerVaild(InputHandlerType::INTERCEPTOR, true);
    EXPECT_EQ(ret, RET_OK);
}
} // namespace MMI
} // namespace OHOS
