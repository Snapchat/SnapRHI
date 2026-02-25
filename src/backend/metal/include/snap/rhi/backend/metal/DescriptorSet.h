#pragma once

#include "snap/rhi/backend/common/DescriptorSet.h"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/metal/Context.h"
#include "snap/rhi/backend/metal/DescriptorPool.h"

#include <Metal/Metal.h>
#include <cstdint>
#include <unordered_map>

namespace snap::rhi::backend::metal {
class Device;
class Buffer;
class Texture;
class Sampler;
class DescriptorSetLayout;
class DescriptorPool;
class RenderPipeline;
class CommandBuffer;

class DescriptorSet final : public snap::rhi::backend::common::DescriptorSet<> {
public:
    DescriptorSet(Device* device, const snap::rhi::DescriptorSetCreateInfo& info);
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
    void setDebugLabel(std::string_view label) override;

    void useResources(Context& context);

    [[nodiscard]] const DescriptorPool::BufferSubRange& getArgumentBufferSubRange() const {
        return argumentBufferSubRange;
    }

private:
    SNAP_RHI_NO_INLINE void bindTextureSlow(const id<MTLArgumentEncoder>& argumentEncoder,
                                            uint32_t binding,
                                            snap::rhi::Texture* texture);
    SNAP_RHI_NO_INLINE void bindSamplerSlow(const id<MTLArgumentEncoder>& argumentEncoder,
                                            uint32_t binding,
                                            snap::rhi::Sampler* sampler);
    SNAP_RHI_NO_INLINE void bindBufferSlow(const id<MTLArgumentEncoder>& argumentEncoder,
                                           uint32_t binding,
                                           snap::rhi::Buffer* buffer,
                                           const uint32_t offset);

    SNAP_RHI_ALWAYS_INLINE void setResource(const uint32_t binding,
                                            snap::rhi::DeviceChild* resource,
                                            const uint64_t gpuAddress = 0) {
        size_t idx = InvalidIdx;
        if (binding < BindingToIdxMaxSize) {
            idx = bindingToIdx[binding];
        } else {
            // Only call the complex helper if absolutely necessary
            idx = getIdxFromBinding(binding);
        }

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
        SNAP_RHI_VALIDATE(validationLayer,
                          idx != InvalidIdx,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::DescriptorSetOp,
                          "[DescriptorSet::setResource] invalid binding %u.",
                          binding);
#endif

        descriptors[idx].resourcePtr = resource;
        if (argumentBufferData) {
            argumentBufferData[idx] = gpuAddress;
        }
    }

    struct CachedMetalInfo {
        MTLResourceUsage usage;
        MTLRenderStages stages;
    };
    std::vector<CachedMetalInfo> cachedInfos;

    id<MTLArgumentEncoder> argumentEncoder = nil;
    uint64_t* argumentBufferData = nullptr;
    DescriptorPool::BufferSubRange argumentBufferSubRange = nullptr;
};
} // namespace snap::rhi::backend::metal
