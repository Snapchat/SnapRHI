# ============================================================================
# SnapRHI Unit Tests (Catch2 v2)
# ============================================================================
# Usage:
#   cmake -B build -S . -DSNAP_RHI_BUILD_TESTS=ON -DSNAP_RHI_ENABLE_METAL=ON
#   cmake --build build --target snap-rhi-tests
#   ctest --test-dir build --output-on-failure
# ============================================================================

# --------------------------------------------------------------------------
# Catch2 v2 dependency
# --------------------------------------------------------------------------
find_package(Catch2 2 QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.10
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# --------------------------------------------------------------------------
# Collect test sources
# --------------------------------------------------------------------------
file(GLOB_RECURSE SNAP_RHI_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# --------------------------------------------------------------------------
# Test executable
# --------------------------------------------------------------------------
add_executable(snap-rhi-tests ${SNAP_RHI_TEST_SOURCES})

target_include_directories(snap-rhi-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(snap-rhi-tests PRIVATE cxx_std_20)

# --------------------------------------------------------------------------
# Link SnapRHI library targets
# --------------------------------------------------------------------------
# Catch2 v2 does not provide Catch2WithMain; main() lives in tests/src/main.cpp
target_link_libraries(snap-rhi-tests PRIVATE
    snap-rhi::public-api
    snap-rhi::backend-common
    Catch2::Catch2
)

if(SNAP_RHI_ENABLE_METAL AND TARGET snap-rhi::backend-metal)
    target_link_libraries(snap-rhi-tests PRIVATE snap-rhi::backend-metal)
endif()

if(SNAP_RHI_ENABLE_VULKAN AND TARGET snap-rhi::backend-vulkan)
    target_link_libraries(snap-rhi-tests PRIVATE snap-rhi::backend-vulkan)
endif()

if(SNAP_RHI_ENABLE_OPENGL AND TARGET snap-rhi::backend-opengl)
    target_link_libraries(snap-rhi-tests PRIVATE snap-rhi::backend-opengl)
endif()

if(SNAP_RHI_WITH_INTEROP AND TARGET snap-rhi::interop)
    target_link_libraries(snap-rhi-tests PRIVATE snap-rhi::interop)
endif()

# --------------------------------------------------------------------------
# Vulkan SPIR-V test shaders
# --------------------------------------------------------------------------
# When glslc is available the shaders are compiled from GLSL at build time.
# Otherwise the pre-generated header (tests/generated/VulkanSPIRV.h) is used
# as a fallback so Vulkan tests always run regardless of the build environment.
if(SNAP_RHI_ENABLE_VULKAN)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompileVulkanShaders.cmake")
    compile_vulkan_shaders(
        TARGET          snap-rhi-tests
        SHADER_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan"
        OUTPUT_HEADER   "${CMAKE_CURRENT_BINARY_DIR}/generated/VulkanSPIRV.h"
        FALLBACK_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/generated/VulkanSPIRV.h"
    )
else()
    target_compile_definitions(snap-rhi-tests PRIVATE
        SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE=0)
endif()

# --------------------------------------------------------------------------
# CTest integration
# --------------------------------------------------------------------------
include(CTest)
add_test(NAME snap-rhi-tests COMMAND snap-rhi-tests)
