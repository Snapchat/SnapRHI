#pragma once

#include "snap/rhi/DescriptorPool.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"
#include <array>
#include <functional>
#include <memory>

namespace snap::rhi::backend::vulkan {
class Device;
class DescriptorSetLayout;

class DescriptorPool final : public snap::rhi::DescriptorPool {
public:
    DescriptorPool(snap::rhi::backend::vulkan::Device* device, const snap::rhi::DescriptorPoolCreateInfo& info);
    ~DescriptorPool() override;

    std::unique_ptr<VkDescriptorSet, std::function<void(VkDescriptorSet*)>> allocateDescriptorSet(
        snap::rhi::backend::vulkan::DescriptorSetLayout* layout) const;

private:
    VkDevice vkDevice = VK_NULL_HANDLE;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    mutable uint32_t availableSetCount = 0;
    mutable std::array<uint32_t, static_cast<size_t>(snap::rhi::DescriptorType::Count)> availableDescriptorTypeCount{};
};
} // namespace snap::rhi::backend::vulkan
