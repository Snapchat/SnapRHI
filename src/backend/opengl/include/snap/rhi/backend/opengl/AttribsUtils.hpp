#pragma once

#include "snap/rhi/RenderPipelineCreateInfo.h"
#include "snap/rhi/VertexAttributeFormat.h"
#include "snap/rhi/backend/opengl/PipelineNativeInfo.h"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/VertexDescriptor.h"
#include "snap/rhi/reflection/RenderPipelineInfo.h"
#include <optional>
#include <unordered_map>

namespace snap::rhi::backend::opengl {
VertexDescriptor buildVertexDescriptor(
    const PipelineNativeInfo& pipelineNativeInfo,
    const RenderPipelineConfigurationInfo& renderPipelineConfigInfo,
    std::vector<snap::rhi::reflection::VertexAttributeInfo>& vertexAttributesReflection,
    const snap::rhi::backend::common::ValidationLayer& validationLayer);
} // namespace snap::rhi::backend::opengl
