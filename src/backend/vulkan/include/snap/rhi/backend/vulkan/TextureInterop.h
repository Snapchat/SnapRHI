#pragma once

#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/common/OS.h"

#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/TextureInterop.h"
#include <snap/rhi/backend/vulkan/vulkan.h>

namespace snap::rhi::backend::vulkan {
class TextureInterop : public virtual snap::rhi::backend::common::TextureInterop {
public:
    virtual VkImage getVulkanTexture(snap::rhi::Device* device) = 0;
    virtual VkDeviceMemory getVulkanTextureMemory(snap::rhi::Device* device) = 0;
};
} // namespace snap::rhi::backend::vulkan
