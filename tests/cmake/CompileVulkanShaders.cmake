# CompileVulkanShaders.cmake
#
# Compiles Vulkan GLSL shaders to SPIR-V at build time using glslc,
# then generates a C++ header with embedded byte arrays.
#
# When glslc is not available, falls back to the pre-generated header
# checked into the repository (FALLBACK_HEADER).  Vulkan tests are
# only disabled when neither glslc nor the fallback header exist.
#
# Usage:
#   include(CompileVulkanShaders)
#   compile_vulkan_shaders(
#       TARGET          snap-rhi-tests
#       SHADER_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan
#       OUTPUT_HEADER   ${CMAKE_CURRENT_BINARY_DIR}/generated/VulkanSPIRV.h
#       FALLBACK_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/generated/VulkanSPIRV.h
#   )

find_program(GLSLC_EXECUTABLE glslc)

function(compile_vulkan_shaders)
    cmake_parse_arguments(ARG "" "TARGET;SHADER_DIR;OUTPUT_HEADER;FALLBACK_HEADER" "" ${ARGN})

    # ------------------------------------------------------------------
    # No glslc → try to use the pre-generated fallback header
    # ------------------------------------------------------------------
    if(NOT GLSLC_EXECUTABLE)
        if(ARG_FALLBACK_HEADER AND EXISTS "${ARG_FALLBACK_HEADER}")
            message(STATUS "glslc not found — using pre-generated Vulkan SPIR-V header: ${ARG_FALLBACK_HEADER}")
            get_filename_component(_FALLBACK_DIR "${ARG_FALLBACK_HEADER}" DIRECTORY)
            target_include_directories(${ARG_TARGET} PRIVATE "${_FALLBACK_DIR}")
            target_compile_definitions(${ARG_TARGET} PRIVATE SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE=1)
        else()
            message(WARNING "glslc not found and no pre-generated SPIR-V header available — Vulkan shader tests will be disabled.")
            target_compile_definitions(${ARG_TARGET} PRIVATE SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE=0)
        endif()
        return()
    endif()

    message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")

    # ------------------------------------------------------------------
    # Compile GLSL → SPIR-V at build time
    # ------------------------------------------------------------------
    file(GLOB SHADER_FILES
        "${ARG_SHADER_DIR}/*.vert"
        "${ARG_SHADER_DIR}/*.frag"
        "${ARG_SHADER_DIR}/*.comp"
    )

    set(SPV_FILES)
    set(SPV_DIR "${CMAKE_CURRENT_BINARY_DIR}/spirv")
    file(MAKE_DIRECTORY "${SPV_DIR}")

    foreach(SHADER_FILE ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
        set(SPV_FILE "${SPV_DIR}/${SHADER_NAME}.spv")

        add_custom_command(
            OUTPUT "${SPV_FILE}"
            COMMAND ${GLSLC_EXECUTABLE}
                    --target-env=vulkan1.0
                    -o "${SPV_FILE}"
                    "${SHADER_FILE}"
            DEPENDS "${SHADER_FILE}"
            COMMENT "Compiling Vulkan shader: ${SHADER_NAME}"
            VERBATIM
        )

        list(APPEND SPV_FILES "${SPV_FILE}")
    endforeach()

    add_custom_target(compile_vulkan_test_shaders DEPENDS ${SPV_FILES})

    # ------------------------------------------------------------------
    # Generate C++ header with embedded SPIR-V byte arrays
    # ------------------------------------------------------------------
    set(GENERATE_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/generate_spirv_header.cmake")
    file(WRITE "${GENERATE_SCRIPT}" "
set(HEADER_CONTENT \"//\\n// VulkanSPIRV.h — Auto-generated at build time. Do not edit.\\n//\\n\\n#pragma once\\n\\n#include <cstdint>\\n#include <span>\\n\\n#define SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE 1\\n\\nnamespace test_shaders { namespace spirv {\\n\\n\")

file(GLOB SPV_FILES_GEN \"${SPV_DIR}/*.spv\")
foreach(SPV_FILE \${SPV_FILES_GEN})
    get_filename_component(SPV_NAME \${SPV_FILE} NAME)
    string(REGEX REPLACE \"\\\\.spv$\" \"\" SPV_NAME \${SPV_NAME})
    string(REPLACE \".\" \"_\" VAR_NAME \${SPV_NAME})

    file(READ \${SPV_FILE} SPV_DATA HEX)
    string(LENGTH \"\${SPV_DATA}\" SPV_HEX_LEN)
    math(EXPR SPV_BYTE_COUNT \"\${SPV_HEX_LEN} / 2\")

    set(BYTE_ARRAY \"\")
    set(LINE_COUNT 0)
    math(EXPR LAST_INDEX \"\${SPV_HEX_LEN} - 1\")
    foreach(i RANGE 0 \${LAST_INDEX} 2)
        string(SUBSTRING \"\${SPV_DATA}\" \${i} 2 BYTE)
        if(LINE_COUNT GREATER 0)
            string(APPEND BYTE_ARRAY \",\")
        endif()
        if(LINE_COUNT EQUAL 16)
            string(APPEND BYTE_ARRAY \"\\n    \")
            set(LINE_COUNT 0)
        endif()
        string(APPEND BYTE_ARRAY \"0x\${BYTE}\")
        math(EXPR LINE_COUNT \"\${LINE_COUNT} + 1\")
    endforeach()

    string(APPEND HEADER_CONTENT \"constexpr uint8_t k_\${VAR_NAME}[] = {\\n    \${BYTE_ARRAY}\\n};\\n\\n\")
    string(APPEND HEADER_CONTENT \"constexpr std::span<const uint8_t> \${VAR_NAME}(k_\${VAR_NAME}, sizeof(k_\${VAR_NAME}));\\n\\n\")
endforeach()

string(APPEND HEADER_CONTENT \"}} // namespace test_shaders::spirv\\n\")
file(WRITE \"${ARG_OUTPUT_HEADER}\" \"\${HEADER_CONTENT}\")
")

    get_filename_component(HEADER_DIR "${ARG_OUTPUT_HEADER}" DIRECTORY)
    file(MAKE_DIRECTORY "${HEADER_DIR}")

    add_custom_command(
        OUTPUT "${ARG_OUTPUT_HEADER}"
        COMMAND ${CMAKE_COMMAND} -P "${GENERATE_SCRIPT}"
        DEPENDS ${SPV_FILES}
        COMMENT "Generating VulkanSPIRV.h from compiled SPIR-V binaries"
        VERBATIM
    )

    add_custom_target(generate_vulkan_spirv_header DEPENDS "${ARG_OUTPUT_HEADER}")
    add_dependencies(generate_vulkan_spirv_header compile_vulkan_test_shaders)
    add_dependencies(${ARG_TARGET} generate_vulkan_spirv_header)

    target_include_directories(${ARG_TARGET} PRIVATE "${HEADER_DIR}")
    target_compile_definitions(${ARG_TARGET} PRIVATE SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE=1)

endfunction()
