//
//  TextureCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/PixelFormat.h"
#include "snap/rhi/Structs.h"
#include "snap/rhi/common/HashCombine.h"
#include <compare>

namespace snap::rhi {
// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkComponentMapping.html

/**
 * @brief Component swizzle mapping for a texture view.
 *
 * Allows remapping texture channels (R/G/B/A) when sampling.
 *
 * Backend notes:
 * - Vulkan supports component swizzles via image views.
 * - OpenGL supports swizzles via `GL_TEXTURE_SWIZZLE_*` when available.
 * - Metal supports swizzles on newer OS versions; support may be limited.
 */
struct ComponentMapping {
    /// Mapping for the red component.
    ComponentSwizzle r = ComponentSwizzle::R;
    /// Mapping for the green component.
    ComponentSwizzle g = ComponentSwizzle::G;
    /// Mapping for the blue component.
    ComponentSwizzle b = ComponentSwizzle::B;
    /// Mapping for the alpha component.
    ComponentSwizzle a = ComponentSwizzle::A;

    /**
     * @brief Three-way comparison.
     */
    constexpr friend auto operator<=>(const ComponentMapping&, const ComponentMapping&) noexcept = default;
};

/**
 * @brief Default component mapping (identity swizzle: RGBA).
 */
constexpr ComponentMapping DefaultComponentMapping{
    ComponentSwizzle::R, ComponentSwizzle::G, ComponentSwizzle::B, ComponentSwizzle::A};

/**
 * @brief Texture creation parameters.
 *
 * Describes a texture resource (dimensions, type, usage, format, mip levels and MSAA sample count).
 *
 * Backend notes / constraints:
 * - Cubemap textures require @ref size.width == @ref size.height.
 * - Multisampled textures generally do not support multiple mip levels.
 * - Certain backends restrict MSAA to 2D textures (and 2D arrays depending on platform).
 */
struct TextureCreateInfo {
    /**
     * @brief Texture dimensions.
     *
     * For 2D textures, depth is typically 1.
     * For 2D array textures, depth is the array layer count.
     * For 3D textures, depth is the Z dimension.
     *
     * @note By definition, the width and height of a cube texture are the same value.
     */
    Extent3D size{};

    /**
     * @brief Number of mipmap levels.
     */
    uint32_t mipLevels = 1;

    /// Texture dimensionality/type.
    TextureType textureType = TextureType::Texture2D;

    /// Usage flags defining how the texture will be used (sampled, attachment, storage, etc.).
    TextureUsage textureUsage = TextureUsage::None;

    /**
     * @brief Sample count for MSAA.
     *
     * `snap::rhi::SampleCount::Count1` means MSAA is disabled.
     *
     * @note Multisampled textures typically do not support multiple mip levels.
     */
    SampleCount sampleCount = SampleCount::Undefined;

    /// Pixel format of the texture.
    PixelFormat format = PixelFormat::Undefined;

    /**
     * @brief Component swizzle mapping.
     *
     * Used primarily for view-based channel remapping.
     */
    ComponentMapping components = DefaultComponentMapping;

    constexpr friend auto operator<=>(const TextureCreateInfo&, const TextureCreateInfo&) noexcept = default;
};
} // namespace snap::rhi

namespace std {
template<>
struct hash<snap::rhi::TextureCreateInfo> {
    /**
     * @brief Computes a hash for a texture configuration.
     *
     * Intended for use in caches and associative containers.
     */
    size_t operator()(const snap::rhi::TextureCreateInfo& textureInfo) const noexcept {
        return snap::rhi::common::hash_combine(textureInfo.size.width,
                                               textureInfo.size.height,
                                               textureInfo.size.depth,
                                               textureInfo.mipLevels,
                                               static_cast<uint32_t>(textureInfo.components.r),
                                               static_cast<uint32_t>(textureInfo.components.g),
                                               static_cast<uint32_t>(textureInfo.components.b),
                                               static_cast<uint32_t>(textureInfo.components.a),
                                               static_cast<uint32_t>(textureInfo.textureType),
                                               static_cast<uint32_t>(textureInfo.textureUsage),
                                               static_cast<uint32_t>(textureInfo.sampleCount),
                                               static_cast<uint32_t>(textureInfo.format));
    }
};
} // namespace std
