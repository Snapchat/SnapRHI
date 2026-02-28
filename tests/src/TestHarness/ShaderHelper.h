//
//  ShaderHelper.h
//  unitest-catch2
//
//  Helper to create shader libraries and modules for each backend from embedded sources.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#pragma once

#include "ShaderSources.h"
#include "snap/rhi/Device.hpp"
#include "snap/rhi/ShaderLibrary.hpp"
#include "snap/rhi/ShaderModule.hpp"
#include "snap/rhi/ShaderLibraryCreateInfo.h"
#include "snap/rhi/ShaderModuleCreateInfo.h"
#include "snap/rhi/opengl/PipelineInfo.h"
#include "snap/rhi/opengl/RenderPipelineInfo.h"
#include "snap/rhi/metal/RenderPipelineInfo.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#if SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE
#include "VulkanSPIRV.h"
#elif !defined(SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE)
#define SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE 0
#endif

namespace test_harness {

/**
 * @brief Holds a shader library and its VS/FS modules for a render pipeline.
 */
struct RenderShaderSet {
    std::shared_ptr<snap::rhi::ShaderLibrary> library;
    std::shared_ptr<snap::rhi::ShaderModule> vertexShader;
    std::shared_ptr<snap::rhi::ShaderModule> fragmentShader;

    /// OpenGL pipeline info (only relevant for GL backend)
    std::optional<snap::rhi::opengl::RenderPipelineInfo> glPipelineInfo;

    /// Metal pipeline info (only relevant for Metal backend)
    std::optional<snap::rhi::metal::RenderPipelineInfo> mtlRenderPipelineInfo;
};

/**
 * @brief Holds a shader library and compute module.
 */
struct ComputeShaderSet {
    std::shared_ptr<snap::rhi::ShaderLibrary> library;
    std::shared_ptr<snap::rhi::ShaderModule> computeShader;

    /// OpenGL pipeline info (only relevant for GL backend)
    std::optional<snap::rhi::opengl::PipelineInfo> glPipelineInfo;
};

/**
 * @brief Creates the passthrough render shader set for the given device's backend.
 */
inline std::optional<RenderShaderSet> createPassthroughShaders(snap::rhi::Device* device) {
    const auto& caps = device->getCapabilities();
    const auto& apiDesc = caps.apiDescription;

    RenderShaderSet result;

    if (apiDesc.isAnyOpenGL()) {
        // OpenGL path: compile from GLSL source
        auto src = test_shaders::kGLSLPassthrough;
        snap::rhi::ShaderLibraryCreateInfo libInfo{};
        libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromSource;
        libInfo.code = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(src.data()), src.size());
        result.library = device->createShaderLibrary(libInfo);
        if (!result.library) return std::nullopt;

        snap::rhi::ShaderModuleCreateInfo vsInfo{};
        vsInfo.shaderStage = snap::rhi::ShaderStage::Vertex;
        vsInfo.name = "test_passthrough_vs";
        vsInfo.shaderLibrary = result.library.get();
        result.vertexShader = device->createShaderModule(vsInfo);

        snap::rhi::ShaderModuleCreateInfo fsInfo{};
        fsInfo.shaderStage = snap::rhi::ShaderStage::Fragment;
        fsInfo.name = "test_passthrough_fs";
        fsInfo.shaderLibrary = result.library.get();
        result.fragmentShader = device->createShaderModule(fsInfo);

        // Build GL pipeline info
        snap::rhi::opengl::RenderPipelineInfo glInfo;
        glInfo.resources = {
            {.name = "TestUBO",
             .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
             .binding = {.descriptorSet = 0, .binding = 0}},
        };
        glInfo.vertexAttributes = {
            {.name = "aPos", .location = 0},
            {.name = "aColor", .location = 1},
        };
        result.glPipelineInfo = glInfo;

    } else if (apiDesc.isAnyMetal()) {
        // Metal path: compile from MSL source
        auto src = test_shaders::kMSLPassthrough;
        snap::rhi::ShaderLibraryCreateInfo libInfo{};
        libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromSource;
        libInfo.code = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(src.data()), src.size());
        result.library = device->createShaderLibrary(libInfo);
        if (!result.library) return std::nullopt;

        snap::rhi::ShaderModuleCreateInfo vsInfo{};
        vsInfo.shaderStage = snap::rhi::ShaderStage::Vertex;
        vsInfo.name = "test_passthrough_vs";
        vsInfo.shaderLibrary = result.library.get();
        result.vertexShader = device->createShaderModule(vsInfo);

        snap::rhi::ShaderModuleCreateInfo fsInfo{};
        fsInfo.shaderStage = snap::rhi::ShaderStage::Fragment;
        fsInfo.name = "test_passthrough_fs";
        fsInfo.shaderLibrary = result.library.get();
        result.fragmentShader = device->createShaderModule(fsInfo);

        // Metal pipeline info with default vertex buffer binding base
        result.mtlRenderPipelineInfo = snap::rhi::metal::RenderPipelineInfo{};

    } else if (apiDesc.isAnyVulkan()) {
#if SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE
        // Vulkan: load pre-compiled SPIR-V for vertex shader
        {
            snap::rhi::ShaderLibraryCreateInfo libInfo{};
            libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromBinary;
            libInfo.code = test_shaders::spirv::passthrough_vert;
            result.library = device->createShaderLibrary(libInfo);
            if (!result.library) return std::nullopt;

            snap::rhi::ShaderModuleCreateInfo vsInfo{};
            vsInfo.shaderStage = snap::rhi::ShaderStage::Vertex;
            vsInfo.name = "main";
            vsInfo.shaderLibrary = result.library.get();
            result.vertexShader = device->createShaderModule(vsInfo);
        }
        // Vulkan: load pre-compiled SPIR-V for fragment shader (separate library)
        {
            snap::rhi::ShaderLibraryCreateInfo libInfo{};
            libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromBinary;
            libInfo.code = test_shaders::spirv::passthrough_frag;
            auto fsLibrary = device->createShaderLibrary(libInfo);
            if (!fsLibrary) return std::nullopt;

            snap::rhi::ShaderModuleCreateInfo fsInfo{};
            fsInfo.shaderStage = snap::rhi::ShaderStage::Fragment;
            fsInfo.name = "main";
            fsInfo.shaderLibrary = fsLibrary.get();
            result.fragmentShader = device->createShaderModule(fsInfo);
        }
#else
        return std::nullopt;
#endif

    } else {
        return std::nullopt;
    }

    if (!result.vertexShader || !result.fragmentShader) return std::nullopt;
    return result;
}

/**
 * @brief Creates the fullscreen fill render shader set for the given device's backend.
 */
inline std::optional<RenderShaderSet> createFillShaders(snap::rhi::Device* device) {
    const auto& caps = device->getCapabilities();
    const auto& apiDesc = caps.apiDescription;

    RenderShaderSet result;

    if (apiDesc.isAnyOpenGL()) {
        auto src = test_shaders::kGLSLFill;
        snap::rhi::ShaderLibraryCreateInfo libInfo{};
        libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromSource;
        libInfo.code = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(src.data()), src.size());
        result.library = device->createShaderLibrary(libInfo);
        if (!result.library) return std::nullopt;

        snap::rhi::ShaderModuleCreateInfo vsInfo{};
        vsInfo.shaderStage = snap::rhi::ShaderStage::Vertex;
        vsInfo.name = "test_fill_vs";
        vsInfo.shaderLibrary = result.library.get();
        result.vertexShader = device->createShaderModule(vsInfo);

        snap::rhi::ShaderModuleCreateInfo fsInfo{};
        fsInfo.shaderStage = snap::rhi::ShaderStage::Fragment;
        fsInfo.name = "test_fill_fs";
        fsInfo.shaderLibrary = result.library.get();
        result.fragmentShader = device->createShaderModule(fsInfo);

        snap::rhi::opengl::RenderPipelineInfo glInfo;
        glInfo.resources = {
            {.name = "FillUBO",
             .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
             .binding = {.descriptorSet = 0, .binding = 0}},
        };
        result.glPipelineInfo = glInfo;

    } else if (apiDesc.isAnyMetal()) {
        auto src = test_shaders::kMSLFill;
        snap::rhi::ShaderLibraryCreateInfo libInfo{};
        libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromSource;
        libInfo.code = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(src.data()), src.size());
        result.library = device->createShaderLibrary(libInfo);
        if (!result.library) return std::nullopt;

        snap::rhi::ShaderModuleCreateInfo vsInfo{};
        vsInfo.shaderStage = snap::rhi::ShaderStage::Vertex;
        vsInfo.name = "test_fill_vs";
        vsInfo.shaderLibrary = result.library.get();
        result.vertexShader = device->createShaderModule(vsInfo);

        snap::rhi::ShaderModuleCreateInfo fsInfo{};
        fsInfo.shaderStage = snap::rhi::ShaderStage::Fragment;
        fsInfo.name = "test_fill_fs";
        fsInfo.shaderLibrary = result.library.get();
        result.fragmentShader = device->createShaderModule(fsInfo);

        // Metal pipeline info with default vertex buffer binding base
        result.mtlRenderPipelineInfo = snap::rhi::metal::RenderPipelineInfo{};

    } else if (apiDesc.isAnyVulkan()) {
#if SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE
        {
            snap::rhi::ShaderLibraryCreateInfo libInfo{};
            libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromBinary;
            libInfo.code = test_shaders::spirv::fill_vert;
            result.library = device->createShaderLibrary(libInfo);
            if (!result.library) return std::nullopt;

            snap::rhi::ShaderModuleCreateInfo vsInfo{};
            vsInfo.shaderStage = snap::rhi::ShaderStage::Vertex;
            vsInfo.name = "main";
            vsInfo.shaderLibrary = result.library.get();
            result.vertexShader = device->createShaderModule(vsInfo);
        }
        {
            snap::rhi::ShaderLibraryCreateInfo libInfo{};
            libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromBinary;
            libInfo.code = test_shaders::spirv::fill_frag;
            auto fsLibrary = device->createShaderLibrary(libInfo);
            if (!fsLibrary) return std::nullopt;

            snap::rhi::ShaderModuleCreateInfo fsInfo{};
            fsInfo.shaderStage = snap::rhi::ShaderStage::Fragment;
            fsInfo.name = "main";
            fsInfo.shaderLibrary = fsLibrary.get();
            result.fragmentShader = device->createShaderModule(fsInfo);
        }
#else
        return std::nullopt;
#endif

    } else {
        return std::nullopt;
    }

    if (!result.vertexShader || !result.fragmentShader) return std::nullopt;
    return result;
}

/**
 * @brief Creates the compute shader set for the given device's backend.
 */
inline std::optional<ComputeShaderSet> createComputeShaders(snap::rhi::Device* device) {
    const auto& caps = device->getCapabilities();
    const auto& apiDesc = caps.apiDescription;

    ComputeShaderSet result;

    if (apiDesc.isAnyOpenGL()) {
        auto src = test_shaders::kGLSLCompute;
        snap::rhi::ShaderLibraryCreateInfo libInfo{};
        libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromSource;
        libInfo.code = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(src.data()), src.size());
        result.library = device->createShaderLibrary(libInfo);
        if (!result.library) return std::nullopt;

        snap::rhi::ShaderModuleCreateInfo csInfo{};
        csInfo.shaderStage = snap::rhi::ShaderStage::Compute;
        csInfo.name = "main";
        csInfo.shaderLibrary = result.library.get();
        result.computeShader = device->createShaderModule(csInfo);

        snap::rhi::opengl::PipelineInfo glInfo;
        glInfo.resources = {
            {.name = "OutputBuffer",
             .descriptorType = snap::rhi::DescriptorType::StorageBuffer,
             .binding = {.descriptorSet = 0, .binding = 0}},
        };
        result.glPipelineInfo = glInfo;

    } else if (apiDesc.isAnyMetal()) {
        auto src = test_shaders::kMSLCompute;
        snap::rhi::ShaderLibraryCreateInfo libInfo{};
        libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromSource;
        libInfo.code = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(src.data()), src.size());
        result.library = device->createShaderLibrary(libInfo);
        if (!result.library) return std::nullopt;

        snap::rhi::ShaderModuleCreateInfo csInfo{};
        csInfo.shaderStage = snap::rhi::ShaderStage::Compute;
        csInfo.name = "test_compute_fill";
        csInfo.shaderLibrary = result.library.get();
        result.computeShader = device->createShaderModule(csInfo);

    } else if (apiDesc.isAnyVulkan()) {
#if SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE
        snap::rhi::ShaderLibraryCreateInfo libInfo{};
        libInfo.libCompileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromBinary;
        libInfo.code = test_shaders::spirv::compute_fill_comp;
        result.library = device->createShaderLibrary(libInfo);
        if (!result.library) return std::nullopt;

        snap::rhi::ShaderModuleCreateInfo csInfo{};
        csInfo.shaderStage = snap::rhi::ShaderStage::Compute;
        csInfo.name = "main";
        csInfo.shaderLibrary = result.library.get();
        result.computeShader = device->createShaderModule(csInfo);
#else
        return std::nullopt;
#endif

    } else {
        return std::nullopt;
    }

    if (!result.computeShader) return std::nullopt;
    return result;
}

} // namespace test_harness
