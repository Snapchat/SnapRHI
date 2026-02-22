#pragma once

#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/backend/common/DescriptorSet.h"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"
#include <memory>
#include <utility>
#include <vector>

namespace snap::rhi::backend::vulkan {
class Device;
class CommandBuffer;
class Texture;

class DescriptorSet final : public snap::rhi::backend::common::DescriptorSet<> {
public:
    struct Descriptor {
        snap::rhi::DescriptorType descriptorType = snap::rhi::DescriptorType::Undefined;
        snap::rhi::DeviceChild* resource = nullptr;
    };

    DescriptorSet(snap::rhi::backend::vulkan::Device* device, const snap::rhi::DescriptorSetCreateInfo& info);
    ~DescriptorSet() final = default;

    void setDebugLabel(std::string_view label) override;

    void bindUniformBuffer(const uint32_t binding,
                           snap::rhi::Buffer* buffer,
                           const uint64_t offset,
                           const uint64_t range) final;
    void bindStorageBuffer(const uint32_t binding,
                           snap::rhi::Buffer* buffer,
                           const uint64_t offset,
                           const uint64_t range) final;

    void bindTexture(const uint32_t binding, snap::rhi::Texture* texture) final;
    void bindStorageTexture(const uint32_t binding, snap::rhi::Texture* texture, uint32_t mipLevel) final;

    void bindSampler(const uint32_t binding, snap::rhi::Sampler* sampler) final;
    void updateDescriptorSet(std::span<snap::rhi::Descriptor> descriptorWrites) final;

    VkDescriptorSet getVkDescriptorSet() const {
        return descriptorSet ? *descriptorSet.get() : VK_NULL_HANDLE;
    }

    void bindToCommandBuffer(snap::rhi::backend::vulkan::CommandBuffer* commandBuffer);

private:
    void setResource(const uint32_t binding, snap::rhi::DeviceChild* resource);

    VkDevice vkDevice = VK_NULL_HANDLE;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    std::unique_ptr<VkDescriptorSet, std::function<void(VkDescriptorSet*)>> descriptorSet = nullptr;
    std::vector<std::pair<snap::rhi::backend::vulkan::Texture*, VkImageLayout>> imagesExpectedLayout;
};
} // namespace snap::rhi::backend::vulkan
