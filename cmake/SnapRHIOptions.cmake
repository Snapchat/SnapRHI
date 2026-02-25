# ============================================================================
# SnapRHI CMake Options
# ============================================================================
# This file defines all configurable options for the SnapRHI library.
# Options are grouped by category for clarity.
# ============================================================================

include_guard(GLOBAL)

# ----------------------------------------------------------------------------
# Master Validation Option
# ----------------------------------------------------------------------------
# Enable this for comprehensive debugging - turns on ALL validation features.
# This is the recommended option for development and debugging sessions.

option(SNAP_RHI_ENABLE_ALL_VALIDATION "Enable ALL validation features for debugging (Vulkan layers, all validation tags, slow checks)" OFF)

# ----------------------------------------------------------------------------
# Backend Selection Options
# ----------------------------------------------------------------------------
# Enable/disable rendering backends. Platform defaults are set automatically.

option(SNAP_RHI_ENABLE_METAL "Enable Metal backend (Apple platforms only)" OFF)
option(SNAP_RHI_ENABLE_VULKAN "Enable Vulkan backend" OFF)
option(SNAP_RHI_ENABLE_OPENGL "Enable OpenGL/OpenGL ES backend" OFF)
option(SNAP_RHI_ENABLE_NOOP "Enable No-op backend (for testing)" OFF)

# Vulkan validation layers
option(SNAP_RHI_VULKAN_ENABLE_LAYERS "Enable Vulkan validation layers" OFF)

# Optional modules
option(SNAP_RHI_WITH_INTEROP "Build the interop module" OFF)


# ----------------------------------------------------------------------------
# Debug and Validation Options
# ----------------------------------------------------------------------------

option(SNAP_RHI_ENABLE_SLOW_SAFETY_CHECKS "Enable additional runtime safety checks (impacts performance)" OFF)
option(SNAP_RHI_ENABLE_SOURCE_LOCATION "Enable source location tracking in API calls" OFF)
option(SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS "Enable custom profiling labels" OFF)
option(SNAP_RHI_ENABLE_DEBUG_LABELS "Enable debug labels for GPU resources" ON)
option(SNAP_RHI_ENABLE_LOGS "Enable logging output" ON)
option(SNAP_RHI_ENABLE_API_DUMP "Enable API call dumping for debugging" OFF)
option(SNAP_RHI_THROW_ON_LIFETIME_VIOLATION "Throw exceptions on object lifetime violations" OFF)

# Report level: all, debug, info, warning, performance_warning, error, critical_error
set(SNAP_RHI_REPORT_LEVEL "warning" CACHE STRING "Minimum report level for logging")
set_property(CACHE SNAP_RHI_REPORT_LEVEL PROPERTY STRINGS
    "all" "debug" "info" "warning" "performance_warning" "error" "critical_error")

# ----------------------------------------------------------------------------
# Validation Tag Options
# ----------------------------------------------------------------------------
# Fine-grained control over which operations include validation

option(SNAP_RHI_VALIDATION_CREATE_OP "Validate create operations" OFF)
option(SNAP_RHI_VALIDATION_DESTROY_OP "Validate destroy operations" OFF)
option(SNAP_RHI_VALIDATION_DEVICE_CONTEXT_OP "Validate device context operations" OFF)
option(SNAP_RHI_VALIDATION_COMMAND_QUEUE_OP "Validate command queue operations" OFF)
option(SNAP_RHI_VALIDATION_COMMAND_BUFFER_OP "Validate command buffer operations" OFF)
option(SNAP_RHI_VALIDATION_RENDER_COMMAND_ENCODER_OP "Validate render command encoder operations" OFF)
option(SNAP_RHI_VALIDATION_COMPUTE_COMMAND_ENCODER_OP "Validate compute command encoder operations" OFF)
option(SNAP_RHI_VALIDATION_BLIT_COMMAND_ENCODER_OP "Validate blit command encoder operations" OFF)
option(SNAP_RHI_VALIDATION_RENDER_PASS_OP "Validate render pass operations" OFF)
option(SNAP_RHI_VALIDATION_FRAMEBUFFER_OP "Validate framebuffer operations" OFF)
option(SNAP_RHI_VALIDATION_RENDER_PIPELINE_OP "Validate render pipeline operations" OFF)
option(SNAP_RHI_VALIDATION_COMPUTE_PIPELINE_OP "Validate compute pipeline operations" OFF)
option(SNAP_RHI_VALIDATION_SHADER_MODULE_OP "Validate shader module operations" OFF)
option(SNAP_RHI_VALIDATION_SHADER_LIBRARY_OP "Validate shader library operations" OFF)
option(SNAP_RHI_VALIDATION_SAMPLER_OP "Validate sampler operations" OFF)
option(SNAP_RHI_VALIDATION_TEXTURE_OP "Validate texture operations" OFF)
option(SNAP_RHI_VALIDATION_BUFFER_OP "Validate buffer operations" OFF)
option(SNAP_RHI_VALIDATION_DESCRIPTOR_SET_LAYOUT_OP "Validate descriptor set layout operations" OFF)
option(SNAP_RHI_VALIDATION_PIPELINE_LAYOUT_OP "Validate pipeline layout operations" OFF)
option(SNAP_RHI_VALIDATION_DESCRIPTOR_POOL_OP "Validate descriptor pool operations" OFF)
option(SNAP_RHI_VALIDATION_DESCRIPTOR_SET_OP "Validate descriptor set operations" OFF)
option(SNAP_RHI_VALIDATION_FENCE_OP "Validate fence operations" OFF)
option(SNAP_RHI_VALIDATION_SEMAPHORE_OP "Validate semaphore operations" OFF)
option(SNAP_RHI_VALIDATION_DEVICE_OP "Validate device operations" OFF)
option(SNAP_RHI_VALIDATION_PIPELINE_CACHE_OP "Validate pipeline cache operations" OFF)
option(SNAP_RHI_VALIDATION_QUERY_POOL_OP "Validate query pool operations" OFF)
option(SNAP_RHI_VALIDATION_GL_COMMAND_QUEUE_EXTERNAL_ERROR "Validate GL command queue external errors" OFF)
option(SNAP_RHI_VALIDATION_GL_COMMAND_QUEUE_INTERNAL_ERROR "Validate GL command queue internal errors" OFF)
option(SNAP_RHI_VALIDATION_GL_STATE_CACHE_OP "Validate GL state cache operations" OFF)
option(SNAP_RHI_VALIDATION_GL_PROGRAM_VALIDATION_OP "Validate GL program validation operations" OFF)
option(SNAP_RHI_VALIDATION_GL_PROFILE_OP "Validate GL profile operations" OFF)

# ----------------------------------------------------------------------------
# Helper Function: Convert option to 0/1 for compile definitions
# ----------------------------------------------------------------------------
function(snap_rhi_bool_to_int OPTION_NAME OUT_VAR)
    if(${OPTION_NAME})
        set(${OUT_VAR} 1 PARENT_SCOPE)
    else()
        set(${OUT_VAR} 0 PARENT_SCOPE)
    endif()
endfunction()

# ----------------------------------------------------------------------------
# Convert report level string to integer
# ----------------------------------------------------------------------------
function(snap_rhi_report_level_to_int LEVEL OUT_VAR)
    if(LEVEL STREQUAL "all")
        set(${OUT_VAR} 0 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "debug")
        set(${OUT_VAR} 1 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "info")
        set(${OUT_VAR} 2 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "warning")
        set(${OUT_VAR} 3 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "performance_warning")
        set(${OUT_VAR} 4 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "error")
        set(${OUT_VAR} 5 PARENT_SCOPE)
    elseif(LEVEL STREQUAL "critical_error")
        set(${OUT_VAR} 6 PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Unknown report level: ${LEVEL}")
    endif()
endfunction()

# Print configuration summary
function(snap_rhi_print_config)
    message(STATUS "")
    message(STATUS "=== SnapRHI Configuration ===")
    message(STATUS "Backends:")
    message(STATUS "  Metal:   ${SNAP_RHI_ENABLE_METAL}")
    message(STATUS "  Vulkan:  ${SNAP_RHI_ENABLE_VULKAN}")
    message(STATUS "  OpenGL:  ${SNAP_RHI_ENABLE_OPENGL}")
    message(STATUS "  No-op:   ${SNAP_RHI_ENABLE_NOOP}")
    message(STATUS "Optional Modules:")
    message(STATUS "  Interop: ${SNAP_RHI_WITH_INTEROP}")
    message(STATUS "Debug:")
    message(STATUS "  Debug Labels: ${SNAP_RHI_ENABLE_DEBUG_LABELS}")
    message(STATUS "  Logs:         ${SNAP_RHI_ENABLE_LOGS}")
    message(STATUS "  Report Level: ${SNAP_RHI_REPORT_LEVEL}")
    if(SNAP_RHI_ENABLE_ALL_VALIDATION)
        message(STATUS "  ALL VALIDATION: ON (all checks enabled)")
    endif()
    if(SNAP_RHI_ENABLE_VULKAN AND SNAP_RHI_VULKAN_ENABLE_LAYERS)
        message(STATUS "  Vulkan Layers:  ON")
    endif()
    message(STATUS "==============================")
    message(STATUS "")
endfunction()

# ----------------------------------------------------------------------------
# Apply All Validation Settings
# ----------------------------------------------------------------------------
# When SNAP_RHI_ENABLE_ALL_VALIDATION is ON, enable all validation features.
# This function should be called after all options are defined.

function(snap_rhi_apply_all_validation)
    if(NOT SNAP_RHI_ENABLE_ALL_VALIDATION)
        return()
    endif()

    message(STATUS "SNAP_RHI_ENABLE_ALL_VALIDATION is ON - enabling all validation features")

    # Enable Vulkan validation layers
    set(SNAP_RHI_VULKAN_ENABLE_LAYERS ON CACHE BOOL "Enable Vulkan validation layers" FORCE)

    # Enable slow safety checks
    set(SNAP_RHI_ENABLE_SLOW_SAFETY_CHECKS ON CACHE BOOL "Enable additional runtime safety checks" FORCE)

    # Enable debug labels and logs
    set(SNAP_RHI_ENABLE_DEBUG_LABELS ON CACHE BOOL "Enable debug labels for GPU resources" FORCE)
    set(SNAP_RHI_ENABLE_LOGS ON CACHE BOOL "Enable logging output" FORCE)

    # Set report level to all
    set(SNAP_RHI_REPORT_LEVEL "all" CACHE STRING "Minimum report level for logging" FORCE)

    # Enable all validation tags
    set(SNAP_RHI_VALIDATION_CREATE_OP ON CACHE BOOL "Validate create operations" FORCE)
    set(SNAP_RHI_VALIDATION_DESTROY_OP ON CACHE BOOL "Validate destroy operations" FORCE)
    set(SNAP_RHI_VALIDATION_DEVICE_CONTEXT_OP ON CACHE BOOL "Validate device context operations" FORCE)
    set(SNAP_RHI_VALIDATION_COMMAND_QUEUE_OP ON CACHE BOOL "Validate command queue operations" FORCE)
    set(SNAP_RHI_VALIDATION_COMMAND_BUFFER_OP ON CACHE BOOL "Validate command buffer operations" FORCE)
    set(SNAP_RHI_VALIDATION_RENDER_COMMAND_ENCODER_OP ON CACHE BOOL "Validate render command encoder operations" FORCE)
    set(SNAP_RHI_VALIDATION_COMPUTE_COMMAND_ENCODER_OP ON CACHE BOOL "Validate compute command encoder operations" FORCE)
    set(SNAP_RHI_VALIDATION_BLIT_COMMAND_ENCODER_OP ON CACHE BOOL "Validate blit command encoder operations" FORCE)
    set(SNAP_RHI_VALIDATION_RENDER_PASS_OP ON CACHE BOOL "Validate render pass operations" FORCE)
    set(SNAP_RHI_VALIDATION_FRAMEBUFFER_OP ON CACHE BOOL "Validate framebuffer operations" FORCE)
    set(SNAP_RHI_VALIDATION_RENDER_PIPELINE_OP ON CACHE BOOL "Validate render pipeline operations" FORCE)
    set(SNAP_RHI_VALIDATION_COMPUTE_PIPELINE_OP ON CACHE BOOL "Validate compute pipeline operations" FORCE)
    set(SNAP_RHI_VALIDATION_SHADER_MODULE_OP ON CACHE BOOL "Validate shader module operations" FORCE)
    set(SNAP_RHI_VALIDATION_SHADER_LIBRARY_OP ON CACHE BOOL "Validate shader library operations" FORCE)
    set(SNAP_RHI_VALIDATION_SAMPLER_OP ON CACHE BOOL "Validate sampler operations" FORCE)
    set(SNAP_RHI_VALIDATION_TEXTURE_OP ON CACHE BOOL "Validate texture operations" FORCE)
    set(SNAP_RHI_VALIDATION_BUFFER_OP ON CACHE BOOL "Validate buffer operations" FORCE)
    set(SNAP_RHI_VALIDATION_DESCRIPTOR_SET_LAYOUT_OP ON CACHE BOOL "Validate descriptor set layout operations" FORCE)
    set(SNAP_RHI_VALIDATION_PIPELINE_LAYOUT_OP ON CACHE BOOL "Validate pipeline layout operations" FORCE)
    set(SNAP_RHI_VALIDATION_DESCRIPTOR_POOL_OP ON CACHE BOOL "Validate descriptor pool operations" FORCE)
    set(SNAP_RHI_VALIDATION_DESCRIPTOR_SET_OP ON CACHE BOOL "Validate descriptor set operations" FORCE)
    set(SNAP_RHI_VALIDATION_FENCE_OP ON CACHE BOOL "Validate fence operations" FORCE)
    set(SNAP_RHI_VALIDATION_SEMAPHORE_OP ON CACHE BOOL "Validate semaphore operations" FORCE)
    set(SNAP_RHI_VALIDATION_DEVICE_OP ON CACHE BOOL "Validate device operations" FORCE)
    set(SNAP_RHI_VALIDATION_PIPELINE_CACHE_OP ON CACHE BOOL "Validate pipeline cache operations" FORCE)
    set(SNAP_RHI_VALIDATION_QUERY_POOL_OP ON CACHE BOOL "Validate query pool operations" FORCE)
    set(SNAP_RHI_VALIDATION_GL_COMMAND_QUEUE_EXTERNAL_ERROR ON CACHE BOOL "Validate GL command queue external errors" FORCE)
    set(SNAP_RHI_VALIDATION_GL_COMMAND_QUEUE_INTERNAL_ERROR ON CACHE BOOL "Validate GL command queue internal errors" FORCE)
    set(SNAP_RHI_VALIDATION_GL_STATE_CACHE_OP ON CACHE BOOL "Validate GL state cache operations" FORCE)
    set(SNAP_RHI_VALIDATION_GL_PROGRAM_VALIDATION_OP ON CACHE BOOL "Validate GL program validation operations" FORCE)
    set(SNAP_RHI_VALIDATION_GL_PROFILE_OP ON CACHE BOOL "Validate GL profile operations" FORCE)

    message(STATUS "All validation features enabled")
endfunction()
