# ============================================================================
# SnapRHI Interop Module
# ============================================================================
# Platform-specific interoperability utilities for texture/buffer sharing
# with external graphics APIs and system frameworks.
# ============================================================================

# Collect common source files
file(GLOB SNAP_RHI_INTEROP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Platform-specific sources
if(APPLE)
    file(GLOB_RECURSE SNAP_RHI_INTEROP_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Apple/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Apple/*.mm
    )
elseif(ANDROID)
    file(GLOB_RECURSE SNAP_RHI_INTEROP_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Android/*.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    file(GLOB_RECURSE SNAP_RHI_INTEROP_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Linux/*.cpp
    )
elseif(WIN32)
    file(GLOB_RECURSE SNAP_RHI_INTEROP_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/*.cpp
    )
else()
    file(GLOB_RECURSE SNAP_RHI_INTEROP_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Other/*.cpp
    )
endif()

list(APPEND SNAP_RHI_INTEROP_SOURCES ${SNAP_RHI_INTEROP_PLATFORM_SOURCES})

message(STATUS "Building snap-rhi interop module: ${SNAP_RHI_INTEROP_PLATFORM_SOURCES}")

# Create the library with namespaced alias
snap_rhi_add_library(interop STATIC ${SNAP_RHI_INTEROP_SOURCES})

# Public include directories
target_include_directories(interop
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link to core libraries
target_link_libraries(interop
    PUBLIC
        snap-rhi::public-api
        snap-rhi::backend-common
)

# ============================================================================
# Backend-specific Dependencies
# ============================================================================
if(SNAP_RHI_ENABLE_VULKAN)
    target_link_libraries(interop PUBLIC snap-rhi::backend-vulkan)
endif()

if(SNAP_RHI_ENABLE_METAL)
    target_link_libraries(interop PUBLIC snap-rhi::backend-metal)

    # CoreFoundation for Metal interop
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    target_link_libraries(interop PRIVATE ${COREFOUNDATION_FRAMEWORK})
endif()

if(SNAP_RHI_ENABLE_OPENGL)
    target_link_libraries(interop PUBLIC snap-rhi::backend-opengl)
endif()

# ============================================================================
# Objective-C++ Settings (Apple)
# ============================================================================
if(APPLE)
    # Find and set ARC for .mm files
    file(GLOB_RECURSE OBJCXX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Apple/*.mm)
    if(OBJCXX_SOURCES)
        set_source_files_properties(${OBJCXX_SOURCES} PROPERTIES
            COMPILE_FLAGS "-fobjc-arc"
        )
    endif()
endif()

# Compiler warnings
snap_rhi_setup_compiler_warnings(interop)
