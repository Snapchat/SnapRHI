#pragma once

#include "snap/rhi/ShaderLibrary.hpp"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include "snap/rhi/backend/vulkan/vulkan.h"

namespace snap::rhi::backend::vulkan {
class Device;
class ShaderModule;

class ShaderLibrary final : public snap::rhi::ShaderLibrary {
public:
    ShaderLibrary(Device* vkDevice, const snap::rhi::ShaderLibraryCreateInfo& info);
    ~ShaderLibrary() final;

    void setDebugLabel(std::string_view label) override;

    VkShaderModule getShaderModule() const {
        return shaderModule;
    }

    const std::vector<EntryPointReflection>& getEntryPointReflections() const {
        return entryPointReflections;
    }

private:
    VkDevice device = VK_NULL_HANDLE;
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    std::vector<EntryPointReflection> entryPointReflections;
};
} // namespace snap::rhi::backend::vulkan
