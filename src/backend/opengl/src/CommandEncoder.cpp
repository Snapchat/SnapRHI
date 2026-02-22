#include "snap/rhi/backend/opengl/CommandEncoder.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include <algorithm>

namespace snap::rhi::backend::opengl {
template<typename CommandEncoderBase>
CommandEncoder<CommandEncoderBase>::CommandEncoder(snap::rhi::backend::opengl::Device* device,
                                                   snap::rhi::backend::opengl::CommandBuffer* commandBuffer)
    : snap::rhi::backend::common::CommandEncoder<CommandEncoderBase>(device, commandBuffer),
      commandAllocator(commandBuffer->getCommandAllocator()) {}

template<typename CommandEncoderBase>
void CommandEncoder<CommandEncoderBase>::writeTimestamp(snap::rhi::QueryPool* queryPool,
                                                        uint32_t query,
                                                        const snap::rhi::TimestampLocation location) {
    // TODO: Implement OpenGL timestamp queries
}

template<typename CommandEncoderBase>
void CommandEncoder<CommandEncoderBase>::pipelineBarrier(
    snap::rhi::PipelineStageBits srcStageMask,
    snap::rhi::PipelineStageBits dstStageMask,
    snap::rhi::DependencyFlags dependencyFlags,
    std::span<snap::rhi::MemoryBarrierInfo> memoryBarriers,
    std::span<snap::rhi::BufferMemoryBarrierInfo> bufferMemoryBarriers,
    std::span<snap::rhi::TextureMemoryBarrierInfo> textureMemoryBarriers) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(CommandEncoderBase::device, "[CommandEncoder][pipelineBarrier]");

    PipelineBarrierCmd* cmd = commandAllocator.allocateCommand<PipelineBarrierCmd>();
    cmd->srcStageMask = srcStageMask;
    cmd->dstStageMask = dstStageMask;
    cmd->dependencyFlags = dependencyFlags;

    for (uint32_t i = 0; i < memoryBarriers.size(); ++i) {
        cmd->memoryBarriers[i] = memoryBarriers[i];
    }
    cmd->memoryBarriersCount = memoryBarriers.size();

    for (uint32_t i = 0; i < bufferMemoryBarriers.size(); ++i) {
        assert(bufferMemoryBarriers[i].buffer);
        cmd->bufferMemoryBarriers[i] = bufferMemoryBarriers[i];

        CommandEncoder::resourceResidencySet.track(bufferMemoryBarriers[i].buffer);
    }
    cmd->bufferMemoryBarriersCount = bufferMemoryBarriers.size();

    for (uint32_t i = 0; i < textureMemoryBarriers.size(); ++i) {
        assert(textureMemoryBarriers[i].texture);
        cmd->textureMemoryBarriers[i] = textureMemoryBarriers[i];

        if (auto* cmdBufferBase = snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::CommandBuffer>(
                CommandEncoderBase::commandBuffer)) {
            cmdBufferBase->tryPreserveInteropTexture(textureMemoryBarriers[i].texture);
        }

        CommandEncoder::resourceResidencySet.track(textureMemoryBarriers[i].texture);
    }
    cmd->textureMemoryBarriersCount = textureMemoryBarriers.size();
    {
        SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing PipelineBarrierCmd******");
        std::string srcStageMaskString = getPipelineStageBitsStr(cmd->srcStageMask);
        std::string dstStageMaskString = getPipelineStageBitsStr(cmd->dstStageMask);

        SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tsrcStageMask: " + srcStageMaskString);
        SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tdstStageMask: " + dstStageMaskString);
        // DependencyFlags enum has only one possible value.
        SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tdependencyFlags: DependencyByRegion");
        SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tbufferMemoryBarriers: ");
        for (uint32_t idx = 0; idx < cmd->bufferMemoryBarriersCount; ++idx) {
            auto& element = cmd->bufferMemoryBarriers[idx];
            SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\tBufferMemoryBarrierInfo element " + std::to_string(idx));
            SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tsrcAccessMask: " + getAccessFlagsToStr(element.srcAccessMask));
            SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tdstAccessMask: " + getAccessFlagsToStr(element.dstAccessMask));
            SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\toffset: " + std::to_string(element.offset));
            SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tsize: " + std::to_string(element.size));
        }

        SNAP_RHI_REPORT(CommandEncoder::validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "\tbuffers: ");
    }
}

template<typename CommandEncoderBase>
void CommandEncoder<CommandEncoderBase>::beginDebugGroup(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    BeginDebugGroupCmd* cmd = commandAllocator.allocateCommand<BeginDebugGroupCmd>();
    const auto copySize = std::min(label.size(), cmd->labelBuffer.size());
    std::ranges::copy(label.substr(0, copySize), cmd->labelBuffer.begin());
    cmd->labelSize = static_cast<uint32_t>(copySize);
#endif
}

template<typename CommandEncoderBase>
void CommandEncoder<CommandEncoderBase>::endDebugGroup() {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    commandAllocator.allocateCommand<EndDebugGroupCmd>();
#endif
}

template class CommandEncoder<snap::rhi::BlitCommandEncoder>;
template class CommandEncoder<snap::rhi::ComputeCommandEncoder>;
template class CommandEncoder<snap::rhi::RenderCommandEncoder>;
} // namespace snap::rhi::backend::opengl
