#include "snap/rhi/backend/vulkan/Framebuffer.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/RenderPass.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"
#include <ranges>

namespace snap::rhi::backend::vulkan {
Framebuffer::Framebuffer(Device* device, const snap::rhi::FramebufferCreateInfo& info)
    : snap::rhi::Framebuffer(device, info), vkDevice(device->getVkLogicalDevice()) {
    std::vector<VkImageView> attachments;
    attachments.reserve(info.attachments.size());

    const auto* vkRenderPass =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::RenderPass>(info.renderPass);
    const auto& renderPassInfo = vkRenderPass->getCreateInfo();

    for (size_t idx = 0; idx < info.attachments.size(); ++idx) {
        auto* texture = info.attachments[idx];
        auto* vkTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(texture);
        const auto& textureInfo = vkTexture->getCreateInfo();
        const auto& textureViewInfo = vkTexture->getViewCreateInfo();

        SNAP_RHI_VALIDATE(device->getValidationLayer(),
                          idx < renderPassInfo.attachmentCount,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::Framebuffer] attachments count must match render pass attachmentCount");

        const auto& attachmentDesc = renderPassInfo.attachments[idx];

        if (info.layers > 1) {
            // TODO: Support multiview / layered framebuffers.
            // For now, fall back to a single-layer view described by render pass attachmentDesc.
        }

        const snap::rhi::TextureSubresourceRange range{
            .baseMipLevel = textureViewInfo.viewInfo.range.baseMipLevel + attachmentDesc.mipLevel,
            .levelCount = 1,
            .baseArrayLayer = textureViewInfo.viewInfo.range.baseArrayLayer + attachmentDesc.layer,
            .layerCount = 1,
        };

        // Basic safety checks to catch invalid render pass configuration vs the actual texture.
        SNAP_RHI_VALIDATE(device->getValidationLayer(),
                          range.baseMipLevel < textureInfo.mipLevels,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::Framebuffer] attachment mipLevel out of range for texture");

        const uint32_t arraySize =
            textureInfo.textureType == snap::rhi::TextureType::Texture3D ?
                textureInfo.size.depth :
                snap::rhi::backend::vulkan::getArraySize(textureInfo.textureType, textureInfo.size);
        SNAP_RHI_VALIDATE(device->getValidationLayer(),
                          range.baseArrayLayer < arraySize,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::Framebuffer] attachment layer out of range for texture");

        attachments.push_back(resolveImageView(vkTexture, range));
    }

    const VkFramebufferCreateInfo framebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::RenderPass>(info.renderPass)->getRenderPass(),
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = info.width,
        .height = info.height,
        .layers = info.layers,
    };

    VkResult result = vkCreateFramebuffer(vkDevice, &framebufferCreateInfo, nullptr, &framebuffer);
    SNAP_RHI_VALIDATE(device->getValidationLayer(),
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Framebuffer] Cannot create VkFramebuffer");
}

Framebuffer::~Framebuffer() {
    assert(framebuffer != VK_NULL_HANDLE);

    vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
    framebuffer = VK_NULL_HANDLE;
}

void Framebuffer::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_FRAMEBUFFER, reinterpret_cast<uint64_t>(framebuffer), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
