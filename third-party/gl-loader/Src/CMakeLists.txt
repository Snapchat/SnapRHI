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
elseif(ANDROID)
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
add_library(gl-loader STATIC ${GL_LOADER_SOURCES})
add_library(gl-loader::gl-loader ALIAS gl-loader)

target_include_directories(gl-loader PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link to GLAD
target_link_libraries(gl-loader PUBLIC glad::glad-gl)

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
        find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
        find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
        target_link_libraries(gl-loader PRIVATE ${OPENGL_FRAMEWORK} ${APPKIT_FRAMEWORK})
    else()
        find_library(OPENGLES_FRAMEWORK OpenGLES REQUIRED)
        target_link_libraries(gl-loader PRIVATE ${OPENGLES_FRAMEWORK})
    endif()
elseif(WIN32)
    target_link_libraries(gl-loader PRIVATE opengl32)
elseif(ANDROID)
    target_link_libraries(gl-loader PRIVATE EGL GLESv2)
elseif(UNIX AND NOT EMSCRIPTEN)
    if(GL_LOADER_LINUX_BACKEND STREQUAL "egl")
        target_link_libraries(gl-loader PRIVATE EGL GL)
    elseif(GL_LOADER_LINUX_BACKEND STREQUAL "osmesa")
        target_link_libraries(gl-loader PRIVATE OSMesa)
    else()
        find_package(X11 REQUIRED)
        target_link_libraries(gl-loader PRIVATE ${X11_LIBRARIES} GL)
    endif()
endif()
