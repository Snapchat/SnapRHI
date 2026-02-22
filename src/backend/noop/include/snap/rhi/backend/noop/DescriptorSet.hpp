#pragma once

#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/DescriptorSetCreateInfo.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class DescriptorSet final : public snap::rhi::DescriptorSet {
public:
    DescriptorSet(snap::rhi::backend::common::Device* device, const DescriptorSetCreateInfo& info);
    ~DescriptorSet() override = default;

    void reset() override;

    void bindUniformBuffer(const uint32_t binding,
                           snap::rhi::Buffer* buffer,
                           const uint64_t offset,
                           const uint64_t range) override;
    void bindStorageBuffer(const uint32_t binding,
                           snap::rhi::Buffer* buffer,
                           const uint64_t offset,
                           const uint64_t range) override;

    void bindTexture(const uint32_t binding, snap::rhi::Texture* texture) override;
    void bindStorageTexture(const uint32_t binding, snap::rhi::Texture* texture, uint32_t mipLevel) override;

    void bindSampler(const uint32_t binding, snap::rhi::Sampler* sampler) override;
    void updateDescriptorSet(std::span<snap::rhi::Descriptor> descriptorWrites) override;

private:
    const snap::rhi::backend::common::ValidationLayer& validationLayer;
};
} // namespace snap::rhi::backend::noop
