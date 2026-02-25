#include "snap/rhi/backend/noop/DescriptorSet.hpp"

#include "snap/rhi/backend/noop/Device.hpp"

#include "snap/rhi/Buffer.hpp"

namespace snap::rhi::backend::noop {
DescriptorSet::DescriptorSet(snap::rhi::backend::common::Device* device, const DescriptorSetCreateInfo& info)
    : snap::rhi::DescriptorSet(device, info), validationLayer(device->getValidationLayer()) {}

void DescriptorSet::reset() {}

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
}

void DescriptorSet::bindStorageTexture(const uint32_t binding, snap::rhi::Texture* texture, uint32_t mipLevel) {
    SNAP_RHI_VALIDATE(validationLayer,
                      (texture->getCreateInfo().textureUsage & snap::rhi::TextureUsage::Storage) ==
                          snap::rhi::TextureUsage::Storage,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::DescriptorSetOp,
                      "[DescriptorSet::bindStorageTexture] textureUsage should contain TextureUsage::Storage bit.");
}

void DescriptorSet::bindSampler(const uint32_t binding, snap::rhi::Sampler* sampler) {}

void DescriptorSet::updateDescriptorSet(std::span<snap::rhi::Descriptor> descriptorWrites) {}
} // namespace snap::rhi::backend::noop
