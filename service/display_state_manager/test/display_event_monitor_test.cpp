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

#include "display_event_monitor.h"
#include "mmi_log.h"
#include "mock.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "DisplayEventMonitorTest"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;
using namespace testing;
} // namespace

class DisplayEventMonitorTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    static inline std::shared_ptr<MessageParcelMock> messageParcelMock_ = nullptr;
};

void DisplayEventMonitorTest::SetUpTestCase(void)
{
    messageParcelMock_ = std::make_shared<MessageParcelMock>();
    MessageParcelMock::messageParcel = messageParcelMock_;
}
void DisplayEventMonitorTest::TearDownTestCase()
{
    MessageParcelMock::messageParcel = nullptr;
    messageParcelMock_ = nullptr;
}

/**
 * @tc.name: DisplayEventMonitorTest_InitCommonEventSubscriber
 * @tc.desc: Test InitCommonEventSubscriber
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DisplayEventMonitorTest, DisplayEventMonitorTest_InitCommonEventSubscriber, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    DisplayEventMonitor displayEventMonitor;
    displayEventMonitor.hasInit_ = true;
    EXPECT_NO_FATAL_FAILURE(displayEventMonitor.InitCommonEventSubscriber());
    displayEventMonitor.hasInit_ = false;
    EXPECT_NO_FATAL_FAILURE(displayEventMonitor.InitCommonEventSubscriber());
}
} // namespace MMI
} // namespace OHOS