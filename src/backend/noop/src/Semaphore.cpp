#include "snap/rhi/backend/noop/Semaphore.hpp"

#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
Semaphore::Semaphore(snap::rhi::backend::common::Device* device, const snap::rhi::SemaphoreCreateInfo& info)
    : snap::rhi::Semaphore(device, info) {}
} // namespace snap::rhi::backend::noop
