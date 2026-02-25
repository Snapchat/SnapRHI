#include "snap/rhi/backend/vulkan/RenderCommandEncoder.h"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/vulkan/Buffer.h"
#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/ComputePipeline.h"
#include "snap/rhi/backend/vulkan/DescriptorSet.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Framebuffer.h"
#include "snap/rhi/backend/vulkan/PipelineLayout.h"
#include "snap/rhi/backend/vulkan/RenderPass.h"
#include "snap/rhi/backend/vulkan/RenderPipeline.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include <algorithm>
#include <ranges>

namespace {
void prepareAttachmentLayout(snap::rhi::backend::vulkan::ImageLayoutManager& imageLayoutManager,
                             snap::rhi::backend::vulkan::Texture* texture,
                             uint32_t mipLevel,
                             uint32_t baseArrayLayer,
                             uint32_t layerCount) {
    const auto& attachmentTextureCreateInfo = texture->getCreateInfo();
    auto attachmentAspectMask = snap::rhi::backend::vulkan::getImageAspectFlags(attachmentTextureCreateInfo.format);
    const auto isDepthStencil = snap::rhi::getDepthStencilFormatTraits(attachmentTextureCreateInfo.format) !=
                                snap::rhi::DepthStencilFormatTraits::None;

    VkPipelineStageFlags targetStageMask =
        isDepthStencil ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT :
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkAccessFlags targetAccessMask =
        isDepthStencil ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT :
                         VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkImageLayout targetLayout =
        isDepthStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    imageLayoutManager.transferLayout(texture,
                                      {.aspectMask = attachmentAspectMask,
                                       .baseMipLevel = mipLevel,
                                       .levelCount = 1,
                                       .baseArrayLayer = baseArrayLayer,
                                       .layerCount = layerCount},
                                      targetStageMask,
                                      targetAccessMask,
                                      targetLayout);
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
RenderCommandEncoder::RenderCommandEncoder(snap::rhi::backend::vulkan::Device* vkDevice,
                                           snap::rhi::backend::vulkan::CommandBuffer* commandBuffer)
    : RenderCommandEncoderBase(vkDevice, commandBuffer),
      DescriptorSetEncoder(vkDevice, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS),
      apiVersion(vkDevice->getPhysicalDeviceProperties().apiVersion) {}

RenderCommandEncoder::~RenderCommandEncoder() {}

void RenderCommandEncoder::beginEncoding(const RenderPassBeginInfo& renderPassBeginInfo) {
    RenderCommandEncoderBase::onBeginEncoding();
    reset();
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    vkCmdBuffer->beginRenderPass();
    auto& imageLayoutManager = vkCmdBuffer->getImageLayoutManager();

    SNAP_RHI_VALIDATE(validationLayer,
                      renderPassBeginInfo.framebuffer != nullptr,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::RenderCommandEncoderOp,
                      "[Vulkan::RenderCommandEncoder] Framebuffer must be specified for RenderPassBeginInfo");
    SNAP_RHI_VALIDATE(validationLayer,
                      renderPassBeginInfo.renderPass != nullptr,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::RenderCommandEncoderOp,
                      "[Vulkan::RenderCommandEncoder] RenderPass must be specified for RenderPassBeginInfo");

    const auto& framebufferCreateInfo = renderPassBeginInfo.framebuffer->getCreateInfo();
    const auto& renderPassCreateInfo = renderPassBeginInfo.renderPass->getCreateInfo();

    const uint32_t layerCount = framebufferCreateInfo.layers;
    {
        const auto& attachments = framebufferCreateInfo.attachments;
        for (size_t i = 0; i < attachments.size(); ++i) {
            const auto& attachment = attachments[i];
            const auto& attachmentDescription = renderPassCreateInfo.attachments[i];
            if (attachment) {
                resourceResidencySet.track(attachment);

                auto* vkAttachmentTexture =
                    snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(attachment);
                const auto& attachmentCreteInfo = vkAttachmentTexture->getCreateInfo();

                const uint32_t mipLevel = attachmentDescription.mipLevel;

                /**
                 * In Vulkan 2D array and Cube textures are represented as multiple layers,
                 * while 3D textures use only one layer and depth is represented using depth slices.
                 * Thus, when preparing the attachment layout, we need to consider this difference.
                 */
                const uint32_t baseArrayLayer = attachmentCreteInfo.textureType == snap::rhi::TextureType::Texture3D ?
                                                    0 :
                                                    attachmentDescription.layer;

                prepareAttachmentLayout(imageLayoutManager, vkAttachmentTexture, mipLevel, baseArrayLayer, layerCount);
            }
        }

        resourceResidencySet.track(renderPassBeginInfo.framebuffer);
        resourceResidencySet.track(renderPassBeginInfo.renderPass);
        resourceResidencySet.track(framebufferCreateInfo.renderPass);
    }

    VkFramebuffer vkFramebuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Framebuffer>(renderPassBeginInfo.framebuffer)
            ->getFramebuffer();
    VkRenderPass vkRenderPass =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::RenderPass>(renderPassBeginInfo.renderPass)
            ->getRenderPass();
    std::vector<VkClearValue> clearValues;
    clearValues.reserve(renderPassBeginInfo.clearValues.size());
    std::ranges::copy(renderPassBeginInfo.clearValues | std::views::transform([](const auto& clearValue) {
                          VkClearValue vkClearValue{};
                          vkClearValue.color.uint32[0] = clearValue.color.uint32[0];
                          vkClearValue.color.uint32[1] = clearValue.color.uint32[1];
                          vkClearValue.color.uint32[2] = clearValue.color.uint32[2];
                          vkClearValue.color.uint32[3] = clearValue.color.uint32[3];
                          return vkClearValue;
                      }),
                      std::back_inserter(clearValues));

    VkRenderPassBeginInfo vkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = vkRenderPass,
        .framebuffer = vkFramebuffer,
        .renderArea =
            {
                .offset = {0, 0},
                .extent = {framebufferCreateInfo.width, framebufferCreateInfo.height},
            },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
    vkCmdBeginRenderPass(vkCmdBuffer->getActiveCommandBuffer(), &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    isDynamicRenderingUsed = false;
}

void RenderCommandEncoder::beginEncoding(const RenderingInfo& renderingInfo) {
    RenderCommandEncoderBase::onBeginEncoding();
    reset();

    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    vkCmdBuffer->beginRenderPass();
    auto& imageLayoutManager = vkCmdBuffer->getImageLayoutManager();

    uint32_t renderAreaWidth = 0;
    uint32_t renderAreaHeight = 0;

    auto attachmentProcessFunc = [&](const auto& attachment) {
        auto* vkTexture =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(attachment.attachment.texture);
        const auto& textureInfo = vkTexture->getCreateInfo();
        const auto& textureViewInfo = vkTexture->getViewCreateInfo();
        resourceResidencySet.track(vkTexture);

        VkImageView vkImageView = VK_NULL_HANDLE;
        if (renderingInfo.layers > 1) {
            // TODO: Support multiview with array layers
        } else {
            const snap::rhi::TextureSubresourceRange range{
                .baseMipLevel = textureViewInfo.viewInfo.range.baseMipLevel + attachment.attachment.mipLevel,
                .levelCount = 1,
                .baseArrayLayer = textureViewInfo.viewInfo.range.baseArrayLayer + attachment.attachment.layer,
                .layerCount = 1,
            };
            vkImageView = resolveImageView(vkTexture, range);
        }
        auto vkImageLayout =
            snap::rhi::getDepthStencilFormatTraits(textureInfo.format) != snap::rhi::DepthStencilFormatTraits::None ?
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        /**
         * In Vulkan 2D array and Cube textures are represented as multiple layers,
         * while 3D textures use only one layer and depth is represented using depth slices.
         * Thus, when preparing the attachment layout, we need to consider this difference.
         */
        const uint32_t baseArrayLayer =
            textureInfo.textureType == snap::rhi::TextureType::Texture3D ? 0 : attachment.attachment.layer;
        prepareAttachmentLayout(
            imageLayoutManager, vkTexture, attachment.attachment.mipLevel, baseArrayLayer, renderingInfo.layers);

        auto* vkResolveTexture = attachment.resolveAttachment.texture ?
                                     snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(
                                         attachment.resolveAttachment.texture) :
                                     nullptr;
        VkImageView vkResolveImageView = VK_NULL_HANDLE;
        auto vkResolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (vkResolveTexture) {
            const auto& resolveTextureInfo = vkResolveTexture->getCreateInfo();
            resourceResidencySet.track(vkResolveTexture);
            vkResolveImageView = vkResolveTexture->getTexture();
            vkResolveImageLayout = snap::rhi::getDepthStencilFormatTraits(resolveTextureInfo.format) !=
                                           snap::rhi::DepthStencilFormatTraits::None ?
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        renderAreaWidth = std::max(renderAreaWidth, textureInfo.size.width);
        renderAreaHeight = std::max(renderAreaHeight, textureInfo.size.height);

        return VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = vkImageView,
            .imageLayout = vkImageLayout,
            .resolveMode = vkResolveTexture ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE,
            .resolveImageView = vkResolveImageView,
            .resolveImageLayout = vkResolveImageLayout,
            .loadOp = snap::rhi::backend::vulkan::toVkAttachmentLoadOp(attachment.loadOp),
            .storeOp = snap::rhi::backend::vulkan::toVkAttachmentStoreOp(attachment.storeOp),
            .clearValue =
                {
                    .color =
                        {
                            .uint32 =
                                {
                                    attachment.clearValue.color.uint32[0],
                                    attachment.clearValue.color.uint32[1],
                                    attachment.clearValue.color.uint32[2],
                                    attachment.clearValue.color.uint32[3],
                                },
                        },
                },
        };
    };

    std::vector<VkRenderingAttachmentInfo> renderingAttachments;
    renderingAttachments.reserve(renderingInfo.colorAttachments.size() +
                                 (renderingInfo.depthAttachment.attachment.texture ? 1 : 0) +
                                 (renderingInfo.stencilAttachment.attachment.texture ? 1 : 0));
    std::ranges::copy(renderingInfo.colorAttachments | std::views::transform(attachmentProcessFunc),
                      std::back_inserter(renderingAttachments));

    VkRenderingAttachmentInfo* depthAttachment = nullptr;
    if (renderingInfo.depthAttachment.attachment.texture) {
        renderingAttachments.push_back(attachmentProcessFunc(renderingInfo.depthAttachment));
        depthAttachment = &renderingAttachments.back();
    }

    VkRenderingAttachmentInfo* stencilAttachment = nullptr;
    if (renderingInfo.stencilAttachment.attachment.texture) {
        renderingAttachments.push_back(attachmentProcessFunc(renderingInfo.stencilAttachment));
        stencilAttachment = &renderingAttachments.back();
    }

    VkRenderingInfo vkRenderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        /**
         * https://registry.khronos.org/vulkan/specs/latest/man/html/VkRenderingFlagBits.html
         */
        .flags = 0,
        .renderArea =
            {
                .offset = {0, 0},
                .extent = {renderAreaWidth, renderAreaHeight},
            },
        .layerCount = renderingInfo.layers,
        .viewMask = renderingInfo.viewMask,
        .colorAttachmentCount = static_cast<uint32_t>(renderingInfo.colorAttachments.size()),
        .pColorAttachments = renderingAttachments.data(),
        .pDepthAttachment = depthAttachment,
        .pStencilAttachment = stencilAttachment,
    };

    auto beginRenderingFunc = apiVersion < VK_API_VERSION_1_3 ? vkCmdBeginRenderingKHR : vkCmdBeginRendering;
    beginRenderingFunc(vkCmdBuffer->getActiveCommandBuffer(), &vkRenderingInfo);
    isDynamicRenderingUsed = true;
}

void RenderCommandEncoder::setViewport(const snap::rhi::Viewport& viewport) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);
    auto vkCommandBuffer = vkCmdBuffer->getActiveCommandBuffer();

    VkViewport nativeViewport{
        .x = static_cast<float>(viewport.x),
        .y = static_cast<float>(viewport.y),
        .width = static_cast<float>(viewport.width),
        .height = static_cast<float>(viewport.height),
        .minDepth = viewport.znear,
        .maxDepth = viewport.zfar,
    };

    VkRect2D scissorRect{
        .offset = {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)},
        .extent = {viewport.width, viewport.height},
    };

    vkCmdSetViewport(vkCommandBuffer, 0, 1, &nativeViewport);
    vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissorRect);
}

void RenderCommandEncoder::bindRenderPipeline(snap::rhi::RenderPipeline* pipeline) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    VkCommandBuffer vkCommandBuffer = vkCmdBuffer->getActiveCommandBuffer();
    auto* vkPipeline = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::RenderPipeline>(pipeline);
    assignPipelineLayout(vkPipeline->getPipelineLayout(), vkPipeline->getDescriptorSetCount());

    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->getVkPipeline());
    resourceResidencySet.track(vkPipeline);
}

void RenderCommandEncoder::bindDescriptorSet(uint32_t binding,
                                             snap::rhi::DescriptorSet* descriptorSet,
                                             std::span<const uint32_t> dynamicOffsets) {
    assignDescriptorSet(binding, descriptorSet, dynamicOffsets);
}

void RenderCommandEncoder::bindVertexBuffers(const uint32_t firstBinding,
                                             std::span<snap::rhi::Buffer*> buffers,
                                             std::span<uint32_t> offsets) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    std::vector<VkBuffer> vkBuffers;
    vkBuffers.reserve(buffers.size());
    std::ranges::copy(buffers | std::views::transform([&](auto* buffer) {
                          resourceResidencySet.track(buffer);
                          auto* vkBuffer =
                              snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Buffer>(buffer);
                          return vkBuffer->getVkBuffer();
                      }),
                      std::back_inserter(vkBuffers));

    std::vector<VkDeviceSize> vkOffsets;
    vkOffsets.reserve(buffers.size());
    std::ranges::copy(offsets | std::views::transform([](auto offset) { return static_cast<VkDeviceSize>(offset); }),
                      std::back_inserter(vkOffsets));

    vkCmdBindVertexBuffers(vkCmdBuffer->getActiveCommandBuffer(),
                           firstBinding,
                           static_cast<uint32_t>(vkBuffers.size()),
                           vkBuffers.data(),
                           vkOffsets.data());
}

void RenderCommandEncoder::bindVertexBuffer(const uint32_t binding, snap::rhi::Buffer* buffer, uint32_t offset) {
    resourceResidencySet.track(buffer);
    bindVertexBuffers(binding, std::span(&buffer, 1), std::span(&offset, 1));
}

void RenderCommandEncoder::bindIndexBuffer(snap::rhi::Buffer* indexBuffer,
                                           const uint32_t offset,
                                           const IndexType indexType) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    resourceResidencySet.track(indexBuffer);
    vkCmdBindIndexBuffer(
        vkCmdBuffer->getActiveCommandBuffer(),
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Buffer>(indexBuffer)->getVkBuffer(),
        static_cast<VkDeviceSize>(offset),
        snap::rhi::backend::vulkan::toVkIndexType(indexType));
}

void RenderCommandEncoder::draw(const uint32_t vertexCount, const uint32_t firstVertex, const uint32_t instanceCount) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    bindDescriptorSets();
    vkCmdDraw(vkCmdBuffer->getActiveCommandBuffer(), vertexCount, instanceCount, firstVertex, 0);
}

void RenderCommandEncoder::drawIndexed(const uint32_t indexCount,
                                       const uint32_t firstIndex,
                                       const uint32_t instanceCount) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    bindDescriptorSets();
    vkCmdDrawIndexed(vkCmdBuffer->getActiveCommandBuffer(), indexCount, instanceCount, firstIndex, 0, 0);
}

void RenderCommandEncoder::setDepthBias(float depthBiasConstantFactor,
                                        float depthBiasSlopeFactor,
                                        float depthBiasClamp) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);
    vkCmdSetDepthBias(
        vkCmdBuffer->getActiveCommandBuffer(), depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

void RenderCommandEncoder::setStencilReference(const StencilFace face, const uint32_t reference) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);
    vkCmdSetStencilReference(
        vkCmdBuffer->getActiveCommandBuffer(), snap::rhi::backend::vulkan::toVkStencilFaceFlags(face), reference);
}

void RenderCommandEncoder::setBlendConstants(const float r, const float g, const float b, const float a) {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    std::array blendConstants{r, g, b, a};
    vkCmdSetBlendConstants(vkCmdBuffer->getActiveCommandBuffer(), blendConstants.data());
}

void RenderCommandEncoder::invokeCustomCallback(std::function<void()>* callback) {
    (*callback)();
}

void RenderCommandEncoder::reset() {
    DescriptorSetEncoder::reset();
}

void RenderCommandEncoder::endEncoding() {
    auto* vkCmdBuffer =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandBuffer>(commandBuffer);

    auto vkCommandBuffer = vkCmdBuffer->getActiveCommandBuffer();
    if (isDynamicRenderingUsed) {
        auto endRenderingFunc = apiVersion < VK_API_VERSION_1_3 ? vkCmdEndRenderingKHR : vkCmdEndRendering;
        endRenderingFunc(vkCommandBuffer);
    } else {
        vkCmdEndRenderPass(vkCommandBuffer);
    }
    RenderCommandEncoderBase::onEndEncoding();
    reset();
    vkCmdBuffer->endRenderPass();
}
} // namespace snap::rhi::backend::vulkan
