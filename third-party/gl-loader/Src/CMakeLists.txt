# ============================================================================
# GL-Loader Source
# ============================================================================

# Collect common source files
file(GLOB GL_LOADER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Platform-specific sources
if(APPLE AND NOT IOS)
    file(GLOB_RECURSE GL_LOADER_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/macOS/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/macOS/*.mm
    )
elseif(IOS)
    file(GLOB_RECURSE GL_LOADER_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/*.mm
    )
elseif(ANDROID OR SNAP_RHI_OPENGL_ES)
    file(GLOB_RECURSE GL_LOADER_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/EGL/*.cpp
    )
elseif(WIN32)
    file(GLOB_RECURSE GL_LOADER_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Windows/*.cpp
    )
elseif(EMSCRIPTEN)
    file(GLOB_RECURSE GL_LOADER_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/WebAssembly/*.cpp
    )
elseif(UNIX)
    set(GL_LOADER_LINUX_BACKEND "glx" CACHE STRING "Linux OpenGL backend (glx, egl, osmesa)")
    if(GL_LOADER_LINUX_BACKEND STREQUAL "egl")
        file(GLOB_RECURSE GL_LOADER_PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/EGL/*.cpp
        )
    elseif(GL_LOADER_LINUX_BACKEND STREQUAL "osmesa")
        file(GLOB_RECURSE GL_LOADER_PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/OSMesa/*.cpp
        )
    else()
        file(GLOB_RECURSE GL_LOADER_PLATFORM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/GLX/*.cpp
        )
    endif()
endif()

list(APPEND GL_LOADER_SOURCES ${GL_LOADER_PLATFORM_SOURCES})

# Create library
snap_rhi_add_library(gl-loader STATIC ${GL_LOADER_SOURCES})

target_include_directories(gl-loader PUBLIC include)

# Link to GLAD
snap_rhi_target_link_libraries(gl-loader glad::glad-gl)

# Platform-specific settings
if(APPLE)
    # Set ARC for Objective-C++ files
    file(GLOB_RECURSE GL_LOADER_OBJCXX_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/macOS/*.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/*.mm
    )
    if(GL_LOADER_OBJCXX_SOURCES)
        set_source_files_properties(${GL_LOADER_OBJCXX_SOURCES} PROPERTIES
            COMPILE_FLAGS "-fobjc-arc"
        )
    endif()

    if(NOT IOS)
        snap_rhi_target_link_libraries(gl-loader "-framework OpenGL" "-framework AppKit")
    else()
        snap_rhi_target_link_libraries(gl-loader "-framework OpenGLES")
    endif()
elseif(WIN32)
    snap_rhi_target_link_libraries(gl-loader opengl32)
elseif(SNAP_RHI_GL_LOADER_LIBRARIES)
    # Cross-compilation: link libraries provided by the build system
    snap_rhi_target_link_libraries(gl-loader ${SNAP_RHI_GL_LOADER_LIBRARIES})
elseif(ANDROID)
    snap_rhi_target_link_libraries(gl-loader EGL GLESv2)
elseif(UNIX AND NOT EMSCRIPTEN)
    if(GL_LOADER_LINUX_BACKEND STREQUAL "egl")
        snap_rhi_target_link_libraries(gl-loader EGL GL)
    elseif(GL_LOADER_LINUX_BACKEND STREQUAL "osmesa")
        snap_rhi_target_link_libraries(gl-loader OSMesa)
    else()
        find_package(X11)
        if(X11_FOUND)
            snap_rhi_target_link_libraries(gl-loader ${X11_LIBRARIES} GL)
        else()
            # Cross-compilation: FindX11 cannot locate X11 in the sysroot.
            # Use explicit linker flags so the toolchain resolves them.
            message(STATUS "gl-loader: X11 not found via FindX11 — linking directly (cross-compilation)")
            snap_rhi_target_link_libraries(gl-loader -lX11 -lGLX -lGL)
        endif()
    endif()
endif()

# Add gl-loader:: alias for consistent target naming across build configurations
if(NOT TARGET gl-loader::gl-loader)
    add_library(gl-loader::gl-loader ALIAS gl-loader)
endif()

# Export target for packaging
snap_rhi_export(gl-loader)
