#pragma once

#include "snap/rhi/BufferCreateInfo.h"
#include "snap/rhi/DescriptorSetLayoutCreateInfo.h"
#include "snap/rhi/RenderPipelineCreateInfo.h"
#include "snap/rhi/SamplerCreateInfo.h"
#include "snap/rhi/TextureCreateInfo.h"

#include <cmath>
#include <functional>

namespace snap::rhi {
constexpr float Epsilon = float(1e-6);

template<class T>
inline bool epsilonEqual(const T& val1, const T& val2, const T& eps = T(Epsilon)) {
    static_assert(std::is_floating_point_v<T>, "only for floating point types");

    return std::abs(val1 - val2) <= eps;
}

constexpr bool operator==(const snap::rhi::ColorBlendStateCreateInfo& lhs,
                          const snap::rhi::ColorBlendStateCreateInfo& rhs) noexcept {
    if (lhs.colorAttachmentsCount != rhs.colorAttachmentsCount)
        return false;

    for (uint32_t i = 0; i < lhs.colorAttachmentsCount; ++i) {
        if (!(lhs.colorAttachmentsBlendState[i] == rhs.colorAttachmentsBlendState[i])) {
            return false;
        }
    }
    return true;
}

constexpr bool operator==(const snap::rhi::VertexInputStateCreateInfo& lhs,
                          const snap::rhi::VertexInputStateCreateInfo& rhs) noexcept {
    if (lhs.bindingsCount != rhs.bindingsCount)
        return false;
    if (lhs.attributesCount != rhs.attributesCount)
        return false;

    for (uint32_t i = 0; i < lhs.attributesCount; ++i) {
        if (!(lhs.attributeDescription[i] == rhs.attributeDescription[i])) {
            return false;
        }
    }

    for (uint32_t i = 0; i < lhs.bindingsCount; ++i) {
        if (!(lhs.bindingDescription[i] == rhs.bindingDescription[i])) {
            return false;
        }
    }
    return true;
}
} // namespace snap::rhi

namespace std {
template<>
struct equal_to<snap::rhi::SamplerCreateInfo> {
    constexpr bool operator()(const snap::rhi::SamplerCreateInfo& lhs,
                              const snap::rhi::SamplerCreateInfo& rhs) const noexcept {
        return lhs.mipFilter == rhs.mipFilter && lhs.minFilter == rhs.minFilter && lhs.magFilter == rhs.magFilter &&
               lhs.compareFunction == rhs.compareFunction && lhs.maxAnisotropy == rhs.maxAnisotropy &&
               lhs.wrapU == rhs.wrapU && lhs.wrapV == rhs.wrapV && lhs.wrapW == rhs.wrapW &&
               snap::rhi::epsilonEqual(lhs.lodMin, lhs.lodMin) && snap::rhi::epsilonEqual(lhs.lodMax, lhs.lodMax) &&
               lhs.borderColor == rhs.borderColor && lhs.unnormalizedCoordinates == rhs.unnormalizedCoordinates &&
               lhs.anisotropyEnable == rhs.anisotropyEnable && lhs.compareEnable == rhs.compareEnable;
    }
};

template<>
struct equal_to<snap::rhi::RenderPipelineCreateInfo> {
    constexpr bool operator()(const snap::rhi::RenderPipelineCreateInfo& lhs,
                              const snap::rhi::RenderPipelineCreateInfo& rhs) const noexcept {
        return lhs.pipelineCreateFlags == rhs.pipelineCreateFlags && lhs.stages == rhs.stages &&
               lhs.renderPass == rhs.renderPass && lhs.pipelineLayout == rhs.pipelineLayout &&
               lhs.subpass == rhs.subpass && lhs.basePipeline == rhs.basePipeline &&
               lhs.pipelineCache == rhs.pipelineCache && lhs.vertexInputState == rhs.vertexInputState &&
               lhs.inputAssemblyState == rhs.inputAssemblyState && lhs.rasterizationState == rhs.rasterizationState &&
               lhs.multisampleState == rhs.multisampleState && lhs.depthStencilState == rhs.depthStencilState &&
               lhs.colorBlendState == rhs.colorBlendState;
    }
};

template<>
struct equal_to<snap::rhi::DescriptorSetLayoutCreateInfo> {
    constexpr bool operator()(const snap::rhi::DescriptorSetLayoutCreateInfo& lhs,
                              const snap::rhi::DescriptorSetLayoutCreateInfo& rhs) const noexcept {
        return lhs.bindings == rhs.bindings;
    }
};
} // namespace std
