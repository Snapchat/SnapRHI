# ============================================================================
# SnapRHI OpenGL Backend
# ============================================================================
# OpenGL/OpenGL ES rendering backend for Windows, Linux, macOS, Android, iOS.
# ============================================================================

# ============================================================================
# Source Files
# ============================================================================
file(GLOB_RECURSE SNAP_RHI_OPENGL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Create the library with namespaced alias
snap_rhi_add_library(backend-opengl STATIC ${SNAP_RHI_OPENGL_SOURCES})

# Public include directories
target_include_directories(backend-opengl PUBLIC include)

# Link to public-api, backend-common, and gl-loader
snap_rhi_target_link_libraries(backend-opengl snap-rhi::public-api snap-rhi::backend-common gl-loader::gl-loader)

# Compiler warnings
snap_rhi_setup_compiler_warnings(backend-opengl)

# Export target for packaging
snap_rhi_export(backend-opengl)
