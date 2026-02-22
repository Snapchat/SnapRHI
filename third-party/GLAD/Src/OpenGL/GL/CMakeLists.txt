# ============================================================================
# GLAD Desktop OpenGL (GL)
# ============================================================================

file(GLOB_RECURSE GLAD_GL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/*.c ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/*.cpp)
add_library(glad-platform-gl STATIC ${GLAD_GL_SRC})
add_library(glad::glad-platform-gl ALIAS glad-platform-gl)

target_include_directories(glad-platform-gl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(glad-platform-gl PUBLIC glad::glad-platform)

# Platform-specific OpenGL linking
if(APPLE)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    target_link_libraries(glad-platform-gl PUBLIC ${OPENGL_FRAMEWORK})
elseif(WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(glad-platform-gl PUBLIC ${OPENGL_LIBRARIES})
    if(MSVC)
        target_compile_options(glad-platform-gl PRIVATE
            /wd5030  # attribute not recognized
            /wd4068  # unknown pragma
        )
    endif()
elseif(UNIX)
    # Linux OpenGL backend selection
    set(GLAD_OPENGL_BACKEND_LINUX "glx" CACHE STRING "Linux OpenGL backend (glx, egl, osmesa)")

    target_link_libraries(glad-platform-gl PUBLIC ${CMAKE_DL_LIBS})

    if(GLAD_OPENGL_BACKEND_LINUX STREQUAL "osmesa")
        message(STATUS "GLAD: Using OSMesa for OpenGL backend")
        # OSMesa linking handled by application
    elseif(GLAD_OPENGL_BACKEND_LINUX STREQUAL "egl")
        message(STATUS "GLAD: Using EGL for OpenGL backend")
        target_link_libraries(glad-platform-gl PUBLIC EGL GL)
    else()
        message(STATUS "GLAD: Using GLX for OpenGL backend")
        find_package(X11 REQUIRED)
        target_link_libraries(glad-platform-gl PUBLIC ${X11_LIBRARIES} GL)
    endif()
endif()
