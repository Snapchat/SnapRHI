# ============================================================================
# SnapRHI Backend Common Library
# ============================================================================
# Common functionality shared across all backend implementations.
# ============================================================================

# Collect source files
file(GLOB SNAP_RHI_BACKEND_COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Create the library with namespaced alias
snap_rhi_add_library(backend-common STATIC ${SNAP_RHI_BACKEND_COMMON_SOURCES})

# Public include directories
target_include_directories(backend-common PUBLIC include)

# Link to public-api
snap_rhi_target_link_libraries(backend-common snap-rhi::public-api)

# Compiler warnings
snap_rhi_setup_compiler_warnings(backend-common)

# Export target for packaging
snap_rhi_export(backend-common)
