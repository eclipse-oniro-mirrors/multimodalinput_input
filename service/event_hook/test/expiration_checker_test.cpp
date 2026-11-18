/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <chrono>

#include "expiration_checker.h"
#include "key_event.h"

#undef MMI_LOG_TAG
#define MMI_LOG_TAG "ExpirationCheckerTest"

namespace OHOS {
namespace MMI {
namespace {
using namespace testing::ext;

std::shared_ptr<KeyEvent> MakeKeyEvent(int32_t eventId)
{
    auto keyEvent = KeyEvent::Create();
    if (keyEvent != nullptr) {
        keyEvent->SetId(eventId);
        keyEvent->SetKeyCode(KeyEvent::KEYCODE_A);
        keyEvent->SetKeyAction(KeyEvent::KEY_ACTION_DOWN);
    }
    return keyEvent;

}
int64_t NowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
}  // namespace

class ExpirationCheckerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
};

/**
 * @tc.name: ExpirationCheckerTest_UpdateAndGet001
 * @tc.desc: Update then Get round-trip; unknown id returns null
 * @tc.type: FUNC
 */
HWTEST_F(ExpirationCheckerTest, ExpirationCheckerTest_UpdateAndGet001, TestSize.Level0)
{
    ExpirationChecker checker;
    EXPECT_EQ(checker.GetEvent(1), nullptr);

    auto keyEvent = MakeKeyEvent(1);
    ASSERT_NE(keyEvent, nullptr);
    checker.UpdateInputEvent(keyEvent);
    EXPECT_EQ(checker.GetEvent(1), keyEvent);
}

/**
 * @tc.name: ExpirationCheckerTest_RemoveEvent001
 * @tc.desc: RemoveEvent removes the stashed event by id
 * @tc.type: FUNC
 */
HWTEST_F(ExpirationCheckerTest, ExpirationCheckerTest_RemoveEvent001, TestSize.Level0)
{
    ExpirationChecker checker;
    auto keyEvent = MakeKeyEvent(1);
    ASSERT_NE(keyEvent, nullptr);
    checker.UpdateInputEvent(keyEvent);
    checker.RemoveEvent(1);
    EXPECT_EQ(checker.GetEvent(1), nullptr);
    // Repeated removal is harmless
    checker.RemoveEvent(1);
    EXPECT_EQ(checker.GetEvent(1), nullptr);
}

/**
 * @tc.name: ExpirationCheckerTest_TakeExpiredEvents001
 * @tc.desc: TakeExpiredEvents takes stale events and keeps fresh ones
 * @tc.type: FUNC
 */
HWTEST_F(ExpirationCheckerTest, ExpirationCheckerTest_TakeExpiredEvents001, TestSize.Level0)
{
    ExpirationChecker checker;
    auto staleEvent = MakeKeyEvent(1);
    ASSERT_NE(staleEvent, nullptr);
    checker.UpdateInputEvent(staleEvent, NowMs() - 5000);
    auto freshEvent = MakeKeyEvent(2);
    ASSERT_NE(freshEvent, nullptr);
    checker.UpdateInputEvent(freshEvent, NowMs());

    auto expired = checker.TakeExpiredEvents();
    ASSERT_EQ(expired.size(), 1u);
    EXPECT_EQ(expired[0]->GetId(), 1);
    // The expired event is out of the stash
    EXPECT_EQ(checker.GetEvent(1), nullptr);
    EXPECT_NE(checker.GetEvent(2), nullptr);
}

/**
 * @tc.name: ExpirationCheckerTest_TakeExpiredEvents002
 * @tc.desc: TakeExpiredEvents with empty or all-fresh stash returns empty
 * @tc.type: FUNC
 */
HWTEST_F(ExpirationCheckerTest, ExpirationCheckerTest_TakeExpiredEvents002, TestSize.Level0)
{
    ExpirationChecker checker;
    EXPECT_TRUE(checker.TakeExpiredEvents().empty());

    auto keyEvent = MakeKeyEvent(1);
    ASSERT_NE(keyEvent, nullptr);
    checker.UpdateInputEvent(keyEvent, NowMs());
    EXPECT_TRUE(checker.TakeExpiredEvents().empty());
    EXPECT_NE(checker.GetEvent(1), nullptr);
}

/**
 * @tc.name: ExpirationCheckerTest_DrainStashEvents001
 * @tc.desc: DrainStashEvents returns all events and clears stash
 * @tc.type: FUNC
 */
HWTEST_F(ExpirationCheckerTest, ExpirationCheckerTest_DrainStashEvents001, TestSize.Level0)
{
    ExpirationChecker checker;
    auto keyEvent1 = MakeKeyEvent(1);
    ASSERT_NE(keyEvent1, nullptr);
    auto keyEvent2 = MakeKeyEvent(2);
    ASSERT_NE(keyEvent2, nullptr);
    checker.UpdateInputEvent(keyEvent1);
    checker.UpdateInputEvent(keyEvent2);

    auto drained = checker.DrainStashEvents();
    ASSERT_EQ(drained.size(), 2u);
    EXPECT_EQ(checker.GetEvent(1), nullptr);
    EXPECT_EQ(checker.GetEvent(2), nullptr);
    EXPECT_TRUE(checker.DrainStashEvents().empty());
}

/**
 * @tc.name: ExpirationCheckerTest_UpdateNullEvent001
 * @tc.desc: UpdateInputEvent with nullptr event is ignored
 * @tc.type: FUNC
 */
HWTEST_F(ExpirationCheckerTest, ExpirationCheckerTest_UpdateNullEvent001, TestSize.Level1)
{
    ExpirationChecker checker;
    checker.UpdateInputEvent(nullptr);
    EXPECT_TRUE(checker.TakeExpiredEvents().empty());
    EXPECT_TRUE(checker.DrainStashEvents().empty());
}
}  // namespace MMI
}  // namespace OHOS
