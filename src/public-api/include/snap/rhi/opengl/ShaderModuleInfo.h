#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace snap::rhi::opengl {
/**
 * @brief Maps a specialization constant ID to a preprocessor define name in GLSL source.
 *
 * @details
 * In standard OpenGL/GLSL there is no built-in specialization constant mechanism
 * (unlike Vulkan SPIR-V `OpSpecConstant`).  SnapRHI emulates specialization constants
 * on the OpenGL backend by injecting `#define` directives into the shader source
 * before compilation.
 *
 * By default, every specialization constant whose ID is @e N gets the macro name
 * `SNAP_RHI_SPECIALIZATION_CONSTANT_N`.  This works well for simple cases where
 * the shader author writes:
 * @code
 *   #ifndef SNAP_RHI_SPECIALIZATION_CONSTANT_0
 *   #define SNAP_RHI_SPECIALIZATION_CONSTANT_0 1
 *   #endif
 *   const bool ENABLE_FEATURE = bool(SNAP_RHI_SPECIALIZATION_CONSTANT_0);
 * @endcode
 *
 * However, when porting shaders that were originally written for Vulkan/SPIR-V or
 * when a more descriptive naming convention is preferred, the caller may supply a
 * custom name for each constant via this structure.  The OpenGL backend will then
 * emit `#define <name> <value>` instead of the default macro.
 *
 * @note
 * - @ref name must match the preprocessor token used in the GLSL source exactly
 *   (case-sensitive, no leading `#define`).
 * - If an entry for a given @ref constantID is not provided, the default
 *   `SNAP_RHI_SPECIALIZATION_CONSTANT_<constantID>` name is used.
 * - Entries are matched to @ref snap::rhi::SpecializationMapEntry by @ref constantID.
 */
struct SpecializationConstantName {
    /**
     * @brief Specialization constant ID.
     *
     * Must correspond to a @ref snap::rhi::SpecializationMapEntry::constantID present
     * in the @ref snap::rhi::ShaderModuleCreateInfo::specializationInfo.
     */
    uint32_t constantID = 0;

    /**
     * @brief GLSL preprocessor macro name for this constant.
     *
     * This name is injected verbatim as `#define <name> <value>` at the top of
     * the shader source (after the `#version` directive).
     *
     * @warning Must be a valid GLSL preprocessor identifier (no spaces, no leading digits).
     */
    std::string name;
};

/**
 * @brief OpenGL-specific metadata for @ref snap::rhi::ShaderModuleCreateInfo.
 *
 * @details
 * Provides optional configuration that is only consumed by the OpenGL backend.
 * Currently this structure carries custom names for specialization constants, but
 * may be extended in the future with other OpenGL-specific shader compilation hints.
 *
 * Usage example:
 * @code
 *   snap::rhi::opengl::ShaderModuleInfo glInfo;
 *   glInfo.specializationConstantNames = {
 *       {0, "USE_NORMAL_MAP"},
 *       {1, "MAX_LIGHT_COUNT"},
 *   };
 *
 *   snap::rhi::ShaderModuleCreateInfo ci{};
 *   ci.shaderStage     = snap::rhi::ShaderStage::Fragment;
 *   ci.name            = "main";
 *   ci.shaderLibrary   = myLibrary;
 *   ci.specializationInfo = mySpecInfo;
 *   ci.glShaderModuleInfo = glInfo;
 * @endcode
 */
struct ShaderModuleInfo {
    /**
     * @brief Custom preprocessor names for specialization constants.
     *
     * When non-empty, the OpenGL backend uses the provided names instead of the
     * default `SNAP_RHI_SPECIALIZATION_CONSTANT_<ID>` pattern for the matching
     * constant IDs.
     *
     * Any constant ID that does not appear in this list still falls back to the
     * default naming convention.
     */
    std::vector<SpecializationConstantName> specializationConstantNames;
};
} // namespace snap::rhi::opengl
