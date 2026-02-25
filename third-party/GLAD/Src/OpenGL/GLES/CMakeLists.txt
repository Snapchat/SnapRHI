# ============================================================================
# GLAD OpenGL ES (GLES)
# ============================================================================

file(GLOB_RECURSE GLAD_GLES_SRC ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/*.c ${CMAKE_CURRENT_SOURCE_DIR}/GLAD/*.cpp)
snap_rhi_add_library(glad-platform-gl-es STATIC ${GLAD_GLES_SRC})

snap_rhi_target_include_directories(glad-platform-gl-es .)

snap_rhi_target_link_libraries(glad-platform-gl-es glad::glad-platform)

# Platform-specific OpenGL ES linking
if(SNAP_RHI_OPENGL_ES_LIBRARIES)
    # Cross-compilation: link libraries provided by the build system
    snap_rhi_target_link_libraries(glad-platform-gl-es ${SNAP_RHI_OPENGL_ES_LIBRARIES})
elseif(ANDROID)
    snap_rhi_target_link_libraries(glad-platform-gl-es GLESv2 EGL)
elseif(IOS)
    snap_rhi_target_link_libraries(glad-platform-gl-es "-framework GLKit" "-framework OpenGLES")
elseif(EMSCRIPTEN)
    # Emscripten provides its own GL implementation — no extra libs needed
endif()

# Export target for packaging
snap_rhi_export(glad-platform-gl-es)
