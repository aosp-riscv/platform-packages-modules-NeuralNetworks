// clang-format off
// Generated file (from: depthwise_conv2d_float_large.mod.py). Do not edit
#include "../../TestGenerated.h"

namespace depthwise_conv2d_float_large {
// Generated depthwise_conv2d_float_large test
#include "generated/examples/depthwise_conv2d_float_large.example.cpp"
// Generated model constructor
#include "generated/models/depthwise_conv2d_float_large.model.cpp"
} // namespace depthwise_conv2d_float_large

TEST_F(GeneratedTests, depthwise_conv2d_float_large) {
    execute(depthwise_conv2d_float_large::CreateModel,
            depthwise_conv2d_float_large::is_ignored,
            depthwise_conv2d_float_large::get_examples());
}

