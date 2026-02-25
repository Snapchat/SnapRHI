// Copyright © 2024 Snap, Inc. All rights reserved.

#pragma once

#include "snap/rhi/Enums.h"

namespace snap::rhi {
class Buffer;
class Texture;
class Sampler;
class Device;

/**
 * @brief Descriptor payload for a sampler binding (`DescriptorType::Sampler`).
 */
struct DescriptorSamplerInfo {
    /** @brief Sampler object to bind. */
    snap::rhi::Sampler* sampler = nullptr;

    /**
     * @brief Defaulted three-way comparison.
     *
     * Useful for caching and deduplication of descriptor write data.
     */
    constexpr friend auto operator<=>(const DescriptorSamplerInfo&, const DescriptorSamplerInfo&) noexcept = default;
};

/**
 * @brief Descriptor payload for a sampled texture binding (`DescriptorType::SampledTexture`).
 */
struct DescriptorSampledTextureInfo {
    /** @brief Texture object to bind as a sampled image. */
    snap::rhi::Texture* texture = nullptr;

    constexpr friend auto operator<=>(const DescriptorSampledTextureInfo&,
                                      const DescriptorSampledTextureInfo&) noexcept = default;
};

/**
 * @brief Descriptor payload for a storage texture binding (`DescriptorType::StorageTexture`).
 */
struct DescriptorStorageTextureInfo {
    /** @brief Texture object to bind as a storage image. */
    snap::rhi::Texture* texture = nullptr;

    /**
     * @brief Mip level to bind.
     *
     * @note Backend notes:
     * - OpenGL uses this value when binding images (see image binding paths).
     * - Vulkan currently binds the full image view; the mip level may be ignored depending on the backend
     *   implementation.
     */
    uint32_t mipLevel = 0;

    constexpr friend auto operator<=>(const DescriptorStorageTextureInfo&,
                                      const DescriptorStorageTextureInfo&) noexcept = default;
};

/**
 * @brief Descriptor payload for buffer bindings (`DescriptorType::UniformBuffer` / `DescriptorType::StorageBuffer`).
 */
struct DescriptorBufferInfo {
    /** @brief Buffer object to bind. */
    snap::rhi::Buffer* buffer = nullptr;

    /**
     * @brief Byte offset into the buffer.
     *
     * @note Backend constraints:
     * - OpenGL validates uniform buffer offset alignment against device capabilities.
     */
    uint64_t offset = 0;

    /**
     * @brief Byte range of the bound region.
     *
     * Use `snap::rhi::WholeSize` where supported to indicate "until end of buffer".
     */
    uint64_t range = 0;

    constexpr friend auto operator<=>(const DescriptorBufferInfo&, const DescriptorBufferInfo&) noexcept = default;
};

/**
 * @brief Generic descriptor write description.
 *
 * This is a backend-agnostic representation of a single descriptor write/update. It is used as input to
 * `DescriptorSet::updateDescriptorSet()`.
 *
 * The active @ref descriptorType determines which `*Info` member is read:
 * - `DescriptorType::Sampler`        -> @ref samplerInfo
 * - `DescriptorType::SampledTexture` -> @ref sampledTextureInfo
 * - `DescriptorType::StorageTexture` -> @ref storageTextureInfo
 * - `DescriptorType::UniformBuffer` / `DescriptorType::StorageBuffer` -> @ref bufferInfo
 *
 * @note Backend mapping
 * - Vulkan maps these writes to `VkWriteDescriptorSet` / `vkUpdateDescriptorSets`.
 * - OpenGL stores these values inside its descriptor set implementation and applies them when binding pipeline state.
 */
struct Descriptor {
    uint32_t binding = 0;
    snap::rhi::DescriptorType descriptorType = snap::rhi::DescriptorType::Undefined;

    DescriptorSamplerInfo samplerInfo{};
    DescriptorSampledTextureInfo sampledTextureInfo{};
    DescriptorStorageTextureInfo storageTextureInfo{};
    DescriptorBufferInfo bufferInfo{};

    constexpr friend auto operator<=>(const Descriptor&, const Descriptor&) noexcept = default;
};
} // namespace snap::rhi
