//
//  Context.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 07.10.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <Metal/Metal.h>
#include <snap/rhi/CommandBufferCreateInfo.h>
#include <snap/rhi/backend/metal/ResourceBatcher.h>

namespace snap::rhi::backend::metal {
class Device;

class Context final {
public:
    explicit Context(Device* mtlDevice, const snap::rhi::CommandBufferCreateInfo& info);
    ~Context();

    SNAP_RHI_ALWAYS_INLINE void useResource(id<MTLResource> __unsafe_unretained resource,
                                            const MTLResourceUsage usage,
                                            const MTLRenderStages stages) {
        resourceBatcher.add(resource, usage, stages);
    }

    const id<MTLRenderCommandEncoder>& beginRender(MTLRenderPassDescriptor* descriptor);
    const id<MTLRenderCommandEncoder>& getRenderEncoder() const;
    void endRender();

    const id<MTLBlitCommandEncoder>& beginBlit();
    const id<MTLBlitCommandEncoder>& getBlitEncoder() const;
    void endBlit();

    const id<MTLComputeCommandEncoder>& beginCompute();
    const id<MTLComputeCommandEncoder>& getComputeEncoder() const;
    void endCompute();

    const id<MTLCommandBuffer>& getCommandBuffer() const;

private:
    Device* device = nullptr;
    ResourceBatcher resourceBatcher;

    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLBlitCommandEncoder> blitEncoder = nil;
    id<MTLRenderCommandEncoder> renderEncoder = nil;
    id<MTLComputeCommandEncoder> computeEncoder = nil;
};
} // namespace snap::rhi::backend::metal
