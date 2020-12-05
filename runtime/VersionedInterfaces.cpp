/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "VersionedInterfaces"

#include "VersionedInterfaces.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/scopeguard.h>
#include <android-base/thread_annotations.h>
#include <cutils/native_handle.h>
#include <fcntl.h>
#include <nnapi/hal/1.3/Utils.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Callbacks.h"
#include "ExecutionBurstController.h"
#include "MetaModel.h"
#include "Tracing.h"
#include "Utils.h"

/*
 * Some notes about HIDL interface objects and lifetimes across processes:
 *
 * All HIDL interface objects inherit from IBase, which itself inherits from
 * ::android::RefBase. As such, all HIDL interface objects are reference counted
 * and must be owned through ::android::sp (or referenced through ::android::wp).
 * Allocating RefBase objects on the stack will log errors and may result in
 * crashes, and deleting a RefBase object through another means (e.g., "delete",
 * "free", or RAII-cleanup through std::unique_ptr or some equivalent) will
 * result in double-free and/or use-after-free undefined behavior.
 *
 * HIDL/Binder manages the reference count of HIDL interface objects
 * automatically across processes. If a process that references (but did not
 * create) the HIDL interface object dies, HIDL/Binder ensures any reference
 * count it held is properly released. (Caveat: it might be possible that
 * HIDL/Binder behave strangely with ::android::wp references.)
 *
 * If the process which created the HIDL interface object dies, any call on this
 * object from another process will result in a HIDL transport error with the
 * code DEAD_OBJECT.
 */

/*
 * Some notes about asynchronous calls across HIDL:
 *
 * For synchronous calls across HIDL, if an error occurs after the function was
 * called but before it returns, HIDL will return a transport error. For
 * example, if the message cannot be delivered to the server process or if the
 * server process dies before returning a result, HIDL will return from the
 * function with the appropriate transport error in the Return<> object which
 * can be queried with Return<>::isOk(), Return<>::isDeadObject(),
 * Return<>::description(), etc.
 *
 * However, HIDL offers no such error management in the case of asynchronous
 * calls. By default, if the client launches an asynchronous task and the server
 * fails to return a result through the callback, the client will be left
 * waiting indefinitely for a result it will never receive.
 *
 * In the NNAPI, IDevice::prepareModel* and IPreparedModel::execute* (but not
 * IPreparedModel::executeSynchronously*) are asynchronous calls across HIDL.
 * Specifically, these asynchronous functions are called with a HIDL interface
 * callback object (IPrepareModelCallback for IDevice::prepareModel* and
 * IExecutionCallback for IPreparedModel::execute*) and are expected to quickly
 * return, and the results are returned at a later time through these callback
 * objects.
 *
 * To protect against the case when the server dies after the asynchronous task
 * was called successfully but before the results could be returned, HIDL
 * provides an object called a "hidl_death_recipient", which can be used to
 * detect when an interface object (and more generally, the server process) has
 * died. VersionedInterfaces uses hidl_death_recipients to detect when the
 * driver process has died, and VersionedInterfaces will unblock any thread
 * waiting on the results of a callback object that may otherwise not be
 * signaled.
 */

namespace android {
namespace nn {

// anonymous namespace
namespace {

void sendFailureMessage(V1_3::IPreparedModelCallback* cb) {
    CHECK(cb != nullptr);
    cb->notify_1_3(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr);
}

// This class is thread safe
template <typename Callback>
class DeathHandler : public hardware::hidl_death_recipient {
   public:
    void serviceDied(uint64_t /*cookie*/, const wp<hidl::base::V1_0::IBase>& /*who*/) override {
        LOG(ERROR) << "DeathHandler::serviceDied -- service unexpectedly died!";
        std::lock_guard<std::mutex> hold(mMutex);
        std::for_each(mCallbacks.begin(), mCallbacks.end(),
                      [](const auto& cb) { cb->notifyAsDeadObject(); });
    }

    [[nodiscard]] base::ScopeGuard<std::function<void()>> protectCallback(
            const sp<Callback>& callback) {
        registerCallback(callback);
        return ::android::base::make_scope_guard(
                [this, callback] { unregisterCallback(callback); });
    }

   private:
    void registerCallback(const sp<Callback>& callback) {
        std::lock_guard<std::mutex> hold(mMutex);
        mCallbacks.push_back(callback);
    }

    void unregisterCallback(const sp<Callback>& callback) {
        std::lock_guard<std::mutex> hold(mMutex);
        mCallbacks.erase(std::remove(mCallbacks.begin(), mCallbacks.end(), callback),
                         mCallbacks.end());
    }

    std::mutex mMutex;
    std::vector<sp<Callback>> mCallbacks GUARDED_BY(mMutex);
};

}  // anonymous namespace

class IDeviceDeathHandler : public DeathHandler<PreparedModelCallback> {};
class IPreparedModelDeathHandler : public DeathHandler<ExecutionCallback> {};

static std::pair<int, std::shared_ptr<VersionedIPreparedModel>> makeVersionedIPreparedModel(
        sp<V1_0::IPreparedModel> preparedModel) {
    CHECK(preparedModel != nullptr)
            << "makeVersionedIPreparedModel passed invalid preparedModel object.";

    // create death handler object
    sp<IPreparedModelDeathHandler> deathHandler = new IPreparedModelDeathHandler();

    // linkToDeath registers a callback that will be invoked on service death to
    // proactively handle service crashes. If the linkToDeath call fails,
    // asynchronous calls are susceptible to hangs if the service crashes before
    // providing the response.
    const hardware::Return<bool> ret = preparedModel->linkToDeath(deathHandler, 0);
    if (ret.isDeadObject()) {
        LOG(ERROR) << "makeVersionedIPreparedModel failed to register a death recipient for the "
                      "IPreparedModel object because the IPreparedModel object is dead.";
        return {ANEURALNETWORKS_DEAD_OBJECT, nullptr};
    }
    if (!ret.isOk()) {
        LOG(ERROR) << "makeVersionedIPreparedModel failed to register a death recipient for the "
                      "IPreparedModel object because of failure: "
                   << ret.description();
        return {ANEURALNETWORKS_OP_FAILED, nullptr};
    }
    if (ret != true) {
        LOG(ERROR) << "makeVersionedIPreparedModel failed to register a death recipient for the "
                      "IPreparedModel object.";
        return {ANEURALNETWORKS_OP_FAILED, nullptr};
    }

    // return a valid VersionedIPreparedModel object
    return {ANEURALNETWORKS_NO_ERROR, std::make_shared<VersionedIPreparedModel>(
                                              std::move(preparedModel), std::move(deathHandler))};
}

VersionedIPreparedModel::VersionedIPreparedModel(sp<V1_0::IPreparedModel> preparedModel,
                                                 sp<IPreparedModelDeathHandler> deathHandler)
    : mPreparedModelV1_0(std::move(preparedModel)),
      mPreparedModelV1_2(V1_2::IPreparedModel::castFrom(mPreparedModelV1_0).withDefault(nullptr)),
      mPreparedModelV1_3(V1_3::IPreparedModel::castFrom(mPreparedModelV1_0).withDefault(nullptr)),
      mDeathHandler(std::move(deathHandler)) {}

VersionedIPreparedModel::~VersionedIPreparedModel() {
    // It is safe to ignore any errors resulting from this unlinkToDeath call
    // because the VersionedIPreparedModel object is already being destroyed and
    // its underlying IPreparedModel object is no longer being used by the NN
    // runtime.
    mPreparedModelV1_0->unlinkToDeath(mDeathHandler).isOk();
}

std::tuple<int, std::vector<OutputShape>, Timing> VersionedIPreparedModel::executeAsynchronously(
        const Request& request, MeasureTiming measure, const std::optional<Deadline>& deadline,
        const OptionalTimeoutDuration& loopTimeoutDuration) const {
    const auto failDeadObject = []() -> std::tuple<int, std::vector<OutputShape>, Timing> {
        return {ANEURALNETWORKS_DEAD_OBJECT, {}, {}};
    };
    const auto failWithStatus = [](V1_3::ErrorStatus status) {
        return getExecutionResult(status, {}, {});
    };
    const auto getResults = [failDeadObject](const ExecutionCallback& cb) {
        if (cb.isDeadObject()) {
            return failDeadObject();
        }
        return getExecutionResult(cb.getStatus(), cb.getOutputShapes(), cb.getTiming());
    };

    const sp<ExecutionCallback> callback = new ExecutionCallback();
    const auto scoped = mDeathHandler->protectCallback(callback);

    // version 1.3 HAL
    const V1_3::Request request13 = convertToV1_3(request);
    if (mPreparedModelV1_3 != nullptr) {
        const auto otp = makeTimePoint(deadline);
        hardware::Return<V1_3::ErrorStatus> ret = mPreparedModelV1_3->execute_1_3(
                request13, convertToV1_2(measure), convertToV1_3(otp),
                convertToV1_3(loopTimeoutDuration), callback);
        if (ret.isDeadObject()) {
            LOG(ERROR) << "execute_1_3 failure: " << ret.description();
            return failDeadObject();
        }
        if (!ret.isOk()) {
            LOG(ERROR) << "execute_1_3 failure: " << ret.description();
            return failWithStatus(V1_3::ErrorStatus::GENERAL_FAILURE);
        }
        if (ret != V1_3::ErrorStatus::NONE) {
            LOG(ERROR) << "execute_1_3 returned " << toString(static_cast<V1_3::ErrorStatus>(ret));
            return failWithStatus(ret);
        }
        callback->wait();
        return getResults(*callback);
    }

    // version 1.2 HAL
    if (mPreparedModelV1_2 != nullptr) {
        const bool compliant = compliantWithV1_2(request13);
        if (!compliant) {
            LOG(ERROR) << "Could not handle execute_1_2!";
            return failWithStatus(V1_3::ErrorStatus::GENERAL_FAILURE);
        }
        const V1_0::Request request12 = convertToV1_2(request13);
        hardware::Return<V1_0::ErrorStatus> ret =
                mPreparedModelV1_2->execute_1_2(request12, convertToV1_2(measure), callback);
        if (ret.isDeadObject()) {
            LOG(ERROR) << "execute_1_2 failure: " << ret.description();
            return failDeadObject();
        }
        if (!ret.isOk()) {
            LOG(ERROR) << "execute_1_2 failure: " << ret.description();
            return failWithStatus(V1_3::ErrorStatus::GENERAL_FAILURE);
        }
        const V1_0::ErrorStatus status = static_cast<V1_0::ErrorStatus>(ret);
        if (status != V1_0::ErrorStatus::NONE) {
            LOG(ERROR) << "execute_1_2 returned " << toString(status);
            return failWithStatus(convertToV1_3(status));
        }
        callback->wait();
        return getResults(*callback);
    }

    // version 1.0 HAL
    if (mPreparedModelV1_0 != nullptr) {
        const bool compliant = compliantWithV1_0(request13);
        if (!compliant) {
            LOG(ERROR) << "Could not handle execute!";
            return failWithStatus(V1_3::ErrorStatus::GENERAL_FAILURE);
        }
        const V1_0::Request request10 = convertToV1_0(request13);
        hardware::Return<V1_0::ErrorStatus> ret = mPreparedModelV1_0->execute(request10, callback);
        if (ret.isDeadObject()) {
            LOG(ERROR) << "execute failure: " << ret.description();
            return failDeadObject();
        }
        if (!ret.isOk()) {
            LOG(ERROR) << "execute failure: " << ret.description();
            return failWithStatus(V1_3::ErrorStatus::GENERAL_FAILURE);
        }
        const V1_0::ErrorStatus status = static_cast<V1_0::ErrorStatus>(ret);
        if (status != V1_0::ErrorStatus::NONE) {
            LOG(ERROR) << "execute returned " << toString(status);
            return failWithStatus(convertToV1_3(status));
        }
        callback->wait();
        return getResults(*callback);
    }

    // No prepared model available
    LOG(ERROR) << "executeAsynchronously called with no preparedModel";
    return failWithStatus(V1_3::ErrorStatus::GENERAL_FAILURE);
}

std::tuple<int, std::vector<OutputShape>, Timing> VersionedIPreparedModel::executeSynchronously(
        const Request& request, MeasureTiming measure, const std::optional<Deadline>& deadline,
        const OptionalTimeoutDuration& loopTimeoutDuration) const {
    const std::tuple<int, std::vector<OutputShape>, Timing> kDeadObject = {
            ANEURALNETWORKS_DEAD_OBJECT, {}, {}};
    const auto kFailure = getExecutionResult(ErrorStatus::GENERAL_FAILURE, {}, {});

    // version 1.3 HAL
    const V1_3::Request request13 = convertToV1_3(request);
    if (mPreparedModelV1_3 != nullptr) {
        std::tuple<int, std::vector<OutputShape>, Timing> result;
        const auto otp = makeTimePoint(deadline);
        hardware::Return<void> ret = mPreparedModelV1_3->executeSynchronously_1_3(
                request13, convertToV1_2(measure), convertToV1_3(otp),
                convertToV1_3(loopTimeoutDuration),
                [&result](V1_3::ErrorStatus error,
                          const hardware::hidl_vec<V1_2::OutputShape>& outputShapes,
                          const V1_2::Timing& timing) {
                    result = getExecutionResult(error, outputShapes, timing);
                });
        if (ret.isDeadObject()) {
            LOG(ERROR) << "executeSynchronously_1_3 failure: " << ret.description();
            return kDeadObject;
        }
        if (!ret.isOk()) {
            LOG(ERROR) << "executeSynchronously_1_3 failure: " << ret.description();
            return kFailure;
        }
        return result;
    }

    // version 1.2 HAL
    if (mPreparedModelV1_2 != nullptr) {
        const bool compliant = compliantWithV1_2(request13);
        if (!compliant) {
            LOG(ERROR) << "Could not handle executeSynchronously!";
            return kFailure;
        }
        const V1_0::Request request12 = convertToV1_2(request13);

        std::tuple<int, std::vector<OutputShape>, Timing> result;
        hardware::Return<void> ret = mPreparedModelV1_2->executeSynchronously(
                request12, convertToV1_2(measure),
                [&result](V1_0::ErrorStatus error,
                          const hardware::hidl_vec<V1_2::OutputShape>& outputShapes,
                          const V1_2::Timing& timing) {
                    result = getExecutionResult(convertToV1_3(error), outputShapes, timing);
                });
        if (ret.isDeadObject()) {
            LOG(ERROR) << "executeSynchronously failure: " << ret.description();
            return kDeadObject;
        }
        if (!ret.isOk()) {
            LOG(ERROR) << "executeSynchronously failure: " << ret.description();
            return kFailure;
        }
        return result;
    }

    // Fallback to asynchronous execution.
    return executeAsynchronously(request, measure, deadline, loopTimeoutDuration);
}

std::tuple<int, std::vector<OutputShape>, Timing> VersionedIPreparedModel::execute(
        const Request& request, MeasureTiming measure, const std::optional<Deadline>& deadline,
        const OptionalTimeoutDuration& loopTimeoutDuration, bool preferSynchronous) const {
    if (preferSynchronous) {
        VLOG(EXECUTION) << "Before executeSynchronously() " << SHOW_IF_DEBUG(request);
        return executeSynchronously(request, measure, deadline, loopTimeoutDuration);
    }

    VLOG(EXECUTION) << "Before executeAsynchronously() " << SHOW_IF_DEBUG(request);
    return executeAsynchronously(request, measure, deadline, loopTimeoutDuration);
}

// This is the amount of time the ExecutionBurstController should spend polling
// the FMQ to see if it has data available before it should fall back to
// waiting on the futex.
static std::chrono::microseconds getPollingTimeWindow() {
    constexpr int32_t defaultPollingTimeWindow = 50;
#ifdef NN_DEBUGGABLE
    constexpr int32_t minPollingTimeWindow = 0;
    const int32_t selectedPollingTimeWindow =
            base::GetIntProperty("debug.nn.burst-conrtoller-polling-window",
                                 defaultPollingTimeWindow, minPollingTimeWindow);
    return std::chrono::microseconds{selectedPollingTimeWindow};
#else
    return std::chrono::microseconds{defaultPollingTimeWindow};
#endif  // NN_DEBUGGABLE
}

std::shared_ptr<ExecutionBurstController> VersionedIPreparedModel::configureExecutionBurst(
        bool preferPowerOverLatency) const {
    if (mPreparedModelV1_2 == nullptr) {
        return nullptr;
    }
    const auto pollingTimeWindow =
            (preferPowerOverLatency ? std::chrono::microseconds{0} : getPollingTimeWindow());
    return ExecutionBurstController::create(mPreparedModelV1_2, pollingTimeWindow);
}

static std::pair<V1_3::ErrorStatus, V1_3::Capabilities> getCapabilitiesFunction(
        V1_3::IDevice* device) {
    CHECK(device != nullptr);
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_INITIALIZATION, "getCapabilities_1_3");
    const std::pair<V1_3::ErrorStatus, V1_3::Capabilities> kFailure = {
            V1_3::ErrorStatus::GENERAL_FAILURE, {}};
    std::pair<V1_3::ErrorStatus, V1_3::Capabilities> result = kFailure;
    const hardware::Return<void> ret = device->getCapabilities_1_3(
            [&result](V1_3::ErrorStatus error, const V1_3::Capabilities& capabilities) {
                result = std::make_pair(error, capabilities);
            });
    if (!ret.isOk()) {
        LOG(ERROR) << "getCapabilities_1_3 failure: " << ret.description();
        return kFailure;
    }
    return result;
}

static GeneralResult<std::pair<Timing, Timing>> convertFencedExecutionCallbackResults(
        const V1_2::Timing& timingLaunched, const V1_2::Timing& timingFenced) {
    return std::make_pair(NN_TRY(V1_3::utils::validatedConvertToCanonical(timingLaunched)),
                          NN_TRY(V1_3::utils::validatedConvertToCanonical(timingFenced)));
}

std::tuple<int, SyncFence, ExecuteFencedInfoCallback, Timing>
VersionedIPreparedModel::executeFenced(const Request& request,
                                       const std::vector<SyncFence>& waitFor, MeasureTiming measure,
                                       const std::optional<Deadline>& deadline,
                                       const OptionalTimeoutDuration& loopTimeoutDuration,
                                       const OptionalTimeoutDuration& timeoutDurationAfterFence) {
    // version 1.3 HAL
    hardware::hidl_handle hidlSyncFence;
    Timing timing = {UINT64_MAX, UINT64_MAX};
    if (mPreparedModelV1_3 != nullptr) {
        ErrorStatus errorStatus;
        sp<V1_3::IFencedExecutionCallback> hidlCallback;
        const auto otp = makeTimePoint(deadline);
        auto waitForHandles = hal::utils::convertSyncFences(waitFor);
        if (!waitForHandles.has_value()) {
            LOG(ERROR) << "executeFenced failure: " << waitForHandles.error().message;
            return std::make_tuple(ANEURALNETWORKS_OP_FAILED, SyncFence::createAsSignaled(),
                                   nullptr, timing);
        }
        hardware::Return<void> ret = mPreparedModelV1_3->executeFenced(
                convertToV1_3(request), std::move(waitForHandles).value(), convertToV1_2(measure),
                convertToV1_3(otp), convertToV1_3(loopTimeoutDuration),
                convertToV1_3(timeoutDurationAfterFence),
                [&hidlSyncFence, &errorStatus, &hidlCallback](
                        V1_3::ErrorStatus error, const hardware::hidl_handle& handle,
                        const sp<V1_3::IFencedExecutionCallback>& callback) {
                    hidlSyncFence = handle;
                    errorStatus = uncheckedConvert(error);
                    hidlCallback = callback;
                });
        if (!ret.isOk()) {
            LOG(ERROR) << "executeFenced failure: " << ret.description();
            return std::make_tuple(ANEURALNETWORKS_OP_FAILED, SyncFence::createAsSignaled(),
                                   nullptr, timing);
        }
        if (errorStatus != ErrorStatus::NONE) {
            LOG(ERROR) << "executeFenced returned " << errorStatus;
            return std::make_tuple(convertErrorStatusToResultCode(errorStatus),
                                   SyncFence::createAsSignaled(), nullptr, timing);
        }

        auto sharedHandle = hal::utils::sharedHandleFromNativeHandle(hidlSyncFence);
        if (!sharedHandle.has_value()) {
            LOG(ERROR) << "executeFenced failure: " << sharedHandle.error().message;
            return std::make_tuple(ANEURALNETWORKS_OP_FAILED, SyncFence::createAsSignaled(),
                                   nullptr, timing);
        }
        auto syncFence = sharedHandle.value() == nullptr
                                 ? SyncFence::createAsSignaled()
                                 : SyncFence::create(std::move(sharedHandle).value());
        if (!syncFence.has_value()) {
            LOG(ERROR) << "executeFenced failure: " << syncFence.error();
            return std::make_tuple(ANEURALNETWORKS_OP_FAILED, SyncFence::createAsSignaled(),
                                   nullptr, timing);
        }

        ExecuteFencedInfoCallback callback =
                [hidlCallback]() -> GeneralResult<std::pair<Timing, Timing>> {
            GeneralResult<std::pair<Timing, Timing>> result = NN_ERROR() << "uninitialized";
            auto cb = [&result](V1_3::ErrorStatus status, const V1_2::Timing& timingLaunched,
                                const V1_2::Timing& timingFenced) {
                if (status != V1_3::ErrorStatus::NONE) {
                    const auto canonical =
                            V1_3::utils::validatedConvertToCanonical(status).value_or(
                                    ErrorStatus::GENERAL_FAILURE);
                    result = NN_ERROR(canonical)
                             << "getExecutionInfo failed with " << toString(status);
                } else {
                    result = convertFencedExecutionCallbackResults(timingLaunched, timingFenced);
                }
            };

            const auto ret = hidlCallback->getExecutionInfo(cb);
            NN_TRY(hal::utils::handleTransportError(ret));
            return result;
        };

        return std::make_tuple(ANEURALNETWORKS_NO_ERROR, std::move(syncFence).value(),
                               std::move(callback), timing);
    }

    // fallback to synchronous execution if sync_fence is not supported
    // first wait for all sync fences to be ready.
    LOG(INFO) << "No drivers able to handle sync fences, falling back to regular execution";
    for (const auto& fence : waitFor) {
        if (!fence.hasFd() || fence.getFd() <= 0) {
            return std::make_tuple(ANEURALNETWORKS_BAD_DATA, SyncFence::createAsSignaled(), nullptr,
                                   timing);
        }
        auto r = fence.syncWait({/* no timeout */});
        if (r != SyncFence::FenceState::SIGNALED) {
            LOG(ERROR) << "syncWait failed, fd: " << fence.getFd() << ", state: " << r;
            return std::make_tuple(ANEURALNETWORKS_OP_FAILED, SyncFence::createAsSignaled(),
                                   nullptr, timing);
        }
    }
    int errorCode;
    std::tie(errorCode, std::ignore, timing) =
            executeSynchronously(request, measure, deadline, loopTimeoutDuration);
    return std::make_tuple(errorCode, SyncFence::createAsSignaled(), nullptr, timing);
}

static std::pair<V1_3::ErrorStatus, V1_3::Capabilities> getCapabilitiesFunction(
        V1_2::IDevice* device) {
    CHECK(device != nullptr);
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_INITIALIZATION, "getCapabilities_1_2");
    const std::pair<V1_3::ErrorStatus, V1_3::Capabilities> kFailure = {
            V1_3::ErrorStatus::GENERAL_FAILURE, {}};
    std::pair<V1_3::ErrorStatus, V1_3::Capabilities> result = kFailure;
    const hardware::Return<void> ret = device->getCapabilities_1_2(
            [&result](V1_0::ErrorStatus error, const V1_2::Capabilities& capabilities) {
                result = std::make_pair(convertToV1_3(error), convertToV1_3(capabilities));
            });
    if (!ret.isOk()) {
        LOG(ERROR) << "getCapabilities_1_2 failure: " << ret.description();
        return kFailure;
    }
    return result;
}

static std::pair<V1_3::ErrorStatus, V1_3::Capabilities> getCapabilitiesFunction(
        V1_1::IDevice* device) {
    CHECK(device != nullptr);
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_INITIALIZATION, "getCapabilities_1_1");
    const std::pair<V1_3::ErrorStatus, V1_3::Capabilities> kFailure = {
            V1_3::ErrorStatus::GENERAL_FAILURE, {}};
    std::pair<V1_3::ErrorStatus, V1_3::Capabilities> result = kFailure;
    const hardware::Return<void> ret = device->getCapabilities_1_1(
            [&result](V1_0::ErrorStatus error, const V1_1::Capabilities& capabilities) {
                // Time taken to convert capabilities is trivial
                result = std::make_pair(convertToV1_3(error), convertToV1_3(capabilities));
            });
    if (!ret.isOk()) {
        LOG(ERROR) << "getCapabilities_1_1 failure: " << ret.description();
        return kFailure;
    }
    return result;
}

static std::pair<V1_3::ErrorStatus, V1_3::Capabilities> getCapabilitiesFunction(
        V1_0::IDevice* device) {
    CHECK(device != nullptr);
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_INITIALIZATION, "getCapabilities");
    const std::pair<V1_3::ErrorStatus, V1_3::Capabilities> kFailure = {
            V1_3::ErrorStatus::GENERAL_FAILURE, {}};
    std::pair<V1_3::ErrorStatus, V1_3::Capabilities> result = kFailure;
    const hardware::Return<void> ret = device->getCapabilities(
            [&result](V1_0::ErrorStatus error, const V1_0::Capabilities& capabilities) {
                // Time taken to convert capabilities is trivial
                result = std::make_pair(convertToV1_3(error), convertToV1_3(capabilities));
            });
    if (!ret.isOk()) {
        LOG(ERROR) << "getCapabilities failure: " << ret.description();
        return kFailure;
    }
    return result;
}

static std::pair<V1_3::ErrorStatus, hardware::hidl_vec<V1_2::Extension>>
getSupportedExtensionsFunction(V1_2::IDevice* device) {
    CHECK(device != nullptr);
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_INITIALIZATION, "getSupportedExtensions");
    const std::pair<V1_3::ErrorStatus, hardware::hidl_vec<V1_2::Extension>> kFailure = {
            V1_3::ErrorStatus::GENERAL_FAILURE, {}};
    std::pair<V1_3::ErrorStatus, hardware::hidl_vec<V1_2::Extension>> result = kFailure;
    const hardware::Return<void> ret = device->getSupportedExtensions(
            [&result](V1_0::ErrorStatus error,
                      const hardware::hidl_vec<V1_2::Extension>& extensions) {
                result = std::make_pair(convertToV1_3(error), extensions);
            });
    if (!ret.isOk()) {
        LOG(ERROR) << "getSupportedExtensions failure: " << ret.description();
        return kFailure;
    }
    return result;
}

static std::pair<V1_3::ErrorStatus, hardware::hidl_vec<V1_2::Extension>>
getSupportedExtensionsFunction(V1_0::IDevice* device) {
    CHECK(device != nullptr);
    return {V1_3::ErrorStatus::NONE, {/* No extensions. */}};
}

static int32_t getTypeFunction(V1_2::IDevice* device) {
    CHECK(device != nullptr);
    constexpr int32_t kFailure = -1;
    int32_t result = kFailure;
    const hardware::Return<void> ret =
            device->getType([&result](V1_0::ErrorStatus error, V1_2::DeviceType deviceType) {
                if (error == V1_0::ErrorStatus::NONE) {
                    result = static_cast<int32_t>(deviceType);
                }
            });
    if (!ret.isOk()) {
        LOG(ERROR) << "getType failure: " << ret.description();
        return kFailure;
    }
    return result;
}

static int32_t getTypeFunction(V1_0::IDevice* device) {
    CHECK(device != nullptr);
    return ANEURALNETWORKS_DEVICE_UNKNOWN;
}

static std::pair<V1_3::ErrorStatus, hardware::hidl_string> getVersionStringFunction(
        V1_2::IDevice* device) {
    CHECK(device != nullptr);
    const std::pair<V1_3::ErrorStatus, hardware::hidl_string> kFailure = {
            V1_3::ErrorStatus::GENERAL_FAILURE, ""};
    std::pair<V1_3::ErrorStatus, hardware::hidl_string> result = kFailure;
    const hardware::Return<void> ret = device->getVersionString(
            [&result](V1_0::ErrorStatus error, const hardware::hidl_string& version) {
                result = std::make_pair(convertToV1_3(error), version);
            });
    if (!ret.isOk()) {
        LOG(ERROR) << "getVersion failure: " << ret.description();
        return kFailure;
    }
    return result;
}

static std::pair<V1_3::ErrorStatus, hardware::hidl_string> getVersionStringFunction(
        V1_0::IDevice* device) {
    CHECK(device != nullptr);
    return {V1_3::ErrorStatus::NONE, "UNKNOWN"};
}

static std::tuple<V1_3::ErrorStatus, uint32_t, uint32_t> getNumberOfCacheFilesNeededFunction(
        V1_2::IDevice* device) {
    CHECK(device != nullptr);
    constexpr std::tuple<V1_3::ErrorStatus, uint32_t, uint32_t> kFailure = {
            V1_3::ErrorStatus::GENERAL_FAILURE, 0, 0};
    std::tuple<V1_3::ErrorStatus, uint32_t, uint32_t> result = kFailure;
    const hardware::Return<void> ret = device->getNumberOfCacheFilesNeeded(
            [&result](V1_0::ErrorStatus error, uint32_t numModelCache, uint32_t numDataCache) {
                result = {convertToV1_3(error), numModelCache, numDataCache};
            });
    if (!ret.isOk()) {
        LOG(ERROR) << "getNumberOfCacheFilesNeeded failure: " << ret.description();
        return kFailure;
    }
    return result;
}

static std::tuple<V1_3::ErrorStatus, uint32_t, uint32_t> getNumberOfCacheFilesNeededFunction(
        V1_0::IDevice* device) {
    CHECK(device != nullptr);
    return {V1_3::ErrorStatus::NONE, 0, 0};
}

struct InitialData {
    V1_3::Capabilities capabilities;
    hardware::hidl_vec<V1_2::Extension> supportedExtensions;
    int32_t type;
    hardware::hidl_string versionString;
    std::pair<uint32_t, uint32_t> numberOfCacheFilesNeeded;
};

template <typename Device>
static std::optional<InitialData> initializeFunction(Device* device) {
    CHECK(device != nullptr);

    auto [capabilitiesStatus, capabilities] = getCapabilitiesFunction(device);
    if (capabilitiesStatus != V1_3::ErrorStatus::NONE) {
        LOG(ERROR) << "IDevice::getCapabilities* returned the error "
                   << toString(capabilitiesStatus);
        return std::nullopt;
    }
    VLOG(MANAGER) << "Capab " << toString(capabilities);

    auto [versionStatus, versionString] = getVersionStringFunction(device);
    if (versionStatus != V1_3::ErrorStatus::NONE) {
        LOG(ERROR) << "IDevice::getVersionString returned the error " << toString(versionStatus);
        return std::nullopt;
    }
    VLOG(MANAGER) << "Version " << versionString;

    const int32_t type = getTypeFunction(device);
    if (type == -1) {
        LOG(ERROR) << "IDevice::getType returned an error";
        return std::nullopt;
    }

    auto [extensionsStatus, supportedExtensions] = getSupportedExtensionsFunction(device);
    if (extensionsStatus != V1_3::ErrorStatus::NONE) {
        LOG(ERROR) << "IDevice::getSupportedExtensions returned the error "
                   << toString(extensionsStatus);
        return std::nullopt;
    }

    const auto [cacheFilesStatus, numModelCacheFiles, numDataCacheFiles] =
            getNumberOfCacheFilesNeededFunction(device);
    if (cacheFilesStatus != V1_3::ErrorStatus::NONE) {
        LOG(ERROR) << "IDevice::getNumberOfCacheFilesNeeded returned the error "
                   << toString(cacheFilesStatus);
        return std::nullopt;
    }

    // The following limit is enforced by VTS
    constexpr uint32_t maxNumCacheFiles =
            static_cast<uint32_t>(V1_2::Constant::MAX_NUMBER_OF_CACHE_FILES);
    if (numModelCacheFiles > maxNumCacheFiles || numDataCacheFiles > maxNumCacheFiles) {
        LOG(ERROR)
                << "IDevice::getNumberOfCacheFilesNeeded returned invalid number of cache files: "
                   "numModelCacheFiles = "
                << numModelCacheFiles << ", numDataCacheFiles = " << numDataCacheFiles
                << ", maxNumCacheFiles = " << maxNumCacheFiles;
        return std::nullopt;
    }

    return InitialData{
            /*.capabilities=*/std::move(capabilities),
            /*.supportedExtensions=*/std::move(supportedExtensions),
            /*.type=*/type,
            /*.versionString=*/std::move(versionString),
            /*.numberOfCacheFilesNeeded=*/{numModelCacheFiles, numDataCacheFiles},
    };
}

template <typename Core>
std::optional<InitialData> initialize(const Core& core) {
    // version 1.3 HAL
    if (const auto device = core.template getDevice<V1_3::IDevice>()) {
        return initializeFunction(device.get());
    }

    // version 1.2 HAL
    if (const auto device = core.template getDevice<V1_2::IDevice>()) {
        return initializeFunction(device.get());
    }

    // version 1.1 HAL
    if (const auto device = core.template getDevice<V1_1::IDevice>()) {
        return initializeFunction(device.get());
    }

    // version 1.0 HAL
    if (const auto device = core.template getDevice<V1_0::IDevice>()) {
        return initializeFunction(device.get());
    }

    // No device available
    LOG(ERROR) << "Device not available!";
    return std::nullopt;
}

std::shared_ptr<VersionedIDevice> VersionedIDevice::create(std::string serviceName,
                                                           const HalDeviceFactory& makeDevice) {
    CHECK(makeDevice != nullptr)
            << "VersionedIDevice::create passed invalid device factory object.";

    // get handle to IDevice object
    sp<V1_0::IDevice> device = makeDevice(/*blocking=*/true);
    if (device == nullptr) {
        VLOG(DRIVER) << "VersionedIDevice::create got a null IDevice for " << serviceName;
        return nullptr;
    }

    auto core = Core::create(std::move(device));
    if (!core.has_value()) {
        LOG(ERROR) << "VersionedIDevice::create failed to create Core.";
        return nullptr;
    }

    auto initialData = initialize(*core);
    if (!initialData.has_value()) {
        LOG(ERROR) << "VersionedIDevice::create failed to initialize.";
        return nullptr;
    }

    auto [capabilities, supportedExtensions, type, versionString, numberOfCacheFilesNeeded] =
            std::move(*initialData);
    return std::make_shared<VersionedIDevice>(
            uncheckedConvert(capabilities), uncheckedConvert(supportedExtensions), type,
            std::move(versionString), numberOfCacheFilesNeeded, std::move(serviceName), makeDevice,
            std::move(core.value()));
}

VersionedIDevice::VersionedIDevice(Capabilities capabilities,
                                   std::vector<Extension> supportedExtensions, int32_t type,
                                   std::string versionString,
                                   std::pair<uint32_t, uint32_t> numberOfCacheFilesNeeded,
                                   std::string serviceName, const HalDeviceFactory& makeDevice,
                                   Core core)
    : kCapabilities(std::move(capabilities)),
      kSupportedExtensions(std::move(supportedExtensions)),
      kType(type),
      kVersionString(std::move(versionString)),
      kNumberOfCacheFilesNeeded(numberOfCacheFilesNeeded),
      kServiceName(std::move(serviceName)),
      kMakeDevice(makeDevice),
      mCore(std::move(core)) {}

std::optional<VersionedIDevice::Core> VersionedIDevice::Core::create(sp<V1_0::IDevice> device) {
    CHECK(device != nullptr) << "VersionedIDevice::Core::create passed invalid device object.";

    // create death handler object
    sp<IDeviceDeathHandler> deathHandler = new IDeviceDeathHandler();

    // linkToDeath registers a callback that will be invoked on service death to
    // proactively handle service crashes. If the linkToDeath call fails,
    // asynchronous calls are susceptible to hangs if the service crashes before
    // providing the response.
    const hardware::Return<bool> ret = device->linkToDeath(deathHandler, 0);
    if (!ret.isOk()) {
        LOG(ERROR) << "VersionedIDevice::Core::create failed to register a death recipient for the "
                      "IDevice object because of failure: "
                   << ret.description();
        return {};
    }
    if (ret != true) {
        LOG(ERROR) << "VersionedIDevice::Core::create failed to register a death recipient for the "
                      "IDevice object.";
        return {};
    }

    // return a valid Core object
    return Core(std::move(device), std::move(deathHandler));
}

// HIDL guarantees all V1_1 interfaces inherit from their corresponding V1_0 interfaces.
VersionedIDevice::Core::Core(sp<V1_0::IDevice> device, sp<IDeviceDeathHandler> deathHandler)
    : mDeviceV1_0(std::move(device)),
      mDeviceV1_1(V1_1::IDevice::castFrom(mDeviceV1_0).withDefault(nullptr)),
      mDeviceV1_2(V1_2::IDevice::castFrom(mDeviceV1_0).withDefault(nullptr)),
      mDeviceV1_3(V1_3::IDevice::castFrom(mDeviceV1_0).withDefault(nullptr)),
      mDeathHandler(std::move(deathHandler)) {}

VersionedIDevice::Core::~Core() {
    if (mDeathHandler != nullptr) {
        CHECK(mDeviceV1_0 != nullptr);
        // It is safe to ignore any errors resulting from this unlinkToDeath call
        // because the VersionedIDevice::Core object is already being destroyed and
        // its underlying IDevice object is no longer being used by the NN runtime.
        mDeviceV1_0->unlinkToDeath(mDeathHandler).isOk();
    }
}

VersionedIDevice::Core::Core(Core&& other) noexcept
    : mDeviceV1_0(std::move(other.mDeviceV1_0)),
      mDeviceV1_1(std::move(other.mDeviceV1_1)),
      mDeviceV1_2(std::move(other.mDeviceV1_2)),
      mDeviceV1_3(std::move(other.mDeviceV1_3)),
      mDeathHandler(std::move(other.mDeathHandler)) {
    other.mDeathHandler = nullptr;
}

VersionedIDevice::Core& VersionedIDevice::Core::operator=(Core&& other) noexcept {
    if (this != &other) {
        mDeviceV1_0 = std::move(other.mDeviceV1_0);
        mDeviceV1_1 = std::move(other.mDeviceV1_1);
        mDeviceV1_2 = std::move(other.mDeviceV1_2);
        mDeviceV1_3 = std::move(other.mDeviceV1_3);
        mDeathHandler = std::move(other.mDeathHandler);
        other.mDeathHandler = nullptr;
    }
    return *this;
}

template <typename T_IDevice>
std::pair<sp<T_IDevice>, sp<IDeviceDeathHandler>> VersionedIDevice::Core::getDeviceAndDeathHandler()
        const {
    return {getDevice<T_IDevice>(), mDeathHandler};
}

template <typename T_Return, typename T_IDevice, typename T_Callback>
hardware::Return<T_Return> callProtected(
        const char* context,
        const std::function<hardware::Return<T_Return>(const sp<T_IDevice>&)>& fn,
        const sp<T_IDevice>& device, const sp<T_Callback>& callback,
        const sp<IDeviceDeathHandler>& deathHandler) {
    const auto scoped = deathHandler->protectCallback(callback);
    hardware::Return<T_Return> ret = fn(device);
    // Suppose there was a transport error.  We have the following cases:
    // 1. Either not due to a dead device, or due to a device that was
    //    already dead at the time of the call to protectCallback().  In
    //    this case, the callback was never signalled.
    // 2. Due to a device that died after the call to protectCallback() but
    //    before fn() completed.  In this case, the callback was (or will
    //    be) signalled by the deathHandler.
    // Furthermore, what if there was no transport error, but the ErrorStatus is
    // other than NONE?  We'll conservatively signal the callback anyway, just in
    // case the driver was sloppy and failed to do so.
    if (!ret.isOk() || ret != T_Return::NONE) {
        // What if the deathHandler has signalled or will signal the callback?
        // This is fine -- we're permitted to signal multiple times; and we're
        // sending the same signal that the deathHandler does.
        //
        // What if the driver signalled the callback?  Then this signal is
        // ignored.

        if (ret.isOk()) {
            LOG(ERROR) << context << " returned " << toString(static_cast<T_Return>(ret));
        } else {
            LOG(ERROR) << context << " failure: " << ret.description();
        }
        sendFailureMessage(callback.get());
    }
    callback->wait();
    return ret;
}
template <typename T_Return, typename T_IDevice>
hardware::Return<T_Return> callProtected(
        const char*, const std::function<hardware::Return<T_Return>(const sp<T_IDevice>&)>& fn,
        const sp<T_IDevice>& device, const std::nullptr_t&, const sp<IDeviceDeathHandler>&) {
    return fn(device);
}

template <typename T_Return, typename T_IDevice, typename T_Callback>
hardware::Return<T_Return> VersionedIDevice::recoverable(
        const char* context,
        const std::function<hardware::Return<T_Return>(const sp<T_IDevice>&)>& fn,
        const T_Callback& callback) const EXCLUDES(mMutex) {
    CHECK_EQ(callback == nullptr, (std::is_same_v<T_Callback, std::nullptr_t>));

    sp<T_IDevice> device;
    sp<IDeviceDeathHandler> deathHandler;
    std::tie(device, deathHandler) = getDeviceAndDeathHandler<T_IDevice>();

    hardware::Return<T_Return> ret = callProtected(context, fn, device, callback, deathHandler);

    if (ret.isDeadObject()) {
        {
            std::unique_lock lock(mMutex);
            // It's possible that another device has already done the recovery.
            // It's harmless but wasteful for us to do so in this case.
            auto pingReturn = mCore.getDevice<T_IDevice>()->ping();
            if (pingReturn.isDeadObject()) {
                VLOG(DRIVER) << "VersionedIDevice::recoverable(" << context << ") -- Recovering "
                             << kServiceName;
                sp<V1_0::IDevice> recoveredDevice = kMakeDevice(/*blocking=*/false);
                if (recoveredDevice == nullptr) {
                    VLOG(DRIVER) << "VersionedIDevice::recoverable got a null IDEVICE for "
                                 << kServiceName;
                    return ret;
                }

                auto core = Core::create(std::move(recoveredDevice));
                if (!core.has_value()) {
                    LOG(ERROR) << "VersionedIDevice::recoverable failed to create Core.";
                    return ret;
                }

                mCore = std::move(core.value());
            } else {
                VLOG(DRIVER) << "VersionedIDevice::recoverable(" << context
                             << ") -- Someone else recovered " << kServiceName;
                // Might still have a transport error, which we need to check
                // before pingReturn goes out of scope.
                (void)pingReturn.isOk();
            }
            std::tie(device, deathHandler) = mCore.getDeviceAndDeathHandler<T_IDevice>();
        }
        ret = callProtected(context, fn, device, callback, deathHandler);
        // It's possible that the device died again, but we're only going to
        // attempt recovery once per call to recoverable().
    }
    return ret;
}

int VersionedIDevice::wait() const {
    std::unique_lock lock(mMutex);
    // It's possible that another device has already done the recovery.
    // It's harmless but wasteful for us to do so in this case.
    auto pingReturn = mCore.getDevice<V1_0::IDevice>()->ping();
    if (pingReturn.isDeadObject()) {
        VLOG(DRIVER) << "VersionedIDevice::wait -- Recovering " << kServiceName;
        sp<V1_0::IDevice> recoveredDevice = kMakeDevice(/*blocking=*/true);
        if (recoveredDevice == nullptr) {
            LOG(ERROR) << "VersionedIDevice::wait got a null IDevice for " << kServiceName;
            return ANEURALNETWORKS_OP_FAILED;
        }

        auto core = Core::create(std::move(recoveredDevice));
        if (!core.has_value()) {
            LOG(ERROR) << "VersionedIDevice::wait failed to create Core.";
            return ANEURALNETWORKS_OP_FAILED;
        }

        mCore = std::move(core.value());
    } else if (!pingReturn.isOk()) {
        LOG(ERROR) << "VersionedIDevice::wait failed -- IDevice::ping returned "
                   << pingReturn.description();
        return ANEURALNETWORKS_OP_FAILED;
    }

    return ANEURALNETWORKS_NO_ERROR;
}

const Capabilities& VersionedIDevice::getCapabilities() const {
    return kCapabilities;
}

const std::vector<Extension>& VersionedIDevice::getSupportedExtensions() const {
    return kSupportedExtensions;
}

std::pair<ErrorStatus, std::vector<bool>> VersionedIDevice::getSupportedOperations(
        const MetaModel& metaModel) const {
    const std::pair<ErrorStatus, std::vector<bool>> kFailure = {ErrorStatus::GENERAL_FAILURE, {}};
    std::pair<ErrorStatus, std::vector<bool>> result;

    const Model& model = metaModel.getModel();

    auto noneSupported = [&model] {
        std::vector<bool> supported(model.main.operations.size(), false);
        return std::make_pair(ErrorStatus::NONE, std::move(supported));
    };

    auto remappedResult =
            [&model](const std::pair<ErrorStatus, std::vector<bool>>& result,
                     const MetaModel::Mapper& slicedModelOperationIndexToModelOperationIndex) {
                const ErrorStatus status = result.first;
                const std::vector<bool>& supported = result.second;
                std::vector<bool> remappedSupported(model.main.operations.size(), false);
                for (size_t i = 0; i < supported.size(); ++i) {
                    if (supported[i]) {
                        remappedSupported[slicedModelOperationIndexToModelOperationIndex(i)] = true;
                    }
                }
                return std::make_pair(status, std::move(remappedSupported));
            };

    // version 1.3 HAL
    const V1_3::Model model13 = convertToV1_3(model);
    if (getDevice<V1_3::IDevice>() != nullptr) {
        NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_COMPILATION, "getSupportedOperations_1_3");
        hardware::Return<void> ret = recoverable<void, V1_3::IDevice>(
                __FUNCTION__, [&model13, &result](const sp<V1_3::IDevice>& device) {
                    return device->getSupportedOperations_1_3(
                            model13, [&result](V1_3::ErrorStatus error,
                                               const hardware::hidl_vec<bool>& supported) {
                                result = std::make_pair(uncheckedConvert(error), supported);
                            });
                });
        if (!ret.isOk()) {
            LOG(ERROR) << "getSupportedOperations_1_3 failure: " << ret.description();
            return kFailure;
        }
        return result;
    }

    // version 1.2 HAL
    if (getDevice<V1_2::IDevice>() != nullptr) {
        const bool compliant = compliantWithV1_2(model13);
        V1_2::Model model12;
        MetaModel::Mapper slicedModelOperationIndexToModelOperationIndex;
        if (compliant) {
            model12 = convertToV1_2(model13);
        } else {
            const auto slice12 = metaModel.getSliceV1_2();
            if (!slice12.has_value()) {
                return noneSupported();
            }
            std::tie(model12, slicedModelOperationIndexToModelOperationIndex) = *slice12;
        }
        NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_COMPILATION, "getSupportedOperations_1_2");
        hardware::Return<void> ret = recoverable<void, V1_2::IDevice>(
                __FUNCTION__, [&model12, &result](const sp<V1_2::IDevice>& device) {
                    return device->getSupportedOperations_1_2(
                            model12, [&result](V1_0::ErrorStatus error,
                                               const hardware::hidl_vec<bool>& supported) {
                                result = std::make_pair(uncheckedConvert(error), supported);
                            });
                });
        if (!ret.isOk()) {
            LOG(ERROR) << "getSupportedOperations_1_2 failure: " << ret.description();
            return kFailure;
        }
        if (!compliant) {
            return remappedResult(result, slicedModelOperationIndexToModelOperationIndex);
        }
        return result;
    }

    // version 1.1 HAL
    if (getDevice<V1_1::IDevice>() != nullptr) {
        const bool compliant = compliantWithV1_1(model13);
        V1_1::Model model11;
        MetaModel::Mapper slicedModelOperationIndexToModelOperationIndex;
        if (compliant) {
            model11 = convertToV1_1(model13);
        } else {
            const auto slice11 = metaModel.getSliceV1_1();
            if (!slice11.has_value()) {
                return noneSupported();
            }
            std::tie(model11, slicedModelOperationIndexToModelOperationIndex) = *slice11;
        }
        NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_COMPILATION, "getSupportedOperations_1_1");
        hardware::Return<void> ret = recoverable<void, V1_1::IDevice>(
                __FUNCTION__, [&model11, &result](const sp<V1_1::IDevice>& device) {
                    return device->getSupportedOperations_1_1(
                            model11, [&result](V1_0::ErrorStatus error,
                                               const hardware::hidl_vec<bool>& supported) {
                                result = std::make_pair(uncheckedConvert(error), supported);
                            });
                });
        if (!ret.isOk()) {
            LOG(ERROR) << "getSupportedOperations_1_1 failure: " << ret.description();
            return kFailure;
        }
        if (!compliant) {
            return remappedResult(result, slicedModelOperationIndexToModelOperationIndex);
        }
        return result;
    }

    // version 1.0 HAL
    if (getDevice<V1_0::IDevice>() != nullptr) {
        const bool compliant = compliantWithV1_0(model13);
        V1_0::Model model10;
        MetaModel::Mapper slicedModelOperationIndexToModelOperationIndex;
        if (compliant) {
            model10 = convertToV1_0(model13);
        } else {
            const auto slice10 = metaModel.getSliceV1_0();
            if (!slice10.has_value()) {
                return noneSupported();
            }
            std::tie(model10, slicedModelOperationIndexToModelOperationIndex) = *slice10;
        }
        NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_COMPILATION, "getSupportedOperations");
        hardware::Return<void> ret = recoverable<void, V1_0::IDevice>(
                __FUNCTION__, [&model10, &result](const sp<V1_0::IDevice>& device) {
                    return device->getSupportedOperations(
                            model10, [&result](V1_0::ErrorStatus error,
                                               const hardware::hidl_vec<bool>& supported) {
                                result = std::make_pair(uncheckedConvert(error), supported);
                            });
                });
        if (!ret.isOk()) {
            LOG(ERROR) << "getSupportedOperations failure: " << ret.description();
            return kFailure;
        }
        if (!compliant) {
            return remappedResult(result, slicedModelOperationIndexToModelOperationIndex);
        }
        return result;
    }

    // No device available
    LOG(ERROR) << "Device not available!";
    return kFailure;
}

// Opens cache file by filename and sets the handle to the opened fd. Returns false on fail. The
// handle is expected to come in as empty, and is only set to a fd when the function returns true.
// The file descriptor is always opened with both read and write permission.
static bool createCacheHandle(const std::string& cache, bool createIfNotExist,
                              hardware::hidl_handle* handle) {
    CHECK(handle->getNativeHandle() == nullptr);
    int fd = open(cache.c_str(), createIfNotExist ? (O_RDWR | O_CREAT) : O_RDWR, S_IRUSR | S_IWUSR);
    NN_RET_CHECK_GE(fd, 0);
    native_handle_t* cacheNativeHandle = native_handle_create(1, 0);
    if (cacheNativeHandle == nullptr) {
        close(fd);
        return false;
    }
    cacheNativeHandle->data[0] = fd;
    handle->setTo(cacheNativeHandle, /*shouldOwn=*/true);
    return true;
}

// Opens a list of cache files and returns the handle vector. Returns empty vector on fail.
// The file descriptors are always opened with both read and write permission.
static hardware::hidl_vec<hardware::hidl_handle> createCacheHandleVec(
        uint32_t numCacheFiles, const std::string& baseFileName, bool createIfNotExist) {
    CHECK(numCacheFiles <= static_cast<uint32_t>(V1_2::Constant::MAX_NUMBER_OF_CACHE_FILES));
    hardware::hidl_vec<hardware::hidl_handle> handles(numCacheFiles);
    for (uint32_t i = 0; i < numCacheFiles; i++) {
        std::string filename = baseFileName + std::to_string(i);
        VLOG(COMPILATION) << "Cache " << i << ": " << filename;
        if (!createCacheHandle(filename, createIfNotExist, &handles[i])) {
            return hardware::hidl_vec<hardware::hidl_handle>();
        }
    }
    return handles;
}

// Maps token to cache file names and sets the handle vectors to the opened fds. Returns false on
// fail and leaves the vectors empty. Each vector is expected to come in as empty.
static bool getCacheHandles(const std::string& cacheDir, const CacheToken& token,
                            const std::pair<uint32_t, uint32_t>& numCacheFiles,
                            bool createIfNotExist,
                            hardware::hidl_vec<hardware::hidl_handle>* modelCache,
                            hardware::hidl_vec<hardware::hidl_handle>* dataCache) {
    // The filename includes ANEURALNETWORKS_BYTE_SIZE_OF_CACHE_TOKEN * 2 characters for token,
    // and 1 character for model/data cache identifier.
    std::string filename(ANEURALNETWORKS_BYTE_SIZE_OF_CACHE_TOKEN * 2 + 1, '0');
    for (uint32_t i = 0; i < ANEURALNETWORKS_BYTE_SIZE_OF_CACHE_TOKEN; i++) {
        filename[i * 2] = 'A' + (token[i] & 0x0F);
        filename[i * 2 + 1] = 'A' + (token[i] >> 4);
    }
    CHECK(cacheDir.empty() || cacheDir.back() == '/');
    std::string cacheFileName = cacheDir + filename;

    const uint32_t cacheTypeIdentifierIndex =
            cacheDir.size() + ANEURALNETWORKS_BYTE_SIZE_OF_CACHE_TOKEN * 2;
    cacheFileName[cacheTypeIdentifierIndex] = '1';
    *modelCache = createCacheHandleVec(numCacheFiles.first, cacheFileName, createIfNotExist);
    if (modelCache->size() != numCacheFiles.first) {
        return false;
    }
    cacheFileName[cacheTypeIdentifierIndex] = '2';
    *dataCache = createCacheHandleVec(numCacheFiles.second, cacheFileName, createIfNotExist);
    if (dataCache->size() != numCacheFiles.second) {
        modelCache->resize(0);
        return false;
    }
    return true;
}

static std::pair<int, std::shared_ptr<VersionedIPreparedModel>> prepareModelFailure(
        ErrorStatus status = ErrorStatus::GENERAL_FAILURE) {
    return {convertErrorStatusToResultCode(status), nullptr};
}

static std::pair<int, std::shared_ptr<VersionedIPreparedModel>> prepareModelResult(
        const PreparedModelCallback& callback, const char* prepareName,
        const std::string& serviceName) {
    callback.wait();
    if (callback.isDeadObject()) {
        LOG(ERROR) << prepareName << " on " << serviceName
                   << " failed because the PreparedModel object is dead";
        return {ANEURALNETWORKS_DEAD_OBJECT, nullptr};
    }
    const ErrorStatus status = callback.getStatus();
    const sp<V1_0::IPreparedModel> preparedModel = callback.getPreparedModel();

    if (status != ErrorStatus::NONE) {
        LOG(ERROR) << prepareName << " on " << serviceName << " failed: "
                   << "prepareReturnStatus=" << status;
        return prepareModelFailure(status);
    }
    if (preparedModel == nullptr) {
        LOG(ERROR) << prepareName << " on " << serviceName << " failed: preparedModel is nullptr";
        return prepareModelFailure();
    }

    return makeVersionedIPreparedModel(preparedModel);
}

std::pair<int, std::shared_ptr<VersionedIPreparedModel>> VersionedIDevice::prepareModelInternal(
        const Model& model, ExecutionPreference preference, Priority priority,
        const std::optional<Deadline>& deadline, const std::string& cacheDir,
        const std::optional<CacheToken>& maybeToken) const {
    // Note that some work within VersionedIDevice will be subtracted from the IPC layer
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_COMPILATION, "prepareModel");
    const std::pair<int, std::shared_ptr<VersionedIPreparedModel>> kDeadObject = {
            ANEURALNETWORKS_DEAD_OBJECT, nullptr};

    // Get cache files if they exist, otherwise create them.
    hardware::hidl_vec<hardware::hidl_handle> modelCache, dataCache;
    if (!maybeToken.has_value() ||
        !getCacheHandles(cacheDir, *maybeToken, kNumberOfCacheFilesNeeded,
                         /*createIfNotExist=*/true, &modelCache, &dataCache)) {
        modelCache.resize(0);
        dataCache.resize(0);
    }

    // Get the token if it exists, otherwise get a null token.
    static const CacheToken kNullToken{};
    const CacheToken token = maybeToken.value_or(kNullToken);

    const V1_3::Model model13 = convertToV1_3(model);
    const sp<PreparedModelCallback> callback = new PreparedModelCallback();

    // If 1.3 device, try preparing model
    if (getDevice<V1_3::IDevice>() != nullptr) {
        const auto otp = makeTimePoint(deadline);
        const hardware::Return<V1_3::ErrorStatus> ret =
                recoverable<V1_3::ErrorStatus, V1_3::IDevice>(
                        __FUNCTION__,
                        [&model13, preference, priority, &otp, &modelCache, &dataCache, &token,
                         &callback](const sp<V1_3::IDevice>& device) {
                            return device->prepareModel_1_3(
                                    model13, convertToV1_1(preference), convertToV1_3(priority),
                                    convertToV1_3(otp), modelCache, dataCache, token, callback);
                        },
                        callback);
        if (ret.isDeadObject()) {
            LOG(ERROR) << "prepareModel_1_3 failure: " << ret.description();
            return kDeadObject;
        }
        if (!ret.isOk()) {
            LOG(ERROR) << "prepareModel_1_3 failure: " << ret.description();
            return prepareModelFailure();
        }
        const ErrorStatus status = uncheckedConvert(ret);
        if (status != ErrorStatus::NONE) {
            LOG(ERROR) << "prepareModel_1_3 returned " << status;
            return prepareModelFailure(status);
        }
        return prepareModelResult(*callback, "prepareModel_1_3", kServiceName);
    }

    // If 1.2 device, try preparing model (requires conversion)
    if (getDevice<V1_2::IDevice>() != nullptr) {
        bool compliant = false;
        V1_2::Model model12;
        {
            // Attribute time spent in model inspection and conversion to
            // Runtime, as the time may be substantial (0.03ms for mobilenet,
            // but could be larger for other models).
            NNTRACE_FULL_SUBTRACT(NNTRACE_LAYER_RUNTIME, NNTRACE_PHASE_COMPILATION,
                                  "VersionedIDevice::prepareModel_1_2");
            compliant = compliantWithV1_2(model13);
            if (compliant) {
                model12 = convertToV1_2(model13);  // copy is elided
            }
        }
        if (compliant) {
            const hardware::Return<V1_0::ErrorStatus> ret =
                    recoverable<V1_0::ErrorStatus, V1_2::IDevice>(
                            __FUNCTION__,
                            [&model12, &preference, &modelCache, &dataCache, &token,
                             &callback](const sp<V1_2::IDevice>& device) {
                                return device->prepareModel_1_2(model12, convertToV1_1(preference),
                                                                modelCache, dataCache, token,
                                                                callback);
                            },
                            callback);
            if (ret.isDeadObject()) {
                LOG(ERROR) << "prepareModel_1_2 failure: " << ret.description();
                return kDeadObject;
            }
            if (!ret.isOk()) {
                LOG(ERROR) << "prepareModel_1_2 failure: " << ret.description();
                return prepareModelFailure();
            }
            const ErrorStatus status = uncheckedConvert(ret);
            if (status != ErrorStatus::NONE) {
                LOG(ERROR) << "prepareModel_1_2 returned " << status;
                return prepareModelFailure(status);
            }
            return prepareModelResult(*callback, "prepareModel_1_2", kServiceName);
        }

        LOG(ERROR) << "Could not handle prepareModel_1_2!";
        return prepareModelFailure();
    }

    // If 1.1 device, try preparing model (requires conversion)
    if (getDevice<V1_1::IDevice>() != nullptr) {
        bool compliant = false;
        V1_1::Model model11;
        {
            // Attribute time spent in model inspection and conversion to
            // Runtime, as the time may be substantial (0.03ms for mobilenet,
            // but could be larger for other models).
            NNTRACE_FULL_SUBTRACT(NNTRACE_LAYER_RUNTIME, NNTRACE_PHASE_COMPILATION,
                                  "VersionedIDevice::prepareModel_1_1");
            compliant = compliantWithV1_1(model13);
            if (compliant) {
                model11 = convertToV1_1(model13);  // copy is elided
            }
        }
        if (compliant) {
            const hardware::Return<V1_0::ErrorStatus> ret =
                    recoverable<V1_0::ErrorStatus, V1_1::IDevice>(
                            __FUNCTION__,
                            [&model11, &preference, &callback](const sp<V1_1::IDevice>& device) {
                                return device->prepareModel_1_1(model11, convertToV1_1(preference),
                                                                callback);
                            },
                            callback);
            if (ret.isDeadObject()) {
                LOG(ERROR) << "prepareModel_1_1 failure: " << ret.description();
                return kDeadObject;
            }
            if (!ret.isOk()) {
                LOG(ERROR) << "prepareModel_1_1 failure: " << ret.description();
                return prepareModelFailure();
            }
            const ErrorStatus status = uncheckedConvert(ret);
            if (status != ErrorStatus::NONE) {
                LOG(ERROR) << "prepareModel_1_1 returned " << status;
                return prepareModelFailure(status);
            }
            return prepareModelResult(*callback, "prepareModel_1_1", kServiceName);
        }

        LOG(ERROR) << "Could not handle prepareModel_1_1!";
        return prepareModelFailure();
    }

    // If 1.0 device, try preparing model (requires conversion)
    if (getDevice<V1_0::IDevice>() != nullptr) {
        bool compliant = false;
        V1_0::Model model10;
        {
            // Attribute time spent in model inspection and conversion to
            // Runtime, as the time may be substantial (0.03ms for mobilenet,
            // but could be larger for other models).
            NNTRACE_FULL_SUBTRACT(NNTRACE_LAYER_RUNTIME, NNTRACE_PHASE_COMPILATION,
                                  "VersionedIDevice::prepareModel");
            compliant = compliantWithV1_0(model13);
            if (compliant) {
                model10 = convertToV1_0(model13);  // copy is elided
            }
        }
        if (compliant) {
            const hardware::Return<V1_0::ErrorStatus> ret =
                    recoverable<V1_0::ErrorStatus, V1_0::IDevice>(
                            __FUNCTION__,
                            [&model10, &callback](const sp<V1_0::IDevice>& device) {
                                return device->prepareModel(model10, callback);
                            },
                            callback);
            if (ret.isDeadObject()) {
                LOG(ERROR) << "prepareModel failure: " << ret.description();
                return kDeadObject;
            }
            if (!ret.isOk()) {
                LOG(ERROR) << "prepareModel failure: " << ret.description();
                return prepareModelFailure();
            }
            const ErrorStatus status = uncheckedConvert(ret);
            if (status != ErrorStatus::NONE) {
                LOG(ERROR) << "prepareModel returned " << status;
                return prepareModelFailure(status);
            }
            return prepareModelResult(*callback, "prepareModel", kServiceName);
        }

        LOG(ERROR) << "Could not handle prepareModel!";
        return prepareModelFailure();
    }

    // Return error because there is no valid device
    LOG(ERROR) << "prepareModel called with no device";
    return prepareModelFailure();
}

std::pair<int, std::shared_ptr<VersionedIPreparedModel>>
VersionedIDevice::prepareModelFromCacheInternal(const std::optional<Deadline>& deadline,
                                                const std::string& cacheDir,
                                                const CacheToken& token) const {
    // Note that some work within VersionedIDevice will be subtracted from the IPC layer
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_COMPILATION, "prepareModelFromCache");
    VLOG(COMPILATION) << "prepareModelFromCache";
    const std::pair<int, std::shared_ptr<VersionedIPreparedModel>> kDeadObject = {
            ANEURALNETWORKS_DEAD_OBJECT, nullptr};

    // Get cache files if they exist, otherwise return from the function early.
    hardware::hidl_vec<hardware::hidl_handle> modelCache, dataCache;
    if (!getCacheHandles(cacheDir, token, kNumberOfCacheFilesNeeded,
                         /*createIfNotExist=*/false, &modelCache, &dataCache)) {
        return prepareModelFailure();
    }

    // version 1.3 HAL
    if (getDevice<V1_3::IDevice>() != nullptr) {
        const auto otp = makeTimePoint(deadline);
        const sp<PreparedModelCallback> callback = new PreparedModelCallback();
        const hardware::Return<V1_3::ErrorStatus> ret =
                recoverable<V1_3::ErrorStatus, V1_3::IDevice>(
                        __FUNCTION__,
                        [&otp, &modelCache, &dataCache, &token,
                         &callback](const sp<V1_3::IDevice>& device) {
                            return device->prepareModelFromCache_1_3(convertToV1_3(otp), modelCache,
                                                                     dataCache, token, callback);
                        },
                        callback);
        if (ret.isDeadObject()) {
            LOG(ERROR) << "prepareModelFromCache_1_3 failure: " << ret.description();
            return kDeadObject;
        }
        if (!ret.isOk()) {
            LOG(ERROR) << "prepareModelFromCache_1_3 failure: " << ret.description();
            return prepareModelFailure();
        }
        const ErrorStatus status = uncheckedConvert(ret);
        if (status != ErrorStatus::NONE) {
            LOG(ERROR) << "prepareModelFromCache_1_3 returned " << status;
            return prepareModelFailure(status);
        }
        return prepareModelResult(*callback, "prepareModelFromCache_1_3", kServiceName);
    }

    // version 1.2 HAL
    if (getDevice<V1_2::IDevice>() != nullptr) {
        const sp<PreparedModelCallback> callback = new PreparedModelCallback();
        const hardware::Return<V1_0::ErrorStatus> ret =
                recoverable<V1_0::ErrorStatus, V1_2::IDevice>(
                        __FUNCTION__,
                        [&modelCache, &dataCache, &token,
                         &callback](const sp<V1_2::IDevice>& device) {
                            return device->prepareModelFromCache(modelCache, dataCache, token,
                                                                 callback);
                        },
                        callback);
        if (ret.isDeadObject()) {
            LOG(ERROR) << "prepareModelFromCache failure: " << ret.description();
            return kDeadObject;
        }
        if (!ret.isOk()) {
            LOG(ERROR) << "prepareModelFromCache failure: " << ret.description();
            return prepareModelFailure();
        }
        const ErrorStatus status = uncheckedConvert(ret);
        if (status != ErrorStatus::NONE) {
            LOG(ERROR) << "prepareModelFromCache returned " << status;
            return prepareModelFailure(status);
        }
        return prepareModelResult(*callback, "prepareModelFromCache", kServiceName);
    }

    // version too low
    if (getDevice<V1_0::IDevice>() != nullptr) {
        LOG(ERROR) << "prepareModelFromCache called on V1_1 or V1_0 device";
        return prepareModelFailure();
    }

    // No device available
    LOG(ERROR) << "prepareModelFromCache called with no device";
    return prepareModelFailure();
}

std::pair<int, std::shared_ptr<VersionedIPreparedModel>> VersionedIDevice::prepareModel(
        const ModelFactory& makeModel, ExecutionPreference preference, Priority priority,
        const std::optional<Deadline>& deadline, const std::string& cacheDir,
        const std::optional<CacheToken>& maybeToken) const {
    // Attempt to compile from cache if token is present.
    if (maybeToken.has_value()) {
        const auto [n, preparedModel] =
                prepareModelFromCacheInternal(deadline, cacheDir, *maybeToken);
        if (n == ANEURALNETWORKS_NO_ERROR) {
            return {n, preparedModel};
        }
    }

    // Fallback to full compilation (possibly with token) if
    // prepareModelFromCache could not be used or failed.
    const Model model = makeModel();
    return prepareModelInternal(model, preference, priority, deadline, cacheDir, maybeToken);
}

int64_t VersionedIDevice::getFeatureLevel() const {
    constexpr int64_t kFailure = -1;

    if (getDevice<V1_3::IDevice>() != nullptr) {
        return __ANDROID_API_R__;
    } else if (getDevice<V1_2::IDevice>() != nullptr) {
        return __ANDROID_API_Q__;
    } else if (getDevice<V1_1::IDevice>() != nullptr) {
        return __ANDROID_API_P__;
    } else if (getDevice<V1_0::IDevice>() != nullptr) {
        return __ANDROID_API_O_MR1__;
    } else {
        LOG(ERROR) << "Device not available!";
        return kFailure;
    }
}

int32_t VersionedIDevice::getType() const {
    return kType;
}

const std::string& VersionedIDevice::getVersionString() const {
    return kVersionString;
}

std::pair<uint32_t, uint32_t> VersionedIDevice::getNumberOfCacheFilesNeeded() const {
    return kNumberOfCacheFilesNeeded;
}

const std::string& VersionedIDevice::getName() const {
    return kServiceName;
}

std::tuple<V1_3::ErrorStatus, sp<V1_3::IBuffer>, uint32_t> VersionedIDevice::allocate(
        const V1_3::BufferDesc& desc,
        const std::vector<std::shared_ptr<VersionedIPreparedModel>>& versionedPreparedModels,
        const std::vector<BufferRole>& inputRoles,
        const std::vector<BufferRole>& outputRoles) const {
    const auto kFailure = std::make_tuple<V1_3::ErrorStatus, sp<V1_3::IBuffer>, uint32_t>(
            V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, 0);

    // version 1.3 HAL
    if (getDevice<V1_3::IDevice>() != nullptr) {
        hardware::hidl_vec<sp<V1_3::IPreparedModel>> preparedModels(versionedPreparedModels.size());
        std::transform(versionedPreparedModels.begin(), versionedPreparedModels.end(),
                       preparedModels.begin(),
                       [](const auto& preparedModel) { return preparedModel->getV1_3(); });

        std::tuple<V1_3::ErrorStatus, sp<V1_3::IBuffer>, int32_t> result;
        const hardware::Return<void> ret = recoverable<void, V1_3::IDevice>(
                __FUNCTION__, [&](const sp<V1_3::IDevice>& device) {
                    return device->allocate(
                            desc, preparedModels, convertToV1_3(inputRoles),
                            convertToV1_3(outputRoles),
                            [&result](V1_3::ErrorStatus error, const sp<V1_3::IBuffer>& buffer,
                                      uint32_t token) {
                                result = {error, buffer, token};
                            });
                });
        if (!ret.isOk()) {
            LOG(ERROR) << "allocate failure: " << ret.description();
            return kFailure;
        }
        return result;
    }

    // version too low or no device available
    LOG(ERROR) << "Could not handle allocate";
    return kFailure;
}

}  // namespace nn
}  // namespace android
