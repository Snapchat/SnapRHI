# ============================================================================
# SnapRHI Vulkan Backend
# ============================================================================
# Vulkan rendering backend for Windows, Linux, Android, and Apple (MoltenVK).
# ============================================================================

# ============================================================================
# External Dependencies
# ============================================================================

# SPIRV-Reflect for shader reflection.
# When a namespaced target exists, create a plain alias;
# otherwise fetch from source.
if(TARGET SPIRV-Reflect::spirv-reflect-static)
    if(NOT TARGET spirv-reflect-static)
        add_library(spirv-reflect-static ALIAS SPIRV-Reflect::spirv-reflect-static)
    endif()
    message(STATUS "[Vulkan] Using pre-built spirv-reflect target")
elseif(NOT TARGET spirv-reflect-static)
    # Standalone — fetch from source.
    include(FetchContent)
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
    snap_rhi_export(spirv-reflect-static)
endif()

# ============================================================================
# Source Files
# ============================================================================
file(GLOB_RECURSE SNAP_RHI_VULKAN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Create the library with namespaced alias
snap_rhi_add_library(backend-vulkan STATIC ${SNAP_RHI_VULKAN_SOURCES})

# Public include directories
target_include_directories(backend-vulkan PUBLIC include)

# Link to public-api, backend-common, glad-vulkan, and spirv-reflect
snap_rhi_target_link_libraries(backend-vulkan snap-rhi::public-api snap-rhi::backend-common glad::glad-vulkan spirv-reflect-static)

# When MoltenVK is provided as an imported target on iOS, it is linked
# statically — bypass GLAD's dynamic loader entirely.
if(IOS AND TARGET molten-vk::molten-vk)
    target_compile_definitions(backend-vulkan PUBLIC SNAP_RHI_VULKAN_STATIC_LINK=1)
    # MoltenVK contains Objective-C classes (Metal wrappers) that the C++
    # linker would strip as unreferenced.  -ObjC forces the linker to keep
    # all ObjC classes and categories from static archives.
    target_link_options(backend-vulkan INTERFACE "-ObjC")
endif()

# ============================================================================
# Vulkan Validation Layers
# ============================================================================
if(SNAP_RHI_VULKAN_ENABLE_LAYERS)
    target_compile_definitions(backend-vulkan PUBLIC SNAP_RHI_VULKAN_NATIVE_VALIDATION_LAYERS=1)
    if(TARGET VULKAN_LOADER::vulkan)
        snap_rhi_target_link_libraries(backend-vulkan VULKAN_LOADER::vulkan)
    endif()
    if(TARGET VVL::vvl)
        snap_rhi_target_link_libraries(backend-vulkan VVL::VkLayer_utils VVL::gpu_av_spirv VVL::vvl)
    endif()
else()
    target_compile_definitions(backend-vulkan PUBLIC SNAP_RHI_VULKAN_NATIVE_VALIDATION_LAYERS=0)
endif()

# Compiler warnings
snap_rhi_setup_compiler_warnings(backend-vulkan)

# ============================================================================
# Vulkan Runtime Manifests (desktop builds with imported targets only)
# ============================================================================
# When Vulkan libraries are provided as imported targets, generate JSON
# manifests at configure time with absolute library paths.  Pass those paths
# to Device.cpp as PRIVATE definitions so it can point the Vulkan Loader at
# them before vkCreateInstance.  PRIVATE ensures the build-directory-specific
# paths are never exported to downstream consumers.
#
# Only for desktop platforms (macOS, Linux, Windows) — iOS and Android use
# different Vulkan library discovery mechanisms (embedded frameworks / APK
# native libs) where build-machine paths are not applicable.
if(NOT IOS AND NOT ANDROID)
    set(_VK_MANIFEST_DIR "${CMAKE_BINARY_DIR}/vulkan-manifests")

    # MoltenVK ICD manifest (macOS)
    if(TARGET molten-vk::molten-vk)
        set(_VK_ICD_MANIFEST "${_VK_MANIFEST_DIR}/MoltenVK_icd.json")
        file(GENERATE
            OUTPUT "${_VK_ICD_MANIFEST}"
            CONTENT [[{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "$<TARGET_FILE:molten-vk::molten-vk>",
        "api_version": "1.2.0",
        "is_portability_driver": true
    }
}
]]
        )
        target_compile_definitions(backend-vulkan PRIVATE
            SNAP_RHI_VK_ICD_MANIFEST_PATH="${_VK_ICD_MANIFEST}")
        message(STATUS "[Vulkan] MoltenVK ICD manifest -> ${_VK_ICD_MANIFEST}")
    endif()

    # Vulkan Validation Layer manifest
    if(SNAP_RHI_VULKAN_ENABLE_LAYERS AND TARGET VVL::vvl)
        set(_VK_LAYER_DIR "${_VK_MANIFEST_DIR}/explicit_layer.d")
        file(GENERATE
            OUTPUT "${_VK_LAYER_DIR}/VkLayer_khronos_validation.json"
            CONTENT [[{
    "file_format_version": "1.2.0",
    "layer": {
        "name": "VK_LAYER_KHRONOS_validation",
        "type": "GLOBAL",
        "library_path": "$<TARGET_FILE:VVL::vvl>",
        "api_version": "1.4.309",
        "implementation_version": "1",
        "description": "Khronos Validation Layer"
    }
}
]]
        )
        target_compile_definitions(backend-vulkan PRIVATE
            SNAP_RHI_VK_LAYER_MANIFEST_DIR="${_VK_LAYER_DIR}")
        message(STATUS "[Vulkan] VVL layer manifest -> ${_VK_LAYER_DIR}")
    endif()
endif()

# Export target for packaging
snap_rhi_export(backend-vulkan)
