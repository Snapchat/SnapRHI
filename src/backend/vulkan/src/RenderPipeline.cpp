#include "snap/rhi/backend/vulkan/RenderPipeline.h"

#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/PipelineCache.h"
#include "snap/rhi/backend/vulkan/PipelineLayout.h"
#include "snap/rhi/backend/vulkan/RenderPass.h"
#include "snap/rhi/backend/vulkan/ShaderModule.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"
#include <algorithm>
#include <ranges>

namespace {
VkPipelineVertexInputStateCreateInfo buildVertexInputStateCreateInfo(
    const std::vector<VkVertexInputBindingDescription>& vertexBindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions) {
    return {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size()),
            .pVertexBindingDescriptions = vertexBindingDescriptions.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size()),
            .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()};
}

VkPipelineInputAssemblyStateCreateInfo buildInputAssemblyStateCreateInfo(
    const snap::rhi::InputAssemblyStateCreateInfo& info) {
    return {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = snap::rhi::backend::vulkan::toVkPrimitiveTopology(info.primitiveTopology),
            .primitiveRestartEnable = static_cast<bool>(
                snap::rhi::backend::vulkan::isStripPrimitiveTopology(info.primitiveTopology) ? VK_TRUE : VK_FALSE)};
}

VkPipelineTessellationStateCreateInfo buildTessellationStateCreateInfo() {
    return {.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .patchControlPoints = 0};
}

VkPipelineViewportStateCreateInfo buildViewportStateCreateInfo() {
    return {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = nullptr,
            .scissorCount = 1,
            .pScissors = nullptr};
}

VkPipelineDynamicStateCreateInfo buildDynamicStateCreateInfo() {
    static constexpr std::array DynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,          // vkCmdSetViewport
        VK_DYNAMIC_STATE_SCISSOR,           // vkCmdSetScissor
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,   // vkCmdSetBlendConstants
        VK_DYNAMIC_STATE_STENCIL_REFERENCE, // vkCmdSetStencilReference
        VK_DYNAMIC_STATE_DEPTH_BIAS,        // vkCmdSetDepthBias
    };

    return {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .dynamicStateCount = static_cast<uint32_t>(DynamicStates.size()),
            .pDynamicStates = DynamicStates.data()};
}

VkPipelineRasterizationStateCreateInfo buildRasterizationStateCreateInfo(
    const snap::rhi::RasterizationStateCreateInfo& info) {
    return {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            /*
             * non-constant-expression cannot be narrowed from type 'int' to 'VkBool32' (aka 'unsigned int')
             * in initializer list [-Wc++11-narrowing]
             **/
            .rasterizerDiscardEnable = static_cast<VkBool32>(info.rasterizationEnabled ? VK_FALSE : VK_TRUE),
            .polygonMode = snap::rhi::backend::vulkan::toVkPolygonMode(info.polygonMode),
            .cullMode = snap::rhi::backend::vulkan::toVkCullModeFlags(info.cullMode),
            .frontFace = snap::rhi::backend::vulkan::toVkFrontFace(info.windingMode),
            .depthBiasEnable = static_cast<VkBool32>(info.depthBiasEnable ? VK_TRUE : VK_FALSE),
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f};
}

VkPipelineMultisampleStateCreateInfo buildMultisampleStateCreateInfo(
    const snap::rhi::MultisampleStateCreateInfo& info) {
    return {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .rasterizationSamples = snap::rhi::backend::vulkan::toVkSampleCountFlagBits(info.samples),
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = static_cast<VkBool32>(info.alphaToCoverageEnable ? VK_TRUE : VK_FALSE),
            .alphaToOneEnable = static_cast<VkBool32>(info.alphaToOneEnable ? VK_TRUE : VK_FALSE)};
}

VkStencilOpState buildStencilOpState(const snap::rhi::StencilOpState& state) {
    return {.failOp = snap::rhi::backend::vulkan::toVkStencilOp(state.failOp),
            .passOp = snap::rhi::backend::vulkan::toVkStencilOp(state.passOp),
            .depthFailOp = snap::rhi::backend::vulkan::toVkStencilOp(state.depthFailOp),
            .compareOp = snap::rhi::backend::vulkan::toVkCompareOp(state.stencilFunc),
            .compareMask = static_cast<uint32_t>(state.readMask),
            .writeMask = static_cast<uint32_t>(state.writeMask),
            .reference = 0};
}

VkPipelineDepthStencilStateCreateInfo buildDepthStencilStateCreateInfo(
    const snap::rhi::DepthStencilStateCreateInfo& info, const VkPhysicalDeviceFeatures& features) {
    return {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = static_cast<VkBool32>(info.depthTest ? VK_TRUE : VK_FALSE),
            .depthWriteEnable = static_cast<VkBool32>(info.depthWrite ? VK_TRUE : VK_FALSE),
            .depthCompareOp = snap::rhi::backend::vulkan::toVkCompareOp(info.depthFunc),
            .depthBoundsTestEnable = static_cast<VkBool32>(features.depthBounds ? VK_TRUE : VK_FALSE),
            .stencilTestEnable = static_cast<VkBool32>(info.stencilEnable ? VK_TRUE : VK_FALSE),
            .front = buildStencilOpState(info.stencilFront),
            .back = buildStencilOpState(info.stencilBack),
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f};
}

VkPipelineColorBlendStateCreateInfo buildColorBlendStateCreateInfo(
    const std::vector<VkPipelineColorBlendAttachmentState>& colorBlendAttachmentsState) {
    return {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = static_cast<uint32_t>(colorBlendAttachmentsState.size()),
            .pAttachments = colorBlendAttachmentsState.data(),
            .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}};
}

VkPipelineRenderingCreateInfo buildRenderingCreateInfo(const snap::rhi::AttachmentFormatsCreateInfo& info,
                                                       const std::vector<VkFormat>& colorAttachmentFormats) {
    return {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size()),
            .pColorAttachmentFormats = colorAttachmentFormats.data(),
            .depthAttachmentFormat = snap::rhi::backend::vulkan::toVkFormat(info.depthAttachmentFormat),
            .stencilAttachmentFormat = snap::rhi::backend::vulkan::toVkFormat(info.stencilAttachmentFormat)};
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
RenderPipeline::RenderPipeline(snap::rhi::backend::vulkan::Device* device,
                               const snap::rhi::RenderPipelineCreateInfo& info)
    : snap::rhi::RenderPipeline(device, info),
      validationLayer(device->getValidationLayer()),
      vkDevice(device->getVkLogicalDevice()),
      resourceResidencySet(device, snap::rhi::backend::common::ResourceResidencySet::RetentionMode::RetainReferences) {
    SNAP_RHI_VALIDATE(validationLayer,
                      info.stages.size() >= 1,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::RenderPipeline] At least one ShaderModule must be specified for RenderPipeline");
    SNAP_RHI_VALIDATE(validationLayer,
                      info.pipelineLayout != nullptr,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::RenderPipeline] PipelineLayout must be specified for RenderPipeline");

    const auto& deviceFeatures = device->getPhysicalDeviceFeatures();

    auto* basePipeline =
        info.basePipeline ?
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::RenderPipeline>(info.basePipeline) :
            nullptr;
    auto* pPipelineCache =
        info.pipelineCache ?
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::PipelineCache>(info.pipelineCache) :
            nullptr;
    VkPipelineCache pipelineCache = pPipelineCache ? pPipelineCache->getPipelineCache() : VK_NULL_HANDLE;

    auto* pRenderPass =
        info.renderPass ?
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::RenderPass>(info.renderPass) :
            nullptr;
    VkRenderPass renderPass = pRenderPass ? pRenderPass->getRenderPass() : VK_NULL_HANDLE;

    auto* pPipelineLayout =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::PipelineLayout>(info.pipelineLayout);
    resourceResidencySet.track(pPipelineLayout);

    descriptorSetCount = static_cast<uint32_t>(pPipelineLayout->getCreateInfo().setLayouts.size());

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(info.stages.size());
    std::ranges::copy(info.stages | std::ranges::views::transform([&](const auto& stage) {
                          SNAP_RHI_VALIDATE(validationLayer,
                                            stage != nullptr,
                                            snap::rhi::ReportLevel::CriticalError,
                                            snap::rhi::ValidationTag::CreateOp,
                                            "[Vulkan::RenderPipeline] ShaderModule in stages must be not null");
                          auto* shaderModule =
                              snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::ShaderModule>(stage);
                          return shaderModule->getShaderStage();
                      }),
                      std::back_inserter(shaderStages));

    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions{};
    vertexBindingDescriptions.reserve(info.vertexInputState.bindingsCount);
    std::ranges::copy(std::span(info.vertexInputState.bindingDescription.data(), info.vertexInputState.bindingsCount) |
                          std::views::transform([](const auto& bindingDescription) {
                              return VkVertexInputBindingDescription{
                                  .binding = bindingDescription.binding,
                                  .stride = bindingDescription.inputRate == snap::rhi::VertexInputRate::Constant ?
                                                0 :
                                                bindingDescription.stride,
                                  .inputRate =
                                      snap::rhi::backend::vulkan::toVkVertexInputRate(bindingDescription.inputRate)};
                          }),
                      std::back_inserter(vertexBindingDescriptions));

    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions{};
    vertexAttributeDescriptions.reserve(info.vertexInputState.attributesCount);
    std::ranges::copy(
        std::span(info.vertexInputState.attributeDescription.data(), info.vertexInputState.attributesCount) |
            std::views::transform([](const auto& attributeDescription) {
                return VkVertexInputAttributeDescription{
                    .location = attributeDescription.location,
                    .binding = attributeDescription.binding,
                    .format = snap::rhi::backend::vulkan::toVkFormat(attributeDescription.format),
                    .offset = attributeDescription.offset};
            }),
        std::back_inserter(vertexAttributeDescriptions));

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentsState{};
    colorBlendAttachmentsState.reserve(info.colorBlendState.colorAttachmentsCount);
    std::ranges::copy(
        std::span(info.colorBlendState.colorAttachmentsBlendState.data(), info.colorBlendState.colorAttachmentsCount) |
            std::views::transform([](const auto& blendState) {
                return VkPipelineColorBlendAttachmentState{
                    .blendEnable = static_cast<VkBool32>(blendState.blendEnable ? VK_TRUE : VK_FALSE),
                    .srcColorBlendFactor = snap::rhi::backend::vulkan::toVkBlendFactor(blendState.srcColorBlendFactor),
                    .dstColorBlendFactor = snap::rhi::backend::vulkan::toVkBlendFactor(blendState.dstColorBlendFactor),
                    .colorBlendOp = snap::rhi::backend::vulkan::toVkBlendOp(blendState.colorBlendOp),
                    .srcAlphaBlendFactor = snap::rhi::backend::vulkan::toVkBlendFactor(blendState.srcAlphaBlendFactor),
                    .dstAlphaBlendFactor = snap::rhi::backend::vulkan::toVkBlendFactor(blendState.dstAlphaBlendFactor),
                    .alphaBlendOp = snap::rhi::backend::vulkan::toVkBlendOp(blendState.alphaBlendOp),
                    .colorWriteMask = static_cast<VkColorComponentFlags>(blendState.colorWriteMask)};
            }),
        std::back_inserter(colorBlendAttachmentsState));

    std::vector<VkFormat> colorAttachmentFormats;
    colorAttachmentFormats.reserve(info.attachmentFormatsCreateInfo.colorAttachmentFormats.size());
    std::ranges::copy(
        info.attachmentFormatsCreateInfo.colorAttachmentFormats |
            std::views::transform([](const auto& format) { return snap::rhi::backend::vulkan::toVkFormat(format); }),
        std::back_inserter(colorAttachmentFormats));

    auto vertexInputState = buildVertexInputStateCreateInfo(vertexBindingDescriptions, vertexAttributeDescriptions);
    auto inputAssemblyState = buildInputAssemblyStateCreateInfo(info.inputAssemblyState);
    auto tessellationState = buildTessellationStateCreateInfo();
    auto viewportState = buildViewportStateCreateInfo();
    auto rasterizationState = buildRasterizationStateCreateInfo(info.rasterizationState);
    auto multisampleState = buildMultisampleStateCreateInfo(info.multisampleState);
    auto depthStencilState = buildDepthStencilStateCreateInfo(info.depthStencilState, deviceFeatures);
    auto colorBlendState = buildColorBlendStateCreateInfo(colorBlendAttachmentsState);
    auto dynamicState = buildDynamicStateCreateInfo();
    auto renderingCreateInfo = buildRenderingCreateInfo(info.attachmentFormatsCreateInfo, colorAttachmentFormats);

    this->pipelineLayout = pPipelineLayout->getPipelineLayout();
    const void* pNext = renderPass != VK_NULL_HANDLE ? nullptr : static_cast<const void*>(&renderingCreateInfo);
    /**
     * https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineCreateFlagBits.html
     */
    const VkPipelineCreateFlags flags =
        basePipeline ? VK_PIPELINE_CREATE_DERIVATIVE_BIT : VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    const VkPipeline basePipelineHandle = basePipeline ? basePipeline->getVkPipeline() : VK_NULL_HANDLE;

    const VkGraphicsPipelineCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = pNext,
        .flags = flags,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pTessellationState = &tessellationState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = this->pipelineLayout,
        .renderPass = renderPass,
        .subpass = renderPass != VK_NULL_HANDLE ? info.subpass : 0u,
        .basePipelineHandle = basePipelineHandle,
        .basePipelineIndex = -1,
    };

    const bool isNativeReflectionAcquired = (info.pipelineCreateFlags & PipelineCreateFlags::AcquireNativeReflection) ==
                                            PipelineCreateFlags::AcquireNativeReflection;
    if (isNativeReflectionAcquired) {
        const auto& stagesTransformView =
            info.stages | std::ranges::views::transform([&](const auto& stage) {
                SNAP_RHI_VALIDATE(validationLayer,
                                  stage != nullptr,
                                  snap::rhi::ReportLevel::CriticalError,
                                  snap::rhi::ValidationTag::CreateOp,
                                  "[Vulkan::RenderPipeline] ShaderModule in stages must be not null");
                auto* shaderModule =
                    snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::ShaderModule>(stage);
                return shaderModule->getEntryPointReflection();
            });
        for (const auto* stageReflection : stagesTransformView) {
            SNAP_RHI_VALIDATE(validationLayer,
                              stageReflection != nullptr,
                              snap::rhi::ReportLevel::CriticalError,
                              snap::rhi::ValidationTag::CreateOp,
                              "[Vulkan::RenderPipeline] ShaderModule reflection must be not null");
            if (reflection != std::nullopt) {
                if (stageReflection->entryPointInfo.stage == snap::rhi::ShaderStage::Vertex) {
                    reflection->vertexAttributes = stageReflection->vertexAttributes;
                }

                for (const auto& dsReflection : stageReflection->descriptorSetInfo) {
                    auto itr = std::ranges::find(reflection->descriptorSetInfos,
                                                 dsReflection.binding,
                                                 &snap::rhi::reflection::DescriptorSetInfo::binding);

                    if (itr == reflection->descriptorSetInfos.end()) {
                        reflection->descriptorSetInfos.push_back(dsReflection);
                    } else {
                        SNAP_RHI_VALIDATE(validationLayer,
                                          dsReflection == *itr,
                                          snap::rhi::ReportLevel::CriticalError,
                                          snap::rhi::ValidationTag::CreateOp,
                                          "[Vulkan::RenderPipeline] RenderPipeline should have the same reflection for "
                                          "each stage for the same DescriptoSet binding");
                    }
                }
            } else {
                reflection = snap::rhi::reflection::RenderPipelineInfo{
                    .descriptorSetInfos = stageReflection->descriptorSetInfo,
                    .vertexAttributes = stageReflection->vertexAttributes,
                };
            }
        }
    }

    VkResult result = vkCreateGraphicsPipelines(vkDevice, pipelineCache, 1, &createInfo, nullptr, &pipeline);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::RenderPipeline] Cannot create RenderPipeline");
}

RenderPipeline::~RenderPipeline() {
    if (pipeline == VK_NULL_HANDLE) {
        return;
    }

    vkDestroyPipeline(vkDevice, pipeline, nullptr);
    pipeline = VK_NULL_HANDLE;
}

void RenderPipeline::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(pipeline), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
