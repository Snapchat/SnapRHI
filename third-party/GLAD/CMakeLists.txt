# ============================================================================
# GLAD - OpenGL/Vulkan Loader
# ============================================================================

cmake_minimum_required(VERSION 3.25)
include_guard(GLOBAL)

project(glad LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJC OBJCXX)
endif()

add_subdirectory(Src)
