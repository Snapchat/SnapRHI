# ============================================================================
# GLAD OpenGL ES (GLES)
# ============================================================================

file(GLOB_RECURSE GLAD_GLES_SRC ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/*.c ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/*.cpp)
add_library(glad-platform-gl-es STATIC ${GLAD_GLES_SRC})
add_library(glad::glad-platform-gl-es ALIAS glad-platform-gl-es)

target_include_directories(glad-platform-gl-es PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(glad-platform-gl-es PUBLIC glad::glad-platform)

# Platform-specific OpenGL ES linking
if(ANDROID)
    target_link_libraries(glad-platform-gl-es PUBLIC GLESv2 EGL)
elseif(IOS)
    find_library(GLKIT_FRAMEWORK GLKit)
    find_library(OPENGLES_FRAMEWORK OpenGLES)
    target_link_libraries(glad-platform-gl-es PUBLIC
        ${GLKIT_FRAMEWORK}
        ${OPENGLES_FRAMEWORK}
    )
endif()
