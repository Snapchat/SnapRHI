#include "snap/rhi/backend/vulkan/ComputePipeline.h"

#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/PipelineCache.h"
#include "snap/rhi/backend/vulkan/PipelineLayout.h"
#include "snap/rhi/backend/vulkan/ShaderModule.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

namespace snap::rhi::backend::vulkan {
ComputePipeline::ComputePipeline(snap::rhi::backend::vulkan::Device* device,
                                 const snap::rhi::ComputePipelineCreateInfo& info)
    : snap::rhi::ComputePipeline(device, info),
      validationLayer(device->getValidationLayer()),
      vkDevice(device->getVkLogicalDevice()),
      resourceResidencySet(device, snap::rhi::backend::common::ResourceResidencySet::RetentionMode::RetainReferences) {
    SNAP_RHI_VALIDATE(validationLayer,
                      info.stage != nullptr,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::ComputePipeline] ShaderModule must be specified for ComputePipeline");
    SNAP_RHI_VALIDATE(validationLayer,
                      info.pipelineLayout != nullptr,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::ComputePipeline] PipelineLayout must be specified for ComputePipeline");

    auto* pPipelineLayout =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::PipelineLayout>(info.pipelineLayout);
    resourceResidencySet.track(pPipelineLayout);

    descriptorSetCount = static_cast<uint32_t>(pPipelineLayout->getCreateInfo().setLayouts.size());

    auto* shaderModule = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::ShaderModule>(info.stage);
    auto* basePipeline =
        info.basePipeline ?
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::ComputePipeline>(info.basePipeline) :
            nullptr;
    auto* pPipelineCache =
        info.pipelineCache ?
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::PipelineCache>(info.pipelineCache) :
            nullptr;

    VkPipelineCache pipelineCache = pPipelineCache ? pPipelineCache->getPipelineCache() : VK_NULL_HANDLE;

    VkComputePipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.pNext = nullptr;

    /**
     * https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineCreateFlagBits.html
     * **/
    createInfo.flags = 0;

    createInfo.stage = shaderModule->getShaderStage();
    createInfo.layout = this->pipelineLayout = pPipelineLayout->getPipelineLayout();

    createInfo.basePipelineIndex = -1;

    if (basePipeline) {
        createInfo.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        createInfo.basePipelineHandle = basePipeline->getVkPipeline();
    } else {
        createInfo.flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
        createInfo.basePipelineHandle = VK_NULL_HANDLE;
    }

    const bool isNativeReflectionAcquired = (info.pipelineCreateFlags & PipelineCreateFlags::AcquireNativeReflection) ==
                                            PipelineCreateFlags::AcquireNativeReflection;
    if (isNativeReflectionAcquired) {
        if (const auto* entryPointReflection = shaderModule->getEntryPointReflection(); entryPointReflection) {
            reflection = snap::rhi::reflection::ComputePipelineInfo{.descriptorSetInfos =
                                                                        entryPointReflection->descriptorSetInfo};
        }
    }

    VkResult result = vkCreateComputePipelines(vkDevice, pipelineCache, 1, &createInfo, nullptr, &pipeline);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::ComputePipeline] Cannot create ComputePipeline");
}

ComputePipeline::~ComputePipeline() {
    if (pipeline == VK_NULL_HANDLE) {
        return;
    }

    vkDestroyPipeline(vkDevice, pipeline, nullptr);
    pipeline = VK_NULL_HANDLE;
}

void ComputePipeline::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(pipeline), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
