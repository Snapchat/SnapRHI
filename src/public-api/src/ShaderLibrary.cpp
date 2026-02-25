#include "snap/rhi/ShaderLibrary.hpp"

namespace snap::rhi {
ShaderLibrary::ShaderLibrary(Device* device, const ShaderLibraryCreateInfo& info)
    : snap::rhi::DeviceChild(device, snap::rhi::ResourceType::ShaderLibrary) {}

const std::optional<reflection::ShaderLibraryInfo>& ShaderLibrary::getReflectionInfo() const {
    return shaderLibraryInfo;
}
} // namespace snap::rhi
