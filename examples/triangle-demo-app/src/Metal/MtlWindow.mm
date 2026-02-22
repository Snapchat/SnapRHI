#include "MtlWindow.h"

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <SDL3/SDL_metal.h>

#include <TargetConditionals.h>

#if TARGET_OS_IOS
#include <UIKit/UIKit.h>
#else
#include <AppKit/AppKit.h>
#endif

#include <SDL3/SDL.h>
#include <snap/rhi/CommandQueue.hpp>
#include <snap/rhi/backend/common/Utils.hpp>
#include <snap/rhi/backend/metal/CommandBuffer.h>
#include <snap/rhi/backend/metal/Device.h>
#include <snap/rhi/backend/metal/DeviceFactory.h>
#include <stdexcept>

namespace snap::rhi::demo {
MtlWindow::MtlWindow(const std::string& title, int width, int height) : Window(title, width, height, SDL_WINDOW_METAL) {
    snap::rhi::backend::metal::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.enabledReportLevel = snap::rhi::ReportLevel::All;
    deviceCreateInfo.enabledTags = snap::rhi::ValidationTag::All;

    device = snap::rhi::backend::metal::createDevice(deviceCreateInfo);
    commandQueue = device->getCommandQueue(0, 0);
    {
        auto* mtlDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Device>(device.get());
#if TARGET_OS_IOS
        UIView* view = (__bridge UIView*)SDL_Metal_CreateView(window);
#else
        NSView* view = (__bridge NSView*)SDL_Metal_CreateView(window);
#endif
        // Setup CAMetalLayer (The Swapchain)
        CAMetalLayer* layer = (CAMetalLayer*)view.layer;
        layer.device = mtlDevice->getMtlDevice();
        layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer = (__bridge_retained void*)layer;
    }

    triangleRenderer = std::make_unique<TriangleRenderer>(device);
}

MtlWindow::~MtlWindow() {
    triangleRenderer.reset();
    device.reset();

    if (metalLayer) {
        CFRelease(metalLayer);
        metalLayer = nullptr;
    }
}

FrameGuard MtlWindow::acquireFrame() {
    CAMetalLayer* layer = (__bridge CAMetalLayer*)metalLayer;
    id<CAMetalDrawable> currentDrawable = [layer nextDrawable];
    if (!currentDrawable) {
        return {}; // Return invalid guard on failure
    }
    this->drawable = (__bridge void*)currentDrawable;

    const id<MTLTexture>& mtlTexture = currentDrawable.texture;

    // Fill the description based on the drawable.
    // Note: CAMetalLayer drawables are always 2D, single-sampled, and have mipLevels=1.
    snap::rhi::TextureCreateInfo textureDesc{
        .format = snap::rhi::PixelFormat::B8G8R8A8Unorm, // CAMetalLayer pixelFormat is set to BGRA8Unorm in ctor;
                                                         // SnapRHI uses this as a logical format.
        .textureType = snap::rhi::TextureType::Texture2D,
        .textureUsage = snap::rhi::TextureUsage::ColorAttachment | snap::rhi::TextureUsage::TransferSrc |
                        snap::rhi::TextureUsage::TransferDst | snap::rhi::TextureUsage::Sampled,
        .sampleCount = snap::rhi::SampleCount::Count1,
        .mipLevels = 1,
        .size = {static_cast<uint32_t>(mtlTexture.width), static_cast<uint32_t>(mtlTexture.height), 1},
    };

    auto* mtlDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Device>(device.get());
    if (!mtlDevice) {
        return {}; // Return invalid guard on failure
    }

    auto renderTarget = mtlDevice->createTexture(mtlTexture, textureDesc);

    // Create command buffer for this frame
    auto commandBuffer = device->createCommandBuffer({
        .commandQueue = commandQueue,
    });

    // Retain the drawable and capture it for the submit callback.
    void* capturedDrawable = (__bridge_retained void*)currentDrawable;

    auto submitCallback = [this, capturedDrawable](std::shared_ptr<snap::rhi::CommandBuffer> cmdBuffer,
                                                   std::shared_ptr<snap::rhi::Texture> /*target*/) {
        // Immediately transfer ownership to ARC to ensure it's released even on early return.
        id<CAMetalDrawable> drawableToPresent = (__bridge_transfer id<CAMetalDrawable>)capturedDrawable;

        if (!cmdBuffer || !commandQueue) {
            return;
        }

        auto* mtlCommandBuffer =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::CommandBuffer>(cmdBuffer.get());
        auto& ctx = mtlCommandBuffer->getContext();
        const auto& buffer = ctx.getCommandBuffer();

        [buffer presentDrawable:drawableToPresent];

        commandQueue->submitCommandBuffer(cmdBuffer.get(), snap::rhi::CommandBufferWaitType::DoNotWait, nullptr);
    };

    return FrameGuard(std::move(commandBuffer), std::move(renderTarget), std::move(submitCallback));
}
} // namespace snap::rhi::demo
