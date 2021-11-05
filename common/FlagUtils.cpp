/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "FlagUtils"

#include "FlagUtils.h"

namespace android {
namespace nn {

namespace {
int64_t getFeatureLevelFlag() {
    if (_getServerFeatureLevelFlag) {
        return _getServerFeatureLevelFlag();
    }
    return kDefaultFeatureLevelNum;
}

FeatureLevelCode flagToFeatureLevelCode(int64_t featureLevelFlag) {
    switch (featureLevelFlag) {
        case 1:
            return ANEURALNETWORKS_FEATURE_LEVEL_1;
        case 2:
            return ANEURALNETWORKS_FEATURE_LEVEL_2;
        case 3:
            return ANEURALNETWORKS_FEATURE_LEVEL_3;
        case 4:
            return ANEURALNETWORKS_FEATURE_LEVEL_4;
        case 5:
            return ANEURALNETWORKS_FEATURE_LEVEL_5;
        case 6:
            return ANEURALNETWORKS_FEATURE_LEVEL_6;
        default:
            return KDefaultFeatureLevelCode;
    }
}
}  // namespace

FeatureLevelCode queryFeatureLevel() {
    static const int64_t featureLevelFlag = getFeatureLevelFlag();
    return flagToFeatureLevelCode(featureLevelFlag);
}

}  // namespace nn
}  // namespace android