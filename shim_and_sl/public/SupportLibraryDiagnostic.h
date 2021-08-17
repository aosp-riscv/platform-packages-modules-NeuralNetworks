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

/**
 * Support Library Diagnostic API.
 *
 * Allows the client to set diagnotic callbacks, and retrieve diagnotic info
 * on those callbacks.
 *
 * Example usage:
 *
 * // Callback to be invoked whenever a compilation has completed.
 * void compilationCallback(const void* context, ANeuralNetworksDiagnosticCompilationInfo* info) {
 *     // The context object can be used to store state without the use of a global variable.
 *     const ExampleLoggerObject* logger = static_cast<const ExampleLoggerObject*>(context);
 *
 *     // Calls to getters to get the details...
 *     const uint64_t sessionId = ANeuralNetworksDiagnosticCompilationInfo_getSessionId(info);
 *
 *     ...
 *
 *     logger->write(...);
 * }
 *
 * void compilationCallback(const void* context, ANeuralNetworksDiagnosticExecutionInfo* info) {
 *      ...
 * }
 *
 * ExampleLoggerObject exampleLoggerObject;
 * ANeuralNetworksDiagnostic_registerCallbacks(&compilationCallback, &executionCallback,
 *                                             static_cast<const void*>(&exampleLoggerObject));
 */

#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

/**
 * Result codes.
 */
typedef enum {
    ANNDIAG_NO_ERROR = 0,

    /**
     * Failure caused by failure to load support library driver.
     */
    ANNDIAG_FAILED_TO_LOAD_SL = 1,

    /**
     * Failure caused by failure to register HAL service.
     */
    ANNDIAG_FAILED_TO_REGISTER_SERVICE = 2,

    /**
     * General failure.
     */
    ANNDIAG_GENERAL_ERROR = 3,

    /**
     * Invalid argument
     */
    ANNDIAG_INVALID_ARGUMENT = 4,

} ANeuralNetworksDiagnosticResultCode;

/**
 * Data class.
 */
typedef enum {
    ANNDIAG_DATA_CLASS_UNKNOWN = 0,
    ANNDIAG_DATA_CLASS_OTHER = 1,
    ANNDIAG_DATA_CLASS_FLOAT32 = 2,
    ANNDIAG_DATA_CLASS_FLOAT16 = 3,
    ANNDIAG_DATA_CLASS_QUANT = 4,
    ANNDIAG_DATA_CLASS_MIXED = 5
} ANeuralNetworksDiagnosticDataClass;

typedef enum {
    ANNDIAG_EXECUTION_MODE_UNKNOWN = 0,
    ANNDIAG_EXECUTION_MODE_SYNC = 1,
    ANNDIAG_EXECUTION_MODE_BURST = 2,
    ANNDIAG_EXECUTION_MODE_ASYNC_WITH_DEPS = 3,
} ANeuralNetworksDiagnosticExecutionMode;

typedef struct ANeuralNetworksDiagnosticCompilationInfo ANeuralNetworksDiagnosticCompilationInfo;

/**
 * Gets the ID that identifies a single session of client interacting with NNAPI runtime.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Session info id.
 */
int32_t ANeuralNetworksDiagnosticCompilationInfo_getSessionId(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the client process/package name.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Client process/package name.
 */
const char* _Nonnull ANeuralNetworksDiagnosticCompilationInfo_getPackageName(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets NNAPI version.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return NNAPI version.
 */
int64_t ANeuralNetworksDiagnosticCompilationInfo_getNnApiVersion(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the hash of the model architecture (without weights).
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Model hash.
 */
const uint8_t* _Nonnull ANeuralNetworksDiagnosticCompilationInfo_getModelArchHash(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the number of devices used.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Device count.
 */
int32_t ANeuralNetworksDiagnosticCompilationInfo_getDeviceCount(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the device IDs as a comma-concatenated string.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Device ID.
 */
const char* _Nonnull ANeuralNetworksDiagnosticCompilationInfo_getDeviceIds(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the error code.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Error code.
 */
int32_t ANeuralNetworksDiagnosticCompilationInfo_getErrorCode(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the type of tensors used for inputs.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Input data class.
 */
ANeuralNetworksDiagnosticDataClass ANeuralNetworksDiagnosticCompilationInfo_getInputDataClass(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the type of tensors used for outputs.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Output data class.
 */
ANeuralNetworksDiagnosticDataClass ANeuralNetworksDiagnosticCompilationInfo_getOutputDataClass(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets how many nanoseconds elapsed when compiling the model.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Time to compile the model in nanoseconds. UINT64_MAX indicates that timing information is
 *         not available.
 */
uint64_t ANeuralNetworksDiagnosticCompilationInfo_getCompilationTimeNanos(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Did the compilation need to fallback to CPU?
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Whether the compilation needed to fallback to the CPU.
 */
bool ANeuralNetworksDiagnosticCompilationInfo_neededFallbackToCpu(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Is the client compiling with explicit set of devices?
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Whether introspection was used.
 */
bool ANeuralNetworksDiagnosticCompilationInfo_isIntrospectionEnabled(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Is caching enabled?
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Whether caching is enabled.
 */
bool ANeuralNetworksDiagnosticCompilationInfo_isCachingEnabled(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Is control flow used?
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Whether control flow was used.
 */
bool ANeuralNetworksDiagnosticCompilationInfo_isControlFlowUsed(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Are dynamic tensors used?
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 * @return Whether dynamic tensors were used.
 */
bool ANeuralNetworksDiagnosticCompilationInfo_areDynamicTensorsUsed(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

typedef struct ANeuralNetworksDiagnosticExecutionInfo ANeuralNetworksDiagnosticExecutionInfo;

/**
 * Gets the ID that identifies a single session of client interacting with NNAPI runtime.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Session info id.
 */
int32_t ANeuralNetworksDiagnosticExecutionInfo_getSessionId(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the client process/package name.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Client process/package name.
 */
const char* _Nonnull ANeuralNetworksDiagnosticExecutionInfo_getPackageName(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets NNAPI version.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return NNAPI version.
 */
int64_t ANeuralNetworksDiagnosticExecutionInfo_getNnApiVersion(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the hash of the model architecture (without weights).
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Model hash.
 */
const uint8_t* _Nonnull ANeuralNetworksDiagnosticExecutionInfo_getModelArchHash(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the number of devices used.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Device count.
 */
int32_t ANeuralNetworksDiagnosticExecutionInfo_getDeviceCount(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the device IDs as a comma-concatenated string.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Device ID.
 */
const char* _Nonnull ANeuralNetworksDiagnosticExecutionInfo_getDeviceIds(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the execution mode.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Execution mode.
 */
ANeuralNetworksDiagnosticExecutionMode ANeuralNetworksDiagnosticExecutionInfo_getExecutionMode(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the input data class.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Input data class.
 */
ANeuralNetworksDiagnosticDataClass ANeuralNetworksDiagnosticExecutionInfo_getInputDataClass(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the output data class.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Output data class.
 */
ANeuralNetworksDiagnosticDataClass ANeuralNetworksDiagnosticExecutionInfo_getOutputDataClass(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the error code.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Error code.
 */
uint32_t ANeuralNetworksDiagnosticExecutionInfo_getErrorCode(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the time taken to execute from runtime, including runtime/ipc overhead.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Time taken to execute as measured by the runtime in nanoseconds. UINT64_MAX indicates
 *         that timing information is not available.
 */
uint64_t ANeuralNetworksDiagnosticExecutionInfo_getDurationRimetimeNanos(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the time taken to execute in the driver, excluding runtime/ipc overhead.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Time taken to execute on the driver in nanoseconds. UINT64_MAX indicates that timing
 *         information is not available.
 */
uint64_t ANeuralNetworksDiagnosticExecutionInfo_getDriverNanos(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the time taken to execute on the hardware, excluding driver overhead.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Time taken to execute on the hardware in nanoseconds. UINT64_MAX indicates that timing
 *         information is not available.
 */
uint64_t ANeuralNetworksDiagnosticExecutionInfo_getHardwareNanos(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Is the client compiling with explicit set of devices?
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Whether introspection was used.
 */
bool ANeuralNetworksDiagnosticExecutionInfo_isIntrospectionEnabled(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Is caching enabled?
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Whether caching is enabled.
 */
bool ANeuralNetworksDiagnosticExecutionInfo_isCachingEnabled(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Is control flow used?
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Whether control flow was used.
 */
bool ANeuralNetworksDiagnosticExecutionInfo_isControlFlowUsed(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Are dynamic tensors used?
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 * @return Whether dynamic tensors were used.
 */
bool ANeuralNetworksDiagnosticExecutionInfo_areDynamicTensorsUsed(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

typedef void (*ANeuralNetworksDiagnosticCompilationFinishedCallback)(
        const void* _Nullable context,
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull info);

typedef void (*ANeuralNetworksDiagnosticExecutionFinishedCallback)(
        const void* _Nullable context, const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull info);

/**
 * Sets the callbacks to be called when compilations or executions finish.
 *
 * @param compilationCallback The compilation callback to set.
 * @param executionCallback The execution callback to set.
 * @param callbackContext The context to be passed to the callbacks when they are invoked.
 */
void ANeuralNetworksDiagnostic_registerCallbacks(
        ANeuralNetworksDiagnosticCompilationFinishedCallback _Nonnull compilationCallback,
        ANeuralNetworksDiagnosticExecutionFinishedCallback _Nonnull executionCallback,
        const void* _Nullable callbackContext);

__END_DECLS
