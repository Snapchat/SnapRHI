# ============================================================================
# GL-Loader - OpenGL Context Management
# ============================================================================

cmake_minimum_required(VERSION 3.25)
include_guard(GLOBAL)

project(gl-loader LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJC OBJCXX)
endif()

add_subdirectory(Src)
