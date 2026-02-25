#pragma once

#include "snap/rhi/TextureCreateInfo.h"
#include "snap/rhi/TextureInterop.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"
#include <atomic>
#include <cassert>
#include <memory>

namespace snap::rhi::backend::vulkan {
class Device;
class CommandBuffer;

class ImageWithMemory final {
public:
    ImageWithMemory(Device* vkDevice,
                    const snap::rhi::TextureCreateInfo& info,
                    void* imageCreateInfoNext = nullptr,
                    void* imageAllocateInfoNext = nullptr);
    ImageWithMemory(Device* vkDevice, const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop);
    ImageWithMemory(Device* vkDevice,
                    const snap::rhi::TextureCreateInfo& info,
                    VkImage externalImage,
                    bool ownsImage,
                    VkImageLayout defaultLayout);
    ~ImageWithMemory();

    VkImage getImage() const;
    VkDeviceMemory getImageMemory() const;

    bool isLayoutUpdatedToDefault() {
        return isDefaultLayout;
    }

    bool shouldUpdateLayoutToDefault() {
        return !isDefaultLayout.exchange(true);
    }

    VkImageLayout getDefaultImageLayout() const {
        return defaultImageLayout;
    }

    VkImageLayout getInitialImageLayout() const {
        return InitialImageLayout;
    }

private:
    VkDevice device = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    std::shared_ptr<snap::rhi::TextureInterop> textureInterop = nullptr;
    bool ownsImage = true;

    VkImageLayout defaultImageLayout = VK_IMAGE_LAYOUT_GENERAL;
    static constexpr VkImageLayout InitialImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    std::atomic<bool> isDefaultLayout{false};
};
} // namespace snap::rhi::backend::vulkan
