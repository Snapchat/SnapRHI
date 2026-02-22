#pragma once

#include "snap/rhi/ShaderModule.hpp"
#include "snap/rhi/common/NonCopyable.h"

#include <string>

namespace snap::rhi {
class ShaderLibrary;
} // namespace snap::rhi

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class ShaderModule final : public snap::rhi::ShaderModule {
public:
    explicit ShaderModule(snap::rhi::backend::common::Device* device, const snap::rhi::ShaderModuleCreateInfo& info);
    ~ShaderModule() override = default;

private:
    std::string name;
};
} // namespace snap::rhi::backend::noop
