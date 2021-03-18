/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "Manager"

#include "Manager.h"

#include <CpuExecutor.h>
#include <LegacyUtils.h>
#include <MetaModel.h>
#include <Tracing.h>
#include <nnapi/IBurst.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/Validation.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "ExecutionCallback.h"
#include "FeatureLevel.h"
#include "Memory.h"
#include "ModelArgumentInfo.h"
#include "TypeManager.h"

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
#include <build/version.h>
#include <cutils/native_handle.h>
#include <nnapi/hal/1.3/Buffer.h>
#include <nnapi/hal/Service.h>

#include "AppInfoFetcher.h"
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

namespace android {
namespace nn {

// A Device with actual underlying driver
class DriverDevice : public Device {
   public:
    // Create a DriverDevice from a name and a DeviceFactory function.
    // Returns nullptr on failure.
    static std::shared_ptr<DriverDevice> create(SharedDevice device);

    // Prefer using DriverDevice::create
    explicit DriverDevice(SharedDevice device);

    const std::string& getName() const override { return kInterface->getName(); }
    const std::string& getVersionString() const override { return kInterface->getVersionString(); }
    int64_t getFeatureLevel() const override;
    int32_t getType() const override { return static_cast<int32_t>(kInterface->getType()); }
    bool isUpdatable() const override { return kInterface->isUpdatable(); }
    const std::vector<Extension>& getSupportedExtensions() const override {
        return kInterface->getSupportedExtensions();
    }
    std::vector<bool> getSupportedOperations(const MetaModel& metaModel) const override;
    Capabilities::PerformanceInfo getPerformance(OperandType type) const override {
        return kInterface->getCapabilities().operandPerformance.lookup(type);
    }
    Capabilities::PerformanceInfo getRelaxedFloat32toFloat16PerformanceScalar() const override {
        return kInterface->getCapabilities().relaxedFloat32toFloat16PerformanceScalar;
    }
    Capabilities::PerformanceInfo getRelaxedFloat32toFloat16PerformanceTensor() const override {
        return kInterface->getCapabilities().relaxedFloat32toFloat16PerformanceTensor;
    }
    Capabilities::PerformanceInfo getIfPerformance() const override {
        return kInterface->getCapabilities().ifPerformance;
    }
    Capabilities::PerformanceInfo getWhilePerformance() const override {
        return kInterface->getCapabilities().whilePerformance;
    }
    bool isCachingSupported() const override {
        // Caching is supported if either of numModelCache or numDataCache is greater than 0.
        const auto [numModelCacheFiles, numDataCacheFiles] =
                kInterface->getNumberOfCacheFilesNeeded();
        return numModelCacheFiles > 0 || numDataCacheFiles > 0;
    }
    int wait() const override {
        auto result = kInterface->wait();
        if (!result.ok()) {
            LOG(ERROR) << "DriverDevice::wait error: " << result.error().message;
            return convertErrorStatusToResultCode(result.error().code);
        }
        return ANEURALNETWORKS_NO_ERROR;
    }

    std::pair<int, std::shared_ptr<RuntimePreparedModel>> prepareModel(
            const ModelFactory& makeModel, ExecutionPreference preference, Priority priority,
            const OptionalTimePoint& deadline, const std::string& cacheDir,
            const std::optional<CacheToken>& maybeToken) const override;

    std::pair<int, std::unique_ptr<RuntimeMemory>> allocate(const MemoryDescriptor& desc,
                                                            OperandType) const override;

   private:
    const SharedDevice kInterface;

    GeneralResult<std::vector<bool>> getSupportedOperationsImpl(const MetaModel& metaModel) const;
    GeneralResult<SharedPreparedModel> prepareModelFromCacheInternal(
            const OptionalTimePoint& deadline, const std::string& cacheDir,
            const CacheToken& token) const;

#ifdef NN_DEBUGGABLE
    // For debugging: behavior of IDevice::getSupportedOperations for SampleDriver.
    // 0 - all operations reported by IDevice::getSupportedOperations() supported
    // 1 - some operations reported by IDevice::getSupportedOperations() supported
    uint32_t mSupported = 0;
#endif  // NN_DEBUGGABLE
};

// A RuntimePreparedModel with underlying IPreparedModel instance return by actual driver.
class DriverPreparedModel : public RuntimePreparedModel {
   public:
    DriverPreparedModel(const Device* device, const SharedPreparedModel& preparedModel)
        : mDevice(device), mPreparedModel(preparedModel) {
        CHECK(mDevice != nullptr);
        CHECK(mPreparedModel != nullptr);
    }

    const Device* getDevice() const override { return mDevice; }
    SharedPreparedModel getInterface() const override { return mPreparedModel; }
    std::tuple<int, std::vector<OutputShape>, Timing> execute(
            const std::vector<ModelArgumentInfo>& inputs,
            const std::vector<ModelArgumentInfo>& outputs,
            const std::vector<const RuntimeMemory*>& memories, const SharedBurst& burstController,
            MeasureTiming measure, const OptionalTimePoint& deadline,
            const OptionalDuration& loopTimeoutDuration) const override;

    std::tuple<int, int, ExecuteFencedInfoCallback, Timing> executeFenced(
            const std::vector<ModelArgumentInfo>& inputs,
            const std::vector<ModelArgumentInfo>& outputs,
            const std::vector<const RuntimeMemory*>& memories, const std::vector<int>& waitFor,
            MeasureTiming measure, const OptionalTimePoint& deadline,
            const OptionalDuration& loopTimeoutDuration,
            const OptionalDuration& timeoutDurationAfterFence) const override;

    GeneralResult<SharedBurst> configureExecutionBurst() const override {
        return mPreparedModel->configureExecutionBurst();
    }

   private:
    const Device* mDevice;
    const SharedPreparedModel mPreparedModel;
};

DriverDevice::DriverDevice(SharedDevice device) : kInterface(std::move(device)) {
    CHECK(kInterface != nullptr);
#ifdef NN_DEBUGGABLE
    static const char samplePrefix[] = "sample";
    if (getName().substr(0, sizeof(samplePrefix) - 1) == samplePrefix) {
        mSupported = getProp("debug.nn.sample.supported");
    }
#endif  // NN_DEBUGGABLE
}

std::shared_ptr<DriverDevice> DriverDevice::create(SharedDevice device) {
    if (device == nullptr) {
        LOG(ERROR) << "DriverDevice::create called with nullptr";
        return nullptr;
    }

    return std::make_shared<DriverDevice>(std::move(device));
}

int64_t DriverDevice::getFeatureLevel() const {
    Version featureLevel = kInterface->getFeatureLevel();
    switch (featureLevel) {
        case Version::ANDROID_OC_MR1:
            return __ANDROID_API_O_MR1__;
        case Version::ANDROID_P:
            return __ANDROID_API_P__;
        case Version::ANDROID_Q:
            return __ANDROID_API_Q__;
        case Version::ANDROID_R:
            return __ANDROID_API_R__;
        default:
            LOG(FATAL) << "Unsupported driver feature level: " << featureLevel;
            return -1;
    };
}

GeneralResult<std::vector<bool>> DriverDevice::getSupportedOperationsImpl(
        const MetaModel& metaModel) const {
    const auto featureLevel = kInterface->getFeatureLevel();
    const auto slice = metaModel.getSlice(featureLevel);
    if (!slice.has_value()) {
        return NN_ERROR() << "getSlice(" << featureLevel << ") failed";
    }

    const auto& [sliceModel, slicedModelOperationIndexToModelOperationIndex] = *slice;
    const std::vector<bool> supported = NN_TRY(kInterface->getSupportedOperations(sliceModel));
    const uint32_t slicedOperationCount = sliceModel.main.operations.size();
    if (supported.size() != slicedOperationCount) {
        return NN_ERROR() << "IDevice::getSupportedOperations returned a vector of length "
                          << supported.size() << " when expecting " << slicedOperationCount;
    }

    const Model& model = metaModel.getModel();
    const uint32_t operationCount = model.main.operations.size();
    std::vector<bool> remappedSupported(operationCount, false);
    for (size_t i = 0; i < supported.size(); ++i) {
        if (supported[i]) {
            remappedSupported[slicedModelOperationIndexToModelOperationIndex(i)] = true;
        }
    }
    return remappedSupported;
}

std::vector<bool> DriverDevice::getSupportedOperations(const MetaModel& metaModel) const {
    const Model& model = metaModel.getModel();

    auto result = getSupportedOperationsImpl(metaModel);
    if (!result.ok()) {
        LOG(ERROR) << "getSupportedOperations failed with code " << result.error().code << ": "
                   << result.error().message;
        // Set the supported operation vectors to all false, so we won't use this driver.
        return std::vector<bool>(model.main.operations.size(), false);
    }

    std::vector<bool>& supportedOperations = result.value();
#ifdef NN_DEBUGGABLE
    if (mSupported != 1) {
        return supportedOperations;
    }

    const uint32_t baseAccumulator = std::hash<std::string>{}(getName());
    for (size_t operationIndex = 0; operationIndex < supportedOperations.size(); operationIndex++) {
        if (!supportedOperations[operationIndex]) {
            continue;
        }

        uint32_t accumulator = baseAccumulator;
        const Operation& operation = model.main.operations[operationIndex];
        accumulator ^= static_cast<uint32_t>(operation.type);
        auto accumulateOperands = [&model, &accumulator](const std::vector<uint32_t>& operands) {
            for (uint32_t operandIndex : operands) {
                const Operand& operand = model.main.operands[operandIndex];
                accumulator ^= static_cast<uint32_t>(operand.type);
                accumulator ^= operand.dimensions.size();
                for (const Dimension& dimension : operand.dimensions) {
                    accumulator ^= dimension;
                    if (operand.lifetime == Operand::LifeTime::CONSTANT_COPY ||
                        operand.lifetime == Operand::LifeTime::CONSTANT_REFERENCE ||
                        operand.lifetime == Operand::LifeTime::POINTER) {
                        accumulator ^= 1;
                    }
                }
            }
        };
        accumulateOperands(operation.inputs);
        accumulateOperands(operation.outputs);
        if (accumulator & 1) {
            supportedOperations[operationIndex] = false;
        }
    }
#endif  // NN_DEBUGGABLE

    return supportedOperations;
}

// Opens a cache file for reading and writing and returns a shared handle.
static GeneralResult<SharedHandle> createCacheHandle(const std::string& filename,
                                                     bool createIfNotExist) {
    auto fd = base::unique_fd(open(filename.c_str(), createIfNotExist ? (O_RDWR | O_CREAT) : O_RDWR,
                                   S_IRUSR | S_IWUSR));
    if (fd.get() == -1) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
               << "Failed to " << (createIfNotExist ? "open or create" : "open") << " cache file "
               << filename;
    }
    std::vector<base::unique_fd> fds;
    fds.push_back(std::move(fd));
    return std::make_shared<const Handle>(Handle{
            .fds = std::move(fds),
            .ints = {},
    });
}

// Opens a list of cache files and returns a vector of shared handles. The files
// are always opened with both read and write permissions.
static GeneralResult<std::vector<SharedHandle>> createCacheHandleVec(
        uint32_t numCacheFiles, const std::string& baseFilename, bool createIfNotExist) {
    CHECK(numCacheFiles <= kMaxNumberOfCacheFiles);
    std::vector<SharedHandle> handles;
    handles.reserve(numCacheFiles);
    for (uint32_t i = 0; i < numCacheFiles; i++) {
        std::string filename = baseFilename + std::to_string(i);
        VLOG(COMPILATION) << "Cache " << i << ": " << filename;
        handles.push_back(NN_TRY(createCacheHandle(filename, createIfNotExist)));
    }
    return handles;
}

struct CacheHandles {
    std::vector<SharedHandle> modelCache;
    std::vector<SharedHandle> dataCache;
};

// Maps a token to cache file names and returns a pair of vectors of shared
// handles to the opened files.
static GeneralResult<CacheHandles> getCacheHandles(
        const std::string& cacheDir, const CacheToken& token,
        const std::pair<uint32_t, uint32_t>& numCacheFiles, bool createIfNotExist) {
    // The filename includes kByteSizeOfCacheToken * 2 characters for token,
    // and 1 character for model/data cache identifier.
    std::string filename(kByteSizeOfCacheToken * 2 + 1, '0');
    for (uint32_t i = 0; i < kByteSizeOfCacheToken; i++) {
        filename[i * 2] = 'A' + (token[i] & 0x0F);
        filename[i * 2 + 1] = 'A' + (token[i] >> 4);
    }

    CHECK(cacheDir.empty() || cacheDir.back() == '/');
    std::string cacheFileName = cacheDir + filename;
    const uint32_t cacheTypeIdentifierIndex = cacheDir.size() + kByteSizeOfCacheToken * 2;

    cacheFileName[cacheTypeIdentifierIndex] = '1';
    std::vector<SharedHandle> modelCache =
            NN_TRY(createCacheHandleVec(numCacheFiles.first, cacheFileName, createIfNotExist));

    cacheFileName[cacheTypeIdentifierIndex] = '2';
    std::vector<SharedHandle> dataCache =
            NN_TRY(createCacheHandleVec(numCacheFiles.second, cacheFileName, createIfNotExist));

    return CacheHandles{
            .modelCache = std::move(modelCache),
            .dataCache = std::move(dataCache),
    };
}

GeneralResult<SharedPreparedModel> DriverDevice::prepareModelFromCacheInternal(
        const OptionalTimePoint& deadline, const std::string& cacheDir,
        const CacheToken& token) const {
    // Get cache files if they exist, otherwise return from the function early.
    const auto cache =
            NN_TRY(getCacheHandles(cacheDir, token, kInterface->getNumberOfCacheFilesNeeded(),
                                   /*createIfNotExist=*/false));
    return kInterface->prepareModelFromCache(deadline, cache.modelCache, cache.dataCache, token);
}

std::pair<int, std::shared_ptr<RuntimePreparedModel>> DriverDevice::prepareModel(
        const ModelFactory& makeModel, ExecutionPreference preference, Priority priority,
        const OptionalTimePoint& deadline, const std::string& cacheDir,
        const std::optional<CacheToken>& maybeToken) const {
    // Attempt to compile from cache if token is present.
    if (maybeToken.has_value()) {
        auto result = prepareModelFromCacheInternal(deadline, cacheDir, *maybeToken);
        if (result.has_value()) {
            return {ANEURALNETWORKS_NO_ERROR,
                    std::make_shared<DriverPreparedModel>(this, std::move(result).value())};
        } else {
            LOG(ERROR) << "prepareModelFromCache failure (" << result.error().code
                       << "): " << result.error().message;
        }
    }

    // Get cache files if they exist, otherwise create them.
    CacheHandles cache;
    if (maybeToken.has_value()) {
        auto result =
                getCacheHandles(cacheDir, *maybeToken, kInterface->getNumberOfCacheFilesNeeded(),
                                /*createIfNotExist=*/true);
        if (result.has_value()) {
            cache = std::move(result).value();
        } else {
            LOG(ERROR) << "getCacheHandles failure (" << result.error().code
                       << "): " << result.error().message;
        }
    }

    // Get the token if it exists, otherwise get a null token.
    static constexpr CacheToken kNullToken = {};
    const CacheToken token = maybeToken.value_or(kNullToken);

    // Fallback to full compilation (possibly with token) if
    // prepareModelFromCache could not be used or failed.
    const Model model = makeModel();
    auto result = kInterface->prepareModel(model, preference, priority, deadline, cache.modelCache,
                                           cache.dataCache, token);
    if (!result.ok()) {
        LOG(ERROR) << "IDevice::prepareModel() error: " << result.error().message;
        return {ANEURALNETWORKS_OP_FAILED, nullptr};  // TODO: confirm
    }
    SharedPreparedModel preparedModel = std::move(result).value();
    CHECK(preparedModel != nullptr)
            << "IDevice::prepareModel() returned nullptr without error code";
    return {ANEURALNETWORKS_NO_ERROR,
            std::make_shared<DriverPreparedModel>(this, std::move(preparedModel))};
}

std::pair<int, std::unique_ptr<RuntimeMemory>> DriverDevice::allocate(const MemoryDescriptor& desc,
                                                                      OperandType) const {
    const BufferDesc bufferDesc = {.dimensions = desc.dimensions};
    std::vector<SharedPreparedModel> preparedModels(desc.preparedModels.size());
    std::transform(desc.preparedModels.begin(), desc.preparedModels.end(), preparedModels.begin(),
                   [](const auto* preparedModel) {
                       const auto versionedPreparedModel = preparedModel->getInterface();
                       CHECK(versionedPreparedModel != nullptr);
                       return versionedPreparedModel;
                   });
    auto result =
            kInterface->allocate(bufferDesc, preparedModels, desc.inputRoles, desc.outputRoles);
    if (!result.ok()) {
        LOG(ERROR) << "DriverDevice::allocate -- memory allocation on device " << getName()
                   << " failed!";
        return {convertErrorStatusToResultCode(result.error().code), nullptr};
    }
    return MemoryFromDevice::create(std::move(result).value());
}

// Figures out how to place each of the input or outputs in a buffer. This just
// does the layout and memory allocation, it does not copy data.  Aligns each
// input a bit.
static std::tuple<int, std::unique_ptr<MemoryAshmem>, std::vector<DataLocation>>
allocatePointerArgumentsToPool(const std::vector<ModelArgumentInfo>& args,
                               std::vector<const RuntimeMemory*>* memories) {
    CHECK(memories != nullptr);
    std::vector<DataLocation> ptrArgsLocations;
    const uint32_t nextPoolIndex = memories->size();
    int64_t total = 0;
    for (const auto& info : args) {
        if (info.state() == ModelArgumentInfo::POINTER) {
            // TODO Good enough alignment?
            total += alignBytesNeeded(static_cast<uint32_t>(total), info.length());
            ptrArgsLocations.push_back({.poolIndex = nextPoolIndex,
                                        .offset = static_cast<uint32_t>(total),
                                        .length = info.length()});
            total += info.length();
        }
    };
    if (total > 0xFFFFFFFF) {
        LOG(ERROR) << "allocatePointerArgumentsToPool: ANeuralNetworksExecution: Size of all "
                      "inputs or outputs exceeds 2^32.";
        return {ANEURALNETWORKS_BAD_DATA, nullptr, std::vector<DataLocation>{}};
    }
    if (total <= 0) {
        return {ANEURALNETWORKS_NO_ERROR, nullptr, std::vector<DataLocation>{}};
    }
    auto [n, memory] = MemoryAshmem::create(total);
    if (n != ANEURALNETWORKS_NO_ERROR) {
        return {n, nullptr, std::vector<DataLocation>{}};
    }
    memories->push_back(memory.get());
    return {ANEURALNETWORKS_NO_ERROR, std::move(memory), std::move(ptrArgsLocations)};
}

// Perform computation on an actual device driver.
//
// Because HIDL cannot take raw pointers, two separate memory pools will be allocated for inputs and
// outputs specified by pointers. The input pointer data will be copied to the input pool prior to
// execution, and the output pointer data will be copied out from the output pool after the
// execution.
std::tuple<int, std::vector<OutputShape>, Timing> DriverPreparedModel::execute(
        const std::vector<ModelArgumentInfo>& inputs, const std::vector<ModelArgumentInfo>& outputs,
        const std::vector<const RuntimeMemory*>& memories, const SharedBurst& burstController,
        MeasureTiming measure, const OptionalTimePoint& deadline,
        const OptionalDuration& loopTimeoutDuration) const {
    NNTRACE_RT(NNTRACE_PHASE_INPUTS_AND_OUTPUTS, "DriverPreparedModel::execute");

    // Make a copy of the memory tracker as we will append memory pools for pointer arguments.
    std::vector<const RuntimeMemory*> localMemories = memories;

    // We separate the input & output pools so accelerators only need to copy
    // the contents of the input pools. We could also use it to set protection
    // on read only memory but that's not currently done.

    // Layout the input and output data
    const auto [n1, inputPtrArgsMemory, inputPtrArgsLocations] =
            allocatePointerArgumentsToPool(inputs, &localMemories);
    if (n1 != ANEURALNETWORKS_NO_ERROR) {
        return {n1, {}, {}};
    }
    const auto [n2, outputPtrArgsMemory, outputPtrArgsLocations] =
            allocatePointerArgumentsToPool(outputs, &localMemories);
    if (n2 != ANEURALNETWORKS_NO_ERROR) {
        return {n2, {}, {}};
    }

    // Copy the input data that was specified via a pointer.
    if (inputPtrArgsMemory != nullptr) {
        uint32_t ptrInputIndex = 0;
        for (const auto& info : inputs) {
            if (info.state() == ModelArgumentInfo::POINTER) {
                const DataLocation& loc = inputPtrArgsLocations[ptrInputIndex++];
                uint8_t* const data = inputPtrArgsMemory->getPointer();
                memcpy(data + loc.offset, info.buffer(), loc.length);
            }
        }
    }

    Request request;
    request.inputs = createRequestArguments(inputs, inputPtrArgsLocations);
    request.outputs = createRequestArguments(outputs, outputPtrArgsLocations);
    request.pools.reserve(localMemories.size());
    std::transform(localMemories.begin(), localMemories.end(), std::back_inserter(request.pools),
                   [](const RuntimeMemory* localMemory) { return localMemory->getMemoryPool(); });

    NNTRACE_FULL_SWITCH(NNTRACE_LAYER_IPC, NNTRACE_PHASE_EXECUTION,
                        "DriverPreparedModel::execute::execute");

    ExecutionResult<std::pair<std::vector<OutputShape>, Timing>> result;

    // compute using burst if present, otherwise compute from IPreparedModel
    const bool burstCompute = (burstController != nullptr);
    if (burstCompute) {
        for (const RuntimeMemory* memory : localMemories) {
            const auto pool = memory->getMemoryPool();
            if (const auto* maybeMemory = std::get_if<SharedMemory>(&pool)) {
                auto cacheHold = burstController->cacheMemory(*maybeMemory);
                memory->hold(cacheHold);
            }
        }

        VLOG(EXECUTION) << "Before burstController->execute() " << SHOW_IF_DEBUG(request);

        result = burstController->execute(request, measure);
    } else {
        result = mPreparedModel->execute(request, measure, deadline, loopTimeoutDuration);
    }

    int n = ANEURALNETWORKS_OP_FAILED;
    std::vector<OutputShape> outputShapes;
    Timing timing;

    if (result.ok()) {
        n = ANEURALNETWORKS_NO_ERROR;
        std::tie(outputShapes, timing) = std::move(result).value();
    } else {
        auto [message, code, returnedOutputShapes] = std::move(result).error();
        VLOG(EXECUTION) << "**Execution failed** (ResultCode = " << n << ")";
        LOG(ERROR) << (burstCompute ? "IBurst" : "IPreparedModel")
                   << "::execute(...) error: " << message;
        n = convertErrorStatusToResultCode(code);
        if (code == ErrorStatus::OUTPUT_INSUFFICIENT_SIZE) {
            outputShapes = std::move(returnedOutputShapes);
        }
    }

    if (n != ANEURALNETWORKS_NO_ERROR) {
        VLOG(EXECUTION) << "**Execution failed** (ResultCode = " << n << ")";
        return {n, std::move(outputShapes), timing};
    }

    // Copy the output data from shared memory to the output buffers.
    NNTRACE_RT_SWITCH(NNTRACE_PHASE_RESULTS, "DriverPreparedModel::execute");
    if (outputPtrArgsMemory != nullptr) {
        uint32_t ptrOutputIndex = 0;
        for (const auto& info : outputs) {
            if (info.state() == ModelArgumentInfo::POINTER) {
                const DataLocation& loc = outputPtrArgsLocations[ptrOutputIndex++];
                const uint8_t* const data = outputPtrArgsMemory->getPointer();
                memcpy(info.buffer(), data + loc.offset, loc.length);
            }
        }
    }

    VLOG(EXECUTION) << "DriverPreparedModel::execute completed";
    return {ANEURALNETWORKS_NO_ERROR, std::move(outputShapes), timing};
}

std::tuple<int, int, ExecuteFencedInfoCallback, Timing> DriverPreparedModel::executeFenced(
        const std::vector<ModelArgumentInfo>& inputs, const std::vector<ModelArgumentInfo>& outputs,
        const std::vector<const RuntimeMemory*>& memories, const std::vector<int>& waitFor,
        MeasureTiming measure, const OptionalTimePoint& deadline,
        const OptionalDuration& loopTimeoutDuration,
        const OptionalDuration& timeoutDurationAfterFence) const {
    NNTRACE_RT(NNTRACE_PHASE_INPUTS_AND_OUTPUTS, "DriverPreparedModel::executeFenced");
    CHECK(std::all_of(waitFor.begin(), waitFor.end(), [](int fd) { return fd > 0; }));
    // Make a copy of the memory tracker as we will append memory pools for pointer arguments.
    std::vector<const RuntimeMemory*> localMemories = memories;

    // We separate the input & output pools so accelerators only need to copy
    // the contents of the input pools. We could also use it to set protection
    // on read only memory but that's not currently done.

    // Layout the input and output data
    const auto [n1, inputPtrArgsMemory, inputPtrArgsLocations] =
            allocatePointerArgumentsToPool(inputs, &localMemories);
    if (n1 != ANEURALNETWORKS_NO_ERROR) {
        return {n1, -1, nullptr, {}};
    }
    const auto [n2, outputPtrArgsMemory, outputPtrArgsLocations] =
            allocatePointerArgumentsToPool(outputs, &localMemories);
    if (n2 != ANEURALNETWORKS_NO_ERROR) {
        return {n2, -1, nullptr, {}};
    }

    // Copy the input data that was specified via a pointer.
    if (inputPtrArgsMemory != nullptr) {
        uint32_t ptrInputIndex = 0;
        for (const auto& info : inputs) {
            if (info.state() == ModelArgumentInfo::POINTER) {
                const DataLocation& loc = inputPtrArgsLocations[ptrInputIndex++];
                uint8_t* const data = inputPtrArgsMemory->getPointer();
                memcpy(data + loc.offset, info.buffer(), loc.length);
            }
        }
    }

    Request request;
    request.inputs = createRequestArguments(inputs, inputPtrArgsLocations);
    request.outputs = createRequestArguments(outputs, outputPtrArgsLocations);
    request.pools.reserve(localMemories.size());
    std::transform(localMemories.begin(), localMemories.end(), std::back_inserter(request.pools),
                   [](const RuntimeMemory* localMemory) { return localMemory->getMemoryPool(); });

    NNTRACE_FULL_SWITCH(NNTRACE_LAYER_IPC, NNTRACE_PHASE_EXECUTION,
                        "DriverPreparedModel::executeFenced");

    std::vector<SyncFence> waitForHandles;
    waitForHandles.reserve(waitFor.size());
    for (int fd : waitFor) {
        int dupFd = dup(fd);
        if (dupFd <= 0) {
            LOG(ERROR) << "Unable to dup the file descriptor";
            return {ANEURALNETWORKS_OP_FAILED, -1, nullptr, {}};
        }
        waitForHandles.push_back(SyncFence::create(base::unique_fd(dupFd)));
    }

    SyncFence syncFence = SyncFence::createAsSignaled();
    ExecuteFencedInfoCallback executeFencedInfoCallback = nullptr;
    Timing timing = {};
    if (mDevice->getFeatureLevel() >= kHalVersionV1_3ToApi.featureLevel) {
        auto result = mPreparedModel->executeFenced(request, waitForHandles, measure, deadline,
                                                    loopTimeoutDuration, timeoutDurationAfterFence);
        if (!result.ok()) {
            LOG(ERROR) << "IPreparedModel::executeFenced() error: " << result.error().message;
            VLOG(EXECUTION) << "**executeFenced failed**";
            return {convertErrorStatusToResultCode(result.error().code), -1, nullptr, {}};
        }
        std::tie(syncFence, executeFencedInfoCallback) = std::move(result).value();
    } else {
        // Fallback to synchronous execution if executeFenced is not supported.
        // First wait for all sync fences to be ready.
        LOG(INFO) << "No drivers able to handle sync fences, falling back to regular execution";
        for (const auto& fence : waitForHandles) {
            if (!fence.hasFd() || fence.getFd() <= 0) {
                return {ANEURALNETWORKS_BAD_DATA, -1, nullptr, {}};
            }
            auto r = fence.syncWait({/* no timeout */});
            if (r != SyncFence::FenceState::SIGNALED) {
                LOG(ERROR) << "syncWait failed, fd: " << fence.getFd() << ", state: " << r;
                return {ANEURALNETWORKS_OP_FAILED, -1, nullptr, {}};
            }
        }
        auto result = mPreparedModel->execute(request, measure, deadline, loopTimeoutDuration);
        if (!result.ok()) {
            LOG(ERROR) << "IPreparedModel::execute() error: " << result.error().message;
            return {convertErrorStatusToResultCode(result.error().code), -1, nullptr, {}};
        }
        std::tie(std::ignore, timing) = result.value();
    }

    int syncFenceFd = -1;
    if (syncFence.hasFd()) {
        syncFenceFd = dup(syncFence.getFd());
        if (syncFenceFd < 0) {
            LOG(ERROR) << "Failed to dup the file descriptor";
            return {ANEURALNETWORKS_OP_FAILED, -1, nullptr, timing};
        }
    }
    // If output buffer is provided as a malloc pointer, wait for the execution to finish.
    // Then copy the output data from shared memory to the output buffers.
    if (outputPtrArgsMemory != nullptr) {
        NNTRACE_RT_SWITCH(NNTRACE_PHASE_RESULTS, "DriverPreparedModel::executeFenced");
        if (syncFenceFd > 0) {
            auto r = syncWait(syncFenceFd, -1);
            if (r != FenceState::SIGNALED) {
                LOG(ERROR) << "syncWait failed, fd: " << syncFenceFd;
                return {ANEURALNETWORKS_OP_FAILED, syncFenceFd, nullptr, timing};
            }
        }
        uint32_t ptrOutputIndex = 0;
        for (const auto& info : outputs) {
            if (info.state() == ModelArgumentInfo::POINTER) {
                const DataLocation& loc = outputPtrArgsLocations[ptrOutputIndex++];
                const uint8_t* const data = outputPtrArgsMemory->getPointer();
                memcpy(info.buffer(), data + loc.offset, loc.length);
            }
        }
    }

    VLOG(EXECUTION) << "DriverPreparedModel::executeFenced completed";
    return {ANEURALNETWORKS_NO_ERROR, syncFenceFd, executeFencedInfoCallback, timing};
}

// A special abstracted device for the CPU. Only one instance of this class will exist.
// Use get() to retrieve it.
class CpuDevice : public Device {
   public:
    // Returns the singleton CPU fallback device.
    static std::shared_ptr<CpuDevice> get() {
        static std::shared_ptr<CpuDevice> instance(new CpuDevice);
        return instance;
    }

    const std::string& getName() const override { return kName; }
    const std::string& getVersionString() const override { return kVersionString; }
    int64_t getFeatureLevel() const override { return kFeatureLevel; }
    int32_t getType() const override { return ANEURALNETWORKS_DEVICE_CPU; }
    bool isUpdatable() const override { return false; }
    const std::vector<Extension>& getSupportedExtensions() const override {
        return kSupportedExtensions;
    }
    std::vector<bool> getSupportedOperations(const MetaModel& metaModel) const override;
    Capabilities::PerformanceInfo getPerformance(OperandType) const override {
        return kPerformance;
    }
    Capabilities::PerformanceInfo getRelaxedFloat32toFloat16PerformanceScalar() const override {
        return kPerformance;
    }
    Capabilities::PerformanceInfo getRelaxedFloat32toFloat16PerformanceTensor() const override {
        return kPerformance;
    }
    Capabilities::PerformanceInfo getIfPerformance() const override { return kPerformance; }
    Capabilities::PerformanceInfo getWhilePerformance() const override { return kPerformance; }
    bool isCachingSupported() const override { return false; }
    int wait() const override { return ANEURALNETWORKS_NO_ERROR; }

    std::pair<int, std::shared_ptr<RuntimePreparedModel>> prepareModel(
            const ModelFactory& makeModel, ExecutionPreference preference, Priority priority,
            const OptionalTimePoint& deadline, const std::string& cacheDir,
            const std::optional<CacheToken>& maybeToken) const override;

    std::pair<int, std::unique_ptr<RuntimeMemory>> allocate(const MemoryDescriptor& desc,
                                                            OperandType type) const override;

   private:
    CpuDevice() = default;
    const int64_t kFeatureLevel = kCurrentNNAPIRuntimeFeatureLevel;
    const std::string kName = "nnapi-reference";
#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
    const std::string kVersionString = build::GetBuildNumber();
#else
    const std::string kVersionString = "UNKNOWN";
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD
    // Since the performance is a ratio compared to the CPU performance,
    // by definition the performance of the CPU is 1.0.
    const Capabilities::PerformanceInfo kPerformance = {.execTime = 1.0f, .powerUsage = 1.0f};
    const std::vector<Extension> kSupportedExtensions{/* No extensions. */};
};

// A special abstracted RuntimePreparedModel for the CPU, constructed by CpuDevice.
class CpuPreparedModel : public RuntimePreparedModel {
   public:
    // Factory method for CpuPreparedModel. Returns ANEURALNETWORKS_NO_ERROR and
    // a prepared model object if successfully created. Returns an error code
    // and nullptr otherwise.
    static std::pair<int, std::shared_ptr<RuntimePreparedModel>> create(Model model);

    const Device* getDevice() const override { return CpuDevice::get().get(); }
    SharedPreparedModel getInterface() const override { return nullptr; }

    std::tuple<int, std::vector<OutputShape>, Timing> execute(
            const std::vector<ModelArgumentInfo>& inputs,
            const std::vector<ModelArgumentInfo>& outputs,
            const std::vector<const RuntimeMemory*>& memories, const SharedBurst& burstController,
            MeasureTiming measure, const OptionalTimePoint& deadline,
            const OptionalDuration& loopTimeoutDuration) const override;

    GeneralResult<SharedBurst> configureExecutionBurst() const override { return nullptr; }

    std::tuple<int, int, ExecuteFencedInfoCallback, Timing> executeFenced(
            const std::vector<ModelArgumentInfo>& inputs,
            const std::vector<ModelArgumentInfo>& outputs,
            const std::vector<const RuntimeMemory*>& memories, const std::vector<int>& wait_for,
            MeasureTiming measure, const OptionalTimePoint& deadline,
            const OptionalDuration& loopTimeoutDuration,
            const OptionalDuration& timeoutDurationAfterFence) const override;

    // Prefer to use CpuPreparedModel::create.
    CpuPreparedModel(Model model, std::vector<RunTimePoolInfo> poolInfos)
        : mModel(std::move(model)), mModelPoolInfos(std::move(poolInfos)) {}

   private:
    const Model mModel;
    const std::vector<RunTimePoolInfo> mModelPoolInfos;
};

std::vector<bool> CpuDevice::getSupportedOperations(const MetaModel& metaModel) const {
    const Model& model = metaModel.getModel();
    const size_t count = model.main.operations.size();
    std::vector<bool> result(count, false);
    for (size_t i = 0; i < count; i++) {
        // TODO(b/119870033): Decide whether and how post-P operations would be supported on CPU.
        //                    We may want to use the slicer for CpuDevice just as we do for
        //                    DriverDevice.
        OperationType operationType = model.main.operations[i].type;
        result[i] = !isExtension(operationType) && operationType != OperationType::OEM_OPERATION;
    }
    return result;
}

std::pair<int, std::shared_ptr<RuntimePreparedModel>> CpuDevice::prepareModel(
        const ModelFactory& makeModel, ExecutionPreference preference, Priority priority,
        const OptionalTimePoint& deadline, const std::string& /*cacheDir*/,
        const std::optional<CacheToken>& maybeToken) const {
    CHECK(!maybeToken.has_value())
            << "Should never call prepareModel with cache information on CpuDevice";

    const Model model = makeModel();
    if (auto result = validate(model); !result.ok()) {
        LOG(ERROR) << "Invalid Model: " << result.error();
        return {ANEURALNETWORKS_OP_FAILED, nullptr};
    }
    if (auto result = validate(preference); !result.ok()) {
        LOG(ERROR) << "Invalid ExecutionPreference: " << result.error();
        return {ANEURALNETWORKS_OP_FAILED, nullptr};
    }
    if (auto result = validate(priority); !result.ok()) {
        LOG(ERROR) << "Invalid Priority: " << result.error();
        return {ANEURALNETWORKS_OP_FAILED, nullptr};
    }
    if (hasDeadlinePassed(deadline)) {
        return {ANEURALNETWORKS_MISSED_DEADLINE_PERSISTENT, nullptr};
    }

    return CpuPreparedModel::create(model);
}

std::pair<int, std::unique_ptr<RuntimeMemory>> CpuDevice::allocate(const MemoryDescriptor& desc,
                                                                   OperandType type) const {
    uint32_t size = TypeManager::get()->getSizeOfData(type, desc.dimensions);
    if (size == 0) {
        LOG(ERROR) << "CpuDevice::allocate -- does not support unknown dimensions.";
        return {ANEURALNETWORKS_OP_FAILED, nullptr};
    }
    return MemoryAshmem::create(size);
}

std::pair<int, std::shared_ptr<RuntimePreparedModel>> CpuPreparedModel::create(Model model) {
    std::vector<RunTimePoolInfo> poolInfos;
    if (!setRunTimePoolInfosFromCanonicalMemories(&poolInfos, model.pools)) {
        return {ANEURALNETWORKS_UNMAPPABLE, nullptr};
    }

    std::shared_ptr<RuntimePreparedModel> preparedModel =
            std::make_shared<CpuPreparedModel>(std::move(model), std::move(poolInfos));
    return {ANEURALNETWORKS_NO_ERROR, std::move(preparedModel)};
}

static std::tuple<int, std::vector<OutputShape>, Timing> computeOnCpu(
        const Model& model, const Request& request,
        const std::vector<RunTimePoolInfo>& modelPoolInfos,
        const std::vector<RunTimePoolInfo>& requestPoolInfos, const OptionalTimePoint& deadline,
        const OptionalDuration& loopTimeoutDuration) {
    NNTRACE_RT(NNTRACE_PHASE_EXECUTION, "computeOnCpu");
    CpuExecutor executor;
    if (loopTimeoutDuration.has_value()) {
        executor.setLoopTimeout(loopTimeoutDuration->count());
    }
    if (deadline.has_value()) {
        executor.setDeadline(*deadline);
    }
    int err = executor.run(model, request, modelPoolInfos, requestPoolInfos);
    const auto& outputShapes = executor.getOutputShapes();
    return {err, outputShapes, {}};
}

std::tuple<int, int, ExecuteFencedInfoCallback, Timing> CpuPreparedModel::executeFenced(
        const std::vector<ModelArgumentInfo>& inputs, const std::vector<ModelArgumentInfo>& outputs,
        const std::vector<const RuntimeMemory*>& memories, const std::vector<int>& waitFor,
        MeasureTiming measure, const OptionalTimePoint& deadline,
        const OptionalDuration& loopTimeoutDuration, const OptionalDuration& duration) const {
    VLOG(EXECUTION)
            << "CpuPreparedModel::executeFenced wait for sync fences to signal before execution";
    for (int syncFd : waitFor) {
        if (syncFd > 0) {
            auto r = syncWait(syncFd, -1);
            if (r != FenceState::SIGNALED) {
                LOG(ERROR) << "sync wait failed, fd: " << syncFd;
                return {ANEURALNETWORKS_OP_FAILED, -1, nullptr, {}};
            }
        }
    }

    // Update deadline if the timeout duration is closer than the deadline.
    auto closestDeadline = deadline;
    if (duration.has_value()) {
        const auto timeoutDurationDeadline = makeDeadline(*duration);
        if (!closestDeadline.has_value() || *closestDeadline > timeoutDurationDeadline) {
            closestDeadline = timeoutDurationDeadline;
        }
    }

    const auto [result, outputShapes, timing] = execute(inputs, outputs, memories, nullptr, measure,
                                                        closestDeadline, loopTimeoutDuration);
    return {result, -1, nullptr, timing};
}

// Perform computation on NNAPI CPU reference implementation.
//
// Contrary to DriverPreparedModel::execute, the NNAPI CPU reference executor lives in the
// same process as the NNAPI runtime and can take raw pointers. We will create as many pools as
// there are input/output in this method to avoid data copying.
//
// Will choose between sync/async execution according to DeviceManager::mSyncExecCpu.
std::tuple<int, std::vector<OutputShape>, Timing> CpuPreparedModel::execute(
        const std::vector<ModelArgumentInfo>& inputs, const std::vector<ModelArgumentInfo>& outputs,
        const std::vector<const RuntimeMemory*>& memories, const SharedBurst& /*burstController*/,
        MeasureTiming /*measure*/, const OptionalTimePoint& deadline,
        const OptionalDuration& loopTimeoutDuration) const {
    if (hasDeadlinePassed(deadline)) {
        return {ANEURALNETWORKS_MISSED_DEADLINE_PERSISTENT, {}, {}};
    }

    std::vector<RunTimePoolInfo> requestPoolInfos;
    requestPoolInfos.reserve(memories.size());
    for (const RuntimeMemory* mem : memories) {
        if (std::optional<RunTimePoolInfo> poolInfo = mem->getRunTimePoolInfo()) {
            requestPoolInfos.emplace_back(*poolInfo);
        } else {
            return {ANEURALNETWORKS_UNMAPPABLE, {}, {}};
        }
    }
    // Create as many pools as there are input / output.
    auto fixPointerArguments =
            [&requestPoolInfos](const std::vector<ModelArgumentInfo>& argumentInfos) {
                std::vector<DataLocation> ptrArgsLocations;
                for (const ModelArgumentInfo& argumentInfo : argumentInfos) {
                    if (argumentInfo.state() == ModelArgumentInfo::POINTER) {
                        ptrArgsLocations.push_back(
                                {.poolIndex = static_cast<uint32_t>(requestPoolInfos.size()),
                                 .offset = 0,
                                 .length = argumentInfo.length(),
                                 .padding = argumentInfo.padding()});
                        requestPoolInfos.emplace_back(RunTimePoolInfo::createFromExistingBuffer(
                                static_cast<uint8_t*>(argumentInfo.buffer())));
                    }
                }
                return ptrArgsLocations;
            };
    const std::vector<DataLocation> inputPtrArgsLocations = fixPointerArguments(inputs);
    const std::vector<DataLocation> outputPtrArgsLocations = fixPointerArguments(outputs);

    Request request;
    request.inputs = createRequestArguments(inputs, inputPtrArgsLocations);
    request.outputs = createRequestArguments(outputs, outputPtrArgsLocations);

    if (!DeviceManager::get()->syncExecCpu()) {
        // TODO: use a thread pool
        // TODO(mikie): this could have NNTRACE so we could measure the overhead
        //              of spinning up a new thread.
        std::tuple<int, std::vector<OutputShape>, Timing> result = {};
        std::thread([this, &request, &requestPoolInfos, &deadline, &loopTimeoutDuration, &result] {
            result = computeOnCpu(mModel, request, mModelPoolInfos, requestPoolInfos, deadline,
                                  loopTimeoutDuration);
        }).join();
        return result;
    }

    return computeOnCpu(mModel, request, mModelPoolInfos, requestPoolInfos, deadline,
                        loopTimeoutDuration);
}

DeviceManager* DeviceManager::get() {
    static DeviceManager manager;
    return &manager;
}

std::shared_ptr<Device> DeviceManager::getCpuDevice() {
    return CpuDevice::get();
}

std::shared_ptr<Device> DeviceManager::forTest_makeDriverDevice(const SharedDevice& device) {
    VLOG(MANAGER) << "forTest_makeDriverDevice(" << device->getName() << ")";
    const auto driverDevice = DriverDevice::create(device);
    CHECK(driverDevice != nullptr);
    return driverDevice;
}

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
std::vector<SharedDevice> getDevices() {
    return hal::getDevices();
}
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

void DeviceManager::findAvailableDevices() {
    VLOG(MANAGER) << "findAvailableDevices";

    // register driver devices
    std::vector<SharedDevice> devices = getDevices();
    for (const auto& device : devices) {
        VLOG(MANAGER) << "Found interface " << device->getName();
        registerDevice(device);
    }

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
    // register CPU fallback device
    mDevices.push_back(CpuDevice::get());
    mDevicesCpuOnly.push_back(CpuDevice::get());
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD
}

static bool updatableDriversAreAllowed() {
#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
    const auto& appInfo = AppInfoFetcher::get()->getAppInfo();
    const bool currentProcessIsOnThePlatform =
            appInfo.appIsSystemApp || appInfo.appIsOnVendorImage || appInfo.appIsOnProductImage;
    return !currentProcessIsOnThePlatform;
#else
    // The concept does not exist in the compatibility library build.
    return true;
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD
}

void DeviceManager::registerDevice(const SharedDevice& device) {
    if (auto driverDevice = DriverDevice::create(device)) {
        if (!driverDevice->isUpdatable() || updatableDriversAreAllowed()) {
            mDevices.push_back(std::move(driverDevice));
        }
    }
}

DeviceManager::DeviceManager() {
    VLOG(MANAGER) << "DeviceManager::DeviceManager";
    findAvailableDevices();
#ifdef NN_DEBUGGABLE
    mStrictSlicing = (getProp("debug.nn.strict-slicing") != 0);
    mPartitioning = getProp("debug.nn.partition", kPartitioningDefault);
    mDebugNNCpuOnly = (getProp("debug.nn.cpuonly") != 0);
    mSyncExecCpu = (getProp("debug.nn.syncexec-cpu", 1) != 0);
    mSyncExecRuntime = (getProp("debug.nn.syncexec-runtime") != 0);
#endif  // NN_DEBUGGABLE
}

}  // namespace nn
}  // namespace android
