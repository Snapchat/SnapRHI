#include "snap/rhi/backend/vulkan/Sampler.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

namespace snap::rhi::backend::vulkan {
Sampler::Sampler(Device* device, const snap::rhi::SamplerCreateInfo& info)
    : snap::rhi::Sampler(device, info), vkDevice(device->getVkLogicalDevice()) {
    const auto& validationLayer = device->getValidationLayer();
    {
        VkSamplerCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext = nullptr;

        /**
         * https://registry.khronos.org/vulkan/specs/latest/man/html/VkSamplerCreateFlagBits.html
         * **/
        createInfo.flags = 0;

        createInfo.magFilter = snap::rhi::backend::vulkan::toVkFilter(info.magFilter);
        createInfo.minFilter = snap::rhi::backend::vulkan::toVkFilter(info.minFilter);

        if (info.mipFilter == snap::rhi::SamplerMipFilter::NotMipmapped) {
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            createInfo.minLod = createInfo.maxLod = 0.0f;
        } else {
            createInfo.mipmapMode = snap::rhi::backend::vulkan::toVkSamplerMipmapMode(info.mipFilter);
            createInfo.minLod = info.lodMin;
            createInfo.maxLod =
                snap::rhi::backend::common::epsilonEqual(info.lodMax, snap::rhi::DefaultSamplerLodMax, 0.0001f) ?
                    VK_LOD_CLAMP_NONE :
                    info.lodMax;
        }

        createInfo.addressModeU = snap::rhi::backend::vulkan::toVkSamplerAddressMode(info.wrapU);
        createInfo.addressModeV = snap::rhi::backend::vulkan::toVkSamplerAddressMode(info.wrapV);
        createInfo.addressModeW = snap::rhi::backend::vulkan::toVkSamplerAddressMode(info.wrapW);

        createInfo.mipLodBias = 0.0f;

        createInfo.anisotropyEnable = info.anisotropyEnable ? VK_TRUE : VK_FALSE;
        createInfo.maxAnisotropy = static_cast<float>(info.maxAnisotropy);

        createInfo.compareEnable = info.compareEnable ? VK_TRUE : VK_FALSE;
        createInfo.compareOp = snap::rhi::backend::vulkan::toVkCompareOp(info.compareFunction);

        createInfo.borderColor = snap::rhi::backend::vulkan::toVkBorderColor(info.borderColor);

        createInfo.unnormalizedCoordinates = info.unnormalizedCoordinates ? VK_TRUE : VK_FALSE;

        VkResult result = vkCreateSampler(this->vkDevice, &createInfo, nullptr, &sampler);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == VK_SUCCESS,
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::CreateOp,
                          "[Vulkan::Sampler] Cannot create VkSampler");
    }
}

Sampler::~Sampler() {
    if (sampler == VK_NULL_HANDLE) {
        return;
    }

    vkDestroySampler(vkDevice, sampler, nullptr);
    sampler = VK_NULL_HANDLE;
}

void Sampler::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64_t>(sampler), label.data());
#endif
}
} // namespace snap::rhi::backend::vulkan
