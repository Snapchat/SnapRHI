#pragma once

#include "snap/rhi/Common.h"
#include "snap/rhi/backend/common/CommandEncoder.h"
#include "snap/rhi/backend/metal/Context.h"
#include <Metal/Metal.h>
#include <span>

namespace snap::rhi::backend::metal {
class Device;
class CommandBuffer;

void writeTimestamp(snap::rhi::CommandBuffer* commandBuffer,
                    const id<MTLCommandEncoder>& encoder,
                    snap::rhi::QueryPool* queryPool,
                    uint32_t query,
                    const snap::rhi::TimestampLocation location);

template<typename CommandEncoderBase>
class CommandEncoder : public snap::rhi::backend::common::CommandEncoder<CommandEncoderBase> {
public:
    CommandEncoder(snap::rhi::backend::metal::Device* device, snap::rhi::backend::metal::CommandBuffer* commandBuffer)
        : snap::rhi::backend::common::CommandEncoder<CommandEncoderBase>(device, commandBuffer) {}
    ~CommandEncoder() override = default;

    void writeTimestamp(snap::rhi::QueryPool* queryPool,
                        uint32_t query,
                        const snap::rhi::TimestampLocation location) override final {
        snap::rhi::backend::metal::writeTimestamp(
            CommandEncoderBase::commandBuffer, encoder, queryPool, query, location);
    }

    void onBeginEncoding(const id<MTLCommandEncoder>& encoder) {
        snap::rhi::backend::common::CommandEncoder<CommandEncoderBase>::onBeginEncoding();
        this->encoder = encoder;
    }

    void onEndEncoding() {
        snap::rhi::backend::common::CommandEncoder<CommandEncoderBase>::onEndEncoding();
        this->encoder = nil;
    }

    void beginDebugGroup(std::string_view label) final {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
        NSString* nsName = [NSString stringWithUTF8String:std::string(label).c_str()];
        [encoder pushDebugGroup:nsName];
#endif
    }

    void endDebugGroup() final {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
        [encoder popDebugGroup];
#endif
    }

private:
    id<MTLCommandEncoder> encoder = nil;
};
} // namespace snap::rhi::backend::metal
