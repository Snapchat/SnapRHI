#pragma once

#include "snap/rhi/DescriptorSetLayout.hpp"

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class DescriptorSetLayout final : public snap::rhi::DescriptorSetLayout {
public:
    DescriptorSetLayout(snap::rhi::backend::common::Device* device, const DescriptorSetLayoutCreateInfo& info);
    ~DescriptorSetLayout() override = default;
};
} // namespace snap::rhi::backend::noop
