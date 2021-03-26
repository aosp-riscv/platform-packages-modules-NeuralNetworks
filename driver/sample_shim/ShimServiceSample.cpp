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

#define LOG_TAG "ShimServiceSample"

#include <android-base/logging.h>
#include <dlfcn.h>
#include "NeuralNetworksShim.h"
#include "SupportLibrarySymbols.h"

int main() {
    NnApiSLDriverImpl* impl = ANeuralNetworks_getSLDriverImpl();
    if (impl == nullptr) {
        LOG(ERROR) << "ANeuralNetworks_getSLDriverImpl returned nullptr";
        return EXIT_FAILURE;
    }

    constexpr int operandsCount = ANEURALNETWORKS_MODEL;
    ANeuralNetworksShimOperandPerformanceInfo operandPerformance[operandsCount];
    for (int i = 0; i < operandsCount; ++i) {
        operandPerformance[i].operandType = i;
        operandPerformance[i].performanceInfo = {.execTime = 0.5, .powerUsage = 0.5};
    };

    ANeuralNetworksShimDeviceInfo* deviceInfo;
    ANeuralNetworksShimDeviceInfo_create(&deviceInfo, "nnapi-sample_sl");
    ANeuralNetworksShimDeviceInfo_setNumberOfCacheFilesNeeded(deviceInfo, 0, 0);
    ANeuralNetworksShimDeviceInfo_setCapabilities(
            deviceInfo, ANeuralNetworksShimPerformanceInfo{.execTime = 0.5, .powerUsage = 0.5},
            ANeuralNetworksShimPerformanceInfo{.execTime = 0.5, .powerUsage = 0.5},
            ANeuralNetworksShimPerformanceInfo{.execTime = 0.5, .powerUsage = 0.5},
            ANeuralNetworksShimPerformanceInfo{.execTime = 0.5, .powerUsage = 0.5}, operandsCount,
            operandPerformance);
    ANeuralNetworksShimDeviceInfo* devices[] = {deviceInfo};
    auto result = ANeuralNetworksShim_registerSupportLibraryService(impl, devices, 1);

    LOG(ERROR) << "ANeuralNetworksShim_registerSupportLibraryService returned with error status: "
               << result;

    ANeuralNetworksShimDeviceInfo_free(deviceInfo);
    return EXIT_FAILURE;
}
