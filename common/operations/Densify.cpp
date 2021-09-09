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

#ifdef NN_EXPERIMENTAL_FEATURE

#include "Densify.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <numeric>
#include <vector>

#include "OperationResolver.h"
#include "OperationsUtils.h"
#include "Tracing.h"
#include "nnapi/OperandTypes.h"
#include "nnapi/TypeUtils.h"
#include "nnapi/Validation.h"

#define LOG_TAG "Operations"

namespace android {
namespace nn {
namespace densify_op {

constexpr uint32_t kMinNumInputs = 5;
constexpr uint32_t kInputTensor = 0;
constexpr uint32_t kInputTravOrder = 1;
constexpr uint32_t kInputBlockMap = 2;
constexpr uint32_t kInputDimFormat = 3;
constexpr uint32_t kInputDimensions = 4;
constexpr uint32_t kInputArrSeg = 5;
constexpr uint32_t kInputArrIdx = 6;
constexpr uint32_t kNumOutputs = 1;
constexpr uint32_t kOutputTensor = 0;
constexpr int32_t DENSE = 0;
constexpr int32_t SPARSE_CSR = 1;

uint64_t getFlattenedIndex(const std::vector<int32_t>& indices, const std::vector<uint32_t>& shape,
                           const int origRank) {
    uint64_t index = 0;
    int subElems = 1;
    // origRank = size of destDims
    for (int i = origRank - 1; i >= 0; i--) {
        index += uint64_t(indices[i] * subElems);
        subElems *= shape[i];
    }
    return index;
}

template <typename T>
void populate(const T* srcData, std::vector<int32_t>* indices, int32_t level, int32_t prevIdx,
              T* destData, const std::vector<uint32_t>& destDims,
              const std::vector<int32_t>& dimFormat, const int32_t* traversalOrder,
              const std::vector<int32_t>& blockSize, const int32_t* blockMap,
              const std::vector<std::vector<int32_t>>& dimMetadata, const int origRank) {
    if (level == (*indices).size()) {  // level == size of traversal order
        std::vector<int> origIdx(origRank);
        int i = 0;
        // Calculating origIdx using dense tensor dimensions
        for (; i < origIdx.size(); i++) {
            int origDim = traversalOrder[i];
            origIdx[origDim] = (*indices)[i];
        }
        // Modifying origIdx using block dimensions
        for (; i < (*indices).size(); i++) {
            const int blockIdx = traversalOrder[i] - origRank;
            const int origDim = blockMap[blockIdx];
            origIdx[origDim] = origIdx[origDim] * blockSize[blockIdx] + (*indices)[i];
        }
        // Writing srcData to destData
        destData[getFlattenedIndex(origIdx, destDims, origRank)] = srcData[prevIdx];
        return;
    }
    const int metadataIdx = 2 * level;
    if (dimFormat[level] == DENSE) {  // DENSE dimension format
        const int shapeOfLevel = dimMetadata[metadataIdx].front();
        for (int i = 0; i < shapeOfLevel; i++) {
            (*indices)[level] = i;
            populate(srcData, indices, level + 1, prevIdx * shapeOfLevel + i, destData, destDims,
                     dimFormat, traversalOrder, blockSize, blockMap, dimMetadata, origRank);
        }
    } else {  // SPARSE_CSR dimension format
        const auto& arraySegments = dimMetadata[metadataIdx];
        const auto& arrayIndices = dimMetadata[metadataIdx + 1];
        for (int i = arraySegments[prevIdx]; i < arraySegments[prevIdx + 1]; i++) {
            (*indices)[level] = arrayIndices[i];
            populate(srcData, indices, level + 1, i, destData, destDims, dimFormat, traversalOrder,
                     blockSize, blockMap, dimMetadata, origRank);
        }
    }
}

template <typename T>
std::vector<T> arrToVector(const T* arr, uint32_t size) {
    return arr == nullptr ? std::vector<T>() : std::vector<T>(arr, arr + size);
}

template <typename T>
inline bool densify(IOperationExecutionContext* context) {
    // Getting all inputs
    std::vector<Shape> inputShapes;
    const uint32_t inputCount = context->getNumInputs();
    inputShapes.reserve(inputCount);
    const T* srcData = context->getInputBuffer<T>(kInputTensor);
    inputShapes.push_back(context->getInputShape(kInputTensor));
    const int32_t* traversalOrder = context->getInputBuffer<int32_t>(kInputTravOrder);
    inputShapes.push_back(context->getInputShape(kInputTravOrder));
    const int32_t* blockMap = context->getInputBuffer<int32_t>(kInputBlockMap);
    inputShapes.push_back(context->getInputShape(kInputBlockMap));
    const int32_t* dimFormatPtr = context->getInputBuffer<int32_t>(kInputDimFormat);
    inputShapes.push_back(context->getInputShape(kInputDimFormat));
    const int32_t* dimensionsPtr = context->getInputBuffer<int32_t>(kInputDimensions);
    inputShapes.push_back(context->getInputShape(kInputDimensions));

    std::vector<const int32_t*> dimMetadataPtrs;
    for (uint32_t i = kInputArrSeg; i < inputCount; i++) {
        inputShapes.push_back(context->getInputShape(i));
        const int32_t* metadata = context->getInputBuffer<int32_t>(i);
        dimMetadataPtrs.push_back(metadata);
    }
    Shape destShape = context->getOutputShape(kOutputTensor);

    // Organizing dimFormat, dimensions, dimMetadata into vectors
    std::vector<int32_t> dimFormat(
            inputShapes[kInputDimFormat].dimensions.front());  // size of dimFormatPtr
    std::vector<int32_t> dimensions(dimFormat.size());
    std::vector<std::vector<int32_t>> dimMetadata(2 * dimFormat.size());
    for (size_t i = 0; i < dimFormat.size(); i++) {
        dimFormat[i] = dimFormatPtr[i];
        dimensions[i] = dimensionsPtr[i];
        if (dimFormat[i] == 0) {
            dimMetadata[i * 2] = {dimensions[i]};
        } else {
            dimMetadata[i * 2] =  // array segments
                    arrToVector(dimMetadataPtrs[i * 2],
                                inputShapes[i * 2 + kInputArrSeg].dimensions.front());
            dimMetadata[i * 2 + 1] =  // array indices
                    arrToVector(dimMetadataPtrs[i * 2 + 1],
                                inputShapes[i * 2 + kInputArrIdx].dimensions.front());
        }
    }

    // Creating blockSize vector
    const int origRank = destShape.dimensions.size();
    std::vector<int32_t> blockSize(
            inputShapes[kInputBlockMap].dimensions.front());  // size of block map
    for (int i = 0; i < inputShapes[kInputBlockMap].dimensions.front(); i++) {
        const int32_t origDim = traversalOrder[origRank + i];
        blockSize[i] = dimensions[origDim];
    }

    // Calculating the number of output entries
    const size_t denseTotal =
            std::accumulate(destShape.dimensions.begin(), destShape.dimensions.end(),
                            static_cast<size_t>(1), std::multiplies<>{});
    T zeroPoint = T();
    if (const OperandType type = inputShapes.front().type;
        type == OperandType::TENSOR_QUANT8_ASYMM ||
        type == OperandType::TENSOR_QUANT8_ASYMM_SIGNED ||
        type == OperandType::TENSOR_QUANT16_ASYMM) {
        zeroPoint = static_cast<T>(inputShapes.front().offset);
    }

    T* destData = context->getOutputBuffer<T>(kOutputTensor);
    for (int32_t i = 0; i < denseTotal; i++) {
        destData[i] = zeroPoint;
    }

    std::vector<int32_t> indices(
            inputShapes[kInputTravOrder].dimensions.front());  // size of traversal order
    populate(srcData, &indices, 0, 0, destData, destShape.dimensions, dimFormat, traversalOrder,
             blockSize, blockMap, dimMetadata, origRank);
    return true;
}

Result<Version> validate(const IOperationValidationContext* context) {
    // Checking number of inputs and outputs
    const uint32_t inputCount = context->getNumInputs();
    NN_RET_CHECK_GE(inputCount, kMinNumInputs);
    NN_RET_CHECK_EQ(inputCount,
                    kMinNumInputs + context->getInputShape(kInputTravOrder).dimensions.front() * 2);
    NN_RET_CHECK_EQ(context->getNumOutputs(), kNumOutputs);
    NN_RET_CHECK_EQ(context->getInputShape(kInputTensor).dimensions.size(), 1);
    for (uint32_t i = 1; i < inputCount; i++) {
        NN_RET_CHECK_EQ(context->getInputShape(i).dimensions.size(), 1);
        NN_RET_CHECK_EQ(context->getInputType(i), OperandType::TENSOR_INT32);
    }
    return Version::EXPERIMENTAL;
}

bool prepare(IOperationExecutionContext* context) {
    // Setting OutputShape
    Shape destShape = context->getInputShape(kInputTensor);

    const int32_t* traversalOrder = context->getInputBuffer<int32_t>(kInputTravOrder);
    const int32_t* blockMap = context->getInputBuffer<int32_t>(kInputBlockMap);
    const int32_t* dimensions = context->getInputBuffer<int32_t>(kInputDimensions);
    Shape dimensionsShape = context->getInputShape(kInputDimensions);
    Shape blockMapShape = context->getInputShape(kInputBlockMap);
    const uint32_t origRank = dimensionsShape.dimensions.front() - blockMapShape.dimensions.front();
    std::vector<uint32_t> destDims(origRank);

    int i = 0;
    for (; i < destDims.size(); i++) {
        const int32_t origDim = traversalOrder[i];
        destDims[origDim] = dimensions[i];
    }
    for (; i < dimensionsShape.dimensions.front(); i++) {
        const int32_t traversalIdx = traversalOrder[i] - origRank;
        const int32_t origDim = blockMap[traversalIdx];
        destDims[origDim] *= dimensions[i];
    }
    destShape.dimensions = destDims;
    return context->setOutputShape(kOutputTensor, destShape);
}

bool execute(IOperationExecutionContext* context) {
    switch (context->getInputType(kInputTensor)) {
        case OperandType::TENSOR_BOOL8:
            return densify<bool8>(context);
        case OperandType::TENSOR_FLOAT32:
            return densify<float>(context);
        case OperandType::TENSOR_FLOAT16:
            return densify<_Float16>(context);
        case OperandType::TENSOR_INT32:
            return densify<int32_t>(context);
        case OperandType::TENSOR_QUANT8_ASYMM:
            return densify<uint8_t>(context);
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
        case OperandType::TENSOR_QUANT8_SYMM:
            return densify<int8_t>(context);
        case OperandType::TENSOR_QUANT16_SYMM:
            return densify<int16_t>(context);
        case OperandType::TENSOR_QUANT16_ASYMM:
            return densify<uint16_t>(context);
        default:
            return false;
    }
}

}  // namespace densify_op

NN_REGISTER_OPERATION(DENSIFY, "DENSIFY", densify_op::validate, densify_op::prepare,
                      densify_op::execute, .allowOmittedOperand = true);

}  // namespace nn
}  // namespace android

#endif  // NN_EXPERIMENTAL_FEATURE