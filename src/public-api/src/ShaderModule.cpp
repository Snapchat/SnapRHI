#include "snap/rhi/ShaderModule.hpp"

namespace snap::rhi {
ShaderModule::ShaderModule(Device* device, const snap::rhi::ShaderModuleCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::ShaderModule), info(info) {}

ShaderStage ShaderModule::getShaderStage() const {
    return info.shaderStage;
}

const std::optional<reflection::ShaderModuleInfo>& ShaderModule::getReflectionInfo() const {
    return shaderReflection;
}

const snap::rhi::ShaderModuleCreateInfo& ShaderModule::getCreateInfo() const {
    return info;
}
} // namespace snap::rhi
