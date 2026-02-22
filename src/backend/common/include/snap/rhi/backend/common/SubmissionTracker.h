#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <span>
#include <stack>
#include <thread>
#include <vector>

#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/DeviceContext.hpp"
#include "snap/rhi/DeviceContextCreateInfo.h"
#include "snap/rhi/Fence.hpp"
#include "snap/rhi/backend/common/Utils.hpp"

namespace snap::rhi::backend::common {
class Device;

/**
 * @brief Manages the lifetime of command buffers and tracks their completion via fences.
 *
 * The SubmissionTracker is responsible for "retiring" command buffers that have been submitted
 * to the GPU. It keeps the CPU-side objects alive (via shared_ptr) until the associated
 * fence is signaled, ensuring that resources are not destroyed while the GPU is still using them.
 */
class SubmissionTracker final {
public:
    using DeviceContextCreateFunc =
        std::function<std::shared_ptr<snap::rhi::DeviceContext>(const snap::rhi::DeviceContextCreateInfo&)>;

public:
    /**
     * @brief Constructs a new Submission Tracker.
     * * @note To enable background processing, you must update this constructor to accept
     * a SubmissionTrackerFlags argument.
     */
    explicit SubmissionTracker(Device* device, const DeviceContextCreateFunc& deviceContextCreateFunc);

    /**
     * @brief Destroys the tracker and ensures all background threads are joined.
     * * @warning Any resources still pending in the queue will be released.
     * You should ensure the GPU is idle before destroying this object.
     */
    ~SubmissionTracker();

    /**
     * @brief Submits a batch of command buffers to be tracked until the fence signals.
     *
     * This increases the reference count of the provided command buffers, ensuring they
     * remain alive on the CPU. Once the provided fence is signaled, the references
     * will be dropped during the next reclaim() cycle.
     *
     * @note Have to be called only after command buffer has been sent to the GPU.
     *
     * @param commandBuffers A span of command buffers involved in the submission.
     * @param fence          The fence associated with this specific submission.
     * The tracker will monitor this fence to determine when resources can be freed.
     */
    void track(snap::rhi::CommandBuffer* commandBuffers, snap::rhi::Fence* fence);

    /**
     * @brief Polls tracked fences and releases resources for completed submissions.
     *
     * Iterates through the queue of in-flight submissions. If a fence is found to be
     * signaled (completed), the associated command buffers are released (ref count decremented).
     */
    void tryReclaim();
    void reclaim();

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    /**
     * @brief Validates that resources used by in-flight command buffers are still alive.
     *
     * This function iterates through any command buffers that have been submitted but
     * not yet completed by the GPU. It verifies that all resources (buffers, textures, etc.)
     * referenced by those command buffers remain valid and resident in memory.
     *
     * * This check is intended to catch resource lifetime bugs where a resource is
     * destroyed on the CPU while the GPU is still actively using it.
     *
     * @note This function is only defined when @SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
     * is true. It involves overhead and should be used strictly for debugging purposes.
     */
    void validateResourceLifetimes() const;
#endif

private:
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    using MutexType = std::recursive_mutex;
    using ConditionVariableType = std::condition_variable_any;
#else
    using MutexType = std::mutex;
    using ConditionVariableType = std::condition_variable;
#endif

    void processSubmissions();
    void reclaimUnsafe(std::unique_lock<MutexType>& lock);

    struct TrackedSubmission {
        std::shared_ptr<snap::rhi::CommandBuffer> commandBuffer = nullptr;
        std::shared_ptr<snap::rhi::Fence> fence = nullptr;
        uint64_t generationID = 0;
    };

    Device* device = nullptr;

    mutable MutexType accessMutex;
    ConditionVariableType conditionVariable;
    std::vector<TrackedSubmission> inFlightSubmissions;
    std::atomic_flag isProcessingSubmissions;
    std::stack<uint32_t> freeSlots;

    std::atomic_bool isRunning{true};
    std::thread workerThread;
};
} // namespace snap::rhi::backend::common
