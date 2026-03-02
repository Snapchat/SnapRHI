#include "snap/rhi/backend/vulkan/BlitCommandEncoder.h"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/Buffer.h"
#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"

namespace {

} // unnamed namespace

namespace snap::rhi::backend::vulkan {
BlitCommandEncoder::BlitCommandEncoder(snap::rhi::backend::vulkan::Device* vkDevice,
                                       snap::rhi::backend::vulkan::CommandBuffer* commandBuffer)
    : BlitCommandEncoderBase(vkDevice, commandBuffer) {}

BlitCommandEncoder::~BlitCommandEncoder() {}

void BlitCommandEncoder::beginEncoding() {
    BlitCommandEncoderBase::onBeginEncoding();
}

void BlitCommandEncoder::copyBuffer(snap::rhi::Buffer* srcBuffer,
                                    snap::rhi::Buffer* dstBuffer,
                                    std::span<const BufferCopy> info) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);
    VkCommandBuffer vkCommandBuffer = vkCmdBuffer->getActiveCommandBuffer();

    auto* vkSrcBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Buffer>(srcBuffer);
    auto* vkDstBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Buffer>(dstBuffer);

    for (const auto& copyInfo : info) {
        const VkBufferCopy copyRegion{
            .srcOffset = copyInfo.srcOffset,
            .dstOffset = copyInfo.dstOffset,
            .size = copyInfo.size,
        };

        vkCmdCopyBuffer(vkCommandBuffer, vkSrcBuffer->getVkBuffer(), vkDstBuffer->getVkBuffer(), 1, &copyRegion);
    }

    {
        resourceResidencySet.track(srcBuffer);
        resourceResidencySet.track(dstBuffer);
    }
}

void BlitCommandEncoder::copyBufferToTexture(snap::rhi::Buffer* srcBuffer,
                                             snap::rhi::Texture* dstTexture,
                                             std::span<const BufferTextureCopy> info) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    VkCommandBuffer vkCommandBuffer = vkCmdBuffer->getActiveCommandBuffer();
    auto& imageLayoutManager = vkCmdBuffer->getImageLayoutManager();

    commandBuffer->tryPreserveInteropTexture(dstTexture);

    auto* vkSrcBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Buffer>(srcBuffer);
    auto* vkDstTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(dstTexture);

    const auto& dstTextureCreateInfo = vkDstTexture->getCreateInfo();

    for (const auto& copyInfo : info) {
        SNAP_RHI_VALIDATE(
            validationLayer,
            !(copyInfo.bufferOffset & 0x3),
            snap::rhi::ReportLevel::Error,
            snap::rhi::ValidationTag::CommandBufferOp,
            "[BlitCommandEncoder::copyBufferToTexture] BlitCommandEncoder bufferOffset must be a multiple of 4.");

        const uint32_t rowSizeInBytes = snap::rhi::bytesPerRow(
            dstTextureCreateInfo.size.width, dstTextureCreateInfo.size.height, dstTextureCreateInfo.format);
        const double bytesPerPixel =
            static_cast<double>(rowSizeInBytes) / static_cast<double>(dstTextureCreateInfo.size.width);
        const uint32_t bufferRowLength =
            copyInfo.bytesPerRow ? static_cast<uint32_t>(copyInfo.bytesPerRow / bytesPerPixel) : 0;

        /**
         * baseArrayLayer + layerCount	=> Image array slices or cube map faces (2D or 2D array images)
         * imageOffset.z + imageExtent.depth =>	Depth within a 3D texture (true 3D volume)
         * */
        const VkBufferImageCopy region{
            .bufferOffset = copyInfo.bufferOffset, // Must be a multiple of 4
            .bufferRowLength = bufferRowLength,
            .bufferImageHeight =
                copyInfo.bytesPerRow ? copyInfo.bytesPerSlice / copyInfo.bytesPerRow : /*Tightly packed*/ 0,
            .imageSubresource =
                {
                    .aspectMask = getImageAspectFlags(dstTextureCreateInfo.format),
                    .mipLevel = copyInfo.textureSubresource.mipLevel,
                    .baseArrayLayer =
                        getBaseArrayLayer(dstTextureCreateInfo.textureType, copyInfo.textureOffset.z),
                    .layerCount =
                        getLayerCount(dstTextureCreateInfo.textureType, copyInfo.textureExtent.depth),
                },
            .imageOffset = {copyInfo.textureOffset.x,
                            copyInfo.textureOffset.y,
                            getImageOffsetZ(dstTextureCreateInfo.textureType, copyInfo.textureOffset.z)},
            .imageExtent = {copyInfo.textureExtent.width,
                            copyInfo.textureExtent.height,
                            getImageExtentDepth(dstTextureCreateInfo.textureType, copyInfo.textureExtent.depth)},
        };

        imageLayoutManager.transferLayout(vkDstTexture,
                                          {.aspectMask = region.imageSubresource.aspectMask,
                                           .baseMipLevel = region.imageSubresource.mipLevel,
                                           .levelCount = 1,
                                           .baseArrayLayer = region.imageSubresource.baseArrayLayer,
                                           .layerCount = region.imageSubresource.layerCount},
                                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                                          VK_ACCESS_TRANSFER_WRITE_BIT,
                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdCopyBufferToImage(vkCommandBuffer,
                               vkSrcBuffer->getVkBuffer(),
                               vkDstTexture->getImage(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &region);
    }

    {
        resourceResidencySet.track(srcBuffer);
        resourceResidencySet.track(dstTexture);
    }
}

void BlitCommandEncoder::copyTextureToBuffer(snap::rhi::Texture* srcTexture,
                                             snap::rhi::Buffer* dstBuffer,
                                             std::span<const BufferTextureCopy> info) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    VkCommandBuffer vkCommandBuffer = vkCmdBuffer->getActiveCommandBuffer();
    auto& imageLayoutManager = vkCmdBuffer->getImageLayoutManager();

    commandBuffer->tryPreserveInteropTexture(srcTexture);

    auto* vkSrcTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(srcTexture);
    auto* vkDstBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Buffer>(dstBuffer);

    const auto& srcTextureCreateInfo = srcTexture->getCreateInfo();

    for (const auto& copyInfo : info) {
        SNAP_RHI_VALIDATE(
            validationLayer,
            !(copyInfo.bufferOffset & 0x3),
            snap::rhi::ReportLevel::Error,
            snap::rhi::ValidationTag::CommandBufferOp,
            "[BlitCommandEncoder::copyTextureToBuffer] BlitCommandEncoder bufferOffset must be a multiple of 4.");

        const uint32_t rowSizeInBytes = snap::rhi::bytesPerRow(
            srcTextureCreateInfo.size.width, srcTextureCreateInfo.size.height, srcTextureCreateInfo.format);
        const double bytesPerPixel =
            static_cast<double>(rowSizeInBytes) / static_cast<double>(srcTextureCreateInfo.size.width);
        const uint32_t bufferRowLength =
            copyInfo.bytesPerRow ? static_cast<uint32_t>(copyInfo.bytesPerRow / bytesPerPixel) : 0;

        /**
         * baseArrayLayer + layerCount	=> Image array slices or cube map faces (2D or 2D array images)
         * imageOffset.z + imageExtent.depth =>	Depth within a 3D texture (true 3D volume)
         * */
        const VkBufferImageCopy region{
            .bufferOffset = copyInfo.bufferOffset, // Must be a multiple of 4
            .bufferRowLength = bufferRowLength,
            .bufferImageHeight =
                copyInfo.bytesPerRow ? copyInfo.bytesPerSlice / copyInfo.bytesPerRow : /*Tightly packed*/ 0,
            .imageSubresource =
                {
                    .aspectMask = getImageAspectFlags(srcTextureCreateInfo.format),
                    .mipLevel = copyInfo.textureSubresource.mipLevel,
                    .baseArrayLayer =
                        getBaseArrayLayer(srcTextureCreateInfo.textureType, copyInfo.textureOffset.z),
                    .layerCount =
                        getLayerCount(srcTextureCreateInfo.textureType, copyInfo.textureExtent.depth),
                },
            .imageOffset = {copyInfo.textureOffset.x,
                            copyInfo.textureOffset.y,
                            getImageOffsetZ(srcTextureCreateInfo.textureType, copyInfo.textureOffset.z)},
            .imageExtent = {copyInfo.textureExtent.width,
                            copyInfo.textureExtent.height,
                            getImageExtentDepth(srcTextureCreateInfo.textureType, copyInfo.textureExtent.depth)},
        };

        imageLayoutManager.transferLayout(vkSrcTexture,
                                          {.aspectMask = region.imageSubresource.aspectMask,
                                           .baseMipLevel = region.imageSubresource.mipLevel,
                                           .levelCount = 1,
                                           .baseArrayLayer = region.imageSubresource.baseArrayLayer,
                                           .layerCount = region.imageSubresource.layerCount},
                                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                                          VK_ACCESS_TRANSFER_READ_BIT,
                                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        vkCmdCopyImageToBuffer(vkCommandBuffer,
                               vkSrcTexture->getImage(),
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               vkDstBuffer->getVkBuffer(),
                               1,
                               &region);
    }

    {
        resourceResidencySet.track(srcTexture);
        resourceResidencySet.track(dstBuffer);
    }
}

void BlitCommandEncoder::copyTexture(snap::rhi::Texture* srcTexture,
                                     snap::rhi::Texture* dstTexture,
                                     std::span<const TextureCopy> info) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    VkCommandBuffer vkCommandBuffer = vkCmdBuffer->getActiveCommandBuffer();
    auto& imageLayoutManager = vkCmdBuffer->getImageLayoutManager();

    commandBuffer->tryPreserveInteropTexture(srcTexture);
    commandBuffer->tryPreserveInteropTexture(dstTexture);

    auto* vkSrcTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(srcTexture);
    auto* vkDstTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(dstTexture);

    const auto& srcTextureCreateInfo = srcTexture->getCreateInfo();
    const auto& dstTextureCreateInfo = vkDstTexture->getCreateInfo();

    for (const auto& copyInfo : info) {
        /**
         * srcOffset.z, dstOffset.z	=> Z offset in 3D texture => VK_IMAGE_TYPE_3D only
         * extent.depth	=> Z size in 3D texture	=> VK_IMAGE_TYPE_3D only
         * baseArrayLayer, layerCount => Layer index/count in 2D array or cube maps	=> VK_IMAGE_TYPE_2D with array/cube
         **/

        // srcImage.imageType == dstImage.imageType <= This is a hard Vulkan rule
        const VkImageCopy copyRegion{
            .srcSubresource =
                {
                    .aspectMask = getImageAspectFlags(srcTextureCreateInfo.format),
                    .mipLevel = copyInfo.srcSubresource.mipLevel,
                    .baseArrayLayer =
                        getBaseArrayLayer(srcTextureCreateInfo.textureType, copyInfo.srcOffset.z),
                    .layerCount = getLayerCount(srcTextureCreateInfo.textureType, copyInfo.extent.depth),
                },
            .srcOffset = {copyInfo.srcOffset.x,
                          copyInfo.srcOffset.y,
                          getImageOffsetZ(srcTextureCreateInfo.textureType, copyInfo.srcOffset.z)},
            .dstSubresource =
                {
                    .aspectMask = getImageAspectFlags(dstTextureCreateInfo.format),
                    .mipLevel = copyInfo.dstSubresource.mipLevel,
                    .baseArrayLayer =
                        getBaseArrayLayer(dstTextureCreateInfo.textureType, copyInfo.dstOffset.z),
                    .layerCount = getLayerCount(dstTextureCreateInfo.textureType, copyInfo.extent.depth),
                },
            .dstOffset = {copyInfo.dstOffset.x,
                          copyInfo.dstOffset.y,
                          getImageOffsetZ(dstTextureCreateInfo.textureType, copyInfo.dstOffset.z)},
            .extent = {copyInfo.extent.width,
                       copyInfo.extent.height,
                       getImageExtentDepth(srcTextureCreateInfo.textureType, copyInfo.extent.depth)},
        };

        imageLayoutManager.transferLayout(vkSrcTexture,
                                          {.aspectMask = copyRegion.srcSubresource.aspectMask,
                                           .baseMipLevel = copyRegion.srcSubresource.mipLevel,
                                           .levelCount = 1,
                                           .baseArrayLayer = copyRegion.srcSubresource.baseArrayLayer,
                                           .layerCount = copyRegion.srcSubresource.layerCount},
                                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                                          VK_ACCESS_TRANSFER_READ_BIT,
                                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        imageLayoutManager.transferLayout(vkDstTexture,
                                          {.aspectMask = copyRegion.dstSubresource.aspectMask,
                                           .baseMipLevel = copyRegion.dstSubresource.mipLevel,
                                           .levelCount = 1,
                                           .baseArrayLayer = copyRegion.dstSubresource.baseArrayLayer,
                                           .layerCount = copyRegion.dstSubresource.layerCount},
                                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                                          VK_ACCESS_TRANSFER_WRITE_BIT,
                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdCopyImage(vkCommandBuffer,
                       vkSrcTexture->getImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       vkDstTexture->getImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &copyRegion);
    }

    {
        resourceResidencySet.track(srcTexture);
        resourceResidencySet.track(dstTexture);
    }
}

void BlitCommandEncoder::generateMipmaps(snap::rhi::Texture* texture) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    VkCommandBuffer vkCommandBuffer = vkCmdBuffer->getActiveCommandBuffer();
    auto& imageLayoutManager = vkCmdBuffer->getImageLayoutManager();

    commandBuffer->tryPreserveInteropTexture(texture);

    const auto& capabilities = device->getCapabilities();
    const auto& createInfo = texture->getCreateInfo();

    auto* vkTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(texture);
    VkImage image = vkTexture->getImage();

    int32_t mipWidth = static_cast<int32_t>(createInfo.size.width);
    int32_t mipHeight = static_cast<int32_t>(createInfo.size.height);
    int32_t mipDepth = static_cast<int32_t>(createInfo.size.depth);

    const VkImageAspectFlags aspectMask = getImageAspectFlags(createInfo.format);

    // If filter is VK_FILTER_LINEAR, then the format features of srcImage must contain
    // VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT
    //
    // If srcImage was created with a depth/stencil format, filter
    // must be VK_FILTER_NEAREST
    VkFilter filter = VK_FILTER_NEAREST;
    if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
        const auto& formatProperties = capabilities.formatProperties[static_cast<uint32_t>(createInfo.format)];
        if ((formatProperties.textureFeatures & snap::rhi::FormatFeatures::SampledFilterLinear) ==
            snap::rhi::FormatFeatures::SampledFilterLinear) {
            filter = VK_FILTER_LINEAR;
        }
    }

    for (uint32_t i = 1; i < createInfo.mipLevels; i++) {
        const int32_t dstWidth = std::max(mipWidth >> 1, 1);
        const int32_t dstHeight = std::max(mipHeight >> 1, 1);
        const int32_t dstDepth = std::max(mipDepth >> 1, 1);

        const VkImageBlit blit{
            .srcSubresource =
                {
                    .aspectMask = aspectMask,
                    .mipLevel = i - 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            .srcOffsets = {{0, 0, 0}, {mipWidth, mipHeight, mipDepth}},
            .dstSubresource =
                {
                    .aspectMask = aspectMask,
                    .mipLevel = i,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            .dstOffsets = {{0, 0, 0}, {dstWidth, dstHeight, dstDepth}},
        };

        imageLayoutManager.transferLayout(vkTexture,
                                          {.aspectMask = blit.srcSubresource.aspectMask,
                                           .baseMipLevel = blit.srcSubresource.mipLevel,
                                           .levelCount = 1,
                                           .baseArrayLayer = blit.srcSubresource.baseArrayLayer,
                                           .layerCount = blit.srcSubresource.layerCount},
                                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                                          VK_ACCESS_TRANSFER_READ_BIT,
                                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        imageLayoutManager.transferLayout(vkTexture,
                                          {.aspectMask = blit.dstSubresource.aspectMask,
                                           .baseMipLevel = blit.dstSubresource.mipLevel,
                                           .levelCount = 1,
                                           .baseArrayLayer = blit.dstSubresource.baseArrayLayer,
                                           .layerCount = blit.dstSubresource.layerCount},
                                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                                          VK_ACCESS_TRANSFER_WRITE_BIT,
                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdBlitImage(vkCommandBuffer,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &blit,
                       filter);

        mipWidth = dstWidth;
        mipHeight = dstHeight;
        mipDepth = dstDepth;
    }

    {
        resourceResidencySet.track(texture);
    }
}

void BlitCommandEncoder::endEncoding() {
    BlitCommandEncoderBase::onEndEncoding();
}
} // namespace snap::rhi::backend::vulkan
