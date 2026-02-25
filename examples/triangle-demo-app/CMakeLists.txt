# ============================================================================
# SnapRHI Triangle Demo Application
# ============================================================================
# A simple triangle rendering demo showcasing SnapRHI capabilities.
#
# Build (from SnapRHI root):
#   cmake -B build -DSNAP_RHI_ENABLE_METAL=ON -DSNAP_RHI_BUILD_EXAMPLES=ON
#   cmake --build build --target snap_rhi_demo_app
# ============================================================================

cmake_minimum_required(VERSION 3.25)
include_guard(GLOBAL)

# Only define project if building standalone (not as subdirectory)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(SnapRhiDemoApp LANGUAGES C CXX)

    # When building standalone, we need to find SnapRHI
    find_package(snap-rhi REQUIRED)
endif()

# ============================================================================
# Demo App Options
# ============================================================================
set(SNAP_RHI_DEMO_APP_API "metal" CACHE STRING "Graphics API for demo app")
set_property(CACHE SNAP_RHI_DEMO_APP_API PROPERTY STRINGS "metal" "vulkan" "opengl")

# Validate API choice matches enabled backend
if(SNAP_RHI_DEMO_APP_API STREQUAL "metal")
    if(NOT APPLE)
        message(FATAL_ERROR "Metal API is only available on Apple platforms")
    endif()
    if(NOT SNAP_RHI_ENABLE_METAL)
        message(FATAL_ERROR "Metal API selected but SNAP_RHI_ENABLE_METAL is OFF")
    endif()
elseif(SNAP_RHI_DEMO_APP_API STREQUAL "vulkan")
    if(NOT SNAP_RHI_ENABLE_VULKAN)
        message(FATAL_ERROR "Vulkan API selected but SNAP_RHI_ENABLE_VULKAN is OFF")
    endif()
elseif(SNAP_RHI_DEMO_APP_API STREQUAL "opengl")
    if(NOT SNAP_RHI_ENABLE_OPENGL)
        message(FATAL_ERROR "OpenGL API selected but SNAP_RHI_ENABLE_OPENGL is OFF")
    endif()
endif()

# ============================================================================
# Enable Objective-C/C++ on Apple
# ============================================================================
if(APPLE)
    enable_language(OBJC OBJCXX)
endif()

# ============================================================================
# FetchContent for Dependencies (SDL3 and GLM)
# ============================================================================
include(FetchContent)

# SDL3 - Cross-platform windowing and input
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

# GLM - Header-only math library
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

# Configure SDL3 build options
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)

message(STATUS "Fetching SDL3 and GLM dependencies...")
FetchContent_MakeAvailable(SDL3 glm)

# ============================================================================
# Source Files
# ============================================================================
file(GLOB DEMO_APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

# Platform-specific sources based on selected API
if(SNAP_RHI_DEMO_APP_API STREQUAL "metal" AND APPLE)
    file(GLOB_RECURSE DEMO_APP_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Metal/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Metal/*.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Metal/*.h
    )
elseif(SNAP_RHI_DEMO_APP_API STREQUAL "vulkan")
    file(GLOB_RECURSE DEMO_APP_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan/*.h
    )
elseif(SNAP_RHI_DEMO_APP_API STREQUAL "opengl")
    file(GLOB_RECURSE DEMO_APP_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/OpenGL/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/OpenGL/*.h
    )
endif()

list(APPEND DEMO_APP_SOURCES ${DEMO_APP_PLATFORM_SOURCES})

# ============================================================================
# Target Definition
# ============================================================================
if(ANDROID)
    add_library(snap_rhi_demo_app SHARED ${DEMO_APP_SOURCES})
else()
    add_executable(snap_rhi_demo_app ${DEMO_APP_SOURCES})
endif()

target_compile_features(snap_rhi_demo_app PRIVATE cxx_std_20)

# ============================================================================
# API Compile Definitions
# ============================================================================
if(SNAP_RHI_DEMO_APP_API STREQUAL "metal")
    target_compile_definitions(snap_rhi_demo_app PRIVATE
        SNAP_RHI_DEMO_APP_API_METAL=1
        SNAP_RHI_DEMO_APP_API_VULKAN=0
        SNAP_RHI_DEMO_APP_API_OPENGL=0
    )
elseif(SNAP_RHI_DEMO_APP_API STREQUAL "vulkan")
    target_compile_definitions(snap_rhi_demo_app PRIVATE
        SNAP_RHI_DEMO_APP_API_METAL=0
        SNAP_RHI_DEMO_APP_API_VULKAN=1
        SNAP_RHI_DEMO_APP_API_OPENGL=0
    )
elseif(SNAP_RHI_DEMO_APP_API STREQUAL "opengl")
    target_compile_definitions(snap_rhi_demo_app PRIVATE
        SNAP_RHI_DEMO_APP_API_METAL=0
        SNAP_RHI_DEMO_APP_API_VULKAN=0
        SNAP_RHI_DEMO_APP_API_OPENGL=1
    )
endif()

# ============================================================================
# Link Dependencies
# ============================================================================

# GLM (header-only)
target_link_libraries(snap_rhi_demo_app PRIVATE glm::glm)

# SDL3
target_link_libraries(snap_rhi_demo_app PRIVATE SDL3::SDL3-static)

# SnapRHI libraries
target_link_libraries(snap_rhi_demo_app PRIVATE snap-rhi::public-api)

if(SNAP_RHI_DEMO_APP_API STREQUAL "metal")
    target_link_libraries(snap_rhi_demo_app PRIVATE snap-rhi::backend-metal)
elseif(SNAP_RHI_DEMO_APP_API STREQUAL "vulkan")
    target_link_libraries(snap_rhi_demo_app PRIVATE snap-rhi::backend-vulkan)
elseif(SNAP_RHI_DEMO_APP_API STREQUAL "opengl")
    target_link_libraries(snap_rhi_demo_app PRIVATE snap-rhi::backend-opengl)
endif()

# ============================================================================
# Android-specific Configuration
# ============================================================================
if(ANDROID)
    target_link_libraries(snap_rhi_demo_app PRIVATE
        android
        log
        OpenSLES
    )
endif()

# ============================================================================
# Apple Platform Configuration
# ============================================================================
if(APPLE)
    # Assets handling - include in bundle
    file(GLOB_RECURSE DEMO_APP_ASSETS CONFIGURE_DEPENDS "assets/*")
    target_sources(snap_rhi_demo_app PRIVATE ${DEMO_APP_ASSETS})

    # Set asset locations in bundle
    foreach(_asset IN LISTS DEMO_APP_ASSETS)
        file(RELATIVE_PATH _asset_rel "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${_asset}")
        get_filename_component(_asset_rel_dir "${_asset_rel}" DIRECTORY)
        if(_asset_rel_dir STREQUAL "")
            set_source_files_properties("${_asset}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources/assets")
        else()
            set_source_files_properties("${_asset}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources/assets/${_asset_rel_dir}")
        endif()
    endforeach()

    # Bundle settings
    set_target_properties(snap_rhi_demo_app PROPERTIES
        MACOSX_BUNDLE TRUE
        RESOURCE "${DEMO_APP_ASSETS}"
    )

    # Apple framework dependencies
    target_link_libraries(snap_rhi_demo_app PRIVATE
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework GameController"
        "-framework CoreHaptics"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework UniformTypeIdentifiers"
    )

    if(IOS)
        # iOS-specific fixes and frameworks
        target_link_options(snap_rhi_demo_app PRIVATE "-ObjC")
        target_link_libraries(snap_rhi_demo_app PRIVATE
            "-framework CoreBluetooth"
            "-framework CoreVideo"
            "-framework CoreGraphics"
            "-framework CoreMotion"
            "-framework Foundation"
            "-framework OpenGLES"
            "-framework UIKit"
        )

        set(IOS_PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios")
        set(APP_ICON "${IOS_PLATFORM_DIR}/Assets.xcassets")
        set(LAUNCH_SCREEN "${IOS_PLATFORM_DIR}/LaunchScreen.storyboard")

        if(EXISTS "${APP_ICON}" AND EXISTS "${LAUNCH_SCREEN}")
            target_sources(snap_rhi_demo_app PRIVATE ${APP_ICON} ${LAUNCH_SCREEN})
            set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
            set_source_files_properties(${LAUNCH_SCREEN} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        endif()

        set_target_properties(snap_rhi_demo_app PROPERTIES
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.snap.rhi.demo"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.snap.rhi.demo"
            MACOSX_BUNDLE_BUNDLE_NAME "SnapRHI"
            MACOSX_BUNDLE_INFO_PLIST "${IOS_PLATFORM_DIR}/Info.plist"
        )

        # Allow setting the development team via CMake cache or environment.
        # Required for on-device deployment.  Pass on the command line:
        #   -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=XXXXXXXXXX
        # or set the DEVELOPMENT_TEAM environment variable.
        if(NOT CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM AND DEFINED ENV{DEVELOPMENT_TEAM})
            set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "$ENV{DEVELOPMENT_TEAM}")
        endif()
        if(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM)
            set_target_properties(snap_rhi_demo_app PROPERTIES
                XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM}"
            )
        endif()

        # ================================================================
        # iOS Vulkan Frameworks Embedding (for validation layers support)
        # ================================================================
        # When SNAP_RHI_IOS_VULKAN_USE_FRAMEWORKS is ON, the Vulkan SDK's dynamic
        # frameworks must be embedded in the app bundle so the Vulkan Loader can
        # discover MoltenVK (ICD) and validation layers at runtime.
        #
        # Bundle layout:
        #   App.app/
        #     Frameworks/
        #       vulkan.framework          <- Vulkan Loader
        #       MoltenVK.framework        <- MoltenVK ICD
        #       VkLayer_khronos_validation.framework  <- validation layer
        #     vulkan/
        #       icd.d/MoltenVK_icd.json   <- ICD manifest
        #       explicit_layer.d/*.json   <- layer manifests
        #
        # NOTE: Apps with embedded dynamic frameworks are NOT eligible for
        # App Store submission. Use static mode for release builds.
        if(SNAP_RHI_DEMO_APP_API STREQUAL "vulkan" AND SNAP_RHI_IOS_VULKAN_USE_FRAMEWORKS)
            message(STATUS "[Demo] iOS Vulkan: Embedding Vulkan SDK frameworks for validation layers")

            # Embed vulkan.framework (Vulkan Loader)
            if(SNAP_RHI_IOS_VULKAN_FRAMEWORK)
                target_sources(snap_rhi_demo_app PRIVATE "${SNAP_RHI_IOS_VULKAN_FRAMEWORK}")
                set_source_files_properties("${SNAP_RHI_IOS_VULKAN_FRAMEWORK}" PROPERTIES
                    MACOSX_PACKAGE_LOCATION Frameworks
                )
            endif()

            # Embed MoltenVK.framework (ICD)
            if(SNAP_RHI_IOS_MOLTENVK_FRAMEWORK)
                target_sources(snap_rhi_demo_app PRIVATE "${SNAP_RHI_IOS_MOLTENVK_FRAMEWORK}")
                set_source_files_properties("${SNAP_RHI_IOS_MOLTENVK_FRAMEWORK}" PROPERTIES
                    MACOSX_PACKAGE_LOCATION Frameworks
                )
            endif()

            # Embed validation layer frameworks
            set(_LAYER_FRAMEWORKS
                "VkLayer_khronos_validation.framework"
            )
            foreach(_layer IN LISTS _LAYER_FRAMEWORKS)
                set(_layer_path "${SNAP_RHI_IOS_VULKAN_SDK_LIB}/${_layer}")
                if(EXISTS "${_layer_path}")
                    target_sources(snap_rhi_demo_app PRIVATE "${_layer_path}")
                    set_source_files_properties("${_layer_path}" PROPERTIES
                        MACOSX_PACKAGE_LOCATION Frameworks
                    )
                    message(STATUS "[Demo] Embedding layer: ${_layer}")
                endif()
            endforeach()

            # Copy ICD and layer JSON manifests into the bundle.
            # The Vulkan Loader expects manifests at specific relative paths from
            # the loader binary. The ICD manifest references MoltenVK via:
            #   "library_path": "../../Frameworks/MoltenVK.framework/MoltenVK"
            # So the manifest must be at: <bundle>/vulkan/icd.d/MoltenVK_icd.json

            # ICD manifest
            set(_ICD_JSON "${SNAP_RHI_IOS_VULKAN_SDK_SHARE}/vulkan/icd.d/MoltenVK_icd.json")
            if(EXISTS "${_ICD_JSON}")
                target_sources(snap_rhi_demo_app PRIVATE "${_ICD_JSON}")
                set_source_files_properties("${_ICD_JSON}" PROPERTIES
                    MACOSX_PACKAGE_LOCATION "vulkan/icd.d"
                )
            endif()

            # Layer manifests
            file(GLOB _LAYER_JSONS "${SNAP_RHI_IOS_VULKAN_SDK_SHARE}/vulkan/explicit_layer.d/*.json")
            foreach(_json IN LISTS _LAYER_JSONS)
                target_sources(snap_rhi_demo_app PRIVATE "${_json}")
                set_source_files_properties("${_json}" PROPERTIES
                    MACOSX_PACKAGE_LOCATION "vulkan/explicit_layer.d"
                )
            endforeach()

            # Ensure the Vulkan Loader can find frameworks at runtime
            set_target_properties(snap_rhi_demo_app PROPERTIES
                XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
            )

            # Code-sign all embedded frameworks after the build.
            # iOS requires every embedded framework to be signed with the same
            # identity as the app. The MACOSX_PACKAGE_LOCATION approach copies
            # frameworks but does not sign them automatically.
            #
            # We write a helper script at configure time and invoke it as a
            # post-build step. Xcode sets EXPANDED_CODE_SIGN_IDENTITY in the
            # environment to the resolved signing identity for the target.
            set(_CODESIGN_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/codesign_frameworks.sh")
            file(WRITE "${_CODESIGN_SCRIPT}"
"#!/bin/sh
FRAMEWORKS_DIR=\"$1/Frameworks\"
if [ -d \"$FRAMEWORKS_DIR\" ]; then
  IDENTITY=\"$EXPANDED_CODE_SIGN_IDENTITY\"
  if [ -z \"$IDENTITY\" ]; then IDENTITY=\"-\"; fi
  echo \"Code-signing embedded frameworks with identity: $IDENTITY\"
  for FW in \"$FRAMEWORKS_DIR\"/*.framework; do
    if [ -d \"$FW\" ]; then
      codesign --force --sign \"$IDENTITY\" --timestamp=none \"$FW\"
    fi
  done
fi
")
            file(CHMOD "${_CODESIGN_SCRIPT}" PERMISSIONS
                OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
            )
            add_custom_command(TARGET snap_rhi_demo_app POST_BUILD
                COMMAND "${_CODESIGN_SCRIPT}" "$<TARGET_BUNDLE_DIR:snap_rhi_demo_app>"
                COMMENT "Code-signing embedded Vulkan frameworks in app bundle"
            )
        endif()
    else()
        # macOS-specific frameworks and settings
        target_link_libraries(snap_rhi_demo_app PRIVATE
            "-framework AppKit"
            "-framework Carbon"
            "-framework ForceFeedback"
            "-framework IOKit"
        )

        set(MACOS_PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platforms/macos")
        if(EXISTS "${MACOS_PLATFORM_DIR}/Info.plist")
            set_target_properties(snap_rhi_demo_app PROPERTIES
                MACOSX_BUNDLE_INFO_PLIST "${MACOS_PLATFORM_DIR}/Info.plist"
            )
        endif()
    endif()

    # Set ARC for Objective-C++ files
    file(GLOB_RECURSE DEMO_APP_OBJCXX_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Metal/*.mm
    )
    if(DEMO_APP_OBJCXX_SOURCES)
        set_source_files_properties(${DEMO_APP_OBJCXX_SOURCES} PROPERTIES
            COMPILE_FLAGS "-fobjc-arc"
        )
    endif()

# ============================================================================
# Desktop (non-Apple, non-Android) Configuration
# ============================================================================
elseif(NOT ANDROID)
    # Copy assets next to executable
    add_custom_command(TARGET snap_rhi_demo_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:snap_rhi_demo_app>/assets"
        COMMENT "Copying assets to output directory"
    )

    # IDE working directory settings
    set_target_properties(snap_rhi_demo_app PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:snap_rhi_demo_app>"
        XCODE_SCHEME_WORKING_DIRECTORY "$<TARGET_FILE_DIR:snap_rhi_demo_app>"
    )
endif()
