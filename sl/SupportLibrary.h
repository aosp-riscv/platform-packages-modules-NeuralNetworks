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

#ifndef ANDROID_PACKAGES_MODULES_NEURALNETWORKS_SL_SUPPORT_LIBRARY_H
#define ANDROID_PACKAGES_MODULES_NEURALNETWORKS_SL_SUPPORT_LIBRARY_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <memory>
#include <string>

#include "SupportLibraryTypes.h"

struct NnApiSupportLibrary {
    void* lib_handle = nullptr;

    ~NnApiSupportLibrary();

    // TODO(181331981): This should be generated from NeuralNetworks.t
    int64_t (*ANeuralNetworks_getRuntimeFeatureLevel)();
    uint64_t (*ANeuralNetworks_getDefaultLoopTimeout)();
    uint64_t (*ANeuralNetworks_getMaximumLoopTimeout)();
    int (*ANeuralNetworks_getDeviceCount)(uint32_t* numDevices);
    int (*ANeuralNetworks_getDevice)(uint32_t devIndex, ANeuralNetworksDevice** device);
    int (*ANeuralNetworksDevice_getName)(const ANeuralNetworksDevice* device, const char** name);
    int (*ANeuralNetworksDevice_getType)(const ANeuralNetworksDevice* device, int32_t* type);
    int (*ANeuralNetworksDevice_getVersion)(const ANeuralNetworksDevice* device,
                                            const char** version);
    int (*ANeuralNetworksDevice_wait)(const ANeuralNetworksDevice* device);
    int (*ANeuralNetworksEvent_wait)(ANeuralNetworksEvent* event);
    int (*ANeuralNetworksMemory_createFromAHardwareBuffer)(const AHardwareBuffer* ahwb,
                                                           ANeuralNetworksMemory** memory);
    int (*ANeuralNetworksMemory_createFromDesc)(const ANeuralNetworksMemoryDesc* desc,
                                                ANeuralNetworksMemory** memory);
    int (*ANeuralNetworksMemory_createFromFd)(size_t size, int protect, int fd, size_t offset,
                                              ANeuralNetworksMemory** memory);
    int (*ANeuralNetworksMemory_copy)(const ANeuralNetworksMemory* src,
                                      const ANeuralNetworksMemory* dst);

    void (*ANeuralNetworksMemory_free)(ANeuralNetworksMemory* memory);
    int (*ANeuralNetworksMemoryDesc_addInputRole)(ANeuralNetworksMemoryDesc* desc,
                                                  const ANeuralNetworksCompilation* compilation,
                                                  int32_t index, float frequency);
    int (*ANeuralNetworksMemoryDesc_addOutputRole)(ANeuralNetworksMemoryDesc* desc,
                                                   const ANeuralNetworksCompilation* compilation,
                                                   uint32_t index, float frequency);
    int (*ANeuralNetworksMemoryDesc_create)(ANeuralNetworksMemoryDesc** desc);
    int (*ANeuralNetworksMemoryDesc_finish)(ANeuralNetworksMemoryDesc* desc);
    void (*ANeuralNetworksMemoryDesc_free)(ANeuralNetworksMemoryDesc* desc);
    int (*ANeuralNetworksMemoryDesc_setDimensions)(ANeuralNetworksMemoryDesc* desc, uint32_t rank,
                                                   const uint32_t* dimensions);
    int (*ANeuralNetworksModel_create)(ANeuralNetworksModel** model);
    void (*ANeuralNetworksModel_free)(ANeuralNetworksModel* model);

    int (*ANeuralNetworksModel_finish)(ANeuralNetworksModel* model);
    int (*ANeuralNetworksModel_addOperand)(ANeuralNetworksModel* model,
                                           const ANeuralNetworksOperandType* type);
    int (*ANeuralNetworksModel_setOperandValue)(ANeuralNetworksModel* model, int32_t index,
                                                const void* buffer, size_t length);

    int (*ANeuralNetworksModel_setOperandSymmPerChannelQuantParams)(
            ANeuralNetworksModel* model, int32_t index,
            const ANeuralNetworksSymmPerChannelQuantParams* channelQuant);
    int (*ANeuralNetworksModel_setOperandValueFromMemory)(ANeuralNetworksModel* model,
                                                          int32_t index,
                                                          const ANeuralNetworksMemory* memory,
                                                          size_t offset, size_t length);

    int (*ANeuralNetworksModel_setOperandValueFromModel)(ANeuralNetworksModel* model, int32_t index,
                                                         const ANeuralNetworksModel* value);
    int (*ANeuralNetworksModel_addOperation)(ANeuralNetworksModel* model,
                                             ANeuralNetworksOperationType type, uint32_t inputCount,
                                             const uint32_t* inputs, uint32_t outputCount,
                                             const uint32_t* outputs);

    int (*ANeuralNetworksModel_identifyInputsAndOutputs)(ANeuralNetworksModel* model,
                                                         uint32_t inputCount,
                                                         const uint32_t* inputs,
                                                         uint32_t outputCount,
                                                         const uint32_t* outputs);
    int (*ANeuralNetworksModel_relaxComputationFloat32toFloat16)(ANeuralNetworksModel* model,
                                                                 bool allow);
    int (*ANeuralNetworksModel_getSupportedOperationsForDevices)(
            const ANeuralNetworksModel* model, const ANeuralNetworksDevice* const* devices,
            uint32_t numDevices, bool* supportedOps);
    int (*ANeuralNetworksCompilation_createForDevices)(ANeuralNetworksModel* model,
                                                       const ANeuralNetworksDevice* const* devices,
                                                       uint32_t numDevices,
                                                       ANeuralNetworksCompilation** compilation);

    void (*ANeuralNetworksCompilation_free)(ANeuralNetworksCompilation* compilation);

    int (*ANeuralNetworksCompilation_setCaching)(ANeuralNetworksCompilation* compilation,
                                                 const char* cacheDir, const uint8_t* token);
    int (*ANeuralNetworksCompilation_setPreference)(ANeuralNetworksCompilation* compilation,
                                                    int32_t preference);

    int (*ANeuralNetworksCompilation_finish)(ANeuralNetworksCompilation* compilation);
    int (*ANeuralNetworksCompilation_setPriority)(ANeuralNetworksCompilation* compilation,
                                                  int priority);
    int (*ANeuralNetworksCompilation_setTimeout)(ANeuralNetworksCompilation* compilation,
                                                 uint64_t duration);

    int (*ANeuralNetworksExecution_create)(ANeuralNetworksCompilation* compilation,
                                           ANeuralNetworksExecution** execution);
    void (*ANeuralNetworksExecution_free)(ANeuralNetworksExecution* execution);

    int (*ANeuralNetworksExecution_getDuration)(const ANeuralNetworksExecution* execution,
                                                int32_t durationCode, uint64_t* duration);

    int (*ANeuralNetworksExecution_setInput)(ANeuralNetworksExecution* execution, int32_t index,
                                             const ANeuralNetworksOperandType* type,
                                             const void* buffer, size_t length);

    int (*ANeuralNetworksExecution_setInputFromMemory)(ANeuralNetworksExecution* execution,
                                                       int32_t index,
                                                       const ANeuralNetworksOperandType* type,
                                                       const ANeuralNetworksMemory* memory,
                                                       size_t offset, size_t length);
    int (*ANeuralNetworksExecution_setMeasureTiming)(ANeuralNetworksExecution* execution,
                                                     bool measure);
    int (*ANeuralNetworksExecution_setOutput)(ANeuralNetworksExecution* execution, int32_t index,
                                              const ANeuralNetworksOperandType* type, void* buffer,
                                              size_t length);
    int (*ANeuralNetworksExecution_setOutputFromMemory)(ANeuralNetworksExecution* execution,
                                                        int32_t index,
                                                        const ANeuralNetworksOperandType* type,
                                                        const ANeuralNetworksMemory* memory,
                                                        size_t offset, size_t length);
    int (*ANeuralNetworksExecution_compute)(ANeuralNetworksExecution* execution);
    int (*ANeuralNetworksExecution_getOutputOperandDimensions)(ANeuralNetworksExecution* execution,
                                                               int32_t index, uint32_t* dimensions);
    int (*ANeuralNetworksExecution_getOutputOperandRank)(ANeuralNetworksExecution* execution,
                                                         int32_t index, uint32_t* rank);
    int (*ANeuralNetworksExecution_setTimeout)(ANeuralNetworksExecution* execution,
                                               uint64_t duration);
    int (*ANeuralNetworksExecution_setLoopTimeout)(ANeuralNetworksExecution* execution,
                                                   uint64_t duration);
    int (*ANeuralNetworksExecution_enableInputAndOutputPadding)(ANeuralNetworksExecution* execution,
                                                                bool enable);
    int (*ANeuralNetworksExecution_setReusable)(ANeuralNetworksExecution* execution, bool reusable);
    int (*ANeuralNetworksEvent_createFromSyncFenceFd)(int sync_fence_fd,
                                                      ANeuralNetworksEvent** event);
    int (*ANeuralNetworksEvent_getSyncFenceFd)(const ANeuralNetworksEvent* event,
                                               int* sync_fence_fd);
    int (*ANeuralNetworksExecution_startComputeWithDependencies)(
            ANeuralNetworksExecution* execution, const ANeuralNetworksEvent* const* dependencies,
            uint32_t num_dependencies, uint64_t duration, ANeuralNetworksEvent** event);
    void (*ANeuralNetworksEvent_free)(ANeuralNetworksEvent* event);
    int (*ANeuralNetworksBurst_create)(ANeuralNetworksCompilation* compilation,
                                       ANeuralNetworksBurst** burst);
    void (*ANeuralNetworksBurst_free)(ANeuralNetworksBurst* burst);
    int (*ANeuralNetworksExecution_burstCompute)(ANeuralNetworksExecution* execution,
                                                 ANeuralNetworksBurst* burst);

    int (*ANeuralNetworksDevice_getExtensionSupport)(const ANeuralNetworksDevice* device,
                                                     const char* extensionName,
                                                     bool* isExtensionSupported);

    int (*ANeuralNetworksModel_getExtensionOperandType)(ANeuralNetworksModel* model,
                                                        const char* extensionName,
                                                        uint16_t operandCodeWithinExtension,
                                                        int32_t* type);
    int (*ANeuralNetworksModel_getExtensionOperationType)(ANeuralNetworksModel* model,
                                                          const char* extensionName,
                                                          uint16_t operationCodeWithinExtension,
                                                          ANeuralNetworksOperationType* type);
    int (*ANeuralNetworksModel_setOperandExtensionData)(ANeuralNetworksModel* model, int32_t index,
                                                        const void* data, size_t length);
    int (*ANeuralNetworksCompilation_getPreferredMemoryAlignmentForInput)(
            const ANeuralNetworksCompilation* compilation, uint32_t index, uint32_t* alignment);
    int (*ANeuralNetworksCompilation_getPreferredMemoryPaddingForInput)(
            const ANeuralNetworksCompilation* compilation, uint32_t index, uint32_t* padding);
    int (*ANeuralNetworksCompilation_getPreferredMemoryAlignmentForOutput)(
            const ANeuralNetworksCompilation* compilation, uint32_t index, uint32_t* alignment);
    int (*ANeuralNetworksCompilation_getPreferredMemoryPaddingForOutput)(
            const ANeuralNetworksCompilation* compilation, uint32_t index, uint32_t* padding);
};

/**
 * Loads the NNAPI support library.
 * The NnApiSupportLibrary structure is filled with all the pointers. If one
 * function doesn't exist, a null pointer is stored.
 */
std::unique_ptr<const NnApiSupportLibrary> LoadNnApiSupportLibrary(const std::string& lib_name);
std::unique_ptr<const NnApiSupportLibrary> LoadNnApiSupportLibrary(void* lib_handle);

#endif  // ANDROID_PACKAGES_MODULES_NEURALNETWORKS_SL_SUPPORT_LIBRARY_H
