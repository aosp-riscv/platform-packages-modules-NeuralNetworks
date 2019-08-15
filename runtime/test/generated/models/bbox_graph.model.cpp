// Generated from bbox_graph.mod.py
// DO NOT EDIT
// clang-format off
#include "TestGenerated.h"

namespace generated_tests::bbox_graph {

void CreateModel_zero_sized(Model *model) {
  OperandType type0(Type::BOOL, {});
  OperandType type1(Type::TENSOR_FLOAT32, {1, 1, 1, 1});
  OperandType type10(Type::TENSOR_FLOAT32, {0, 2, 2, 1});
  OperandType type11(Type::TENSOR_FLOAT32, {8, 4});
  OperandType type12(Type::TENSOR_FLOAT32, {8});
  OperandType type13(Type::TENSOR_FLOAT32, {0, 8});
  OperandType type14(Type::TENSOR_FLOAT32, {2, 4});
  OperandType type15(Type::TENSOR_FLOAT32, {2});
  OperandType type16(Type::TENSOR_FLOAT32, {0, 2});
  OperandType type2(Type::TENSOR_FLOAT32, {1, 1, 1, 4});
  OperandType type3(Type::TENSOR_FLOAT32, {1, 4});
  OperandType type4(Type::TENSOR_FLOAT32, {1, 2});
  OperandType type5(Type::TENSOR_FLOAT32, {0});
  OperandType type6(Type::TENSOR_FLOAT32, {0, 4});
  OperandType type7(Type::TENSOR_INT32, {0});
  OperandType type8(Type::FLOAT32, {});
  OperandType type9(Type::INT32, {});
  // Phase 1, operands
  auto scores = model->addOperand(&type1);
  auto deltas = model->addOperand(&type2);
  auto anchors = model->addOperand(&type3);
  auto imageInfo = model->addOperand(&type4);
  auto param = model->addOperand(&type8);
  auto param1 = model->addOperand(&type8);
  auto param2 = model->addOperand(&type9);
  auto param3 = model->addOperand(&type9);
  auto param4 = model->addOperand(&type8);
  auto param5 = model->addOperand(&type8);
  auto layout = model->addOperand(&type0);
  auto scores1 = model->addOperand(&type5);
  auto roi = model->addOperand(&type6);
  auto batches = model->addOperand(&type7);
  auto featureMap = model->addOperand(&type1);
  auto param6 = model->addOperand(&type9);
  auto param7 = model->addOperand(&type9);
  auto param8 = model->addOperand(&type8);
  auto param9 = model->addOperand(&type8);
  auto param10 = model->addOperand(&type9);
  auto param11 = model->addOperand(&type9);
  auto scores2 = model->addOperand(&type10);
  auto weights = model->addOperand(&type11);
  auto bias = model->addOperand(&type12);
  auto param12 = model->addOperand(&type9);
  auto delta = model->addOperand(&type13);
  auto weights1 = model->addOperand(&type14);
  auto bias1 = model->addOperand(&type15);
  auto param13 = model->addOperand(&type9);
  auto scores3 = model->addOperand(&type16);
  auto roi1 = model->addOperand(&type13);
  auto param14 = model->addOperand(&type8);
  auto param15 = model->addOperand(&type9);
  auto param16 = model->addOperand(&type9);
  auto param17 = model->addOperand(&type8);
  auto param18 = model->addOperand(&type8);
  auto param19 = model->addOperand(&type8);
  auto scores4 = model->addOperand(&type5);
  auto roi2 = model->addOperand(&type6);
  auto classes = model->addOperand(&type7);
  auto batches1 = model->addOperand(&type7);
  // Phase 2, operations
  static float param_init[] = {1.0f};
  model->setOperandValue(param, param_init, sizeof(float) * 1);
  static float param1_init[] = {1.0f};
  model->setOperandValue(param1, param1_init, sizeof(float) * 1);
  static int32_t param2_init[] = {-1};
  model->setOperandValue(param2, param2_init, sizeof(int32_t) * 1);
  static int32_t param3_init[] = {-1};
  model->setOperandValue(param3, param3_init, sizeof(int32_t) * 1);
  static float param4_init[] = {0.3f};
  model->setOperandValue(param4, param4_init, sizeof(float) * 1);
  static float param5_init[] = {10.0f};
  model->setOperandValue(param5, param5_init, sizeof(float) * 1);
  static bool8 layout_init[] = {false};
  model->setOperandValue(layout, layout_init, sizeof(bool8) * 1);
  static int32_t param6_init[] = {2};
  model->setOperandValue(param6, param6_init, sizeof(int32_t) * 1);
  static int32_t param7_init[] = {2};
  model->setOperandValue(param7, param7_init, sizeof(int32_t) * 1);
  static float param8_init[] = {1.0f};
  model->setOperandValue(param8, param8_init, sizeof(float) * 1);
  static float param9_init[] = {1.0f};
  model->setOperandValue(param9, param9_init, sizeof(float) * 1);
  static int32_t param10_init[] = {4};
  model->setOperandValue(param10, param10_init, sizeof(int32_t) * 1);
  static int32_t param11_init[] = {4};
  model->setOperandValue(param11, param11_init, sizeof(int32_t) * 1);
  static float weights_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights, weights_init, sizeof(float) * 32);
  static float bias_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(bias, bias_init, sizeof(float) * 8);
  static int32_t param12_init[] = {0};
  model->setOperandValue(param12, param12_init, sizeof(int32_t) * 1);
  static float weights1_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights1, weights1_init, sizeof(float) * 8);
  static float bias1_init[] = {1.0f, 1.0f};
  model->setOperandValue(bias1, bias1_init, sizeof(float) * 2);
  static int32_t param13_init[] = {0};
  model->setOperandValue(param13, param13_init, sizeof(int32_t) * 1);
  static float param14_init[] = {0.1f};
  model->setOperandValue(param14, param14_init, sizeof(float) * 1);
  static int32_t param15_init[] = {-1};
  model->setOperandValue(param15, param15_init, sizeof(int32_t) * 1);
  static int32_t param16_init[] = {0};
  model->setOperandValue(param16, param16_init, sizeof(int32_t) * 1);
  static float param17_init[] = {0.3f};
  model->setOperandValue(param17, param17_init, sizeof(float) * 1);
  static float param18_init[] = {1.0f};
  model->setOperandValue(param18, param18_init, sizeof(float) * 1);
  static float param19_init[] = {0.1f};
  model->setOperandValue(param19, param19_init, sizeof(float) * 1);
  model->addOperation(ANEURALNETWORKS_GENERATE_PROPOSALS, {scores, deltas, anchors, imageInfo, param, param1, param2, param3, param4, param5, layout}, {scores1, roi, batches});
  model->addOperation(ANEURALNETWORKS_ROI_ALIGN, {featureMap, roi, batches, param6, param7, param8, param9, param10, param11, layout}, {scores2});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights, bias, param12}, {delta});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights1, bias1, param13}, {scores3});
  model->addOperation(ANEURALNETWORKS_AXIS_ALIGNED_BBOX_TRANSFORM, {roi, delta, batches, imageInfo}, {roi1});
  model->addOperation(ANEURALNETWORKS_BOX_WITH_NMS_LIMIT, {scores3, roi1, batches, param14, param15, param16, param17, param18, param19}, {scores4, roi2, classes, batches1});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {scores, deltas, anchors, imageInfo, featureMap},
    {scores1, scores4, roi2, classes, batches1});
  assert(model->isValid());
}

bool is_ignored_zero_sized(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}

} // namespace generated_tests::bbox_graph
namespace generated_tests::bbox_graph {

void CreateModel_zero_sized_relaxed(Model *model) {
  OperandType type0(Type::BOOL, {});
  OperandType type1(Type::TENSOR_FLOAT32, {1, 1, 1, 1});
  OperandType type10(Type::TENSOR_FLOAT32, {0, 2, 2, 1});
  OperandType type11(Type::TENSOR_FLOAT32, {8, 4});
  OperandType type12(Type::TENSOR_FLOAT32, {8});
  OperandType type13(Type::TENSOR_FLOAT32, {0, 8});
  OperandType type14(Type::TENSOR_FLOAT32, {2, 4});
  OperandType type15(Type::TENSOR_FLOAT32, {2});
  OperandType type16(Type::TENSOR_FLOAT32, {0, 2});
  OperandType type2(Type::TENSOR_FLOAT32, {1, 1, 1, 4});
  OperandType type3(Type::TENSOR_FLOAT32, {1, 4});
  OperandType type4(Type::TENSOR_FLOAT32, {1, 2});
  OperandType type5(Type::TENSOR_FLOAT32, {0});
  OperandType type6(Type::TENSOR_FLOAT32, {0, 4});
  OperandType type7(Type::TENSOR_INT32, {0});
  OperandType type8(Type::FLOAT32, {});
  OperandType type9(Type::INT32, {});
  // Phase 1, operands
  auto scores = model->addOperand(&type1);
  auto deltas = model->addOperand(&type2);
  auto anchors = model->addOperand(&type3);
  auto imageInfo = model->addOperand(&type4);
  auto param = model->addOperand(&type8);
  auto param1 = model->addOperand(&type8);
  auto param2 = model->addOperand(&type9);
  auto param3 = model->addOperand(&type9);
  auto param4 = model->addOperand(&type8);
  auto param5 = model->addOperand(&type8);
  auto layout = model->addOperand(&type0);
  auto scores1 = model->addOperand(&type5);
  auto roi = model->addOperand(&type6);
  auto batches = model->addOperand(&type7);
  auto featureMap = model->addOperand(&type1);
  auto param6 = model->addOperand(&type9);
  auto param7 = model->addOperand(&type9);
  auto param8 = model->addOperand(&type8);
  auto param9 = model->addOperand(&type8);
  auto param10 = model->addOperand(&type9);
  auto param11 = model->addOperand(&type9);
  auto scores2 = model->addOperand(&type10);
  auto weights = model->addOperand(&type11);
  auto bias = model->addOperand(&type12);
  auto param12 = model->addOperand(&type9);
  auto delta = model->addOperand(&type13);
  auto weights1 = model->addOperand(&type14);
  auto bias1 = model->addOperand(&type15);
  auto param13 = model->addOperand(&type9);
  auto scores3 = model->addOperand(&type16);
  auto roi1 = model->addOperand(&type13);
  auto param14 = model->addOperand(&type8);
  auto param15 = model->addOperand(&type9);
  auto param16 = model->addOperand(&type9);
  auto param17 = model->addOperand(&type8);
  auto param18 = model->addOperand(&type8);
  auto param19 = model->addOperand(&type8);
  auto scores4 = model->addOperand(&type5);
  auto roi2 = model->addOperand(&type6);
  auto classes = model->addOperand(&type7);
  auto batches1 = model->addOperand(&type7);
  // Phase 2, operations
  static float param_init[] = {1.0f};
  model->setOperandValue(param, param_init, sizeof(float) * 1);
  static float param1_init[] = {1.0f};
  model->setOperandValue(param1, param1_init, sizeof(float) * 1);
  static int32_t param2_init[] = {-1};
  model->setOperandValue(param2, param2_init, sizeof(int32_t) * 1);
  static int32_t param3_init[] = {-1};
  model->setOperandValue(param3, param3_init, sizeof(int32_t) * 1);
  static float param4_init[] = {0.3f};
  model->setOperandValue(param4, param4_init, sizeof(float) * 1);
  static float param5_init[] = {10.0f};
  model->setOperandValue(param5, param5_init, sizeof(float) * 1);
  static bool8 layout_init[] = {false};
  model->setOperandValue(layout, layout_init, sizeof(bool8) * 1);
  static int32_t param6_init[] = {2};
  model->setOperandValue(param6, param6_init, sizeof(int32_t) * 1);
  static int32_t param7_init[] = {2};
  model->setOperandValue(param7, param7_init, sizeof(int32_t) * 1);
  static float param8_init[] = {1.0f};
  model->setOperandValue(param8, param8_init, sizeof(float) * 1);
  static float param9_init[] = {1.0f};
  model->setOperandValue(param9, param9_init, sizeof(float) * 1);
  static int32_t param10_init[] = {4};
  model->setOperandValue(param10, param10_init, sizeof(int32_t) * 1);
  static int32_t param11_init[] = {4};
  model->setOperandValue(param11, param11_init, sizeof(int32_t) * 1);
  static float weights_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights, weights_init, sizeof(float) * 32);
  static float bias_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(bias, bias_init, sizeof(float) * 8);
  static int32_t param12_init[] = {0};
  model->setOperandValue(param12, param12_init, sizeof(int32_t) * 1);
  static float weights1_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights1, weights1_init, sizeof(float) * 8);
  static float bias1_init[] = {1.0f, 1.0f};
  model->setOperandValue(bias1, bias1_init, sizeof(float) * 2);
  static int32_t param13_init[] = {0};
  model->setOperandValue(param13, param13_init, sizeof(int32_t) * 1);
  static float param14_init[] = {0.1f};
  model->setOperandValue(param14, param14_init, sizeof(float) * 1);
  static int32_t param15_init[] = {-1};
  model->setOperandValue(param15, param15_init, sizeof(int32_t) * 1);
  static int32_t param16_init[] = {0};
  model->setOperandValue(param16, param16_init, sizeof(int32_t) * 1);
  static float param17_init[] = {0.3f};
  model->setOperandValue(param17, param17_init, sizeof(float) * 1);
  static float param18_init[] = {1.0f};
  model->setOperandValue(param18, param18_init, sizeof(float) * 1);
  static float param19_init[] = {0.1f};
  model->setOperandValue(param19, param19_init, sizeof(float) * 1);
  model->addOperation(ANEURALNETWORKS_GENERATE_PROPOSALS, {scores, deltas, anchors, imageInfo, param, param1, param2, param3, param4, param5, layout}, {scores1, roi, batches});
  model->addOperation(ANEURALNETWORKS_ROI_ALIGN, {featureMap, roi, batches, param6, param7, param8, param9, param10, param11, layout}, {scores2});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights, bias, param12}, {delta});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights1, bias1, param13}, {scores3});
  model->addOperation(ANEURALNETWORKS_AXIS_ALIGNED_BBOX_TRANSFORM, {roi, delta, batches, imageInfo}, {roi1});
  model->addOperation(ANEURALNETWORKS_BOX_WITH_NMS_LIMIT, {scores3, roi1, batches, param14, param15, param16, param17, param18, param19}, {scores4, roi2, classes, batches1});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {scores, deltas, anchors, imageInfo, featureMap},
    {scores1, scores4, roi2, classes, batches1});
  // Phase 4: set relaxed execution
  model->relaxComputationFloat32toFloat16(true);
  assert(model->isValid());
}

bool is_ignored_zero_sized_relaxed(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}

} // namespace generated_tests::bbox_graph
namespace generated_tests::bbox_graph {

void CreateModel_zero_sized_float16(Model *model) {
  OperandType type0(Type::BOOL, {});
  OperandType type17(Type::TENSOR_FLOAT16, {1, 4});
  OperandType type18(Type::TENSOR_FLOAT16, {8});
  OperandType type19(Type::TENSOR_FLOAT16, {2});
  OperandType type20(Type::TENSOR_FLOAT16, {0, 8});
  OperandType type21(Type::TENSOR_FLOAT16, {1, 1, 1, 4});
  OperandType type22(Type::TENSOR_FLOAT16, {1, 1, 1, 1});
  OperandType type23(Type::TENSOR_FLOAT16, {1, 2});
  OperandType type24(Type::FLOAT16, {});
  OperandType type25(Type::TENSOR_FLOAT16, {0, 4});
  OperandType type26(Type::TENSOR_FLOAT16, {0});
  OperandType type27(Type::TENSOR_FLOAT16, {0, 2, 2, 1});
  OperandType type28(Type::TENSOR_FLOAT16, {0, 2});
  OperandType type29(Type::TENSOR_FLOAT16, {8, 4});
  OperandType type30(Type::TENSOR_FLOAT16, {2, 4});
  OperandType type7(Type::TENSOR_INT32, {0});
  OperandType type9(Type::INT32, {});
  // Phase 1, operands
  auto scores = model->addOperand(&type22);
  auto deltas = model->addOperand(&type21);
  auto anchors = model->addOperand(&type17);
  auto imageInfo = model->addOperand(&type23);
  auto param = model->addOperand(&type24);
  auto param1 = model->addOperand(&type24);
  auto param2 = model->addOperand(&type9);
  auto param3 = model->addOperand(&type9);
  auto param4 = model->addOperand(&type24);
  auto param5 = model->addOperand(&type24);
  auto layout = model->addOperand(&type0);
  auto scores1 = model->addOperand(&type26);
  auto roi = model->addOperand(&type25);
  auto batches = model->addOperand(&type7);
  auto featureMap = model->addOperand(&type22);
  auto param6 = model->addOperand(&type9);
  auto param7 = model->addOperand(&type9);
  auto param8 = model->addOperand(&type24);
  auto param9 = model->addOperand(&type24);
  auto param10 = model->addOperand(&type9);
  auto param11 = model->addOperand(&type9);
  auto scores2 = model->addOperand(&type27);
  auto weights = model->addOperand(&type29);
  auto bias = model->addOperand(&type18);
  auto param12 = model->addOperand(&type9);
  auto delta = model->addOperand(&type20);
  auto weights1 = model->addOperand(&type30);
  auto bias1 = model->addOperand(&type19);
  auto param13 = model->addOperand(&type9);
  auto scores3 = model->addOperand(&type28);
  auto roi1 = model->addOperand(&type20);
  auto param14 = model->addOperand(&type24);
  auto param15 = model->addOperand(&type9);
  auto param16 = model->addOperand(&type9);
  auto param17 = model->addOperand(&type24);
  auto param18 = model->addOperand(&type24);
  auto param19 = model->addOperand(&type24);
  auto scores4 = model->addOperand(&type26);
  auto roi2 = model->addOperand(&type25);
  auto classes = model->addOperand(&type7);
  auto batches1 = model->addOperand(&type7);
  // Phase 2, operations
  static _Float16 param_init[] = {1.0f};
  model->setOperandValue(param, param_init, sizeof(_Float16) * 1);
  static _Float16 param1_init[] = {1.0f};
  model->setOperandValue(param1, param1_init, sizeof(_Float16) * 1);
  static int32_t param2_init[] = {-1};
  model->setOperandValue(param2, param2_init, sizeof(int32_t) * 1);
  static int32_t param3_init[] = {-1};
  model->setOperandValue(param3, param3_init, sizeof(int32_t) * 1);
  static _Float16 param4_init[] = {0.30000001192092896f};
  model->setOperandValue(param4, param4_init, sizeof(_Float16) * 1);
  static _Float16 param5_init[] = {10.0f};
  model->setOperandValue(param5, param5_init, sizeof(_Float16) * 1);
  static bool8 layout_init[] = {false};
  model->setOperandValue(layout, layout_init, sizeof(bool8) * 1);
  static int32_t param6_init[] = {2};
  model->setOperandValue(param6, param6_init, sizeof(int32_t) * 1);
  static int32_t param7_init[] = {2};
  model->setOperandValue(param7, param7_init, sizeof(int32_t) * 1);
  static _Float16 param8_init[] = {1.0f};
  model->setOperandValue(param8, param8_init, sizeof(_Float16) * 1);
  static _Float16 param9_init[] = {1.0f};
  model->setOperandValue(param9, param9_init, sizeof(_Float16) * 1);
  static int32_t param10_init[] = {4};
  model->setOperandValue(param10, param10_init, sizeof(int32_t) * 1);
  static int32_t param11_init[] = {4};
  model->setOperandValue(param11, param11_init, sizeof(int32_t) * 1);
  static _Float16 weights_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights, weights_init, sizeof(_Float16) * 32);
  static _Float16 bias_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(bias, bias_init, sizeof(_Float16) * 8);
  static int32_t param12_init[] = {0};
  model->setOperandValue(param12, param12_init, sizeof(int32_t) * 1);
  static _Float16 weights1_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights1, weights1_init, sizeof(_Float16) * 8);
  static _Float16 bias1_init[] = {1.0f, 1.0f};
  model->setOperandValue(bias1, bias1_init, sizeof(_Float16) * 2);
  static int32_t param13_init[] = {0};
  model->setOperandValue(param13, param13_init, sizeof(int32_t) * 1);
  static _Float16 param14_init[] = {0.10000000149011612f};
  model->setOperandValue(param14, param14_init, sizeof(_Float16) * 1);
  static int32_t param15_init[] = {-1};
  model->setOperandValue(param15, param15_init, sizeof(int32_t) * 1);
  static int32_t param16_init[] = {0};
  model->setOperandValue(param16, param16_init, sizeof(int32_t) * 1);
  static _Float16 param17_init[] = {0.30000001192092896f};
  model->setOperandValue(param17, param17_init, sizeof(_Float16) * 1);
  static _Float16 param18_init[] = {1.0f};
  model->setOperandValue(param18, param18_init, sizeof(_Float16) * 1);
  static _Float16 param19_init[] = {0.10000000149011612f};
  model->setOperandValue(param19, param19_init, sizeof(_Float16) * 1);
  model->addOperation(ANEURALNETWORKS_GENERATE_PROPOSALS, {scores, deltas, anchors, imageInfo, param, param1, param2, param3, param4, param5, layout}, {scores1, roi, batches});
  model->addOperation(ANEURALNETWORKS_ROI_ALIGN, {featureMap, roi, batches, param6, param7, param8, param9, param10, param11, layout}, {scores2});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights, bias, param12}, {delta});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights1, bias1, param13}, {scores3});
  model->addOperation(ANEURALNETWORKS_AXIS_ALIGNED_BBOX_TRANSFORM, {roi, delta, batches, imageInfo}, {roi1});
  model->addOperation(ANEURALNETWORKS_BOX_WITH_NMS_LIMIT, {scores3, roi1, batches, param14, param15, param16, param17, param18, param19}, {scores4, roi2, classes, batches1});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {scores, deltas, anchors, imageInfo, featureMap},
    {scores1, scores4, roi2, classes, batches1});
  assert(model->isValid());
}

bool is_ignored_zero_sized_float16(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}

} // namespace generated_tests::bbox_graph
namespace generated_tests::bbox_graph {

void CreateModel_zero_sized_quant8(Model *model) {
  OperandType type0(Type::BOOL, {});
  OperandType type31(Type::TENSOR_QUANT16_SYMM, {1, 4}, 0.125f, 0);
  OperandType type32(Type::TENSOR_INT32, {8}, 0.01f, 0);
  OperandType type33(Type::TENSOR_INT32, {2}, 0.01f, 0);
  OperandType type34(Type::TENSOR_QUANT8_ASYMM, {0, 8}, 0.1f, 128);
  OperandType type35(Type::TENSOR_QUANT8_ASYMM, {1, 1, 1, 4}, 0.1f, 128);
  OperandType type36(Type::TENSOR_QUANT8_ASYMM, {1, 1, 1, 1}, 0.1f, 128);
  OperandType type37(Type::TENSOR_QUANT16_ASYMM, {1, 2}, 0.125f, 0);
  OperandType type38(Type::TENSOR_QUANT16_ASYMM, {0, 4}, 0.125f, 0);
  OperandType type39(Type::TENSOR_QUANT16_ASYMM, {0, 8}, 0.125f, 0);
  OperandType type40(Type::TENSOR_QUANT8_ASYMM, {0}, 0.1f, 128);
  OperandType type41(Type::TENSOR_QUANT8_ASYMM, {0, 2, 2, 1}, 0.1f, 128);
  OperandType type42(Type::TENSOR_QUANT8_ASYMM, {0, 2}, 0.1f, 128);
  OperandType type43(Type::TENSOR_QUANT8_ASYMM, {8, 4}, 0.1f, 128);
  OperandType type44(Type::TENSOR_QUANT8_ASYMM, {2, 4}, 0.1f, 128);
  OperandType type7(Type::TENSOR_INT32, {0});
  OperandType type8(Type::FLOAT32, {});
  OperandType type9(Type::INT32, {});
  // Phase 1, operands
  auto scores = model->addOperand(&type36);
  auto deltas = model->addOperand(&type35);
  auto anchors = model->addOperand(&type31);
  auto imageInfo = model->addOperand(&type37);
  auto param = model->addOperand(&type8);
  auto param1 = model->addOperand(&type8);
  auto param2 = model->addOperand(&type9);
  auto param3 = model->addOperand(&type9);
  auto param4 = model->addOperand(&type8);
  auto param5 = model->addOperand(&type8);
  auto layout = model->addOperand(&type0);
  auto scores1 = model->addOperand(&type40);
  auto roi = model->addOperand(&type38);
  auto batches = model->addOperand(&type7);
  auto featureMap = model->addOperand(&type36);
  auto param6 = model->addOperand(&type9);
  auto param7 = model->addOperand(&type9);
  auto param8 = model->addOperand(&type8);
  auto param9 = model->addOperand(&type8);
  auto param10 = model->addOperand(&type9);
  auto param11 = model->addOperand(&type9);
  auto scores2 = model->addOperand(&type41);
  auto weights = model->addOperand(&type43);
  auto bias = model->addOperand(&type32);
  auto param12 = model->addOperand(&type9);
  auto delta = model->addOperand(&type34);
  auto weights1 = model->addOperand(&type44);
  auto bias1 = model->addOperand(&type33);
  auto param13 = model->addOperand(&type9);
  auto scores3 = model->addOperand(&type42);
  auto roi1 = model->addOperand(&type39);
  auto param14 = model->addOperand(&type8);
  auto param15 = model->addOperand(&type9);
  auto param16 = model->addOperand(&type9);
  auto param17 = model->addOperand(&type8);
  auto param18 = model->addOperand(&type8);
  auto param19 = model->addOperand(&type8);
  auto scores4 = model->addOperand(&type40);
  auto roi2 = model->addOperand(&type38);
  auto classes = model->addOperand(&type7);
  auto batches1 = model->addOperand(&type7);
  // Phase 2, operations
  static float param_init[] = {1.0f};
  model->setOperandValue(param, param_init, sizeof(float) * 1);
  static float param1_init[] = {1.0f};
  model->setOperandValue(param1, param1_init, sizeof(float) * 1);
  static int32_t param2_init[] = {-1};
  model->setOperandValue(param2, param2_init, sizeof(int32_t) * 1);
  static int32_t param3_init[] = {-1};
  model->setOperandValue(param3, param3_init, sizeof(int32_t) * 1);
  static float param4_init[] = {0.3f};
  model->setOperandValue(param4, param4_init, sizeof(float) * 1);
  static float param5_init[] = {10.0f};
  model->setOperandValue(param5, param5_init, sizeof(float) * 1);
  static bool8 layout_init[] = {false};
  model->setOperandValue(layout, layout_init, sizeof(bool8) * 1);
  static int32_t param6_init[] = {2};
  model->setOperandValue(param6, param6_init, sizeof(int32_t) * 1);
  static int32_t param7_init[] = {2};
  model->setOperandValue(param7, param7_init, sizeof(int32_t) * 1);
  static float param8_init[] = {1.0f};
  model->setOperandValue(param8, param8_init, sizeof(float) * 1);
  static float param9_init[] = {1.0f};
  model->setOperandValue(param9, param9_init, sizeof(float) * 1);
  static int32_t param10_init[] = {4};
  model->setOperandValue(param10, param10_init, sizeof(int32_t) * 1);
  static int32_t param11_init[] = {4};
  model->setOperandValue(param11, param11_init, sizeof(int32_t) * 1);
  static uint8_t weights_init[] = {138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138};
  model->setOperandValue(weights, weights_init, sizeof(uint8_t) * 32);
  static int32_t bias_init[] = {100, 100, 100, 100, 100, 100, 100, 100};
  model->setOperandValue(bias, bias_init, sizeof(int32_t) * 8);
  static int32_t param12_init[] = {0};
  model->setOperandValue(param12, param12_init, sizeof(int32_t) * 1);
  static uint8_t weights1_init[] = {138, 138, 138, 138, 138, 138, 138, 138};
  model->setOperandValue(weights1, weights1_init, sizeof(uint8_t) * 8);
  static int32_t bias1_init[] = {100, 100};
  model->setOperandValue(bias1, bias1_init, sizeof(int32_t) * 2);
  static int32_t param13_init[] = {0};
  model->setOperandValue(param13, param13_init, sizeof(int32_t) * 1);
  static float param14_init[] = {0.1f};
  model->setOperandValue(param14, param14_init, sizeof(float) * 1);
  static int32_t param15_init[] = {-1};
  model->setOperandValue(param15, param15_init, sizeof(int32_t) * 1);
  static int32_t param16_init[] = {0};
  model->setOperandValue(param16, param16_init, sizeof(int32_t) * 1);
  static float param17_init[] = {0.3f};
  model->setOperandValue(param17, param17_init, sizeof(float) * 1);
  static float param18_init[] = {1.0f};
  model->setOperandValue(param18, param18_init, sizeof(float) * 1);
  static float param19_init[] = {0.1f};
  model->setOperandValue(param19, param19_init, sizeof(float) * 1);
  model->addOperation(ANEURALNETWORKS_GENERATE_PROPOSALS, {scores, deltas, anchors, imageInfo, param, param1, param2, param3, param4, param5, layout}, {scores1, roi, batches});
  model->addOperation(ANEURALNETWORKS_ROI_ALIGN, {featureMap, roi, batches, param6, param7, param8, param9, param10, param11, layout}, {scores2});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights, bias, param12}, {delta});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights1, bias1, param13}, {scores3});
  model->addOperation(ANEURALNETWORKS_AXIS_ALIGNED_BBOX_TRANSFORM, {roi, delta, batches, imageInfo}, {roi1});
  model->addOperation(ANEURALNETWORKS_BOX_WITH_NMS_LIMIT, {scores3, roi1, batches, param14, param15, param16, param17, param18, param19}, {scores4, roi2, classes, batches1});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {scores, deltas, anchors, imageInfo, featureMap},
    {scores1, scores4, roi2, classes, batches1});
  assert(model->isValid());
}

bool is_ignored_zero_sized_quant8(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}

} // namespace generated_tests::bbox_graph
namespace generated_tests::bbox_graph {

void CreateModel_zero_sized_dynamic_output_shape(Model *model) {
  OperandType type0(Type::BOOL, {});
  OperandType type1(Type::TENSOR_FLOAT32, {1, 1, 1, 1});
  OperandType type10(Type::TENSOR_FLOAT32, {0, 2, 2, 1});
  OperandType type11(Type::TENSOR_FLOAT32, {8, 4});
  OperandType type12(Type::TENSOR_FLOAT32, {8});
  OperandType type13(Type::TENSOR_FLOAT32, {0, 8});
  OperandType type14(Type::TENSOR_FLOAT32, {2, 4});
  OperandType type15(Type::TENSOR_FLOAT32, {2});
  OperandType type16(Type::TENSOR_FLOAT32, {0, 2});
  OperandType type2(Type::TENSOR_FLOAT32, {1, 1, 1, 4});
  OperandType type3(Type::TENSOR_FLOAT32, {1, 4});
  OperandType type4(Type::TENSOR_FLOAT32, {1, 2});
  OperandType type45(Type::TENSOR_FLOAT32, {0, 0});
  OperandType type5(Type::TENSOR_FLOAT32, {0});
  OperandType type6(Type::TENSOR_FLOAT32, {0, 4});
  OperandType type7(Type::TENSOR_INT32, {0});
  OperandType type8(Type::FLOAT32, {});
  OperandType type9(Type::INT32, {});
  // Phase 1, operands
  auto scores = model->addOperand(&type1);
  auto deltas = model->addOperand(&type2);
  auto anchors = model->addOperand(&type3);
  auto imageInfo = model->addOperand(&type4);
  auto param = model->addOperand(&type8);
  auto param1 = model->addOperand(&type8);
  auto param2 = model->addOperand(&type9);
  auto param3 = model->addOperand(&type9);
  auto param4 = model->addOperand(&type8);
  auto param5 = model->addOperand(&type8);
  auto layout = model->addOperand(&type0);
  auto scores1 = model->addOperand(&type5);
  auto roi = model->addOperand(&type6);
  auto batches = model->addOperand(&type7);
  auto featureMap = model->addOperand(&type1);
  auto param6 = model->addOperand(&type9);
  auto param7 = model->addOperand(&type9);
  auto param8 = model->addOperand(&type8);
  auto param9 = model->addOperand(&type8);
  auto param10 = model->addOperand(&type9);
  auto param11 = model->addOperand(&type9);
  auto scores2 = model->addOperand(&type10);
  auto weights = model->addOperand(&type11);
  auto bias = model->addOperand(&type12);
  auto param12 = model->addOperand(&type9);
  auto delta = model->addOperand(&type13);
  auto weights1 = model->addOperand(&type14);
  auto bias1 = model->addOperand(&type15);
  auto param13 = model->addOperand(&type9);
  auto scores3 = model->addOperand(&type16);
  auto roi1 = model->addOperand(&type13);
  auto param14 = model->addOperand(&type8);
  auto param15 = model->addOperand(&type9);
  auto param16 = model->addOperand(&type9);
  auto param17 = model->addOperand(&type8);
  auto param18 = model->addOperand(&type8);
  auto param19 = model->addOperand(&type8);
  auto scores4 = model->addOperand(&type5);
  auto roi2 = model->addOperand(&type45);
  auto classes = model->addOperand(&type7);
  auto batches1 = model->addOperand(&type7);
  // Phase 2, operations
  static float param_init[] = {1.0f};
  model->setOperandValue(param, param_init, sizeof(float) * 1);
  static float param1_init[] = {1.0f};
  model->setOperandValue(param1, param1_init, sizeof(float) * 1);
  static int32_t param2_init[] = {-1};
  model->setOperandValue(param2, param2_init, sizeof(int32_t) * 1);
  static int32_t param3_init[] = {-1};
  model->setOperandValue(param3, param3_init, sizeof(int32_t) * 1);
  static float param4_init[] = {0.3f};
  model->setOperandValue(param4, param4_init, sizeof(float) * 1);
  static float param5_init[] = {10.0f};
  model->setOperandValue(param5, param5_init, sizeof(float) * 1);
  static bool8 layout_init[] = {false};
  model->setOperandValue(layout, layout_init, sizeof(bool8) * 1);
  static int32_t param6_init[] = {2};
  model->setOperandValue(param6, param6_init, sizeof(int32_t) * 1);
  static int32_t param7_init[] = {2};
  model->setOperandValue(param7, param7_init, sizeof(int32_t) * 1);
  static float param8_init[] = {1.0f};
  model->setOperandValue(param8, param8_init, sizeof(float) * 1);
  static float param9_init[] = {1.0f};
  model->setOperandValue(param9, param9_init, sizeof(float) * 1);
  static int32_t param10_init[] = {4};
  model->setOperandValue(param10, param10_init, sizeof(int32_t) * 1);
  static int32_t param11_init[] = {4};
  model->setOperandValue(param11, param11_init, sizeof(int32_t) * 1);
  static float weights_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights, weights_init, sizeof(float) * 32);
  static float bias_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(bias, bias_init, sizeof(float) * 8);
  static int32_t param12_init[] = {0};
  model->setOperandValue(param12, param12_init, sizeof(int32_t) * 1);
  static float weights1_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights1, weights1_init, sizeof(float) * 8);
  static float bias1_init[] = {1.0f, 1.0f};
  model->setOperandValue(bias1, bias1_init, sizeof(float) * 2);
  static int32_t param13_init[] = {0};
  model->setOperandValue(param13, param13_init, sizeof(int32_t) * 1);
  static float param14_init[] = {0.1f};
  model->setOperandValue(param14, param14_init, sizeof(float) * 1);
  static int32_t param15_init[] = {-1};
  model->setOperandValue(param15, param15_init, sizeof(int32_t) * 1);
  static int32_t param16_init[] = {0};
  model->setOperandValue(param16, param16_init, sizeof(int32_t) * 1);
  static float param17_init[] = {0.3f};
  model->setOperandValue(param17, param17_init, sizeof(float) * 1);
  static float param18_init[] = {1.0f};
  model->setOperandValue(param18, param18_init, sizeof(float) * 1);
  static float param19_init[] = {0.1f};
  model->setOperandValue(param19, param19_init, sizeof(float) * 1);
  model->addOperation(ANEURALNETWORKS_GENERATE_PROPOSALS, {scores, deltas, anchors, imageInfo, param, param1, param2, param3, param4, param5, layout}, {scores1, roi, batches});
  model->addOperation(ANEURALNETWORKS_ROI_ALIGN, {featureMap, roi, batches, param6, param7, param8, param9, param10, param11, layout}, {scores2});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights, bias, param12}, {delta});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights1, bias1, param13}, {scores3});
  model->addOperation(ANEURALNETWORKS_AXIS_ALIGNED_BBOX_TRANSFORM, {roi, delta, batches, imageInfo}, {roi1});
  model->addOperation(ANEURALNETWORKS_BOX_WITH_NMS_LIMIT, {scores3, roi1, batches, param14, param15, param16, param17, param18, param19}, {scores4, roi2, classes, batches1});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {scores, deltas, anchors, imageInfo, featureMap},
    {scores1, scores4, roi2, classes, batches1});
  assert(model->isValid());
}

bool is_ignored_zero_sized_dynamic_output_shape(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}

} // namespace generated_tests::bbox_graph
namespace generated_tests::bbox_graph {

void CreateModel_zero_sized_dynamic_output_shape_relaxed(Model *model) {
  OperandType type0(Type::BOOL, {});
  OperandType type1(Type::TENSOR_FLOAT32, {1, 1, 1, 1});
  OperandType type10(Type::TENSOR_FLOAT32, {0, 2, 2, 1});
  OperandType type11(Type::TENSOR_FLOAT32, {8, 4});
  OperandType type12(Type::TENSOR_FLOAT32, {8});
  OperandType type13(Type::TENSOR_FLOAT32, {0, 8});
  OperandType type14(Type::TENSOR_FLOAT32, {2, 4});
  OperandType type15(Type::TENSOR_FLOAT32, {2});
  OperandType type16(Type::TENSOR_FLOAT32, {0, 2});
  OperandType type2(Type::TENSOR_FLOAT32, {1, 1, 1, 4});
  OperandType type3(Type::TENSOR_FLOAT32, {1, 4});
  OperandType type4(Type::TENSOR_FLOAT32, {1, 2});
  OperandType type45(Type::TENSOR_FLOAT32, {0, 0});
  OperandType type5(Type::TENSOR_FLOAT32, {0});
  OperandType type6(Type::TENSOR_FLOAT32, {0, 4});
  OperandType type7(Type::TENSOR_INT32, {0});
  OperandType type8(Type::FLOAT32, {});
  OperandType type9(Type::INT32, {});
  // Phase 1, operands
  auto scores = model->addOperand(&type1);
  auto deltas = model->addOperand(&type2);
  auto anchors = model->addOperand(&type3);
  auto imageInfo = model->addOperand(&type4);
  auto param = model->addOperand(&type8);
  auto param1 = model->addOperand(&type8);
  auto param2 = model->addOperand(&type9);
  auto param3 = model->addOperand(&type9);
  auto param4 = model->addOperand(&type8);
  auto param5 = model->addOperand(&type8);
  auto layout = model->addOperand(&type0);
  auto scores1 = model->addOperand(&type5);
  auto roi = model->addOperand(&type6);
  auto batches = model->addOperand(&type7);
  auto featureMap = model->addOperand(&type1);
  auto param6 = model->addOperand(&type9);
  auto param7 = model->addOperand(&type9);
  auto param8 = model->addOperand(&type8);
  auto param9 = model->addOperand(&type8);
  auto param10 = model->addOperand(&type9);
  auto param11 = model->addOperand(&type9);
  auto scores2 = model->addOperand(&type10);
  auto weights = model->addOperand(&type11);
  auto bias = model->addOperand(&type12);
  auto param12 = model->addOperand(&type9);
  auto delta = model->addOperand(&type13);
  auto weights1 = model->addOperand(&type14);
  auto bias1 = model->addOperand(&type15);
  auto param13 = model->addOperand(&type9);
  auto scores3 = model->addOperand(&type16);
  auto roi1 = model->addOperand(&type13);
  auto param14 = model->addOperand(&type8);
  auto param15 = model->addOperand(&type9);
  auto param16 = model->addOperand(&type9);
  auto param17 = model->addOperand(&type8);
  auto param18 = model->addOperand(&type8);
  auto param19 = model->addOperand(&type8);
  auto scores4 = model->addOperand(&type5);
  auto roi2 = model->addOperand(&type45);
  auto classes = model->addOperand(&type7);
  auto batches1 = model->addOperand(&type7);
  // Phase 2, operations
  static float param_init[] = {1.0f};
  model->setOperandValue(param, param_init, sizeof(float) * 1);
  static float param1_init[] = {1.0f};
  model->setOperandValue(param1, param1_init, sizeof(float) * 1);
  static int32_t param2_init[] = {-1};
  model->setOperandValue(param2, param2_init, sizeof(int32_t) * 1);
  static int32_t param3_init[] = {-1};
  model->setOperandValue(param3, param3_init, sizeof(int32_t) * 1);
  static float param4_init[] = {0.3f};
  model->setOperandValue(param4, param4_init, sizeof(float) * 1);
  static float param5_init[] = {10.0f};
  model->setOperandValue(param5, param5_init, sizeof(float) * 1);
  static bool8 layout_init[] = {false};
  model->setOperandValue(layout, layout_init, sizeof(bool8) * 1);
  static int32_t param6_init[] = {2};
  model->setOperandValue(param6, param6_init, sizeof(int32_t) * 1);
  static int32_t param7_init[] = {2};
  model->setOperandValue(param7, param7_init, sizeof(int32_t) * 1);
  static float param8_init[] = {1.0f};
  model->setOperandValue(param8, param8_init, sizeof(float) * 1);
  static float param9_init[] = {1.0f};
  model->setOperandValue(param9, param9_init, sizeof(float) * 1);
  static int32_t param10_init[] = {4};
  model->setOperandValue(param10, param10_init, sizeof(int32_t) * 1);
  static int32_t param11_init[] = {4};
  model->setOperandValue(param11, param11_init, sizeof(int32_t) * 1);
  static float weights_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights, weights_init, sizeof(float) * 32);
  static float bias_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(bias, bias_init, sizeof(float) * 8);
  static int32_t param12_init[] = {0};
  model->setOperandValue(param12, param12_init, sizeof(int32_t) * 1);
  static float weights1_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights1, weights1_init, sizeof(float) * 8);
  static float bias1_init[] = {1.0f, 1.0f};
  model->setOperandValue(bias1, bias1_init, sizeof(float) * 2);
  static int32_t param13_init[] = {0};
  model->setOperandValue(param13, param13_init, sizeof(int32_t) * 1);
  static float param14_init[] = {0.1f};
  model->setOperandValue(param14, param14_init, sizeof(float) * 1);
  static int32_t param15_init[] = {-1};
  model->setOperandValue(param15, param15_init, sizeof(int32_t) * 1);
  static int32_t param16_init[] = {0};
  model->setOperandValue(param16, param16_init, sizeof(int32_t) * 1);
  static float param17_init[] = {0.3f};
  model->setOperandValue(param17, param17_init, sizeof(float) * 1);
  static float param18_init[] = {1.0f};
  model->setOperandValue(param18, param18_init, sizeof(float) * 1);
  static float param19_init[] = {0.1f};
  model->setOperandValue(param19, param19_init, sizeof(float) * 1);
  model->addOperation(ANEURALNETWORKS_GENERATE_PROPOSALS, {scores, deltas, anchors, imageInfo, param, param1, param2, param3, param4, param5, layout}, {scores1, roi, batches});
  model->addOperation(ANEURALNETWORKS_ROI_ALIGN, {featureMap, roi, batches, param6, param7, param8, param9, param10, param11, layout}, {scores2});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights, bias, param12}, {delta});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights1, bias1, param13}, {scores3});
  model->addOperation(ANEURALNETWORKS_AXIS_ALIGNED_BBOX_TRANSFORM, {roi, delta, batches, imageInfo}, {roi1});
  model->addOperation(ANEURALNETWORKS_BOX_WITH_NMS_LIMIT, {scores3, roi1, batches, param14, param15, param16, param17, param18, param19}, {scores4, roi2, classes, batches1});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {scores, deltas, anchors, imageInfo, featureMap},
    {scores1, scores4, roi2, classes, batches1});
  // Phase 4: set relaxed execution
  model->relaxComputationFloat32toFloat16(true);
  assert(model->isValid());
}

bool is_ignored_zero_sized_dynamic_output_shape_relaxed(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}

} // namespace generated_tests::bbox_graph
namespace generated_tests::bbox_graph {

void CreateModel_zero_sized_dynamic_output_shape_float16(Model *model) {
  OperandType type0(Type::BOOL, {});
  OperandType type17(Type::TENSOR_FLOAT16, {1, 4});
  OperandType type18(Type::TENSOR_FLOAT16, {8});
  OperandType type19(Type::TENSOR_FLOAT16, {2});
  OperandType type20(Type::TENSOR_FLOAT16, {0, 8});
  OperandType type21(Type::TENSOR_FLOAT16, {1, 1, 1, 4});
  OperandType type22(Type::TENSOR_FLOAT16, {1, 1, 1, 1});
  OperandType type23(Type::TENSOR_FLOAT16, {1, 2});
  OperandType type24(Type::FLOAT16, {});
  OperandType type25(Type::TENSOR_FLOAT16, {0, 4});
  OperandType type27(Type::TENSOR_FLOAT16, {0, 2, 2, 1});
  OperandType type28(Type::TENSOR_FLOAT16, {0, 2});
  OperandType type29(Type::TENSOR_FLOAT16, {8, 4});
  OperandType type30(Type::TENSOR_FLOAT16, {2, 4});
  OperandType type46(Type::TENSOR_FLOAT16, {0});
  OperandType type47(Type::TENSOR_FLOAT16, {0, 0});
  OperandType type7(Type::TENSOR_INT32, {0});
  OperandType type9(Type::INT32, {});
  // Phase 1, operands
  auto scores = model->addOperand(&type22);
  auto deltas = model->addOperand(&type21);
  auto anchors = model->addOperand(&type17);
  auto imageInfo = model->addOperand(&type23);
  auto param = model->addOperand(&type24);
  auto param1 = model->addOperand(&type24);
  auto param2 = model->addOperand(&type9);
  auto param3 = model->addOperand(&type9);
  auto param4 = model->addOperand(&type24);
  auto param5 = model->addOperand(&type24);
  auto layout = model->addOperand(&type0);
  auto scores1 = model->addOperand(&type46);
  auto roi = model->addOperand(&type25);
  auto batches = model->addOperand(&type7);
  auto featureMap = model->addOperand(&type22);
  auto param6 = model->addOperand(&type9);
  auto param7 = model->addOperand(&type9);
  auto param8 = model->addOperand(&type24);
  auto param9 = model->addOperand(&type24);
  auto param10 = model->addOperand(&type9);
  auto param11 = model->addOperand(&type9);
  auto scores2 = model->addOperand(&type27);
  auto weights = model->addOperand(&type29);
  auto bias = model->addOperand(&type18);
  auto param12 = model->addOperand(&type9);
  auto delta = model->addOperand(&type20);
  auto weights1 = model->addOperand(&type30);
  auto bias1 = model->addOperand(&type19);
  auto param13 = model->addOperand(&type9);
  auto scores3 = model->addOperand(&type28);
  auto roi1 = model->addOperand(&type20);
  auto param14 = model->addOperand(&type24);
  auto param15 = model->addOperand(&type9);
  auto param16 = model->addOperand(&type9);
  auto param17 = model->addOperand(&type24);
  auto param18 = model->addOperand(&type24);
  auto param19 = model->addOperand(&type24);
  auto scores4 = model->addOperand(&type46);
  auto roi2 = model->addOperand(&type47);
  auto classes = model->addOperand(&type7);
  auto batches1 = model->addOperand(&type7);
  // Phase 2, operations
  static _Float16 param_init[] = {1.0f};
  model->setOperandValue(param, param_init, sizeof(_Float16) * 1);
  static _Float16 param1_init[] = {1.0f};
  model->setOperandValue(param1, param1_init, sizeof(_Float16) * 1);
  static int32_t param2_init[] = {-1};
  model->setOperandValue(param2, param2_init, sizeof(int32_t) * 1);
  static int32_t param3_init[] = {-1};
  model->setOperandValue(param3, param3_init, sizeof(int32_t) * 1);
  static _Float16 param4_init[] = {0.30000001192092896f};
  model->setOperandValue(param4, param4_init, sizeof(_Float16) * 1);
  static _Float16 param5_init[] = {10.0f};
  model->setOperandValue(param5, param5_init, sizeof(_Float16) * 1);
  static bool8 layout_init[] = {false};
  model->setOperandValue(layout, layout_init, sizeof(bool8) * 1);
  static int32_t param6_init[] = {2};
  model->setOperandValue(param6, param6_init, sizeof(int32_t) * 1);
  static int32_t param7_init[] = {2};
  model->setOperandValue(param7, param7_init, sizeof(int32_t) * 1);
  static _Float16 param8_init[] = {1.0f};
  model->setOperandValue(param8, param8_init, sizeof(_Float16) * 1);
  static _Float16 param9_init[] = {1.0f};
  model->setOperandValue(param9, param9_init, sizeof(_Float16) * 1);
  static int32_t param10_init[] = {4};
  model->setOperandValue(param10, param10_init, sizeof(int32_t) * 1);
  static int32_t param11_init[] = {4};
  model->setOperandValue(param11, param11_init, sizeof(int32_t) * 1);
  static _Float16 weights_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights, weights_init, sizeof(_Float16) * 32);
  static _Float16 bias_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(bias, bias_init, sizeof(_Float16) * 8);
  static int32_t param12_init[] = {0};
  model->setOperandValue(param12, param12_init, sizeof(int32_t) * 1);
  static _Float16 weights1_init[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  model->setOperandValue(weights1, weights1_init, sizeof(_Float16) * 8);
  static _Float16 bias1_init[] = {1.0f, 1.0f};
  model->setOperandValue(bias1, bias1_init, sizeof(_Float16) * 2);
  static int32_t param13_init[] = {0};
  model->setOperandValue(param13, param13_init, sizeof(int32_t) * 1);
  static _Float16 param14_init[] = {0.10000000149011612f};
  model->setOperandValue(param14, param14_init, sizeof(_Float16) * 1);
  static int32_t param15_init[] = {-1};
  model->setOperandValue(param15, param15_init, sizeof(int32_t) * 1);
  static int32_t param16_init[] = {0};
  model->setOperandValue(param16, param16_init, sizeof(int32_t) * 1);
  static _Float16 param17_init[] = {0.30000001192092896f};
  model->setOperandValue(param17, param17_init, sizeof(_Float16) * 1);
  static _Float16 param18_init[] = {1.0f};
  model->setOperandValue(param18, param18_init, sizeof(_Float16) * 1);
  static _Float16 param19_init[] = {0.10000000149011612f};
  model->setOperandValue(param19, param19_init, sizeof(_Float16) * 1);
  model->addOperation(ANEURALNETWORKS_GENERATE_PROPOSALS, {scores, deltas, anchors, imageInfo, param, param1, param2, param3, param4, param5, layout}, {scores1, roi, batches});
  model->addOperation(ANEURALNETWORKS_ROI_ALIGN, {featureMap, roi, batches, param6, param7, param8, param9, param10, param11, layout}, {scores2});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights, bias, param12}, {delta});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights1, bias1, param13}, {scores3});
  model->addOperation(ANEURALNETWORKS_AXIS_ALIGNED_BBOX_TRANSFORM, {roi, delta, batches, imageInfo}, {roi1});
  model->addOperation(ANEURALNETWORKS_BOX_WITH_NMS_LIMIT, {scores3, roi1, batches, param14, param15, param16, param17, param18, param19}, {scores4, roi2, classes, batches1});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {scores, deltas, anchors, imageInfo, featureMap},
    {scores1, scores4, roi2, classes, batches1});
  assert(model->isValid());
}

bool is_ignored_zero_sized_dynamic_output_shape_float16(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}

} // namespace generated_tests::bbox_graph
namespace generated_tests::bbox_graph {

void CreateModel_zero_sized_dynamic_output_shape_quant8(Model *model) {
  OperandType type0(Type::BOOL, {});
  OperandType type31(Type::TENSOR_QUANT16_SYMM, {1, 4}, 0.125f, 0);
  OperandType type32(Type::TENSOR_INT32, {8}, 0.01f, 0);
  OperandType type33(Type::TENSOR_INT32, {2}, 0.01f, 0);
  OperandType type34(Type::TENSOR_QUANT8_ASYMM, {0, 8}, 0.1f, 128);
  OperandType type35(Type::TENSOR_QUANT8_ASYMM, {1, 1, 1, 4}, 0.1f, 128);
  OperandType type36(Type::TENSOR_QUANT8_ASYMM, {1, 1, 1, 1}, 0.1f, 128);
  OperandType type37(Type::TENSOR_QUANT16_ASYMM, {1, 2}, 0.125f, 0);
  OperandType type38(Type::TENSOR_QUANT16_ASYMM, {0, 4}, 0.125f, 0);
  OperandType type39(Type::TENSOR_QUANT16_ASYMM, {0, 8}, 0.125f, 0);
  OperandType type40(Type::TENSOR_QUANT8_ASYMM, {0}, 0.1f, 128);
  OperandType type41(Type::TENSOR_QUANT8_ASYMM, {0, 2, 2, 1}, 0.1f, 128);
  OperandType type42(Type::TENSOR_QUANT8_ASYMM, {0, 2}, 0.1f, 128);
  OperandType type43(Type::TENSOR_QUANT8_ASYMM, {8, 4}, 0.1f, 128);
  OperandType type44(Type::TENSOR_QUANT8_ASYMM, {2, 4}, 0.1f, 128);
  OperandType type48(Type::TENSOR_QUANT16_ASYMM, {0, 0}, 0.125f, 0);
  OperandType type7(Type::TENSOR_INT32, {0});
  OperandType type8(Type::FLOAT32, {});
  OperandType type9(Type::INT32, {});
  // Phase 1, operands
  auto scores = model->addOperand(&type36);
  auto deltas = model->addOperand(&type35);
  auto anchors = model->addOperand(&type31);
  auto imageInfo = model->addOperand(&type37);
  auto param = model->addOperand(&type8);
  auto param1 = model->addOperand(&type8);
  auto param2 = model->addOperand(&type9);
  auto param3 = model->addOperand(&type9);
  auto param4 = model->addOperand(&type8);
  auto param5 = model->addOperand(&type8);
  auto layout = model->addOperand(&type0);
  auto scores1 = model->addOperand(&type40);
  auto roi = model->addOperand(&type38);
  auto batches = model->addOperand(&type7);
  auto featureMap = model->addOperand(&type36);
  auto param6 = model->addOperand(&type9);
  auto param7 = model->addOperand(&type9);
  auto param8 = model->addOperand(&type8);
  auto param9 = model->addOperand(&type8);
  auto param10 = model->addOperand(&type9);
  auto param11 = model->addOperand(&type9);
  auto scores2 = model->addOperand(&type41);
  auto weights = model->addOperand(&type43);
  auto bias = model->addOperand(&type32);
  auto param12 = model->addOperand(&type9);
  auto delta = model->addOperand(&type34);
  auto weights1 = model->addOperand(&type44);
  auto bias1 = model->addOperand(&type33);
  auto param13 = model->addOperand(&type9);
  auto scores3 = model->addOperand(&type42);
  auto roi1 = model->addOperand(&type39);
  auto param14 = model->addOperand(&type8);
  auto param15 = model->addOperand(&type9);
  auto param16 = model->addOperand(&type9);
  auto param17 = model->addOperand(&type8);
  auto param18 = model->addOperand(&type8);
  auto param19 = model->addOperand(&type8);
  auto scores4 = model->addOperand(&type40);
  auto roi2 = model->addOperand(&type48);
  auto classes = model->addOperand(&type7);
  auto batches1 = model->addOperand(&type7);
  // Phase 2, operations
  static float param_init[] = {1.0f};
  model->setOperandValue(param, param_init, sizeof(float) * 1);
  static float param1_init[] = {1.0f};
  model->setOperandValue(param1, param1_init, sizeof(float) * 1);
  static int32_t param2_init[] = {-1};
  model->setOperandValue(param2, param2_init, sizeof(int32_t) * 1);
  static int32_t param3_init[] = {-1};
  model->setOperandValue(param3, param3_init, sizeof(int32_t) * 1);
  static float param4_init[] = {0.3f};
  model->setOperandValue(param4, param4_init, sizeof(float) * 1);
  static float param5_init[] = {10.0f};
  model->setOperandValue(param5, param5_init, sizeof(float) * 1);
  static bool8 layout_init[] = {false};
  model->setOperandValue(layout, layout_init, sizeof(bool8) * 1);
  static int32_t param6_init[] = {2};
  model->setOperandValue(param6, param6_init, sizeof(int32_t) * 1);
  static int32_t param7_init[] = {2};
  model->setOperandValue(param7, param7_init, sizeof(int32_t) * 1);
  static float param8_init[] = {1.0f};
  model->setOperandValue(param8, param8_init, sizeof(float) * 1);
  static float param9_init[] = {1.0f};
  model->setOperandValue(param9, param9_init, sizeof(float) * 1);
  static int32_t param10_init[] = {4};
  model->setOperandValue(param10, param10_init, sizeof(int32_t) * 1);
  static int32_t param11_init[] = {4};
  model->setOperandValue(param11, param11_init, sizeof(int32_t) * 1);
  static uint8_t weights_init[] = {138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138};
  model->setOperandValue(weights, weights_init, sizeof(uint8_t) * 32);
  static int32_t bias_init[] = {100, 100, 100, 100, 100, 100, 100, 100};
  model->setOperandValue(bias, bias_init, sizeof(int32_t) * 8);
  static int32_t param12_init[] = {0};
  model->setOperandValue(param12, param12_init, sizeof(int32_t) * 1);
  static uint8_t weights1_init[] = {138, 138, 138, 138, 138, 138, 138, 138};
  model->setOperandValue(weights1, weights1_init, sizeof(uint8_t) * 8);
  static int32_t bias1_init[] = {100, 100};
  model->setOperandValue(bias1, bias1_init, sizeof(int32_t) * 2);
  static int32_t param13_init[] = {0};
  model->setOperandValue(param13, param13_init, sizeof(int32_t) * 1);
  static float param14_init[] = {0.1f};
  model->setOperandValue(param14, param14_init, sizeof(float) * 1);
  static int32_t param15_init[] = {-1};
  model->setOperandValue(param15, param15_init, sizeof(int32_t) * 1);
  static int32_t param16_init[] = {0};
  model->setOperandValue(param16, param16_init, sizeof(int32_t) * 1);
  static float param17_init[] = {0.3f};
  model->setOperandValue(param17, param17_init, sizeof(float) * 1);
  static float param18_init[] = {1.0f};
  model->setOperandValue(param18, param18_init, sizeof(float) * 1);
  static float param19_init[] = {0.1f};
  model->setOperandValue(param19, param19_init, sizeof(float) * 1);
  model->addOperation(ANEURALNETWORKS_GENERATE_PROPOSALS, {scores, deltas, anchors, imageInfo, param, param1, param2, param3, param4, param5, layout}, {scores1, roi, batches});
  model->addOperation(ANEURALNETWORKS_ROI_ALIGN, {featureMap, roi, batches, param6, param7, param8, param9, param10, param11, layout}, {scores2});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights, bias, param12}, {delta});
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {scores2, weights1, bias1, param13}, {scores3});
  model->addOperation(ANEURALNETWORKS_AXIS_ALIGNED_BBOX_TRANSFORM, {roi, delta, batches, imageInfo}, {roi1});
  model->addOperation(ANEURALNETWORKS_BOX_WITH_NMS_LIMIT, {scores3, roi1, batches, param14, param15, param16, param17, param18, param19}, {scores4, roi2, classes, batches1});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {scores, deltas, anchors, imageInfo, featureMap},
    {scores1, scores4, roi2, classes, batches1});
  assert(model->isValid());
}

bool is_ignored_zero_sized_dynamic_output_shape_quant8(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}

} // namespace generated_tests::bbox_graph
