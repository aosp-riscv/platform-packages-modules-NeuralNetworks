/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ANDROID_ML_NN_COMMON_UTILS_H
#define ANDROID_ML_NN_COMMON_UTILS_H

#include "HalInterfaces.h"
#include "NeuralNetworks.h"
#include "ValidateHal.h"

#include <android-base/logging.h>
#include <vector>

namespace android {
namespace nn {

// The number of data types (OperandCode) defined in NeuralNetworks.h.
const int kNumberOfDataTypes = 13;

// The number of operation types (OperationCode) defined in NeuralNetworks.h.
const int kNumberOfOperationTypes = 95;

// The number of execution preferences defined in NeuralNetworks.h.
const int kNumberOfPreferences = 3;

// The number of data types (OperandCode) defined in NeuralNetworksOEM.h.
const int kNumberOfDataTypesOEM = 2;

// The number of operation types (OperationCode) defined in NeuralNetworksOEM.h.
const int kNumberOfOperationTypesOEM = 1;

// The lowest number assigned to any OEM Code in NeuralNetworksOEM.h.
const int kOEMCodeBase = 10000;

/* IMPORTANT: if you change the following list, don't
 * forget to update the corresponding 'tags' table in
 * the initVlogMask() function implemented in Utils.cpp.
 */
enum VLogFlags {
    MODEL = 0,
    COMPILATION,
    EXECUTION,
    CPUEXE,
    MANAGER,
    DRIVER
};

#define VLOG_IS_ON(TAG) \
    ((vLogMask & (1 << (TAG))) != 0)

#define VLOG(TAG)         \
    if (LIKELY(!VLOG_IS_ON(TAG))) \
        ;                 \
    else                  \
        LOG(INFO)

extern int vLogMask;
void initVLogMask();

#ifdef NN_DEBUGGABLE
#define SHOW_IF_DEBUG(msg) msg
#else
#define SHOW_IF_DEBUG(msg) ""
#endif

// DEPRECATED(b/118737105). Use CHECK.
#define nnAssert(v) CHECK(v)

#define NN_RETURN_IF_ERROR(expr)                      \
    do {                                              \
        int _errorCode = (expr);                      \
        if (_errorCode != ANEURALNETWORKS_NO_ERROR) { \
            return _errorCode;                        \
        }                                             \
    } while (0)

// The NN_RET_CHECK family of macros defined below is similar to the CHECK family defined in
// system/core/base/include/android-base/logging.h
//
// The difference is that NN_RET_CHECK macros use LOG(ERROR) instead of LOG(FATAL)
// and return false instead of aborting.

// Logs an error and returns false. Append context using << after. For example:
//
//   NN_RET_CHECK_FAIL() << "Something went wrong";
//
// The containing function must return a bool.
#define NN_RET_CHECK_FAIL() return ::android::nn::FalseyErrorStream() << "NN_RET_CHECK failed: "

// Logs an error and returns false if condition is false. Extra logging can be appended using <<
// after. For example:
//
//   NN_RET_CHECK(false) << "Something went wrong";
//
// The containing function must return a bool.
#define NN_RET_CHECK(condition) \
    while (UNLIKELY(!(condition))) NN_RET_CHECK_FAIL() << #condition << " "

// Helper for NN_CHECK_xx(x, y) macros.
#define NN_RET_CHECK_OP(LHS, RHS, OP)                                                 \
    for (auto _values = ::android::base::MakeEagerEvaluator(LHS, RHS);                \
         UNLIKELY(!(_values.lhs OP _values.rhs));                                     \
         /* empty */)                                                                 \
    NN_RET_CHECK_FAIL() << #LHS << " " << #OP << " " << #RHS << " (" << #LHS << " = " \
                        << _values.lhs << ", " << #RHS << " = " << _values.rhs << ") "

// Logs an error and returns false if a condition between x and y does not hold. Extra logging can
// be appended using << after. For example:
//
//   NN_RET_CHECK_EQ(a, b) << "Something went wrong";
//
// The values must implement the appropriate comparison operator as well as
// `operator<<(std::ostream&, ...)`.
// The containing function must return a bool.
#define NN_RET_CHECK_EQ(x, y) NN_RET_CHECK_OP(x, y, ==)
#define NN_RET_CHECK_NE(x, y) NN_RET_CHECK_OP(x, y, !=)
#define NN_RET_CHECK_LE(x, y) NN_RET_CHECK_OP(x, y, <=)
#define NN_RET_CHECK_LT(x, y) NN_RET_CHECK_OP(x, y, <)
#define NN_RET_CHECK_GE(x, y) NN_RET_CHECK_OP(x, y, >=)
#define NN_RET_CHECK_GT(x, y) NN_RET_CHECK_OP(x, y, >)

// A wrapper around LOG(ERROR) that can be implicitly converted to bool (always evaluates to false).
// Used to implement stream logging in NN_RET_CHECK.
class FalseyErrorStream {
    DISALLOW_COPY_AND_ASSIGN(FalseyErrorStream);

   public:
    FalseyErrorStream() {}

    template <typename T>
    FalseyErrorStream& operator<<(const T& value) {
        mBuffer << value;
        return *this;
    }

    ~FalseyErrorStream() { LOG(ERROR) << mBuffer.str(); }

    operator bool() const { return false; }

   private:
    std::ostringstream mBuffer;
};

// Returns true if an operand type is an extension type.
bool isExtensionOperandType(OperandType type);

// Returns true if an operation type is an extension type.
bool isExtensionOperationType(OperationType type);

// Returns the amount of space needed to store a value of the specified
// dimensions and type. For a tensor with at least one
// unspecified dimension, returns zero.
uint32_t sizeOfData(OperandType type, const std::vector<uint32_t>& dimensions);

// Returns the amount of space needed to store a value of the dimensions and
// type of this operand.
inline uint32_t sizeOfData(const Operand& operand) {
    return sizeOfData(operand.type, operand.dimensions);
}

// Returns the name of the operation type in ASCII.
std::string getOperationName(OperationType opCode);

// Returns the name of the operand type in ASCII.
std::string getOperandTypeName(OperandType type);

// Whether a operand has a tensor type that is not fully specified.
bool hasUnspecifiedDimensions(int type, const uint32_t* dim, uint32_t dimCount);
bool hasUnspecifiedDimensions(const Operand& operand);
bool hasUnspecifiedDimensions(const ANeuralNetworksOperandType* type);

// Memory is unmapped.
// Memory is reference counted by hidl_memory instances, and is deallocated
// once there are no more references.
hidl_memory allocateSharedMemory(int64_t size);

// Returns the number of padding bytes needed to align data of the
// specified length.  It aligns object of length:
// 2, 3 on a 2 byte boundary,
// 4+ on a 4 byte boundary.
// We may want to have different alignments for tensors.
// TODO: This is arbitrary, more a proof of concept.  We need
// to determine what this should be.
uint32_t alignBytesNeeded(uint32_t index, size_t length);

// Does a detailed LOG(INFO) of the model
void logModelToInfo(const V1_0::Model& model);
void logModelToInfo(const V1_1::Model& model);
void logModelToInfo(const V1_2::Model& model);

inline std::string toString(uint32_t obj) {
    return std::to_string(obj);
}

template <typename Type>
std::string toString(const std::vector<Type>& range) {
    std::string os = "[";
    for (size_t i = 0; i < range.size(); ++i) {
        os += (i == 0 ? "" : ", ") + toString(range[i]);
    }
    return os += "]";
}

inline std::string toString(HalVersion halVersion) {
    switch (halVersion) {
        case HalVersion::UNKNOWN:
            return "UNKNOWN HAL version";
        case HalVersion::V1_0:
            return "HAL version 1.0";
        case HalVersion::V1_1:
            return "HAL version 1.1";
        case HalVersion::V1_2:
            return "HAL version 1.2";
    }
}

inline bool validCode(uint32_t codeCount, uint32_t codeCountOEM, uint32_t code) {
    return (code < codeCount) || (code >= kOEMCodeBase && (code - kOEMCodeBase) < codeCountOEM);
}

bool validateOperandSymmPerChannelQuantParams(
        const Operand& halOperand, const ANeuralNetworksSymmPerChannelQuantParams& channelQuant,
        const char* tag);

// Validates the type. If allowPartial is true, the dimensions may be underspecified.
int validateOperandType(const ANeuralNetworksOperandType& type, const char* tag, bool allowPartial);
int validateOperandList(uint32_t count, const uint32_t* list, uint32_t operandCount,
                        const char* tag);

// Returns ANEURALNETWORKS_NO_ERROR if the corresponding operation is defined and can handle the
// provided operand types in the given HAL version, otherwise returns ANEURALNETWORKS_BAD_DATA.
int validateOperation(ANeuralNetworksOperationType opType, uint32_t inputCount,
                      const uint32_t* inputIndexes, uint32_t outputCount,
                      const uint32_t* outputIndexes, const std::vector<Operand>& operands,
                      HalVersion halVersion);

inline size_t getSizeFromInts(int lower, int higher) {
    return (uint32_t)(lower) + ((uint64_t)(uint32_t)(higher) << 32);
}

// Convert ANEURALNETWORKS_* result code to ErrorStatus.
// Not guaranteed to be a 1-to-1 mapping.
ErrorStatus convertResultCodeToErrorStatus(int resultCode);

// Convert ErrorStatus to ANEURALNETWORKS_* result code.
// Not guaranteed to be a 1-to-1 mapping.
int convertErrorStatusToResultCode(ErrorStatus status);

// Versioning

bool compliantWithV1_0(const V1_0::Capabilities& capabilities);
bool compliantWithV1_0(const V1_1::Capabilities& capabilities);
bool compliantWithV1_1(const V1_0::Capabilities& capabilities);
bool compliantWithV1_1(const V1_1::Capabilities& capabilities);

bool compliantWithV1_0(const V1_0::Model& model);
bool compliantWithV1_0(const V1_1::Model& model);
bool compliantWithV1_0(const V1_2::Model& model);
bool compliantWithV1_1(const V1_0::Model& model);
bool compliantWithV1_1(const V1_1::Model& model);
bool compliantWithV1_1(const V1_2::Model& model);

V1_0::Capabilities convertToV1_0(const V1_0::Capabilities& capabilities);
V1_0::Capabilities convertToV1_0(const V1_1::Capabilities& capabilities);
V1_1::Capabilities convertToV1_1(const V1_0::Capabilities& capabilities);
V1_1::Capabilities convertToV1_1(const V1_1::Capabilities& capabilities);

V1_0::Model convertToV1_0(const V1_0::Model& model);
V1_0::Model convertToV1_0(const V1_1::Model& model);
V1_0::Model convertToV1_0(const V1_2::Model& model);
V1_1::Model convertToV1_1(const V1_0::Model& model);
V1_1::Model convertToV1_1(const V1_1::Model& model);
V1_1::Model convertToV1_1(const V1_2::Model& model);
V1_2::Model convertToV1_2(const V1_0::Model& model);
V1_2::Model convertToV1_2(const V1_1::Model& model);
V1_2::Model convertToV1_2(const V1_2::Model& model);

V1_2::Operand convertToV1_2(const V1_0::Operand& operand);
V1_2::Operand convertToV1_2(const V1_2::Operand& operand);

hidl_vec<V1_2::Operand> convertToV1_2(const hidl_vec<V1_0::Operand>& operands);
hidl_vec<V1_2::Operand> convertToV1_2(const hidl_vec<V1_2::Operand>& operands);

#ifdef NN_DEBUGGABLE
uint32_t getProp(const char* str, uint32_t defaultValue = 0);
#endif  // NN_DEBUGGABLE

}  // namespace nn
}  // namespace android

#endif  // ANDROID_ML_NN_COMMON_UTILS_H
