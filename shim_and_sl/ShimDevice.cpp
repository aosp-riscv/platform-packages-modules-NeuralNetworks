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

#define LOG_TAG "ShimDevice"

#include "ShimDevice.h"

#include <NeuralNetworks.h>
#include <aidl/android/hardware/neuralnetworks/DataLocation.h>
#include <aidl/android/hardware/neuralnetworks/ErrorStatus.h>
#include <aidl/android/hardware/neuralnetworks/Extension.h>
#include <aidl/android/hardware/neuralnetworks/ExtensionOperandTypeInformation.h>
#include <aidl/android/hardware/neuralnetworks/Memory.h>
#include <aidl/android/hardware/neuralnetworks/NumberOfCacheFiles.h>
#include <aidl/android/hardware/neuralnetworks/OperandLifeTime.h>
#include <aidl/android/hardware/neuralnetworks/OperandPerformance.h>
#include <android-base/logging.h>
#include <android-base/scopeguard.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/hal/aidl/Conversions.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "ShimConverter.h"
#include "ShimPreparedModel.h"
#include "ShimUtils.h"
#include "SupportLibrary.h"

using namespace ::android::nn::sl_wrapper;

namespace aidl::android::hardware::neuralnetworks {

namespace {

constexpr std::optional<::android::nn::wrapper::ExecutePriority> convertToNDKPriority(
        Priority priority) {
    switch (priority) {
        case Priority::LOW:
            return ::android::nn::wrapper::ExecutePriority::LOW;
        case Priority::MEDIUM:
            return ::android::nn::wrapper::ExecutePriority::MEDIUM;
        case Priority::HIGH:
            return ::android::nn::wrapper::ExecutePriority::HIGH;
    }
    LOG(ERROR) << "unrecognized priority: " << static_cast<int32_t>(priority);
    return std::nullopt;
}

constexpr std::optional<::android::nn::wrapper::ExecutePreference> convertToNDKPreference(
        ExecutionPreference preference) {
    switch (preference) {
        case ExecutionPreference::LOW_POWER:
            return ::android::nn::wrapper::ExecutePreference::PREFER_LOW_POWER;
        case ExecutionPreference::FAST_SINGLE_ANSWER:
            return ::android::nn::wrapper::ExecutePreference::PREFER_FAST_SINGLE_ANSWER;
        case ExecutionPreference::SUSTAINED_SPEED:
            return ::android::nn::wrapper::ExecutePreference::PREFER_SUSTAINED_SPEED;
    }
    LOG(ERROR) << "unrecognized preference: " << static_cast<int32_t>(preference);
    return std::nullopt;
}

// Safely downcast an IPreparedModel object to ShimPreparedModel.
// This function will return nullptr if the IPreparedModel object is not originated from the
// shim process.
const ShimPreparedModel* castToShimPreparedModel(IPreparedModel* preparedModel) {
    if (preparedModel->isRemote()) {
        return nullptr;
    }
    // This static_cast is safe because ShimPreparedModel is the only class that implements
    // the IPreparedModel interface in the sample driver process.
    return static_cast<const ShimPreparedModel*>(preparedModel);
}

static PerformanceInfo convertPerformanceInfo(const SL_ANeuralNetworksPerformanceInfo& info) {
    return {.execTime = info.execTime, .powerUsage = info.powerUsage};
}

Capabilities getCapabilities(const NnApiSupportLibrary* nnapi, ANeuralNetworksDevice* device) {
    Capabilities capabilities;
    SL_ANeuralNetworksPerformanceInfo performanceInfo;

    nnapi->SL_ANeuralNetworksDevice_getPerformanceInfo(
            device, SL_ANEURALNETWORKS_CAPABILITIES_PERFORMANCE_RELAXED_SCALAR, &performanceInfo);
    capabilities.relaxedFloat32toFloat16PerformanceScalar = convertPerformanceInfo(performanceInfo);

    nnapi->SL_ANeuralNetworksDevice_getPerformanceInfo(
            device, SL_ANEURALNETWORKS_CAPABILITIES_PERFORMANCE_RELAXED_TENSOR, &performanceInfo);
    capabilities.relaxedFloat32toFloat16PerformanceTensor = convertPerformanceInfo(performanceInfo);

    nnapi->SL_ANeuralNetworksDevice_getPerformanceInfo(
            device, SL_ANEURALNETWORKS_CAPABILITIES_PERFORMANCE_IF, &performanceInfo);
    capabilities.ifPerformance = convertPerformanceInfo(performanceInfo);

    nnapi->SL_ANeuralNetworksDevice_getPerformanceInfo(
            device, SL_ANEURALNETWORKS_CAPABILITIES_PERFORMANCE_WHILE, &performanceInfo);
    capabilities.whilePerformance = convertPerformanceInfo(performanceInfo);

    constexpr auto fn = [](SL_ANeuralNetworksOperandPerformanceInfo info, void* context) {
        auto* out = static_cast<std::vector<OperandPerformance>*>(context);
        out->push_back(OperandPerformance{
                .type = static_cast<OperandType>(info.operandType),
                .info = convertPerformanceInfo(info.performanceInfo),
        });
    };

    nnapi->SL_ANeuralNetworksDevice_forEachOperandPerformanceInfo(
            device, static_cast<void*>(&capabilities.operandPerformance), fn);

    return capabilities;
}

NumberOfCacheFiles getNumberOfCacheFilesNeeded(const NnApiSupportLibrary* nnapi,
                                               ANeuralNetworksDevice* device) {
    uint32_t numModelCacheFiles;
    uint32_t numDataCacheFiles;
    nnapi->SL_ANeuralNetworksDevice_getNumberOfCacheFilesNeeded(device, &numModelCacheFiles,
                                                                &numDataCacheFiles);
    return {
            .numModelCache = static_cast<int32_t>(numModelCacheFiles),
            .numDataCache = static_cast<int32_t>(numDataCacheFiles),
    };
}

std::vector<Extension> getVendorExtensions(const NnApiSupportLibrary* nnapi,
                                           ANeuralNetworksDevice* device) {
    uint32_t vendorExtensionCount;
    nnapi->SL_ANeuralNetworksDevice_getVendorExtensionCount(device, &vendorExtensionCount);

    std::vector<Extension> extensions(vendorExtensionCount);

    for (uint32_t vendorExtensionIndex = 0; vendorExtensionIndex < vendorExtensionCount;
         ++vendorExtensionIndex) {
        auto& extension = extensions[vendorExtensionIndex];

        const char* extensionName;
        nnapi->SL_ANeuralNetworksDevice_getVendorExtensionName(device, vendorExtensionIndex,
                                                               &extensionName);
        extension.name = extensionName;

        constexpr auto fn = [](SL_ANeuralNetworksExtensionOperandTypeInformation info,
                               void* context) {
            auto* out = static_cast<std::vector<ExtensionOperandTypeInformation>*>(context);
            out->push_back(ExtensionOperandTypeInformation{
                    .type = info.type,
                    .isTensor = info.isTensor,
                    .byteSize = static_cast<int32_t>(info.byteSize),
            });
        };
        nnapi->SL_ANeuralNetworksDevice_forEachVendorExtensionOperandTypeInformation(
                device, vendorExtensionIndex, static_cast<void*>(&extension.operandTypes), fn);
    }

    return extensions;
}

}  // namespace

ShimDevice::ShimDevice(std::shared_ptr<const NnApiSupportLibrary> nnapi,
                       ANeuralNetworksDevice* device, std::string serviceName)
    : mNnapi(std::move(nnapi)),
      mBufferTracker(ShimBufferTracker::create()),
      mServiceName(std::move(serviceName)),
      mDevice(device),
      mCapabilities(neuralnetworks::getCapabilities(mNnapi.get(), mDevice)),
      mNumberOfCacheFiles(neuralnetworks::getNumberOfCacheFilesNeeded(mNnapi.get(), mDevice)),
      mExtensions(neuralnetworks::getVendorExtensions(mNnapi.get(), mDevice)) {}

// Manages the data buffer for an operand.
class ShimBuffer : public BnBuffer {
   public:
    ShimBuffer(const NnApiSupportLibrary* nnApi, const ::android::nn::Dimensions initialDimensions,
               const ::android::nn::OperandType type,
               std::shared_ptr<::android::nn::sl_wrapper::Memory> memory,
               std::unique_ptr<ShimBufferTracker::Token> token)
        : kInitialDimensions(initialDimensions),
          kType(type),
          mNnApi(nnApi),
          mMemory(std::move(memory)),
          kToken(std::move(token)) {}

    bool tensorHasUnspecifiedDimensions(::android::nn::OperandType type,
                                        const ::android::nn::Dimensions& dimensions) {
        if (!::android::nn::isExtension(type)) {
            if (isNonExtensionScalar(type)) {
                return false;
            }
        }
        return dimensions.size() == 0 || std::any_of(dimensions.begin(), dimensions.end(),
                                                     [](int32_t dim) { return dim == 0; });
    }

    bool validateDimensions(const ::android::nn::Dimensions& dimensions) {
        if (isNonExtensionScalar(kType)) {
            if (!dimensions.empty()) {
                LOG(ERROR) << "ShimBuffer::validateDimensions -- invalid dimensions for scalar "
                              "operand";
                return false;
            }
            return true;
        }

        if (dimensions.empty()) {
            if (tensorHasUnspecifiedDimensions(kType, kInitialDimensions)) {
                LOG(ERROR) << "ShimBuffer::validateDimensions -- the initial dimensions are not "
                              "fully specified and no dimension update is provided: ";

                return false;
            }
        } else {
            if (tensorHasUnspecifiedDimensions(kType, dimensions)) {
                LOG(ERROR) << "ShimBuffer::validateDimensions -- the updated dimensions are not "
                              "fully specified: ";

                return false;
            }
        }

        const auto combined = ::android::nn::combineDimensions(kInitialDimensions, dimensions);
        if (!combined.has_value()) {
            LOG(ERROR) << "ShimBuffer::validateDimensions -- incompatible dimensions";
            return false;
        }
        return true;
    }

    ndk::ScopedAStatus copyFrom(const aidl::android::hardware::neuralnetworks::Memory& src,
                                const std::vector<int32_t>& dimensions) override {
        auto memory = convertFromHAL(mNnApi, src);

        if (!memory) {
            LOG(ERROR) << "Failed to convert HAL Memory to SL memory";
            return toAStatus(ErrorStatus::INVALID_ARGUMENT);
        }
        const auto unsignedDimensions = ::android::nn::toUnsigned(dimensions);
        if (!unsignedDimensions.has_value()) {
            return toAStatus(aidl_hal::ErrorStatus::INVALID_ARGUMENT,
                             unsignedDimensions.error().message);
        }

        if (!validateDimensions(unsignedDimensions.value())) {
            LOG(ERROR) << "Invalid dimensions";
            return toAStatus(ErrorStatus::INVALID_ARGUMENT);
        }
        Result result = memory->copyTo(*mMemory.get());

        // Special case expected error status for uninitialized source memory
        if (result == Result::BAD_DATA) {
            // NNAPI Runtime reports both uninitialized memory
            // and incompatible dimensions as BAD_DATA, but
            // VTS expects to see INVALID_ARGUMENT for bad dimensions,
            // and GENERAL_FAILURE for uninitialized memory.
            if (memory->getSize() != mMemory->getSize()) {
                return toAStatus(ErrorStatus::INVALID_ARGUMENT, "Incompatible sizes");
            }

            return toAStatus(ErrorStatus::GENERAL_FAILURE);
        }
        SLW2SAS_RETURN_IF_ERROR(result);
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus copyTo(const Memory& dst) override {
        auto memory = convertFromHAL(mNnApi, dst);

        if (!memory) {
            LOG(ERROR) << "Failed to convert HAL Memory to SL memory";
            return toAStatus(ErrorStatus::INVALID_ARGUMENT);
        }

        Result result = mMemory->copyTo(*memory);
        // Special case expected error status for uninitialized source memory
        if (result == Result::BAD_DATA) {
            // NNAPI Runtime reports both uninitialized memory
            // and incompatible dimensions as BAD_DATA, but
            // VTS expects to see INVALID_ARGUMENT for bad dimensions,
            // and GENERAL_FAILURE for uninitialized memory.
            if (memory->getSize() != mMemory->getSize()) {
                return toAStatus(ErrorStatus::INVALID_ARGUMENT, "Incompatible sizes");
            }
            return toAStatus(ErrorStatus::GENERAL_FAILURE);
        }
        SLW2SAS_RETURN_IF_ERROR(result);
        return ndk::ScopedAStatus::ok();
    }

   private:
    const ::android::nn::Dimensions kInitialDimensions;
    const ::android::nn::OperandType kType;

    const NnApiSupportLibrary* mNnApi;
    std::shared_ptr<::android::nn::sl_wrapper::Memory> mMemory;
    const std::unique_ptr<ShimBufferTracker::Token> kToken;
};

::ndk::ScopedAStatus ShimDevice::allocate(const BufferDesc& desc,
                                          const std::vector<IPreparedModelParcel>& preparedModels,
                                          const std::vector<BufferRole>& inputRoles,
                                          const std::vector<BufferRole>& outputRoles,
                                          DeviceBuffer* buffer) {
    if (!isValidDimension(desc.dimensions)) {
        LOG(ERROR) << "ShimDriver::allocate -- passed invalid dimension values.";
        return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                         "ShimDriver::allocate -- passed invalid dimension values");
    }
    ANeuralNetworksMemoryDesc* slDesc = nullptr;
    mNnapi->ANeuralNetworksMemoryDesc_create(&slDesc);
    const auto slDescGuard = ::android::base::make_scope_guard(
            [this, slDesc] { mNnapi->ANeuralNetworksMemoryDesc_free(slDesc); });

    auto unsignedDimensions = ::android::nn::toUnsigned(desc.dimensions).value();
    if (mNnapi->ANeuralNetworksMemoryDesc_setDimensions(slDesc, desc.dimensions.size(),
                                                        unsignedDimensions.data()) !=
        ANEURALNETWORKS_NO_ERROR) {
        LOG(ERROR) << "ShimDriver::allocate -- ANeuralNetworksMemoryDesc_setDimensions fail.";
        return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                         "ShimDriver::allocate -- ANeuralNetworksMemoryDesc_setDimensions fail");
    }

    constexpr auto getCompilation = [](IPreparedModel* preparedModel) -> const ShimPreparedModel* {
        const auto* samplePreparedModel = castToShimPreparedModel(preparedModel);
        if (samplePreparedModel == nullptr) {
            LOG(ERROR) << "ShimDriver::allocate -- unknown remote IPreparedModel.";
            return nullptr;
        }
        return samplePreparedModel;
    };

    std::optional<::android::nn::OperandType> type;
    std::vector<uint32_t> dimensions = ::android::nn::toUnsigned(desc.dimensions).value();

    for (const auto& role : inputRoles) {
        if (role.modelIndex < 0 || role.modelIndex >= preparedModels.size()) {
            LOG(ERROR) << "Invalid modelIndex value " << role.modelIndex;
            return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                             "ShimDriver::allocate -- Input role modeIndex with invalid value");
        }
        auto preparedModel = preparedModels[role.modelIndex];
        if (preparedModel.preparedModel == nullptr) {
            return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                             "ShimDriver::allocate -- nullptr model");
        }

        auto pmodel = getCompilation(preparedModel.preparedModel.get());
        if (pmodel == nullptr) {
            return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                             "ShimDriver::allocate -- nullptr model");
        }

        auto result = mNnapi->ANeuralNetworksMemoryDesc_addInputRole(
                slDesc, pmodel->getCompilation().getHandle(), role.ioIndex, role.probability);

        if (result != ANEURALNETWORKS_NO_ERROR) {
            LOG(ERROR) << "SampleDriver::allocate -- ANeuralNetworksMemoryDesc_addInputRole fail.";
            return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                             "ShimDriver::allocate -- ANeuralNetworksMemoryDesc_addInputRole fail");
        }

        const auto& model = pmodel->getMainModel();
        const auto& op = model.getOperands()[model.getInputs()[role.ioIndex]];
        auto operandType = static_cast<::android::nn::OperandType>(op.operandType.type);
        if (!type) {
            type = operandType;
        }
        if (dimensions.empty()) {
            dimensions = op.dimensions;
        }
    }

    for (const auto& role : outputRoles) {
        if (role.modelIndex < 0 || role.modelIndex >= preparedModels.size()) {
            LOG(ERROR) << "Invalid modelIndex value " << role.modelIndex;
            return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                             "ShimDriver::allocate -- Ou0tput role modeIndex with invalid value");
        }
        auto preparedModel = preparedModels[role.modelIndex];
        if (preparedModel.preparedModel == nullptr) {
            return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                             "ShimDriver::allocate -- nullptr model");
        }

        auto pmodel = getCompilation(preparedModel.preparedModel.get());
        if (pmodel == nullptr) {
            return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                             "ShimDriver::allocate -- nullptr model");
        }

        auto result = mNnapi->ANeuralNetworksMemoryDesc_addOutputRole(
                slDesc, pmodel->getCompilation().getHandle(), role.ioIndex, role.probability);

        if (result != ANEURALNETWORKS_NO_ERROR) {
            LOG(ERROR) << "SampleDriver::allocate -- ANeuralNetworksMemoryDesc_addInputRole fail.";
            return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                             "ShimDriver::allocate -- ANeuralNetworksMemoryDesc_addInputRole fail");
        }
        const auto& model = pmodel->getMainModel();
        const auto& op = model.getOperands()[model.getInputs()[role.ioIndex]];
        auto operandType = static_cast<::android::nn::OperandType>(op.operandType.type);
        if (!type) {
            type = operandType;
        }
        if (dimensions.empty()) {
            dimensions = op.dimensions;
        }
    }

    auto typeSize = ::android::nn::getNonExtensionSize(*type, dimensions);
    if (!typeSize.has_value()) {
        return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                         "ShimDriver::allocate -- failed to get underlying type size, "
                         "possibly an extension type");
    }

    mNnapi->ANeuralNetworksMemoryDesc_finish(slDesc);
    auto memory =
            std::make_shared<::android::nn::sl_wrapper::Memory>(mNnapi.get(), slDesc, *typeSize);

    if (!memory->isValid()) {
        LOG(ERROR) << "ShimDriver::allocate -- ANeuralNetworksMemory_createFromDesc failed.";
        return toAStatus(ErrorStatus::GENERAL_FAILURE,
                         "ShimDriver::allocate -- ANeuralNetworksMemory_createFromDesc failed");
    }

    auto token = mBufferTracker->add(memory);
    if (token == nullptr) {
        LOG(ERROR) << "ShimDriver::allocate -- ShimBufferTracker returned invalid token.";
        return toAStatus(ErrorStatus::GENERAL_FAILURE,
                         "ShimDriver::allocate -- ShimBufferTracker returned invalid token.");
    }
    const uint32_t tokenValue = token->get();
    auto shimbuffer = ndk::SharedRefBase::make<ShimBuffer>(mNnapi.get(), dimensions, *type,
                                                           std::move(memory), std::move(token));
    buffer->buffer = std::move(shimbuffer);
    buffer->token = tokenValue;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ShimDevice::getCapabilities(Capabilities* capabilities) {
    *capabilities = mCapabilities;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ShimDevice::getNumberOfCacheFilesNeeded(NumberOfCacheFiles* numberOfCacheFiles) {
    *numberOfCacheFiles = mNumberOfCacheFiles;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ShimDevice::getSupportedExtensions(std::vector<Extension>* extensions) {
    *extensions = mExtensions;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ShimDevice::getSupportedOperations(const Model& model,
                                                      std::vector<bool>* supportedOperations) {
    const auto numOperations = model.main.operations.size();
    supportedOperations->resize(numOperations);

    ErrorStatus convertErrorStatus = ErrorStatus::NONE;
    std::vector<uint8_t> copiedOperandValues;
    auto modelAndMemory =
            convertFromHAL(mNnapi.get(), model, &copiedOperandValues, &convertErrorStatus);
    if (!modelAndMemory || modelAndMemory->models.empty()) {
        LOG(ERROR) << "Failed to convert HAL model to SL model";
        return toAStatus(convertErrorStatus);
    }

    auto annModel = modelAndMemory->models[0].getHandle();
    auto supportedOps = std::make_unique<bool[]>(numOperations);

    auto result = mNnapi->ANeuralNetworksModel_getSupportedOperationsForDevices(
            annModel, &mDevice, /*numDevices=*/1, supportedOps.get());
    SLW2SAS_RETURN_IF_ERROR(result);

    std::copy(supportedOps.get(), supportedOps.get() + numOperations, supportedOperations->begin());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ShimDevice::getType(DeviceType* type) {
    int32_t deviceType;
    auto result = mNnapi->ANeuralNetworksDevice_getType(mDevice, &deviceType);
    SLW2SAS_RETURN_IF_ERROR(result);
    *type = static_cast<DeviceType>(deviceType);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ShimDevice::getVersionString(std::string* versionString) {
    const char* buffer;
    auto result = mNnapi->ANeuralNetworksDevice_getVersion(mDevice, &buffer);
    SLW2SAS_RETURN_IF_ERROR(result);

    *versionString = std::string(buffer);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ShimDevice::prepareModel(
        const Model& model, ExecutionPreference preference, Priority priority, int64_t deadlineNs,
        const std::vector<::ndk::ScopedFileDescriptor>& modelCache,
        const std::vector<::ndk::ScopedFileDescriptor>& dataCache,
        const std::vector<uint8_t>& token,
        const std::shared_ptr<IPreparedModelCallback>& callback) {
    // TODO(183398748): Run model preparation in detached thread.
    if (callback == nullptr) {
        return toAStatus(ErrorStatus::INVALID_ARGUMENT);
    }

    ErrorStatus convertErrorStatus = ErrorStatus::NONE;
    std::vector<uint8_t> copiedOperandValues;
    auto modelAndMemory =
            convertFromHAL(mNnapi.get(), model, &copiedOperandValues, &convertErrorStatus);

    if (!modelAndMemory || modelAndMemory->models.empty()) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(convertErrorStatus);
    }

    auto compilation = ::android::nn::sl_wrapper::Compilation::createForDevice(
            mNnapi.get(), &modelAndMemory->models[0], mDevice);
    if (compilation.first != Result::NO_ERROR) {
        SLW2SAS_RETURN_AND_CALLBACK_IF_ERROR(compilation.first, callback);
    }

    if (auto p = convertToNDKPreference(preference)) {
        SLW2SAS_RETURN_AND_CALLBACK_IF_ERROR(compilation.second.setPreference(*p), callback);
    } else {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(ErrorStatus::INVALID_ARGUMENT);
    }

    if (auto p = convertToNDKPriority(priority)) {
        SLW2SAS_RETURN_AND_CALLBACK_IF_ERROR(compilation.second.setPriority(*p), callback);
    } else {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(ErrorStatus::INVALID_ARGUMENT);
    }

    // TODO(170375075): support caching and deadline
    if (compilation.second.finish() != Result::NO_ERROR) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(ErrorStatus::INVALID_ARGUMENT);
    }

    const std::shared_ptr<ShimPreparedModel> preparedModel =
            ndk::SharedRefBase::make<ShimPreparedModel>(
                    mNnapi, mBufferTracker, std::move(compilation.second),
                    std::move(modelAndMemory->models), std::move(modelAndMemory->memory),
                    std::move(copiedOperandValues));

    callback->notify(ErrorStatus::NONE, preparedModel);

    // TODO(170375075): support caching and deadline
    (void)deadlineNs;
    (void)modelCache;
    (void)dataCache;
    (void)token;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ShimDevice::prepareModelFromCache(
        int64_t deadlineNs, const std::vector<::ndk::ScopedFileDescriptor>& modelCache,
        const std::vector<::ndk::ScopedFileDescriptor>& dataCache,
        const std::vector<uint8_t>& token,
        const std::shared_ptr<IPreparedModelCallback>& callback) {
    const auto ret = callback->notify(ErrorStatus::GENERAL_FAILURE, nullptr);

    (void)deadlineNs;
    (void)modelCache;
    (void)dataCache;
    (void)token;

    // TODO(170375075): support caching and deadline
    return toAStatus(ErrorStatus::GENERAL_FAILURE);
}

}  // namespace aidl::android::hardware::neuralnetworks
