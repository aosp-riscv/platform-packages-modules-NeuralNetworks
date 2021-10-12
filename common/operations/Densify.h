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

#ifndef ANDROID_FRAMEWORKS_ML_NN_COMMON_OPERATIONS_DENSIFY_H
#define ANDROID_FRAMEWORKS_ML_NN_COMMON_OPERATIONS_DENSIFY_H

#include <vector>

#include "LegacyUtils.h"
#include "OperationResolver.h"
#include "OperationsUtils.h"

namespace android {
namespace nn {
namespace densify_op {

/**
 * getFlattenedIndex:
 * Gets the index of destData where indices points to. Uses shape and origRank
 * for calculations.
 */
uint64_t getFlattenedIndex(const std::vector<int32_t>& indices, const std::vector<uint32_t>& shape,
                           const int origRank);

/**
 * populate (Recursive Function):
 * Used to populate the destData with elements from srcData one value at a time.
 * Inputs:
 * * srcData = input data of non-zero values.
 * * indices = used to determine the index in destData where we write srcData to. Uses block
 *   dimension.
 * * level = used to keep track of recursion level. Each recursive instance exits when level == size
 *   of traversal order.
 * * prevIdx = used to keep placement in array segments and srcData.
 * * destData = dense output data. Input being written to.
 * * denseShape = shape of the output tensor. Used to calculate the flattened idx.
 * * dimFormat = dimension format for each entry in traversal order. The format is either DENSE
 *   (dimFormat[i] == 0) or SPARSE_CSR (dimFormat[i] == 1). Format is significant to determine how
 *   recursive iterations will occur and what metadata is stored in dimMetadata.
 * * traversalOrder = contains n+k elements. The first n elements are a permutation of the dense
 *   tensor shape. The last k elements are a permutation of the block dimensions. Used to determine
 *   order of traversal paths.
 * * blockSize = dense size of blocks. The last k elements of dimensions.
 * * blockMap = Used to determine how the block dimension maps to the original tensor dimension.
 * * dimMetadata = metadata varies depending on dimFormat values. If format is DENSE,
 *   dimMetadata[i*2][0] is the total number of elements in the dense tensor on the ith traversal
 *   path, and recursive iterations are through a standard for loop from 0 to dimMetadata[i*2][0].
 *   If format is SPARSE_CSR, dimMetadata[i*2] is a vector of array segments and
 *   dimMetadata[i*2+1] is a vector of array indices. The next recursive iterations will be
 *   looping through the array segments vector (since array segments are the same as row pointers in
 *   CSR format, the ith entry should never be greater than the ith+1 entry) and modifying the input
 *   indices with elements from the array indices vector.
 * * origRank = the size of denseShape. Used for calculating flattened index of indices.
 */
template <typename T>
void populate(const T* srcData, std::vector<int32_t>* indices, uint32_t level, uint32_t prevIdx,
              T* destData, const std::vector<uint32_t>& denseShape,
              const std::vector<int32_t>& dimFormat, const int32_t* traversalOrder,
              const std::vector<int32_t>& blockSize, const int32_t* blockMap,
              const std::vector<std::vector<int32_t>>& dimMetadata, const int origRank);

/**
 * arrToVector:
 * Converts a T array into an T vector.
 */
template <typename T>
std::vector<T> arrToVector(const T* arr, uint32_t size);

/**
 * densify:
 * Core of execution function. Prepares inputs for Populate function.
 */
template <typename T>
inline bool densify(IOperationExecutionContext* context);

Result<Version> validate(const IOperationValidationContext* context);

bool prepare(IOperationExecutionContext* context);

bool execute(IOperationExecutionContext* context);

}  // namespace densify_op
}  // namespace nn
}  // namespace android

#endif  // ANDROID_FRAMEWORKS_ML_NN_COMMON_OPERATIONS_DENSIFY_H
