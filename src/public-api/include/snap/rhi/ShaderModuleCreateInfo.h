#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"
#include "snap/rhi/SpecializationConstantFormat.h"
#include "snap/rhi/opengl/ShaderModuleInfo.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace snap::rhi {
class ShaderLibrary;

/**
 * @brief Describes a single specialization constant mapping.
 *
 * Specialization constants allow patching constant values into a shader module at pipeline creation time without
 * recompiling the shader source.
 *
 * Backend notes:
 * - Vulkan maps 1:1 to `VkSpecializationMapEntry`.
 * - Metal maps entries to function constants (`MTLFunctionConstantValues`).
 * - OpenGL typically applies specialization constants by injecting preprocessor defines.
 */
struct SpecializationMapEntry {
    /**
     * @brief Specialization constant ID.
     *
     * Must be in `[0, snap::rhi::Capabilities::maxSpecializationConstants - 1]`.
     */
    uint32_t constantID = 0;

    /**
     * @brief Byte offset into `SpecializationInfo::pData`.
     */
    uint32_t offset = 0;

    /**
     * @brief Value format for the constant.
     */
    SpecializationConstantFormat format = SpecializationConstantFormat::Undefined;
};

/**
 * @brief Specialization constants payload for a shader module.
 *
 * @ref pMapEntries describes how to interpret values stored in the byte buffer pointed to by @ref pData.
 */
struct SpecializationInfo {
    /// Number of entries in @ref pMapEntries.
    uint32_t mapEntryCount = 0;

    /// Pointer to an array of @ref SpecializationMapEntry.
    const SpecializationMapEntry* pMapEntries = nullptr;

    /// Size in bytes of the @ref pData buffer.
    size_t dataSize = 0;

    /**
     * @brief Pointer to raw specialization constants data.
     *
     * The buffer must remain valid for the duration of shader module creation.
     */
    const void* pData = nullptr;
};

/**
 * @brief Parameters controlling `snap::rhi::ShaderModule` creation.
 *
 * A shader module represents a single entry point (function) selected from a @ref ShaderLibrary.
 */
struct ShaderModuleCreateInfo {
    /// Backend-specific creation flags.
    ShaderModuleCreateFlags createFlags = ShaderModuleCreateFlags::None;

    /// Shader stage for this entry point.
    ShaderStage shaderStage = ShaderStage::Vertex;

    /**
     * @brief Entry point name.
     *
     * Name of the entry point/function inside @ref shaderLibrary.
     *
     * Backend notes:
     * - Vulkan uses this as `VkPipelineShaderStageCreateInfo::pName`.
     * - Metal uses this to lookup/create an `MTLFunction`.
     * - OpenGL backends may treat this as the exported entry name and/or use it to select/enable a function.
     */
    std::string_view name;

    /// Specialization constants for this module.
    SpecializationInfo specializationInfo{};

    /**
     * @brief Source library containing the entry point.
     *
     * Must not be null.
     */
    ShaderLibrary* shaderLibrary = nullptr;

    /**
     * @brief Optional OpenGL-specific shader module metadata.
     *
     * When provided, the OpenGL backend uses this to customise specialization constant
     * preprocessor names.  Other backends ignore this field.
     *
     * @see snap::rhi::opengl::ShaderModuleInfo
     */
    std::optional<snap::rhi::opengl::ShaderModuleInfo> glShaderModuleInfo = std::nullopt;
};
} // namespace snap::rhi
