#include "snap/rhi/backend/vulkan/ImageLayoutManager.h"
#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include <snap/rhi/common/Scope.h>

namespace {
constexpr VkPipelineStageFlags DefaultStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
constexpr VkAccessFlags DefaultAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
ImageLayoutManager::ImageLayoutManager(snap::rhi::backend::vulkan::Device* vkDevice,
                                       snap::rhi::backend::vulkan::CommandBuffer* commandBuffer)
    : vkDevice(vkDevice), validationLayer(vkDevice->getValidationLayer()), commandBuffer(commandBuffer) {}

ImageLayoutManager::~ImageLayoutManager() {}

void ImageLayoutManager::transferLayout(snap::rhi::backend::vulkan::Texture* texture,
                                        const VkImageSubresourceRange& range,
                                        const VkPipelineStageFlags targetStageMask,
                                        const VkAccessFlags targetAccessMask,
                                        const VkImageLayout targetLayout) {
    const VkImage image = texture->getImage();

    auto itr = imageToLayout.find(image);
    if (itr == imageToLayout.end()) {
        const VkPipelineStageFlags stageMask = DefaultStageMask;
        const VkAccessFlags accessMask = DefaultAccessMask;
        const VkImageLayout layout = texture->getDefaultImageLayout();

        if (!texture->isLayoutUpdatedToDefault()) {
            auto& resourcesInitInfo = commandBuffer->getResourcesInitInfo();
            resourcesInitInfo.textures.push_back(texture);
        }

        imageToInfo[image] = {.aspectMask = range.aspectMask, .defaultLayout = layout};

        const auto& createInfo = texture->getCreateInfo();
        // Use getArraySize since for 2D textures layers have to be handled separately (e.g. cubemap should return 6 *
        // depth) while 3D texture must only have 1 layer
        const auto& [insertItr, inserted] =
            imageToLayout.insert(std::make_pair(image,
                                                TextureLayout(getArraySize(createInfo.textureType, createInfo.size),
                                                              createInfo.mipLevels,
                                                              stageMask,
                                                              accessMask,
                                                              layout)));
        itr = insertItr;
    }

    auto& textureLayoutInfo = itr->second;
    transferLayout(textureLayoutInfo, image, range, targetStageMask, targetAccessMask, targetLayout);
}

void ImageLayoutManager::transferLayout(TextureLayout& textureLayoutInfo,
                                        const VkImage image,
                                        const VkImageSubresourceRange& range,
                                        const VkPipelineStageFlags targetStageMask,
                                        const VkAccessFlags targetAccessMask,
                                        const VkImageLayout targetLayout) {
    const VkCommandBuffer nativeCommandBuffer = commandBuffer->getSyncCommandBuffer();

    bool canTransferWholeRange = true;
    for (uint32_t i = 0; canTransferWholeRange && i < range.layerCount; ++i) {
        for (uint32_t j = 0; canTransferWholeRange && j < range.levelCount; ++j) {
            auto& layoutInfo = textureLayoutInfo.getLayout(range.baseArrayLayer + i, range.baseMipLevel + j);
            canTransferWholeRange &=
                layoutInfo == textureLayoutInfo.getLayout(range.baseArrayLayer, range.baseMipLevel);
        }
    }

    SNAP_RHI_ON_SCOPE_EXIT {
        for (uint32_t i = 0; i < range.layerCount; ++i) {
            for (uint32_t j = 0; j < range.levelCount; ++j) {
                auto& layoutInfo = textureLayoutInfo.getLayout(range.baseArrayLayer + i, range.baseMipLevel + j);

                layoutInfo.layout = targetLayout;
                layoutInfo.accessMask = targetAccessMask;
                layoutInfo.stageMask = targetStageMask;
            }
        }
    };

    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.dstAccessMask = targetAccessMask;
    imageMemoryBarrier.newLayout = targetLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;

    if (canTransferWholeRange) {
        const auto& oldLayoutInfo = textureLayoutInfo.getLayout(range.baseArrayLayer, range.baseMipLevel);

        imageMemoryBarrier.srcAccessMask = oldLayoutInfo.accessMask;
        imageMemoryBarrier.oldLayout = oldLayoutInfo.layout;
        imageMemoryBarrier.subresourceRange = range;

        vkCmdPipelineBarrier(nativeCommandBuffer,
                             oldLayoutInfo.stageMask,
                             targetStageMask,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &imageMemoryBarrier);
    } else {
        for (uint32_t i = 0; i < range.layerCount; ++i) {
            for (uint32_t j = 0; j < range.levelCount; ++j) {
                auto& layoutInfo = textureLayoutInfo.getLayout(range.baseArrayLayer + i, range.baseMipLevel + j);

                imageMemoryBarrier.srcAccessMask = layoutInfo.accessMask;
                imageMemoryBarrier.oldLayout = layoutInfo.layout;
                imageMemoryBarrier.subresourceRange = {.aspectMask = range.aspectMask,
                                                       .baseMipLevel = range.baseMipLevel + j,
                                                       .levelCount = 1,
                                                       .baseArrayLayer = range.baseArrayLayer + i,
                                                       .layerCount = 1};

                vkCmdPipelineBarrier(nativeCommandBuffer,
                                     layoutInfo.stageMask,
                                     targetStageMask,
                                     0,
                                     0,
                                     nullptr,
                                     0,
                                     nullptr,
                                     1,
                                     &imageMemoryBarrier);
            }
        }
    }
}

void ImageLayoutManager::transferImagesIntoDefaultLayout() {
    const VkPipelineStageFlags targetStageMask = DefaultStageMask;
    const VkAccessFlags targetAccessMask = DefaultAccessMask;

    for (auto& [image, textureLayout] : imageToLayout) {
        const auto& info = imageToInfo[image];

        const VkImageLayout targetLayout = info.defaultLayout;
        VkImageSubresourceRange range = {.aspectMask = info.aspectMask,
                                         .baseMipLevel = 0,
                                         .levelCount = textureLayout.mipLevels,
                                         .baseArrayLayer = 0,
                                         .layerCount = textureLayout.layers};

        transferLayout(textureLayout, image, range, targetStageMask, targetAccessMask, targetLayout);
    }
}
} // namespace snap::rhi::backend::vulkan
