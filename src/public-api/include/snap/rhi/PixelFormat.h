//
//  PixelFormat.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 22.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <cstdint>

namespace snap::rhi {

/**
 * @brief Describes the texel storage format for textures and texture views.
 *
 * PixelFormat is API-agnostic. Each backend maps these formats to the closest native pixel/texture format.
 *
 * Naming convention:
 * - Component order is given in the name (e.g. R8G8B8A8).
 * - `Unorm`/`Snorm` represent normalized integer formats (values are converted to/from floating-point in shaders).
 * - `Uint`/`Sint` represent integer formats (integer values are preserved in shaders).
 * - `Float` represents IEEE floating-point formats.
 * - `Srgb`/`sRGB` represent sRGB-encoded color formats (color channels are encoded/decoded as sRGB; alpha is linear).
 *
 * @note Not every format is supported on every backend/device. Unsupported formats may fail creation or be emulated.
 */
enum class PixelFormat : uint32_t {
    /**
     * @brief Undefined / unspecified pixel format.
     *
     * This value is typically used as a default or to indicate that a format is not known.
     */
    Undefined = 0,

    /**
     * @name Unsigned normalized formats (UNORM)
     * @brief Unsigned integer formats interpreted as normalized floating-point values.
     *
     * In shaders/samplers these are typically converted to floating-point in the [0, 1] range.
     */
    ///@{
    /** @brief 8-bit UNORM, single component (R). */
    R8Unorm,
    /** @brief 8-bit UNORM, two components (R,G). */
    R8G8Unorm,
    /** @brief 8-bit UNORM, three components (R,G,B). */
    R8G8B8Unorm,
    /** @brief 8-bit UNORM, four components (R,G,B,A). */
    R8G8B8A8Unorm,
    /** @brief 16-bit UNORM, single component (R). */
    R16Unorm,
    /** @brief 16-bit UNORM, two components (R,G). */
    R16G16Unorm,
    /** @brief 16-bit UNORM, four components (R,G,B,A). */
    R16G16B16A16Unorm,
    ///@}

    /**
     * @name Signed normalized formats (SNORM)
     * @brief Signed integer formats interpreted as normalized floating-point values.
     *
     * In shaders these are typically converted to floating-point in the [-1, 1] range.
     */
    ///@{
    /** @brief 8-bit SNORM, single component (R). */
    R8Snorm,
    /** @brief 8-bit SNORM, two components (R,G). */
    R8G8Snorm,
    /** @brief 8-bit SNORM, four components (R,G,B,A). */
    R8G8B8A8Snorm,
    /** @brief 16-bit SNORM, single component (R). */
    R16Snorm,
    /** @brief 16-bit SNORM, two components (R,G). */
    R16G16Snorm,
    /** @brief 16-bit SNORM, four components (R,G,B,A). */
    R16G16B16A16Snorm,
    ///@}

    /**
     * @name Unsigned integer formats (UINT)
     * @brief Unsigned integer texel formats.
     *
     * Values are not normalized and are read/written as integers in shaders.
     */
    ///@{
    /** @brief 8-bit UINT, single component (R). */
    R8Uint,
    /** @brief 8-bit UINT, two components (R,G). */
    R8G8Uint,
    /** @brief 8-bit UINT, four components (R,G,B,A). */
    R8G8B8A8Uint,
    /** @brief 16-bit UINT, single component (R). */
    R16Uint,
    /** @brief 16-bit UINT, two components (R,G). */
    R16G16Uint,
    /** @brief 16-bit UINT, four components (R,G,B,A). */
    R16G16B16A16Uint,
    /** @brief 32-bit UINT, single component (R). */
    R32Uint,
    /** @brief 32-bit UINT, two components (R,G). */
    R32G32Uint,
    /** @brief 32-bit UINT, four components (R,G,B,A). */
    R32G32B32A32Uint,
    ///@}

    /**
     * @name Signed integer formats (SINT)
     * @brief Signed integer texel formats.
     *
     * Values are not normalized and are read/written as integers in shaders.
     */
    ///@{
    /** @brief 8-bit SINT, single component (R). */
    R8Sint,
    /** @brief 8-bit SINT, two components (R,G). */
    R8G8Sint,
    /** @brief 8-bit SINT, four components (R,G,B,A). */
    R8G8B8A8Sint,
    /** @brief 16-bit SINT, single component (R). */
    R16Sint,
    /** @brief 16-bit SINT, two components (R,G). */
    R16G16Sint,
    /** @brief 16-bit SINT, four components (R,G,B,A). */
    R16G16B16A16Sint,
    /** @brief 32-bit SINT, single component (R). */
    R32Sint,
    /** @brief 32-bit SINT, two components (R,G). */
    R32G32Sint,
    /** @brief 32-bit SINT, four components (R,G,B,A). */
    R32G32B32A32Sint,
    ///@}

    /**
     * @name Floating-point formats
     * @brief IEEE floating-point texel formats.
     */
    ///@{
    /** @brief 16-bit floating-point, single component (R). */
    R16Float,
    /** @brief 16-bit floating-point, two components (R,G). */
    R16G16Float,
    /** @brief 16-bit floating-point, four components (R,G,B,A). */
    R16G16B16A16Float,
    /** @brief 32-bit floating-point, single component (R). */
    R32Float,
    /** @brief 32-bit floating-point, two components (R,G). */
    R32G32Float,
    /** @brief 32-bit floating-point, four components (R,G,B,A). */
    R32G32B32A32Float,
    ///@}

    /**
     * @name Special / packed color formats
     */
    ///@{
    /**
     * @brief Single-channel 8-bit grayscale format.
     *
     * This is intended for luminance-like data. Backends may treat this as an `R8Unorm`-compatible format depending on
     * platform support.
     */
    Grayscale,

    /** @brief 8-bit UNORM packed in BGRA component order. */
    B8G8R8A8Unorm,

    /**
     * @brief 8-bit sRGB encoded RGBA format.
     *
     * Color channels are sRGB encoded/decoded; alpha is linear.
     */
    R8G8B8A8Srgb,

    /** @brief Packed 10-bit RGB + 2-bit alpha, UNORM. */
    R10G10B10A2Unorm,

    /** @brief Packed 10-bit RGB + 2-bit alpha, UINT. */
    R10G10B10A2Uint,

    /** @brief Packed floating-point format: 11-bit R, 11-bit G, 10-bit B. */
    R11G11B10Float,
    ///@}

    /**
     * @name Depth / stencil formats
     * @brief Formats intended for depth and depth-stencil attachments.
     */
    ///@{
    /** @brief 16-bit UNORM depth-only format. */
    Depth16Unorm,

    /** @brief 32-bit floating-point depth-only format. */
    DepthFloat,

    /**
     * @brief Combined depth-stencil format.
     *
     * Exact bit layout is backend-dependent; this format is used when both depth and stencil aspects are required.
     */
    DepthStencil,
    ///@}

    /**
     * @name Block-compressed formats
     * @brief GPU block-compressed texel formats.
     *
     * These formats are stored in fixed-size blocks (e.g., 4x4 texels). They are intended for sampled textures and
     * may have restrictions (for example, they are typically not renderable).
     */
    ///@{
    /** @brief ETC compressed RGB format, UNORM (ETC1-like). */
    ETC_R8G8B8_Unorm,
    /** @brief ETC2 compressed RGB format, UNORM. */
    ETC2_R8G8B8_Unorm,
    /** @brief ETC2 compressed RGB with 1-bit alpha, UNORM. */
    ETC2_R8G8B8A1_Unorm,
    /** @brief ETC2 compressed RGBA format, UNORM. */
    ETC2_R8G8B8A8_Unorm,

    /** @brief ETC2 compressed RGB format, sRGB. */
    ETC2_R8G8B8_sRGB,
    /** @brief ETC2 compressed RGB with 1-bit alpha, sRGB. */
    ETC2_R8G8B8A1_sRGB,
    /** @brief ETC2 compressed RGBA format, sRGB. */
    ETC2_R8G8B8A8_sRGB,

    /** @brief BC3 (DXT5) compressed RGBA format, sRGB. */
    BC3_sRGBA,
    /** @brief BC3 (DXT5) compressed RGBA format, UNORM. */
    BC3_RGBA,
    /** @brief BC7 compressed RGBA format, sRGB. */
    BC7_sRGBA,
    /** @brief BC7 compressed RGBA format, UNORM. */
    BC7_RGBA,

    /** @brief ASTC 4x4 block-compressed format, UNORM. */
    ASTC_4x4_Unorm,
    /** @brief ASTC 4x4 block-compressed format, sRGB. */
    ASTC_4x4_sRGB,
    ///@}

    /**
     * @brief Sentinel value equal to the number of defined pixel formats.
     *
     * This value is not a valid pixel format for resources.
     */
    Count,
};
} // namespace snap::rhi
