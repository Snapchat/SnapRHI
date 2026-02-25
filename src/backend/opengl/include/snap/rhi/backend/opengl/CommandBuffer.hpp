//
//  CommandBuffer.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 13.10.2021.
//

#pragma once

#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/ResourceResidencySet.h"
#include "snap/rhi/backend/opengl/CommandAllocator.hpp"

#include "snap/rhi/backend/opengl/BlitCommandEncoder.hpp"
#include "snap/rhi/backend/opengl/ComputeCommandEncoder.hpp"
#include "snap/rhi/backend/opengl/RenderCommandEncoder.hpp"

#include "snap/rhi/CommandBuffer.hpp"

#include <string_view>

namespace snap::rhi::backend::opengl {
class Device;

class CommandBuffer final : public snap::rhi::backend::common::CommandBuffer {
public:
    CommandBuffer(snap::rhi::backend::opengl::Device* device, const snap::rhi::CommandBufferCreateInfo& info);
    ~CommandBuffer() override = default;

    void resetQueryPool(snap::rhi::QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount) override;

    snap::rhi::RenderCommandEncoder* getRenderCommandEncoder() override;
    snap::rhi::BlitCommandEncoder* getBlitCommandEncoder() override;
    snap::rhi::ComputeCommandEncoder* getComputeCommandEncoder() override;

    /**
     * This method is used to finish recording commands.
     */
    void finishRecording();

    /**
     * Resets the states, so that the CommandBuffer can be reused.
     */
    void reset(const snap::rhi::CommandBufferCreateInfo& info);

    /**
     * We use the Command Allocator to iterate over all the commands in the Command Recorder..
     */
    snap::rhi::backend::opengl::CommandAllocator& getCommandAllocator();

private:
    /**
     * Please don't change the order of the fields, because encoders can use holder/commandAllocator.
     */

    snap::rhi::backend::opengl::CommandAllocator commandAllocator;
    snap::rhi::backend::opengl::BlitCommandEncoder blitCommandEncoder;
    snap::rhi::backend::opengl::RenderCommandEncoder renderCommandEncoder;
    snap::rhi::backend::opengl::ComputeCommandEncoder computeCommandEncoder;

    std::string label = "";
};
} // namespace snap::rhi::backend::opengl
