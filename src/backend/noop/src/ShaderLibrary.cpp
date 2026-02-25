#include "snap/rhi/backend/noop/ShaderLibrary.hpp"

#include "snap/rhi/backend/noop/Device.hpp"

namespace snap::rhi::backend::noop {
ShaderLibrary::ShaderLibrary(snap::rhi::backend::common::Device* device, const snap::rhi::ShaderLibraryCreateInfo& info)
    : snap::rhi::ShaderLibrary(device, info) {}
} // namespace snap::rhi::backend::noop
