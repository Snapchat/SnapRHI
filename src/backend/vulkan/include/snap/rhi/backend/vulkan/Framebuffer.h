#pragma once

#include "snap/rhi/Framebuffer.hpp"
#include "snap/rhi/FramebufferCreateInfo.h"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;

class Framebuffer final : public snap::rhi::Framebuffer {
public:
    explicit Framebuffer(Device* device, const snap::rhi::FramebufferCreateInfo& info);
    ~Framebuffer() final;

    void setDebugLabel(std::string_view label) override;

    VkFramebuffer getFramebuffer() const {
        return framebuffer;
    }

private:
    VkDevice vkDevice = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
};
} // namespace snap::rhi::backend::vulkan
