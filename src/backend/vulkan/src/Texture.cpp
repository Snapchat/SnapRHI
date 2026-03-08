#include "snap/rhi/backend/vulkan/Texture.h"

#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/ImageViewCache.h"
#include "snap/rhi/backend/vulkan/ImageWithMemory.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"
#include "snap/rhi/common/Throw.h"

namespace snap::rhi::backend::vulkan {
Texture::Texture(Device* vkDevice, const TextureViewCreateInfo& viewCreateInfo)
    : snap::rhi::Texture(
          vkDevice,
          common::buildTextureCreateInfoFromView(viewCreateInfo),
          common::buildTextureViewCreateInfo(viewCreateInfo.texture->getViewCreateInfo(), viewCreateInfo)),
      vkDevice(vkDevice->getVkLogicalDevice()),
      imageViewCache(snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(viewInfo.texture)
                         ->imageViewCache) {
    const auto baseInfo = viewInfo.texture->getCreateInfo();
    const auto key = buildTextureViewInfo(baseInfo,
                                          viewCreateInfo.viewInfo.range,
                                          viewCreateInfo.viewInfo.format,
                                          viewCreateInfo.viewInfo.textureType,
                                          viewCreateInfo.viewInfo.components);
    texture = imageViewCache->acquire(key);
    image = imageViewCache->getTexture()->getImage();
}

Texture::Texture(Device* vkDevice, const snap::rhi::TextureCreateInfo& info, void* imageCreateInfoNext)
    : snap::rhi::Texture(vkDevice, info),
      vkDevice(vkDevice->getVkLogicalDevice()),
      imageViewCache(
          std::make_shared<snap::rhi::backend::vulkan::ImageViewCache>(vkDevice, info, imageCreateInfoNext)) {
    const snap::rhi::TextureSubresourceRange range{
        .baseMipLevel = 0,
        .levelCount = info.mipLevels,
        .baseArrayLayer = 0,
        .layerCount = snap::rhi::backend::vulkan::getArraySize(info.textureType, info.size),
    };

    const auto key = buildTextureViewInfo(info, range, info.format, info.textureType, info.components);
    texture = imageViewCache->acquire(key);
    image = imageViewCache->getTexture()->getImage();
}

Texture::Texture(Device* vkDevice, const std::shared_ptr<TextureInterop>& textureInterop)
    : snap::rhi::Texture(vkDevice, textureInterop),
      vkDevice(vkDevice->getVkLogicalDevice()),
      imageViewCache(std::make_shared<snap::rhi::backend::vulkan::ImageViewCache>(vkDevice, textureInterop)) {
    const auto texInfo = textureInterop->getTextureCreateInfo();

    const snap::rhi::TextureSubresourceRange range{
        .baseMipLevel = 0,
        .levelCount = texInfo.mipLevels,
        .baseArrayLayer = 0,
        .layerCount = snap::rhi::backend::vulkan::getArraySize(texInfo.textureType, texInfo.size),
    };

    const auto key = buildTextureViewInfo(texInfo, range, texInfo.format, texInfo.textureType, texInfo.components);
    texture = imageViewCache->acquire(key);
    image = imageViewCache->getTexture()->getImage();
}

Texture::Texture(Device* vkDevice,
                 const snap::rhi::TextureCreateInfo& info,
                 const std::shared_ptr<ImageViewCache>& imageViewCache)
    : snap::rhi::Texture(vkDevice, info), vkDevice(vkDevice->getVkLogicalDevice()), imageViewCache(imageViewCache) {
    const snap::rhi::TextureSubresourceRange range{
        .baseMipLevel = 0,
        .levelCount = info.mipLevels,
        .baseArrayLayer = 0,
        .layerCount = snap::rhi::backend::vulkan::getArraySize(info.textureType, info.size),
    };

    const auto key = buildTextureViewInfo(info, range, info.format, info.textureType, info.components);
    texture = imageViewCache->acquire(key);
    image = imageViewCache->getTexture()->getImage();
}

Texture::Texture(Device* vkDevice,
                 const snap::rhi::TextureCreateInfo& info,
                 VkImage externalImage,
                 bool ownsImage,
                 VkImageLayout defaultLayout)
    : snap::rhi::Texture(vkDevice, info),
      vkDevice(vkDevice->getVkLogicalDevice()),
      imageViewCache(std::make_shared<snap::rhi::backend::vulkan::ImageViewCache>(
          vkDevice, info, externalImage, ownsImage, defaultLayout)) {
    const snap::rhi::TextureSubresourceRange range{
        .baseMipLevel = 0,
        .levelCount = info.mipLevels,
        .baseArrayLayer = 0,
        .layerCount = snap::rhi::backend::vulkan::getArraySize(info.textureType, info.size),
    };

    const auto key = buildTextureViewInfo(info, range, info.format, info.textureType, info.components);
    texture = imageViewCache->acquire(key);
    image = imageViewCache->getTexture()->getImage();
}

Texture::~Texture() {
    imageViewCache.reset();
    texture = VK_NULL_HANDLE;
    image = VK_NULL_HANDLE;
}

const std::shared_ptr<ImageWithMemory>& Texture::getImageWithMemory() const {
    return imageViewCache->getTexture();
}

bool Texture::isLayoutUpdatedToDefault() const {
    return getImageWithMemory()->isLayoutUpdatedToDefault();
}

bool Texture::shouldUpdateLayoutToDefault() const {
    return getImageWithMemory()->shouldUpdateLayoutToDefault();
}

VkImageLayout Texture::getDefaultImageLayout() const {
    return getImageWithMemory()->getDefaultImageLayout();
}

VkImageLayout Texture::getInitialImageLayout() const {
    return getImageWithMemory()->getInitialImageLayout();
}

void Texture::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(image), label.data());
    if (texture != VK_NULL_HANDLE) {
        setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(texture), label.data());
    }
#endif
}
} // namespace snap::rhi::backend::vulkan
