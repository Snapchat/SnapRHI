# ============================================================================
# SnapRHI CMake Install Rules
# ============================================================================
# Defines install targets for all SnapRHI libraries and headers.
# Generates a CMake config package so consumers can use:
#
#   find_package(SnapRHI REQUIRED)
#   target_link_libraries(app PRIVATE snap-rhi::public-api snap-rhi::backend-vulkan)
#
# Included from the root CMakeLists.txt when SNAP_RHI_INSTALL is ON.
# ============================================================================

include_guard(GLOBAL)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ── Collect targets to install ───────────────────────────────────────────────
set(_SNAP_RHI_INSTALL_TARGETS public-api backend-common)

# Third-party: GLAD platform (always present when Vulkan or OpenGL is enabled)
if(TARGET glad-platform)
    list(APPEND _SNAP_RHI_INSTALL_TARGETS glad-platform)
endif()

if(SNAP_RHI_ENABLE_METAL)
    list(APPEND _SNAP_RHI_INSTALL_TARGETS backend-metal)
endif()

if(SNAP_RHI_ENABLE_VULKAN)
    list(APPEND _SNAP_RHI_INSTALL_TARGETS backend-vulkan)
    # GLAD Vulkan loader
    if(TARGET glad-vulkan)
        list(APPEND _SNAP_RHI_INSTALL_TARGETS glad-vulkan)
    endif()
    # spirv-reflect is a build dependency of backend-vulkan
    if(TARGET spirv-reflect-static)
        list(APPEND _SNAP_RHI_INSTALL_TARGETS spirv-reflect-static)
    endif()
endif()

if(SNAP_RHI_ENABLE_OPENGL)
    list(APPEND _SNAP_RHI_INSTALL_TARGETS backend-opengl)
    # GLAD OpenGL loader + platform-specific GL sub-target
    if(TARGET glad-gl)
        list(APPEND _SNAP_RHI_INSTALL_TARGETS glad-gl)
    endif()
    if(TARGET glad-platform-gl)
        list(APPEND _SNAP_RHI_INSTALL_TARGETS glad-platform-gl)
    endif()
    if(TARGET glad-platform-gl-es)
        list(APPEND _SNAP_RHI_INSTALL_TARGETS glad-platform-gl-es)
    endif()
    # gl-loader
    if(TARGET gl-loader)
        list(APPEND _SNAP_RHI_INSTALL_TARGETS gl-loader)
    endif()
endif()

if(SNAP_RHI_ENABLE_NOOP)
    list(APPEND _SNAP_RHI_INSTALL_TARGETS backend-noop)
endif()

if(SNAP_RHI_WITH_INTEROP)
    list(APPEND _SNAP_RHI_INSTALL_TARGETS interop)
endif()

# ── Install libraries ────────────────────────────────────────────────────────
install(TARGETS ${_SNAP_RHI_INSTALL_TARGETS}
    EXPORT SnapRHITargets
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ── Install public headers ───────────────────────────────────────────────────
# Each target's public headers live under  <target>/include/snap/
# Install them all into a single <prefix>/include/ tree so
# #include <snap/rhi/...> works.

install(DIRECTORY src/public-api/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(DIRECTORY src/backend/common/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

if(SNAP_RHI_ENABLE_METAL)
    install(DIRECTORY src/backend/metal/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
endif()

if(SNAP_RHI_ENABLE_VULKAN)
    install(DIRECTORY src/backend/vulkan/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
endif()

if(SNAP_RHI_ENABLE_OPENGL)
    install(DIRECTORY src/backend/opengl/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
endif()

if(SNAP_RHI_WITH_INTEROP)
    install(DIRECTORY src/interop/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
endif()

# ── Install GLAD / gl-loader headers ─────────────────────────────────────────
# These third-party targets expose public headers that consumers need.

if(SNAP_RHI_ENABLE_VULKAN OR SNAP_RHI_ENABLE_OPENGL)
    # glad-platform headers
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLAD/Src/include")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLAD/Src/include/"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
        )
    endif()
endif()

if(SNAP_RHI_ENABLE_VULKAN)
    # glad-vulkan headers (GLAD/ subdirectory at Vulkan source root)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLAD/Src/Vulkan/GLAD")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLAD/Src/Vulkan/GLAD"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h"
        )
    endif()
    # spirv-reflect headers
    if(TARGET spirv-reflect-static)
        get_target_property(_spirv_src_dir spirv-reflect-static SOURCE_DIR)
        if(_spirv_src_dir AND EXISTS "${_spirv_src_dir}/include")
            install(DIRECTORY "${_spirv_src_dir}/include/"
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                FILES_MATCHING PATTERN "*.h"
            )
        elseif(_spirv_src_dir AND EXISTS "${_spirv_src_dir}/spirv_reflect.h")
            install(FILES "${_spirv_src_dir}/spirv_reflect.h"
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            )
        endif()
    endif()
endif()

if(SNAP_RHI_ENABLE_OPENGL)
    # glad-gl headers
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLAD/Src/OpenGL/include")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLAD/Src/OpenGL/include/"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h"
        )
    endif()
    # glad-platform-gl or glad-platform-gl-es headers
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLAD/Src/OpenGL/GL/GLAD")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLAD/Src/OpenGL/GL/GLAD"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h"
        )
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLAD/Src/OpenGL/GLES/GLAD")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLAD/Src/OpenGL/GLES/GLAD"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h"
        )
    endif()
    # gl-loader headers
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/gl-loader/Src/include")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/gl-loader/Src/include/"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
        )
    endif()
endif()

# ── CMake package config ─────────────────────────────────────────────────────
install(EXPORT SnapRHITargets
    FILE        SnapRHITargets.cmake
    NAMESPACE   snap-rhi::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SnapRHI
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SnapRHIConfigVersion.cmake"
    VERSION     ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SnapRHIConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SnapRHIConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SnapRHI
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SnapRHIConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SnapRHIConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SnapRHI
)

# ── Install license ─────────────────────────────────────────────────────────
install(FILES LICENSE.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
