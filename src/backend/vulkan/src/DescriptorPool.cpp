#include "snap/rhi/backend/vulkan/DescriptorPool.h"
#include "snap/rhi/backend/vulkan/DescriptorSetLayout.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include <algorithm>

namespace snap::rhi::backend::vulkan {
DescriptorPool::DescriptorPool(snap::rhi::backend::vulkan::Device* device,
                               const snap::rhi::DescriptorPoolCreateInfo& info)
    : snap::rhi::DescriptorPool(device, info),
      vkDevice(device->getVkLogicalDevice()),
      validationLayer(device->getValidationLayer()) {
    std::ranges::fill(availableDescriptorTypeCount, 0);
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(info.descriptorCount.size());
    for (size_t i = 0; i < info.descriptorCount.size(); ++i) {
        const auto descriptorType = static_cast<snap::rhi::DescriptorType>(i);
        if (descriptorType == snap::rhi::DescriptorType::Undefined || info.descriptorCount[i] == 0) {
            continue;
        }

        availableDescriptorTypeCount[i] += info.descriptorCount[i];
        poolSizes.push_back(VkDescriptorPoolSize{.type = getDescriptorType(descriptorType),
                                                 .descriptorCount = info.descriptorCount[i]});
    }

    availableSetCount = info.maxSets;
    const VkDescriptorPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        /**
         * https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorPoolCreateFlagBits.html
         * **/
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = info.maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    const VkResult result = vkCreateDescriptorPool(vkDevice, &createInfo, nullptr, &descriptorPool);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::DescriptorPoolOp,
                      "[Vulkan::DescriptorPool] Cannot create DescriptorPool");
}

DescriptorPool::~DescriptorPool() {
    if (descriptorPool == VK_NULL_HANDLE) {
        return;
    }

    vkDestroyDescriptorPool(vkDevice, descriptorPool, nullptr);
    descriptorPool = VK_NULL_HANDLE;
}

std::unique_ptr<VkDescriptorSet, std::function<void(VkDescriptorSet*)>> DescriptorPool::allocateDescriptorSet(
    snap::rhi::backend::vulkan::DescriptorSetLayout* layout) const {
    const auto& descriptorSetLayoutInfo = layout->getCreateInfo();
    auto descriptorSetLayout = layout->getVkDescriptorSetLayout();

    if (availableSetCount == 0) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::DescriptorPoolOp,
                        "[Vulkan::DescriptorPool] Cannot allocate DescriptorSet, not enough sets");
        return nullptr;
    }
    --availableSetCount;

    std::array<uint32_t, static_cast<size_t>(snap::rhi::DescriptorType::Count)> acquiredDescriptorTypeCount{};
    std::ranges::fill(acquiredDescriptorTypeCount, 0);
    for (const auto& poolSize : descriptorSetLayoutInfo.bindings) {
        const auto type = static_cast<size_t>(poolSize.descriptorType);
        ++acquiredDescriptorTypeCount[type];
    }

    for (size_t i = 0; i < acquiredDescriptorTypeCount.size(); ++i) {
        if (acquiredDescriptorTypeCount[i] > availableDescriptorTypeCount[i]) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Warning,
                            snap::rhi::ValidationTag::DescriptorPoolOp,
                            "[Vulkan::DescriptorPool] Cannot allocate DescriptorSet, not enough descriptors{%d}",
                            i);
            return nullptr;
        }
    }

    for (size_t i = 0; i < acquiredDescriptorTypeCount.size(); ++i) {
        availableDescriptorTypeCount[i] -= acquiredDescriptorTypeCount[i];
    }

    VkDescriptorSetAllocateInfo createInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                           .pNext = nullptr,
                                           .descriptorPool = descriptorPool,
                                           .descriptorSetCount = 1u,
                                           .pSetLayouts = &descriptorSetLayout};
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkResult result = vkAllocateDescriptorSets(vkDevice, &createInfo, &descriptorSet);
    if (result != VK_SUCCESS) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::DescriptorPoolOp,
                        "[Vulkan::DescriptorPool] Cannot allocate DescriptorSet");
        return nullptr;
    }

    return {new VkDescriptorSet(descriptorSet),
            [this, resources = std::move(acquiredDescriptorTypeCount)](VkDescriptorSet* ptr) {
                if (!ptr) {
                    return;
                }

                ++availableSetCount;
                for (size_t i = 0; i < resources.size(); ++i) {
                    availableDescriptorTypeCount[i] += resources[i];
                }

                vkFreeDescriptorSets(vkDevice, descriptorPool, 1, ptr);
                delete ptr;
            }};
}
} // namespace snap::rhi::backend::vulkan
