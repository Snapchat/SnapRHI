#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/Common.h"
#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <functional>
#include <ranges>
#include <spirv_reflect.h>
#include <stack>
#include <string>
#include <unordered_set>

namespace {
std::vector<snap::rhi::reflection::SpecializationInfo> buildSpecializationInfos(std::span<const uint32_t> code) {
    struct SpecConstFound {
        uint32_t resultId = 0; // The internal SPIR-V ID
        uint32_t typeId = 0;   // The internal Type ID
    };

    // SPIR-V Opcodes
    static constexpr uint32_t SpvOpTypeBool = 20;
    static constexpr uint32_t SpvOpTypeInt = 21;
    static constexpr uint32_t SpvOpTypeFloat = 22;
    static constexpr uint32_t SpvOpSpecConstantTrue = 48;
    static constexpr uint32_t SpvOpSpecConstantFalse = 49;
    static constexpr uint32_t SpvOpSpecConstant = 50;
    static constexpr uint32_t SpvOpDecorate = 71;
    // Decoration Type for Specialization Constants
    static constexpr uint32_t SpvDecorationSpecId = 1;
    static constexpr uint32_t SPIRV_HEADER_SIZE = 5; // SPIR-V Header is 5 words
    const uint32_t* ptr = code.data() + SPIRV_HEADER_SIZE;

    std::unordered_map<uint32_t, snap::rhi::SpecializationConstantFormat> typeNames; // TypeID -> "int", "float"
    std::unordered_map<uint32_t, uint32_t> specIds;  // ResultID -> constant_id (from layout)
    std::unordered_map<uint32_t, std::string> names; // ResultID -> Name
    std::vector<SpecConstFound> constants;

    while (ptr < code.data() + code.size()) {
        // Ensure we can safely read the OpCode word
        if (ptr + 1 > code.data() + code.size())
            break;

        uint32_t opCodeAndSize = *ptr;
        uint32_t count = opCodeAndSize >> 16;
        uint32_t opCode = opCodeAndSize & 0xFFFF;

        // Validation: WordCount must be >= 1 and the instruction must fit in the buffer
        if (count == 0 || ptr + count > code.data() + code.size()) {
            break;
        }

        if (opCode == SpvOpName) {
            // [OpName] [TargetID] [String...]
            uint32_t targetId = ptr[1];

            // The string starts at the 3rd word (ptr + 2).
            // SPIR-V strings are null-terminated and padded to word boundaries.
            // Casting the uint32_t* to char* is safe here.
            const char* namePtr = reinterpret_cast<const char*>(ptr + 2);

            // Use strnlen logic or just trust SPIR-V is valid null-terminated
            // We create a string using the pointer.
            names[targetId] = std::string(namePtr);
        } else if (opCode == SpvOpDecorate) {
            // [OpDecorate] [TargetID] [Decoration] [Value (Optional)]
            uint32_t targetId = ptr[1];
            uint32_t decoration = ptr[2];

            if (decoration == SpvDecorationSpecId) {
                uint32_t specIdVal = ptr[3];
                specIds[targetId] = specIdVal;
            }
        } else if (opCode == SpvOpTypeInt) {
            // [OpTypeInt] [ResultID] [Width] [Signedness]
            uint32_t resId = ptr[1];
            uint32_t width = ptr[2];
            uint32_t isSigned = ptr[3];

            snap::rhi::SpecializationConstantFormat format = isSigned ? snap::rhi::SpecializationConstantFormat::Int32 :
                                                                        snap::rhi::SpecializationConstantFormat::UInt32;
            assert(width == 32); // Only 32-bit specialization constants are supported
            if (width != 32) {   // Fallback for 16-bit or 64-bit integers
                format = snap::rhi::SpecializationConstantFormat::Undefined;
            }
            typeNames[resId] = format;
        } else if (opCode == SpvOpTypeFloat) {
            // [OpTypeFloat] [ResultID] [Width]
            uint32_t resId = ptr[1];
            uint32_t width = ptr[2];

            snap::rhi::SpecializationConstantFormat format = snap::rhi::SpecializationConstantFormat::Float;
            assert(width == 32); // Only 32-bit specialization constants are supported
            if (width != 32) {   // Fallback for 16-bit or 64-bit float
                format = snap::rhi::SpecializationConstantFormat::Undefined;
            }

            typeNames[resId] = format;
        } else if (opCode == SpvOpTypeBool) {
            // [OpTypeBool] [ResultID]
            uint32_t resId = ptr[1];
            const snap::rhi::SpecializationConstantFormat format = snap::rhi::SpecializationConstantFormat::Bool32;
            typeNames[resId] = format;
        } else if (opCode == SpvOpSpecConstant) {
            // [OpSpecConstant] [ResultType] [ResultID] [Value]
            uint32_t typeId = ptr[1];
            uint32_t resultId = ptr[2];
            constants.push_back({resultId, typeId});
        } else if (opCode == SpvOpSpecConstantTrue || opCode == SpvOpSpecConstantFalse) {
            // [OpSpecConstantTrue] [ResultType] [ResultID]
            uint32_t typeId = ptr[1];
            uint32_t resultId = ptr[2];
            constants.push_back({resultId, typeId});
        }

        ptr += count;
    }

    std::vector<snap::rhi::reflection::SpecializationInfo> specInfos{};
    for (const auto& c : constants) {
        // Only if it actually has a SpecId decoration (meaning it's exposed to the API)
        if (auto spIdItr = specIds.find(c.resultId); spIdItr != specIds.end()) {
            uint32_t id = spIdItr->second;

            auto format = snap::rhi::SpecializationConstantFormat::Undefined;
            if (auto typeNameItr = typeNames.find(c.typeId); typeNameItr != typeNames.end()) {
                format = typeNameItr->second;
            }

            std::string name;
            if (auto nameItr = names.find(c.resultId); nameItr != names.end()) {
                name = nameItr->second;
            }

            snap::rhi::reflection::SpecializationInfo specInfo{
                .name = name,
                .constantID = id,
                .format = format,
                // SPIR-V always embeds a default value for a specialization constant; treat as not strictly required.
                .required = false};

            specInfos.push_back(specInfo);
        }
    }
    std::ranges::sort(specInfos, {}, &snap::rhi::reflection::SpecializationInfo::constantID);

    return specInfos;
}

snap::rhi::VertexAttributeFormat mapInputFormat(const SpvReflectInterfaceVariable* var) {
    if (!var) {
        return snap::rhi::VertexAttributeFormat::Undefined;
    }
    switch (var->format) {
        // 32-bit float
        case SPV_REFLECT_FORMAT_R32_SFLOAT:
            return snap::rhi::VertexAttributeFormat::Float;
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
            return snap::rhi::VertexAttributeFormat::Float2;
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
            return snap::rhi::VertexAttributeFormat::Float3;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
            return snap::rhi::VertexAttributeFormat::Float4;

        // 32-bit signed int
        case SPV_REFLECT_FORMAT_R32_SINT:
            return snap::rhi::VertexAttributeFormat::Int;
        case SPV_REFLECT_FORMAT_R32G32_SINT:
            return snap::rhi::VertexAttributeFormat::Int2;
        case SPV_REFLECT_FORMAT_R32G32B32_SINT:
            return snap::rhi::VertexAttributeFormat::Int3;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
            return snap::rhi::VertexAttributeFormat::Int4;

        // 32-bit unsigned int
        case SPV_REFLECT_FORMAT_R32_UINT:
            return snap::rhi::VertexAttributeFormat::UInt;
        case SPV_REFLECT_FORMAT_R32G32_UINT:
            return snap::rhi::VertexAttributeFormat::UInt2;
        case SPV_REFLECT_FORMAT_R32G32B32_UINT:
            return snap::rhi::VertexAttributeFormat::UInt3;
        case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
            return snap::rhi::VertexAttributeFormat::UInt4;

        // 16-bit float
        case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
            return snap::rhi::VertexAttributeFormat::HalfFloat2;
        case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
            return snap::rhi::VertexAttributeFormat::HalfFloat4;

        // 16-bit signed int
        case SPV_REFLECT_FORMAT_R16G16_SINT:
            return snap::rhi::VertexAttributeFormat::Short2;
        case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
            return snap::rhi::VertexAttributeFormat::Short4;

        // 16-bit unsigned int
        case SPV_REFLECT_FORMAT_R16G16_UINT:
            return snap::rhi::VertexAttributeFormat::UnsignedShort2;
        case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
            return snap::rhi::VertexAttributeFormat::UnsignedShort4;

        default:
            return snap::rhi::VertexAttributeFormat::Undefined;
    }
}

snap::rhi::reflection::UniformFormat mapUniformFormat(const SpvReflectTypeDescription* type) {
    if (!type) {
        return snap::rhi::reflection::UniformFormat::Undefined;
    }

    const uint32_t flags = type->type_flags; // bitmask

    // Matrix (always float matrices in shaders we handle)
    if ((flags & SPV_REFLECT_TYPE_FLAG_MATRIX) && type->traits.numeric.matrix.row_count > 0 &&
        type->traits.numeric.matrix.column_count > 0) {
        const uint32_t r = type->traits.numeric.matrix.row_count;
        const uint32_t c = type->traits.numeric.matrix.column_count;
        if (r == 2 && c == 2)
            return snap::rhi::reflection::UniformFormat::Float2x2;
        if (r == 3 && c == 3)
            return snap::rhi::reflection::UniformFormat::Float3x3;
        if (r == 4 && c == 4)
            return snap::rhi::reflection::UniformFormat::Float4x4;
        return snap::rhi::reflection::UniformFormat::Undefined; // unsupported matrix shape
    }

    const uint32_t vecCount =
        type->traits.numeric.vector.component_count == 0 ? 1 : type->traits.numeric.vector.component_count;
    auto pick = [&](snap::rhi::reflection::UniformFormat s,
                    snap::rhi::reflection::UniformFormat v2,
                    snap::rhi::reflection::UniformFormat v3,
                    snap::rhi::reflection::UniformFormat v4) {
        switch (vecCount) {
            case 1:
                return s;
            case 2:
                return v2;
            case 3:
                return v3;
            case 4:
                return v4;
            default:
                return snap::rhi::reflection::UniformFormat::Undefined;
        }
    };

    if (flags & SPV_REFLECT_TYPE_FLAG_BOOL) {
        return pick(snap::rhi::reflection::UniformFormat::Bool,
                    snap::rhi::reflection::UniformFormat::Bool2,
                    snap::rhi::reflection::UniformFormat::Bool3,
                    snap::rhi::reflection::UniformFormat::Bool4);
    }

    if (flags & SPV_REFLECT_TYPE_FLAG_INT) {
        const bool signedness = type->traits.numeric.scalar.signedness != 0; // 1 signed, 0 unsigned
        if (signedness) {
            return pick(snap::rhi::reflection::UniformFormat::Int,
                        snap::rhi::reflection::UniformFormat::Int2,
                        snap::rhi::reflection::UniformFormat::Int3,
                        snap::rhi::reflection::UniformFormat::Int4);
        } else {
            return pick(snap::rhi::reflection::UniformFormat::UInt,
                        snap::rhi::reflection::UniformFormat::UInt2,
                        snap::rhi::reflection::UniformFormat::UInt3,
                        snap::rhi::reflection::UniformFormat::UInt4);
        }
    }

    if (flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
        return pick(snap::rhi::reflection::UniformFormat::Float,
                    snap::rhi::reflection::UniformFormat::Float2,
                    snap::rhi::reflection::UniformFormat::Float3,
                    snap::rhi::reflection::UniformFormat::Float4);
    }

    return snap::rhi::reflection::UniformFormat::Undefined;
}

snap::rhi::reflection::TextureType mapTextureType(const SpvReflectDescriptorBinding* b) {
    if (!b)
        return snap::rhi::reflection::TextureType::Undefined;
    switch (b->image.dim) {
        case SpvDim1D:
            return b->image.arrayed ? snap::rhi::reflection::TextureType::Texture1DArray :
                                      snap::rhi::reflection::TextureType::Texture1D;
        case SpvDim2D:
            if (b->image.ms && b->image.arrayed)
                return snap::rhi::reflection::TextureType::Texture2DMSArray;
            if (b->image.ms)
                return snap::rhi::reflection::TextureType::Texture2DMS;
            return b->image.arrayed ? snap::rhi::reflection::TextureType::Texture2DArray :
                                      snap::rhi::reflection::TextureType::Texture2D;
        case SpvDim3D:
            return snap::rhi::reflection::TextureType::Texture3D;
        case SpvDimCube:
            return b->image.arrayed ? snap::rhi::reflection::TextureType::TextureCubeArray :
                                      snap::rhi::reflection::TextureType::TextureCube;
        default:
            return snap::rhi::reflection::TextureType::Undefined;
    }
}

static snap::rhi::reflection::ImageAccess mapImageAccess(const SpvReflectDescriptorBinding* b) {
    if (!b)
        return snap::rhi::reflection::ImageAccess::Undefined;
    // if (b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
    //     if (b->image.read && b->image.write) return snap::rhi::reflection::ImageAccess::ReadWrite;
    //     if (b->image.read) return snap::rhi::reflection::ImageAccess::ReadOnly;
    //     if (b->image.write) return snap::rhi::reflection::ImageAccess::WriteOnly;
    // }
    return snap::rhi::reflection::ImageAccess::Undefined;
}

snap::rhi::reflection::StorageBufferAccess mapStorageBufferAccess(const SpvReflectDescriptorBinding* b) { // ???
    if (!b)
        return snap::rhi::reflection::StorageBufferAccess::Undefined;
    if (b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
        const bool readonly =
            (b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER && !b->block.size); // heuristic
        return readonly ? snap::rhi::reflection::StorageBufferAccess::ReadOnly :
                          snap::rhi::reflection::StorageBufferAccess::ReadWrite;
    }
    return snap::rhi::reflection::StorageBufferAccess::Undefined;
}
} // namespace

namespace snap::rhi::backend::vulkan {
VkFormat toVkFormat(const snap::rhi::PixelFormat format) {
    switch (format) {
        case snap::rhi::PixelFormat::Undefined:
            return VK_FORMAT_UNDEFINED;

        case snap::rhi::PixelFormat::R8Unorm:
            return VK_FORMAT_R8_UNORM;
        case snap::rhi::PixelFormat::R8G8Unorm:
            return VK_FORMAT_R8G8_UNORM;
        case snap::rhi::PixelFormat::R8G8B8Unorm:
            return VK_FORMAT_R8G8B8_UNORM;
        case snap::rhi::PixelFormat::R8G8B8A8Unorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case snap::rhi::PixelFormat::R16Unorm:
            return VK_FORMAT_R16_UNORM;
        case snap::rhi::PixelFormat::R16G16Unorm:
            return VK_FORMAT_R16G16_UNORM;
        case snap::rhi::PixelFormat::R16G16B16A16Unorm:
            return VK_FORMAT_R16G16B16A16_UNORM;

        case snap::rhi::PixelFormat::R8Snorm:
            return VK_FORMAT_R8_SNORM;
        case snap::rhi::PixelFormat::R8G8Snorm:
            return VK_FORMAT_R8G8_SNORM;
        case snap::rhi::PixelFormat::R8G8B8A8Snorm:
            return VK_FORMAT_R8G8B8A8_SNORM;
        case snap::rhi::PixelFormat::R16Snorm:
            return VK_FORMAT_R16_SNORM;
        case snap::rhi::PixelFormat::R16G16Snorm:
            return VK_FORMAT_R16G16_SNORM;
        case snap::rhi::PixelFormat::R16G16B16A16Snorm:
            return VK_FORMAT_R16G16B16A16_SNORM;

        case snap::rhi::PixelFormat::R8Uint:
            return VK_FORMAT_R8_UINT;
        case snap::rhi::PixelFormat::R8G8Uint:
            return VK_FORMAT_R8G8_UINT;
        case snap::rhi::PixelFormat::R8G8B8A8Uint:
            return VK_FORMAT_R8G8B8A8_UINT;
        case snap::rhi::PixelFormat::R16Uint:
            return VK_FORMAT_R16_UINT;
        case snap::rhi::PixelFormat::R16G16Uint:
            return VK_FORMAT_R16G16_UINT;
        case snap::rhi::PixelFormat::R16G16B16A16Uint:
            return VK_FORMAT_R16G16B16A16_UINT;
        case snap::rhi::PixelFormat::R32Uint:
            return VK_FORMAT_R32_UINT;
        case snap::rhi::PixelFormat::R32G32Uint:
            return VK_FORMAT_R32G32_UINT;
        case snap::rhi::PixelFormat::R32G32B32A32Uint:
            return VK_FORMAT_R32G32B32A32_UINT;

        case snap::rhi::PixelFormat::R8Sint:
            return VK_FORMAT_R8_SINT;
        case snap::rhi::PixelFormat::R8G8Sint:
            return VK_FORMAT_R8G8_SINT;
        case snap::rhi::PixelFormat::R8G8B8A8Sint:
            return VK_FORMAT_R8G8B8A8_SINT;
        case snap::rhi::PixelFormat::R16Sint:
            return VK_FORMAT_R16_SINT;
        case snap::rhi::PixelFormat::R16G16Sint:
            return VK_FORMAT_R16G16_SINT;
        case snap::rhi::PixelFormat::R16G16B16A16Sint:
            return VK_FORMAT_R16G16B16A16_SINT;
        case snap::rhi::PixelFormat::R32Sint:
            return VK_FORMAT_R32_SINT;
        case snap::rhi::PixelFormat::R32G32Sint:
            return VK_FORMAT_R32G32_SINT;
        case snap::rhi::PixelFormat::R32G32B32A32Sint:
            return VK_FORMAT_R32G32B32A32_SINT;

        case snap::rhi::PixelFormat::R16Float:
            return VK_FORMAT_R16_SFLOAT;
        case snap::rhi::PixelFormat::R16G16Float:
            return VK_FORMAT_R16G16_SFLOAT;
        case snap::rhi::PixelFormat::R16G16B16A16Float:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case snap::rhi::PixelFormat::R32Float:
            return VK_FORMAT_R32_SFLOAT;
        case snap::rhi::PixelFormat::R32G32Float:
            return VK_FORMAT_R32G32_SFLOAT;
        case snap::rhi::PixelFormat::R32G32B32A32Float:
            return VK_FORMAT_R32G32B32A32_SFLOAT;

        case snap::rhi::PixelFormat::Grayscale:
            return VK_FORMAT_R8_UNORM;
        case snap::rhi::PixelFormat::B8G8R8A8Unorm:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case snap::rhi::PixelFormat::R8G8B8A8Srgb:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case snap::rhi::PixelFormat::R10G10B10A2Unorm:
            return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        case snap::rhi::PixelFormat::R10G10B10A2Uint:
            return VK_FORMAT_A2R10G10B10_UINT_PACK32;
        case snap::rhi::PixelFormat::R11G11B10Float:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

        case snap::rhi::PixelFormat::Depth16Unorm:
            return VK_FORMAT_D16_UNORM;
        case snap::rhi::PixelFormat::DepthFloat:
            return VK_FORMAT_D32_SFLOAT;
        case snap::rhi::PixelFormat::DepthStencil:
#if SNAP_RHI_OS_APPLE()
            // MoltenVK on macOS doesn't support VK_FORMAT_D24_UNORM_S8_UINT
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
#else
            return VK_FORMAT_D24_UNORM_S8_UINT;
#endif

        case snap::rhi::PixelFormat::ETC_R8G8B8_Unorm:
            return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        case snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm:
            return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm:
            return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm:
            return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        case snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB:
            return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB:
            return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB:
            return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
        case snap::rhi::PixelFormat::ASTC_4x4_Unorm:
            return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case snap::rhi::PixelFormat::ASTC_4x4_sRGB:
            return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;

        case snap::rhi::PixelFormat::BC3_sRGBA:
            return VK_FORMAT_BC3_SRGB_BLOCK;
        case snap::rhi::PixelFormat::BC3_RGBA:
            return VK_FORMAT_BC3_UNORM_BLOCK;
        case snap::rhi::PixelFormat::BC7_sRGBA:
            return VK_FORMAT_BC7_SRGB_BLOCK;
        case snap::rhi::PixelFormat::BC7_RGBA:
            return VK_FORMAT_BC7_UNORM_BLOCK;

        default:
            snap::rhi::common::throwException("[getFormat] unsupported image format");
    }
}

uint32_t getArraySize(const snap::rhi::TextureType textureType, const snap::rhi::Extent3D& extent3D) {
    switch (textureType) {
        case snap::rhi::TextureType::Texture2D:
            return 1;

        case snap::rhi::TextureType::Texture2DArray:
            return extent3D.depth;

        case snap::rhi::TextureType::TextureCubemap:
            return extent3D.depth * 6;

        case snap::rhi::TextureType::Texture3D:
            return 1;

        default:
            snap::rhi::common::throwException("[getImageType] unsupported image type");
    }
}

uint32_t getBaseArrayLayer(const snap::rhi::TextureType textureType, const uint32_t z) {
    if (textureType == snap::rhi::TextureType::Texture3D) {
        return 0;
    }

    return z;
}

uint32_t getLayerCount(const snap::rhi::TextureType textureType, const uint32_t depth) {
    if (textureType == snap::rhi::TextureType::Texture3D) {
        return 1;
    }

    return depth;
}

int32_t getImageOffsetZ(const snap::rhi::TextureType textureType, const uint32_t z) {
    if (textureType == snap::rhi::TextureType::Texture3D) {
        return static_cast<int32_t>(z);
    }

    return 0;
}

uint32_t getImageExtentDepth(const snap::rhi::TextureType textureType, const uint32_t depth) {
    if (textureType == snap::rhi::TextureType::Texture3D) {
        return depth;
    }

    return 1;
}

VkImageAspectFlags getImageAspectFlags(const snap::rhi::PixelFormat format) {
    VkImageAspectFlags aspectMask = 0;

    if (hasDepthAspect(format)) {
        aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    if (hasStencilAspect(format)) {
        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if (!aspectMask) {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    return aspectMask;
}

VkShaderStageFlags getShaderStageFlags(const snap::rhi::ShaderStageBits bits) {
    VkShaderStageFlags result = 0;

    if (static_cast<bool>(bits & snap::rhi::ShaderStageBits::VertexShaderBit)) {
        result |= VK_SHADER_STAGE_VERTEX_BIT;
    }

    if (static_cast<bool>(bits & snap::rhi::ShaderStageBits::FragmentShaderBit)) {
        result |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    if (static_cast<bool>(bits & snap::rhi::ShaderStageBits::ComputeShaderBit)) {
        result |= VK_SHADER_STAGE_COMPUTE_BIT;
    }

    return result;
}

VkShaderStageFlagBits getShaderStageFlags(const snap::rhi::ShaderStage stage) {
    switch (stage) {
        case snap::rhi::ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;

        case snap::rhi::ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;

        case snap::rhi::ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;

        default:
            assert(false);
            return static_cast<VkShaderStageFlagBits>(0);
    }
}

uint32_t getDataSize(const snap::rhi::SpecializationConstantFormat format) {
    switch (format) {
        /**
         * If the specialization constant is of type boolean, size must be the byte size of VkBool32
         * (https://docs.vulkan.org/spec/latest/chapters/pipelines.html#VUID-VkSpecializationMapEntry-constantID-00776)
         */
        case snap::rhi::SpecializationConstantFormat::Bool32:
            return sizeof(VkBool32);

        case snap::rhi::SpecializationConstantFormat::Float:
        case snap::rhi::SpecializationConstantFormat::Int32:
        case snap::rhi::SpecializationConstantFormat::UInt32:
            return 4;

        default:
            assert(false);
            return 0;
    }
}

VkDescriptorType getDescriptorType(const snap::rhi::DescriptorType type) {
    switch (type) {
        case snap::rhi::DescriptorType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case snap::rhi::DescriptorType::SampledTexture:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case snap::rhi::DescriptorType::StorageTexture:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case snap::rhi::DescriptorType::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case snap::rhi::DescriptorType::StorageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case snap::rhi::DescriptorType::UniformBufferDynamic:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case snap::rhi::DescriptorType::StorageBufferDynamic:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

        default:
            snap::rhi::common::throwException("[convertToVkDescriptorType] invalid DescriptorType: " +
                                              std::to_string(static_cast<uint32_t>(type)));
    }
}

VkFilter toVkFilter(const snap::rhi::SamplerMinMagFilter filter) {
    switch (filter) {
        case snap::rhi::SamplerMinMagFilter::Nearest:
            return VK_FILTER_NEAREST;

        case snap::rhi::SamplerMinMagFilter::Linear:
            return VK_FILTER_LINEAR;

        default:
            snap::rhi::common::throwException("[toVkFilter] invalid SamplerMinMagFilter: " +
                                              std::to_string(static_cast<uint32_t>(filter)));
    }
}

VkSamplerMipmapMode toVkSamplerMipmapMode(const snap::rhi::SamplerMipFilter filter) {
    switch (filter) {
        case snap::rhi::SamplerMipFilter::Nearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case snap::rhi::SamplerMipFilter::Linear:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;

        case snap::rhi::SamplerMipFilter::NotMipmapped: // Should be handled separately
        default:
            snap::rhi::common::throwException("[toVkSamplerMipmapMode] invalid SamplerMinMagFilter: " +
                                              std::to_string(static_cast<uint32_t>(filter)));
    }
}

VkSamplerAddressMode toVkSamplerAddressMode(const snap::rhi::WrapMode mode) {
    switch (mode) {
        case snap::rhi::WrapMode::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        case snap::rhi::WrapMode::Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;

        case snap::rhi::WrapMode::MirrorRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

        case snap::rhi::WrapMode::ClampToBorderColor:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

        default:
            snap::rhi::common::throwException("[toVkSamplerAddressMode] invalid WrapMode: " +
                                              std::to_string(static_cast<uint32_t>(mode)));
    }
}

VkCompareOp toVkCompareOp(const snap::rhi::CompareFunction compareFunction) {
    switch (compareFunction) {
        case snap::rhi::CompareFunction::Never:
            return VK_COMPARE_OP_NEVER;

        case snap::rhi::CompareFunction::Less:
            return VK_COMPARE_OP_LESS;

        case snap::rhi::CompareFunction::Equal:
            return VK_COMPARE_OP_EQUAL;

        case snap::rhi::CompareFunction::LessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;

        case snap::rhi::CompareFunction::Greater:
            return VK_COMPARE_OP_GREATER;

        case snap::rhi::CompareFunction::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;

        case snap::rhi::CompareFunction::GreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;

        case snap::rhi::CompareFunction::Always:
            return VK_COMPARE_OP_ALWAYS;

        default:
            snap::rhi::common::throwException("[toVkCompareOp] invalid CompareFunction: " +
                                              std::to_string(static_cast<uint32_t>(compareFunction)));
    }
}

VkBorderColor toVkBorderColor(const snap::rhi::SamplerBorderColor color) {
    switch (color) {
        case snap::rhi::SamplerBorderColor::TransparentBlack:
            return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

        case snap::rhi::SamplerBorderColor::OpaqueBlack:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

        case snap::rhi::SamplerBorderColor::OpaqueWhite:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        default:
            snap::rhi::common::throwException("[toVkBorderColor] invalid SamplerBorderColor: " +
                                              std::to_string(static_cast<uint32_t>(color)));
    }
}

VkPipelineStageFlags toVkPipelineStageFlags(const snap::rhi::PipelineStageBits bits) {
    VkPipelineStageFlags result = 0;

    constexpr auto stageMap = std::to_array<std::pair<snap::rhi::PipelineStageBits, VkPipelineStageFlagBits>>({
        {snap::rhi::PipelineStageBits::TopOfPipeBit, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT},
        {snap::rhi::PipelineStageBits::VertexInputBit, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
        {snap::rhi::PipelineStageBits::VertexShaderBit, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT},
        {snap::rhi::PipelineStageBits::FragmentShaderBit, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
        {snap::rhi::PipelineStageBits::EarlyFragmentTestsBit, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT},
        {snap::rhi::PipelineStageBits::LateFragmentTestsBit, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT},
        {snap::rhi::PipelineStageBits::ColorAttachmentOutputBit, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        {snap::rhi::PipelineStageBits::ComputeShaderBit, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
        {snap::rhi::PipelineStageBits::TransferBit, VK_PIPELINE_STAGE_TRANSFER_BIT},
        {snap::rhi::PipelineStageBits::BottomOfPipeBit, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT},
        {snap::rhi::PipelineStageBits::HostBit, VK_PIPELINE_STAGE_HOST_BIT},
        {snap::rhi::PipelineStageBits::AllGraphicsBit, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT},
        {snap::rhi::PipelineStageBits::AllCommandsBit, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT},
    });

    for (const auto& [rhiBit, vkBit] : stageMap) {
        if (static_cast<bool>(bits & rhiBit)) {
            result |= vkBit;
        }
    }

    return result;
}

VkSampleCountFlagBits toVkSampleCountFlagBits(const snap::rhi::SampleCount sampleCount) {
    switch (sampleCount) {
        case snap::rhi::SampleCount::Count1:
            return VK_SAMPLE_COUNT_1_BIT;

        case snap::rhi::SampleCount::Count2:
            return VK_SAMPLE_COUNT_2_BIT;

        case snap::rhi::SampleCount::Count4:
            return VK_SAMPLE_COUNT_4_BIT;

        case snap::rhi::SampleCount::Count8:
            return VK_SAMPLE_COUNT_8_BIT;

        case snap::rhi::SampleCount::Count16:
            return VK_SAMPLE_COUNT_16_BIT;

        case snap::rhi::SampleCount::Count32:
            return VK_SAMPLE_COUNT_32_BIT;

        case snap::rhi::SampleCount::Count64:
            return VK_SAMPLE_COUNT_64_BIT;

        default:
            snap::rhi::common::throwException("[getSampleCount] unsupported sample count");
    }
}

VkAttachmentLoadOp toVkAttachmentLoadOp(const snap::rhi::AttachmentLoadOp loadOp) {
    switch (loadOp) {
        case snap::rhi::AttachmentLoadOp::Load:
            return VK_ATTACHMENT_LOAD_OP_LOAD;

        case snap::rhi::AttachmentLoadOp::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;

        case snap::rhi::AttachmentLoadOp::DontCare:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;

        default:
            snap::rhi::common::throwException("[toVkAttachmentLoadOp] invalid AttachmentLoadOp: " +
                                              std::to_string(static_cast<uint32_t>(loadOp)));
    }
}

VkAttachmentStoreOp toVkAttachmentStoreOp(const snap::rhi::AttachmentStoreOp storeOp) {
    switch (storeOp) {
        case snap::rhi::AttachmentStoreOp::Store:
            return VK_ATTACHMENT_STORE_OP_STORE;

        case snap::rhi::AttachmentStoreOp::DontCare:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;

        default:
            snap::rhi::common::throwException("[toVkAttachmentStoreOp] invalid AttachmentStoreOp: " +
                                              std::to_string(static_cast<uint32_t>(storeOp)));
    }
}

VkAccessFlags toVkAccessFlags(const snap::rhi::AccessFlags bits) {
    VkAccessFlags result = 0;

    constexpr auto accessMap = std::to_array<std::pair<snap::rhi::AccessFlags, VkAccessFlagBits>>({
        {snap::rhi::AccessFlags::IndexRead, VK_ACCESS_INDEX_READ_BIT},
        {snap::rhi::AccessFlags::VertexAttributeRead, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT},
        {snap::rhi::AccessFlags::UniformRead, VK_ACCESS_UNIFORM_READ_BIT},
        //{snap::rhi::AccessFlags::InputAttachmentRead, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT},
        {snap::rhi::AccessFlags::ShaderRead, VK_ACCESS_SHADER_READ_BIT},
        {snap::rhi::AccessFlags::ShaderWrite, VK_ACCESS_SHADER_WRITE_BIT},
        {snap::rhi::AccessFlags::ColorAttachmentRead, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT},
        {snap::rhi::AccessFlags::ColorAttachmentWrite, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT},
        {snap::rhi::AccessFlags::DepthStencilAttachmentRead, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT},
        {snap::rhi::AccessFlags::DepthStencilAttachmentWrite, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
        {snap::rhi::AccessFlags::TransferRead, VK_ACCESS_TRANSFER_READ_BIT},
        {snap::rhi::AccessFlags::TransferWrite, VK_ACCESS_TRANSFER_WRITE_BIT},
        {snap::rhi::AccessFlags::HostRead, VK_ACCESS_HOST_READ_BIT},
        {snap::rhi::AccessFlags::HostWrite, VK_ACCESS_HOST_WRITE_BIT},
        {snap::rhi::AccessFlags::MemoryRead, VK_ACCESS_MEMORY_READ_BIT},
        {snap::rhi::AccessFlags::MemoryWrite, VK_ACCESS_MEMORY_WRITE_BIT},
    });

    for (const auto& [rhiBit, vkBit] : accessMap) {
        if (static_cast<bool>(bits & rhiBit)) {
            result |= vkBit;
        }
    }

    return result;
}

VkDependencyFlags toVkDependencyFlags(const snap::rhi::DependencyFlags bits) {
    VkDependencyFlags result = 0;

    constexpr auto dependencyMap = std::to_array<std::pair<snap::rhi::DependencyFlags, VkDependencyFlagBits>>({
        {snap::rhi::DependencyFlags::ByRegion, VK_DEPENDENCY_BY_REGION_BIT},
    });

    for (const auto& [rhiBit, vkBit] : dependencyMap) {
        if (static_cast<bool>(bits & rhiBit)) {
            result |= vkBit;
        }
    }

    return result;
}

VkVertexInputRate toVkVertexInputRate(const snap::rhi::VertexInputRate rate) {
    switch (rate) {
        case snap::rhi::VertexInputRate::PerVertex:
            return VK_VERTEX_INPUT_RATE_VERTEX;

        case snap::rhi::VertexInputRate::PerInstance:
            return VK_VERTEX_INPUT_RATE_INSTANCE;

        case snap::rhi::VertexInputRate::Constant:
            return VK_VERTEX_INPUT_RATE_VERTEX;

        default:
            snap::rhi::common::throwException("[toVkVertexInputRate] invalid VertexInputRate: " +
                                              std::to_string(static_cast<uint32_t>(rate)));
    }
}

VkFormat toVkVertexFormat(const snap::rhi::VertexAttributeFormat format) {
    switch (format) {
        case snap::rhi::VertexAttributeFormat::Float:
            return VK_FORMAT_R32_SFLOAT;

        case snap::rhi::VertexAttributeFormat::Float2:
            return VK_FORMAT_R32G32_SFLOAT;

        case snap::rhi::VertexAttributeFormat::Float3:
            return VK_FORMAT_R32G32B32_SFLOAT;

        case snap::rhi::VertexAttributeFormat::Float4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;

        default:
            snap::rhi::common::throwException("[toVkVertexFormat] invalid VertexFormat: " +
                                              std::to_string(static_cast<uint32_t>(format)));
    }
}

VkFormat toVkFormat(const snap::rhi::VertexAttributeFormat format) {
    switch (format) {
        case snap::rhi::VertexAttributeFormat::Float:
            return VK_FORMAT_R32_SFLOAT;

        case snap::rhi::VertexAttributeFormat::Float2:
            return VK_FORMAT_R32G32_SFLOAT;

        case snap::rhi::VertexAttributeFormat::Float3:
            return VK_FORMAT_R32G32B32_SFLOAT;

        case snap::rhi::VertexAttributeFormat::Float4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;

        case snap::rhi::VertexAttributeFormat::Int:
            return VK_FORMAT_R32_SINT;

        case snap::rhi::VertexAttributeFormat::Int2:
            return VK_FORMAT_R32G32_SINT;

        case snap::rhi::VertexAttributeFormat::Int3:
            return VK_FORMAT_R32G32B32_SINT;

        case snap::rhi::VertexAttributeFormat::Int4:
            return VK_FORMAT_R32G32B32A32_SINT;

        case snap::rhi::VertexAttributeFormat::UInt:
            return VK_FORMAT_R32_UINT;

        case snap::rhi::VertexAttributeFormat::UInt2:
            return VK_FORMAT_R32G32_UINT;

        case snap::rhi::VertexAttributeFormat::UInt3:
            return VK_FORMAT_R32G32B32_UINT;

        case snap::rhi::VertexAttributeFormat::UInt4:
            return VK_FORMAT_R32G32B32A32_UINT;

        case snap::rhi::VertexAttributeFormat::HalfFloat2:
            return VK_FORMAT_R16G16_SFLOAT;
        case snap::rhi::VertexAttributeFormat::HalfFloat3:
            return VK_FORMAT_R16G16B16_SFLOAT;
        case snap::rhi::VertexAttributeFormat::HalfFloat4:
            return VK_FORMAT_R16G16B16A16_SFLOAT;

        case snap::rhi::VertexAttributeFormat::Short2:
            return VK_FORMAT_R16G16_SINT;
        case snap::rhi::VertexAttributeFormat::Short3:
            return VK_FORMAT_R16G16B16_SINT;
        case snap::rhi::VertexAttributeFormat::Short4:
            return VK_FORMAT_R16G16B16A16_SINT;

        case snap::rhi::VertexAttributeFormat::UnsignedShort2:
            return VK_FORMAT_R16G16_UINT;
        case snap::rhi::VertexAttributeFormat::UnsignedShort3:
            return VK_FORMAT_R16G16B16_UINT;
        case snap::rhi::VertexAttributeFormat::UnsignedShort4:
            return VK_FORMAT_R16G16B16A16_UINT;

        case snap::rhi::VertexAttributeFormat::Short2Normalized:
            return VK_FORMAT_R16G16_SNORM;
        case snap::rhi::VertexAttributeFormat::Short3Normalized:
            return VK_FORMAT_R16G16B16_SNORM;
        case snap::rhi::VertexAttributeFormat::Short4Normalized:
            return VK_FORMAT_R16G16B16A16_SNORM;

        case snap::rhi::VertexAttributeFormat::UnsignedShort2Normalized:
            return VK_FORMAT_R16G16_UNORM;
        case snap::rhi::VertexAttributeFormat::UnsignedShort3Normalized:
            return VK_FORMAT_R16G16B16_UNORM;
        case snap::rhi::VertexAttributeFormat::UnsignedShort4Normalized:
            return VK_FORMAT_R16G16B16A16_UNORM;

        case snap::rhi::VertexAttributeFormat::Byte2:
            return VK_FORMAT_R8G8_SINT;
        case snap::rhi::VertexAttributeFormat::Byte3:
            return VK_FORMAT_R8G8B8_SINT;
        case snap::rhi::VertexAttributeFormat::Byte4:
            return VK_FORMAT_R8G8B8A8_SINT;

        case snap::rhi::VertexAttributeFormat::UnsignedByte2:
            return VK_FORMAT_R8G8_UINT;
        case snap::rhi::VertexAttributeFormat::UnsignedByte3:
            return VK_FORMAT_R8G8B8_UINT;
        case snap::rhi::VertexAttributeFormat::UnsignedByte4:
            return VK_FORMAT_R8G8B8A8_UINT;

        case snap::rhi::VertexAttributeFormat::Byte2Normalized:
            return VK_FORMAT_R8G8_SNORM;
        case snap::rhi::VertexAttributeFormat::Byte3Normalized:
            return VK_FORMAT_R8G8B8_SNORM;
        case snap::rhi::VertexAttributeFormat::Byte4Normalized:
            return VK_FORMAT_R8G8B8A8_SNORM;

        case snap::rhi::VertexAttributeFormat::UnsignedByte2Normalized:
            return VK_FORMAT_R8G8_UNORM;
        case snap::rhi::VertexAttributeFormat::UnsignedByte3Normalized:
            return VK_FORMAT_R8G8B8_UNORM;
        case snap::rhi::VertexAttributeFormat::UnsignedByte4Normalized:
            return VK_FORMAT_R8G8B8A8_UNORM;

        default:
            snap::rhi::common::throwException("[toVkFormat] invalid VertexFormat: " +
                                              std::to_string(static_cast<uint32_t>(format)));
    }
}

bool isStripPrimitiveTopology(const snap::rhi::Topology topology) {
    switch (topology) {
        case snap::rhi::Topology::LineStrip:
        case snap::rhi::Topology::TriangleStrip:
            return true;

        case snap::rhi::Topology::Points:
        case snap::rhi::Topology::Lines:
        case snap::rhi::Topology::Triangles:
            return false;

        default:
            snap::rhi::common::throwException("[isStripPrimitiveTopology] invalid Topology: " +
                                              std::to_string(static_cast<uint32_t>(topology)));
    }
}

VkPrimitiveTopology toVkPrimitiveTopology(const snap::rhi::Topology topology) {
    switch (topology) {
        case snap::rhi::Topology::Points:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        case snap::rhi::Topology::Lines:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

        case snap::rhi::Topology::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

        case snap::rhi::Topology::Triangles:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        case snap::rhi::Topology::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        default:
            snap::rhi::common::throwException("[toVkPrimitiveTopology] invalid Topology: " +
                                              std::to_string(static_cast<uint32_t>(topology)));
    }
}

VkPolygonMode toVkPolygonMode(const snap::rhi::PolygonMode mode) {
    switch (mode) {
        case snap::rhi::PolygonMode::Fill:
            return VK_POLYGON_MODE_FILL;

        case snap::rhi::PolygonMode::Line:
            return VK_POLYGON_MODE_LINE;

        default:
            snap::rhi::common::throwException("[toVkPolygonMode] invalid PolygonMode: " +
                                              std::to_string(static_cast<uint32_t>(mode)));
    }
}

VkCullModeFlags toVkCullModeFlags(const snap::rhi::CullMode mode) {
    switch (mode) {
        case snap::rhi::CullMode::Back:
            return VK_CULL_MODE_BACK_BIT;

        case snap::rhi::CullMode::Front:
            return VK_CULL_MODE_FRONT_BIT;

        case snap::rhi::CullMode::None:
            return VK_CULL_MODE_NONE;

        default:
            snap::rhi::common::throwException("[toVkCullModeFlags] invalid CullMode: " +
                                              std::to_string(static_cast<uint32_t>(mode)));
    }
}

VkFrontFace toVkFrontFace(const snap::rhi::Winding winding) {
    switch (winding) {
        case snap::rhi::Winding::CCW:
            return VK_FRONT_FACE_COUNTER_CLOCKWISE;

        case snap::rhi::Winding::CW:
            return VK_FRONT_FACE_CLOCKWISE;

        default:
            snap::rhi::common::throwException("[toVkFrontFace] invalid Winding: " +
                                              std::to_string(static_cast<uint32_t>(winding)));
    }
}

VkStencilOp toVkStencilOp(const snap::rhi::StencilOp op) {
    switch (op) {
        case snap::rhi::StencilOp::Keep:
            return VK_STENCIL_OP_KEEP;

        case snap::rhi::StencilOp::Zero:
            return VK_STENCIL_OP_ZERO;

        case snap::rhi::StencilOp::Replace:
            return VK_STENCIL_OP_REPLACE;

        case snap::rhi::StencilOp::IncAndClamp:
            return VK_STENCIL_OP_INCREMENT_AND_CLAMP;

        case snap::rhi::StencilOp::DecAndClamp:
            return VK_STENCIL_OP_DECREMENT_AND_CLAMP;

        case snap::rhi::StencilOp::Invert:
            return VK_STENCIL_OP_INVERT;

        case snap::rhi::StencilOp::IncAndWrap:
            return VK_STENCIL_OP_INCREMENT_AND_WRAP;

        case snap::rhi::StencilOp::DecAndWrap:
            return VK_STENCIL_OP_DECREMENT_AND_WRAP;

        default:
            snap::rhi::common::throwException("[toVkStencilOp] invalid StencilOp: " +
                                              std::to_string(static_cast<uint32_t>(op)));
    }
}

VkBlendFactor toVkBlendFactor(const snap::rhi::BlendFactor factor) {
    switch (factor) {
        case snap::rhi::BlendFactor::Zero:
            return VK_BLEND_FACTOR_ZERO;

        case snap::rhi::BlendFactor::One:
            return VK_BLEND_FACTOR_ONE;

        case snap::rhi::BlendFactor::SrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;

        case snap::rhi::BlendFactor::OneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;

        case snap::rhi::BlendFactor::DstColor:
            return VK_BLEND_FACTOR_DST_COLOR;

        case snap::rhi::BlendFactor::OneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;

        case snap::rhi::BlendFactor::SrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;

        case snap::rhi::BlendFactor::OneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        case snap::rhi::BlendFactor::DstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;

        case snap::rhi::BlendFactor::OneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;

        case snap::rhi::BlendFactor::ConstantColor:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;

        case snap::rhi::BlendFactor::OneMinusConstantColor:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;

        case snap::rhi::BlendFactor::ConstantAlpha:
            return VK_BLEND_FACTOR_CONSTANT_ALPHA;

        case snap::rhi::BlendFactor::OneMinusConstantAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;

        case snap::rhi::BlendFactor::SrcAlphaSaturate:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;

        default:
            snap::rhi::common::throwException("[toVkBlendFactor] invalid BlendFactor: " +
                                              std::to_string(static_cast<uint32_t>(factor)));
    }
}

VkBlendOp toVkBlendOp(const snap::rhi::BlendOp operation) {
    switch (operation) {
        case snap::rhi::BlendOp::Add:
            return VK_BLEND_OP_ADD;

        case snap::rhi::BlendOp::Subtract:
            return VK_BLEND_OP_SUBTRACT;

        case snap::rhi::BlendOp::ReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;

        case snap::rhi::BlendOp::Min:
            return VK_BLEND_OP_MIN;

        case snap::rhi::BlendOp::Max:
            return VK_BLEND_OP_MAX;

        default:
            snap::rhi::common::throwException("[toVkBlendOp] invalid BlendOp: " +
                                              std::to_string(static_cast<uint32_t>(operation)));
    }
}

VkStencilFaceFlags toVkStencilFaceFlags(const snap::rhi::StencilFace face) {
    switch (face) {
        case snap::rhi::StencilFace::Front:
            return VK_STENCIL_FACE_FRONT_BIT;

        case snap::rhi::StencilFace::Back:
            return VK_STENCIL_FACE_BACK_BIT;

        case snap::rhi::StencilFace::FrontAndBack:
            return VK_STENCIL_FACE_FRONT_AND_BACK;

        default:
            snap::rhi::common::throwException("[toVkStencilFaceFlags] invalid StencilFace: " +
                                              std::to_string(static_cast<uint32_t>(face)));
    }
}

VkIndexType toVkIndexType(const snap::rhi::IndexType indexType) {
    switch (indexType) {
        case snap::rhi::IndexType::UInt16:
            return VK_INDEX_TYPE_UINT16;

        case snap::rhi::IndexType::UInt32:
            return VK_INDEX_TYPE_UINT32;

        default:
            snap::rhi::common::throwException("[toVkIndexType] invalid IndexType: " +
                                              std::to_string(static_cast<uint32_t>(indexType)));
    }
}

VkImageCreateFlags toVkImageCreateFlags(snap::rhi::TextureType textureType) {
    if (textureType == snap::rhi::TextureType::TextureCubemap) {
        return VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    return 0;
}

VkImageType toVkImageType(snap::rhi::TextureType textureType) {
    switch (textureType) {
        case snap::rhi::TextureType::Texture2D:
        case snap::rhi::TextureType::Texture2DArray:
        case snap::rhi::TextureType::TextureCubemap:
            return VK_IMAGE_TYPE_2D;

        case snap::rhi::TextureType::Texture3D:
            return VK_IMAGE_TYPE_3D;

        default:
            snap::rhi::common::throwException("[getImageType] unsupported image type");
    }
}

VkImageUsageFlags toVkImageUsageFlags(const snap::rhi::TextureUsage usage) {
    VkImageUsageFlags imageUsage = 0;

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::Sampled)) {
        imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::Storage)) {
        imageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::ColorAttachment)) {
        imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::DepthStencilAttachment)) {
        imageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::TransferSrc)) {
        imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    if (static_cast<bool>(usage & snap::rhi::TextureUsage::TransferDst)) {
        imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    return imageUsage;
}

std::vector<EntryPointReflection> buildEntryPointReflection(std::span<const uint32_t> code,
                                                            const SpvReflectShaderModule& spvModule) {
    auto specInfos = buildSpecializationInfos(code);

    std::vector<EntryPointReflection> result;
    uint32_t entryPointCount = spvModule.entry_point_count;
    result.reserve(entryPointCount);

    // Shared helper to flatten a block (top-level + nested struct/array members)
    auto buildBlockMembers = [](const SpvReflectBlockVariable& block) {
        std::vector<snap::rhi::reflection::BufferMemberInfo> members;
        members.reserve(block.member_count * 4); // heuristic reserve

        // Top-level members first
        for (uint32_t i = 0; i < block.member_count; ++i) {
            const SpvReflectBlockVariable& m = block.members[i];
            snap::rhi::reflection::BufferMemberInfo info{};
            info.name = m.name ? m.name : (std::string("member") + std::to_string(i));
            info.offset = m.offset;
            info.stride = m.padded_size;
            info.arraySize = m.array.dims_count ? m.array.dims[0] : 0;
            info.format = mapUniformFormat(m.type_description);
            members.push_back(info);
        }

        struct StackEntry {
            const SpvReflectBlockVariable* var;
            std::string name;
            uint32_t baseOffset;
        };
        std::vector<StackEntry> stack;
        stack.reserve(block.member_count * 4);

        for (uint32_t i = 0; i < block.member_count; ++i) {
            const SpvReflectBlockVariable& m = block.members[i];
            if (m.member_count > 0 || (m.array.dims_count > 0 && m.array.dims[0] > 0)) {
                stack.push_back({&m, m.name ? m.name : (std::string("member") + std::to_string(i)), m.offset});
            }
        }

        // Helper lambdas
        auto pushLeaf = [&](const SpvReflectBlockVariable& var, const std::string& fullName, uint32_t offset) {
            snap::rhi::reflection::BufferMemberInfo leaf{};
            leaf.name = fullName;
            leaf.offset = offset;
            leaf.stride = var.padded_size;
            leaf.arraySize = var.array.dims_count ? var.array.dims[0] : 0; // report first dim size for legacy field
            leaf.format = mapUniformFormat(var.type_description);
            members.push_back(leaf);
        };

        // Recursively expand multi-dimensional array preserving each dimension in name.
        // We only have top-level stride from SPIRV-Reflect (stride between elements of dim0). For inner dimensions
        // SPIR-V arr-of-arr layout implies stride(innerDimIdx) = stride(parent) / dimSize(parentIdx) (heuristic).
        std::function<void(const SpvReflectBlockVariable&,
                           uint32_t /*dimIdx*/,
                           uint32_t /*baseOffset*/,
                           const std::string& /*baseName*/,
                           uint32_t /*currentStride*/)>
            expandMultiDim;
        expandMultiDim = [&](const SpvReflectBlockVariable& var,
                             uint32_t dimIdx,
                             uint32_t baseOffset,
                             const std::string& baseName,
                             uint32_t currentStride) {
            if (dimIdx >= var.array.dims_count) {
                // Reached element, if struct expand members, else leaf
                if (var.member_count > 0) {
                    for (uint32_t m = 0; m < var.member_count; ++m) {
                        const auto& child = var.members[m];
                        std::string childName =
                            baseName + "." + (child.name ? child.name : (std::string("member") + std::to_string(m)));
                        uint32_t childOffset = baseOffset + child.offset;
                        if (child.array.dims_count > 0) {
                            // Child itself an array
                            if (child.member_count > 0 || child.array.dims_count > 0) {
                                if (child.array.dims_count > 0) {
                                    uint32_t stride0 = child.array.stride ? child.array.stride : child.padded_size;
                                    expandMultiDim(child, 0, childOffset, childName, stride0);
                                    continue;
                                }
                            }
                        }
                        if (child.member_count > 0) {
                            // Non-array struct member
                            stack.push_back({&child, childName, childOffset});
                        } else {
                            pushLeaf(child, childName, childOffset);
                        }
                    }
                } else {
                    pushLeaf(var, baseName, baseOffset);
                }
                return;
            }

            uint32_t dimSize = var.array.dims[dimIdx];
            if (dimSize == 0) {
                return;
            }
            // Derive next stride heuristically
            uint32_t nextStride = currentStride;
            if (dimSize > 0) {
                // Avoid divide-by-zero; if not divisible keep same stride (fallback)
                if (dimIdx + 1 < var.array.dims_count && (currentStride % dimSize) == 0) {
                    nextStride = currentStride / dimSize;
                }
            }
            for (uint32_t idx = 0; idx < dimSize; ++idx) {
                uint32_t elementOffset = baseOffset + idx * currentStride;
                std::string elementName = baseName + "[" + std::to_string(idx) + "]";
                expandMultiDim(var, dimIdx + 1, elementOffset, elementName, nextStride);
            }
        };

        while (!stack.empty()) {
            StackEntry current = stack.back();
            stack.pop_back();
            const SpvReflectBlockVariable& var = *current.var;
            const bool isStruct = var.member_count > 0;
            const bool isArray = var.array.dims_count > 0 && var.array.dims[0] > 0;

            if (isArray) {
                uint32_t stride0 = var.array.stride ? var.array.stride : var.padded_size; // fallback
                expandMultiDim(var, 0, current.baseOffset, current.name, stride0);
            } else if (isStruct) {
                for (uint32_t m = 0; m < var.member_count; ++m) {
                    const SpvReflectBlockVariable& child = var.members[m];
                    std::string childName =
                        current.name + "." + (child.name ? child.name : (std::string("member") + std::to_string(m)));
                    uint32_t childOffset = current.baseOffset + child.offset;
                    if (child.member_count > 0 || (child.array.dims_count > 0 && child.array.dims[0] > 0)) {
                        stack.push_back({&child, childName, childOffset});
                    } else {
                        pushLeaf(child, childName, childOffset);
                    }
                }
            } else { // Plain scalar/vector nested
                pushLeaf(var, current.name, current.baseOffset);
            }
        }

        std::ranges::sort(members, std::ranges::less{}, &snap::rhi::reflection::BufferMemberInfo::offset);
        return members;
    };

    for (uint32_t epIndex = 0; epIndex < entryPointCount; ++epIndex) {
        const SpvReflectEntryPoint& spvEntryPoint = spvModule.entry_points[epIndex];
        EntryPointReflection reflection{};

        reflection.entryPointInfo.name = spvEntryPoint.name ? spvEntryPoint.name : "";
        switch (spvEntryPoint.shader_stage) {
            case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
                reflection.entryPointInfo.stage = snap::rhi::ShaderStage::Vertex;
                break;
            case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
                reflection.entryPointInfo.stage = snap::rhi::ShaderStage::Fragment;
                break;
            case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
                reflection.entryPointInfo.stage = snap::rhi::ShaderStage::Compute;
                break;
            default:
                reflection.entryPointInfo.stage = snap::rhi::ShaderStage::Vertex;
                break;
        }

        reflection.entryPointInfo.specializationsInfo = specInfos;

        // Vertex Input Attributes
        if (spvEntryPoint.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
            uint32_t inputVarCount = 0;
            spvReflectEnumerateEntryPointInputVariables(&spvModule, spvEntryPoint.name, &inputVarCount, nullptr);
            if (inputVarCount) {
                std::vector<SpvReflectInterfaceVariable*> inputVars(inputVarCount);
                spvReflectEnumerateEntryPointInputVariables(
                    &spvModule, spvEntryPoint.name, &inputVarCount, inputVars.data());
                for (auto* var : inputVars) {
                    if (!var)
                        continue;
                    if (var->built_in != -1)
                        continue;
                    snap::rhi::reflection::VertexAttributeInfo attr{};
                    attr.name = var->name ? var->name : "";
                    attr.location = var->location;
                    attr.format = mapInputFormat(var);
                    reflection.vertexAttributes.push_back(attr);
                }
            }
        }

        // Descriptor Sets
        uint32_t descriptorSetCount = 0;
        spvReflectEnumerateDescriptorSets(&spvModule, &descriptorSetCount, nullptr);
        if (descriptorSetCount) {
            std::vector<SpvReflectDescriptorSet*> spvSets(descriptorSetCount);
            spvReflectEnumerateDescriptorSets(&spvModule, &descriptorSetCount, spvSets.data());
            for (auto* spvSet : spvSets) {
                if (!spvSet)
                    continue;
                snap::rhi::reflection::DescriptorSetInfo setInfo{};
                setInfo.binding = spvSet->set;
                std::unordered_set<std::string> processedNames;

                for (uint32_t bindingIndex = 0; bindingIndex < spvSet->binding_count; ++bindingIndex) {
                    const SpvReflectDescriptorBinding* binding = spvSet->bindings[bindingIndex];
                    if (!binding)
                        continue;
                    const char* bindingName = binding->name ? binding->name : "";
                    if (processedNames.count(bindingName))
                        continue;
                    processedNames.insert(bindingName);

                    switch (binding->descriptor_type) {
                        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
                            snap::rhi::reflection::UniformBufferInfo ubo{};
                            ubo.name = bindingName;
                            ubo.binding = binding->binding;
                            ubo.size = binding->block.size;
                            if (binding->block.member_count) {
                                ubo.membersInfo = buildBlockMembers(binding->block);
                            }
                            setInfo.uniformBuffers.push_back(ubo);
                        } break;
                        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                            snap::rhi::reflection::StorageBufferInfo sb{};
                            sb.name = bindingName;
                            sb.binding = binding->binding;
                            sb.size = binding->block.size;
                            sb.access = mapStorageBufferAccess(binding);
                            if (binding->block.member_count) {
                                sb.membersInfo = buildBlockMembers(binding->block);
                            }
                            setInfo.storageBuffers.push_back(sb);
                        } break;
                        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
                            snap::rhi::reflection::SampledTextureInfo tex{};
                            tex.name = bindingName;
                            tex.binding = binding->binding;
                            tex.type = mapTextureType(binding);
                            setInfo.sampledTextures.push_back(tex);
                        } break;
                        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
                            snap::rhi::reflection::StorageTextureInfo st{};
                            st.name = bindingName;
                            st.binding = binding->binding;
                            st.type = mapTextureType(binding);
                            st.access = mapImageAccess(binding);
                            setInfo.storageTextures.push_back(st);
                        } break;
                        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: {
                            snap::rhi::reflection::SamplerInfo si{};
                            si.name = bindingName;
                            si.binding = binding->binding;
                            si.type = snap::rhi::reflection::SamplerType::Sampler;
                            setInfo.samplers.push_back(si);
                        } break;
                        default:
                            break;
                    }
                }

                reflection.descriptorSetInfo.push_back(setInfo);
            }
        }

        result.push_back(std::move(reflection));
    }

    return result;
}

VkComponentSwizzle toVkComponentSwizzle(const snap::rhi::ComponentSwizzle swizzle) {
    switch (swizzle) {
        case snap::rhi::ComponentSwizzle::Zero:
            return VK_COMPONENT_SWIZZLE_ZERO;
        case snap::rhi::ComponentSwizzle::One:
            return VK_COMPONENT_SWIZZLE_ONE;
        case snap::rhi::ComponentSwizzle::R:
            return VK_COMPONENT_SWIZZLE_R;
        case snap::rhi::ComponentSwizzle::G:
            return VK_COMPONENT_SWIZZLE_G;
        case snap::rhi::ComponentSwizzle::B:
            return VK_COMPONENT_SWIZZLE_B;
        case snap::rhi::ComponentSwizzle::A:
            return VK_COMPONENT_SWIZZLE_A;
        default:
            return VK_COMPONENT_SWIZZLE_IDENTITY;
    }
}

VkComponentMapping toVkComponentMapping(const snap::rhi::ComponentMapping& mapping,
                                        const snap::rhi::PixelFormat viewFormat) {
    if (viewFormat == snap::rhi::PixelFormat::Grayscale) {
        static constexpr VkComponentMapping GrayscaleMapping{.r = VK_COMPONENT_SWIZZLE_R,
                                                             .g = VK_COMPONENT_SWIZZLE_R,
                                                             .b = VK_COMPONENT_SWIZZLE_R,
                                                             .a = VK_COMPONENT_SWIZZLE_ONE};
        return GrayscaleMapping;
    }

    const VkComponentMapping componentMapping{
        .r = toVkComponentSwizzle(mapping.r),
        .g = toVkComponentSwizzle(mapping.g),
        .b = toVkComponentSwizzle(mapping.b),
        .a = toVkComponentSwizzle(mapping.a),
    };
    return componentMapping;
}

VkImageViewType getImageViewType(const snap::rhi::TextureType textureType) {
    switch (textureType) {
        case snap::rhi::TextureType::Texture2D:
            return VK_IMAGE_VIEW_TYPE_2D;

        case snap::rhi::TextureType::Texture2DArray:
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

        case snap::rhi::TextureType::TextureCubemap:
            return VK_IMAGE_VIEW_TYPE_CUBE;

        case snap::rhi::TextureType::Texture3D:
            return VK_IMAGE_VIEW_TYPE_3D;

        default:
            snap::rhi::common::throwException("[getImageType] getImageViewType image type");
    }
}

VkImageAspectFlags getImageAspect(const snap::rhi::PixelFormat format) {
    VkImageAspectFlags result = 0;
    snap::rhi::DepthStencilFormatTraits info = snap::rhi::getDepthStencilFormatTraits(format);

    if (static_cast<bool>(info & snap::rhi::DepthStencilFormatTraits::HasDepthAspect)) {
        result |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    if (static_cast<bool>(info & snap::rhi::DepthStencilFormatTraits::HasStencilAspect)) {
        result |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if (!result) {
        result = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    return result;
}

VkImageView resolveImageView(const Texture* texture, const snap::rhi::TextureSubresourceRange& range) {
    const auto& imageViewCache = texture->getImageViewCache();
    const auto baseInfo = texture->getCreateInfo();
    const auto* vkDevice = snap::rhi::backend::common::smart_cast<const Device>(texture->getDevice());

    snap::rhi::TextureType viewType = baseInfo.textureType;
    if (baseInfo.textureType == snap::rhi::TextureType::Texture3D && range.layerCount == 1 &&
        vkDevice->supportsImageView2DOn3DImage()) {
        viewType = snap::rhi::TextureType::Texture2D;
    }

    const auto key = buildTextureViewInfo(baseInfo, range, baseInfo.format, viewType, baseInfo.components);
    const VkImageView imageView = imageViewCache->acquire(key);
    return imageView;
}

snap::rhi::TextureViewInfo buildTextureViewInfo(const snap::rhi::TextureCreateInfo& info,
                                                const snap::rhi::TextureSubresourceRange& range,
                                                snap::rhi::PixelFormat viewFormat,
                                                snap::rhi::TextureType viewType,
                                                const snap::rhi::ComponentMapping& components) {
    snap::rhi::TextureViewInfo view{
        .format = viewFormat,
        .textureType = viewType,
        .range = range,
        .components = components,
    };

    // For 3D images, Vulkan requires baseArrayLayer==0 and layerCount==1.
    // For 2D-array/cubemap, layerCount is the number of layers/faces exposed by the view.
    if (viewType == snap::rhi::TextureType::Texture3D) {
        view.range.baseArrayLayer = 0;
        view.range.layerCount = 1;
    } else {
        view.range.layerCount = range.layerCount;
    }

    return view;
}
} // namespace snap::rhi::backend::vulkan
