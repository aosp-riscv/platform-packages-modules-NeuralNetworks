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
 */

#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

#include "NeuralNetworksSupportLibraryImpl.h"

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

typedef struct ANeuralNetworksDiagnosticCompilationInfo ANeuralNetworksDiagnosticCompilationInfo;

/**
 * Allocates ANeuralNetworksDiagnosticCompilationInfo struct.
 *
 * @param nnapiSupportLibraryPackage Handle to a NNAPI SL implementation.
 * @param outRegistrationParams The {@link ANeuralNetworksDiagnosticCompilationInfo} to be created.
 *                  Set to NULL if unsuccessful.
 * @return {@link ANeuralNetworksShimResultCode} enum values.
 *         Returns ANNDIAG_NO_ERROR if successful.
 */
ANeuralNetworksDiagnosticResultCode ANeuralNetworksDiagnosticCompilationInfo_create(
        NnApiSLDriverImpl* _Nonnull nnapiSupportLibraryPackage,
        ANeuralNetworksDiagnosticCompilationInfo* _Nullable* _Nonnull diagnosticCompilationInfo);

/**
 * Frees ANeuralNetworksDiagnosticCompilationInfo struct.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object to be destroyed.
 *        Passing NULL is acceptable and results in no operation.
 */
void ANeuralNetworksDiagnosticCompilationInfo_free(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the ID that identifies a single session of client interacting with NNAPI runtime.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 */
uint64_t ANeuralNetworksDiagnosticCompilationInfo_getSessionId(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the client process/package name.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 */
const char* ANeuralNetworksDiagnosticCompilationInfo_getPackageName(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets NNAPI version.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 */
uint32_t ANeuralNetworksDiagnosticCompilationInfo_getNnApiVersion(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the hash of the model architecture (without weights).
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 */
const char* ANeuralNetworksDiagnosticCompilationInfo_getModelArchHash(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the number of devices used.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 */
uint32_t ANeuralNetworksDiagnosticCompilationInfo_getDeviceCount(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the device IDs as a comma-concatenated string.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 */
const char* ANeuralNetworksDiagnosticCompilationInfo_getDeviceIds(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Gets the error code.
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 */
uint32_t ANeuralNetworksDiagnosticCompilationInfo_getErrorCode(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Is caching enabled?
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 */
bool ANeuralNetworksDiagnosticCompilationInfo_isCachingEnabled(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Is control flow used?
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 */
bool ANeuralNetworksDiagnosticCompilationInfo_isControlFlowUsed(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

/**
 * Are dynamic tensors used?
 *
 * @param diagnosticCompilationInfo The NNAPI diagnostic compilation info object.
 */
bool ANeuralNetworksDiagnosticCompilationInfo_areDynamicTensorsUsed(
        ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo);

typedef void (*ANeuralNetworksDiagnosticCompilationFinishedCallback)(
        ANeuralNetworksDiagnosticCompilationInfo*);

/**
 * Sets the callback for compilation finished event.
 *
 * @param ANeuralNetworksDiagnosticCompilationFinishedCallback The callback to set.
 */
void ANeuralNetworksDiagnostic_setOnCompilationFinished(
        ANeuralNetworksDiagnosticCompilationFinishedCallback callback);

typedef struct ANeuralNetworksDiagnosticExecutionInfo ANeuralNetworksDiagnosticExecutionInfo;

/**
 * Allocates ANeuralNetworksDiagnosticExecutionInfo struct.
 *
 * @param nnapiSupportLibraryPackage Handle to a NNAPI SL implementation.
 * @param outRegistrationParams The {@link ANeuralNetworksDiagnosticExecutionInfo} to be created.
 *                  Set to NULL if unsuccessful.
 * @return {@link ANeuralNetworksDiagnosticResultCode} enum values.
 *         Returns ANNDIAG_NO_ERROR if successful.
 */
ANeuralNetworksDiagnosticResultCode ANeuralNetworksDiagnosticExecutionInfo_create(
        NnApiSLDriverImpl* _Nonnull nnapiSupportLibraryPackage,
        ANeuralNetworksDiagnosticExecutionInfo* _Nullable* _Nonnull diagnosticExecutionInfo);

/**
 * Frees ANeuralNetworksDiagnosticExecutionInfo struct.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object to be destroyed.
 *        Passing NULL is acceptable and results in no operation.
 */
void ANeuralNetworksDiagnosticExecutionInfo_free(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the ID that identifies a single session of client interacting with NNAPI runtime.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
uint64_t ANeuralNetworksDiagnosticExecutionInfo_getSessionId(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the client process/package name.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
const char* ANeuralNetworksDiagnosticExecutionInfo_getPackageName(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets NNAPI version.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
uint32_t ANeuralNetworksDiagnosticExecutionInfo_getNnApiVersion(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the hash of the model architecture (without weights).
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
const char* ANeuralNetworksDiagnosticExecutionInfo_getModelArchHash(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the number of devices used.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
uint32_t ANeuralNetworksDiagnosticExecutionInfo_getDeviceCount(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the device IDs as a comma-concatenated string.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
const char* ANeuralNetworksDiagnosticExecutionInfo_getDeviceIds(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

typedef enum {
    ANNDIAG_UNKNOWN_EXECUTION_MODE = 0,
    ANNDIAG_SYNC = 1,
    ANNDIAG_BURST = 2,
    ANNDIAG_ASYNC_WITH_DEPS = 3,
} ANeuralNetworksDiagnosticExecutionMode;

/**
 * Gets the execution mode.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
ANeuralNetworksDiagnosticExecutionMode ANeuralNetworksDiagnosticExecutionInfo_getExecutionMode(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

typedef enum {
    ANNDIAG_UNKNOWN_DATA_TYPES = 0,
    ANNDIAG_QUANT = 1,
    ANNDIAG_FLOAT16 = 2,
    ANNDIAG_FLOAT32 = 3,
} ANeuralNetworksDiagnosticDataClass;

/**
 * Gets the input data class.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
ANeuralNetworksDiagnosticDataClass ANeuralNetworksDiagnosticExecutionInfo_getInputDataClass(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the output data class.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
ANeuralNetworksDiagnosticDataClass ANeuralNetworksDiagnosticExecutionInfo_getOutputDataClass(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Gets the error code.
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
uint32_t ANeuralNetworksDiagnosticExecutionInfo_getErrorCode(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Is caching enabled?
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
bool ANeuralNetworksDiagnosticExecutionInfo_isCachingEnabled(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Is control flow used?
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
bool ANeuralNetworksDiagnosticExecutionInfo_isControlFlowUsed(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

/**
 * Are dynamic tensors used?
 *
 * @param diagnosticExecutionInfo The NNAPI diagnostic compilation info object.
 */
bool ANeuralNetworksDiagnosticExecutionInfo_areDynamicTensorsUsed(
        ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo);

typedef void (*ANeuralNetworksDiagnosticExecutionFinishedCallback)(
        ANeuralNetworksDiagnosticExecutionInfo*);

void ANeuralNetworksDiagnostic_setOnExecutionFinished(
        ANeuralNetworksDiagnosticExecutionFinishedCallback callback);

__END_DECLS
