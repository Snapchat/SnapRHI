#pragma once

#include "snap/rhi/Structs.h"
#include "snap/rhi/reflection/Info.hpp"

#include <unordered_map>
#include <vector>

namespace snap::rhi::reflection {
struct RenderPipelineInfo {
    std::vector<DescriptorSetInfo> descriptorSetInfos;
    std::vector<VertexAttributeInfo> vertexAttributes;

    constexpr friend bool operator==(const RenderPipelineInfo&, const RenderPipelineInfo&) noexcept = default;
};
} // namespace snap::rhi::reflection
