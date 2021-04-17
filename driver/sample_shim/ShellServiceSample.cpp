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
#include <aidl/android/hardware/neuralnetworks/Capabilities.h>
#include <aidl/android/hardware/neuralnetworks/Extension.h>
#include <aidl/android/hardware/neuralnetworks/ExtensionOperandTypeInformation.h>
#include <aidl/android/hardware/neuralnetworks/NumberOfCacheFiles.h>
#include <aidl/android/hardware/neuralnetworks/OperandType.h>
#include <android-base/logging.h>
#include <android-base/scopeguard.h>
#include <android/binder_enums.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <dlfcn.h>
#include <nnapi/hal/aidl/InvalidDevice.h>

#include <algorithm>
#include <limits>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

typedef struct NnApiSLDriverImpl NnApiSLDriverImpl;

namespace aidl::android::hardware::neuralnetworks {
namespace {

constexpr auto kPerf = PerformanceInfo{.execTime = 0.5f, .powerUsage = 0.5f};

struct Names {
    std::string driverName;
    std::string serviceName;
};

struct Metadata {
    Capabilities capabilities;
    NumberOfCacheFiles numberOfCacheFiles;
    std::vector<Extension> vendorExtensions;
};

Metadata generateFakeMetadata() {
    Metadata metadata;

    std::vector<OperandPerformance> operandPerformance;
    constexpr auto operandTypes = ndk::enum_range<OperandType>{};
    operandPerformance.reserve(std::distance(operandTypes.begin(), operandTypes.end()));
    for (auto operandType : operandTypes) {
        if (operandType != OperandType::SUBGRAPH) {
            operandPerformance.push_back(OperandPerformance{.type = operandType, .info = kPerf});
        }
    }
    std::sort(operandPerformance.begin(), operandPerformance.end(),
              [](const OperandPerformance& lhs, const OperandPerformance& rhs) {
                  return lhs.type < rhs.type;
              });

    metadata.capabilities = Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = kPerf,
            .relaxedFloat32toFloat16PerformanceTensor = kPerf,
            .operandPerformance = std::move(operandPerformance),
            .ifPerformance = kPerf,
            .whilePerformance = kPerf,
    };

    metadata.numberOfCacheFiles = NumberOfCacheFiles{.numModelCache = 0, .numDataCache = 0};

    metadata.vendorExtensions = {};

    return metadata;
}

std::unordered_map<std::string, Metadata> readMetadataTable(const std::string& /*metadataPath*/) {
    // TODO: load this from a text file that resides alongside the support library.
    std::unordered_map<std::string, Metadata> metadata;
    metadata.emplace("nnapi-sample_sl", generateFakeMetadata());
    return metadata;
}

void registerInvalidDevices(const std::vector<Names>& names) {
    for (const auto& [_, name] : names) {
        const auto invalidDevice = InvalidDevice::create();
        const std::string instance = std::string(IDevice::descriptor) + "/" + name;
        LOG(INFO) << "Attempting service registration for " << instance;
        const auto status = AServiceManager_registerLazyService(invalidDevice->asBinder().get(),
                                                                instance.c_str());
        if (status != STATUS_OK) {
            LOG(ERROR) << "AServiceManager_registerLazyService failed for " << name
                       << ", error code " << status;
            return;
        }
    }
    ABinderProcess_setThreadPoolMaxThreadCount(15);
    ABinderProcess_joinThreadPool();
}

ANeuralNetworksShimPerformanceInfo convert(const PerformanceInfo& performanceInfo) {
    return {.execTime = performanceInfo.execTime, .powerUsage = performanceInfo.powerUsage};
}

ANeuralNetworksShimOperandPerformanceInfo convert(const OperandPerformance& operandPerformance) {
    return {.operandType = static_cast<int32_t>(operandPerformance.type),
            .performanceInfo = convert(operandPerformance.info)};
}

std::vector<ANeuralNetworksShimOperandPerformanceInfo> convert(
        const std::vector<OperandPerformance>& operandPerformances) {
    std::vector<ANeuralNetworksShimOperandPerformanceInfo> result;
    result.reserve(operandPerformances.size());
    std::transform(operandPerformances.begin(), operandPerformances.end(),
                   std::back_inserter(result),
                   [](const OperandPerformance& opPerf) { return convert(opPerf); });
    return result;
}

ANeuralNetworksShimExtensionOperandTypeInformation convert(
        const ExtensionOperandTypeInformation& opTypeInfo) {
    return {
            .type = opTypeInfo.type,
            .byteSize = static_cast<uint32_t>(opTypeInfo.byteSize),
            .isTensor = opTypeInfo.isTensor,
    };
}

std::vector<ANeuralNetworksShimExtensionOperandTypeInformation> convert(
        const std::vector<ExtensionOperandTypeInformation>& opTypeInfos) {
    std::vector<ANeuralNetworksShimExtensionOperandTypeInformation> result;
    result.reserve(opTypeInfos.size());
    std::transform(
            opTypeInfos.begin(), opTypeInfos.end(), std::back_inserter(result),
            [](const ExtensionOperandTypeInformation& opTypeInfo) { return convert(opTypeInfo); });
    return result;
}

void addDeviceInfoToParams(ANeuralNetworksShimRegistrationParams* params, const Names& names,
                           const Metadata& metadata) {
    const auto& capabilities = metadata.capabilities;
    const auto& numberOfCacheFiles = metadata.numberOfCacheFiles;
    const auto& vendorExtensions = metadata.vendorExtensions;

    const auto relaxedFloat32toFloat16PerformanceScalar =
            convert(capabilities.relaxedFloat32toFloat16PerformanceScalar);
    const auto relaxedFloat32toFloat16PerformanceTensor =
            convert(capabilities.relaxedFloat32toFloat16PerformanceTensor);
    const auto operandPerformance = convert(capabilities.operandPerformance);
    const auto ifPerformance = convert(capabilities.ifPerformance);
    const auto whilePerformance = convert(capabilities.whilePerformance);

    ANeuralNetworksShimDeviceInfo* deviceInfo;
    ANeuralNetworksShimDeviceInfo_create(&deviceInfo, names.driverName.c_str(),
                                         names.serviceName.c_str());
    const auto guardDeviceInfo = ::android::base::make_scope_guard(
            [deviceInfo] { ANeuralNetworksShimDeviceInfo_free(deviceInfo); });

    ANeuralNetworksShimDeviceInfo_setNumberOfCacheFilesNeeded(
            deviceInfo, numberOfCacheFiles.numModelCache, numberOfCacheFiles.numDataCache);
    ANeuralNetworksShimDeviceInfo_setCapabilities(
            deviceInfo, ifPerformance, whilePerformance, relaxedFloat32toFloat16PerformanceScalar,
            relaxedFloat32toFloat16PerformanceTensor, operandPerformance.size(),
            operandPerformance.data());
    for (const auto& vendorExtension : vendorExtensions) {
        const auto opTypeInfo = convert(vendorExtension.operandTypes);
        ANeuralNetworksShimDeviceInfo_addVendorExtension(deviceInfo, vendorExtension.name.c_str(),
                                                         opTypeInfo.size(), opTypeInfo.data());
    }

    ANeuralNetworksShimRegistrationParams_addDeviceInfo(params, deviceInfo);
}

int registerDevices(const std::string& driverPath, const std::string& metadataPath,
                    const std::vector<Names>& devices) {
    // Load support library.
    void* libHandle = dlopen(driverPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (libHandle == nullptr) {
        LOG(ERROR) << "Failed to load sample SL driver: " << driverPath;
        registerInvalidDevices(devices);
        return EXIT_FAILURE;
    }

    // Load support library entry point.
    using GetSlDriverImplFn = std::add_pointer_t<NnApiSLDriverImpl*()>;
    GetSlDriverImplFn getSlDriverImpl = reinterpret_cast<GetSlDriverImplFn>(
            dlsym(libHandle, "ANeuralNetworks_getSLDriverImpl"));
    if (getSlDriverImpl == nullptr) {
        LOG(ERROR) << "Failed to find ANeuralNetworks_getSLDriverImpl symbol in: " << driverPath;
        registerInvalidDevices(devices);
        return EXIT_FAILURE;
    }

    // Call support library entry point to obtain functionality.
    NnApiSLDriverImpl* impl = getSlDriverImpl();
    if (impl == nullptr) {
        LOG(ERROR) << "ANeuralNetworks_getSLDriverImpl returned nullptr: " << driverPath;
        registerInvalidDevices(devices);
        return EXIT_FAILURE;
    }

    // Populate metadata information.
    const auto metadataTable = readMetadataTable(metadataPath);

    // Ensure all devices to be registered are included in the metadata table.
    for (const auto& device : devices) {
        if (metadataTable.count(device.driverName) == 0) {
            registerInvalidDevices(devices);
            return EXIT_FAILURE;
        }
    }

    ANeuralNetworksShimRegistrationParams* params;
    ANeuralNetworksShimRegistrationParams_create(impl, &params);
    const auto guardParams = ::android::base::make_scope_guard(
            [params] { ANeuralNetworksShimRegistrationParams_free(params); });

    // The default is 15, use more only if there's more devices exposed.
    ANeuralNetworksShimRegistrationParams_setNumberOfListenerThreads(params, 15);
    ANeuralNetworksShimRegistrationParams_registerAsLazyService(params, /*asLazy=*/true);
    ANeuralNetworksShimRegistrationParams_fallbackToMinimumSupportDevice(params, /*fallback=*/true);

    for (const auto& device : devices) {
        const auto& metadata = metadataTable.at(device.driverName);
        addDeviceInfoToParams(params, device, metadata);
    }

    // Register the support library as a binderized AIDL service.
    auto result = ANeuralNetworksShim_registerSupportLibraryService(params);
    LOG(ERROR) << "ANeuralNetworksShim_registerSupportLibraryService returned with error status: "
               << result;

    return EXIT_FAILURE;
}

}  // namespace
}  // namespace aidl::android::hardware::neuralnetworks

using aidl::android::hardware::neuralnetworks::Names;
using aidl::android::hardware::neuralnetworks::registerDevices;

int main() {
    const std::string driverPath = "/vendor/lib64/neuralnetworks_sample_sl_driver_prebuilt.so";

    // TODO: use a real path when loading the configuration is implemented.
    const std::string metadataPath = "/path/to/support/library/package/config.xml";

    const std::vector<Names> devicesToRegister = {
            {.driverName = "nnapi-sample_sl", .serviceName = "nnapi-sample_sl_updatable"},
    };

    return registerDevices(driverPath, metadataPath, devicesToRegister);
}
