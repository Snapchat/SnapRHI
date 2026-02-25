# ============================================================================
# GLAD OpenGL Loader
# ============================================================================

file(GLOB GLAD_LOADER_OPENGL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
snap_rhi_add_library(glad-gl STATIC ${GLAD_LOADER_OPENGL_SRC})

target_include_directories(glad-gl PUBLIC include)

# Determine desktop vs mobile OpenGL
set(GLAD_DESKTOP 0)
set(GLAD_MOBILE 0)

if(ANDROID OR IOS OR EMSCRIPTEN OR SNAP_RHI_OPENGL_ES)
    set(GLAD_MOBILE 1)
    add_subdirectory(GLES)
    snap_rhi_target_link_libraries(glad-gl glad-platform-gl-es)
    message(STATUS "GLAD: Using OpenGL ES backend")
elseif(WIN32 OR UNIX)
    set(GLAD_DESKTOP 1)
    add_subdirectory(GL)
    snap_rhi_target_link_libraries(glad-gl glad-platform-gl)
    message(STATUS "GLAD: Using OpenGL backend")
endif()

target_compile_definitions(glad-gl PUBLIC
    GLAD_OPENGL_ES=${GLAD_MOBILE}
    GLAD_OPENGL=${GLAD_DESKTOP}
)

snap_rhi_target_link_libraries(glad-gl glad::glad-platform)

# Add glad:: alias for consistent target naming across build configurations
if(NOT TARGET glad::glad-gl)
    add_library(glad::glad-gl ALIAS glad-gl)
endif()

# Export target for packaging
snap_rhi_export(glad-gl)
