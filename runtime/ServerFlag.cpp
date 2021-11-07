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

#define LOG_TAG "ServerFlag"

#include <FlagUtils.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <server_configurable_flags/get_flags.h>
#include <stdint.h>

#include <string>

namespace android {
namespace nn {

namespace {
int64_t getServerFlagInt(std::string flagName, int64_t defaultValue, int64_t minValue,
                         int64_t maxValue) {
    int64_t flagValue = defaultValue;
    if (!android::base::ParseInt(
                server_configurable_flags::GetServerConfigurableFlag(
                        std::string(kExprCategoryName), flagName, std::to_string(defaultValue)),
                &flagValue, minValue, maxValue)) {
        LOG(WARNING) << "Failed to parse flag " << flagName << " to int type. errno: " << errno;
    }
    return flagValue;
}

}  // namespace

int64_t _getServerFeatureLevelFlag() {
    return getServerFlagInt(kCurrentFeatureLevelFlagName, kDefaultFeatureLevelNum,
                            kMinFeatureLevelNum, kMaxFeatureLevelNum);
}

}  // namespace nn
}  // namespace android