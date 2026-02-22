//
//  TextureViewCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 05.05.2021.
//

#pragma once

#include "snap/rhi/Structs.h"
#include "snap/rhi/TextureCreateInfo.h"

namespace snap::rhi {
class Texture;

/**
 * @brief Parameters controlling a texture view properties.
 *
 * A texture view is a reinterpretation of an existing texture resource with an optional format override, subresource
 * range (mip levels / array layers), and component swizzle mapping.
 *
 * Backend notes:
 * - Vulkan can create views via image views; some view settings may be limited by the underlying image format and
 *   usage.
 * - Metal supports texture views, but some features (e.g., component swizzle) may require newer OS versions.
 *
 */
struct TextureViewInfo {
    /**
     * @brief View format.
     *
     * This may differ from the underlying texture format when the backend allows compatible reinterpretation.
     */
    PixelFormat format = PixelFormat::R8G8B8A8Unorm;

    /**
     * @brief Texture view type.
     *
     * This controls whether the view is treated as 2D/2D-array/3D/cubemap, etc.
     */
    TextureType textureType = TextureType::Texture2D;

    /**
     * @brief Subresource range covered by this view.
     *
     * Selects the mip levels and array layers that the view exposes.
     */
    TextureSubresourceRange range{};

    /**
     * @brief Component swizzle mapping used when sampling from this view.
     *
     * @note Backend support varies. For example, Vulkan view swizzles may be restricted or currently treated as
     * identity by the backend, and Metal swizzles may require newer OS versions.
     */
    ComponentMapping components = DefaultComponentMapping;

    constexpr friend auto operator<=>(const TextureViewInfo&, const TextureViewInfo&) noexcept = default;
};

/**
 * @brief Parameters controlling creation of a texture view.
 *
 * @note The source @texture must remain alive for the lifetime of the created view.
 */
struct TextureViewCreateInfo {
    TextureViewInfo viewInfo{};

    /**
     * @brief Source texture to create the view from.
     */
    Texture* texture = nullptr;

    constexpr friend auto operator<=>(const TextureViewCreateInfo&, const TextureViewCreateInfo&) noexcept = default;
};
} // namespace snap::rhi

namespace std {
template<>
struct hash<snap::rhi::TextureViewInfo> {
    size_t operator()(const snap::rhi::TextureViewInfo& textureInfo) const noexcept {
        return snap::rhi::common::hash_combine(textureInfo.format,
                                               static_cast<uint32_t>(textureInfo.textureType),
                                               textureInfo.range.baseMipLevel,
                                               textureInfo.range.levelCount,
                                               textureInfo.range.baseArrayLayer,
                                               textureInfo.range.layerCount,
                                               static_cast<uint32_t>(textureInfo.components.r),
                                               static_cast<uint32_t>(textureInfo.components.g),
                                               static_cast<uint32_t>(textureInfo.components.b),
                                               static_cast<uint32_t>(textureInfo.components.a));
    }
};

template<>
struct hash<snap::rhi::TextureViewCreateInfo> {
    size_t operator()(const snap::rhi::TextureViewCreateInfo& textureInfo) const noexcept {
        return snap::rhi::common::hash_combine(textureInfo.texture,
                                               hash<snap::rhi::TextureViewInfo>()(textureInfo.viewInfo));
    }
};
} // namespace std
