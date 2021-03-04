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

#include "SupportLibrary.h"

#include <dlfcn.h>

#include <cstring>
#include <memory>
#include <string>

#define NNAPI_LOG(format, ...) fprintf(stderr, format "\n", __VA_ARGS__);

namespace {

void* LoadFunction(void* handle, const char* name, bool optional) {
    if (handle == nullptr) {
        return nullptr;
    }
    void* fn = dlsym(handle, name);
    if (fn == nullptr && !optional) {
        NNAPI_LOG("nnapi error: unable to open function %s: %s", name, dlerror());
    }
    return fn;
}

#define LOAD_FUNCTION(handle, name) \
    nnapi->name = reinterpret_cast<name##_fn>(LoadFunction(handle, #name, /*optional*/ false));

}  // namespace

NnApiSupportLibrary::~NnApiSupportLibrary() {
    if (lib_handle != nullptr) {
        dlclose(lib_handle);
        lib_handle = nullptr;
    }
}

std::unique_ptr<const NnApiSupportLibrary> LoadNnApiSupportLibrary(const std::string& lib_name) {
    void* lib_handle = nullptr;
    lib_handle = dlopen(lib_name.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (lib_handle == nullptr) {
        NNAPI_LOG("nnapi error: unable to open library %s: %s", lib_name.c_str(), dlerror());
    }

    return LoadNnApiSupportLibrary(lib_handle);
}

std::unique_ptr<const NnApiSupportLibrary> LoadNnApiSupportLibrary(void* lib_handle) {
    auto nnapi = std::make_unique<NnApiSupportLibrary>();

    LOAD_FUNCTION(lib_handle, ANeuralNetworks_getRuntimeFeatureLevel);
    LOAD_FUNCTION(lib_handle, ANeuralNetworks_getDefaultLoopTimeout);
    LOAD_FUNCTION(lib_handle, ANeuralNetworks_getMaximumLoopTimeout);
    LOAD_FUNCTION(lib_handle, ANeuralNetworks_getDeviceCount);
    LOAD_FUNCTION(lib_handle, ANeuralNetworks_getDevice);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksDevice_getName);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksDevice_getType);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksDevice_getVersion);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksDevice_wait);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemory_createFromAHardwareBuffer);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemory_createFromDesc);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemory_createFromFd);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemory_copy);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemory_free);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemoryDesc_addInputRole);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemoryDesc_addOutputRole);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemoryDesc_create);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemoryDesc_finish);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemoryDesc_free);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksMemoryDesc_setDimensions);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_create);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_free);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_finish);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_addOperand);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_setOperandSymmPerChannelQuantParams);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_setOperandValue);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_setOperandValueFromMemory);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_setOperandValueFromModel);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_addOperation);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_identifyInputsAndOutputs);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_relaxComputationFloat32toFloat16);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_getSupportedOperationsForDevices);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksCompilation_createForDevices);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksCompilation_free);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksCompilation_setCaching);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksCompilation_setPreference);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksCompilation_finish);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksCompilation_setPriority);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksCompilation_setTimeout);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_create);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_free);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_getDuration);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_setInput);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_setInputFromMemory);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_setMeasureTiming);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_setOutput);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_setOutputFromMemory);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_compute);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_startComputeWithDependencies);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_getOutputOperandDimensions);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_getOutputOperandRank);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_setTimeout);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_setLoopTimeout);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksEvent_createFromSyncFenceFd);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksEvent_getSyncFenceFd);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksEvent_free);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksEvent_wait);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksBurst_create);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksBurst_free);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksExecution_burstCompute);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksDevice_getExtensionSupport);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_getExtensionOperandType);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_getExtensionOperationType);
    LOAD_FUNCTION(lib_handle, ANeuralNetworksModel_setOperandExtensionData);

    nnapi->lib_handle = lib_handle;
    return nnapi;
}
