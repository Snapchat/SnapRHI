#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/SpecializationConstantFormat.h"
#include "snap/rhi/VertexAttributeFormat.h"
#include "snap/rhi/reflection/Enums.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace snap::rhi::reflection {
struct VertexAttributeInfo {
    std::string name;
    uint32_t location = Undefined;
    snap::rhi::VertexAttributeFormat format = snap::rhi::VertexAttributeFormat::Undefined;

    friend bool operator==(const snap::rhi::reflection::VertexAttributeInfo& lhs,
                           const snap::rhi::reflection::VertexAttributeInfo& rhs) noexcept {
        return lhs.name == rhs.name && lhs.location == rhs.location && lhs.format == rhs.format;
    }

    friend bool operator<(const snap::rhi::reflection::VertexAttributeInfo& lhs,
                          const snap::rhi::reflection::VertexAttributeInfo& rhs) noexcept {
        return lhs.name < rhs.name;
    }
};

/**
 * Note: For non-array uniform @arraySize should be 0.
 */
struct BufferMemberInfo {
    std::string name;
    uint32_t offset = 0;
    uint32_t stride = 0;
    uint32_t arraySize = 0;
    snap::rhi::reflection::UniformFormat format = snap::rhi::reflection::UniformFormat::Undefined;

    friend bool operator==(const snap::rhi::reflection::BufferMemberInfo& lhs,
                           const snap::rhi::reflection::BufferMemberInfo& rhs) noexcept {
        return lhs.name == rhs.name && lhs.offset == rhs.offset && lhs.arraySize == rhs.arraySize &&
               lhs.stride == rhs.stride && lhs.format == rhs.format;
    }
};

struct UniformBufferInfo {
    std::string name;
    uint32_t binding = snap::rhi::Undefined;
    uint32_t size = 0;

    std::vector<BufferMemberInfo> membersInfo;

    friend bool operator==(const snap::rhi::reflection::UniformBufferInfo& lhs,
                           const snap::rhi::reflection::UniformBufferInfo& rhs) noexcept {
        return lhs.name == rhs.name && lhs.binding == rhs.binding && lhs.size == rhs.size &&
               lhs.membersInfo == rhs.membersInfo;
    }

    friend bool operator<(const snap::rhi::reflection::UniformBufferInfo& lhs,
                          const snap::rhi::reflection::UniformBufferInfo& rhs) noexcept {
        return lhs.name < rhs.name;
    }
};

struct StorageBufferInfo {
    std::string name;
    uint32_t binding = snap::rhi::Undefined;
    uint32_t size = 0;
    snap::rhi::reflection::StorageBufferAccess access = snap::rhi::reflection::StorageBufferAccess::Undefined;

    std::vector<BufferMemberInfo> membersInfo;

    friend bool operator==(const snap::rhi::reflection::StorageBufferInfo& lhs,
                           const snap::rhi::reflection::StorageBufferInfo& rhs) noexcept {
        return lhs.name == rhs.name && lhs.binding == rhs.binding && lhs.size == rhs.size &&
               lhs.membersInfo == rhs.membersInfo && lhs.access == rhs.access;
    }

    friend bool operator<(const snap::rhi::reflection::StorageBufferInfo& lhs,
                          const snap::rhi::reflection::StorageBufferInfo& rhs) noexcept {
        return lhs.name < rhs.name;
    }
};

struct SampledTextureInfo {
    std::string name;
    uint32_t binding = snap::rhi::Undefined;
    snap::rhi::reflection::TextureType type = snap::rhi::reflection::TextureType::Undefined;

    friend bool operator==(const snap::rhi::reflection::SampledTextureInfo& lhs,
                           const snap::rhi::reflection::SampledTextureInfo& rhs) noexcept {
        return lhs.name == rhs.name && lhs.binding == rhs.binding && lhs.type == rhs.type;
    }

    friend bool operator<(const snap::rhi::reflection::SampledTextureInfo& lhs,
                          const snap::rhi::reflection::SampledTextureInfo& rhs) noexcept {
        return lhs.name < rhs.name;
    }
};

struct StorageTextureInfo {
    std::string name;
    uint32_t binding = snap::rhi::Undefined;
    snap::rhi::reflection::TextureType type = snap::rhi::reflection::TextureType::Undefined;
    snap::rhi::reflection::ImageAccess access = snap::rhi::reflection::ImageAccess::Undefined;

    friend bool operator==(const snap::rhi::reflection::StorageTextureInfo& lhs,
                           const snap::rhi::reflection::StorageTextureInfo& rhs) noexcept {
        return lhs.name == rhs.name && lhs.binding == rhs.binding && lhs.type == rhs.type;
    }

    friend bool operator<(const snap::rhi::reflection::StorageTextureInfo& lhs,
                          const snap::rhi::reflection::StorageTextureInfo& rhs) noexcept {
        return lhs.name < rhs.name;
    }
};

struct SamplerInfo {
    std::string name;
    uint32_t binding = snap::rhi::Undefined;
    snap::rhi::reflection::SamplerType type = snap::rhi::reflection::SamplerType::Undefined;

    friend bool operator==(const snap::rhi::reflection::SamplerInfo& lhs,
                           const snap::rhi::reflection::SamplerInfo& rhs) noexcept {
        return lhs.name == rhs.name && lhs.binding == rhs.binding && lhs.type == rhs.type;
    }

    friend bool operator<(const snap::rhi::reflection::SamplerInfo& lhs,
                          const snap::rhi::reflection::SamplerInfo& rhs) noexcept {
        return lhs.name < rhs.name;
    }
};

struct SpecializationInfo {
    std::string name;
    uint32_t constantID = snap::rhi::Undefined;
    SpecializationConstantFormat format = SpecializationConstantFormat::Undefined;
    bool required = false;

    friend bool operator==(const snap::rhi::reflection::SpecializationInfo& lhs,
                           const snap::rhi::reflection::SpecializationInfo& rhs) noexcept {
        return lhs.name == rhs.name && lhs.constantID == rhs.constantID && lhs.format == rhs.format &&
               lhs.required == rhs.required;
    }
};

struct EntryPointInfo {
    std::string name;
    std::vector<SpecializationInfo> specializationsInfo{};
    snap::rhi::ShaderStage stage = snap::rhi::ShaderStage::Vertex;

    friend bool operator==(const snap::rhi::reflection::EntryPointInfo& lhs,
                           const snap::rhi::reflection::EntryPointInfo& rhs) noexcept {
        return lhs.name == rhs.name && lhs.specializationsInfo == rhs.specializationsInfo && lhs.stage == rhs.stage;
    }
};

/**
 * Note: SnapRHI should provide reflection for each element of the array.
 */
struct DescriptorSetInfo {
    uint32_t binding = 0;

    std::vector<UniformBufferInfo> uniformBuffers;
    std::vector<StorageBufferInfo> storageBuffers;

    std::vector<SampledTextureInfo> sampledTextures;
    std::vector<StorageTextureInfo> storageTextures;

    std::vector<SamplerInfo> samplers;

    friend bool operator==(const snap::rhi::reflection::DescriptorSetInfo& lhs,
                           const snap::rhi::reflection::DescriptorSetInfo& rhs) noexcept {
        return lhs.uniformBuffers == rhs.uniformBuffers && lhs.storageBuffers == rhs.storageBuffers &&
               lhs.sampledTextures == rhs.sampledTextures && lhs.storageTextures == rhs.storageTextures &&
               lhs.samplers == rhs.samplers && lhs.binding == rhs.binding;
    }
};
} // namespace snap::rhi::reflection
