#pragma once

#include "snap/rhi/Semaphore.hpp"

#include <memory>

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class Semaphore final : public snap::rhi::Semaphore {
public:
    Semaphore(snap::rhi::backend::common::Device* device, const snap::rhi::SemaphoreCreateInfo& info);
    ~Semaphore() override = default;
};
} // namespace snap::rhi::backend::noop
