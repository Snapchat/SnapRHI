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
target_include_directories(backend-metal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link to public-api and backend-common
target_link_libraries(backend-metal
    PUBLIC
        snap-rhi::public-api
        snap-rhi::backend-common
)

# ============================================================================
# Apple Framework Dependencies
# ============================================================================
# Use find_library for cleaner CMake and better IDE integration

find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
find_library(METAL_FRAMEWORK Metal REQUIRED)
find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)

target_link_libraries(backend-metal
    PRIVATE
        ${FOUNDATION_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
)

# ============================================================================
# Objective-C++ Compiler Settings
# ============================================================================
# Enable Automatic Reference Counting (ARC) for Objective-C++ files
set_source_files_properties(${SNAP_RHI_METAL_SOURCES} PROPERTIES
    COMPILE_FLAGS "-fobjc-arc"
)

# Compiler warnings
snap_rhi_setup_compiler_warnings(backend-metal)
