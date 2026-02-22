#include "snap/rhi/backend/vulkan/DescriptorSet.h"
#include "snap/rhi/backend/vulkan/Buffer.h"
#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/DescriptorPool.h"
#include "snap/rhi/backend/vulkan/DescriptorSetLayout.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/ImageViewCache.h"
#include "snap/rhi/backend/vulkan/Sampler.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/VkDebugUtils.h"
#include <snap/rhi/common/Throw.h>

namespace snap::rhi::backend::vulkan {
DescriptorSet::DescriptorSet(snap::rhi::backend::vulkan::Device* device, const snap::rhi::DescriptorSetCreateInfo& info)
    : snap::rhi::backend::common::DescriptorSet<>(device, info),
      vkDevice(device->getVkLogicalDevice()),
      validationLayer(device->getValidationLayer()) {
    auto* pDescriptorSetLayout =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::DescriptorSetLayout>(
            info.descriptorSetLayout);
    auto pDescriptorPool =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::DescriptorPool>(info.descriptorPool);
    descriptorSet = pDescriptorPool->allocateDescriptorSet(pDescriptorSetLayout);
}

void DescriptorSet::setResource(const uint32_t binding, snap::rhi::DeviceChild* resource) {
    const size_t idx = getIdxFromBinding(binding);
    SNAP_RHI_VALIDATE(validationLayer,
                      idx != InvalidIdx,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::setResource] invalid binding %u.",
                      binding);

    descriptors[idx].resourcePtr = resource;
}

void DescriptorSet::bindUniformBuffer(const uint32_t binding,
                                      snap::rhi::Buffer* buffer,
                                      const uint64_t offset,
                                      const uint64_t range) {
    auto* pBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Buffer>(buffer);
    setResource(binding, pBuffer);

    VkDescriptorBufferInfo bufferInfo{
        .buffer = pBuffer->getVkBuffer(), .offset = offset, .range = range == WholeSize ? VK_WHOLE_SIZE : range};

    VkWriteDescriptorSet descriptorWrites{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                          .pNext = nullptr,
                                          .dstSet = getVkDescriptorSet(),
                                          .dstBinding = binding,
                                          .dstArrayElement = 0,
                                          .descriptorCount = 1u,
                                          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                          .pImageInfo = nullptr,
                                          .pBufferInfo = &bufferInfo,
                                          .pTexelBufferView = nullptr};

    vkUpdateDescriptorSets(vkDevice, 1u, &descriptorWrites, 0, nullptr);
}

void DescriptorSet::bindStorageBuffer(const uint32_t binding,
                                      snap::rhi::Buffer* buffer,
                                      const uint64_t offset,
                                      const uint64_t range) {
    auto* pBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Buffer>(buffer);
    setResource(binding, pBuffer);

    VkDescriptorBufferInfo bufferInfo{
        .buffer = pBuffer->getVkBuffer(), .offset = offset, .range = range == WholeSize ? VK_WHOLE_SIZE : range};

    VkWriteDescriptorSet descriptorWrites{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                          .pNext = nullptr,
                                          .dstSet = getVkDescriptorSet(),
                                          .dstBinding = binding,
                                          .dstArrayElement = 0,
                                          .descriptorCount = 1u,
                                          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                          .pImageInfo = nullptr,
                                          .pBufferInfo = &bufferInfo,
                                          .pTexelBufferView = nullptr};

    vkUpdateDescriptorSets(vkDevice, 1u, &descriptorWrites, 0, nullptr);
}

void DescriptorSet::bindTexture(const uint32_t binding, snap::rhi::Texture* texture) {
    auto* pTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(texture);
    tryPreserveInteropTexture(texture);
    setResource(binding, pTexture);

    VkDescriptorImageInfo imageInfo{.sampler = VK_NULL_HANDLE,
                                    .imageView = pTexture->getTexture(),
                                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    imagesExpectedLayout.emplace_back(std::make_pair(pTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

    VkWriteDescriptorSet descriptorWrites{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                          .pNext = nullptr,
                                          .dstSet = getVkDescriptorSet(),
                                          .dstBinding = binding,
                                          .dstArrayElement = 0,
                                          .descriptorCount = 1u,
                                          .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                          .pImageInfo = &imageInfo,
                                          .pBufferInfo = nullptr,
                                          .pTexelBufferView = nullptr};

    vkUpdateDescriptorSets(vkDevice, 1u, &descriptorWrites, 0, nullptr);
}

void DescriptorSet::bindStorageTexture(const uint32_t binding, snap::rhi::Texture* texture, uint32_t mipLevel) {
    auto* pTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(texture);
    tryPreserveInteropTexture(texture);
    setResource(binding, pTexture);

    const auto& viewCreateInfo = pTexture->getViewCreateInfo();
    snap::rhi::TextureSubresourceRange range = viewCreateInfo.viewInfo.range;
    range.baseMipLevel += mipLevel;
    range.levelCount = 1;
    const auto imageView = resolveImageView(pTexture, range);

    VkDescriptorImageInfo imageInfo{
        .sampler = VK_NULL_HANDLE, .imageView = imageView, .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
    imagesExpectedLayout.emplace_back(std::make_pair(pTexture, VK_IMAGE_LAYOUT_GENERAL));

    VkWriteDescriptorSet descriptorWrites{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                          .pNext = nullptr,
                                          .dstSet = getVkDescriptorSet(),
                                          .dstBinding = binding,
                                          .dstArrayElement = 0,
                                          .descriptorCount = 1u,
                                          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                          .pImageInfo = &imageInfo,
                                          .pBufferInfo = nullptr,
                                          .pTexelBufferView = nullptr};

    vkUpdateDescriptorSets(vkDevice, 1u, &descriptorWrites, 0, nullptr);
}

void DescriptorSet::bindSampler(const uint32_t binding, snap::rhi::Sampler* sampler) {
    auto* pSampler = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Sampler>(sampler);
    setResource(binding, pSampler);

    VkDescriptorImageInfo imageInfo{
        .sampler = pSampler->getSampler(), .imageView = VK_NULL_HANDLE, .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED};

    VkWriteDescriptorSet descriptorWrites{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                          .pNext = nullptr,
                                          .dstSet = getVkDescriptorSet(),
                                          .dstBinding = binding,
                                          .dstArrayElement = 0,
                                          .descriptorCount = 1u,
                                          .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                                          .pImageInfo = &imageInfo,
                                          .pBufferInfo = nullptr,
                                          .pTexelBufferView = nullptr};

    vkUpdateDescriptorSets(vkDevice, 1u, &descriptorWrites, 0, nullptr);
}

void DescriptorSet::updateDescriptorSet(std::span<snap::rhi::Descriptor> descriptorWrites) {
    std::vector<VkWriteDescriptorSet> nativeDescriptorWrites(descriptorWrites.size());
    std::vector<VkDescriptorImageInfo> imagesInfo(descriptorWrites.size());
    std::vector<VkDescriptorBufferInfo> buffersInfo(descriptorWrites.size());

    for (size_t i = 0; i < nativeDescriptorWrites.size(); ++i) {
        const auto& descInfo = descriptorWrites[i];
        nativeDescriptorWrites[i] = VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = getVkDescriptorSet(),
            .dstBinding = descInfo.binding,
            .dstArrayElement = 0,
            .descriptorCount = 1u,
            .descriptorType = snap::rhi::backend::vulkan::getDescriptorType(descInfo.descriptorType),
            .pImageInfo = nullptr,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr};

        switch (descInfo.descriptorType) {
            case snap::rhi::DescriptorType::Sampler: {
                auto* pSampler = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Sampler>(
                    descInfo.samplerInfo.sampler);
                setResource(descInfo.binding, pSampler);

                imagesInfo.emplace_back(VkDescriptorImageInfo{.sampler = pSampler->getSampler(),
                                                              .imageView = VK_NULL_HANDLE,
                                                              .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED});
                nativeDescriptorWrites[i].pImageInfo = &imagesInfo.back();
            } break;

            case snap::rhi::DescriptorType::SampledTexture: {
                auto* pTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(
                    descInfo.sampledTextureInfo.texture);
                setResource(descInfo.binding, pTexture);

                imagesInfo.emplace_back(VkDescriptorImageInfo{.sampler = VK_NULL_HANDLE,
                                                              .imageView = pTexture->getTexture(),
                                                              .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
                imagesExpectedLayout.emplace_back(std::make_pair(pTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
                nativeDescriptorWrites[i].pImageInfo = &imagesInfo.back();
            } break;

            case snap::rhi::DescriptorType::StorageTexture: {
                auto* pTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Texture>(
                    descInfo.storageTextureInfo.texture);
                setResource(descInfo.binding, pTexture);

                imagesInfo.emplace_back(VkDescriptorImageInfo{.sampler = VK_NULL_HANDLE,
                                                              .imageView = pTexture->getTexture(),
                                                              .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
                imagesExpectedLayout.emplace_back(std::make_pair(pTexture, VK_IMAGE_LAYOUT_GENERAL));
                nativeDescriptorWrites[i].pImageInfo = &imagesInfo.back();
            } break;

            case snap::rhi::DescriptorType::UniformBuffer:
            case snap::rhi::DescriptorType::StorageBuffer: {
                auto* pBuffer = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Buffer>(
                    descInfo.bufferInfo.buffer);
                setResource(descInfo.binding, pBuffer);

                buffersInfo.emplace_back(VkDescriptorBufferInfo{
                    .buffer = pBuffer->getVkBuffer(),
                    .offset = descInfo.bufferInfo.offset,
                    .range = descInfo.bufferInfo.range == WholeSize ? VK_WHOLE_SIZE : descInfo.bufferInfo.range});
                nativeDescriptorWrites[i].pBufferInfo = &buffersInfo.back();
            } break;

            default: {
                snap::rhi::common::throwException("[DescriptorSet::updateDescriptorSet] unexpected descriptorType");
            }
        }
    }

    vkUpdateDescriptorSets(
        vkDevice, static_cast<uint32_t>(nativeDescriptorWrites.size()), nativeDescriptorWrites.data(), 0, nullptr);
}

void DescriptorSet::bindToCommandBuffer(snap::rhi::backend::vulkan::CommandBuffer* commandBuffer) {
    collectReferences(commandBuffer);
    commandBuffer->preserveInteropTextures(getInteropTextures());

    { // Prepare all image layouts for the command buffer
        auto& imageLayoutManager = commandBuffer->getImageLayoutManager();

        for (const auto& [texture, expectedLayout] : imagesExpectedLayout) {
            const auto& createInfo = texture->getCreateInfo();
            imageLayoutManager.transferLayout(
                texture,
                {.aspectMask = getImageAspectFlags(createInfo.format),
                 .baseMipLevel = 0,
                 .levelCount = createInfo.mipLevels,
                 .baseArrayLayer = 0,
                 // Use getArraySize since for 2D textures layers have to be handled separately (e.g. cubemap should
                 // return 6 * depth) while 3D texture must only have 1 layer
                 .layerCount = getArraySize(createInfo.textureType, createInfo.size)},
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
                expectedLayout);
        }

        imagesExpectedLayout.clear();
    }
}

void DescriptorSet::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    VkDescriptorSet ds = getVkDescriptorSet();
    if (ds != VK_NULL_HANDLE) {
        setVkObjectDebugName(vkDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(ds), label.data());
    }
#endif
}
} // namespace snap::rhi::backend::vulkan
