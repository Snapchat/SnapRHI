#pragma once

#include "snap/rhi/backend/common/TextureViewCache.h"
#include "snap/rhi/backend/vulkan/vulkan.h"

#include <memory>

namespace snap::rhi::backend::vulkan {
class ImageWithMemory;
class Device;
/**
 * @brief Per-image cache of VkImageView objects.
 *
 * Image views are expensive to create and are frequently duplicated when creating multiple snap::rhi::Texture objects
 * referencing the same VkImage (e.g., texture views).
 *
 * This cache is intended to be owned by the underlying image resource (ImageWithMemory) and shared via std::shared_ptr
 * among all Texture wrappers referencing that image.
 */
class ImageViewCache final : public common::TextureViewCache<std::shared_ptr<ImageWithMemory>, VkImageView> {
public:
    ImageViewCache(Device* vkDevice,
                   const snap::rhi::TextureCreateInfo& info,
                   void* imageCreateInfoNext = nullptr,
                   void* imageAllocateInfoNext = nullptr);
    ImageViewCache(Device* vkDevice, const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop);
    ImageViewCache(Device* vkDevice,
                   const snap::rhi::TextureCreateInfo& info,
                   VkImage externalImage,
                   bool ownsImage,
                   VkImageLayout defaultLayout);
    ~ImageViewCache() override;

private:
    VkImageView createView(const snap::rhi::TextureViewInfo& key) const override;
};
} // namespace snap::rhi::backend::vulkan
