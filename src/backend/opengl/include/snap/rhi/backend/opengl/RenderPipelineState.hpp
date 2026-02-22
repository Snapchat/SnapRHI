// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include "snap/rhi/Compare.hpp"
#include "snap/rhi/RenderPipelineCreateInfo.h"
#include <mutex>
#include <unordered_map>

namespace snap::rhi::backend::opengl {
using PipelineStateUUID = uint64_t;

struct RenderPipelineStatesUUID {
    std::array<PipelineStateUUID, MaxColorAttachments> colorAttachmentsBlendStates{};
    PipelineStateUUID depthStencilState = 0;
    PipelineStateUUID rasterizationState = 0;
    PipelineStateUUID multisampleState = 0;
    PipelineStateUUID inputAssemblyState = 0;

    RenderPipelineStatesUUID() {
        colorAttachmentsBlendStates.fill(0);
    }
};

struct RenderPipelineStates {
private:
    template<typename T>
    struct RenderPipelineStateUUID {
    private:
        std::unordered_map<T, PipelineStateUUID> stateUUIDs;
        PipelineStateUUID stateUUIDGen = 0;

    public:
        PipelineStateUUID getUUID(const T& s) {
            const auto& itr = stateUUIDs.find(s);
            if (itr != stateUUIDs.end()) {
                return itr->second;
            }

            stateUUIDs[s] = ++stateUUIDGen;
            return stateUUIDGen;
        }
    };

public:
    RenderPipelineStatesUUID getRenderPipelineState(const snap::rhi::RenderPipelineCreateInfo& info);

private:
    std::mutex accessMutex;

    RenderPipelineStateUUID<snap::rhi::RenderPipelineColorBlendAttachmentState> colorAttachmentsBlendState;
    RenderPipelineStateUUID<snap::rhi::DepthStencilStateCreateInfo> depthStencilState;
    RenderPipelineStateUUID<snap::rhi::MultisampleStateCreateInfo> multisampleState;
    RenderPipelineStateUUID<snap::rhi::RasterizationStateCreateInfo> rasterizationState;
    RenderPipelineStateUUID<snap::rhi::InputAssemblyStateCreateInfo> inputAssemblyState;
};
} // namespace snap::rhi::backend::opengl
