#include "snap/rhi/reflection/Enums.h"

#include "snap/rhi/common/Throw.h"
#include <array>

namespace snap::rhi::reflection {
constexpr std::array<uint32_t, static_cast<uint32_t>(UniformFormat::Count)> buildUniformTypeStride() {
    std::array<uint32_t, static_cast<uint32_t>(UniformFormat::Count)> result{};

    result[static_cast<uint32_t>(UniformFormat::Undefined)] = 0;

    result[static_cast<uint32_t>(UniformFormat::Float)] = sizeof(float);
    result[static_cast<uint32_t>(UniformFormat::Float2)] = sizeof(float) * 2;
    result[static_cast<uint32_t>(UniformFormat::Float3)] = sizeof(float) * 3;
    result[static_cast<uint32_t>(UniformFormat::Float4)] = sizeof(float) * 4;

    result[static_cast<uint32_t>(UniformFormat::Float2x2)] = sizeof(float) * 2 * 2;
    result[static_cast<uint32_t>(UniformFormat::Float3x3)] = sizeof(float) * 3 * 4;
    result[static_cast<uint32_t>(UniformFormat::Float4x4)] = sizeof(float) * 4 * 4;

    result[static_cast<uint32_t>(UniformFormat::Int)] = sizeof(int32_t);
    result[static_cast<uint32_t>(UniformFormat::Int2)] = sizeof(int32_t) * 2;
    result[static_cast<uint32_t>(UniformFormat::Int3)] = sizeof(int32_t) * 3;
    result[static_cast<uint32_t>(UniformFormat::Int4)] = sizeof(int32_t) * 4;

    result[static_cast<uint32_t>(UniformFormat::UInt)] = sizeof(uint32_t);
    result[static_cast<uint32_t>(UniformFormat::UInt2)] = sizeof(uint32_t) * 2;
    result[static_cast<uint32_t>(UniformFormat::UInt3)] = sizeof(uint32_t) * 3;
    result[static_cast<uint32_t>(UniformFormat::UInt4)] = sizeof(uint32_t) * 4;

    return result;
}

constexpr std::array<uint32_t, static_cast<uint32_t>(UniformFormat::Count)> UniformTypeStride =
    buildUniformTypeStride();

uint32_t getUniformTypeSize(const UniformFormat type) noexcept {
    return UniformTypeStride[static_cast<uint32_t>(type)];
}

snap::rhi::TextureType convertToTextureType(const snap::rhi::reflection::TextureType format) {
    switch (format) {
        case snap::rhi::reflection::TextureType::Texture2D:
            return snap::rhi::TextureType::Texture2D;

        case snap::rhi::reflection::TextureType::Texture3D:
            return snap::rhi::TextureType::Texture3D;

        case snap::rhi::reflection::TextureType::Texture2DArray:
            return snap::rhi::TextureType::Texture2DArray;

        case snap::rhi::reflection::TextureType::TextureCube:
            return snap::rhi::TextureType::TextureCubemap;

        default: {
            snap::rhi::common::throwException("[convertToTextureType] unsupported TextureType");
        }
    }
}
} // namespace snap::rhi::reflection
