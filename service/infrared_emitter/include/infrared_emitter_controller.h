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

#ifndef INFRARED_EMMITTER_CONTROLLER_H
#define INFRARED_EMMITTER_CONTROLLER_H

#include "nocopyable.h"
#include "refbase.h"
#include "v1_0/consumer_ir.h"
#include <inttypes.h>
#include <vector>

namespace OHOS {
namespace MMI {
struct InfraredFrequencyInfo {
    int64_t max_ { 0 };
    int64_t min_ { 0 };
};
class InfraredEmitterController {
public:
    /**
     * @brief Obtains an <b>InfraredEmitterController</b> instance.
     * @return Returns the pointer to the <b>InfraredEmitterController</b> instance.
     * @since 12
     */
    static InfraredEmitterController *GetInstance();
    virtual ~InfraredEmitterController();
    sptr<OHOS::HDI::Consumerir::V1_0::ConsumerIr> InitInfraredEmitter();
    bool Transmit(int64_t carrierFreq, const std::vector<int64_t> pattern);
    bool GetFrequencies(std::vector<InfraredFrequencyInfo> &frequencyInfo);

private:
    InfraredEmitterController();
    DISALLOW_COPY_AND_MOVE(InfraredEmitterController);
    static InfraredEmitterController *instance_;
};
}
}
#endif // INFRARED_EMMITTER_CONTROLLER_H