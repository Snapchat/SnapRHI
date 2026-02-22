//
//  ShaderLibraryCreateInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <span>

#include "snap/rhi/Enums.h"
#include "snap/rhi/ShaderModuleCreateInfo.h"
#include "snap/rhi/reflection/ShaderLibraryInfo.h"

namespace snap::rhi {

/**
 * @brief Parameters controlling `snap::rhi::ShaderLibrary` creation.
 *
 * A shader library contains one or more shader entry points compiled as a single backend library/module.
 *
 * Backend support notes:
 * - OpenGL backends typically support only `ShaderLibraryCreateFlag::CompileFromSource` (GLSL source).
 * - Vulkan backends typically support only `ShaderLibraryCreateFlag::CompileFromBinary` (SPIR-V binary).
 * - Metal backends may support both source and binary libraries.
 */
struct ShaderLibraryCreateInfo {
    /**
     * @brief Compilation mode for the shader library.
     */
    ShaderLibraryCreateFlag libCompileFlag = ShaderLibraryCreateFlag::CompileFromSource;

    /**
     * @brief Shader code payload.
     *
     * The interpretation of this byte span depends on @ref libCompileFlag and the active backend:
     * - Compile-from-source: UTF-8 text containing shader source.
     * - Compile-from-binary: backend-specific binary format.
     *
     * Vulkan (SPIR-V):
     * - `code.size()` must be a multiple of 4.
     * - `code.data()` must be aligned suitably for reading `uint32_t` words.
     */
    std::span<const uint8_t> code{};
};
} // namespace snap::rhi
