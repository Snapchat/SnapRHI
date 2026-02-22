//
//  DescriptorSet.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 8/15/22.
//

#pragma once

#include "snap/rhi/DescriptorSetCreateInfo.h"

#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/backend/common/DescriptorSet.h"
#include "snap/rhi/backend/common/ResourceResidencySet.h"

namespace snap::rhi::backend::opengl {
class Device;
class Profile;
class Buffer;
class Texture;
class Sampler;

struct DescriptorData : public snap::rhi::backend::common::DescriptorData {
    uint64_t offset = 0;
    uint64_t range = 0;
    uint32_t mipLevel = 0;
};

class DescriptorSet final : public snap::rhi::backend::common::DescriptorSet<DescriptorData> {
public:
    DescriptorSet(snap::rhi::backend::opengl::Device* device, const DescriptorSetCreateInfo& info);
    ~DescriptorSet() override = default;

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

private:
};
} // namespace snap::rhi::backend::opengl
