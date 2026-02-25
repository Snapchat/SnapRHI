//
//  ShaderModule.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 21.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <cstdint>
#include <optional>

#include "snap/rhi/DeviceChild.hpp"
#include "snap/rhi/ShaderModuleCreateInfo.h"
#include "snap/rhi/reflection/ShaderModuleInfo.h"

namespace snap::rhi {
class Device;

/**
 * @brief A single shader entry point extracted from a `snap::rhi::ShaderLibrary`.
 *
 * A ShaderModule represents one entry point (for example a vertex or fragment function) together with its
 * specialization constants and optional reflection metadata.
 *
 * Shader modules are typically created via `snap::rhi::Device::createShaderModule()` and then referenced by pipeline
 * create info structures.
 *
 * Reflection:
 * - When supported by the backend, `getReflectionInfo()` returns metadata such as vertex attribute information.
 * - If reflection is unavailable, it returns an empty optional.
 *
 * Backend notes:
 * - Vulkan uses the entry point name and specialization constants to set up pipeline stage creation.
 * - OpenGL builds the final shader source by combining the library source, specialization defines, and the requested
 *   entry point.
 * - Metal may create the underlying function asynchronously; `getReflectionInfo()` may implicitly wait for completion.
 */
class ShaderModule : public snap::rhi::DeviceChild {
public:
    ShaderModule(Device* device, const snap::rhi::ShaderModuleCreateInfo& info);
    ~ShaderModule() override = default;

    /**
     * @brief Returns the shader stage of this module.
     */
    [[nodiscard]] ShaderStage getShaderStage() const;

    [[nodiscard]] const snap::rhi::ShaderModuleCreateInfo& getCreateInfo() const;

    /**
     * @brief Returns optional reflection metadata for this module.
     */
    [[nodiscard]] virtual const std::optional<reflection::ShaderModuleInfo>& getReflectionInfo() const;

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
    std::optional<snap::rhi::reflection::ShaderModuleInfo> shaderReflection = std::nullopt;
    snap::rhi::ShaderModuleCreateInfo info{};

private:
};
} // namespace snap::rhi
