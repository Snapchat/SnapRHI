#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/common/HashCombine.h"
#include <array>
#include <bit>
#include <cstdint>

namespace snap::rhi {
/**
 * @brief Default maximum LOD clamp used by `snap::rhi::SamplerCreateInfo`.
 *
 * This is used when the application does not specify an explicit maximum LOD. Some backends treat very large values as
 * "no clamp" (for example Vulkan may map this to `VK_LOD_CLAMP_NONE`).
 */
constexpr float DefaultSamplerLodMax = 1000.0f;

/**
 * @brief Sampler creation parameters.
 *
 * A sampler describes how textures are sampled: filtering, wrapping/addressing, LOD range and optional features such
 * as anisotropy and comparison sampling.
 *
 * Backend notes:
 * - Vulkan/Metal typically create a native sampler object.
 * - OpenGL may create a sampler object when supported, otherwise equivalent state may be applied via texture
 *   parameters at bind time.
 */
struct SamplerCreateInfo {
    /// Minification filter (when the texture is minified).
    SamplerMinMagFilter minFilter = SamplerMinMagFilter::Nearest;
    /// Magnification filter (when the texture is magnified).
    SamplerMinMagFilter magFilter = SamplerMinMagFilter::Nearest;
    /// Mipmap sampling mode.
    SamplerMipFilter mipFilter = SamplerMipFilter::NotMipmapped;

    /// Addressing mode for the U texture coordinate.
    WrapMode wrapU = WrapMode::ClampToEdge;
    /// Addressing mode for the V texture coordinate.
    WrapMode wrapV = WrapMode::ClampToEdge;
    /// Addressing mode for the W texture coordinate (relevant for 3D textures).
    WrapMode wrapW = WrapMode::ClampToEdge;

    /**
     * @brief Enables anisotropic filtering.
     *
     * If enabled, @ref maxAnisotropy specifies the maximum anisotropy level.
     *
     * @note Support is backend/device dependent.
     */
    bool anisotropyEnable = false;

    /// Maximum anisotropy level (only used when @ref anisotropyEnable is true).
    AnisotropicFiltering maxAnisotropy = AnisotropicFiltering::Count1;

    /**
     * @brief Enables comparison sampling.
     *
     * This is typically used for shadow map sampling.
     */
    bool compareEnable = false;

    /// Comparison function used when @ref compareEnable is true.
    CompareFunction compareFunction = CompareFunction::Always;

    /// Minimum LOD clamp.
    float lodMin = 0.0f;
    /// Maximum LOD clamp.
    float lodMax = DefaultSamplerLodMax;

    /**
     * @brief Border color used with clamp-to-border addressing modes.
     *
     * Some backends only support a subset of border colors.
     */
    SamplerBorderColor borderColor = SamplerBorderColor::TransparentBlack;

    /**
     * @brief Enables unnormalized texture coordinates.
     *
     * When true, texture coordinates are interpreted in texel space rather than normalized [0,1] space.
     *
     * @note Support is backend/device dependent.
     */
    bool unnormalizedCoordinates = false;
};
} // namespace snap::rhi

namespace std {
/**
 * @brief Hash specialization for `snap::rhi::SamplerCreateInfo`.
 */
template<>
struct hash<snap::rhi::SamplerCreateInfo> {
    /**
     * @brief Computes a hash value for a sampler configuration.
     *
     * This is intended for cache keys (for example pipeline/descriptor state caches). The hash includes all fields of
     * `SamplerCreateInfo`.
     */
    size_t operator()(const snap::rhi::SamplerCreateInfo& sampler) const noexcept {
        auto hashFloat = [](float v) -> uint32_t {
            // Ignore float value due to precision issues.
            return 0;
        };

        return snap::rhi::common::hash_combine(static_cast<uint32_t>(sampler.mipFilter),
                                               static_cast<uint32_t>(sampler.minFilter),
                                               static_cast<uint32_t>(sampler.magFilter),
                                               static_cast<uint32_t>(sampler.wrapU),
                                               static_cast<uint32_t>(sampler.wrapV),
                                               static_cast<uint32_t>(sampler.wrapW),
                                               static_cast<uint32_t>(sampler.anisotropyEnable),
                                               static_cast<uint32_t>(sampler.maxAnisotropy),
                                               static_cast<uint32_t>(sampler.compareEnable),
                                               static_cast<uint32_t>(sampler.compareFunction),
                                               hashFloat(sampler.lodMin),
                                               hashFloat(sampler.lodMax),
                                               static_cast<uint32_t>(sampler.borderColor),
                                               static_cast<uint32_t>(sampler.unnormalizedCoordinates));
    }
};
} // namespace std
