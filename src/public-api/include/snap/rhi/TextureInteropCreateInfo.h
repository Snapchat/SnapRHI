#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/PixelFormat.h"
#include "snap/rhi/Structs.h"
#include <compare>

namespace snap::rhi {

/**
 * @brief Parameters describing a platform/external texture used for interoperability.
 *
 * This structure describes the minimal, API-agnostic properties required to wrap a native/external texture into
 * `snap::rhi::TextureInterop`.
 *
 * How it is used:
 * - Backends use this information to derive an internal `snap::rhi::TextureCreateInfo` (for example, setting a 2D
 * texture with `mipLevels = 1`, `sampleCount = Count1`, and the specified format/usage).
 * - The resulting `TextureCreateInfo` is then used when an `snap::rhi::Texture` is created from the interop object.
 *
 * @note This create info intentionally does not include API-specific handles; those are owned by backend-specific
 *       `TextureInterop` implementations.
 */
struct TextureInteropCreateInfo {
    /**
     * @brief Texture size in pixels (width, height, depth).
     *
     * Interop textures created with this create info are treated as 3D textures by the public API.
     */
    Extent3D size{};

    /**
     * @brief Intended usage flags for the texture.
     *
     * These flags determine how the wrapped texture will be used by SnapRHI (sampling, attachment usage, storage
     * access, etc.). Support may be backend dependent.
     */
    TextureUsage textureUsage = TextureUsage::None;

    /**
     * @brief Pixel format of the external texture.
     */
    PixelFormat format = PixelFormat::Undefined;
    TextureType textureType = TextureType::Texture2D;

    constexpr friend auto operator<=>(const TextureInteropCreateInfo&,
                                      const TextureInteropCreateInfo&) noexcept = default;
};
} // namespace snap::rhi
