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
 * @addtogroup NeuralNetworks
 * @{
 */

/**
 * @file NeuralNetworksShim.h
 */

#pragma once

/******************************************************************
 *
 * IMPORTANT NOTICE:
 *
 *   This file is part of Android's set of stable system headers
 *   exposed by the Android NDK (Native Development Kit).
 *
 *   Third-party source AND binary code relies on the definitions
 *   here to be FROZEN ON ALL UPCOMING PLATFORM RELEASES.
 *
 *   - DO NOT MODIFY ENUMS (EXCEPT IF YOU ADD NEW 32-BIT VALUES)
 *   - DO NOT MODIFY CONSTANTS OR FUNCTIONAL MACROS
 *   - DO NOT CHANGE THE SIGNATURE OF FUNCTIONS IN ANY WAY
 *   - DO NOT CHANGE THE LAYOUT OR SIZE OF STRUCTURES
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

#include "NeuralNetworksSupportLibraryImpl.h"

__BEGIN_DECLS

/**
 * Result codes.
 */
typedef enum {
    ANNSHIM_NO_ERROR = 0,

    /**
     * Failure caused by failure to load support library driver.
     */
    ANNSHIM_FAILED_TO_LOAD_SL = 1,

    /**
     * Failure caused by failure to register HAL service.
     */
    ANNSHIM_FAILED_TO_REGISTER_SERVICE = 2,

    /**
     * General failure.
     */
    ANNSHIM_GENERAL_ERROR = 3,

    /**
     * Invalid argument
     */
    ANNSHIM_INVALID_ARGUMENT = 4,

} ANeuralNetworksShimResultCode;

/**
 * Information about expected time and power usage.
 * See NNAPI HAL IDevice::getCapabilities for more information
 */
typedef struct {
    float execTime;
    float powerUsage;
} ANeuralNetworksShimPerformanceInfo;

/**
 * Information about expected time and power usage for a datatype.
 * See NNAPI HAL IDevice::getCapabilities for more information
 */
typedef struct {
    int32_t operandType;
    ANeuralNetworksShimPerformanceInfo performanceInfo;
} ANeuralNetworksShimOperandPerformanceInfo;

/**
 * Information about NNAPI Vendor extension operand type.
 * See NNAPI HAL ExtensionOperandTypeInformation for more information
 */
typedef struct {
    uint32_t byteSize;
    uint16_t type;
    bool isTensor;
} ANeuralNetworksShimExtensionOperandTypeInformation;

/**
 * Supplementary information required to expose NNAPI HAL Service in top of
 * a NNAPI SL Driver.
 */
typedef struct ANeuralNetworksShimDeviceInfo ANeuralNetworksShimDeviceInfo;

/**
 * Allocate ANeuralNetworksShimDeviceInfo struct with a device name.
 *
 * @param deviceInfo The {@link ANeuralNetworksShimDeviceInfo} to be created.
 *                   Set to NULL if unsuccessful.
 * @param deviceName has to match NNAPI Device name exposed by SL Driver.
 * @param serviceName name of the AIDL service backed by this SL Driver device.
 *                    If null, the deviceName will be used as the service name.
 * @return {@link ANeuralNetworksShimResultCode} enum values.
 *         Returns ANNSHIM_NO_ERROR if successful.
 */
int ANeuralNetworksShimDeviceInfo_create(ANeuralNetworksShimDeviceInfo** deviceInfo,
                                         const char* deviceName, const char* serviceName)
        __INTRODUCED_IN(31);

/**
 * Free ANeuralNetworksShimDeviceInfo struct.
 *
 * @param deviceInfo The NNAPI shim device info to be destroyed. Passing NULL is acceptable and
 *                   results in no operation.
 */
void ANeuralNetworksShimDeviceInfo_free(ANeuralNetworksShimDeviceInfo* deviceInfo)
        __INTRODUCED_IN(31);

/**
 * Set NNAPI Device compilation caching Information.
 * See NNAPI HAL IDevice::getNumberOfCacheFilesNeeded for more information.
 *
 * @param deviceInfo The NNAPI shim device info to be modified.
 * @param numDataCacheFiles Value returned in NNAPI HAL IDevice::getNumberOfCacheFilesNeeded for
 *                          data files.
 * @param numModelCacheFiles Value returned in NNAPI HAL IDevice::getNumberOfCacheFilesNeeded for
 *                           model files.

 * @return {@link ANeuralNetworksShimResultCode} enum values.
 *         Returns ANNSHIM_NO_ERROR if successful.
 *         Returns ANNSHIM_INVALID_ARGUMENT if any num param set to more than NNAPI HAL
 *         IDevice::MAX_NUMBER_OF_CACHE_FILES (32).
 */
int ANeuralNetworksShimDeviceInfo_setNumberOfCacheFilesNeeded(
        ANeuralNetworksShimDeviceInfo* deviceInfo, uint32_t numDataCacheFiles,
        uint32_t numModelCacheFiles) __INTRODUCED_IN(31);

/**
 * Set NNAPI Device performance/power capabilities.
 * See NNAPI HAL IDevice::getCapabilities for more information
 *
 * @param deviceInfo The NNAPI shim device info to be modified.
 * @param ifPerformanced Value returned in NNAPI HAL IDevice::getCapabilities()->ifPerformance.
 * @param whilePerformanced Value returned in NNAPI HAL
 *                          IDevice::getCapabilities()c->whilePerformance.
 * @param relaxedFloat32toFloat16PerformanceScalar
 *        Value returned in NNAPI HAL IDevice::getCapabilities()
 *        relaxedFloat32toFloat16PerformanceScalar.
 * @param relaxedFloat32toFloat16PerformanceTensor
 *        Value returned in NNAPI HAL IDevice::getCapabilities()
 *        relaxedFloat32toFloat16PerformanceTensor.
 * @param operandPerformanceCount Number of entries in operandPerformance param
 * @param operandPerformance Sorted array (by ascending operandType in
 *                           ANeuralNetworksShimOperandPerformanceInfo) with values to be
 *                           returned in NNAPI HAL IDevice::getCapabilities()
 *                           operandPerformance. It's illegal to use operandType with
 *                           ANEURALNETWORKS_MODEL value.
 * @return {@link ANeuralNetworksShimResultCode} enum values.
 *         Returns ANNSHIM_NO_ERROR if successful.
 *         Returns ANNSHIM_INVALID_ARGUMENT if operandPerformance is ill-ordered or with illegal
 *         operandType values.
 */
int ANeuralNetworksShimDeviceInfo_setCapabilities(
        ANeuralNetworksShimDeviceInfo* deviceInfo, ANeuralNetworksShimPerformanceInfo ifPerformance,
        ANeuralNetworksShimPerformanceInfo whilePerformance,
        ANeuralNetworksShimPerformanceInfo relaxedFloat32toFloat16PerformanceScalar,
        ANeuralNetworksShimPerformanceInfo relaxedFloat32toFloat16PerformanceTensor,
        uint32_t operandPerformanceCount,
        ANeuralNetworksShimOperandPerformanceInfo operandPerformance[]) __INTRODUCED_IN(31);

/**
 * Add NNAPI Device vendor extension to be exposed by the shim.
 * See NNAPI HAL IDevice::getSupportedExtensions for more information
 *
 * @param deviceInfo The NNAPI shim device info to be modified.
 * @param extensionName Value set in NNAPI HAL Extension .name field.
 * @param extensionOperandTypeInformationCount Number of entries in extensionOperandTypeInformation
 *                                             array.
 * @param extensionOperandTypeInformation Value set in NNAPI HAL Extension .operandTypes field.
 * @return {@link ANeuralNetworksShimResultCode} enum values.
 *         Returns ANNSHIM_NO_ERROR if successful.
 */
int ANeuralNetworksShimDeviceInfo_addVendorExtension(
        ANeuralNetworksShimDeviceInfo* deviceInfo, const char* extensionName,
        uint32_t extensionOperandTypeInformationCount,
        ANeuralNetworksShimExtensionOperandTypeInformation extensionOperandTypeInformation[])
        __INTRODUCED_IN(31);

/**
 * Register NNAPI support library driver as HAL services.
 *
 * Takes a NNAPI SL implementation and registers each NNAPI Device it exposes as a
 * separate HAL/AIDL service.
 *
 * If loading SL driver is successful, it blocks and never returns. If there's
 * any problem with the support library driver, it returns on error.
 *
 * @param dlhandle Shared object handle returned by dlopen.
 * @param devicesToRegister Array of ANeuralNetworksShimDeviceInfo structs pointers, with
 *                          name and supplementary info about NNAPI devices to register.
 * @param devicesToRegisterCount Count of named NNAPI devices to register.
 * @return {@link ANeuralNetworksShimResultCode} enum values.
 *         Blocks forever if successful.
 *
 * Available since API level 31.
 */
int ANeuralNetworksShim_registerSupportLibraryService(
        NnApiSLDriverImpl* nnapiImpl, ANeuralNetworksShimDeviceInfo* devicesToRegister[],
        int devicesToRegisterCount) __INTRODUCED_IN(31);

__END_DECLS

/** @} */
