# ============================================================================
# SnapRHI CMake Helpers
# ============================================================================
include_guard(GLOBAL)

# Creates a library target and adds a namespaced alias for it.
function(snap_rhi_add_library TARGET_NAME)
    add_library(${TARGET_NAME} ${ARGN})
    add_library(snap-rhi::${TARGET_NAME} ALIAS ${TARGET_NAME})
endfunction()

# Wrapper around target_link_libraries for consistent usage across SnapRHI.
# Dependencies are always PUBLIC (transitive) — use target_link_libraries
# PRIVATE directly for non-transitive deps like system frameworks.
# Overridden by the packaging module when present.
function(snap_rhi_target_link_libraries TARGET_NAME)
    target_link_libraries(${TARGET_NAME} PUBLIC ${ARGN})
endfunction()

# Sets up public include directories for a SnapRHI target.
# INCLUDE_DIR is a relative path from the calling CMakeLists.txt directory.
# Use "." for the current source directory, or "include" for a subdirectory.
# Overridden by the packaging module when present.
function(snap_rhi_target_include_directories TARGET_NAME INCLUDE_DIR)
    if(INCLUDE_DIR STREQUAL ".")
        set(_abs_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    else()
        set(_abs_dir "${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}")
    endif()
    target_include_directories(${TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${_abs_dir}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
endfunction()

# Rewrites a target's INTERFACE_INCLUDE_DIRECTORIES to use generator
# expressions for install-tree compatibility.  Used to patch third-party
# targets (e.g. spirv-reflect) whose upstream CMakeLists set absolute source
# paths as PUBLIC includes.  Overridden by the packaging module when present.
function(snap_rhi_patch_target_includes TARGET_NAME)
    get_target_property(_inc ${TARGET_NAME} INTERFACE_INCLUDE_DIRECTORIES)
    if(_inc)
        set(_patched "")
        foreach(_dir IN LISTS _inc)
            list(APPEND _patched
                $<BUILD_INTERFACE:${_dir}>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
            )
        endforeach()
        set_target_properties(${TARGET_NAME} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_patched}"
        )
    endif()
endfunction()

# Export helper — call after a target is fully configured.
# No-op by default; overridden by packaging module when present.
function(snap_rhi_export)
    # Intentionally empty — packaging hook.
endfunction()

# Export-complete helper — call once at the end of the root CMakeLists.
# No-op by default; overridden by packaging module when present.
function(snap_rhi_export_complete)
    # Intentionally empty — packaging hook.
endfunction()

# Hook for additional packaging modules.
# Called after all backends and optional modules are configured.
# No-op by default; overridden by packaging module when present.
function(snap_rhi_add_packaging_modules)
    # Intentionally empty — packaging hook.
endfunction()

# Finds an Apple framework and creates an imported target for it.
function(snap_rhi_find_framework FRAMEWORK_NAME)
    cmake_parse_arguments(ARG "REQUIRED" "" "" ${ARGN})
    find_library(${FRAMEWORK_NAME}_FRAMEWORK ${FRAMEWORK_NAME})
    if(NOT ${FRAMEWORK_NAME}_FRAMEWORK)
        if(ARG_REQUIRED)
            message(FATAL_ERROR "Required framework '${FRAMEWORK_NAME}' not found")
        endif()
        return()
    endif()
    if(NOT TARGET SnapRHI::Framework::${FRAMEWORK_NAME})
        add_library(SnapRHI::Framework::${FRAMEWORK_NAME} INTERFACE IMPORTED GLOBAL)
        set_target_properties(SnapRHI::Framework::${FRAMEWORK_NAME} PROPERTIES
            INTERFACE_LINK_LIBRARIES "${${FRAMEWORK_NAME}_FRAMEWORK}"
        )
    endif()
    message(STATUS "Found framework: ${FRAMEWORK_NAME} -> ${${FRAMEWORK_NAME}_FRAMEWORK}")
endfunction()
# Sets up compiler warning flags for a target.
function(snap_rhi_setup_compiler_warnings TARGET_NAME)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${TARGET_NAME} PRIVATE
            -ftemplate-backtrace-limit=0
            -Wall
            -Wno-deprecated-declarations
            -Wno-error=missing-prototypes
            -Wno-error=float-conversion
            -Wno-error=sign-compare
            -Wno-error=unused-parameter
            -Wno-error=sign-conversion
            -Wno-error=conversion
            -Wno-unknown-warning-option
            "-Wno-error=#warnings"
        )
        if(SNAP_RHI_WARNINGS_AS_ERRORS)
            target_compile_options(${TARGET_NAME} PRIVATE -Werror)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter)
        if(SNAP_RHI_WARNINGS_AS_ERRORS)
            target_compile_options(${TARGET_NAME} PRIVATE -Werror)
        endif()
    elseif(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /wd4100 /wd4189)
        if(SNAP_RHI_WARNINGS_AS_ERRORS)
            target_compile_options(${TARGET_NAME} PRIVATE /WX)
        endif()
    endif()
endfunction()
