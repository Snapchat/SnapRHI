#include "snap/rhi/backend/noop/ShaderModule.hpp"

#include "snap/rhi/backend/noop/Device.hpp"
#include "snap/rhi/backend/noop/ShaderLibrary.hpp"

namespace snap::rhi::backend::noop {
ShaderModule::ShaderModule(snap::rhi::backend::common::Device* device, const snap::rhi::ShaderModuleCreateInfo& info)
    : snap::rhi::ShaderModule(device, info) {
    name = std::string(info.name);
}
} // namespace snap::rhi::backend::noop
