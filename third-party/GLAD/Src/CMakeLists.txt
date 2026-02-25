# ============================================================================
# GLAD Source
# ============================================================================

# Platform library (common)
snap_rhi_add_library(glad-platform STATIC gladPlatfrom.cpp)
target_include_directories(glad-platform PUBLIC include)

# Add glad:: alias for consistent target naming across build configurations
if(NOT TARGET glad::glad-platform)
    add_library(glad::glad-platform ALIAS glad-platform)
endif()

# Export target for packaging
snap_rhi_export(glad-platform)

# OpenGL loader (optional, included by parent when SNAP_RHI_ENABLE_OPENGL)
if(SNAP_RHI_ENABLE_OPENGL)
    add_subdirectory(OpenGL)
endif()

# Vulkan loader (optional, included by parent when SNAP_RHI_ENABLE_VULKAN)
if(SNAP_RHI_ENABLE_VULKAN)
    add_subdirectory(Vulkan)
endif()
