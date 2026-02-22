# ============================================================================
# GLAD OpenGL Loader
# ============================================================================

file(GLOB GLAD_LOADER_OPENGL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(glad-gl STATIC ${GLAD_LOADER_OPENGL_SRC})
add_library(glad::glad-gl ALIAS glad-gl)

target_include_directories(glad-gl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Determine desktop vs mobile OpenGL
set(GLAD_DESKTOP 0)
set(GLAD_MOBILE 0)

if(ANDROID OR IOS)
    set(GLAD_MOBILE 1)
    add_subdirectory(GLES)
    target_link_libraries(glad-gl PUBLIC glad-platform-gl-es)
    message(STATUS "GLAD: Using OpenGL ES backend")
elseif(WIN32 OR UNIX)
    set(GLAD_DESKTOP 1)
    add_subdirectory(GL)
    target_link_libraries(glad-gl PUBLIC glad-platform-gl)
    message(STATUS "GLAD: Using OpenGL backend")
endif()

target_compile_definitions(glad-gl PUBLIC
    GLAD_OPENGL_ES=${GLAD_MOBILE}
    GLAD_OPENGL=${GLAD_DESKTOP}
)

target_link_libraries(glad-gl PUBLIC glad::glad-platform)
