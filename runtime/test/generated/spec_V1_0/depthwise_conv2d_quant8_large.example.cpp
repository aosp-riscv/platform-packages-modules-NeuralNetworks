// Generated from depthwise_conv2d_quant8_large.mod.py
// DO NOT EDIT
// clang-format off
#include "TestHarness.h"
using namespace test_helper;

namespace generated_tests::depthwise_conv2d_quant8_large {

const TestModel& get_test_model() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .inputIndexes = {0},
        .isRelaxed = false,
        .minSupportedVersion = TestHalVersion::V1_0,
        .operands = {{
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({4, 16, 4, 32, 4, 64, 4, 128}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_INPUT,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({2, 4, 2, 0, 2, 2, 2, 0}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0, 0}),
                .dimensions = {2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.25f,
                .type = TestOperandType::TENSOR_INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 4,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({1}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 2,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({1}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({8, 48}),
                .dimensions = {1, 1, 1, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_OUTPUT,
                .numberOfConsumers = 0,
                .scale = 1.0f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }},
        .operations = {{
                .inputs = {0, 1, 2, 3, 3, 3, 3, 4, 4, 5, 6},
                .outputs = {7},
                .type = TestOperationType::DEPTHWISE_CONV_2D
            }},
        .outputIndexes = {7}
    };
    return model;
}

const auto dummy_test_model = TestModelManager::get().add("depthwise_conv2d_quant8_large", get_test_model());

}  // namespace generated_tests::depthwise_conv2d_quant8_large

namespace generated_tests::depthwise_conv2d_quant8_large {

const TestModel& get_test_model_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .inputIndexes = {8},
        .isRelaxed = false,
        .minSupportedVersion = TestHalVersion::V1_0,
        .operands = {{
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({2, 4, 2, 0, 2, 2, 2, 0}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0, 0}),
                .dimensions = {2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.25f,
                .type = TestOperandType::TENSOR_INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 4,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({1}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 2,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({1}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({8, 48}),
                .dimensions = {1, 1, 1, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_OUTPUT,
                .numberOfConsumers = 0,
                .scale = 1.0f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({4, 16, 4, 32, 4, 64, 4, 128}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_INPUT,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({0}),
                .dimensions = {1},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }},
        .operations = {{
                .inputs = {8, 9, 10},
                .outputs = {0},
                .type = TestOperationType::ADD
            }, {
                .inputs = {0, 1, 2, 3, 3, 3, 3, 4, 4, 5, 6},
                .outputs = {7},
                .type = TestOperationType::DEPTHWISE_CONV_2D
            }},
        .outputIndexes = {7}
    };
    return model;
}

const auto dummy_test_model_all_inputs_as_internal = TestModelManager::get().add("depthwise_conv2d_quant8_large_all_inputs_as_internal", get_test_model_all_inputs_as_internal());

}  // namespace generated_tests::depthwise_conv2d_quant8_large

namespace generated_tests::depthwise_conv2d_quant8_large {

const TestModel& get_test_model_all_tensors_as_inputs() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .inputIndexes = {0, 1, 2},
        .isRelaxed = false,
        .minSupportedVersion = TestHalVersion::V1_0,
        .operands = {{
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({4, 16, 4, 32, 4, 64, 4, 128}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_INPUT,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({2, 4, 2, 0, 2, 2, 2, 0}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_INPUT,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0, 0}),
                .dimensions = {2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_INPUT,
                .numberOfConsumers = 1,
                .scale = 0.25f,
                .type = TestOperandType::TENSOR_INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 4,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({1}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 2,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({1}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({8, 48}),
                .dimensions = {1, 1, 1, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_OUTPUT,
                .numberOfConsumers = 0,
                .scale = 1.0f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }},
        .operations = {{
                .inputs = {0, 1, 2, 3, 3, 3, 3, 4, 4, 5, 6},
                .outputs = {7},
                .type = TestOperationType::DEPTHWISE_CONV_2D
            }},
        .outputIndexes = {7}
    };
    return model;
}

const auto dummy_test_model_all_tensors_as_inputs = TestModelManager::get().add("depthwise_conv2d_quant8_large_all_tensors_as_inputs", get_test_model_all_tensors_as_inputs());

}  // namespace generated_tests::depthwise_conv2d_quant8_large

namespace generated_tests::depthwise_conv2d_quant8_large {

const TestModel& get_test_model_all_tensors_as_inputs_all_inputs_as_internal() {
    static TestModel model = {
        .expectFailure = false,
        .expectedMultinomialDistributionTolerance = 0,
        .inputIndexes = {2, 8, 11},
        .isRelaxed = false,
        .minSupportedVersion = TestHalVersion::V1_0,
        .operands = {{
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0, 0}),
                .dimensions = {2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_INPUT,
                .numberOfConsumers = 1,
                .scale = 0.25f,
                .type = TestOperandType::TENSOR_INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 4,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({1}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 2,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({1}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({8, 48}),
                .dimensions = {1, 1, 1, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_OUTPUT,
                .numberOfConsumers = 0,
                .scale = 1.0f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({4, 16, 4, 32, 4, 64, 4, 128}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_INPUT,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({0}),
                .dimensions = {1},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({2, 4, 2, 0, 2, 2, 2, 0}),
                .dimensions = {1, 2, 2, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_INPUT,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<uint8_t>({0}),
                .dimensions = {1},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.5f,
                .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }},
        .operations = {{
                .inputs = {8, 9, 10},
                .outputs = {0},
                .type = TestOperationType::ADD
            }, {
                .inputs = {11, 12, 13},
                .outputs = {1},
                .type = TestOperationType::ADD
            }, {
                .inputs = {0, 1, 2, 3, 3, 3, 3, 4, 4, 5, 6},
                .outputs = {7},
                .type = TestOperationType::DEPTHWISE_CONV_2D
            }},
        .outputIndexes = {7}
    };
    return model;
}

const auto dummy_test_model_all_tensors_as_inputs_all_inputs_as_internal = TestModelManager::get().add("depthwise_conv2d_quant8_large_all_tensors_as_inputs_all_inputs_as_internal", get_test_model_all_tensors_as_inputs_all_inputs_as_internal());

}  // namespace generated_tests::depthwise_conv2d_quant8_large

