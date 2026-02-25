# ============================================================================
# SnapRHI Metal Backend
# ============================================================================
# Metal rendering backend for Apple platforms (macOS, iOS, tvOS).
# ============================================================================

# Collect source files (Objective-C++ .mm files)
file(GLOB SNAP_RHI_METAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
)

# Create the library with namespaced alias
snap_rhi_add_library(backend-metal STATIC ${SNAP_RHI_METAL_SOURCES})

# Public include directories
snap_rhi_target_include_directories(backend-metal include)

# Link to public-api and backend-common
snap_rhi_target_link_libraries(backend-metal snap-rhi::public-api snap-rhi::backend-common)

# Apple Framework Dependencies
snap_rhi_target_link_libraries(backend-metal "-framework Foundation" "-framework Metal" "-framework QuartzCore")

# ============================================================================
# Objective-C++ Compiler Settings
# ============================================================================
# Enable Automatic Reference Counting (ARC) for Objective-C++ files
set_source_files_properties(${SNAP_RHI_METAL_SOURCES} PROPERTIES
    COMPILE_FLAGS "-fobjc-arc"
)

# Compiler warnings
snap_rhi_setup_compiler_warnings(backend-metal)

# Export target for packaging
snap_rhi_export(backend-metal)
