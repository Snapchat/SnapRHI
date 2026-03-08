#include "snap/rhi/backend/vulkan/Sampler.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"

namespace snap::rhi::backend::vulkan {
Sampler::Sampler(Device* device, const snap::rhi::SamplerCreateInfo& info)
    : snap::rhi::Sampler(device, info), vkDevice(device->getVkLogicalDevice()) {
    const auto& validationLayer = device->getValidationLayer();
    {
        const VkSamplerMipmapMode mipmapMode = info.mipFilter == snap::rhi::SamplerMipFilter::NotMipmapped ?
                                                   VK_SAMPLER_MIPMAP_MODE_NEAREST :
                                                   snap::rhi::backend::vulkan::toVkSamplerMipmapMode(info.mipFilter);
        const float minLod = info.mipFilter == snap::rhi::SamplerMipFilter::NotMipmapped ? 0.0f : info.lodMin;
        const float maxLod =
            info.mipFilter == snap::rhi::SamplerMipFilter::NotMipmapped ?
                0.0f :
                (snap::rhi::backend::common::epsilonEqual(info.lodMax, snap::rhi::DefaultSamplerLodMax, 0.0001f) ?
                     VK_LOD_CLAMP_NONE :
                     info.lodMax);

        const VkSamplerCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            /**
             * https://registry.khronos.org/vulkan/specs/latest/man/html/VkSamplerCreateFlagBits.html
             * **/
            .flags = 0,
            .magFilter = snap::rhi::backend::vulkan::toVkFilter(info.magFilter),
            .minFilter = snap::rhi::backend::vulkan::toVkFilter(info.minFilter),
            .mipmapMode = mipmapMode,
            .addressModeU = snap::rhi::backend::vulkan::toVkSamplerAddressMode(info.wrapU),
            .addressModeV = snap::rhi::backend::vulkan::toVkSamplerAddressMode(info.wrapV),
            .addressModeW = snap::rhi::backend::vulkan::toVkSamplerAddressMode(info.wrapW),
            .mipLodBias = 0.0f,
            .anisotropyEnable = static_cast<VkBool32>(info.anisotropyEnable ? VK_TRUE : VK_FALSE),
            .maxAnisotropy = static_cast<float>(info.maxAnisotropy),
            .compareEnable = static_cast<VkBool32>(info.compareEnable ? VK_TRUE : VK_FALSE),
            .compareOp = snap::rhi::backend::vulkan::toVkCompareOp(info.compareFunction),
            .minLod = minLod,
            .maxLod = maxLod,
            .borderColor = snap::rhi::backend::vulkan::toVkBorderColor(info.borderColor),
            .unnormalizedCoordinates = static_cast<VkBool32>(info.unnormalizedCoordinates ? VK_TRUE : VK_FALSE),
        };

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
