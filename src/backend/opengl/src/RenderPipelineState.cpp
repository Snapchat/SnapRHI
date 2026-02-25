#include "snap/rhi/backend/opengl/RenderPipelineState.hpp"

namespace snap::rhi::backend::opengl {
RenderPipelineStatesUUID RenderPipelineStates::getRenderPipelineState(const snap::rhi::RenderPipelineCreateInfo& info) {
    std::lock_guard<std::mutex> guard(accessMutex);
    RenderPipelineStatesUUID result{};

    for (uint32_t i = 0; i < info.colorBlendState.colorAttachmentsCount; ++i) {
        result.colorAttachmentsBlendStates[i] =
            colorAttachmentsBlendState.getUUID(info.colorBlendState.colorAttachmentsBlendState[i]);
    }
    result.depthStencilState = depthStencilState.getUUID(info.depthStencilState);
    result.multisampleState = multisampleState.getUUID(info.multisampleState);
    result.rasterizationState = rasterizationState.getUUID(info.rasterizationState);
    result.inputAssemblyState = inputAssemblyState.getUUID(info.inputAssemblyState);

    return result;
}
} // namespace snap::rhi::backend::opengl
