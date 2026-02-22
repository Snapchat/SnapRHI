# ============================================================================
# GLAD Source
# ============================================================================

# Platform library (common)
add_library(glad-platform STATIC gladPlatfrom.cpp)
add_library(glad::glad-platform ALIAS glad-platform)
target_include_directories(glad-platform PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# OpenGL loader (optional, included by parent when SNAP_RHI_ENABLE_OPENGL)
if(SNAP_RHI_ENABLE_OPENGL)
    add_subdirectory(OpenGL)
endif()

# Vulkan loader (optional, included by parent when SNAP_RHI_ENABLE_VULKAN)
if(SNAP_RHI_ENABLE_VULKAN)
    add_subdirectory(Vulkan)
endif()
