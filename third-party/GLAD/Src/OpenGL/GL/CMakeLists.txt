# ============================================================================
# GLAD Desktop OpenGL (GL)
# ============================================================================

file(GLOB_RECURSE GLAD_GL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/*.c ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/*.cpp)
snap_rhi_add_library(glad-platform-gl STATIC ${GLAD_GL_SRC})

snap_rhi_target_include_directories(glad-platform-gl .)

snap_rhi_target_link_libraries(glad-platform-gl glad::glad-platform)

# Platform-specific OpenGL linking
if(APPLE)
    snap_rhi_target_link_libraries(glad-platform-gl "-framework OpenGL")
elseif(WIN32)
    find_package(OpenGL REQUIRED)
    snap_rhi_target_link_libraries(glad-platform-gl ${OPENGL_LIBRARIES})
    if(MSVC)
        target_compile_options(glad-platform-gl PRIVATE
            /wd5030  # attribute not recognized
            /wd4068  # unknown pragma
        )
    endif()
elseif(UNIX)
    # Linux OpenGL backend selection
    set(GLAD_OPENGL_BACKEND_LINUX "glx" CACHE STRING "Linux OpenGL backend (glx, egl, osmesa)")

    snap_rhi_target_link_libraries(glad-platform-gl ${CMAKE_DL_LIBS})

    if(GLAD_OPENGL_BACKEND_LINUX STREQUAL "osmesa")
        message(STATUS "GLAD: Using OSMesa for OpenGL backend")
        # OSMesa linking handled by application
    elseif(GLAD_OPENGL_BACKEND_LINUX STREQUAL "egl")
        message(STATUS "GLAD: Using EGL for OpenGL backend")
        snap_rhi_target_link_libraries(glad-platform-gl EGL GL)
    else()
        message(STATUS "GLAD: Using GLX for OpenGL backend")
        find_package(X11)
        if(X11_FOUND)
            snap_rhi_target_link_libraries(glad-platform-gl ${X11_LIBRARIES} GL)
        else()
            # Cross-compilation: FindX11 cannot locate X11 in the sysroot.
            # Use explicit linker flags so the toolchain resolves them.
            message(STATUS "GLAD: X11 not found via FindX11 — linking directly (cross-compilation)")
            snap_rhi_target_link_libraries(glad-platform-gl -lX11 -lGLX -lGL)
        endif()
    endif()
endif()

# Export target for packaging
snap_rhi_export(glad-platform-gl)
