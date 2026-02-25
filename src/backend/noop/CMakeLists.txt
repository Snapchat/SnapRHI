# ============================================================================
# SnapRHI No-op Backend
# ============================================================================
# Stub backend for testing and development without GPU hardware.
# ============================================================================

# Collect source files
file(GLOB SNAP_RHI_NOOP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Create the library with namespaced alias
snap_rhi_add_library(backend-noop STATIC ${SNAP_RHI_NOOP_SOURCES})

# Public include directories
snap_rhi_target_include_directories(backend-noop include)

# Link to public-api and backend-common
snap_rhi_target_link_libraries(backend-noop snap-rhi::public-api snap-rhi::backend-common)

# Compiler warnings
snap_rhi_setup_compiler_warnings(backend-noop)

# Export target for packaging
snap_rhi_export(backend-noop)
