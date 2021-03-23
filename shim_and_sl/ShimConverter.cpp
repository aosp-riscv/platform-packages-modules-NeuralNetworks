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

#define LOG_TAG "ShimConverter"

#include "ShimConverter.h"
#include <android-base/logging.h>
#include <android-base/scopeguard.h>
#include <android/hardware_buffer.h>
#include <cutils/native_handle.h>
#include <sys/mman.h>
#include <vndk/hardware_buffer.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <nnapi/TypeUtils.h>
#include <nnapi/hal/aidl/Conversions.h>
#include <nnapi/hal/aidl/Utils.h>

using namespace ::android::nn::sl_wrapper;

namespace aidl::android::hardware::neuralnetworks {

namespace {

// Assumes that isValid(model) holds
ANeuralNetworksModel* convertSubgraphFromHAL(
        const NnApiSupportLibrary* nnapi,
        const std::vector<std::unique_ptr<::android::nn::sl_wrapper::Memory>>& memoryPools,
        const neuralnetworks::Model& model,
        std::vector<std::optional<::android::nn::sl_wrapper::Model>>* allModels,
        size_t subgraphIndex, ErrorStatus* errorStatus) {
    *errorStatus = ErrorStatus::NONE;
    if ((*allModels)[subgraphIndex].has_value()) {
        return (*allModels)[subgraphIndex]->getHandle();
    }

    const auto& subgraph = subgraphIndex == 0 ? model.main : model.referenced[subgraphIndex - 1];
    ::android::nn::sl_wrapper::Model resultModel(nnapi);

    for (int i = 0; i < subgraph.operands.size(); ++i) {
        const auto& operand = subgraph.operands[i];

        const std::vector<uint32_t> dimensions =
                ::android::nn::toUnsigned(operand.dimensions).value();

        ::android::nn::wrapper::OperandType operandType(
                static_cast<::android::nn::wrapper::Type>(operand.type), dimensions, operand.scale,
                operand.zeroPoint);

        if (operand.type == OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL) {
            const auto& params = operand.extraParams->get<OperandExtraParams::Tag::channelQuant>();
            operandType.channelQuant = ::android::nn::wrapper::SymmPerChannelQuantParams(
                    params.scales, static_cast<uint32_t>(params.channelDim));
        }

        resultModel.addOperand(&operandType);
        if (!resultModel.isValid()) {
            LOG(ERROR) << "Failed to add operand with index" << i;
            *errorStatus = ErrorStatus::INVALID_ARGUMENT;
            return nullptr;
        }

        switch (operand.lifetime) {
            case OperandLifeTime::CONSTANT_COPY: {
                if (operand.location.length <= 128) {
                    resultModel.setOperandValue(
                            i, model.operandValues.data() + operand.location.offset,
                            operand.location.length);
                } else {
                    LOG(ERROR) << " NNAPI shim does not support CONSTANT_COPY operand with data "
                               << "larger than 128 bytes (operand " << i << " has "
                               << operand.location.length << "bytes )";
                    // Return GENERAL_FAILURE, the model is valid, just not supported
                    // To fix: One approach we could take is to duplicate the Model struct
                    // (clone(model)), and store the cloned Model alongside the ANNModel object.
                    // Then, the model.operandValues could be used without worrying about the
                    // lifetime.
                    *errorStatus = ErrorStatus::GENERAL_FAILURE;
                    return nullptr;
                }
                break;
            }
            case OperandLifeTime::CONSTANT_POOL: {
                resultModel.setOperandValueFromMemory(
                        i, memoryPools[operand.location.poolIndex].get(), operand.location.offset,
                        operand.location.length);
                break;
            }
            case OperandLifeTime::SUBGRAPH: {
                ErrorStatus otherErrorStatus = ErrorStatus::NONE;
                auto subgraph =
                        convertSubgraphFromHAL(nnapi, memoryPools, model, allModels,
                                               operand.location.offset + 1, &otherErrorStatus);
                if (subgraph) {
                    resultModel.setOperandValueFromModel(i, subgraph);
                } else {
                    LOG(ERROR) << "Failed to set subgraph operand value";
                    *errorStatus = otherErrorStatus;
                    return nullptr;
                }
                break;
            }
            case OperandLifeTime::NO_VALUE: {
                resultModel.setOperandValue(i, nullptr, 0);
                break;
            }
            case OperandLifeTime::TEMPORARY_VARIABLE:
            case OperandLifeTime::SUBGRAPH_OUTPUT:
            case OperandLifeTime::SUBGRAPH_INPUT: {
                break;
            }
            default:
                LOG(ERROR) << "Invalid operand type: " << static_cast<int>(operand.lifetime);
                *errorStatus = ErrorStatus::INVALID_ARGUMENT;
                return nullptr;
        }

        if (!resultModel.isValid()) {
            LOG(ERROR) << "Failed to add operand with index" << i;
            *errorStatus = ErrorStatus::INVALID_ARGUMENT;
            return nullptr;
        }
    }

    for (int i = 0; i < subgraph.operations.size(); ++i) {
        const auto& operation = subgraph.operations[i];

        std::vector<uint32_t> inputs(operation.inputs.begin(), operation.inputs.end());
        std::vector<uint32_t> outputs(operation.outputs.begin(), operation.outputs.end());
        resultModel.addOperation(static_cast<int>(operation.type), inputs, outputs);

        if (!resultModel.isValid()) {
            LOG(ERROR) << "Failed to add operation with index" << i;
            *errorStatus = ErrorStatus::INVALID_ARGUMENT;
            return nullptr;
        }
    }

    std::vector<uint32_t> inputIndexes(subgraph.inputIndexes.begin(), subgraph.inputIndexes.end());
    std::vector<uint32_t> outputIndexes(subgraph.outputIndexes.begin(),
                                        subgraph.outputIndexes.end());

    resultModel.identifyInputsAndOutputs(inputIndexes, outputIndexes);
    if (!resultModel.isValid()) {
        LOG(ERROR) << "Model identifyInputsAndOutputs failed";
        *errorStatus = ErrorStatus::INVALID_ARGUMENT;
        return nullptr;
    }

    if (resultModel.finish() != Result::NO_ERROR) {
        LOG(ERROR) << "Model finish failed";
        *errorStatus = ErrorStatus::INVALID_ARGUMENT;
        return nullptr;
    }

    if (!resultModel.isValid()) {
        LOG(ERROR) << "Invalid model";
        *errorStatus = ErrorStatus::INVALID_ARGUMENT;
        return nullptr;
    }

    (*allModels)[subgraphIndex] = std::move(resultModel);
    return (*allModels)[subgraphIndex]->getHandle();
}

bool isValid(const Subgraph& subgraph) {
    // Either the operand has a known value before model execution begins, or we've seen a writer
    // for this operand while walking operands in execution order. Initialize to known operands.
    std::vector<bool> operandValueKnown;
    operandValueKnown.reserve(subgraph.operands.size());
    std::transform(subgraph.operands.begin(), subgraph.operands.end(),
                   std::back_inserter(operandValueKnown), [](const Operand& operand) {
                       return operand.lifetime != OperandLifeTime::TEMPORARY_VARIABLE &&
                              operand.lifetime != OperandLifeTime::SUBGRAPH_OUTPUT;
                   });

    // Validate that operations are sorted into execution order.
    //
    // If there is a cycle in the graph, the operations will not
    // appear to be sorted into execution order: Some operation will
    // have an input for which operandValueKnown[] is false.
    for (size_t i = 0; i < subgraph.operations.size(); ++i) {
        const auto& operation = subgraph.operations[i];

        for (size_t j = 0; j < operation.inputs.size(); ++j) {
            const uint32_t k = operation.inputs[j];
            if (!operandValueKnown[k]) {
                LOG(ERROR) << "Operation " << i << " input " << j << " (operand " << k
                           << ") is read before it is written";
                return false;
            }
        }

        for (size_t j = 0; j < operation.outputs.size(); ++j) {
            const uint32_t k = operation.outputs[j];
            // Assuming validateOperations() has not returned an error, we know that this output is
            // TEMPORARY_VARIABLE or MODEL_OUTPUT, and so the only way operandValueKnown[k] can be
            // true is if we've already seen a writer for this operand.
            if (operandValueKnown[k]) {
                LOG(ERROR) << "Operation " << i << " output " << j << " (operand " << k
                           << ") has already been written";
                return false;
            }
            operandValueKnown[k] = true;
        }
    }

    // Verify all operands are written.
    for (size_t i = 0; i < subgraph.operands.size(); ++i) {
        if (!operandValueKnown[i]) {
            LOG(ERROR) << "Operand " << i << " is never written";
            return false;
        }
        const auto& operand = subgraph.operands[i];

        if (operand.lifetime == OperandLifeTime::SUBGRAPH_OUTPUT) {
            if (std::find(subgraph.outputIndexes.begin(), subgraph.outputIndexes.end(), i) ==
                subgraph.outputIndexes.end()) {
                LOG(ERROR) << "Op with output liftime, but not on output list: " << i;
                return false;
            }
        }
    }

    // Validate input and output lifetime
    for (auto index : subgraph.inputIndexes) {
        if (subgraph.operands[index].lifetime != OperandLifeTime::SUBGRAPH_INPUT) {
            LOG(ERROR) << "Input with index" << index << " has invalid lifetime";
            return false;
        }
    }
    for (auto index : subgraph.outputIndexes) {
        if (subgraph.operands[index].lifetime != OperandLifeTime::SUBGRAPH_OUTPUT) {
            LOG(ERROR) << "Output with index" << index << " has invalid lifetime";
            return false;
        }
    }

    // TODO(b/77871786): verify that every operation has at least one output operand that is read?
    return true;
}

}  // namespace

bool isValid(const neuralnetworks::Model& model) {
    return (isValid(model.main) &&
            std::all_of(model.referenced.begin(), model.referenced.end(),
                        [](const Subgraph& subgraph) { return isValid(subgraph); }));
}

std::optional<ShimConvertedModel> convertFromHAL(const NnApiSupportLibrary* nnapi,
                                                 const neuralnetworks::Model& model,
                                                 ErrorStatus* errorStatus) {
    *errorStatus = ErrorStatus::NONE;

    // Using this pulls in OperationResolver and huge chunk of dependencies.
    // TODO(172925288): Replace as followup work
    //    if (!::aidl::android::hardware::neuralnetworks::utils::valid(model)) {
    if (!isValid(model)) {
        LOG(ERROR) << "Invalid HAL model, failed to convert into SL model";
        *errorStatus = ErrorStatus::INVALID_ARGUMENT;
        return std::nullopt;
    }

    std::vector<std::unique_ptr<::android::nn::sl_wrapper::Memory>> memoryPools;
    memoryPools.reserve(model.pools.size());
    for (const auto& pool : model.pools) {
        std::unique_ptr<::android::nn::sl_wrapper::Memory> memory = convertFromHAL(nnapi, pool);
        if (!memory) {
            LOG(ERROR) << "Failed to convert HAL memory into SL memory";
            *errorStatus = ErrorStatus::INVALID_ARGUMENT;
            return std::nullopt;
        }
        memoryPools.push_back(std::move(memory));
    }

    std::vector<std::optional<::android::nn::sl_wrapper::Model>> allModels(model.referenced.size() +
                                                                           1);

    for (size_t i = 0; i < allModels.size(); ++i) {
        if (convertSubgraphFromHAL(nnapi, memoryPools, model, &allModels, i, errorStatus) ==
            nullptr) {
            LOG(ERROR) << "Failed to convert HAL subgraphs into SL subgraphs, index: " << i;
            // Error status already set by convertSubgraphFromHAL
            return std::nullopt;
        }
    }

    std::vector<::android::nn::sl_wrapper::Model> result;
    result.reserve(allModels.size());
    for (size_t i = 0; i < allModels.size(); ++i) {
        if (!allModels[i].has_value()) {
            LOG(ERROR) << "Missing SL subgraph";
            *errorStatus = ErrorStatus::INVALID_ARGUMENT;
            return std::nullopt;
        }
        result.push_back(std::move(*allModels[i]));
    }

    return ShimConvertedModel{.memory = std::move(memoryPools), .models = std::move(result)};
}

namespace {

uint32_t roundUpToMultiple(uint32_t value, uint32_t multiple) {
    return (value + multiple - 1) / multiple * multiple;
}

}  // namespace

std::unique_ptr<::android::nn::sl_wrapper::Memory> convertFromHAL(
        const NnApiSupportLibrary* nnapi, const neuralnetworks::Memory& pool) {
    if (pool.name == "ashmem") {
        size_t size = pool.size;
        int fd = pool.handle.fds[0].get();

        auto memory = std::make_unique<::android::nn::sl_wrapper::Memory>(
                nnapi, size, PROT_READ | PROT_WRITE, fd, 0, /*ownsFd=*/false);
        if (!memory->isValid()) {
            return nullptr;
        }
        return memory;
    } else if (pool.name == "mmap_fd") {
        size_t size = pool.size;
        int fd = pool.handle.fds[0].get();
        int prot = pool.handle.ints[0];
        size_t offset = ::android::nn::getOffsetFromInts(pool.handle.ints[1], pool.handle.ints[2]);

        auto memory = std::make_unique<::android::nn::sl_wrapper::Memory>(nnapi, size, prot, fd,
                                                                          offset, /*ownsFd=*/false);
        if (!memory->isValid()) {
            return nullptr;
        }
        return memory;
    } else if (pool.name == "hardware_buffer_blob") {
        const auto numFds = pool.handle.fds.size();
        const auto numInts = pool.handle.ints.size();
        auto handle = native_handle_create(numFds, numInts);
        const auto handleGuard = ::android::base::make_scope_guard([handle] {
            native_handle_close(handle);
            native_handle_delete(handle);
        });

        std::fill(handle->data + 0, handle->data + handle->numFds, -1);
        for (int i = 0; i < numFds; ++i) {
            int fd = dup(pool.handle.fds[i].get());
            if (fd == -1) {
                LOG(ERROR) << "Dup of the hardware_buffer_blob memory pool failed";
                return nullptr;
            }
            handle->data[i] = fd;
        }
        for (int i = 0; i < numInts; ++i) {
            handle->data[numFds + i] = pool.handle.ints[i];
        }
        const auto size = pool.size;
        const auto format = AHARDWAREBUFFER_FORMAT_BLOB;
        const auto usage =
                AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
        const uint32_t width = size;
        const uint32_t height = 1;  // height is always 1 for BLOB mode AHardwareBuffer.
        const uint32_t layers = 1;  // layers is always 1 for BLOB mode AHardwareBuffer.

        AHardwareBuffer* hardwareBuffer = nullptr;

        // AHardwareBuffer_createFromHandle() might fail because an allocator
        // expects a specific stride value. In that case, we try to guess it by
        // aligning the width to small powers of 2.
        ::android::status_t status = ::android::UNKNOWN_ERROR;
        for (uint32_t alignment : {1, 4, 32, 64, 128, 2, 8, 16}) {
            const uint32_t stride = roundUpToMultiple(width, alignment);
            AHardwareBuffer_Desc desc{
                    .width = width,
                    .height = height,
                    .layers = layers,
                    .format = format,
                    .usage = usage,
                    .stride = stride,
            };
            status = AHardwareBuffer_createFromHandle(
                    &desc, handle, AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE,
                    &hardwareBuffer);
            if (status == ::android::NO_ERROR) {
                break;
            }
        }

        if (status == ::android::NO_ERROR) {
            LOG(ERROR) << "createFromHandle failed";
            return nullptr;
        }

        // Takes ownership of hardwareBuffer, handle gets closed
        auto memory = std::make_unique<::android::nn::sl_wrapper::Memory>(nnapi, hardwareBuffer,
                                                                          /*ownAHB=*/true);

        return memory;
    } else {
        LOG(ERROR) << "Can't convert to SL Memory, unknown pool name: " << pool.name;
        return nullptr;
    }
}

}  // namespace aidl::android::hardware::neuralnetworks
