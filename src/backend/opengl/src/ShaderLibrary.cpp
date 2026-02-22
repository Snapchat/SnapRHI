#include "snap/rhi/backend/opengl/ShaderLibrary.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"

namespace snap::rhi::backend::opengl {
ShaderLibrary::ShaderLibrary(Device* glesDevice, const snap::rhi::ShaderLibraryCreateInfo& info)
    : snap::rhi::ShaderLibrary(glesDevice, info) {
    const auto& validationLayer = glesDevice->getValidationLayer();
    SNAP_RHI_VALIDATE(validationLayer,
                      info.libCompileFlag == ShaderLibraryCreateFlag::CompileFromSource,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[ShaderLibrary] wrong libCompileFlag{%d}, only CompileFromSource supported for OpenGL",
                      info.libCompileFlag);

    src = std::string(reinterpret_cast<const char*>(info.code.data()), info.code.size());
}

const std::string& ShaderLibrary::getSource() const {
    return src;
}
} // namespace snap::rhi::backend::opengl
