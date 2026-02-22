# ============================================================================
# SnapRHI Public API Library
# ============================================================================
# Core public interface definitions and common utilities shared across
# all backends.
# ============================================================================

# Collect source files
file(GLOB_RECURSE SNAP_RHI_PUBLIC_API_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Create the library with namespaced alias
snap_rhi_add_library(public-api STATIC ${SNAP_RHI_PUBLIC_API_SOURCES})

# Public include directories
target_include_directories(public-api
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# C++20 required
target_compile_features(public-api PUBLIC cxx_std_20)

# Configure compile definitions based on options
snap_rhi_configure_public_api_definitions(public-api)

# Compiler warnings
snap_rhi_setup_compiler_warnings(public-api)
