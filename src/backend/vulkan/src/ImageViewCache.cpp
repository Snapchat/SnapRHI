#include "snap/rhi/backend/vulkan/ImageViewCache.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/ImageWithMemory.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"

#include <ranges>

namespace snap::rhi::backend::vulkan {
ImageViewCache::ImageViewCache(Device* vkDevice,
                               const snap::rhi::TextureCreateInfo& info,
                               void* imageCreateInfoNext,
                               void* imageAllocateInfoNext)
    : common::TextureViewCache<std::shared_ptr<ImageWithMemory>, VkImageView>(
          vkDevice, std::make_shared<ImageWithMemory>(vkDevice, info, imageCreateInfoNext, imageAllocateInfoNext)) {}

ImageViewCache::ImageViewCache(Device* vkDevice, const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop)
    : common::TextureViewCache<std::shared_ptr<ImageWithMemory>, VkImageView>(
          vkDevice, std::make_shared<ImageWithMemory>(vkDevice, textureInterop)) {}

ImageViewCache::ImageViewCache(Device* vkDevice,
                               const snap::rhi::TextureCreateInfo& info,
                               VkImage externalImage,
                               bool ownsImage,
                               VkImageLayout defaultLayout)
    : common::TextureViewCache<std::shared_ptr<ImageWithMemory>, VkImageView>(
          vkDevice, std::make_shared<ImageWithMemory>(vkDevice, info, externalImage, ownsImage, defaultLayout)) {}

ImageViewCache::~ImageViewCache() {
    const auto* vkDevice = common::smart_cast<Device>(device);
    const auto device = vkDevice->getVkLogicalDevice();

    std::lock_guard lock(mutex);
    for (const auto& view : views | std::views::values) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, view, nullptr);
        }
    }
    views.clear();
}

VkImageView ImageViewCache::createView(const snap::rhi::TextureViewInfo& key) const {
    const auto* vkDevice = common::smart_cast<Device>(device);
    const auto device = vkDevice->getVkLogicalDevice();
    const auto image = texture->getImage();

    VkImageViewCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image,
        .viewType = getImageViewType(key.textureType),
        .format = snap::rhi::backend::vulkan::toVkFormat(key.format),
        .components = toVkComponentMapping(key.components, key.format),
        .subresourceRange =
            {
                .aspectMask = getImageAspect(key.format),
                .baseMipLevel = key.range.baseMipLevel,
                .levelCount = key.range.levelCount,
                .baseArrayLayer = key.range.baseArrayLayer,
                .layerCount = key.range.layerCount,
            },
    };

    // For 3D images, Vulkan requires baseArrayLayer==0 and layerCount==1.
    // For 2D-array/cubemap, layerCount is the number of layers/faces exposed by the view.
    if (key.textureType == snap::rhi::TextureType::Texture3D) {
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
    }

    VkImageView view = VK_NULL_HANDLE;
    const VkResult result = vkCreateImageView(device, &createInfo, nullptr, &view);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::ImageViewCache] cannot create image view");

    return view;
}
} // namespace snap::rhi::backend::vulkan
