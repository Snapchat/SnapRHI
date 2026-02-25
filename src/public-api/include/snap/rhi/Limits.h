//
//  Limits.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 22.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <cstdint>
#include <limits>

namespace snap::rhi {
/**
 * @name Sentinel values
 * @brief Common sentinel values used by the SnapRHI public API.
 *
 * These constants are API-agnostic and are used in create infos / descriptors to represent a special meaning
 * ("unspecified" or "no practical limit").
 */

/**
 * @brief Sentinel value meaning "undefined" / "not specified".
 *
 * This is typically used for fields where `0` is a valid value and an explicit sentinel is required.
 */
static constexpr uint32_t Undefined = std::numeric_limits<uint32_t>::max();

/**
 * @brief Sentinel value meaning "unlimited".
 *
 * Indicates there is no explicit application-level limit (backend limits may still apply).
 */
static constexpr uint32_t Unlimited = std::numeric_limits<uint32_t>::max();

/**
 * @name General fixed limits
 * @brief Conservative upper bounds used by SnapRHI data structures.
 *
 * These are primarily used to size internal arrays and validate create infos. They are not intended to reflect the
 * maximum capability of any particular backend/device.
 */

/// @brief Maximum number of memory types supported by SnapRHI devices.
static constexpr uint32_t MaxMemoryTypes = 32u;

/// @brief Maximum number of dynamic offsets per descriptor set.
static constexpr uint32_t MaxDynamicOffsetsPerDescriptorSet = 8u;

/// @brief Maximum number of vertex attributes supported by SnapRHI pipelines.
static constexpr uint32_t MaxVertexAttributes = 32u;

/// @brief Maximum size (in bytes) of debug marker names.
static constexpr uint32_t MaxDebugMarkerNameSize = 32u;

/// @brief Maximum number of vertex attributes sourced from a single vertex buffer binding.
static constexpr uint32_t MaxVertexAttributesPerBuffer = 16u;

/// @brief Maximum number of vertex buffer bindings.
static constexpr uint32_t MaxVertexBuffers = 8u;

/// @brief Maximum number of color attachments supported by a render pass/subpass.
static constexpr uint32_t MaxColorAttachments = 8u;

/// @brief Maximum number of subpasses supported by SnapRHI render passes.
///
/// @note Some backends may support more subpasses, but the API currently targets a single-subpass model.
static constexpr uint32_t MaxSubpasses = 1u;

/// @brief Maximum number of inter-subpass dependencies (derived from @ref MaxSubpasses).
static constexpr uint32_t MaxDependencies = MaxSubpasses * (MaxSubpasses - 1);

/// @brief Maximum number of attachments in a render pass.
///
/// This includes color + depth/stencil, and their optional resolve/MSAA counterparts.
static constexpr uint32_t MaxAttachments =
    (MaxColorAttachments + 1u) * 2u; // color + depth/stencil, each potentially with resolve

namespace SupportedLimit {
/**
 * @name Capability clamping limits
 * @brief Upper bounds used when reporting or normalizing device capabilities.
 *
 * These values are intended to be API-agnostic "supported ceilings" for SnapRHI. A backend/device may report
 * higher native limits, but SnapRHI may clamp to the values below for portability and to keep internal
 * representations compact.
 */

/// @brief Maximum number of queue families SnapRHI exposes.
static constexpr uint32_t MaxQueueFamilies = 4u;

/// @brief Maximum number of descriptor sets that can be bound at once.
///
/// Mirrors the common cross-API concept of a small number of bind groups / descriptor set slots.
static constexpr uint32_t MaxBoundDescriptorSets = 4u;

/// @brief Maximum number of specialization constants supported by the API.
static constexpr uint32_t MaxSpecializationConstants = 65535u;

/// @brief Maximum number of uniform buffers per shader stage.
///
/// Chosen primarily for portability across constrained APIs.
static constexpr uint32_t MaxPerStageUniformBuffers = 12u;

/// @brief Maximum total number of uniform buffers across all stages.
static constexpr uint32_t MaxUniformBuffers = 24u;

/// @brief Maximum size (in bytes) of a single vertex shader uniform buffer binding, that guarantees portability..
static constexpr uint64_t MaxVertexShaderUniformBufferSize = 2048u;

/// @brief Maximum size (in bytes) of a single fragment shader uniform buffer binding, that guarantees portability..
static constexpr uint64_t MaxFragmentShaderUniformBufferSize = 256u;

/// @brief Maximum size (in bytes) of a single compute shader uniform buffer binding, that guarantees portability..
static constexpr uint64_t MaxComputeShaderUniformBufferSize = 256u;

/// @brief Maximum total uniform buffer data size (in bytes) visible to the vertex shader stage.
static constexpr uint64_t MaxVertexShaderTotalUniformBufferSize = 2048u;

/// @brief Maximum total uniform buffer data size (in bytes) visible to the fragment shader stage.
static constexpr uint64_t MaxFragmentShaderTotalUniformBufferSize = 256u;

/// @brief Maximum total uniform buffer data size (in bytes) visible to the compute shader stage, that guarantees
/// portability.
static constexpr uint64_t MaxComputeShaderTotalUniformBufferSize = 256u;

/// @brief Maximum number of sampled textures accessible from the vertex stage.
static constexpr uint32_t MaxVertexShaderTextures = 0u;

/// @brief Maximum number of sampled textures accessible from the fragment stage.
static constexpr uint32_t MaxFragmentShaderTextures = 8u;

/// @brief Maximum number of sampled textures accessible from the compute stage, that guarantees portability.
static constexpr uint32_t MaxComputeShaderTextures = 0u;

/// @brief Maximum total number of sampled textures across all stages.
static constexpr uint32_t MaxTextures = 8u;

/// @brief Maximum number of samplers accessible from the vertex stage.
static constexpr uint32_t MaxVertexShaderSamplers = 0u;

/// @brief Maximum number of samplers accessible from the fragment stage.
static constexpr uint32_t MaxFragmentShaderSamplers = 8u;

/// @brief Maximum number of samplers accessible from the compute stage, that guarantees portability.
static constexpr uint32_t MaxComputeShaderSamplers = 0u;

/// @brief Maximum total number of samplers across all stages.
static constexpr uint32_t MaxSamplers = 8u;

/// @brief Maximum number of vertex input attributes in a pipeline.
static constexpr uint32_t MaxVertexInputAttributes = 8u;

/// @brief Maximum number of vertex input bindings in a pipeline.
static constexpr uint32_t MaxVertexInputBindings = 8u;

/// @brief Maximum byte offset of a vertex attribute within its vertex buffer binding.
static constexpr uint32_t MaxVertexInputAttributeOffset = 2047u;

/// @brief Maximum byte stride of a vertex buffer binding.
static constexpr uint32_t MaxVertexInputBindingStride = 2047u;
} // namespace SupportedLimit
} // namespace snap::rhi
