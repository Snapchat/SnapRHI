//
//  DescriptorPool.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 9/1/22.
//

#pragma once

#include "snap/rhi/DescriptorPool.hpp"

#include <stack>
#include <unordered_set>
#include <vector>

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {
class DescriptorPool final : public snap::rhi::DescriptorPool {
public:
    DescriptorPool(snap::rhi::backend::common::Device* device, const DescriptorPoolCreateInfo& info);
    ~DescriptorPool() override = default;

private:
};
} // namespace snap::rhi::backend::noop
