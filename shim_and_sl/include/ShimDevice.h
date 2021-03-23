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

#pragma once

#include <aidl/android/hardware/neuralnetworks/BnBuffer.h>
#include <aidl/android/hardware/neuralnetworks/BnDevice.h>
#include <android/binder_auto_utils.h>

#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "NeuralNetworksShim.h"
#include "ShimBufferTracker.h"
#include "SupportLibrary.h"
#include "SupportLibraryWrapper.h"

#include <android-base/macros.h>
#include <android-base/thread_annotations.h>

namespace aidl::android::hardware::neuralnetworks {

/**
 * Information about an NNAPI Device capabilities.
 * See NNAPI HAL IDevice::getCapabilities for more information.
 */
struct ShimDeviceCapabilities {
    std::vector<ANeuralNetworksShimOperandPerformanceInfo> operandPerformance;
    ANeuralNetworksShimPerformanceInfo ifPerformance{0.0f, 0.0f};
    ANeuralNetworksShimPerformanceInfo whilePerformance{0.0f, 0.0f};
    ANeuralNetworksShimPerformanceInfo relaxedFloat32toFloat16PerformanceScalar{0.0f, 0.0f};
    ANeuralNetworksShimPerformanceInfo relaxedFloat32toFloat16PerformanceTensor{0.0f, 0.0f};
};

/**
 * Information about an NNAPI Device Vendor extension.
 * See NNAPI HAL IDevice::getSupportedExtensions for more information.
 */
struct ShimDeviceVendorExtension {
    std::string extensionName;
    std::vector<ANeuralNetworksShimExtensionOperandTypeInformation> operandTypeInformation;
};

/**
 * Supplementary information needed for NNAPI service implementation.
 * See NNAPI HAL IDevice::getNumberOfCacheFilesNeeded for more information.
 */
struct ShimDeviceAdditionalData {
    uint32_t numDataCacheFiles = 0;
    uint32_t numModelCacheFiles = 0;

    std::vector<ShimDeviceVendorExtension> vendorExtensions;
};

/**
 * Information about an NNAPI Device to register.
 */
struct ShimDeviceInfo {
    /**
     * Name of the target device, as returned by SL ANeuralNetworksDevice_getName
     */
    std::string deviceName;

    /**
     * Capabilities information, to be returned from NNAPI HAL.
     * See NNAPI HAL IDevice::getCapabilities for more information.
     */
    ShimDeviceCapabilities capabilities;

    ShimDeviceAdditionalData additionalData;
};

class ShimDevice : public BnDevice {
   public:
    ShimDevice(std::shared_ptr<const NnApiSupportLibrary>, ANeuralNetworksDevice*,
               const ShimDeviceInfo& deviceInfo);
    ::ndk::ScopedAStatus allocate(const BufferDesc& desc,
                                  const std::vector<IPreparedModelParcel>& preparedModels,
                                  const std::vector<BufferRole>& inputRoles,
                                  const std::vector<BufferRole>& outputRoles,
                                  DeviceBuffer* deviceBuffer) override;
    ::ndk::ScopedAStatus getCapabilities(Capabilities* capabilities) override;
    ::ndk::ScopedAStatus getNumberOfCacheFilesNeeded(
            NumberOfCacheFiles* numberOfCacheFiles) override;
    ::ndk::ScopedAStatus getSupportedExtensions(std::vector<Extension>* extensions) override;
    ::ndk::ScopedAStatus getSupportedOperations(const Model& model,
                                                std::vector<bool>* supportedOperations) override;
    ::ndk::ScopedAStatus getType(DeviceType* deviceType) override;
    ::ndk::ScopedAStatus getVersionString(std::string* versionString) override;
    ::ndk::ScopedAStatus prepareModel(
            const Model& model, ExecutionPreference preference, Priority priority, int64_t deadline,
            const std::vector<::ndk::ScopedFileDescriptor>& modelCache,
            const std::vector<::ndk::ScopedFileDescriptor>& dataCache,
            const std::vector<uint8_t>& token,
            const std::shared_ptr<IPreparedModelCallback>& callback) override;
    ::ndk::ScopedAStatus prepareModelFromCache(
            int64_t deadline, const std::vector<::ndk::ScopedFileDescriptor>& modelCache,
            const std::vector<::ndk::ScopedFileDescriptor>& dataCache,
            const std::vector<uint8_t>& token,
            const std::shared_ptr<IPreparedModelCallback>& callback) override;
    int registerService();

   private:
    std::shared_ptr<const NnApiSupportLibrary> mNnapi;
    std::shared_ptr<ShimBufferTracker> mBufferTracker;
    ANeuralNetworksDevice* mDevice;
    ShimDeviceAdditionalData mDeviceAdditionalData;
    Capabilities mCapabilities;
};

}  // namespace aidl::android::hardware::neuralnetworks
