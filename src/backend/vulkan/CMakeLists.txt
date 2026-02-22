# ============================================================================
# SnapRHI Vulkan Backend
# ============================================================================
# Vulkan rendering backend for Windows, Linux, Android, and Apple (MoltenVK).
# ============================================================================

include(FetchContent)

# ============================================================================
# External Dependencies
# ============================================================================

# SPIRV-Reflect for shader reflection
# Using SDK 1.3.296 which has updated CMake minimum version
FetchContent_Declare(
    spirv-reflect
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
    GIT_TAG        vulkan-sdk-1.3.296.0
    GIT_SHALLOW    TRUE
)

set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(spirv-reflect)

# ============================================================================
# Source Files
# ============================================================================
file(GLOB_RECURSE SNAP_RHI_VULKAN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Create the library with namespaced alias
snap_rhi_add_library(backend-vulkan STATIC ${SNAP_RHI_VULKAN_SOURCES})

# Public include directories
target_include_directories(backend-vulkan
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link to public-api, backend-common, and glad-vulkan (from third-party)
target_link_libraries(backend-vulkan
    PUBLIC
        snap-rhi::public-api
        snap-rhi::backend-common
        glad::glad-vulkan
    PRIVATE
        spirv-reflect-static
)

# ============================================================================
# Vulkan Validation Layers Option
# ============================================================================
if(SNAP_RHI_VULKAN_ENABLE_LAYERS)
    target_compile_definitions(backend-vulkan PUBLIC SNAP_RHI_VULKAN_NATIVE_VALIDATION_LAYERS=1)
else()
    target_compile_definitions(backend-vulkan PUBLIC SNAP_RHI_VULKAN_NATIVE_VALIDATION_LAYERS=0)
endif()

# Compiler warnings
snap_rhi_setup_compiler_warnings(backend-vulkan)
