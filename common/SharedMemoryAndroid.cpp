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

#include <algorithm>
#include <any>
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
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

#ifndef NN_NO_AHWB
#include <android/hardware_buffer.h>
#include <vndk/hardware_buffer.h>
#endif  // NN_NO_AHWB

namespace android::nn {
namespace {

#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
using ::android::hardware::hidl_memory;
using ::android::hidl::allocator::V1_0::IAllocator;

const char* const kAllocatorService = "ashmem";

GeneralResult<hidl_memory> allocateSharedMemory(size_t size) {
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

    return maybeMemory;
}

GeneralResult<hardware::hidl_handle> hidlHandleFromSharedHandle(const SharedHandle& handle) {
    if (handle == nullptr) {
        return {};
    }

    std::vector<base::unique_fd> fds;
    fds.reserve(handle->fds.size());
    for (const auto& fd : handle->fds) {
        int dupFd = dup(fd);
        if (dupFd == -1) {
            return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to dup the fd";
        }
        fds.emplace_back(dupFd);
    }

    native_handle_t* nativeHandle = native_handle_create(handle->fds.size(), handle->ints.size());
    if (nativeHandle == nullptr) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to create native_handle";
    }
    for (size_t i = 0; i < fds.size(); ++i) {
        nativeHandle->data[i] = fds[i].release();
    }
    std::copy(handle->ints.begin(), handle->ints.end(), &nativeHandle->data[nativeHandle->numFds]);

    hardware::hidl_handle hidlHandle;
    hidlHandle.setTo(nativeHandle, /*shouldOwn=*/true);
    return hidlHandle;
}

GeneralResult<SharedHandle> sharedHandleFromNativeHandle(const native_handle_t* handle) {
    if (handle == nullptr) {
        return nullptr;
    }

    std::vector<base::unique_fd> fds;
    fds.reserve(handle->numFds);
    for (int i = 0; i < handle->numFds; ++i) {
        int dupFd = dup(handle->data[i]);
        if (dupFd == -1) {
            return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to dup the fd";
        }
        fds.emplace_back(dupFd);
    }

    std::vector<int> ints(&handle->data[handle->numFds],
                          &handle->data[handle->numFds + handle->numInts]);

    return std::make_shared<const Handle>(Handle{
            .fds = std::move(fds),
            .ints = std::move(ints),
    });
}

GeneralResult<Memory> createMemory(const hidl_memory& memory) {
    CHECK_LE(memory.size(), std::numeric_limits<uint32_t>::max());
    return Memory{
            .handle = NN_TRY(sharedHandleFromNativeHandle(memory.handle())),
            .size = static_cast<uint32_t>(memory.size()),
            .name = memory.name(),
    };
}

GeneralResult<hidl_memory> createHidlMemory(const Memory& memory) {
    return hidl_memory(memory.name, NN_TRY(hidlHandleFromSharedHandle(memory.handle)), memory.size);
}

GeneralResult<Mapping> mapAshmem(const Memory& memory) {
    const auto hidlMemory = NN_TRY(createHidlMemory(memory));
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

#endif  // NN_COMPATIBILITY_LIBRARY_BUILD

struct MmapFdMappingContext {
    int prot;
    std::any context;
};

GeneralResult<Mapping> mapMemFd(const Memory& memory) {
    const size_t size = memory.size;
    const SharedHandle& handle = memory.handle;
    const int fd = handle->fds[0];
    const int prot = handle->ints[0];
    const size_t offset = getOffsetFromInts(handle->ints[1], handle->ints[2]);

    std::shared_ptr<base::MappedFile> mapping = base::MappedFile::FromFd(fd, offset, size, prot);
    if (mapping == nullptr) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Can't mmap the file descriptor.";
    }
    void* data = mapping->data();

    auto context = MmapFdMappingContext{.prot = prot, .context = std::move(mapping)};
    return Mapping{.pointer = data, .size = size, .context = std::move(context)};
}

#ifndef NN_NO_AHWB

static uint32_t roundUpToMultiple(uint32_t value, uint32_t multiple) {
    return (value + multiple - 1) / multiple * multiple;
}

GeneralResult<Mapping> mapAhwbBlobMemory(const Memory& memory) {
    const SharedHandle& handle = memory.handle;
    const auto size = memory.size;
    const auto format = AHARDWAREBUFFER_FORMAT_BLOB;
    const auto usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    const uint32_t width = size;
    const uint32_t height = 1;  // height is always 1 for BLOB mode AHardwareBuffer.
    const uint32_t layers = 1;  // layers is always 1 for BLOB mode AHardwareBuffer.

    // AHardwareBuffer_createFromHandle() might fail because an allocator
    // expects a specific stride value. In that case, we try to guess it by
    // aligning the width to small powers of 2.
    // TODO(b/174120849): Avoid stride assumptions.
    AHardwareBuffer* hardwareBuffer = nullptr;
    status_t status = UNKNOWN_ERROR;
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
        status = AHardwareBuffer_createFromHandle(&desc, NN_TRY(hidlHandleFromSharedHandle(handle)),
                                                  AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE,
                                                  &hardwareBuffer);
        if (status == NO_ERROR) {
            break;
        }
    }
    if (status != NO_ERROR) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
               << "Can't create AHardwareBuffer from handle. Error: " << status;
    }

    void* data = nullptr;
    status = AHardwareBuffer_lock(hardwareBuffer, usage, -1, nullptr, &data);
    if (status != NO_ERROR) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
               << "Can't lock the AHardwareBuffer. Error: " << status;
        // TODO(b/169166682): do we need to call AHardwareBuffer_release?
    }

    // Create shared scoped object to munmap.
    auto scoped = base::make_scope_guard([hardwareBuffer] {
        AHardwareBuffer_unlock(hardwareBuffer, nullptr);
        if (hardwareBuffer != nullptr) {
            AHardwareBuffer_release(hardwareBuffer);
        }
    });
    auto sharedScoped = std::make_shared<decltype(scoped)>(std::move(scoped));

    return Mapping{.pointer = data, .size = size, .context = std::move(sharedScoped)};
}

GeneralResult<Mapping> mapAhwbMemory(const Memory& /*memory*/) {
    return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
           << "Unable to map non-BLOB AHardwareBuffer memory";
}

#endif  // NN_NO_AHWB

}  // namespace

GeneralResult<Memory> createSharedMemory(size_t size) {
#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
    const auto memory = NN_TRY(allocateSharedMemory(size));
    return createSharedMemoryFromHidlMemory(memory);
#else
    (void)size;
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Unimplemented";
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD
}

GeneralResult<Memory> createSharedMemoryFromFd(size_t size, int prot, int fd, size_t offset) {
    if (size == 0 || fd < 0) {
        return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Invalid size or fd";
    }

    // Duplicate the file descriptor so the resultant Memory owns its own version.
    int dupFd = dup(fd);
    if (dupFd == -1) {
        // TODO(b/120417090): is ANEURALNETWORKS_UNEXPECTED_NULL the correct error to return here?
        return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Failed to dup the fd";
    }

    std::vector<base::unique_fd> fds;
    fds.emplace_back(dupFd);

    const auto [lowOffsetBits, highOffsetBits] = getIntsFromOffset(offset);
    std::vector<int> ints = {prot, lowOffsetBits, highOffsetBits};

    SharedHandle handle = std::make_shared<const Handle>(Handle{
            .fds = std::move(fds),
            .ints = std::move(ints),
    });
    return Memory{.handle = std::move(handle), .size = size, .name = "mmap_fd"};
}

GeneralResult<Memory> createSharedMemoryFromHidlMemory(const hardware::hidl_memory& memory) {
#ifndef NN_NO_AHWB
    return createMemory(memory);
#else
    (void)memory;
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "hidl_memory not supported";
#endif  // NN_NO_AHWB
}

GeneralResult<Memory> createSharedMemoryFromAHWB(const AHardwareBuffer& ahwb) {
#ifndef NN_NO_AHWB
    AHardwareBuffer_Desc bufferDesc;
    AHardwareBuffer_describe(&ahwb, &bufferDesc);
    const native_handle_t* handle = AHardwareBuffer_getNativeHandle(&ahwb);

    if (bufferDesc.format == AHARDWAREBUFFER_FORMAT_BLOB) {
        return Memory{
                .handle = NN_TRY(sharedHandleFromNativeHandle(handle)),
                .size = bufferDesc.width,
                .name = "hardware_buffer_blob",
        };
    }

    // memory size is not used for non-BLOB AHWB memory.
    return Memory{
            .handle = NN_TRY(sharedHandleFromNativeHandle(handle)),
            .size = 0,
            .name = "hardware_buffer",
    };
#else
    (void)ahwb;
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT)
           << "AHardwareBuffer memory not implemented for support library build yet";
#endif  // NN_NO_AHWB
}

GeneralResult<Mapping> map(const Memory& memory) {
    if (memory.name == "mmap_fd") {
        return mapMemFd(memory);
    }
#ifndef NN_COMPATIBILITY_LIBRARY_BUILD
    if (memory.name == "ashmem") {
        return mapAshmem(memory);
    }
    if (memory.name == "hardware_buffer_blob") {
        return mapAhwbBlobMemory(memory);
    }
    if (memory.name == "hardware_buffer") {
        return mapAhwbMemory(memory);
    }
#endif  // NN_COMPATIBILITY_LIBRARY_BUILD
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Cannot map unknown memory " << memory.name;
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
