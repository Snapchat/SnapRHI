# ============================================================================
# SnapRHI - Cross-Platform Rendering Hardware Interface
# ============================================================================
# A modern, cross-platform abstraction layer over Metal, Vulkan, and OpenGL.
#
# Build instructions:
#   mkdir build && cd build
#   cmake .. -DSNAP_RHI_ENABLE_METAL=ON
#   cmake --build .
#
# For the demo app with SDL3:
#   cmake .. -DSNAP_RHI_ENABLE_METAL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON
# ============================================================================

cmake_minimum_required(VERSION 3.25)
include_guard(GLOBAL)

project(snap-rhi
    VERSION 1.0.0
    DESCRIPTION "Cross-platform Rendering Hardware Interface"
    LANGUAGES C CXX
)

# ============================================================================
# CMake Module Path
# ============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include our custom CMake modules
include(SnapRHIOptions)
include(SnapRHIHelpers)
include(SnapRHICompileDefinitions)

# Apply all validation settings if master validation option is enabled
# This must be called early, before other options are used
snap_rhi_apply_all_validation()

# ============================================================================
# Project-wide Settings
# ============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Objective-C/Objective-C++ on Apple platforms
if(APPLE)
    enable_language(OBJC OBJCXX)
endif()

# Prevent Windows.h from defining min/max macros, which break
# std::min, std::max, and std::numeric_limits<T>::max() throughout the codebase.
if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# Position Independent Code (required for shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Build Options
# ============================================================================
option(SNAP_RHI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SNAP_RHI_BUILD_EXAMPLES "Build example applications" OFF)

# ============================================================================
# Print Configuration Summary
# ============================================================================
snap_rhi_print_config()

# ============================================================================
# Third-Party Dependencies
# ============================================================================
# GLAD provides loaders for both OpenGL and Vulkan
if(SNAP_RHI_ENABLE_OPENGL OR SNAP_RHI_ENABLE_VULKAN)
    add_subdirectory(third-party/GLAD)
endif()

if(SNAP_RHI_ENABLE_OPENGL)
    add_subdirectory(third-party/gl-loader)
endif()

# ============================================================================
# Core Library Targets
# ============================================================================
add_subdirectory(src/public-api)
add_subdirectory(src/backend/common)

# ============================================================================
# Backend Targets (conditionally enabled)
# ============================================================================
if(SNAP_RHI_ENABLE_VULKAN)
    add_subdirectory(src/backend/vulkan)
endif()

if(SNAP_RHI_ENABLE_NOOP)
    add_subdirectory(src/backend/noop)
endif()

if(SNAP_RHI_ENABLE_METAL)
    if(NOT APPLE)
        message(FATAL_ERROR "Metal backend requires Apple platforms (macOS, iOS)")
    endif()
    add_subdirectory(src/backend/metal)
endif()

if(SNAP_RHI_ENABLE_OPENGL)
    add_subdirectory(src/backend/opengl)
endif()

# ============================================================================
# Optional Modules
# ============================================================================
if(SNAP_RHI_WITH_INTEROP)
    add_subdirectory(src/interop)
endif()

# ============================================================================
# Examples
# ============================================================================
if(SNAP_RHI_BUILD_EXAMPLES)
    add_subdirectory(examples/triangle-demo-app)
endif()
