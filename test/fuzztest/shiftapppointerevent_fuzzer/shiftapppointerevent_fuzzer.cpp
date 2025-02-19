﻿/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "shiftapppointerevent_fuzzer.h"

#include "input_manager.h"
#include "define_multimodal.h"
#include "mmi_service.h"
#include "mmi_log.h"


namespace OHOS {
namespace MMI {
template<class T>
size_t GetObject(T& object, const uint8_t* data, size_t size)
{
    size_t objectSize = sizeof(object);
    if (objectSize > size) {
        return 0;
    }
    errno_t ret = memcpy_s(&object, objectSize, data, objectSize);
    if (ret != EOK) {
        return 0;
    }
    return objectSize;
}

bool ShiftAppPointerEvent(const uint8_t* data, const size_t size, size_t& startPos)
{
    bool autoGenDown;
    ShiftWindowParam param;
    startPos += GetObject<ShiftWindowParam>(param, data + startPos, size - startPos);
    startPos += GetObject<bool>(autoGenDown, data + startPos, size - startPos);
    InputManager::GetInstance()->ShiftAppPointerEvent(param, autoGenDown);
    return true;
}
		
bool ShiftAppPointerEventNoHapTokenVerify(const uint8_t* data, const size_t size, size_t& startPos)
{
    ShiftWindowParam param;
    bool autoGenDown;
    startPos += GetObject<ShiftWindowParam>(param, data + startPos, size - startPos);
    startPos += GetObject<bool>(autoGenDown, data + startPos, size - startPos);
    WIN_MGR->ShiftAppPointerEvent(param, autoGenDown);
    return true;
}
		
bool ShiftAppPointerEventFuzzTest(const uint8_t* data, const size_t size)
{
    size_t startPos = 0;
    if (ShiftAppPointerEvent(data, size, startPos) && ShiftAppPointerEventNoHapTokenVerify(data, size, startPos)) {
        return true;
    }
    return false;
}
} // MMI
} // OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::MMI::ShiftAppPointerEventFuzzTest(data, size);
    return 0;
}