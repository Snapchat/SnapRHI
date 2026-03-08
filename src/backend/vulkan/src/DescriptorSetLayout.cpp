#include "snap/rhi/backend/vulkan/DescriptorSetLayout.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

namespace {
VkDescriptorSetLayoutBinding buildDescriptorSetLayoutBinding(const snap::rhi::DescriptorSetLayoutBinding& info) {
    VkDescriptorSetLayoutBinding result{.binding = info.binding,
                                        .descriptorType =
                                            snap::rhi::backend::vulkan::getDescriptorType(info.descriptorType),
                                        .descriptorCount = 1u,
                                        .stageFlags = snap::rhi::backend::vulkan::getShaderStageFlags(info.stageBits),
                                        .pImmutableSamplers = nullptr};

    return result;
}
} // unnamed namespace

namespace snap::rhi::backend::vulkan {
DescriptorSetLayout::DescriptorSetLayout(Device* device, const snap::rhi::DescriptorSetLayoutCreateInfo& info)
    : snap::rhi::DescriptorSetLayout(device, info), vkDevice(device->getVkLogicalDevice()) {
    const auto& validationLayer = device->getValidationLayer();
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings(info.bindings.size());
        for (size_t i = 0; i < bindings.size(); ++i) {
            bindings[i] = buildDescriptorSetLayoutBinding(info.bindings[i]);
        }

        const VkDescriptorSetLayoutCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            /**
             * https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorSetLayoutCreateFlagBits.html
             * **/
            .flags = 0,
            .bindingCount = static_cast<uint32_t>(info.bindings.size()),
            .pBindings = bindings.data(),
        };

        VkResult result = vkCreateDescriptorSetLayout(this->vkDevice, &createInfo, nullptr, &setLayout);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == VK_SUCCESS,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::DescriptorSetLayoutOp,
                          "[Vulkan::DescriptorSetLayout] cannot create DescriptorSetLayout");
    }
}

DescriptorSetLayout::~DescriptorSetLayout() {
    if (setLayout == VK_NULL_HANDLE) {
        return;
    }

    vkDestroyDescriptorSetLayout(vkDevice, setLayout, nullptr);
    setLayout = VK_NULL_HANDLE;
}

void DescriptorSetLayout::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(
        vkDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, reinterpret_cast<uint64_t>(setLayout), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
