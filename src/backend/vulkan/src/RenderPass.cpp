#include "snap/rhi/backend/vulkan/RenderPass.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"
#include <algorithm>
#include <ranges>

namespace {
VkRenderPass createRenderPass_1_0(const snap::rhi::backend::common::ValidationLayer& validationLayer,
                                  VkDevice vkDevice,
                                  const snap::rhi::RenderPassCreateInfo& info) {
    std::vector<VkAttachmentDescription> vkAttachments(info.attachmentCount);
    std::ranges::copy(
        std::span<const snap::rhi::AttachmentDescription>(info.attachments.data(), info.attachmentCount) |
            std::views::transform([](const auto& attachmentDescription) {
                VkImageLayout layout = snap::rhi::getDepthStencilFormatTraits(attachmentDescription.format) !=
                                               snap::rhi::DepthStencilFormatTraits::None ?
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                return VkAttachmentDescription{
                    /**
                     * https://registry.khronos.org/vulkan/specs/latest/man/html/VkAttachmentDescriptionFlagBits.html
                     * */
                    .flags = 0,
                    .format = snap::rhi::backend::vulkan::toVkFormat(attachmentDescription.format),
                    .samples = snap::rhi::backend::vulkan::toVkSampleCountFlagBits(attachmentDescription.samples),
                    .loadOp = snap::rhi::backend::vulkan::toVkAttachmentLoadOp(attachmentDescription.loadOp),
                    .storeOp = snap::rhi::backend::vulkan::toVkAttachmentStoreOp(attachmentDescription.storeOp),
                    .stencilLoadOp =
                        snap::rhi::backend::vulkan::toVkAttachmentLoadOp(attachmentDescription.stencilLoadOp),
                    .stencilStoreOp =
                        snap::rhi::backend::vulkan::toVkAttachmentStoreOp(attachmentDescription.stencilStoreOp),
                    .initialLayout = layout,
                    .finalLayout = layout};
            }),
        vkAttachments.begin());

    std::vector<VkAttachmentReference> attachmentReferences;
    std::vector<VkSubpassDescription> vkSubpasses(info.subpassCount);
    std::ranges::copy(
        std::span<const snap::rhi::SubpassDescription>(info.subpasses.data(), info.subpassCount) |
            std::views::transform([&attachmentReferences](const auto& attachmentDescription) {
                size_t colorStartIndex = attachmentReferences.size();
                std::ranges::copy(
                    std::span<const snap::rhi::AttachmentReference>(attachmentDescription.colorAttachments.data(),
                                                                    attachmentDescription.colorAttachmentCount) |
                        std::views::transform([](const auto& attachmentReference) {
                            return VkAttachmentReference{.attachment = attachmentReference.attachment,
                                                         .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
                        }),
                    std::back_inserter(attachmentReferences));

                size_t resolveStartIndex = attachmentReferences.size();
                std::ranges::copy(
                    std::span<const snap::rhi::AttachmentReference>(attachmentDescription.resolveAttachments.data(),
                                                                    attachmentDescription.resolveAttachmentCount) |
                        std::views::transform([](const auto& attachmentReference) {
                            return VkAttachmentReference{.attachment = attachmentReference.attachment,
                                                         .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
                        }),
                    std::back_inserter(attachmentReferences));

                size_t depthStencilStartIndex = attachmentReferences.size();
                if (attachmentDescription.depthStencilAttachment.attachment != snap::rhi::AttachmentUnused) {
                    attachmentReferences.push_back(
                        VkAttachmentReference{.attachment = attachmentDescription.depthStencilAttachment.attachment,
                                              .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
                }

                VkAttachmentReference* pColorAttachments = attachmentDescription.colorAttachmentCount ?
                                                               attachmentReferences.data() + colorStartIndex :
                                                               nullptr;
                VkAttachmentReference* pResolveAttachments = attachmentDescription.resolveAttachmentCount ?
                                                                 attachmentReferences.data() + resolveStartIndex :
                                                                 nullptr;
                VkAttachmentReference* pDepthStencilAttachment =
                    attachmentDescription.depthStencilAttachment.attachment != snap::rhi::AttachmentUnused ?
                        attachmentReferences.data() + depthStencilStartIndex :
                        nullptr;
                return VkSubpassDescription{
                    /**
                     * https://registry.khronos.org/vulkan/specs/latest/man/html/VkSubpassDescriptionFlagBits.html
                     * */
                    .flags = 0,
                    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,

                    /**
                     * SnapRHI currently doesn't support subpasses
                     * */
                    .inputAttachmentCount = 0,
                    .pInputAttachments = nullptr,
                    .colorAttachmentCount = attachmentDescription.colorAttachmentCount,
                    .pColorAttachments = pColorAttachments,
                    .pResolveAttachments = pResolveAttachments,
                    .pDepthStencilAttachment = pDepthStencilAttachment,
                    .preserveAttachmentCount = 0,
                    .pPreserveAttachments = nullptr};
            }),
        vkSubpasses.begin());

    std::vector<VkSubpassDependency> vkDependencies(info.dependencyCount);
    std::ranges::copy(
        std::span<const snap::rhi::SubpassDependency>(info.dependencies.data(), info.dependencyCount) |
            std::views::transform([](const auto& dependency) {
                return VkSubpassDependency{
                    .srcSubpass = dependency.srcSubpass,
                    .dstSubpass = dependency.dstSubpass,
                    .srcStageMask = snap::rhi::backend::vulkan::toVkPipelineStageFlags(dependency.srcStageMask),
                    .dstStageMask = snap::rhi::backend::vulkan::toVkPipelineStageFlags(dependency.dstStageMask),
                    .srcAccessMask = snap::rhi::backend::vulkan::toVkAccessFlags(dependency.srcAccessMask),
                    .dstAccessMask = snap::rhi::backend::vulkan::toVkAccessFlags(dependency.dstAccessMask),
                    .dependencyFlags = snap::rhi::backend::vulkan::toVkDependencyFlags(dependency.dependencyFlags)};
            }),
        vkDependencies.begin());

    VkRenderPassCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,

        /**
         * https://registry.khronos.org/vulkan/specs/latest/man/html/VkRenderPassCreateFlagBits.html
         * */
        .flags = 0,
        .attachmentCount = static_cast<uint32_t>(vkAttachments.size()),
        .pAttachments = vkAttachments.data(),
        .subpassCount = static_cast<uint32_t>(vkSubpasses.size()),
        .pSubpasses = vkSubpasses.data(),
        .dependencyCount = static_cast<uint32_t>(vkDependencies.size()),
        .pDependencies = vkDependencies.data(),
    };

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkResult result = vkCreateRenderPass(vkDevice, &createInfo, nullptr, &renderPass);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::RenderPass] cannot create renderpass");
    return renderPass;
}

VkRenderPass createRenderPass_1_2(const snap::rhi::backend::common::ValidationLayer& validationLayer,
                                  VkDevice vkDevice,
                                  const snap::rhi::RenderPassCreateInfo& info) {
    std::vector<VkAttachmentDescription2> vkAttachments(info.attachmentCount);
    std::ranges::copy(
        std::span<const snap::rhi::AttachmentDescription>(info.attachments.data(), info.attachmentCount) |
            std::views::transform([](const auto& attachmentDescription) {
                VkImageLayout layout = snap::rhi::getDepthStencilFormatTraits(attachmentDescription.format) !=
                                               snap::rhi::DepthStencilFormatTraits::None ?
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                return VkAttachmentDescription2{
                    .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
                    .pNext = nullptr,
                    /**
                     * https://registry.khronos.org/vulkan/specs/latest/man/html/VkAttachmentDescriptionFlagBits.html
                     * */
                    .flags = 0,
                    .format = snap::rhi::backend::vulkan::toVkFormat(attachmentDescription.format),
                    .samples = snap::rhi::backend::vulkan::toVkSampleCountFlagBits(attachmentDescription.samples),
                    .loadOp = snap::rhi::backend::vulkan::toVkAttachmentLoadOp(attachmentDescription.loadOp),
                    .storeOp = snap::rhi::backend::vulkan::toVkAttachmentStoreOp(attachmentDescription.storeOp),
                    .stencilLoadOp =
                        snap::rhi::backend::vulkan::toVkAttachmentLoadOp(attachmentDescription.stencilLoadOp),
                    .stencilStoreOp =
                        snap::rhi::backend::vulkan::toVkAttachmentStoreOp(attachmentDescription.stencilStoreOp),
                    .initialLayout = layout,
                    .finalLayout = layout};
            }),
        vkAttachments.begin());

    std::vector<VkAttachmentReference2> attachmentReferences;
    std::vector<VkSubpassDescriptionDepthStencilResolve> depthStencilResolves;
    std::vector<VkSubpassDescription2> vkSubpasses(info.subpassCount);
    std::ranges::copy(
        std::span<const snap::rhi::SubpassDescription>(info.subpasses.data(), info.subpassCount) |
            std::views::transform([&attachmentReferences, &depthStencilResolves](const auto& attachmentDescription) {
                size_t colorStartIndex = attachmentReferences.size();
                std::ranges::copy(
                    std::span<const snap::rhi::AttachmentReference>(attachmentDescription.colorAttachments.data(),
                                                                    attachmentDescription.colorAttachmentCount) |
                        std::views::transform([](const auto& attachmentReference) {
                            return VkAttachmentReference2{.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                                                          .pNext = nullptr,
                                                          .attachment = attachmentReference.attachment,
                                                          .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                          /**
                                                           * aspectMask is ignored when this structure is used to
                                                           * describe anything other than an input attachment reference.
                                                           * */
                                                          .aspectMask = 0};
                        }),
                    std::back_inserter(attachmentReferences));

                size_t resolveAttachmentsStartIndex = attachmentReferences.size();
                std::ranges::copy(
                    std::span<const snap::rhi::AttachmentReference>(attachmentDescription.resolveAttachments.data(),
                                                                    attachmentDescription.resolveAttachmentCount) |
                        std::views::transform([](const auto& attachmentReference) {
                            return VkAttachmentReference2{.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                                                          .pNext = nullptr,
                                                          .attachment = attachmentReference.attachment,
                                                          .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                          /**
                                                           * aspectMask is ignored when this structure is used to
                                                           * describe anything other than an input attachment reference.
                                                           * */
                                                          .aspectMask = 0};
                        }),
                    std::back_inserter(attachmentReferences));

                size_t depthStencilStartIndex = attachmentReferences.size();
                if (attachmentDescription.depthStencilAttachment.attachment != snap::rhi::AttachmentUnused) {
                    attachmentReferences.push_back(
                        VkAttachmentReference2{.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                                               .pNext = nullptr,
                                               .attachment = attachmentDescription.depthStencilAttachment.attachment,
                                               .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                               /**
                                                * aspectMask is ignored when this structure is used to
                                                * describe anything other than an input attachment reference.
                                                * */
                                               .aspectMask = 0});
                }

                const void* pNext = nullptr;
                if (attachmentDescription.resolveDepthStencilAttachment.attachment != snap::rhi::AttachmentUnused) {
                    size_t resolveDepthStencilStartIndex = attachmentReferences.size();
                    attachmentReferences.emplace_back(VkAttachmentReference2{
                        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                        .pNext = nullptr,
                        .attachment = attachmentDescription.resolveDepthStencilAttachment.attachment,
                        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        /**
                         * aspectMask is ignored when this structure is used to
                         * describe anything other than an input attachment reference.
                         * */
                        .aspectMask = 0});

                    depthStencilResolves.push_back(VkSubpassDescriptionDepthStencilResolve{
                        .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE,
                        .pNext = nullptr,
                        /**
                         * https://registry.khronos.org/vulkan/specs/latest/man/html/VkResolveModeFlagBits.html
                         * */
                        .depthResolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
                        .stencilResolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
                        .pDepthStencilResolveAttachment = attachmentReferences.data() + resolveDepthStencilStartIndex});
                    pNext = &depthStencilResolves.back();
                }

                VkAttachmentReference2* pColorAttachments = attachmentDescription.colorAttachmentCount ?
                                                                attachmentReferences.data() + colorStartIndex :
                                                                nullptr;
                VkAttachmentReference2* pResolveAttachments =
                    attachmentDescription.resolveAttachmentCount ?
                        attachmentReferences.data() + resolveAttachmentsStartIndex :
                        nullptr;
                VkAttachmentReference2* pDepthStencilAttachment =
                    attachmentDescription.depthStencilAttachment.attachment != snap::rhi::AttachmentUnused ?
                        attachmentReferences.data() + depthStencilStartIndex :
                        nullptr;

                return VkSubpassDescription2{
                    .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
                    .pNext = pNext,
                    /**
                     * https://registry.khronos.org/vulkan/specs/latest/man/html/VkSubpassDescriptionFlagBits.html
                     * */
                    .flags = 0,
                    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                    .viewMask = attachmentDescription.viewMask,
                    /**
                     * SnapRHI currently doesn't support subpasses
                     * */
                    .inputAttachmentCount = 0,
                    .pInputAttachments = nullptr,
                    .colorAttachmentCount = attachmentDescription.colorAttachmentCount,
                    .pColorAttachments = pColorAttachments,
                    .pResolveAttachments = pResolveAttachments,
                    .pDepthStencilAttachment = pDepthStencilAttachment,
                    .preserveAttachmentCount = 0,
                    .pPreserveAttachments = nullptr};
            }),
        vkSubpasses.begin());

    std::vector<VkSubpassDependency2> vkDependencies(info.dependencyCount);
    std::ranges::copy(
        std::span<const snap::rhi::SubpassDependency>(info.dependencies.data(), info.dependencyCount) |
            std::views::transform([](const auto& dependency) {
                return VkSubpassDependency2{
                    .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                    .pNext = nullptr,
                    .srcSubpass = dependency.srcSubpass,
                    .dstSubpass = dependency.dstSubpass,
                    .srcStageMask = snap::rhi::backend::vulkan::toVkPipelineStageFlags(dependency.srcStageMask),
                    .dstStageMask = snap::rhi::backend::vulkan::toVkPipelineStageFlags(dependency.dstStageMask),
                    .srcAccessMask = snap::rhi::backend::vulkan::toVkAccessFlags(dependency.srcAccessMask),
                    .dstAccessMask = snap::rhi::backend::vulkan::toVkAccessFlags(dependency.dstAccessMask),
                    .dependencyFlags = snap::rhi::backend::vulkan::toVkDependencyFlags(dependency.dependencyFlags),
                    .viewOffset = 0};
            }),
        vkDependencies.begin());

    VkRenderPassCreateInfo2 createInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .pNext = nullptr,

        /**
         * https://registry.khronos.org/vulkan/specs/latest/man/html/VkRenderPassCreateFlagBits.html
         * */
        .flags = 0,
        .attachmentCount = static_cast<uint32_t>(vkAttachments.size()),
        .pAttachments = vkAttachments.data(),
        .subpassCount = static_cast<uint32_t>(vkSubpasses.size()),
        .pSubpasses = vkSubpasses.data(),
        .dependencyCount = static_cast<uint32_t>(vkDependencies.size()),
        .pDependencies = vkDependencies.data(),
        .correlatedViewMaskCount = 0,
        .pCorrelatedViewMasks = nullptr,
    };

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkResult result = vkCreateRenderPass2(vkDevice, &createInfo, nullptr, &renderPass);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::RenderPass] cannot create renderpass");

    return renderPass;
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
RenderPass::RenderPass(snap::rhi::backend::vulkan::Device* device, const snap::rhi::RenderPassCreateInfo& info)
    : snap::rhi::RenderPass(device, info), vkDevice(device->getVkLogicalDevice()) {
    const auto& validationLayer = device->getValidationLayer();

    const auto& deviceProperties = device->getPhysicalDeviceProperties();

    // TODO: Add VK_KHR_depth_stencil_resolve support check
    if (deviceProperties.apiVersion < VK_API_VERSION_1_2) {
        SNAP_RHI_VALIDATE(validationLayer,
                          std::ranges::all_of(
                              std::span<const snap::rhi::SubpassDescription>(info.subpasses.data(), info.subpassCount),
                              [](const auto& subpass) { return subpass.viewMask == 0; }),
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::RenderPass] Multiview not supported in Vulkan 1.0");

        SNAP_RHI_VALIDATE(validationLayer,
                          std::ranges::all_of(
                              std::span<const snap::rhi::SubpassDescription>(info.subpasses.data(), info.subpassCount),
                              [](const auto& subpass) {
                                  return subpass.resolveDepthStencilAttachment.attachment ==
                                         snap::rhi::AttachmentUnused;
                              }),
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::RenderPass] ResolveDepthStencilAttachment not supported in Vulkan 1.0");
        renderPass = createRenderPass_1_0(validationLayer, vkDevice, info);
    } else {
        renderPass = createRenderPass_1_2(validationLayer, vkDevice, info);
    }
}

RenderPass::~RenderPass() {
    assert(renderPass != VK_NULL_HANDLE);

    vkDestroyRenderPass(vkDevice, renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;
}

void RenderPass::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_RENDER_PASS, reinterpret_cast<uint64_t>(renderPass), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
