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

#ifndef ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_TYPES_H
#define ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_TYPES_H

#include <android-base/expected.h>
#include <android-base/unique_fd.h>

#include <array>
#include <chrono>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "nnapi/OperandTypes.h"
#include "nnapi/OperationTypes.h"
#include "nnapi/Result.h"

namespace android::nn {

// Forward declarations

class IBuffer;
class IBurst;
class IDevice;
class IPreparedModel;

// Default values

constexpr float kDefaultExecTime = std::numeric_limits<float>::max();
constexpr float kDefaultPowerUsage = std::numeric_limits<float>::max();
constexpr uint32_t kByteSizeOfCacheToken = 32;
constexpr uint32_t kMaxNumberOfCacheFiles = 32;
constexpr uint8_t kExtensionTypeBits = 16;
constexpr uint8_t kExtensionPrefixBits = 16;

// Aliases

using AlignedData = std::max_align_t;
using SharedBuffer = std::shared_ptr<const IBuffer>;
using SharedBurst = std::shared_ptr<const IBurst>;
using SharedDevice = std::shared_ptr<const IDevice>;
using SharedPreparedModel = std::shared_ptr<const IPreparedModel>;

// Canonical types

enum class DeviceStatus {
    AVAILABLE = 0,
    BUSY = 1,
    OFFLINE = 2,
    UNKNOWN = 3,
};

enum class ExecutionPreference {
    LOW_POWER = 0,
    FAST_SINGLE_ANSWER = 1,
    SUSTAINED_SPEED = 2,
    DEFAULT = FAST_SINGLE_ANSWER,
};

enum class DeviceType {
    UNKNOWN = 0,
    OTHER = 1,
    CPU = 2,
    GPU = 3,
    ACCELERATOR = 4,
};

enum class MeasureTiming {
    NO = 0,
    YES = 1,
};

enum class Priority {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
    DEFAULT = MEDIUM,
};

// TODO: Should more errors from NeuralNetworks.h be incorporated? The left name shows errors that
// appear in NeuralNetworks.h but not in the HAL, and the right column shows what these values could
// map to:
// * OUT_OF_MEMORY ==> GENERAL_FAILURE / RESOURCE_EXHAUSTED_*
// * INCOMPLETE ==> GENERAL_FAILURE
// * UNEXPECTED_NULL ==> INVALID_ARGUMENT
// * UNMAPPABLE ==> GENERAL_FAILURE
// * BAD_STATE ==> INVALID_ARGUMENT
enum class ErrorStatus {
    NONE = 0,
    DEVICE_UNAVAILABLE = 1,
    GENERAL_FAILURE = 2,
    OUTPUT_INSUFFICIENT_SIZE = 3,
    INVALID_ARGUMENT = 4,
    MISSED_DEADLINE_TRANSIENT = 5,
    MISSED_DEADLINE_PERSISTENT = 6,
    RESOURCE_EXHAUSTED_TRANSIENT = 7,
    RESOURCE_EXHAUSTED_PERSISTENT = 8,
    DEAD_OBJECT = 10000,
};

struct GeneralError {
    std::string message;
    ErrorStatus code = ErrorStatus::GENERAL_FAILURE;
};

template <typename Type>
using GeneralResult = base::expected<Type, GeneralError>;

enum class FusedActivationFunc : int32_t {
    NONE = 0,
    RELU = 1,
    RELU1 = 2,
    RELU6 = 3,
};

using Dimension = uint32_t;
using Dimensions = std::vector<Dimension>;

using CacheToken = std::array<uint8_t, kByteSizeOfCacheToken>;

struct OutputShape {
    std::vector<uint32_t> dimensions;
    bool isSufficient = false;
};

struct ExecutionError {
    std::string message;
    ErrorStatus code = ErrorStatus::GENERAL_FAILURE;
    // OutputShapes for code == OUTPUT_INSUFFICIENT_SIZE
    std::vector<OutputShape> outputShapes = {};
};

template <typename Type>
using ExecutionResult = base::expected<Type, ExecutionError>;

struct Capabilities {
    struct PerformanceInfo {
        float execTime = kDefaultExecTime;
        float powerUsage = kDefaultPowerUsage;
    };
    struct OperandPerformance {
        OperandType type{};
        PerformanceInfo info;
    };
    class OperandPerformanceTable {
       public:
        static Result<OperandPerformanceTable> create(
                std::vector<OperandPerformance> operandPerformances);

        PerformanceInfo lookup(OperandType type) const;
        const std::vector<OperandPerformance>& asVector() const;

       private:
        explicit OperandPerformanceTable(std::vector<OperandPerformance> operandPerformances);
        std::vector<OperandPerformance> mSorted;
    };

    PerformanceInfo relaxedFloat32toFloat16PerformanceScalar;
    PerformanceInfo relaxedFloat32toFloat16PerformanceTensor;
    OperandPerformanceTable operandPerformance;
    PerformanceInfo ifPerformance;
    PerformanceInfo whilePerformance;
};

struct Extension {
    struct OperandTypeInformation {
        uint16_t type = 0;
        bool isTensor = false;
        uint32_t byteSize = 0;
    };

    std::string name;
    std::vector<OperandTypeInformation> operandTypes;
};

struct Operation {
    OperationType type{};
    std::vector<uint32_t> inputs;
    std::vector<uint32_t> outputs;
};

struct DataLocation {
    std::variant<const void*, void*> pointer;
    uint32_t poolIndex = 0;
    uint32_t offset = 0;
    uint32_t length = 0;
};

struct Operand {
    enum class LifeTime {
        TEMPORARY_VARIABLE = 0,
        SUBGRAPH_INPUT = 1,
        SUBGRAPH_OUTPUT = 2,
        CONSTANT_COPY = 3,
        CONSTANT_REFERENCE = 4,
        NO_VALUE = 5,
        SUBGRAPH = 6,
        POINTER = 7,
    };
    using NoParams = std::monostate;
    struct SymmPerChannelQuantParams {
        std::vector<float> scales;
        uint32_t channelDim = 0;
    };
    using ExtensionParams = std::vector<uint8_t>;
    using ExtraParams = std::variant<NoParams, SymmPerChannelQuantParams, ExtensionParams>;

    OperandType type{};
    Dimensions dimensions;
    float scale = 0.0f;
    int32_t zeroPoint = 0;
    LifeTime lifetime{};
    DataLocation location;
    ExtraParams extraParams;
};

struct Handle {
    std::vector<base::unique_fd> fds;
    std::vector<int> ints;
};

using SharedHandle = std::shared_ptr<const Handle>;

struct Memory {
    SharedHandle handle;
    size_t size = 0;
    std::string name;
};

struct Model {
    struct Subgraph {
        std::vector<Operand> operands;
        std::vector<Operation> operations;
        std::vector<uint32_t> inputIndexes;
        std::vector<uint32_t> outputIndexes;
    };
    class OperandValues {
       public:
        OperandValues();
        OperandValues(const uint8_t* data, size_t length);

        DataLocation append(const uint8_t* data, size_t length);

        const uint8_t* data() const;
        size_t size() const;

       private:
        std::vector<AlignedData> mData;
    };
    struct ExtensionNameAndPrefix {
        std::string name;
        uint16_t prefix = 0;
    };

    Subgraph main;
    std::vector<Subgraph> referenced;
    OperandValues operandValues;
    std::vector<Memory> pools;
    bool relaxComputationFloat32toFloat16 = false;
    std::vector<ExtensionNameAndPrefix> extensionNameToPrefix;
};

struct BufferDesc {
    Dimensions dimensions;
};

struct BufferRole {
    uint32_t modelIndex = 0;
    uint32_t ioIndex = 0;
    float frequency = 0.0f;
};

struct Request {
    struct Argument {
        enum class LifeTime {
            POOL = 0,
            NO_VALUE = 1,
            POINTER = 2,
        };

        LifeTime lifetime{};
        DataLocation location;
        Dimensions dimensions;
    };
    enum class MemoryDomainToken : uint32_t {};
    using MemoryPool = std::variant<Memory, MemoryDomainToken, SharedBuffer>;

    std::vector<Argument> inputs;
    std::vector<Argument> outputs;
    std::vector<MemoryPool> pools;
};

// Representation of sync_fence.
class SyncFence {
   public:
    static SyncFence createAsSignaled();
    static SyncFence create(base::unique_fd fd);
    static Result<SyncFence> create(SharedHandle syncFence);

    // The function syncWait() has the same semantics as the system function
    // ::sync_wait(), except that the syncWait() return value is semantically
    // richer.
    enum class FenceState {
        ACTIVE,    // fence has not been signaled
        SIGNALED,  // fence has been signaled
        ERROR,     // fence has been placed in the error state
        UNKNOWN,   // either bad argument passed to syncWait(), or internal error
    };
    using Timeout = std::chrono::duration<int, std::milli>;
    using OptionalTimeout = std::optional<Timeout>;

    FenceState syncWait(OptionalTimeout optionalTimeout) const;

    SharedHandle getSharedHandle() const;
    bool hasFd() const;
    int getFd() const;

   private:
    explicit SyncFence(SharedHandle syncFence);

    SharedHandle mSyncFence;
};

using Clock = std::chrono::steady_clock;

using Duration = std::chrono::duration<uint64_t, std::nano>;
using OptionalDuration = std::optional<Duration>;

using TimePoint = std::chrono::time_point<Clock, Duration>;
using OptionalTimePoint = std::optional<TimePoint>;

struct Timing {
    OptionalDuration timeOnDevice;
    OptionalDuration timeInDriver;
};

enum class Version { ANDROID_OC_MR1, ANDROID_P, ANDROID_Q, ANDROID_R, ANDROID_S, CURRENT_RUNTIME };

}  // namespace android::nn

#endif  // ANDROID_FRAMEWORKS_ML_NN_COMMON_NNAPI_TYPES_H
