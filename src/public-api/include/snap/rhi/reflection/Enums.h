//
//  Enums.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 06.10.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Enums.h"

namespace snap::rhi::reflection {
enum class UniformFormat : uint32_t {
    /**
     *  The format is not specified.
     */
    Undefined = 0,

    Bool, // 32-bit integer
    Bool2,
    Bool3,
    Bool4,

    Int, // 32-bit integer
    Int2,
    Int3,
    Int4,

    UInt, // 32-bit unsigned integer
    UInt2,
    UInt3,
    UInt4,

    Float,
    Float2,
    Float3, // Float4
    Float4,

    Float2x2,
    Float3x3, // Float3x4
    Float4x4,

    Count
};

enum class SamplerType : uint32_t {
    /**
     *  The format is not specified.
     */
    Undefined = 0,

    Sampler,
    SamplerShadow,

    Count
};

enum class TextureType : uint32_t {
    /**
     *  The format is not specified.
     */
    Undefined = 0,

    Texture1D,
    Texture2D,
    TextureCube,
    Texture3D,

    Texture1DArray,
    Texture2DArray,
    TextureCubeArray,

    Texture2DMS,
    Texture2DMSArray,

    TextureBuffer,

    TextureExternal,

    Count
};

enum class ImageAccess : uint32_t {
    Undefined = 0,

    ReadOnly,
    WriteOnly,
    ReadWrite,

    Count
};

enum class StorageBufferAccess : uint32_t {
    Undefined = 0,

    ReadOnly,
    WriteOnly,
    ReadWrite,

    Count
};

uint32_t getUniformTypeSize(const UniformFormat type) noexcept;

constexpr bool isIntegerUniformFormat(const snap::rhi::reflection::UniformFormat format) noexcept {
    switch (format) {
        case UniformFormat::Int:
        case UniformFormat::Int2:
        case UniformFormat::Int3:
        case UniformFormat::Int4:

        case UniformFormat::UInt:
        case UniformFormat::UInt2:
        case UniformFormat::UInt3:
        case UniformFormat::UInt4:

        case UniformFormat::Bool:
        case UniformFormat::Bool2:
        case UniformFormat::Bool3:
        case UniformFormat::Bool4:
            return true;

        default:
            return false;
    }
}

snap::rhi::TextureType convertToTextureType(const snap::rhi::reflection::TextureType format);
} // namespace snap::rhi::reflection
