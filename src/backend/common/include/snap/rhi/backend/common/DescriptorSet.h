#pragma once

#include "snap/rhi/DescriptorPool.hpp"
#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/DescriptorSetLayout.hpp"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Device.hpp"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include <algorithm>
#include <functional>

namespace snap::rhi::backend::common {
struct DescriptorData {
    snap::rhi::DeviceChild* resourcePtr = nullptr;
};

template<typename T>
concept DescriptorDataBased = std::derived_from<T, DescriptorData>;

template<DescriptorDataBased T = DescriptorData>
class DescriptorSet : public snap::rhi::DescriptorSet {
public:
    DescriptorSet(common::Device* device, const snap::rhi::DescriptorSetCreateInfo& info)
        : snap::rhi::DescriptorSet(device, info),
          setResourceResidencySet(device, ResourceResidencySet::RetentionMode::RetainReferences),
          validationLayer(device->getValidationLayer()) {
        setResourceResidencySet.track(info.descriptorSetLayout);

        const auto& layoutInfo = info.descriptorSetLayout->getCreateInfo();
        descriptors.resize(layoutInfo.bindings.size());

        bindingToIdx.fill(InvalidIdx);
        for (size_t idx = 0; idx < layoutInfo.bindings.size(); ++idx) {
            if (layoutInfo.bindings[idx].binding < BindingToIdxMaxSize) {
                bindingToIdx[layoutInfo.bindings[idx].binding] = idx;
            }
        }
    }

    ~DescriptorSet() override = default;

    void reset() override {
        interopTextures.clear();
        std::ranges::fill(descriptors, T{});
    }

    void updateDescriptorSet(const std::span<snap::rhi::Descriptor> descriptorWrites) override {
        for (const auto& descriptorInfo : descriptorWrites) {
            switch (descriptorInfo.descriptorType) {
                case snap::rhi::DescriptorType::Sampler: {
                    bindSampler(descriptorInfo.binding, descriptorInfo.samplerInfo.sampler);
                } break;

                case snap::rhi::DescriptorType::SampledTexture: {
                    bindTexture(descriptorInfo.binding, descriptorInfo.sampledTextureInfo.texture);
                } break;

                case snap::rhi::DescriptorType::StorageTexture: {
                    bindStorageTexture(descriptorInfo.binding,
                                       descriptorInfo.storageTextureInfo.texture,
                                       descriptorInfo.storageTextureInfo.mipLevel);
                } break;

                case snap::rhi::DescriptorType::UniformBuffer:
                case snap::rhi::DescriptorType::UniformBufferDynamic: {
                    bindUniformBuffer(descriptorInfo.binding,
                                      descriptorInfo.bufferInfo.buffer,
                                      descriptorInfo.bufferInfo.offset,
                                      descriptorInfo.bufferInfo.range);
                } break;

                case snap::rhi::DescriptorType::StorageBuffer:
                case snap::rhi::DescriptorType::StorageBufferDynamic: {
                    bindStorageBuffer(descriptorInfo.binding,
                                      descriptorInfo.bufferInfo.buffer,
                                      descriptorInfo.bufferInfo.offset,
                                      descriptorInfo.bufferInfo.range);
                } break;

                default: {
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
                    SNAP_RHI_REPORT(validationLayer,
                                    snap::rhi::ReportLevel::Error,
                                    snap::rhi::ValidationTag::DescriptorSetOp,
                                    "[DescriptorSet::updateDescriptorSet] unsupported descriptor.descriptorType{%d}",
                                    descriptorInfo.descriptorType);
#endif
                } break;
            }
        }
    }

    SNAP_RHI_ALWAYS_INLINE void tryPreserveInteropTexture(snap::rhi::Texture* texture) noexcept {
        if (!texture) {
            return;
        }

        const auto& textureInterop = texture->getTextureInterop();
        if (textureInterop) {
            interopTextures.push_back(textureInterop.get());
        }
    }

    SNAP_RHI_ALWAYS_INLINE const auto& getInteropTextures() const noexcept {
        return interopTextures;
    }

    /**
     * @brief Iterates over all bound resources and registers them with the residency set.
     * This ensures resources used by this set are kept alive during GPU execution.
     * Also track the descriptor set itself.
     */
    SNAP_RHI_ALWAYS_INLINE void collectReferences(snap::rhi::backend::common::CommandBuffer* commandBuffer) {
        const auto& commandBufferCreateInfo = commandBuffer->getCreateInfo();
        if (!SNAP_RHI_ENABLE_SLOW_VALIDATIONS() &&
            static_cast<bool>(commandBufferCreateInfo.commandBufferCreateFlags &
                              snap::rhi::CommandBufferCreateFlags::UnretainedResources)) {
            return;
        }

        {
            auto& resourceResidencySet = commandBuffer->getResourceResidencySet();
            for (size_t i = 0; i < descriptors.size(); ++i) {
                const auto& descriptor = descriptors[i];
#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
                const auto& layoutInfo = info.descriptorSetLayout->getCreateInfo();
                SNAP_RHI_VALIDATE(validationLayer,
                                  descriptor.resourcePtr,
                                  snap::rhi::ReportLevel::Warning,
                                  snap::rhi::ValidationTag::DescriptorSetOp,
                                  "[DescriptorSet::collectReferences] Descriptor at {binding: %d, type: %d} is not "
                                  "bound to a resource.",
                                  layoutInfo.bindings[i].binding,
                                  static_cast<int32_t>(layoutInfo.bindings[i].descriptorType));
#endif
                if (descriptor.resourcePtr) {
                    resourceResidencySet.track(descriptor.resourcePtr);
                }
            }

            resourceResidencySet.track(this);
        }

        {
            /**
             * DescriptorPool must be retained in a separate ResourceResidencySet,
             * since DescriptorSet have to be released before DescriptorPool
             */
            auto& descriptorPoolResourceResidencySet = commandBuffer->getDescriptorPoolResourceResidencySet();
            descriptorPoolResourceResidencySet.track(info.descriptorPool);
        }
    }

    void for_each(const std::function<void(const DescriptorSetLayoutBinding&, const T&)>& func) {
        const auto& layoutInfo = info.descriptorSetLayout->getCreateInfo();
        const auto& descriptorInfos = layoutInfo.bindings;

        for (size_t i = 0; i < descriptors.size(); ++i) {
            func(descriptorInfos[i], descriptors[i]);
        }
    }

protected:
    SNAP_RHI_ALWAYS_INLINE size_t getIdxFromBinding(const uint32_t binding) const {
        if (binding < BindingToIdxMaxSize) {
            return bindingToIdx[binding];
        }

        const auto& layoutInfo = info.descriptorSetLayout->getCreateInfo();
        const auto& descriptorInfos = layoutInfo.bindings;

        auto itr = std::lower_bound(descriptorInfos.begin(),
                                    descriptorInfos.end(),
                                    binding,
                                    [](const auto& info, uint32_t binding) { return info.binding < binding; });
        if (itr == descriptorInfos.end()) {
            return InvalidIdx;
        }

        if (itr->binding != binding) {
            return InvalidIdx;
        }

        return itr - descriptorInfos.begin();
    }

    static constexpr uint32_t BindingToIdxMaxSize = 256u;
    static constexpr size_t InvalidIdx = std::numeric_limits<size_t>::max();
    std::array<size_t, BindingToIdxMaxSize> bindingToIdx{};
    std::vector<T> descriptors;
    std::vector<snap::rhi::TextureInterop*> interopTextures;
    ResourceResidencySet setResourceResidencySet;

    const snap::rhi::backend::common::ValidationLayer& validationLayer;
};
} // namespace snap::rhi::backend::common
