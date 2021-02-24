/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "SharedMemory.h"

#include <android-base/logging.h>

#include <limits>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "Result.h"
#include "Types.h"

namespace android::nn {
namespace {

constexpr size_t safeDivideRoundedUp(size_t numerator, size_t denominator) {
    CHECK_NE(denominator, 0u);
    CHECK_LE(numerator, std::numeric_limits<size_t>::max() - denominator);
    return (numerator + denominator - 1) / denominator;
}

constexpr size_t safeMultiply(size_t a, size_t b) {
    if (b == 0) {
        return 0;
    }
    CHECK_LE(a, std::numeric_limits<size_t>::max() / b);
    return a * b;
}

constexpr size_t extendToAlignment(size_t length) {
    constexpr size_t kMaxAlignment = alignof(AlignedData);
    return safeMultiply(safeDivideRoundedUp(length, kMaxAlignment), kMaxAlignment);
}

}  // namespace

MutableMemoryBuilder::MutableMemoryBuilder(uint32_t poolIndex) : mPoolIndex(poolIndex) {}

DataLocation MutableMemoryBuilder::append(size_t length) {
    CHECK_GT(length, 0u);
    const size_t offset = mSize;
    mSize += extendToAlignment(length);
    CHECK_LE(offset, std::numeric_limits<uint32_t>::max());
    CHECK_LE(length, std::numeric_limits<uint32_t>::max());
    return {.poolIndex = mPoolIndex,
            .offset = static_cast<uint32_t>(offset),
            .length = static_cast<uint32_t>(length)};
}

bool MutableMemoryBuilder::empty() const {
    return mSize == 0;
}

GeneralResult<SharedMemory> MutableMemoryBuilder::finish() {
    return createSharedMemory(mSize);
}

ConstantMemoryBuilder::ConstantMemoryBuilder(uint32_t poolIndex) : mBuilder(poolIndex) {}

DataLocation ConstantMemoryBuilder::append(const void* data, size_t length) {
    const auto location = mBuilder.append(length);
    CHECK_EQ(location.length, length);
    mSlices.push_back({.data = data, .length = length, .offset = location.offset});
    return location;
}

bool ConstantMemoryBuilder::empty() const {
    return mBuilder.empty();
}

GeneralResult<SharedMemory> ConstantMemoryBuilder::finish() {
    // Allocate the memory.
    auto memory = NN_TRY(mBuilder.finish());

    // Map the memory.
    const auto [pointer, size, context] = NN_TRY(map(memory););

    // Get mutable pointer.
    uint8_t* mutablePointer = static_cast<uint8_t*>(std::get<void*>(pointer));

    // Copy data to the memory pool.
    std::for_each(mSlices.begin(), mSlices.end(), [mutablePointer](const auto& slice) {
        std::memcpy(mutablePointer + slice.offset, slice.data, slice.length);
    });

    return memory;
}

}  // namespace android::nn
