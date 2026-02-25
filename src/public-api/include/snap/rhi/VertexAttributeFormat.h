//
//  VertexAttributeFormat.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 22.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <cstdint>

namespace snap::rhi {

/**
 * @brief Vertex attribute data format.
 *
 * Specifies the element type and component count of a vertex attribute as consumed by the graphics pipeline.
 *
 * Normalized formats (`*Normalized`) convert integer values into floating-point values in the shader input stage:
 * - Signed normalized formats map to [-1, 1].
 * - Unsigned normalized formats map to [0, 1].
 *
 * Backend notes:
 * - Vulkan maps supported formats to `VkFormat` (not all formats are supported on all backends).
 * - OpenGL uses `glVertexAttribPointer`/`glVertexAttribIPointer` with the appropriate type and normalization flag.
 * - Metal primarily reports/consumes float/int/uint formats via Metal reflection; some packed formats may be
 *   unavailable depending on platform.
 * - WebGPU supports only a subset of formats; unsupported formats will fail validation.
 */
enum class VertexAttributeFormat : uint32_t {
    /**
     * @brief The format is not specified.
     */
    Undefined = 0,

    // 8-bit signed integer
    /** @brief 2-component signed 8-bit integer vector. */
    Byte2,
    /** @brief 3-component signed 8-bit integer vector. */
    Byte3,
    /** @brief 4-component signed 8-bit integer vector. */
    Byte4,

    // 8-bit signed normalized
    /** @brief 2-component signed normalized 8-bit integer vector. */
    Byte2Normalized,
    /** @brief 3-component signed normalized 8-bit integer vector. */
    Byte3Normalized,
    /** @brief 4-component signed normalized 8-bit integer vector. */
    Byte4Normalized,

    // 8-bit unsigned integer
    /** @brief 2-component unsigned 8-bit integer vector. */
    UnsignedByte2,
    /** @brief 3-component unsigned 8-bit integer vector. */
    UnsignedByte3,
    /** @brief 4-component unsigned 8-bit integer vector. */
    UnsignedByte4,

    // 8-bit unsigned normalized
    /** @brief 2-component unsigned normalized 8-bit integer vector. */
    UnsignedByte2Normalized,
    /** @brief 3-component unsigned normalized 8-bit integer vector. */
    UnsignedByte3Normalized,
    /** @brief 4-component unsigned normalized 8-bit integer vector. */
    UnsignedByte4Normalized,

    // 16-bit signed integer
    /** @brief 2-component signed 16-bit integer vector. */
    Short2,
    /** @brief 3-component signed 16-bit integer vector. */
    Short3,
    /** @brief 4-component signed 16-bit integer vector. */
    Short4,

    // 16-bit signed normalized
    /** @brief 2-component signed normalized 16-bit integer vector. */
    Short2Normalized,
    /** @brief 3-component signed normalized 16-bit integer vector. */
    Short3Normalized,
    /** @brief 4-component signed normalized 16-bit integer vector. */
    Short4Normalized,

    // 16-bit unsigned integer
    /** @brief 2-component unsigned 16-bit integer vector. */
    UnsignedShort2,
    /** @brief 3-component unsigned 16-bit integer vector. */
    UnsignedShort3,
    /** @brief 4-component unsigned 16-bit integer vector. */
    UnsignedShort4,

    // 16-bit unsigned normalized
    /** @brief 2-component unsigned normalized 16-bit integer vector. */
    UnsignedShort2Normalized,
    /** @brief 3-component unsigned normalized 16-bit integer vector. */
    UnsignedShort3Normalized,
    /** @brief 4-component unsigned normalized 16-bit integer vector. */
    UnsignedShort4Normalized,

    // 16-bit float
    /** @brief 2-component 16-bit floating point vector. */
    HalfFloat2,
    /** @brief 3-component 16-bit floating point vector. */
    HalfFloat3,
    /** @brief 4-component 16-bit floating point vector. */
    HalfFloat4,

    // 32-bit float
    /** @brief 1-component 32-bit floating point scalar. */
    Float,
    /** @brief 2-component 32-bit floating point vector. */
    Float2,
    /** @brief 3-component 32-bit floating point vector. */
    Float3,
    /** @brief 4-component 32-bit floating point vector. */
    Float4,

    // 32-bit signed integer
    /** @brief 1-component 32-bit signed integer scalar. */
    Int,
    /** @brief 2-component 32-bit signed integer vector. */
    Int2,
    /** @brief 3-component 32-bit signed integer vector. */
    Int3,
    /** @brief 4-component 32-bit signed integer vector. */
    Int4,

    // 32-bit unsigned integer
    /** @brief 1-component 32-bit unsigned integer scalar. */
    UInt,
    /** @brief 2-component 32-bit unsigned integer vector. */
    UInt2,
    /** @brief 3-component 32-bit unsigned integer vector. */
    UInt3,
    /** @brief 4-component 32-bit unsigned integer vector. */
    UInt4,

    /**
     * @brief Sentinel value.
     */
    Count,
};
} // namespace snap::rhi
