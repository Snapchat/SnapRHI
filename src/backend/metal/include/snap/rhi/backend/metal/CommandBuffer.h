//
//  CommandBuffer.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 07.10.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/metal/BlitCommandEncoder.h"
#include "snap/rhi/backend/metal/ComputeCommandEncoder.h"
#include "snap/rhi/backend/metal/Context.h"
#include "snap/rhi/backend/metal/RenderCommandEncoder.h"

#include <Metal/Metal.h>
#include <vector>

#include <string_view>

namespace snap::rhi::backend::metal {
class Device;
class CommandQueue;
class DepsHolder;

class CommandBuffer final : public snap::rhi::backend::common::CommandBuffer {
    friend class snap::rhi::backend::metal::BlitCommandEncoder;
    friend class snap::rhi::backend::metal::RenderCommandEncoder;
    friend class snap::rhi::backend::metal::ComputeCommandEncoder;

public:
    CommandBuffer(Device* mtlDevice, const snap::rhi::CommandBufferCreateInfo& info);
    ~CommandBuffer() override;

    void resetQueryPool(snap::rhi::QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount) override;

    snap::rhi::RenderCommandEncoder* getRenderCommandEncoder() override;
    snap::rhi::BlitCommandEncoder* getBlitCommandEncoder() override;
    snap::rhi::ComputeCommandEncoder* getComputeCommandEncoder() override;

    Context& getContext();

    void setDebugLabel(std::string_view label) override;

private:
    Context context;
    BlitCommandEncoder blitEncoder;
    RenderCommandEncoder renderEncoder;
    ComputeCommandEncoder computeEncoder;
};
} // namespace snap::rhi::backend::metal
