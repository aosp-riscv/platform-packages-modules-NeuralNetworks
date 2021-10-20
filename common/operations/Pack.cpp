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

#define LOG_TAG "Operations"

#include "OperationResolver.h"
#include "OperationsUtils.h"

#ifdef NN_INCLUDE_CPU_IMPLEMENTATION
#include <limits>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wsign-compare"
#include <tensorflow/lite/kernels/internal/reference/reference_ops.h>
#pragma clang diagnostic pop

#include "CpuOperationUtils.h"
#endif  // NN_INCLUDE_CPU_IMPLEMENTATION

namespace android {
namespace nn {
namespace pack_op {

constexpr char kOperationName[] = "PACK";

// inputs consist of a scalar specifying the axis, plus 1 or more tensors
constexpr uint32_t kMinNumInputs = 2;
constexpr uint32_t kInputAxisScalar = 0;
constexpr uint32_t kInputFirstTensor = 1;

constexpr uint32_t kNumOutputs = 1;
constexpr uint32_t kOutputTensor = 0;

Result<Version> validate(const IOperationValidationContext* context) {
    NN_RET_CHECK_GE(context->getNumInputs(), kMinNumInputs);
    NN_RET_CHECK_EQ(context->getNumOutputs(), kNumOutputs);

    // Validate the axis scalar.
    const OperandType axisScalarType = context->getInputType(kInputAxisScalar);
    NN_RET_CHECK_EQ(axisScalarType, OperandType::INT32)
            << "Unsupported axis scalar type for pack op";

    // Validate the output tensor.
    const OperandType outputType = context->getOutputType(kOutputTensor);
    NN_RET_CHECK(outputType == OperandType::TENSOR_FLOAT16 ||
                 outputType == OperandType::TENSOR_FLOAT32 ||
                 outputType == OperandType::TENSOR_QUANT8_ASYMM ||
                 outputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED ||
                 outputType == OperandType::TENSOR_INT32);

    // All input tensors must agree with the output tensor in type, scale, and zeroPoint (offset).
    // All input tensors must agree in rank, which must be one less than rank of the output tensor.
    const Shape outputShape = context->getOutputShape(kOutputTensor);
    //     Either a rank we must match, or zero if we haven't determined the rank.
    size_t requiredInputRank = [outputRank = getNumberOfDimensions(outputShape)] {
        return (outputRank ? outputRank - 1 : 0);
    }();
    for (uint32_t inputTensorNum = 0, inputTensorCount = context->getNumInputs() - 1;
         inputTensorNum < inputTensorCount; ++inputTensorNum) {
        const Shape inputShape = context->getInputShape(kInputFirstTensor + inputTensorNum);
        NN_RET_CHECK_EQ(inputShape.type, outputShape.type)
                << "Input tensor #" << inputTensorNum << " type " << inputShape.type
                << " does not match output tensor type " << outputShape.type;
        if (outputType == OperandType::TENSOR_QUANT8_ASYMM ||
            outputType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
            NN_RET_CHECK_EQ(inputShape.scale, outputShape.scale)
                    << "Input tensor #" << inputTensorNum << " scale " << inputShape.scale
                    << " does not match output tensor scale " << outputShape.scale;
            NN_RET_CHECK_EQ(inputShape.offset, outputShape.offset)
                    << "Input tensor #" << inputTensorNum << " offset " << inputShape.offset
                    << " does not match output tensor offset " << outputShape.offset;
        }
        if (const size_t inputRank = inputShape.dimensions.size()) {
            if (requiredInputRank) {
                NN_RET_CHECK_EQ(requiredInputRank, inputRank)
                        << "Input tensor #" << inputTensorNum << " has inconsistent rank";
            } else {
                requiredInputRank = inputRank;
            }
        }
    }

    return Version::FEATURE_LEVEL_6;
}

#ifdef NN_INCLUDE_CPU_IMPLEMENTATION
bool prepare(IOperationExecutionContext* context) {
    // All input tensors must have the same dimensions and be of rank 1 or higher.
    const Shape firstInputTensorShape = context->getInputShape(kInputFirstTensor);
    const uint32_t firstInputTensorRank = getNumberOfDimensions(firstInputTensorShape);
    NN_RET_CHECK_GE(firstInputTensorRank, 1U);
    for (uint32_t inputTensorNum = 1, inputTensorCount = context->getNumInputs() - 1;
         inputTensorNum < inputTensorCount; ++inputTensorNum) {
        NN_RET_CHECK(SameShape(firstInputTensorShape,
                               context->getInputShape(kInputFirstTensor + inputTensorNum)))
                << "Input tensor #" << inputTensorNum
                << " dimensions do not match input tensor #0 dimensions";
    }

    // Fetch the axis dimension value.
    const int32_t axisDimension = context->getInputValue<int32_t>(kInputAxisScalar);
    NN_RET_CHECK_GE(axisDimension, 0);
    NN_RET_CHECK_LT(uint32_t(axisDimension), firstInputTensorRank + 1);

    // TODO: http://b/78268320 validate that output shape is consistent with input rather than
    // blindly overwriting it. Output tensor is of rank 1 higher than input tensors.
    const uint32_t outputTensorRank = firstInputTensorRank + 1;
    // For the (j)th output dimension:
    // - If (j) is less than the axis dimension, the (j)th output dimension must match the (j)th
    // input dimension.
    // - If (j) is the axis dimension, the (j)th output dimension must equal the number of input
    // tensors.
    // - If (j) is greater than the axis dimension, the (j)th output dimension must match the
    // (j-1)th input dimension.
    Shape outputShape = context->getOutputShape(kOutputTensor);
    outputShape.dimensions.resize(outputTensorRank);
    for (int32_t j = 0; j < axisDimension; ++j) {
        outputShape.dimensions[j] = firstInputTensorShape.dimensions[j];
    }
    outputShape.dimensions[axisDimension] = context->getNumInputs() - 1;
    for (int32_t j = axisDimension + 1; j < int32_t(outputTensorRank); ++j) {
        outputShape.dimensions[j] = firstInputTensorShape.dimensions[j - 1];
    }
    return context->setOutputShape(kOutputTensor, outputShape);
}

bool packParams(IOperationExecutionContext* context, tflite::PackParams* params) {
    const int32_t axis = context->getInputValue<int32_t>(kInputAxisScalar);
    NN_RET_CHECK_LE(axis, std::numeric_limits<typeof(params->axis)>().max())
            << "axis value out of range";
    params->axis = axis;

    const uint32_t inputTensorCount = context->getNumInputs() - 1;
    NN_RET_CHECK_LE(inputTensorCount, std::numeric_limits<typeof(params->inputs_count)>().max())
            << "input count out of range";
    params->inputs_count = inputTensorCount;

    // Note that the NNAPI PACK operation specification requires all input
    // tensors and the output tensor to have the same zeroPoint and scale.
    const Shape tensorShape = context->getInputShape(kInputFirstTensor);

    const std::vector<int32_t> paramsInputZeroPoint(inputTensorCount, tensorShape.offset);
    params->input_zeropoint = paramsInputZeroPoint.data();
    const std::vector<float> paramsInputScale(inputTensorCount, tensorShape.scale);
    params->input_scale = paramsInputScale.data();
    params->output_zeropoint = tensorShape.offset;
    params->output_scale = tensorShape.scale;

    return true;
}

template <typename T>
bool pack(IOperationExecutionContext* context) {
    tflite::PackParams params;
    NN_RET_CHECK(packParams(context, &params));

    const uint32_t inputTensorCount = context->getNumInputs() - 1;

    // Note that the NNAPI PACK operation specification requires all input
    // tensors to have the same dimensions.
    const tflite::RuntimeShape inputTensorShapes =
            convertShapeToTflshape(context->getInputShape(kInputFirstTensor));
    const std::vector<const tflite::RuntimeShape*> inputShapesPtrs(inputTensorCount,
                                                                   &inputTensorShapes);

    std::vector<const T*> inputData(inputTensorCount);
    for (uint32_t inputTensorNum = 0; inputTensorNum < inputTensorCount; ++inputTensorNum) {
        inputData[inputTensorNum] = context->getInputBuffer<T>(kInputFirstTensor + inputTensorNum);
    }

    tflite::reference_ops::Pack(params, inputShapesPtrs.data(), inputData.data(),
                                convertShapeToTflshape(context->getOutputShape(kOutputTensor)),
                                context->getOutputBuffer<T>(kOutputTensor));
    return true;
}

bool execute(IOperationExecutionContext* context) {
    switch (context->getInputType(kInputFirstTensor)) {
        case OperandType::TENSOR_FLOAT16:
            return pack<_Float16>(context);
        case OperandType::TENSOR_FLOAT32:
            return pack<float>(context);
        case OperandType::TENSOR_QUANT8_ASYMM:
            return pack<uint8_t>(context);
        case OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            return pack<int8_t>(context);
        case OperandType::TENSOR_INT32:
            return pack<int32_t>(context);
        default:
            NN_RET_CHECK_FAIL() << "Unsupported tensor type for operation " << kOperationName;
    }
}
#endif  // NN_INCLUDE_CPU_IMPLEMENTATION

}  // namespace pack_op

NN_REGISTER_OPERATION(PACK, pack_op::kOperationName, pack_op::validate, pack_op::prepare,
                      pack_op::execute);

}  // namespace nn
}  // namespace android
