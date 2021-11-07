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

#ifndef ANDROID_FRAMEWORKS_ML_NN_RUNTIME_FEATURE_LEVEL_H
#define ANDROID_FRAMEWORKS_ML_NN_RUNTIME_FEATURE_LEVEL_H

#include "NeuralNetworks.h"

#ifdef NN_EXPERIMENTAL_FEATURE
#include "NeuralNetworksExperimentalFeatures.h"
#endif  // NN_EXPERIMENTAL_FEATURE

namespace android {
namespace nn {

// TODO(b/201399117): Set this value based on feature level flag.
// The current feature level of the NNAPI Runtime
#ifdef NN_EXPERIMENTAL_FEATURE
constexpr int64_t kCurrentNNAPIRuntimeFeatureLevel = ANEURALNETWORKS_FEATURE_LEVEL_EXPERIMENTAL;
#else   // NN_EXPERIMENTAL_FEATURE
constexpr int64_t kCurrentNNAPIRuntimeFeatureLevel = ANEURALNETWORKS_FEATURE_LEVEL_6;
#endif  // NN_EXPERIMENTAL_FEATURE

// The current version of the NNAPI APEX module.
// Keep this value in sync with packages/modules/NeuralNetworks/apex/manifest.json.
constexpr int64_t kNnapiApexVersion = 319999900;

}  // namespace nn
}  // namespace android

#endif  // ANDROID_FRAMEWORKS_ML_NN_RUNTIME_FEATURE_LEVEL_H
