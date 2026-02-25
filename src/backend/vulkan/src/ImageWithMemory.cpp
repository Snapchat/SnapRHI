#include "snap/rhi/backend/vulkan/ImageWithMemory.h"

#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"

#include "snap/rhi/backend/common/TextureInterop.h"
#include "snap/rhi/backend/vulkan/ImageViewCache.h"
#include "snap/rhi/backend/vulkan/TextureInterop.h"
#include "snap/rhi/common/Throw.h"

namespace {
uint32_t getDepth(const snap::rhi::TextureType textureType, const snap::rhi::Extent3D& extent3D) {
    switch (textureType) {
        case snap::rhi::TextureType::Texture2D:
        case snap::rhi::TextureType::Texture2DArray:
        case snap::rhi::TextureType::TextureCubemap:
            return 1;

        case snap::rhi::TextureType::Texture3D:
            return extent3D.depth;

        default:
            snap::rhi::common::throwException("[getImageType] unsupported image type");
    }
}

VkImageLayout getDefaultImageLayoutFromTextureUsage(const snap::rhi::TextureUsage usage) {
    if (static_cast<bool>(usage & snap::rhi::TextureUsage::Storage)) {
        return VK_IMAGE_LAYOUT_GENERAL;
    }

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::ColorAttachment)) {
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::DepthStencilAttachment)) {
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::Sampled)) {
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::TransferDst)) {
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::TransferSrc)) {
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }

    return VK_IMAGE_LAYOUT_GENERAL;
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
ImageWithMemory::ImageWithMemory(Device* vkDevice,
                                 const snap::rhi::TextureCreateInfo& info,
                                 void* imageCreateInfoNext,
                                 void* memoryAllocateInfoNext)
    : device(vkDevice->getVkLogicalDevice()) {
    const auto& validationLayer = vkDevice->getValidationLayer();

    {
        VkImageCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.pNext = imageCreateInfoNext;
        createInfo.flags = toVkImageCreateFlags(info.textureType);

        if (info.textureType == snap::rhi::TextureType::Texture3D && vkDevice->supportsImageView2DOn3DImage()) {
            createInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
        }

        createInfo.imageType = toVkImageType(info.textureType);
        createInfo.format = toVkFormat(info.format);
        createInfo.extent.width = info.size.width;
        createInfo.extent.height = info.size.height;
        createInfo.extent.depth = getDepth(info.textureType, info.size);
        createInfo.mipLevels = info.mipLevels;
        createInfo.arrayLayers = getArraySize(info.textureType, info.size);
        createInfo.samples = toVkSampleCountFlagBits(info.sampleCount);
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = toVkImageUsageFlags(info.textureUsage);

        /**
         * VK_SHARING_MODE_EXCLUSIVE specifies that access to any range or image subresource of the object will be
         * exclusive to a single queue family at a time
         *
         * SnapRHI always assume that subresource of the object will be exclusive to a single queue family at a
         * time
         * */
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;

        {
            /**
             * SnapRHI will transfer image from initialLayout{VK_IMAGE_LAYOUT_UNDEFINED}
             * into default layout on first image usage
             * */
            defaultImageLayout = getDefaultImageLayoutFromTextureUsage(info.textureUsage);

            createInfo.initialLayout = InitialImageLayout;
        }

        VkResult result = vkCreateImage(device, &createInfo, nullptr, &image);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == VK_SUCCESS,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::Texture] cannot create image");
    }

    {
        VkMemoryRequirements memRequirements{};
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        constexpr VkMemoryPropertyFlags memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = memoryAllocateInfoNext;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vkDevice->chooseMemoryTypeIndex(memRequirements.memoryTypeBits, memoryProperty);

        VkResult result = vkAllocateMemory(device, &allocInfo, nullptr, &memory);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == VK_SUCCESS,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::Image] cannot allocate memory for image");
    }

    vkBindImageMemory(device, image, memory, 0);
}

ImageWithMemory::ImageWithMemory(Device* vkDevice, const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop)
    : device(vkDevice->getVkLogicalDevice()), textureInterop(textureInterop) {
    auto* vkTextureInterop = snap::rhi::backend::common::smart_dynamic_cast<snap::rhi::backend::vulkan::TextureInterop>(
        textureInterop.get());
    image = vkTextureInterop->getVulkanTexture(vkDevice);
    memory = vkTextureInterop->getVulkanTextureMemory(vkDevice);
}

ImageWithMemory::ImageWithMemory(Device* vkDevice,
                                 const snap::rhi::TextureCreateInfo& info,
                                 VkImage externalImage,
                                 bool ownsImage,
                                 VkImageLayout defaultLayout)
    : device(vkDevice->getVkLogicalDevice()), image(externalImage), memory(VK_NULL_HANDLE), ownsImage(ownsImage) {
    isDefaultLayout = true;
    defaultImageLayout = defaultLayout;
}

ImageWithMemory::~ImageWithMemory() {
    if (!textureInterop && ownsImage && device != VK_NULL_HANDLE) {
        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(device, image, nullptr);
        }

        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
        }
    }
}

VkImage ImageWithMemory::getImage() const {
    return image;
}

VkDeviceMemory ImageWithMemory::getImageMemory() const {
    return memory;
}
} // namespace snap::rhi::backend::vulkan
