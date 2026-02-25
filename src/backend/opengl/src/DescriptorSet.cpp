#include "snap/rhi/backend/opengl/DescriptorSet.hpp"

#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Buffer.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Texture.hpp"

#include <algorithm>

namespace snap::rhi::backend::opengl {
DescriptorSet::DescriptorSet(snap::rhi::backend::opengl::Device* device, const DescriptorSetCreateInfo& info)
    : snap::rhi::backend::common::DescriptorSet<DescriptorData>(device, info) {}

void DescriptorSet::bindUniformBuffer(const uint32_t binding,
                                      snap::rhi::Buffer* buffer,
                                      const uint64_t offset,
                                      const uint64_t range) {
    SNAP_RHI_VALIDATE(validationLayer,
                      offset % device->getCapabilities().minUniformBufferOffsetAlignment == 0,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::bindUniformBuffer] wrong @offset alignment.");

    const auto& bufferInfo = buffer->getCreateInfo();
    SNAP_RHI_VALIDATE(validationLayer,
                      (bufferInfo.bufferUsage & snap::rhi::BufferUsage::UniformBuffer) ==
                          snap::rhi::BufferUsage::UniformBuffer,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::bindUniformBuffer] buffer must have buffer usage UniformBuffer");

    size_t idx = getIdxFromBinding(binding);
    if (idx == InvalidIdx) {
        return;
    }

    auto& descriptor = descriptors[idx];
    assert(info.descriptorSetLayout->getCreateInfo().bindings[idx].descriptorType ==
           snap::rhi::DescriptorType::UniformBuffer);

    descriptor.resourcePtr = buffer;
    descriptor.offset = offset;
    descriptor.range = range;
}

void DescriptorSet::bindStorageBuffer(const uint32_t binding,
                                      snap::rhi::Buffer* buffer,
                                      const uint64_t offset,
                                      const uint64_t range) {
    const auto& bufferInfo = buffer->getCreateInfo();
    SNAP_RHI_VALIDATE(validationLayer,
                      (bufferInfo.bufferUsage & snap::rhi::BufferUsage::StorageBuffer) ==
                          snap::rhi::BufferUsage::StorageBuffer,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::bindStorageBuffer] buffer must have buffer usage StorageBuffer");
    SNAP_RHI_VALIDATE(validationLayer,
                      offset < bufferInfo.size,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::bindStorageBuffer] offset must be lower than buffer size");

    size_t idx = getIdxFromBinding(binding);
    if (idx == InvalidIdx) {
        return;
    }

    auto& descriptor = descriptors[idx];
    assert(info.descriptorSetLayout->getCreateInfo().bindings[idx].descriptorType ==
           snap::rhi::DescriptorType::StorageBuffer);

    descriptor.resourcePtr = buffer;
    descriptor.offset = offset;
    descriptor.range = range;
}

void DescriptorSet::bindTexture(const uint32_t binding, snap::rhi::Texture* texture) {
    SNAP_RHI_VALIDATE(validationLayer,
                      texture,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::bindTexture] texture is nullptr.");

    SNAP_RHI_VALIDATE(validationLayer,
                      (texture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::Sampled) ==
                          snap::rhi::TextureUsage::Sampled,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::bindTexture] textureUsage should contain TextureUsage::Sampled bit.");
    tryPreserveInteropTexture(texture);

    size_t idx = getIdxFromBinding(binding);
    if (idx == InvalidIdx) {
        return;
    }

    auto& descriptor = descriptors[idx];
    assert(info.descriptorSetLayout->getCreateInfo().bindings[idx].descriptorType ==
           snap::rhi::DescriptorType::SampledTexture);

    descriptor.resourcePtr = texture;
}

void DescriptorSet::bindStorageTexture(const uint32_t binding, snap::rhi::Texture* texture, uint32_t mipLevel) {
    SNAP_RHI_VALIDATE(validationLayer,
                      (texture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::Storage) ==
                          snap::rhi::TextureUsage::Storage,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::bindStorageTexture] textureUsage should contain TextureUsage::Storage bit.");
    tryPreserveInteropTexture(texture);

    size_t idx = getIdxFromBinding(binding);
    if (idx == InvalidIdx) {
        return;
    }

    auto& descriptor = descriptors[idx];
    assert(info.descriptorSetLayout->getCreateInfo().bindings[idx].descriptorType ==
           snap::rhi::DescriptorType::StorageTexture);

    descriptor.resourcePtr = texture;
    descriptor.mipLevel = mipLevel;
}

void DescriptorSet::bindSampler(const uint32_t binding, snap::rhi::Sampler* sampler) {
    size_t idx = getIdxFromBinding(binding);
    if (idx == InvalidIdx) {
        return;
    }

    auto& descriptor = descriptors[idx];
    assert(info.descriptorSetLayout->getCreateInfo().bindings[idx].descriptorType ==
           snap::rhi::DescriptorType::Sampler);

    descriptor.resourcePtr = sampler;
}
} // namespace snap::rhi::backend::opengl
