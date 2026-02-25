#pragma once

#include "snap/rhi/Texture.hpp"
#include "snap/rhi/TextureViewCreateInfo.h"

#include "snap/rhi/backend/vulkan/vulkan.h"
#include <memory>

namespace snap::rhi::backend::vulkan {
class Device;
class ImageViewCache;
class ImageWithMemory;

class Texture final : public snap::rhi::Texture {
public:
    explicit Texture(Device* vkDevice, const snap::rhi::TextureViewCreateInfo& info);
    explicit Texture(Device* vkDevice, const snap::rhi::TextureCreateInfo& info, void* imageCreateInfoNext = nullptr);
    explicit Texture(Device* vkDevice, const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop);
    explicit Texture(Device* vkDevice,
                     const snap::rhi::TextureCreateInfo& info,
                     const std::shared_ptr<ImageViewCache>& imageWithMemory);
    explicit Texture(Device* vkDevice,
                     const snap::rhi::TextureCreateInfo& info,
                     VkImage externalImage,
                     bool ownsImage,
                     VkImageLayout defaultLayout);
    ~Texture() override;

    void setDebugLabel(std::string_view label) override;

    VkImageView getTexture() const {
        return texture;
    }

    VkImage getImage() const {
        return image;
    }

    const std::shared_ptr<ImageViewCache>& getImageViewCache() const {
        return imageViewCache;
    }

    const std::shared_ptr<ImageWithMemory>& getImageWithMemory() const;

    bool isLayoutUpdatedToDefault() const;
    bool shouldUpdateLayoutToDefault() const;
    VkImageLayout getDefaultImageLayout() const;
    VkImageLayout getInitialImageLayout() const;

private:
    VkDevice vkDevice = VK_NULL_HANDLE;

    std::shared_ptr<ImageViewCache> imageViewCache = nullptr;
    VkImageView texture = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
};
} // namespace snap::rhi::backend::vulkan
