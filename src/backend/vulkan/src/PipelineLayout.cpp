#include "snap/rhi/backend/vulkan/PipelineLayout.h"
#include "snap/rhi/backend/vulkan/DescriptorSetLayout.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

namespace snap::rhi::backend::vulkan {
PipelineLayout::PipelineLayout(snap::rhi::backend::vulkan::Device* device,
                               const snap::rhi::PipelineLayoutCreateInfo& info)
    : snap::rhi::backend::common::PipelineLayout(device, info), vkDevice(device->getVkLogicalDevice()) {
    const auto& validationLayer = device->getValidationLayer();
    {
        std::vector<VkDescriptorSetLayout> setLayouts(info.setLayouts.size());
        for (size_t i = 0; i < info.setLayouts.size(); ++i) {
            auto* setLayout = snap::rhi::backend::common::smart_cast<DescriptorSetLayout>(info.setLayouts[i]);
            setLayouts[i] = setLayout->getVkDescriptorSetLayout();
        }

        const VkPipelineLayoutCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            /**
             * https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineLayoutCreateFlagBits.html
             * **/
            .flags = 0,
            .setLayoutCount = static_cast<uint32_t>(info.setLayouts.size()),
            .pSetLayouts = setLayouts.data(),
            /**
             * SnapRHI doesn't implement PushConstant API
             * */
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        };

        VkResult result = vkCreatePipelineLayout(this->vkDevice, &createInfo, nullptr, &pipelineLayout);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == VK_SUCCESS,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::PipelineLayout] cannot create pipeline layout");
    }
}

PipelineLayout::~PipelineLayout() {
    if (pipelineLayout == VK_NULL_HANDLE) {
        return;
    }

    vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;
}

void PipelineLayout::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(
        vkDevice, VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(pipelineLayout), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
