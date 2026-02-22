#pragma once

#include "snap/rhi/Fence.hpp"
#include "snap/rhi/FenceCreateInfo.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class Fence final : public snap::rhi::Fence {
public:
    explicit Fence(Device* device, const snap::rhi::FenceCreateInfo& info);
    ~Fence() final;

    void setDebugLabel(std::string_view label) override;

    std::unique_ptr<snap::rhi::PlatformSyncHandle> exportPlatformSyncHandle() override;

    snap::rhi::FenceStatus getStatus(uint64_t generationID) final;
    void waitForComplete() final;
    void waitForScheduled() override;
    void reset() final;

    VkFence getFence() const {
        return fence;
    }

private:
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    VkDevice vkDevice = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
};
} // namespace snap::rhi::backend::vulkan
