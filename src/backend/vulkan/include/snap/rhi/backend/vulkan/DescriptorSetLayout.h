#pragma once

#include "snap/rhi/DescriptorSetLayout.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class DescriptorSetLayout final : public snap::rhi::DescriptorSetLayout {
public:
    explicit DescriptorSetLayout(Device* device, const snap::rhi::DescriptorSetLayoutCreateInfo& info);
    ~DescriptorSetLayout() override;

    void setDebugLabel(std::string_view label) override;

    VkDescriptorSetLayout getVkDescriptorSetLayout() const {
        return setLayout;
    }

private:
    VkDevice vkDevice = VK_NULL_HANDLE;
    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
};
} // namespace snap::rhi::backend::vulkan
