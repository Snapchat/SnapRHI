#pragma once

#include "snap/rhi/RenderPass.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class RenderPass final : public snap::rhi::RenderPass {
public:
    explicit RenderPass(snap::rhi::backend::vulkan::Device* device, const snap::rhi::RenderPassCreateInfo& info);
    ~RenderPass() override;

    void setDebugLabel(std::string_view label) override;

    VkRenderPass getRenderPass() const {
        return renderPass;
    }

private:
    VkDevice vkDevice = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
};
} // namespace snap::rhi::backend::vulkan
