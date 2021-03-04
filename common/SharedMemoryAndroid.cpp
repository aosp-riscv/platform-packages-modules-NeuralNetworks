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

#define LOG_TAG "SharedMemoryAndroid"

#include <android-base/logging.h>
#include <android-base/mapped_file.h>
#include <android-base/scopeguard.h>
#include <android/hardware_buffer.h>

#include <algorithm>
#include <any>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "Result.h"
#include "SharedMemory.h"
#include "TypeUtils.h"
#include "Types.h"

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <hidl/HidlSupport.h>
#include <hidlmemory/mapping.h>
#include <sys/mman.h>
#else
#include <android/sharedmem.h>
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

namespace android::nn {
namespace {

GeneralResult<SharedMemory> createSharedMemoryFromUniqueFd(base::unique_fd fd, size_t size,
                                                           int prot, size_t offset) {
    CHECK_GT(size, 0u);
    CHECK(fd.ok());

    std::vector<base::unique_fd> fds;
    fds.push_back(std::move(fd));

    const auto [lowOffsetBits, highOffsetBits] = getIntsFromOffset(offset);
    std::vector<int> ints = {prot, lowOffsetBits, highOffsetBits};

    auto handle = Handle{.fds = std::move(fds), .ints = std::move(ints)};
    return std::make_shared<const Memory>(
            Memory{.handle = std::move(handle), .size = size, .name = "mmap_fd"});
}

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD

using ::android::hardware::hidl_memory;
using ::android::hidl::allocator::V1_0::IAllocator;

const char* const kAllocatorService = "ashmem";

GeneralResult<hardware::hidl_handle> hidlHandleFromSharedHandle(const Handle& handle) {
    auto fds = NN_TRY(dupFds(handle.fds.begin(), handle.fds.end()));

    constexpr size_t kMaxInt = std::numeric_limits<int>::max();
    CHECK_LE(handle.fds.size(), kMaxInt);
    CHECK_LE(handle.ints.size(), kMaxInt);
    native_handle_t* nativeHandle = native_handle_create(static_cast<int>(handle.fds.size()),
                                                         static_cast<int>(handle.ints.size()));
    if (nativeHandle == nullptr) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to create native_handle";
    }
    for (size_t i = 0; i < fds.size(); ++i) {
        nativeHandle->data[i] = fds[i].release();
    }
    std::copy(handle.ints.begin(), handle.ints.end(), &nativeHandle->data[nativeHandle->numFds]);

    hardware::hidl_handle hidlHandle;
    hidlHandle.setTo(nativeHandle, /*shouldOwn=*/true);
    return hidlHandle;
}

GeneralResult<Handle> sharedHandleFromNativeHandle(const native_handle_t* handle) {
    if (handle == nullptr) {
        return NN_ERROR() << "sharedHandleFromNativeHandle was passed nullptr for handle";
    }

    auto fds = NN_TRY(dupFds(handle->data + 0, handle->data + handle->numFds));

    auto ints = std::vector<int>(handle->data + handle->numFds,
                                 handle->data + handle->numFds + handle->numInts);

    return Handle{.fds = std::move(fds), .ints = std::move(ints)};
}

GeneralResult<SharedMemory> allocateSharedMemory(size_t size) {
    static const auto allocator = IAllocator::getService(kAllocatorService);
    CHECK_GT(size, 0u);

    hidl_memory maybeMemory;
    auto fn = [&maybeMemory](bool success, const hidl_memory& memory) {
        if (success) {
            maybeMemory = memory;
        }
    };
    allocator->allocate(size, fn);

    if (!maybeMemory.valid()) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
               << "IAllocator::allocate returned an invalid (empty) memory object";
    }

    CHECK_LE(maybeMemory.size(), std::numeric_limits<uint32_t>::max());
    return std::make_shared<const Memory>(Memory{
            .handle = NN_TRY(sharedHandleFromNativeHandle(maybeMemory.handle())),
            .size = static_cast<uint32_t>(maybeMemory.size()),
            .name = maybeMemory.name(),
    });
}

GeneralResult<Mapping> mapAshmem(const Memory& memory) {
    const auto hidlMemory = hidl_memory(
            memory.name, NN_TRY(hidlHandleFromSharedHandle(std::get<Handle>(memory.handle))),
            memory.size);

    const auto mapping = mapMemory(hidlMemory);
    if (mapping == nullptr) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to map memory";
    }

    auto* const pointer = mapping->getPointer().withDefault(nullptr);
    if (pointer == nullptr) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to get the mapped pointer";
    }

    const auto fullSize = mapping->getSize().withDefault(0);
    if (fullSize == 0 || fullSize > std::numeric_limits<size_t>::max()) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to get the mapped size";
    }

    const size_t size = static_cast<size_t>(fullSize);

    return Mapping{
            .pointer = pointer,
            .size = size,
            .context = mapping,
    };
}

#else

GeneralResult<SharedMemory> allocateSharedMemory(size_t size) {
    CHECK_GT(size, 0u);

    auto fd = base::unique_fd(ASharedMemory_create(nullptr, size));
    if (!fd.ok()) {
        return NN_ERROR() << "ASharedMemory_create failed";
    }

    const size_t readSize = ASharedMemory_getSize(fd.get());
    CHECK_GE(readSize, size);

    constexpr int prot = PROT_READ | PROT_WRITE;
    constexpr size_t offset = 0;
    return createSharedMemoryFromUniqueFd(std::move(fd), size, prot, offset);
}

GeneralResult<Mapping> mapAshmem(const Memory& /*memory*/) {
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Cannot map ashmem memory";
}

#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

struct MmapFdMappingContext {
    int prot;
    std::any context;
};

GeneralResult<Mapping> mapMemFd(const Memory& memory) {
    const size_t size = memory.size;
    const Handle& handle = std::get<Handle>(memory.handle);
    const int fd = handle.fds.front().get();
    const int prot = handle.ints[0];
    const size_t offset = getOffsetFromInts(handle.ints[1], handle.ints[2]);

    std::shared_ptr<base::MappedFile> mapping = base::MappedFile::FromFd(fd, offset, size, prot);
    if (mapping == nullptr) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Can't mmap the file descriptor.";
    }
    void* data = mapping->data();

    auto context = MmapFdMappingContext{.prot = prot, .context = std::move(mapping)};
    return Mapping{.pointer = data, .size = size, .context = std::move(context)};
}

GeneralResult<Mapping> mapAhwbBlobMemory(const HardwareBufferHandle& memory) {
    AHardwareBuffer_Desc desc;
    AHardwareBuffer_describe(memory.get(), &desc);

    CHECK_EQ(desc.format, AHARDWAREBUFFER_FORMAT_BLOB);
    const uint32_t size = desc.width;

    void* data = nullptr;
    const auto status = AHardwareBuffer_lock(memory.get(), desc.usage, -1, nullptr, &data);
    if (status != /*NO_ERROR*/ 0) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
               << "Can't lock the AHardwareBuffer. Error: " << status;
    }

    // Create shared scoped object to munmap.
    auto scoped = base::make_scope_guard(
            [ahwb = memory.get()] { AHardwareBuffer_unlock(ahwb, nullptr); });
    auto sharedScoped = std::make_shared<decltype(scoped)>(std::move(scoped));

    return Mapping{.pointer = data, .size = size, .context = std::move(sharedScoped)};
}

GeneralResult<Mapping> mapAhwbMemory(const Memory& /*memory*/) {
    return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
           << "Unable to map non-BLOB AHardwareBuffer memory";
}

void freeHardwareBuffer(AHardwareBuffer* buffer) {
    if (buffer) {
        AHardwareBuffer_release(buffer);
    }
}

void freeNoop(AHardwareBuffer* /*buffer*/) {}

}  // namespace

HardwareBufferHandle::HardwareBufferHandle(AHardwareBuffer* handle, bool takeOwnership)
    : mHandle(handle, (takeOwnership ? freeHardwareBuffer : freeNoop)) {
    CHECK(mHandle != nullptr);
}

AHardwareBuffer* HardwareBufferHandle::get() const {
    return mHandle.get();
}

GeneralResult<base::unique_fd> dupFd(int fd) {
    if (fd < 0) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "dupFd was passed an invalid fd";
    }
    auto uniqueFd = base::unique_fd(dup(fd));
    if (!uniqueFd.ok()) {
        // TODO(b/120417090): is ANEURALNETWORKS_UNEXPECTED_NULL the correct error to return here?
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to dup the fd";
    }
    return uniqueFd;
}

GeneralResult<SharedMemory> createSharedMemory(size_t size) {
    return allocateSharedMemory(size);
}

GeneralResult<SharedMemory> createSharedMemoryFromFd(size_t size, int prot, int fd, size_t offset) {
    return createSharedMemoryFromUniqueFd(NN_TRY(dupFd(fd)), size, prot, offset);
}

GeneralResult<SharedMemory> createSharedMemoryFromAHWB(AHardwareBuffer* ahwb, bool takeOwnership) {
    CHECK(ahwb != nullptr);
    auto handle = HardwareBufferHandle(ahwb, takeOwnership);

    AHardwareBuffer_Desc bufferDesc;
    AHardwareBuffer_describe(ahwb, &bufferDesc);

    const bool isBlobMode = bufferDesc.format == AHARDWAREBUFFER_FORMAT_BLOB;
    // memory size is not used for non-BLOB AHWB memory.
    const size_t size = isBlobMode ? bufferDesc.width : 0;
    std::string name = isBlobMode ? "hardware_buffer_blob" : "hardware_buffer";

    return std::make_shared<const Memory>(Memory{
            .handle = std::move(handle),
            .size = size,
            .name = std::move(name),
    });
}

GeneralResult<Mapping> map(const SharedMemory& memory) {
    if (memory->name == "mmap_fd") {
        return mapMemFd(*memory);
    }
    if (memory->name == "ashmem") {
        return mapAshmem(*memory);
    }
    if (memory->name == "hardware_buffer_blob") {
        return mapAhwbBlobMemory(std::get<HardwareBufferHandle>(memory->handle));
    }
    if (memory->name == "hardware_buffer") {
        return mapAhwbMemory(*memory);
    }
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Cannot map unknown memory " << memory->name;
}

bool flush(const Mapping& mapping) {
    if (const auto* mmapFdMapping = std::any_cast<MmapFdMappingContext>(&mapping.context)) {
        if (!std::holds_alternative<void*>(mapping.pointer)) {
            return true;
        }
        void* data = std::get<void*>(mapping.pointer);
        const int prot = mmapFdMapping->prot;
        if (prot & PROT_WRITE) {
            const size_t size = mapping.size;
            return msync(data, size, MS_SYNC) == 0;
        }
    }
    // No-op for other types of memory.
    return true;
}

}  // namespace android::nn
