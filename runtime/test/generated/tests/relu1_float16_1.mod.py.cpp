// clang-format off
// Generated file (from: relu1_float16_1.mod.py). Do not edit
#include "../../TestGenerated.h"

namespace relu1_float16_1 {
// Generated relu1_float16_1 test
#include "generated/examples/relu1_float16_1.example.cpp"
// Generated model constructor
#include "generated/models/relu1_float16_1.model.cpp"
} // namespace relu1_float16_1

TEST_F(GeneratedTests, relu1_float16_1) {
    execute(relu1_float16_1::CreateModel,
            relu1_float16_1::is_ignored,
            relu1_float16_1::get_examples());
}

