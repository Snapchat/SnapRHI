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
target_include_directories(backend-noop
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link to public-api and backend-common
target_link_libraries(backend-noop
    PUBLIC
        snap-rhi::public-api
        snap-rhi::backend-common
)

# Compiler warnings
snap_rhi_setup_compiler_warnings(backend-noop)
