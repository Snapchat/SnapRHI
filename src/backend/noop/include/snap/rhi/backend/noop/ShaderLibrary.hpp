#pragma once

#include "snap/rhi/ShaderLibrary.hpp"

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class ShaderLibrary final : public snap::rhi::ShaderLibrary {
public:
    ShaderLibrary(snap::rhi::backend::common::Device* device, const snap::rhi::ShaderLibraryCreateInfo& info);
    ~ShaderLibrary() override = default;
};
} // namespace snap::rhi::backend::noop
