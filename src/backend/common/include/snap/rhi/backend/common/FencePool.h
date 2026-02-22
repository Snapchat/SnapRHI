#pragma once

#include "snap/rhi/Fence.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/common/NonCopyable.h"
#include <functional>
#include <memory>
#include <mutex>
#include <stack>
#include <vector>

namespace snap::rhi::backend::common {
class Device;

/**
 * @class FencePool
 * @brief Manages a reusable collection of synchronization fences to minimize allocation overhead.
 *
 * The FencePool handles the allocation, recycling, and destruction of Fence objects
 * (e.g., VkFence or MTLSharedEvent). It is designed to reduce the driver overhead associated
 * with constantly creating and destroying synchronization primitives per frame.
 *
 * @warning **CRITICAL LIFECYCLE CONSTRAINT**
 * Unlike standard smart-pointer pools, this class **does not** use custom deleters to
 * automatically return fences to the pool when they go out of scope.
 * - **Reason:** Fences may be retained internally by the Device
 * beyond the scope of the C++ wrapper. Using a custom deleter attached to a standard
 * shared_ptr would risk premature release or invalid memory access if the pool
 * wrapper is destroyed before the GPU is finished.
 *
 */
class FencePool final : public snap::rhi::common::NonCopyable {
public:
    using FenceCreateFunc = std::function<std::shared_ptr<snap::rhi::Fence>(const snap::rhi::FenceCreateInfo&)>;

public:
    explicit FencePool(FenceCreateFunc fenceCreateFunc, const ValidationLayer& validationLayer);
    ~FencePool();

    std::shared_ptr<Fence> acquireFence();
    void clear();

private:
    FenceCreateFunc fenceCreateFunc;
    const ValidationLayer& validationLayer;

    std::mutex accessMutex;
    std::vector<std::shared_ptr<snap::rhi::Fence>> fences;
};
} // namespace snap::rhi::backend::common
