// clang-format off
// Generated file (from: l2_pool_float_large_relaxed.mod.py). Do not edit
#include "../../TestGenerated.h"

namespace l2_pool_float_large_relaxed {
// Generated l2_pool_float_large_relaxed test
#include "generated/examples/l2_pool_float_large_relaxed.example.cpp"
// Generated model constructor
#include "generated/models/l2_pool_float_large_relaxed.model.cpp"
} // namespace l2_pool_float_large_relaxed

TEST_F(GeneratedTests, l2_pool_float_large_relaxed) {
    execute(l2_pool_float_large_relaxed::CreateModel,
            l2_pool_float_large_relaxed::is_ignored,
            l2_pool_float_large_relaxed::get_examples());
}

