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

# Optional packaging hook (no-op when SnapRHIPackaging module is absent)
include(SnapRHIPackaging OPTIONAL)

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
include(GNUInstallDirs)
option(SNAP_RHI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SNAP_RHI_BUILD_EXAMPLES "Build example applications" OFF)
option(SNAP_RHI_INSTALL "Generate CMake install rules" OFF)

# ============================================================================
# Print Configuration Summary
# ============================================================================
snap_rhi_print_config()

# ============================================================================
# Third-Party Dependencies
# ============================================================================
# GLAD, gl-loader and spirv-reflect are built from the local third-party
# sources for standalone / public CMake builds.  When pre-built targets are
# already available (packaged builds), the local sources are skipped.
# ============================================================================

# GLAD — OpenGL / Vulkan loaders
if(SNAP_RHI_ENABLE_OPENGL OR SNAP_RHI_ENABLE_VULKAN)
    if(NOT TARGET glad::glad-platform)
        if(DEFINED SNAP_RHI_OPENGL_BACKEND_LINUX)
            set(GLAD_OPENGL_BACKEND_LINUX "${SNAP_RHI_OPENGL_BACKEND_LINUX}" CACHE STRING
                "Linux OpenGL backend (glx, egl, osmesa)" FORCE)
        endif()
        add_subdirectory(third-party/GLAD)
    else()
        message(STATUS "[SnapRHI] Using pre-built GLAD targets")
    endif()
endif()

# gl-loader — OpenGL context management
if(SNAP_RHI_ENABLE_OPENGL)
    if(NOT TARGET gl-loader::gl-loader)
        if(DEFINED SNAP_RHI_OPENGL_BACKEND_LINUX)
            set(GL_LOADER_LINUX_BACKEND "${SNAP_RHI_OPENGL_BACKEND_LINUX}" CACHE STRING
                "Linux OpenGL backend (glx, egl, osmesa)" FORCE)
        endif()
        add_subdirectory(third-party/gl-loader)
    else()
        message(STATUS "[SnapRHI] Using pre-built gl-loader target")
    endif()
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
# Packaging-specific Modules
# ============================================================================
# Hook for additional packaging targets. No-op in standalone builds.
snap_rhi_add_packaging_modules()

# ============================================================================
# Examples
# ============================================================================
if(SNAP_RHI_BUILD_EXAMPLES)
    add_subdirectory(examples/triangle-demo-app)
endif()

# ============================================================================
# Packaging Export Complete
# ============================================================================
# Finalize packaging exports. No-op in standalone builds.
snap_rhi_export_complete()

# ============================================================================
# Install Rules
# ============================================================================
if(SNAP_RHI_INSTALL)
    include(SnapRHIInstall)
endif()
