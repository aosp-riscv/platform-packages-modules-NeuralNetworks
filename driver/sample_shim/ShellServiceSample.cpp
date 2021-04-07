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

#define LOG_TAG "ShellServiceSample"

#include <NeuralNetworksShim.h>
#include <aidl/android/hardware/neuralnetworks/BnDevice.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <dlfcn.h>

#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "InvalidInterfaces.h"

typedef struct NnApiSLDriverImpl NnApiSLDriverImpl;

static const char* const kServiceName = "nnapi-sample_sl_updatable";

namespace aidl::android::hardware::neuralnetworks {

void registerInvalidDevice(const std::string& name) {
    constexpr auto perf = PerformanceInfo{
            .execTime = std::numeric_limits<float>::max(),
            .powerUsage = std::numeric_limits<float>::max(),
    };
    auto capabilities = Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = perf,
            .relaxedFloat32toFloat16PerformanceTensor = perf,
            .operandPerformance = {},
            .ifPerformance = perf,
            .whilePerformance = perf,
    };
    constexpr auto numberOfCacheFiles = NumberOfCacheFiles{
            .numModelCache = 0,
            .numDataCache = 0,
    };
    std::vector<Extension> extensions{};
    constexpr auto deviceType = DeviceType::OTHER;
    std::string versionString = "example";

    const auto invalidDevice = ndk::SharedRefBase::make<InvalidDevice>(
            std::move(capabilities), numberOfCacheFiles, std::move(extensions), deviceType,
            std::move(versionString));

    const std::string instance = std::string(IDevice::descriptor) + "/" + name;
    LOG(INFO) << "Attempting service registration for " << instance;
    const binder_status_t status =
            AServiceManager_addService(invalidDevice->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
        LOG(ERROR) << "AServiceManager_addService failed for " << instance << ", error code "
                   << status;
        return;
    }
    ABinderProcess_setThreadPoolMaxThreadCount(4);
    ABinderProcess_joinThreadPool();
}

}  // namespace aidl::android::hardware::neuralnetworks

int main() {
    const char* driver_path = "/vendor/lib64/neuralnetworks_sample_sl_driver_prebuilt.so";
    // const char* driver_path =
    //         "/vendor/lib64/neuralnetworks_sample_sl_driver_invalid_name_prebuilt.so";

    // Load support library.
    void* lib_handle = dlopen(driver_path, RTLD_LAZY | RTLD_LOCAL);
    if (lib_handle == nullptr) {
        LOG(ERROR) << "Failed to load sample SL driver: " << driver_path;
        aidl::android::hardware::neuralnetworks::registerInvalidDevice(kServiceName);
        return EXIT_FAILURE;
    }

    // Load support library entry point.
    using GetSlDriverImplFn = std::add_pointer_t<NnApiSLDriverImpl*()>;
    GetSlDriverImplFn getSlDriverImpl = reinterpret_cast<GetSlDriverImplFn>(
            dlsym(lib_handle, "ANeuralNetworks_getSLDriverImpl"));
    if (getSlDriverImpl == nullptr) {
        LOG(ERROR) << "Failed to find ANeuralNetworks_getSLDriverImpl symbol in: " << driver_path;
        aidl::android::hardware::neuralnetworks::registerInvalidDevice(kServiceName);
        return EXIT_FAILURE;
    }

    // Call support library entry point to obtain functionality.
    NnApiSLDriverImpl* impl = getSlDriverImpl();
    if (impl == nullptr) {
        LOG(ERROR) << "ANeuralNetworks_getSLDriverImpl returned nullptr: " << driver_path;
        aidl::android::hardware::neuralnetworks::registerInvalidDevice(kServiceName);
        return EXIT_FAILURE;
    }

    // Populate metadata information.
    // TODO: load this from a text file that resides alongside the support library.

    constexpr int operandsCount = ANEURALNETWORKS_MODEL;
    ANeuralNetworksShimOperandPerformanceInfo operandPerformance[operandsCount];
    for (int i = 0; i < operandsCount; ++i) {
        operandPerformance[i].operandType = i;
        operandPerformance[i].performanceInfo = {.execTime = 0.5, .powerUsage = 0.5};
    };

    ANeuralNetworksShimDeviceInfo* deviceInfo;
    ANeuralNetworksShimDeviceInfo_create(&deviceInfo,
                                         /*deviceName=*/"nnapi-sample_sl",
                                         /*serviceName=*/kServiceName);
    ANeuralNetworksShimDeviceInfo_setNumberOfCacheFilesNeeded(deviceInfo, 0, 0);
    ANeuralNetworksShimDeviceInfo_setCapabilities(
            deviceInfo, ANeuralNetworksShimPerformanceInfo{.execTime = 0.5, .powerUsage = 0.5},
            ANeuralNetworksShimPerformanceInfo{.execTime = 0.5, .powerUsage = 0.5},
            ANeuralNetworksShimPerformanceInfo{.execTime = 0.5, .powerUsage = 0.5},
            ANeuralNetworksShimPerformanceInfo{.execTime = 0.5, .powerUsage = 0.5}, operandsCount,
            operandPerformance);
    ANeuralNetworksShimDeviceInfo* devices[] = {deviceInfo};

    // Register the support library as a binderized AIDL service.
    auto result = ANeuralNetworksShim_registerSupportLibraryService(impl, devices, 1);
    LOG(ERROR) << "ANeuralNetworksShim_registerSupportLibraryService returned with error status: "
               << result;

    ANeuralNetworksShimDeviceInfo_free(deviceInfo);
    return EXIT_FAILURE;
}
