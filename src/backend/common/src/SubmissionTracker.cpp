#include <cassert>
#include <mutex>
#include <snap/rhi/backend/common/CommandBuffer.h>
#include <snap/rhi/backend/common/Device.hpp>
#include <snap/rhi/backend/common/SubmissionTracker.h>
#include <snap/rhi/backend/common/Utils.hpp>

namespace {
constexpr snap::rhi::DeviceContextCreateInfo DCCreateInfo{};
} // unnamed namespace

namespace snap::rhi::backend::common {
SubmissionTracker::SubmissionTracker(Device* device, const DeviceContextCreateFunc& deviceContextCreateFunc)
    : device(device) {
    if (const bool enableBackgroundThread = static_cast<bool>(device->getDeviceCreateInfo().deviceCreateFlags &
                                                              snap::rhi::DeviceCreateFlags::ReclaimAsync);
        enableBackgroundThread && !workerThread.joinable()) {
        workerThread = std::thread([this, threadDeviceContext = deviceContextCreateFunc(DCCreateInfo)]() {
            auto deviceContextGuard = this->device->setCurrent(threadDeviceContext.get());
            while (isRunning) {
                processSubmissions();
            }
        });
    }
}

SubmissionTracker::~SubmissionTracker() {
    isRunning.store(false);
    conditionVariable.notify_one();
    if (workerThread.joinable()) {
        workerThread.join();
    }

    std::lock_guard lock(accessMutex);
    assert(inFlightSubmissions.size() == freeSlots.size());
    inFlightSubmissions.clear();
}

void SubmissionTracker::track(snap::rhi::CommandBuffer* commandBuffers, snap::rhi::Fence* fence) {
    assert(fence != nullptr);
    if (!fence) {
        return;
    }

    const auto& commandBufferSharedRef = smart_cast<snap::rhi::CommandBuffer>(device->resolveResource(commandBuffers));
    const auto& fenceSharedRef = smart_cast<snap::rhi::Fence>(device->resolveResource(fence));
    {
        std::lock_guard lock(accessMutex);
        if (!freeSlots.empty()) {
            const uint32_t slotIdx = freeSlots.top();
            freeSlots.pop();
            inFlightSubmissions[slotIdx] =
                TrackedSubmission{commandBufferSharedRef, fenceSharedRef, fence->getGenerationID()};
        } else {
            inFlightSubmissions.push_back(
                TrackedSubmission{commandBufferSharedRef, fenceSharedRef, fence->getGenerationID()});
        }
    }
    conditionVariable.notify_one();
}

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
void SubmissionTracker::validateResourceLifetimes() const {
    std::lock_guard lock(accessMutex);
    for (const auto& submission : inFlightSubmissions) {
        if (submission.commandBuffer && submission.fence &&
            submission.fence->getStatus(submission.generationID) != snap::rhi::FenceStatus::Completed) {
            auto* commandBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::CommandBuffer>(
                submission.commandBuffer.get());
            commandBuffer->validateResourceLifetimes();
        }
    }
}
#endif

void SubmissionTracker::tryReclaim() {
    std::lock_guard lock(accessMutex);
    for (size_t i = 0; i < inFlightSubmissions.size(); ++i) {
        if (auto& submission = inFlightSubmissions[i];
            submission.fence &&
            submission.fence->getStatus(submission.generationID) == snap::rhi::FenceStatus::Completed) {
            submission = TrackedSubmission{};
            freeSlots.push(static_cast<uint32_t>(i));
        }
    }
}

void SubmissionTracker::reclaim() {
    {
        std::unique_lock lock(accessMutex);
        if (device->isNativeContextAttached()) {
            reclaimUnsafe(lock);
        } else {
            /**
             * Clear all in-flight submissions without waiting, as we cannot
             * make any GPU progress without a valid context.
             */
            for (size_t i = 0; i < inFlightSubmissions.size(); ++i) {
                if (auto& submission = inFlightSubmissions[i]; submission.fence) {
                    submission = {};
                    freeSlots.push(static_cast<uint32_t>(i));
                }
            }
        }
    }

    /**
     * Wait until any background processing is completed
     */
    isProcessingSubmissions.wait(true, std::memory_order_relaxed);
}

void SubmissionTracker::reclaimUnsafe(std::unique_lock<SubmissionTracker::MutexType>& lock) {
    for (size_t i = 0; i < inFlightSubmissions.size(); ++i) {
        if (auto& submission = inFlightSubmissions[i]; submission.fence) {
            TrackedSubmission toWaitSubmission{};
            std::swap(toWaitSubmission, submission);
            freeSlots.push(static_cast<uint32_t>(i));

            lock.unlock();
            if (toWaitSubmission.fence->getStatus(submission.generationID) != snap::rhi::FenceStatus::Completed) {
                toWaitSubmission.fence->waitForComplete();
            }
            toWaitSubmission = {};
            lock.lock();
        }
    }
}

void SubmissionTracker::processSubmissions() {
    std::unique_lock lock(accessMutex);
    conditionVariable.wait(lock,
                           [this] { return inFlightSubmissions.size() != freeSlots.size() || !isRunning.load(); });
    isProcessingSubmissions.test_and_set();
    reclaimUnsafe(lock);
    isProcessingSubmissions.clear(std::memory_order_release);
    isProcessingSubmissions.notify_one();
}
} // namespace snap::rhi::backend::common
