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

#include "SupportLibraryDiagnostic.h"

#include <android-base/logging.h>

#include <functional>
#include <utility>

#include "ExecutionBuilder.h"
#include "Telemetry.h"

namespace {

using android::nn::ExecutionMode;
using android::nn::telemetry::DataClass;
using android::nn::telemetry::DiagnosticCompilationInfo;
using android::nn::telemetry::DiagnosticExecutionInfo;

const DiagnosticCompilationInfo* castTo(const ANeuralNetworksDiagnosticCompilationInfo* info) {
    CHECK(info != nullptr);
    return reinterpret_cast<const DiagnosticCompilationInfo*>(info);
}

const DiagnosticExecutionInfo* castTo(const ANeuralNetworksDiagnosticExecutionInfo* info) {
    CHECK(info != nullptr);
    return reinterpret_cast<const DiagnosticExecutionInfo*>(info);
}

const ANeuralNetworksDiagnosticCompilationInfo* castFrom(const DiagnosticCompilationInfo* info) {
    CHECK(info != nullptr);
    return reinterpret_cast<const ANeuralNetworksDiagnosticCompilationInfo*>(info);
}

const ANeuralNetworksDiagnosticExecutionInfo* castFrom(const DiagnosticExecutionInfo* info) {
    CHECK(info != nullptr);
    return reinterpret_cast<const ANeuralNetworksDiagnosticExecutionInfo*>(info);
}

ANeuralNetworksDiagnosticDataClass convert(DataClass dataClass) {
    switch (dataClass) {
        case DataClass::UNKNOWN:
            return ANNDIAG_DATA_CLASS_UNKNOWN;
        case DataClass::OTHER:
            return ANNDIAG_DATA_CLASS_OTHER;
        case DataClass::FLOAT32:
            return ANNDIAG_DATA_CLASS_FLOAT32;
        case DataClass::FLOAT16:
            return ANNDIAG_DATA_CLASS_FLOAT16;
        case DataClass::QUANT:
            return ANNDIAG_DATA_CLASS_QUANT;
        case DataClass::MIXED:
            return ANNDIAG_DATA_CLASS_MIXED;
    }
    LOG(FATAL) << "Unrecognized DataClass " << static_cast<int32_t>(dataClass);
    return ANNDIAG_DATA_CLASS_UNKNOWN;
}

ANeuralNetworksDiagnosticExecutionMode convert(ExecutionMode executionMode) {
    switch (executionMode) {
        case ExecutionMode::ASYNC:
            return ANNDIAG_EXECUTION_MODE_UNKNOWN;
        case ExecutionMode::SYNC:
            return ANNDIAG_EXECUTION_MODE_SYNC;
        case ExecutionMode::BURST:
            return ANNDIAG_EXECUTION_MODE_BURST;
        case ExecutionMode::ASYNC_WITH_DEPS:
            return ANNDIAG_EXECUTION_MODE_ASYNC_WITH_DEPS;
    }
    LOG(FATAL) << "Unrecognized ExecutionMode " << static_cast<int32_t>(executionMode);
    return ANNDIAG_EXECUTION_MODE_UNKNOWN;
}

}  // namespace

int32_t ANeuralNetworksDiagnosticCompilationInfo_getSessionId(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->sessionId;
}

const char* _Nonnull ANeuralNetworksDiagnosticCompilationInfo_getPackageName(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->packageName;
}

int64_t ANeuralNetworksDiagnosticCompilationInfo_getNnApiVersion(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->versionNnapiModule;
}

const uint8_t* _Nonnull ANeuralNetworksDiagnosticCompilationInfo_getModelArchHash(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->modelArchHash;
}

int32_t ANeuralNetworksDiagnosticCompilationInfo_getDeviceCount(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->deviceCount;
}

const char* _Nonnull ANeuralNetworksDiagnosticCompilationInfo_getDeviceIds(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->deviceId;
}

int32_t ANeuralNetworksDiagnosticCompilationInfo_getErrorCode(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->errorCode;
}

ANeuralNetworksDiagnosticDataClass ANeuralNetworksDiagnosticCompilationInfo_getInputDataClass(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return convert(castTo(diagnosticCompilationInfo)->inputDataClass);
}

ANeuralNetworksDiagnosticDataClass ANeuralNetworksDiagnosticCompilationInfo_getOutputDataClass(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return convert(castTo(diagnosticCompilationInfo)->outputDataClass);
}

uint64_t ANeuralNetworksDiagnosticCompilationInfo_getCompilationTimeNanos(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->compilationTimeNanos;
}

bool ANeuralNetworksDiagnosticCompilationInfo_neededFallbackToCpu(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->fallbackToCpuFromError;
}

bool ANeuralNetworksDiagnosticCompilationInfo_isIntrospectionEnabled(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->introspectionEnabled;
}

bool ANeuralNetworksDiagnosticCompilationInfo_isCachingEnabled(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->cacheEnabled;
}

bool ANeuralNetworksDiagnosticCompilationInfo_isControlFlowUsed(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->hasControlFlow;
}

bool ANeuralNetworksDiagnosticCompilationInfo_areDynamicTensorsUsed(
        const ANeuralNetworksDiagnosticCompilationInfo* _Nonnull diagnosticCompilationInfo) {
    return castTo(diagnosticCompilationInfo)->hasDynamicTemporaries;
}

int32_t ANeuralNetworksDiagnosticExecutionInfo_getSessionId(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->sessionId;
}

const char* _Nonnull ANeuralNetworksDiagnosticExecutionInfo_getPackageName(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->packageName;
}

int64_t ANeuralNetworksDiagnosticExecutionInfo_getNnApiVersion(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->versionNnapiModule;
}

const uint8_t* _Nonnull ANeuralNetworksDiagnosticExecutionInfo_getModelArchHash(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->modelArchHash;
}

int32_t ANeuralNetworksDiagnosticExecutionInfo_getDeviceCount(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->deviceCount;
}

const char* _Nonnull ANeuralNetworksDiagnosticExecutionInfo_getDeviceIds(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->deviceId;
}

ANeuralNetworksDiagnosticExecutionMode ANeuralNetworksDiagnosticExecutionInfo_getExecutionMode(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return convert(castTo(diagnosticExecutionInfo)->executionMode);
}

ANeuralNetworksDiagnosticDataClass ANeuralNetworksDiagnosticExecutionInfo_getInputDataClass(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return convert(castTo(diagnosticExecutionInfo)->inputDataClass);
}

ANeuralNetworksDiagnosticDataClass ANeuralNetworksDiagnosticExecutionInfo_getOutputDataClass(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return convert(castTo(diagnosticExecutionInfo)->outputDataClass);
}

uint32_t ANeuralNetworksDiagnosticExecutionInfo_getErrorCode(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->errorCode;
}

uint64_t ANeuralNetworksDiagnosticExecutionInfo_getDurationRimetimeNanos(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->durationRuntimeNanos;
}

uint64_t ANeuralNetworksDiagnosticExecutionInfo_getDriverNanos(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->durationDriverNanos;
}

uint64_t ANeuralNetworksDiagnosticExecutionInfo_getHardwareNanos(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->durationHardwareNanos;
}

bool ANeuralNetworksDiagnosticExecutionInfo_isIntrospectionEnabled(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->introspectionEnabled;
}

bool ANeuralNetworksDiagnosticExecutionInfo_isCachingEnabled(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->cacheEnabled;
}

bool ANeuralNetworksDiagnosticExecutionInfo_isControlFlowUsed(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->hasControlFlow;
}

bool ANeuralNetworksDiagnosticExecutionInfo_areDynamicTensorsUsed(
        const ANeuralNetworksDiagnosticExecutionInfo* _Nonnull diagnosticExecutionInfo) {
    return castTo(diagnosticExecutionInfo)->hasDynamicTemporaries;
}

void ANeuralNetworksDiagnostic_registerCallbacks(
        ANeuralNetworksDiagnosticCompilationFinishedCallback compilationCallback,
        ANeuralNetworksDiagnosticExecutionFinishedCallback executionCallback,
        const void* callbackContext) {
    using android::nn::telemetry::registerTelemetryCallbacks;

    std::function<void(const DiagnosticCompilationInfo*)> compilation =
            [compilationCallback, callbackContext](const DiagnosticCompilationInfo* info) {
                compilationCallback(callbackContext, castFrom(info));
            };
    std::function<void(const DiagnosticExecutionInfo*)> execution =
            [executionCallback, callbackContext](const DiagnosticExecutionInfo* info) {
                executionCallback(callbackContext, castFrom(info));
            };

    registerTelemetryCallbacks(std::move(compilation), std::move(execution));
}
