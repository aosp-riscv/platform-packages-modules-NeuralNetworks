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

#ifndef ANDROID_FRAMEWORKS_ML_NN_COMMON_FLAG_UTILS_H
#define ANDROID_FRAMEWORKS_ML_NN_COMMON_FLAG_UTILS_H

#include <stdint.h>

#include "NeuralNetworks.h"

namespace android {
namespace nn {

// Keep these values consistent with server side configuration in
// google3/googledata/experiments/mobile/android_platform/nnapi_native/features/feature_level.gcl.
constexpr char kExprCategoryName[] = "nnapi_native";
constexpr char kCurrentFeatureLevelFlagName[] = "current_feature_level";
constexpr int64_t kDefaultFeatureLevelNum = 5;
constexpr FeatureLevelCode KDefaultFeatureLevelCode = ANEURALNETWORKS_FEATURE_LEVEL_5;
constexpr int64_t kMinFeatureLevelNum = 5;
constexpr int64_t kMaxFeatureLevelNum = 6;

// Weak symbol to get server feature level flag so that other targets with different build options
// (e.g. not vendor available) can implement this function.
// Note that this function should NOT be used directly and may not be present in the final artifact.
// Clients are expected to use queryFeatureLevel instead.
int64_t _getServerFeatureLevelFlag() __attribute__((weak));

// Queries system flag for the current feature level.
FeatureLevelCode queryFeatureLevel();

}  // namespace nn
}  // namespace android

#endif  // ANDROID_FRAMEWORKS_ML_NN_COMMON_FLAG_UTILS_H