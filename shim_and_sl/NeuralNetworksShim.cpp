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

#define LOG_TAG "NeuralNetworksShim"

#include "NeuralNetworksShim.h"

#include <android-base/logging.h>
#include <nnapi/Types.h>

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "ShimDevice.h"
#include "ShimDeviceManager.h"

static_assert(offsetof(NnApiSLDriverImplFL5, base.implFeatureLevel) == 0,
              ".base.implFeatureLevel is not at offset 0 of a NnApiSLDriverImplFL5 struct");
static_assert(offsetof(NnApiSLDriverImpl, implFeatureLevel) == 0,
              ".implFeatureLevel is not at offset 0 of a NnApiSLDriverImpl struct");

static_assert(sizeof(ANeuralNetworksShimPerformanceInfo) == sizeof(float) * 2,
              "ANeuralNetworksShimPerformanceInfo size changed");
static_assert(sizeof(ANeuralNetworksShimOperandPerformanceInfo) ==
                      sizeof(float) * 2 + sizeof(int32_t),
              "ANeuralNetworksShimOperandPerformanceInfo size changed");
static_assert(sizeof(ANeuralNetworksShimExtensionOperandTypeInformation) == 8,
              "ANeuralNetworksShimExtensionOperandTypeInformation size changed");

static_assert(sizeof(NnApiSLDriverImpl) == sizeof(int64_t), "NnApiSLDriverImpl size changed");

// 71 real ones and 1 synthetic placeholder to algin to 8 bytes on 32 bit archs
static_assert(sizeof(NnApiSLDriverImplFL5) == sizeof(int64_t) + 72 * sizeof(void*),
              "NnApiSLDriverImplFL5 size changed");

static_assert(ANNSHIM_NO_ERROR == 0, "ANNSHIM_NO_ERROR has changed");
static_assert(ANNSHIM_FAILED_TO_LOAD_SL == 1, "ANNSHIM_FAILED_TO_LOAD_SL has changed");
static_assert(ANNSHIM_FAILED_TO_REGISTER_SERVICE == 2,
              "ANNSHIM_FAILED_TO_REGISTER_SERVICE has changed");
static_assert(ANNSHIM_GENERAL_ERROR == 3, "ANNSHIM_GENERAL_ERROR has changed");
static_assert(ANNSHIM_INVALID_ARGUMENT == 4, "ANNSHIM_INVALID_ARGUMENT has changed");

using namespace aidl::android::hardware::neuralnetworks;
using android::neuralnetworks::shim::registerDevices;
using android::neuralnetworks::shim::RegistrationParams;

int ANeuralNetworksShim_registerSupportLibraryService(
        const ANeuralNetworksShimRegistrationParams* registrationParams) {
    if (registrationParams == nullptr) {
        LOG(ERROR) << "Invalid arguments, registrationParams == nullptr ";
        return ANNSHIM_INVALID_ARGUMENT;
    }
    const auto* params = reinterpret_cast<const RegistrationParams*>(registrationParams);

    NnApiSLDriverImpl* const nnapiImpl = params->nnapiSupportLibraryPackage;
    const auto& deviceInfos = params->deviceInfos;
    const uint32_t numberOfListenerThreads = params->numberOfListenerThreads;
    const bool registerAsLazyService = params->registerAsLazyService;
    const bool fallbackToMinimumSupportDevice = params->fallbackToMinimumSupportDevice;

    return static_cast<int>(registerDevices(nnapiImpl, deviceInfos, numberOfListenerThreads,
                                            registerAsLazyService, fallbackToMinimumSupportDevice));
}

int ANeuralNetworksShimDeviceInfo_create(ANeuralNetworksShimDeviceInfo** deviceInfo,
                                         const char* deviceName, const char* serviceName) {
    if (deviceInfo != nullptr) {
        *deviceInfo = nullptr;
    }

    if (deviceName == nullptr) {
        LOG(ERROR) << "Invalid arguments, deviceName passed a nullptr";
        return ANNSHIM_INVALID_ARGUMENT;
    }

    auto result = new (std::nothrow)
            ShimDeviceInfo{.deviceName = std::string(deviceName),
                           .serviceName = (serviceName == nullptr || strlen(serviceName) == 0)
                                                  ? std::string(deviceName)
                                                  : std::string(serviceName)};
    if (result == nullptr) {
        return ANNSHIM_GENERAL_ERROR;
    }
    *deviceInfo = reinterpret_cast<ANeuralNetworksShimDeviceInfo*>(result);
    return ANNSHIM_NO_ERROR;
}

void ANeuralNetworksShimDeviceInfo_free(ANeuralNetworksShimDeviceInfo* deviceInfo) {
    delete reinterpret_cast<ShimDeviceInfo*>(deviceInfo);
}

int ANeuralNetworksShimDeviceInfo_setCapabilities(
        ANeuralNetworksShimDeviceInfo* deviceInfo, ANeuralNetworksShimPerformanceInfo ifPerformance,
        ANeuralNetworksShimPerformanceInfo whilePerformance,
        ANeuralNetworksShimPerformanceInfo relaxedFloat32toFloat16PerformanceScalar,
        ANeuralNetworksShimPerformanceInfo relaxedFloat32toFloat16PerformanceTensor,
        uint32_t operandPerformanceCount,
        const ANeuralNetworksShimOperandPerformanceInfo operandPerformance[]) {
    if (deviceInfo == nullptr) {
        LOG(ERROR) << "ANeuralNetworksShimDeviceInfo_setOperandsPerformance passed a nullptr";
        return ANNSHIM_INVALID_ARGUMENT;
    }

    if (operandPerformance == nullptr && operandPerformanceCount > 0) {
        LOG(ERROR) << "ANeuralNetworksShimDeviceInfo_setOperandsPerformance passed a "
                   << "operandPerformance == nullptr && operandPerformanceCount > 0";
        return ANNSHIM_INVALID_ARGUMENT;
    }

    int32_t prevOperandType = std::numeric_limits<int32_t>::min();
    for (uint32_t i = 0; i < operandPerformanceCount; ++i) {
        if (operandPerformance[i].operandType <= prevOperandType) {
            LOG(ERROR) << "ANeuralNetworksShimDeviceInfo_setOperandsPerformance passed a "
                       << "unsorted operandPerformance. Entry with index " << i << " uses "
                       << "operandType " << operandPerformance[i].operandType << " previous "
                       << "operandType is " << prevOperandType;
            return ANNSHIM_INVALID_ARGUMENT;
        }
        if (operandPerformance[i].operandType == ANEURALNETWORKS_MODEL) {
            LOG(ERROR) << "ANeuralNetworksShimDeviceInfo_setOperandsPerformance passed a "
                       << "operandPerformance with ANEURALNETWORKS_MODEL entry.";
            return ANNSHIM_INVALID_ARGUMENT;
        }
        prevOperandType = operandPerformance[i].operandType;
    }

    auto shimDeviceInfo = reinterpret_cast<ShimDeviceInfo*>(deviceInfo);
    shimDeviceInfo->capabilities.relaxedFloat32toFloat16PerformanceTensor =
            relaxedFloat32toFloat16PerformanceTensor;
    shimDeviceInfo->capabilities.relaxedFloat32toFloat16PerformanceScalar =
            relaxedFloat32toFloat16PerformanceScalar;
    shimDeviceInfo->capabilities.whilePerformance = whilePerformance;
    shimDeviceInfo->capabilities.ifPerformance = ifPerformance;

    shimDeviceInfo->capabilities.operandPerformance =
            std::vector<ANeuralNetworksShimOperandPerformanceInfo>(
                    operandPerformance + 0, operandPerformance + operandPerformanceCount);

    return ANNSHIM_NO_ERROR;
}

int ANeuralNetworksShimDeviceInfo_setNumberOfCacheFilesNeeded(
        ANeuralNetworksShimDeviceInfo* deviceInfo, uint32_t numDataCacheFiles,
        uint32_t numModelCacheFiles) {
    if (deviceInfo == nullptr) {
        LOG(ERROR) << "ANeuralNetworksShimDeviceInfo_setNumberOfCacheFilesNeeded passed a nullptr";
        return ANNSHIM_INVALID_ARGUMENT;
    }
    if (numModelCacheFiles > ::android::nn::kMaxNumberOfCacheFiles ||
        numDataCacheFiles > ::android::nn::kMaxNumberOfCacheFiles) {
        LOG(ERROR) << "ANeuralNetworksShimDeviceInfo_setNumberOfCacheFilesNeeded passed number of "
                      "cache files larger then"
                   << ::android::nn::kMaxNumberOfCacheFiles;
        return ANNSHIM_INVALID_ARGUMENT;
    }
    auto shimDeviceInfo = reinterpret_cast<ShimDeviceInfo*>(deviceInfo);
    shimDeviceInfo->additionalData.numDataCacheFiles = numDataCacheFiles;
    shimDeviceInfo->additionalData.numModelCacheFiles = numModelCacheFiles;
    return ANNSHIM_NO_ERROR;
}

int ANeuralNetworksShimDeviceInfo_addVendorExtension(
        ANeuralNetworksShimDeviceInfo* deviceInfo, const char* extensionName,
        uint32_t extensionOperandTypeInformationCount,
        const ANeuralNetworksShimExtensionOperandTypeInformation
                extensionOperandTypeInformation[]) {
    if (deviceInfo == nullptr || extensionName == nullptr) {
        LOG(ERROR) << "ANeuralNetworksShimDeviceInfo_addVendorExtension passed a nullptr";
        return ANNSHIM_INVALID_ARGUMENT;
    }

    if (extensionOperandTypeInformation == nullptr && extensionOperandTypeInformationCount != 0) {
        LOG(ERROR) << "ANeuralNetworksShimDeviceInfo_addVendorExtension passed "
                   << "extensionOperandTypeInformation == nullptr "
                   << "&& extensionOperandTypeInformationCount != 0";
        return ANNSHIM_INVALID_ARGUMENT;
    }

    auto shimDeviceInfo = reinterpret_cast<ShimDeviceInfo*>(deviceInfo);

    ShimDeviceVendorExtension extension{.extensionName = std::string(extensionName),
                                        .operandTypeInformation = {}};
    extension.operandTypeInformation =
            std::vector<ANeuralNetworksShimExtensionOperandTypeInformation>(
                    extensionOperandTypeInformation + 0,
                    extensionOperandTypeInformation + extensionOperandTypeInformationCount);
    shimDeviceInfo->additionalData.vendorExtensions.push_back(std::move(extension));

    return ANNSHIM_NO_ERROR;
}

int ANeuralNetworksShimRegistrationParams_create(
        NnApiSLDriverImpl* nnapiSupportLibraryPackage,
        ANeuralNetworksShimRegistrationParams** outRegistrationParams) {
    if (outRegistrationParams != nullptr) {
        *outRegistrationParams = nullptr;
    }

    if (nnapiSupportLibraryPackage == nullptr) {
        LOG(ERROR) << "Invalid arguments, nnapiSupportLibraryPackage == nullptr ";
        return ANNSHIM_INVALID_ARGUMENT;
    }
    if (outRegistrationParams == nullptr) {
        LOG(ERROR) << "Invalid arguments, outRegistrationParams == nullptr ";
        return ANNSHIM_INVALID_ARGUMENT;
    }

    auto result = new (std::nothrow) RegistrationParams{
            .nnapiSupportLibraryPackage = nnapiSupportLibraryPackage,
            .registerAsLazyService = false,
            .fallbackToMinimumSupportDevice = false,
    };
    if (result == nullptr) {
        return ANNSHIM_GENERAL_ERROR;
    }
    *outRegistrationParams = reinterpret_cast<ANeuralNetworksShimRegistrationParams*>(result);
    return ANNSHIM_NO_ERROR;
}

void ANeuralNetworksShimRegistrationParams_free(
        ANeuralNetworksShimRegistrationParams* registrationParams) {
    delete reinterpret_cast<RegistrationParams*>(registrationParams);
}

int ANeuralNetworksShimRegistrationParams_addDeviceInfo(
        ANeuralNetworksShimRegistrationParams* registrationParams,
        const ANeuralNetworksShimDeviceInfo* deviceInfo) {
    if (registrationParams == nullptr) {
        LOG(ERROR) << "Invalid arguments, registrationParams == nullptr";
        return ANNSHIM_INVALID_ARGUMENT;
    }
    if (deviceInfo == nullptr) {
        LOG(ERROR) << "Invalid arguments, deviceInfo == nullptr";
        return ANNSHIM_INVALID_ARGUMENT;
    }

    auto params = reinterpret_cast<RegistrationParams*>(registrationParams);
    auto info = reinterpret_cast<const ShimDeviceInfo*>(deviceInfo);
    params->deviceInfos.push_back(*info);
    return ANNSHIM_NO_ERROR;
}

int ANeuralNetworksShimRegistrationParams_setNumberOfListenerThreads(
        ANeuralNetworksShimRegistrationParams* registrationParams,
        uint32_t numberOfListenerThreads) {
    if (registrationParams == nullptr) {
        LOG(ERROR) << "Invalid arguments, registrationParams == nullptr";
        return ANNSHIM_INVALID_ARGUMENT;
    }
    if (registrationParams == 0) {
        LOG(ERROR) << "Invalid arguments, numberOfListenerThreads == 0";
        return ANNSHIM_INVALID_ARGUMENT;
    }
    auto params = reinterpret_cast<RegistrationParams*>(registrationParams);
    params->numberOfListenerThreads = numberOfListenerThreads;
    return ANNSHIM_NO_ERROR;
}

int ANeuralNetworksShimRegistrationParams_registerAsLazyService(
        ANeuralNetworksShimRegistrationParams* registrationParams, bool asLazy) {
    if (registrationParams == nullptr) {
        LOG(ERROR) << "Invalid arguments, registrationParams == nullptr";
        return ANNSHIM_INVALID_ARGUMENT;
    }
    auto params = reinterpret_cast<RegistrationParams*>(registrationParams);
    params->registerAsLazyService = asLazy;
    return ANNSHIM_NO_ERROR;
}

int ANeuralNetworksShimRegistrationParams_fallbackToMinimumSupportDevice(
        ANeuralNetworksShimRegistrationParams* registrationParams, bool fallback) {
    if (registrationParams == nullptr) {
        LOG(ERROR) << "Invalid arguments, registrationParams == nullptr";
        return ANNSHIM_INVALID_ARGUMENT;
    }
    auto params = reinterpret_cast<RegistrationParams*>(registrationParams);
    params->fallbackToMinimumSupportDevice = fallback;
    return ANNSHIM_NO_ERROR;
}
