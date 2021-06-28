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

#include <mutex>
#include <thread>

#include "SupportLibraryDiagnostic.h"
#include "DiagnosticUtils.h"

namespace {

ANeuralNetworksDiagnosticCompilationFinishedCallback gCompilationFinishedCallback = nullptr;
ANeuralNetworksDiagnosticExecutionFinishedCallback gExecutionFinishedCallback = nullptr;
std::mutex gMutex;

}  // namespace

void ANeuralNetworksDiagnostic_setOnCompilationFinished(
        ANeuralNetworksDiagnosticCompilationFinishedCallback callback) {
    const std::lock_guard<std::mutex> lock(gMutex);
    gCompilationFinishedCallback = callback;
}

ANeuralNetworksDiagnosticCompilationFinishedCallback getOnCompilationFinishedCallback() {
    return gCompilationFinishedCallback;
}

void ANeuralNetworksDiagnostic_setOnExecutionFinished(
        ANeuralNetworksDiagnosticExecutionFinishedCallback callback) {
    const std::lock_guard<std::mutex> lock(gMutex);
    gExecutionFinishedCallback = callback;
}

ANeuralNetworksDiagnosticExecutionFinishedCallback getOnExecutionFinishedCallback() {
    return gExecutionFinishedCallback;
}
