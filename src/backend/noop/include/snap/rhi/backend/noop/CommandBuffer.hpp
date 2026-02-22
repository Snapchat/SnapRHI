#pragma once

#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/noop/BlitCommandEncoder.hpp"
#include "snap/rhi/backend/noop/ComputeCommandEncoder.hpp"
#include "snap/rhi/backend/noop/RenderCommandEncoder.hpp"

#include "snap/rhi/CommandBuffer.hpp"

#include <string_view>

namespace snap::rhi::backend::common {
class Device;
} // namespace snap::rhi::backend::common

namespace snap::rhi::backend::noop {

class CommandBuffer final : public snap::rhi::backend::common::CommandBuffer {
    friend class snap::rhi::backend::noop::BlitCommandEncoder;
    friend class snap::rhi::backend::noop::RenderCommandEncoder;
    friend class snap::rhi::backend::noop::ComputeCommandEncoder;

public:
    CommandBuffer(snap::rhi::backend::common::Device* device, const snap::rhi::CommandBufferCreateInfo& info);
    ~CommandBuffer() override = default;

    void resetQueryPool(snap::rhi::QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount) override;

    snap::rhi::RenderCommandEncoder* getRenderCommandEncoder() override;
    snap::rhi::BlitCommandEncoder* getBlitCommandEncoder() override;
    snap::rhi::ComputeCommandEncoder* getComputeCommandEncoder() override;

private:
    snap::rhi::backend::noop::BlitCommandEncoder blitCommandEncoder;
    snap::rhi::backend::noop::RenderCommandEncoder renderCommandEncoder;
    snap::rhi::backend::noop::ComputeCommandEncoder computeCommandEncoder;
};
} // namespace snap::rhi::backend::noop
