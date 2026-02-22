//
//  ShaderLibrary.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <optional>

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/ShaderLibraryCreateInfo.h"
#include "snap/rhi/reflection/ShaderLibraryInfo.h"

namespace snap::rhi {
class Device;

/**
 * @brief A collection of shader entry points compiled as a single library.
 *
 * A ShaderLibrary represents a backend-specific shader module/library container that can provide multiple entry points
 * (vertex/fragment/compute, etc.) depending on the backend.
 *
 * Creation:
 * - Created via `snap::rhi::Device::createShaderLibrary()`.
 * - Backends differ in what `ShaderLibraryCreateFlag` values they support:
 *   - Vulkan typically supports only `CompileFromBinary` (SPIR-V).
 *   - OpenGL typically supports only `CompileFromSource` (GLSL source).
 *   - Metal may support both source and binary libraries.
 *
 * Reflection:
 * - Some backends populate entry point reflection (names, stages, specialization constants).
 * - If reflection is not available, `getReflectionInfo()` returns an empty optional.
 */
class ShaderLibrary : public snap::rhi::DeviceChild {
public:
    ShaderLibrary(Device* device, const ShaderLibraryCreateInfo& info);
    ~ShaderLibrary() override = default;

    /**
     * @brief Returns optional reflection metadata for this library.
     */
    const std::optional<reflection::ShaderLibraryInfo>& getReflectionInfo() const;

    /**
     * @brief Returns an estimate of CPU-side memory used by this object.
     */
    [[nodiscard]] uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    /**
     * @brief Returns an estimate of GPU-side memory used by this object.
     */
    [[nodiscard]] uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    std::optional<reflection::ShaderLibraryInfo> shaderLibraryInfo = std::nullopt;

private:
};
} // namespace snap::rhi
