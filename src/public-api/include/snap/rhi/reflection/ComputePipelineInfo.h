#pragma once

#include "snap/rhi/Structs.h"
#include "snap/rhi/reflection/Info.hpp"
#include <vector>

namespace snap::rhi::reflection {
struct ComputePipelineInfo {
    std::vector<DescriptorSetInfo> descriptorSetInfos;
};
} // namespace snap::rhi::reflection
