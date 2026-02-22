#include "snap/rhi/backend/metal/Context.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Buffer.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Texture.h"
#include "snap/rhi/common/Throw.h"

namespace {
id<MTLCommandBuffer> createMtlCommandBuffer(const snap::rhi::CommandBufferCreateInfo& info) {
    auto* commandQueue =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::CommandQueue>(info.commandQueue);
    id<MTLCommandQueue> mtlCommandQueue = commandQueue->getMtlCommandQueue();

    /**
     * For Metal, we always create command buffer with unretained references to avoid retain cycle.
     */
    return [mtlCommandQueue commandBufferWithUnretainedReferences];
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
Context::Context(Device* mtlDevice, const snap::rhi::CommandBufferCreateInfo& info)
    : device(mtlDevice),
      resourceBatcher(mtlDevice->acquireResourceBatcher()),
      commandBuffer(createMtlCommandBuffer(info)) {}

Context::~Context() {
    assert(blitEncoder == nil && renderEncoder == nil && computeEncoder == nil);
    resourceBatcher.reset();
    device->releaseResourceBatcher(std::move(resourceBatcher));
}

const id<MTLRenderCommandEncoder>& Context::beginRender(MTLRenderPassDescriptor* descriptor) {
    assert(blitEncoder == nil && renderEncoder == nil);
    renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:descriptor];
    return renderEncoder;
}

const id<MTLRenderCommandEncoder>& Context::getRenderEncoder() const {
    return renderEncoder;
}

void Context::endRender() {
    assert(blitEncoder == nil && computeEncoder == nil && renderEncoder != nil && commandBuffer != nil);

    resourceBatcher.useResources(renderEncoder);
    [renderEncoder endEncoding];
    renderEncoder = nil;
    resourceBatcher.reset();
}

const id<MTLBlitCommandEncoder>& Context::beginBlit() {
    assert(renderEncoder == nil && blitEncoder == nil);
    blitEncoder = [commandBuffer blitCommandEncoder];

    return blitEncoder;
}

const id<MTLBlitCommandEncoder>& Context::getBlitEncoder() const {
    return blitEncoder;
}

void Context::endBlit() {
    assert(renderEncoder == nil && blitEncoder != nil && commandBuffer != nil);

    [blitEncoder endEncoding];
    blitEncoder = nil;
    resourceBatcher.reset();
}

const id<MTLComputeCommandEncoder>& Context::beginCompute() {
    assert(renderEncoder == nil && blitEncoder == nil && computeEncoder == nil);
    computeEncoder = [commandBuffer computeCommandEncoder];

    return computeEncoder;
}

const id<MTLComputeCommandEncoder>& Context::getComputeEncoder() const {
    return computeEncoder;
}

void Context::endCompute() {
    assert(renderEncoder == nil && blitEncoder == nil && computeEncoder != nil && commandBuffer != nil);

    resourceBatcher.useResources(computeEncoder);
    [computeEncoder endEncoding];
    computeEncoder = nil;
    resourceBatcher.reset();
}

const id<MTLCommandBuffer>& Context::getCommandBuffer() const {
    return commandBuffer;
}
} // namespace snap::rhi::backend::metal
