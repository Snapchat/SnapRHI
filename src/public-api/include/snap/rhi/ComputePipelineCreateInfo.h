#pragma once

#include <optional>

#include "snap/rhi/Structs.h"
#include "snap/rhi/metal/PipelineInfo.h"
#include "snap/rhi/opengl/PipelineInfo.h"
#include "snap/rhi/reflection/ComputePipelineInfo.h"

namespace snap::rhi {
class ShaderModule;
class PipelineCache;
class ComputePipeline;
class PipelineLayout;

// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkComputePipelineCreateInfo.html
struct ComputePipelineCreateInfo {
    /**
     * @brief Pipeline creation flags.
     *
     * Controls backend-specific pipeline creation behavior.
     *
     * Commonly used flags include:
     * - `PipelineCreateFlags::AllowAsyncCreation` (Metal): allow pipeline compilation to happen asynchronously.
     * - `PipelineCreateFlags::NullOnPipelineCacheMiss` (Metal): if a pipeline cache/binary archive miss occurs,
     * pipeline creation returns `nullptr` instead of falling back to normal compilation.
     * - `PipelineCreateFlags::AcquireNativeReflection` (Vulkan and others): attempt to populate `ComputePipeline`
     * reflection data from native shader metadata.
     *
     * @note Reflection-only pipelines:
     * If `AcquireNativeReflection` is used, some backends allow `pipelineLayout` to be `nullptr` and `glPipelineInfo`
     * to be empty; in this mode the created pipeline may be invalid for execution and intended only for querying
     * reflection.
     */
    PipelineCreateFlags pipelineCreateFlags = PipelineCreateFlags::None;

    /**
     * @brief Pipeline layout describing descriptor set layouts used by the compute shader.
     *
     * Must match the resources declared in @ref stage.
     *
     * @note Backend requirements:
     * - Vulkan validates that this is non-null for executable pipelines.
     * - Metal uses it to build descriptor set reflection (`buildDescriptorSetReflection`).
     */
    PipelineLayout* pipelineLayout = nullptr;

    /**
     * @brief Compute shader module.
     *
     * The shader entry point is backend-defined;
     * most backends use the module's default/compiled entry point for compute.
     *
     * @note Backend requirements:
     * - Vulkan validates that this is non-null.
     */
    ShaderModule* stage = nullptr;

    /**
     * @brief Local threadgroup/workgroup size used by the compute shader.
     *
     * This is the per-workgroup threadgroup size (often called `local_size_x/y/z` in shader languages).
     *
     * @note Backend usage:
     * - Metal uses this value to derive `threadsPerThreadgroup` when dispatching.
     * - Vulkan/OpenGL typically encode the local size in the shader itself; this value can still be used by
     * higher-level code for validation and dispatch calculations.
     */
    Extent3D localThreadGroupSize{1, 1, 1};

    /**
     * @brief Optional base pipeline used for derivative pipeline creation.
     *
     * If provided, backends may reuse compilation artifacts from the base pipeline.
     *
     * @note Backend usage:
     * - Vulkan sets `VK_PIPELINE_CREATE_DERIVATIVE_BIT` when a base pipeline is supplied.
     * - Metal may reuse an existing pipeline state if the base pipeline uses the same function.
     */
    ComputePipeline* basePipeline = nullptr;

    /**
     * @brief Optional pipeline cache.
     *
     * When provided, backends may use the cache to speed up pipeline creation.
     *
     * @note Backend usage:
     * - Vulkan passes the underlying `VkPipelineCache` to `vkCreateComputePipelines`.
     * - Metal may use an `MTLBinaryArchive`. When combined with `NullOnPipelineCacheMiss`, pipeline creation may return
     *   `nullptr` if the binary archive does not contain the pipeline.
     */
    PipelineCache* pipelineCache = nullptr;

    /**
     * @brief OpenGL-specific pipeline configuration and reflection mapping.
     *
     * OpenGL requires a mapping from resource names to `{descriptorSetIndex, binding}` in order to configure the
     * program state correctly.
     */
    std::optional<snap::rhi::opengl::PipelineInfo> glPipelineInfo = std::nullopt;

    /**
     * @brief Metal-specific pipeline configuration and reflection mapping.
     *
     * Metal configurations require extra information to bind resources correctly.
     * When provided, this data is consumed only by the Metal backend.
     */
    std::optional<snap::rhi::metal::PipelineInfo> mtlPipelineInfo = std::nullopt;
};
} // namespace snap::rhi
