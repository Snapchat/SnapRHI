//
//  RenderCommandEncoderTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::RenderCommandEncoder — draw calls, state setting, dynamic rendering.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "TestHarness/ReadbackHelper.h"
#include "TestHarness/ShaderHelper.h"
#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/DescriptorPool.hpp"
#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/DescriptorSetLayout.hpp"
#include "snap/rhi/Framebuffer.hpp"
#include "snap/rhi/PipelineLayout.hpp"
#include "snap/rhi/RenderCommandEncoder.hpp"
#include "snap/rhi/RenderPass.hpp"
#include "snap/rhi/RenderPipeline.hpp"
#include "snap/rhi/ShaderLibrary.hpp"
#include "snap/rhi/ShaderModule.hpp"
#include "snap/rhi/Texture.hpp"
#include <catch2/catch.hpp>
#include <cstring>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

/**
 * @brief Helper: creates all resources needed for a basic render pipeline test.
 */
struct RenderTestResources {
    test_harness::RenderShaderSet shaders;
    std::shared_ptr<snap::rhi::DescriptorSetLayout> dsLayout;
    std::shared_ptr<snap::rhi::PipelineLayout> pipelineLayout;
    std::shared_ptr<snap::rhi::RenderPass> renderPass;
    std::shared_ptr<snap::rhi::Texture> renderTarget;
    std::shared_ptr<snap::rhi::Framebuffer> framebuffer;
    std::shared_ptr<snap::rhi::RenderPipeline> pipeline;
    std::shared_ptr<snap::rhi::Buffer> vertexBuffer;
    std::shared_ptr<snap::rhi::Buffer> uniformBuffer;
    std::shared_ptr<snap::rhi::DescriptorPool> descriptorPool;
    std::shared_ptr<snap::rhi::DescriptorSet> descriptorSet;
};

static std::optional<RenderTestResources> createRenderResources(
    snap::rhi::Device* device,
    const test_harness::DeviceTestContext& ctx) {

    auto shaders = test_harness::createPassthroughShaders(device);
    if (!shaders) return std::nullopt;

    RenderTestResources res;
    res.shaders = *shaders;

    // DS layout
    snap::rhi::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
    dsLayoutInfo.bindings.push_back({
        .binding = 0,
        .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
        .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
    });
    res.dsLayout = device->createDescriptorSetLayout(dsLayoutInfo);

    // Pipeline layout
    snap::rhi::DescriptorSetLayout* layouts[] = {res.dsLayout.get()};
    snap::rhi::PipelineLayoutCreateInfo plInfo{};
    plInfo.setLayouts = layouts;
    res.pipelineLayout = device->createPipelineLayout(plInfo);

    // Render pass
    snap::rhi::RenderPassCreateInfo rpInfo{};
    rpInfo.attachments[0].format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
    rpInfo.attachments[0].samples = snap::rhi::SampleCount::Count1;
    rpInfo.attachments[0].loadOp = snap::rhi::AttachmentLoadOp::Clear;
    rpInfo.attachments[0].storeOp = snap::rhi::AttachmentStoreOp::Store;
    rpInfo.attachmentCount = 1;

    rpInfo.subpasses[0].colorAttachments[0] = {.attachment = 0};
    rpInfo.subpasses[0].colorAttachmentCount = 1;
    rpInfo.subpassCount = 1;
    res.renderPass = device->createRenderPass(rpInfo);

    // Render target texture
    snap::rhi::TextureCreateInfo texInfo{};
    texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
    texInfo.textureType = snap::rhi::TextureType::Texture2D;
    texInfo.textureUsage = snap::rhi::TextureUsage::ColorAttachment |
                           snap::rhi::TextureUsage::TransferSrc |
                           snap::rhi::TextureUsage::Sampled;
    texInfo.sampleCount = snap::rhi::SampleCount::Count1;
    texInfo.mipLevels = 1;
    texInfo.size = {64, 64, 1};
    res.renderTarget = device->createTexture(texInfo);

    // Framebuffer
    snap::rhi::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = res.renderPass.get();
    fbInfo.attachments = {res.renderTarget.get()};
    fbInfo.width = 64;
    fbInfo.height = 64;
    res.framebuffer = device->createFramebuffer(fbInfo);

    // Pipeline
    snap::rhi::RenderPipelineCreateInfo pipeInfo{};
    pipeInfo.stages = {res.shaders.vertexShader.get(), res.shaders.fragmentShader.get()};
    pipeInfo.pipelineLayout = res.pipelineLayout.get();
    pipeInfo.renderPass = res.renderPass.get();
    pipeInfo.subpass = 0;

    pipeInfo.vertexInputState.attributesCount = 2;
    pipeInfo.vertexInputState.attributeDescription[0] = {
        .location = 0, .binding = 0, .offset = 0, .format = snap::rhi::VertexAttributeFormat::Float2};
    pipeInfo.vertexInputState.attributeDescription[1] = {
        .location = 1, .binding = 0, .offset = 8, .format = snap::rhi::VertexAttributeFormat::Float4};
    pipeInfo.vertexInputState.bindingsCount = 1;
    pipeInfo.vertexInputState.bindingDescription[0] = {.binding = 0, .stride = 24};

    pipeInfo.rasterizationState.rasterizationEnabled = true;
    pipeInfo.rasterizationState.cullMode = snap::rhi::CullMode::None;
    pipeInfo.colorBlendState.colorAttachmentsCount = 1;
    pipeInfo.colorBlendState.colorAttachmentsBlendState[0].colorWriteMask = snap::rhi::ColorMask::All;

    if (res.shaders.glPipelineInfo) {
        pipeInfo.glRenderPipelineInfo = res.shaders.glPipelineInfo;
    }
    if (res.shaders.mtlRenderPipelineInfo) {
        pipeInfo.mtlRenderPipelineInfo = res.shaders.mtlRenderPipelineInfo;
    }

    res.pipeline = device->createRenderPipeline(pipeInfo);

    // Vertex buffer (3 verts: pos vec2 + color vec4 = 24 bytes each)
    snap::rhi::BufferCreateInfo vbInfo{};
    vbInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
    vbInfo.memoryProperties =
        snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
    vbInfo.size = 3 * 24; // 3 vertices * 24 bytes
    res.vertexBuffer = device->createBuffer(vbInfo);
    {
        float verts[] = {
            // pos.x, pos.y, r, g, b, a
            0.0f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f,
           -0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f,
            0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
        };
        auto* mapped = res.vertexBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(verts));
        std::memcpy(mapped, verts, sizeof(verts));
        res.vertexBuffer->unmap();
    }

    // Uniform buffer (vec4 colorMultiplier = white)
    snap::rhi::BufferCreateInfo ubInfo{};
    ubInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
    ubInfo.memoryProperties =
        snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
    ubInfo.size = 16;
    res.uniformBuffer = device->createBuffer(ubInfo);
    {
        float ubo[] = {1.0f, 1.0f, 1.0f, 1.0f};
        auto* mapped = res.uniformBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(ubo));
        std::memcpy(mapped, ubo, sizeof(ubo));
        res.uniformBuffer->unmap();
    }

    // Descriptor pool & set
    snap::rhi::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.maxSets = 1;
    poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] = 1;
    res.descriptorPool = device->createDescriptorPool(poolInfo);

    snap::rhi::DescriptorSetCreateInfo dsInfo{};
    dsInfo.descriptorPool = res.descriptorPool.get();
    dsInfo.descriptorSetLayout = res.dsLayout.get();
    res.descriptorSet = device->createDescriptorSet(dsInfo);
    res.descriptorSet->bindUniformBuffer(0, res.uniformBuffer.get());

    return res;
}

TEST_CASE("RenderCommandEncoder — Basic draw call", "[api][rendering][encoder]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {

        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            auto res = createRenderResources(ctx->device.get(), *ctx);
            if (!res) {
                WARN("Skipping render encoder test for " << ctx->apiName);
                break;
            }

            // ── Draw triangle ──────────────────────────────────
            {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                REQUIRE(cmdBuf != nullptr);

                auto* renderEncoder = cmdBuf->getRenderCommandEncoder();
                REQUIRE(renderEncoder != nullptr);

                snap::rhi::RenderPassBeginInfo rpBegin{};
                rpBegin.renderPass = res->renderPass.get();
                rpBegin.framebuffer = res->framebuffer.get();
                snap::rhi::ClearValue clearValue{};
                clearValue.color.float32 = {0.0f, 0.0f, 0.0f, 1.0f};
                rpBegin.clearValues.push_back(clearValue);

                renderEncoder->beginEncoding(rpBegin);
                renderEncoder->setViewport({0, 0, 64, 64, 0.0f, 1.0f});
                renderEncoder->bindRenderPipeline(res->pipeline.get());
                renderEncoder->bindVertexBuffer(0, res->vertexBuffer.get(), 0);
                renderEncoder->bindDescriptorSet(0, res->descriptorSet.get(), {});
                renderEncoder->draw(3, 0, 1);
                renderEncoder->endEncoding();

                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

                // Readback & verify pixel output
                auto pixels = test_harness::readbackTexture(
                    ctx->device.get(), ctx->commandQueue, res->renderTarget.get(), 64, 64);
                REQUIRE(pixels.size() == 64u * 64u);

                // Corner (0,0) should be the clear color (black, opaque)
                test_harness::Pixel clearPx{0, 0, 0, 255};
                CHECK(test_harness::pixelApproxEqual(
                    test_harness::getPixel(pixels, 64, 0, 0), clearPx, 5));

                // Triangle must have produced non-black pixels
                CHECK(test_harness::hasNonBlackPixel(pixels));
            }

            // ── Dynamic state ──────────────────────────────────
            {
                auto cmdBuf2 = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* enc2 = cmdBuf2->getRenderCommandEncoder();

                snap::rhi::RenderPassBeginInfo rpBegin2{};
                rpBegin2.renderPass = res->renderPass.get();
                rpBegin2.framebuffer = res->framebuffer.get();
                snap::rhi::ClearValue cv{};
                cv.color.float32 = {0.0f, 0.0f, 0.0f, 1.0f};
                rpBegin2.clearValues.push_back(cv);

                enc2->beginEncoding(rpBegin2);
                enc2->setViewport({0, 0, 64, 64, 0.0f, 1.0f});
                enc2->bindRenderPipeline(res->pipeline.get());
                enc2->setDepthBias(0.0f, 0.0f, 0.0f);
                enc2->setStencilReference(snap::rhi::StencilFace::FrontAndBack, 0);
                enc2->setBlendConstants(1.0f, 1.0f, 1.0f, 1.0f);
                enc2->bindVertexBuffer(0, res->vertexBuffer.get(), 0);
                enc2->bindDescriptorSet(0, res->descriptorSet.get(), {});
                enc2->draw(3, 0, 1);
                enc2->endEncoding();

                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf2.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
            }

            ctx->commandQueue->waitIdle();
        }
    }
}

TEST_CASE("RenderCommandEncoder — Dynamic rendering (VK_KHR_dynamic_rendering)", "[api][rendering][dynamic]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {

        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        // Dynamic rendering requires specific capability
        const auto& caps = ctx->device->getCapabilities();
        if (!caps.isDynamicRenderingSupported) continue;

        auto shaders = test_harness::createPassthroughShaders(ctx->device.get());
        if (!shaders) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            // DS layout + pipeline layout (same as above)
            snap::rhi::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
            dsLayoutInfo.bindings.push_back({
                .binding = 0,
                .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
            });
            auto dsLayout = ctx->device->createDescriptorSetLayout(dsLayoutInfo);
            snap::rhi::DescriptorSetLayout* layouts[] = {dsLayout.get()};
            snap::rhi::PipelineLayoutCreateInfo plInfo{};
            plInfo.setLayouts = layouts;
            auto pipelineLayout = ctx->device->createPipelineLayout(plInfo);

            // Render target
            snap::rhi::TextureCreateInfo texInfo{};
            texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            texInfo.textureType = snap::rhi::TextureType::Texture2D;
            texInfo.textureUsage = snap::rhi::TextureUsage::ColorAttachment |
                                   snap::rhi::TextureUsage::TransferSrc |
                                   snap::rhi::TextureUsage::Sampled;
            texInfo.sampleCount = snap::rhi::SampleCount::Count1;
            texInfo.mipLevels = 1;
            texInfo.size = {64, 64, 1};
            auto renderTarget = ctx->device->createTexture(texInfo);

            // Pipeline with attachment formats (no render pass)
            snap::rhi::RenderPipelineCreateInfo pipeInfo{};
            pipeInfo.stages = {shaders->vertexShader.get(), shaders->fragmentShader.get()};
            pipeInfo.pipelineLayout = pipelineLayout.get();
            pipeInfo.attachmentFormatsCreateInfo.colorAttachmentFormats = {snap::rhi::PixelFormat::R8G8B8A8Unorm};
            pipeInfo.vertexInputState.attributesCount = 2;
            pipeInfo.vertexInputState.attributeDescription[0] = {
                .location = 0, .binding = 0, .offset = 0, .format = snap::rhi::VertexAttributeFormat::Float2};
            pipeInfo.vertexInputState.attributeDescription[1] = {
                .location = 1, .binding = 0, .offset = 8, .format = snap::rhi::VertexAttributeFormat::Float4};
            pipeInfo.vertexInputState.bindingsCount = 1;
            pipeInfo.vertexInputState.bindingDescription[0] = {.binding = 0, .stride = 24};
            pipeInfo.rasterizationState.rasterizationEnabled = true;
            pipeInfo.rasterizationState.cullMode = snap::rhi::CullMode::None;
            pipeInfo.colorBlendState.colorAttachmentsCount = 1;
            pipeInfo.colorBlendState.colorAttachmentsBlendState[0].colorWriteMask = snap::rhi::ColorMask::All;

            if (shaders->glPipelineInfo) {
                pipeInfo.glRenderPipelineInfo = shaders->glPipelineInfo;
            }
            if (shaders->mtlRenderPipelineInfo) {
                pipeInfo.mtlRenderPipelineInfo = shaders->mtlRenderPipelineInfo;
            }

            auto pipeline = ctx->device->createRenderPipeline(pipeInfo);
            REQUIRE(pipeline != nullptr);

            // Vertex buffer (3 verts: pos vec2 + color vec4 = 24 bytes each)
            snap::rhi::BufferCreateInfo vbInfo{};
            vbInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
            vbInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            vbInfo.size = 3 * 24;
            auto vertexBuffer = ctx->device->createBuffer(vbInfo);
            {
                float verts[] = {
                    0.0f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f,
                   -0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f,
                    0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
                };
                auto* mapped = vertexBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(verts));
                std::memcpy(mapped, verts, sizeof(verts));
                vertexBuffer->unmap();
            }

            // Uniform buffer (vec4 colorMultiplier = white)
            snap::rhi::BufferCreateInfo ubInfo{};
            ubInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
            ubInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            ubInfo.size = 16;
            auto uniformBuffer = ctx->device->createBuffer(ubInfo);
            {
                float ubo[] = {1.0f, 1.0f, 1.0f, 1.0f};
                auto* mapped = uniformBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(ubo));
                std::memcpy(mapped, ubo, sizeof(ubo));
                uniformBuffer->unmap();
            }

            // Descriptor pool & set
            snap::rhi::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.maxSets = 1;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] = 1;
            auto descriptorPool = ctx->device->createDescriptorPool(poolInfo);

            snap::rhi::DescriptorSetCreateInfo dsSetInfo{};
            dsSetInfo.descriptorPool = descriptorPool.get();
            dsSetInfo.descriptorSetLayout = dsLayout.get();
            auto descriptorSet = ctx->device->createDescriptorSet(dsSetInfo);
            descriptorSet->bindUniformBuffer(0, uniformBuffer.get());

            // Begin dynamic rendering
            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            auto* renderEncoder = cmdBuf->getRenderCommandEncoder();

            snap::rhi::RenderingAttachmentInfo colorAtt{};
            colorAtt.attachment.texture = renderTarget.get();
            colorAtt.loadOp = snap::rhi::AttachmentLoadOp::Clear;
            colorAtt.storeOp = snap::rhi::AttachmentStoreOp::Store;
            colorAtt.clearValue.color.float32 = {0.0f, 0.0f, 0.0f, 1.0f};

            snap::rhi::RenderingInfo renderingInfo{};
            renderingInfo.colorAttachments.push_back(colorAtt);
            renderEncoder->beginEncoding(renderingInfo);
            renderEncoder->setViewport({0, 0, 64, 64, 0.0f, 1.0f});
            renderEncoder->bindRenderPipeline(pipeline.get());
            renderEncoder->bindVertexBuffer(0, vertexBuffer.get(), 0);
            renderEncoder->bindDescriptorSet(0, descriptorSet.get(), {});
            renderEncoder->draw(3, 0, 1);
            renderEncoder->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

            // Readback & verify
            auto pixels = test_harness::readbackTexture(
                ctx->device.get(), ctx->commandQueue, renderTarget.get(), 64, 64);
            REQUIRE(pixels.size() == 64u * 64u);
            CHECK(test_harness::hasNonBlackPixel(pixels));
        }
    }
}

TEST_CASE("RenderCommandEncoder — Indexed draw", "[api][rendering][indexed]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {

        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            auto res = createRenderResources(ctx->device.get(), *ctx);
            if (!res) continue;

            // Index buffer (3 indices)
            snap::rhi::BufferCreateInfo ibInfo{};
            ibInfo.bufferUsage = snap::rhi::BufferUsage::IndexBuffer;
            ibInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            ibInfo.size = 3 * sizeof(uint16_t);
            auto indexBuffer = ctx->device->createBuffer(ibInfo);
            {
                uint16_t indices[] = {0, 1, 2};
                auto* mapped = indexBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(indices));
                std::memcpy(mapped, indices, sizeof(indices));
                indexBuffer->unmap();
            }

            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            auto* renderEncoder = cmdBuf->getRenderCommandEncoder();

            snap::rhi::RenderPassBeginInfo rpBegin{};
            rpBegin.renderPass = res->renderPass.get();
            rpBegin.framebuffer = res->framebuffer.get();
            snap::rhi::ClearValue cv{};
            cv.color.float32 = {0.0f, 0.0f, 0.0f, 1.0f};
            rpBegin.clearValues.push_back(cv);

            renderEncoder->beginEncoding(rpBegin);
            renderEncoder->setViewport({0, 0, 64, 64, 0.0f, 1.0f});
            renderEncoder->bindRenderPipeline(res->pipeline.get());
            renderEncoder->bindVertexBuffer(0, res->vertexBuffer.get(), 0);
            renderEncoder->bindIndexBuffer(indexBuffer.get(), 0, snap::rhi::IndexType::UInt16);
            renderEncoder->bindDescriptorSet(0, res->descriptorSet.get(), {});
            renderEncoder->drawIndexed(3, 0, 1);
            renderEncoder->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

            // Readback & verify indexed draw output
            auto pixels = test_harness::readbackTexture(
                ctx->device.get(), ctx->commandQueue, res->renderTarget.get(), 64, 64);
            REQUIRE(pixels.size() == 64u * 64u);

            test_harness::Pixel clearPx{0, 0, 0, 255};
            CHECK(test_harness::pixelApproxEqual(
                test_harness::getPixel(pixels, 64, 0, 0), clearPx, 5));
            CHECK(test_harness::hasNonBlackPixel(pixels));

            ctx->commandQueue->waitIdle();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Instanced drawing — draw the same triangle N times with per-instance offset
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("RenderCommandEncoder — Instanced draw", "[api][rendering][instancing]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {

        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            auto res = createRenderResources(ctx->device.get(), *ctx);
            if (!res) {
                WARN("Skipping instanced draw test for " << ctx->apiName);
                break;
            }

            // ── Single-instance reference ─────────────────────
            std::vector<test_harness::Pixel> singleInstancePixels;
            {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* enc = cmdBuf->getRenderCommandEncoder();
                snap::rhi::RenderPassBeginInfo rpBegin{};
                rpBegin.renderPass = res->renderPass.get();
                rpBegin.framebuffer = res->framebuffer.get();
                snap::rhi::ClearValue cv{}; cv.color.float32 = {0, 0, 0, 1};
                rpBegin.clearValues.push_back(cv);
                enc->beginEncoding(rpBegin);
                enc->setViewport({0, 0, 64, 64, 0, 1});
                enc->bindRenderPipeline(res->pipeline.get());
                enc->bindVertexBuffer(0, res->vertexBuffer.get(), 0);
                enc->bindDescriptorSet(0, res->descriptorSet.get(), {});
                enc->draw(3, 0, 1);
                enc->endEncoding();
                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
                singleInstancePixels = test_harness::readbackTexture(
                    ctx->device.get(), ctx->commandQueue, res->renderTarget.get(), 64, 64);
            }
            REQUIRE(test_harness::hasNonBlackPixel(singleInstancePixels));

            // ── Multi-instance draw ───────────────────────────
            {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* enc = cmdBuf->getRenderCommandEncoder();
                snap::rhi::RenderPassBeginInfo rpBegin{};
                rpBegin.renderPass = res->renderPass.get();
                rpBegin.framebuffer = res->framebuffer.get();
                snap::rhi::ClearValue cv{}; cv.color.float32 = {0, 0, 0, 1};
                rpBegin.clearValues.push_back(cv);
                enc->beginEncoding(rpBegin);
                enc->setViewport({0, 0, 64, 64, 0, 1});
                enc->bindRenderPipeline(res->pipeline.get());
                enc->bindVertexBuffer(0, res->vertexBuffer.get(), 0);
                enc->bindDescriptorSet(0, res->descriptorSet.get(), {});
                enc->draw(3, 0, 4); // 4 instances
                enc->endEncoding();
                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
            }
            auto multiPixels = test_harness::readbackTexture(
                ctx->device.get(), ctx->commandQueue, res->renderTarget.get(), 64, 64);
            REQUIRE(multiPixels.size() == 64u * 64u);
            CHECK(test_harness::hasNonBlackPixel(multiPixels));

            // Without per-instance data all instances overlap — output should match
            // single-instance result pixel-for-pixel (proves instancing didn't corrupt state).
            auto singleAvg = test_harness::averageColor(singleInstancePixels);
            auto multiAvg  = test_harness::averageColor(multiPixels);
            CHECK(test_harness::pixelApproxEqual(singleAvg, multiAvg, 5));

            // ── Indexed instanced draw ────────────────────────
            snap::rhi::BufferCreateInfo ibInfo{};
            ibInfo.bufferUsage = snap::rhi::BufferUsage::IndexBuffer;
            ibInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            ibInfo.size = 3 * sizeof(uint16_t);
            auto indexBuffer = ctx->device->createBuffer(ibInfo);
            {
                uint16_t idx[] = {0, 1, 2};
                auto* m = indexBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(idx));
                std::memcpy(m, idx, sizeof(idx));
                indexBuffer->unmap();
            }
            {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* enc = cmdBuf->getRenderCommandEncoder();
                snap::rhi::RenderPassBeginInfo rpBegin{};
                rpBegin.renderPass = res->renderPass.get();
                rpBegin.framebuffer = res->framebuffer.get();
                snap::rhi::ClearValue cv{}; cv.color.float32 = {0, 0, 0, 1};
                rpBegin.clearValues.push_back(cv);
                enc->beginEncoding(rpBegin);
                enc->setViewport({0, 0, 64, 64, 0, 1});
                enc->bindRenderPipeline(res->pipeline.get());
                enc->bindVertexBuffer(0, res->vertexBuffer.get(), 0);
                enc->bindIndexBuffer(indexBuffer.get(), 0, snap::rhi::IndexType::UInt16);
                enc->bindDescriptorSet(0, res->descriptorSet.get(), {});
                enc->drawIndexed(3, 0, 4); // 4 instances, indexed
                enc->endEncoding();
                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
            }
            auto indexedPixels = test_harness::readbackTexture(
                ctx->device.get(), ctx->commandQueue, res->renderTarget.get(), 64, 64);
            CHECK(test_harness::hasNonBlackPixel(indexedPixels));

            ctx->commandQueue->waitIdle();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Alpha blending — blend a semi-transparent triangle over a cleared background
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("RenderCommandEncoder — Alpha blending", "[api][rendering][blending]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {

        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        auto shaders = test_harness::createPassthroughShaders(ctx->device.get());
        if (!shaders) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            // DS layout + pipeline layout
            snap::rhi::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
            dsLayoutInfo.bindings.push_back({
                .binding = 0,
                .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
            });
            auto dsLayout = ctx->device->createDescriptorSetLayout(dsLayoutInfo);
            snap::rhi::DescriptorSetLayout* layouts[] = {dsLayout.get()};
            snap::rhi::PipelineLayoutCreateInfo plInfo{};
            plInfo.setLayouts = layouts;
            auto pipelineLayout = ctx->device->createPipelineLayout(plInfo);

            // Render pass + render target
            snap::rhi::RenderPassCreateInfo rpInfo{};
            rpInfo.attachments[0].format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            rpInfo.attachments[0].samples = snap::rhi::SampleCount::Count1;
            rpInfo.attachments[0].loadOp = snap::rhi::AttachmentLoadOp::Clear;
            rpInfo.attachments[0].storeOp = snap::rhi::AttachmentStoreOp::Store;
            rpInfo.attachmentCount = 1;
            rpInfo.subpasses[0].colorAttachments[0] = {.attachment = 0};
            rpInfo.subpasses[0].colorAttachmentCount = 1;
            rpInfo.subpassCount = 1;
            auto renderPass = ctx->device->createRenderPass(rpInfo);

            snap::rhi::TextureCreateInfo texInfo{};
            texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            texInfo.textureType = snap::rhi::TextureType::Texture2D;
            texInfo.textureUsage = snap::rhi::TextureUsage::ColorAttachment |
                                   snap::rhi::TextureUsage::TransferSrc |
                                   snap::rhi::TextureUsage::Sampled;
            texInfo.sampleCount = snap::rhi::SampleCount::Count1;
            texInfo.mipLevels = 1;
            texInfo.size = {64, 64, 1};
            auto renderTarget = ctx->device->createTexture(texInfo);

            snap::rhi::FramebufferCreateInfo fbInfo{};
            fbInfo.renderPass = renderPass.get();
            fbInfo.attachments = {renderTarget.get()};
            fbInfo.width = 64;
            fbInfo.height = 64;
            auto framebuffer = ctx->device->createFramebuffer(fbInfo);

            // Pipeline with alpha blending enabled
            snap::rhi::RenderPipelineCreateInfo pipeInfo{};
            pipeInfo.stages = {shaders->vertexShader.get(), shaders->fragmentShader.get()};
            pipeInfo.pipelineLayout = pipelineLayout.get();
            pipeInfo.renderPass = renderPass.get();
            pipeInfo.subpass = 0;
            pipeInfo.vertexInputState.attributesCount = 2;
            pipeInfo.vertexInputState.attributeDescription[0] = {
                .location = 0, .binding = 0, .offset = 0, .format = snap::rhi::VertexAttributeFormat::Float2};
            pipeInfo.vertexInputState.attributeDescription[1] = {
                .location = 1, .binding = 0, .offset = 8, .format = snap::rhi::VertexAttributeFormat::Float4};
            pipeInfo.vertexInputState.bindingsCount = 1;
            pipeInfo.vertexInputState.bindingDescription[0] = {.binding = 0, .stride = 24};
            pipeInfo.rasterizationState.rasterizationEnabled = true;
            pipeInfo.rasterizationState.cullMode = snap::rhi::CullMode::None;

            // Alpha blend: SrcAlpha / OneMinusSrcAlpha
            pipeInfo.colorBlendState.colorAttachmentsCount = 1;
            auto& blendState = pipeInfo.colorBlendState.colorAttachmentsBlendState[0];
            blendState.blendEnable = true;
            blendState.srcColorBlendFactor = snap::rhi::BlendFactor::SrcAlpha;
            blendState.dstColorBlendFactor = snap::rhi::BlendFactor::OneMinusSrcAlpha;
            blendState.colorBlendOp = snap::rhi::BlendOp::Add;
            blendState.srcAlphaBlendFactor = snap::rhi::BlendFactor::One;
            blendState.dstAlphaBlendFactor = snap::rhi::BlendFactor::Zero;
            blendState.alphaBlendOp = snap::rhi::BlendOp::Add;
            blendState.colorWriteMask = snap::rhi::ColorMask::All;

            if (shaders->glPipelineInfo) pipeInfo.glRenderPipelineInfo = shaders->glPipelineInfo;
            if (shaders->mtlRenderPipelineInfo) pipeInfo.mtlRenderPipelineInfo = shaders->mtlRenderPipelineInfo;

            auto pipeline = ctx->device->createRenderPipeline(pipeInfo);
            REQUIRE(pipeline != nullptr);

            // Vertex buffer: full-screen quad (2 triangles) with 50% alpha red
            snap::rhi::BufferCreateInfo vbInfo{};
            vbInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
            vbInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            vbInfo.size = 6 * 24; // 6 verts * 24 bytes
            auto vertexBuffer = ctx->device->createBuffer(vbInfo);
            {
                // Full-screen quad: red color, 50% alpha
                float verts[] = {
                    -1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.5f,
                     1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.5f,
                     1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.5f,
                    -1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.5f,
                     1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.5f,
                    -1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.5f,
                };
                auto* mapped = vertexBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(verts));
                std::memcpy(mapped, verts, sizeof(verts));
                vertexBuffer->unmap();
            }

            // UBO
            snap::rhi::BufferCreateInfo ubInfo{};
            ubInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
            ubInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            ubInfo.size = 16;
            auto uniformBuffer = ctx->device->createBuffer(ubInfo);
            {
                float ubo[] = {1.0f, 1.0f, 1.0f, 1.0f};
                auto* mapped = uniformBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(ubo));
                std::memcpy(mapped, ubo, sizeof(ubo));
                uniformBuffer->unmap();
            }

            snap::rhi::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.maxSets = 1;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] = 1;
            auto pool = ctx->device->createDescriptorPool(poolInfo);
            snap::rhi::DescriptorSetCreateInfo dsInfo{};
            dsInfo.descriptorPool = pool.get();
            dsInfo.descriptorSetLayout = dsLayout.get();
            auto ds = ctx->device->createDescriptorSet(dsInfo);
            ds->bindUniformBuffer(0, uniformBuffer.get());

            // Draw
            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            auto* renderEncoder = cmdBuf->getRenderCommandEncoder();

            snap::rhi::RenderPassBeginInfo rpBegin{};
            rpBegin.renderPass = renderPass.get();
            rpBegin.framebuffer = framebuffer.get();
            snap::rhi::ClearValue cv{};
            cv.color.float32 = {0.0f, 0.0f, 1.0f, 1.0f}; // Clear to blue
            rpBegin.clearValues.push_back(cv);

            renderEncoder->beginEncoding(rpBegin);
            renderEncoder->setViewport({0, 0, 64, 64, 0.0f, 1.0f});
            renderEncoder->bindRenderPipeline(pipeline.get());
            renderEncoder->bindVertexBuffer(0, vertexBuffer.get(), 0);
            renderEncoder->bindDescriptorSet(0, ds.get(), {});
            renderEncoder->draw(6, 0, 1);
            renderEncoder->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

            auto pixels = test_harness::readbackTexture(
                ctx->device.get(), ctx->commandQueue, renderTarget.get(), 64, 64);
            REQUIRE(pixels.size() == 64u * 64u);

            // Clear was blue (0,0,255,255). Triangle is red at 50% alpha.
            // Blended result: R = 255*0.5 + 0*0.5 ≈ 128, G = 0, B = 0*0.5 + 255*0.5 ≈ 128
            // The center pixel should be a purple-ish blend, NOT pure blue and NOT pure red.
            auto center = test_harness::getPixel(pixels, 64, 32, 32);
            test_harness::Pixel pureBlue{0, 0, 255, 255};
            test_harness::Pixel pureRed{255, 0, 0, 255};
            CHECK_FALSE(test_harness::pixelApproxEqual(center, pureBlue, 10));
            CHECK_FALSE(test_harness::pixelApproxEqual(center, pureRed, 10));
            // Must have both red and blue components from the blend
            CHECK(center.r > 50);
            CHECK(center.b > 50);

            ctx->commandQueue->waitIdle();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Depth testing — draw two overlapping triangles with depth test enabled
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("RenderCommandEncoder — Depth testing", "[api][rendering][depth]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {

        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        auto shaders = test_harness::createPassthroughShaders(ctx->device.get());
        if (!shaders) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            // DS layout + pipeline layout
            snap::rhi::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
            dsLayoutInfo.bindings.push_back({
                .binding = 0,
                .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
            });
            auto dsLayout = ctx->device->createDescriptorSetLayout(dsLayoutInfo);
            snap::rhi::DescriptorSetLayout* dslayouts[] = {dsLayout.get()};
            snap::rhi::PipelineLayoutCreateInfo plInfo{};
            plInfo.setLayouts = dslayouts;
            auto pipelineLayout = ctx->device->createPipelineLayout(plInfo);

            // Render pass with color + depth attachment
            snap::rhi::RenderPassCreateInfo rpInfo{};
            rpInfo.attachments[0].format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            rpInfo.attachments[0].samples = snap::rhi::SampleCount::Count1;
            rpInfo.attachments[0].loadOp = snap::rhi::AttachmentLoadOp::Clear;
            rpInfo.attachments[0].storeOp = snap::rhi::AttachmentStoreOp::Store;
            rpInfo.attachments[1].format = snap::rhi::PixelFormat::Depth16Unorm;
            rpInfo.attachments[1].samples = snap::rhi::SampleCount::Count1;
            rpInfo.attachments[1].loadOp = snap::rhi::AttachmentLoadOp::Clear;
            rpInfo.attachments[1].storeOp = snap::rhi::AttachmentStoreOp::DontCare;
            rpInfo.attachmentCount = 2;
            rpInfo.subpasses[0].colorAttachments[0] = {.attachment = 0};
            rpInfo.subpasses[0].colorAttachmentCount = 1;
            rpInfo.subpasses[0].depthStencilAttachment = {.attachment = 1};
            rpInfo.subpassCount = 1;
            auto renderPass = ctx->device->createRenderPass(rpInfo);
            REQUIRE(renderPass != nullptr);

            // Textures
            snap::rhi::TextureCreateInfo colorTexInfo{};
            colorTexInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            colorTexInfo.textureType = snap::rhi::TextureType::Texture2D;
            colorTexInfo.textureUsage = snap::rhi::TextureUsage::ColorAttachment |
                                        snap::rhi::TextureUsage::TransferSrc |
                                        snap::rhi::TextureUsage::Sampled;
            colorTexInfo.sampleCount = snap::rhi::SampleCount::Count1;
            colorTexInfo.mipLevels = 1;
            colorTexInfo.size = {64, 64, 1};
            auto colorTex = ctx->device->createTexture(colorTexInfo);

            snap::rhi::TextureCreateInfo depthTexInfo{};
            depthTexInfo.format = snap::rhi::PixelFormat::Depth16Unorm;
            depthTexInfo.textureType = snap::rhi::TextureType::Texture2D;
            depthTexInfo.textureUsage = snap::rhi::TextureUsage::DepthStencilAttachment;
            depthTexInfo.sampleCount = snap::rhi::SampleCount::Count1;
            depthTexInfo.mipLevels = 1;
            depthTexInfo.size = {64, 64, 1};
            auto depthTex = ctx->device->createTexture(depthTexInfo);
            REQUIRE(depthTex != nullptr);

            snap::rhi::FramebufferCreateInfo fbInfo{};
            fbInfo.renderPass = renderPass.get();
            fbInfo.attachments = {colorTex.get(), depthTex.get()};
            fbInfo.width = 64;
            fbInfo.height = 64;
            auto framebuffer = ctx->device->createFramebuffer(fbInfo);
            REQUIRE(framebuffer != nullptr);

            // Pipeline with depth test enabled (Less)
            snap::rhi::RenderPipelineCreateInfo pipeInfo{};
            pipeInfo.stages = {shaders->vertexShader.get(), shaders->fragmentShader.get()};
            pipeInfo.pipelineLayout = pipelineLayout.get();
            pipeInfo.renderPass = renderPass.get();
            pipeInfo.subpass = 0;
            pipeInfo.vertexInputState.attributesCount = 2;
            pipeInfo.vertexInputState.attributeDescription[0] = {
                .location = 0, .binding = 0, .offset = 0, .format = snap::rhi::VertexAttributeFormat::Float2};
            pipeInfo.vertexInputState.attributeDescription[1] = {
                .location = 1, .binding = 0, .offset = 8, .format = snap::rhi::VertexAttributeFormat::Float4};
            pipeInfo.vertexInputState.bindingsCount = 1;
            pipeInfo.vertexInputState.bindingDescription[0] = {.binding = 0, .stride = 24};
            pipeInfo.rasterizationState.rasterizationEnabled = true;
            pipeInfo.rasterizationState.cullMode = snap::rhi::CullMode::None;
            pipeInfo.colorBlendState.colorAttachmentsCount = 1;
            pipeInfo.colorBlendState.colorAttachmentsBlendState[0].colorWriteMask = snap::rhi::ColorMask::All;
            pipeInfo.depthStencilState.depthTest = true;
            pipeInfo.depthStencilState.depthWrite = true;
            pipeInfo.depthStencilState.depthFunc = snap::rhi::CompareFunction::Less;

            if (shaders->glPipelineInfo) pipeInfo.glRenderPipelineInfo = shaders->glPipelineInfo;
            if (shaders->mtlRenderPipelineInfo) pipeInfo.mtlRenderPipelineInfo = shaders->mtlRenderPipelineInfo;

            auto pipeline = ctx->device->createRenderPipeline(pipeInfo);
            REQUIRE(pipeline != nullptr);

            // UBO + descriptor set
            snap::rhi::BufferCreateInfo ubInfo{};
            ubInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
            ubInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            ubInfo.size = 16;
            auto uniformBuffer = ctx->device->createBuffer(ubInfo);
            { float ubo[] = {1,1,1,1}; auto* m = uniformBuffer->map(snap::rhi::MemoryAccess::Write, 0, 16); std::memcpy(m, ubo, 16); uniformBuffer->unmap(); }

            snap::rhi::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.maxSets = 1;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] = 1;
            auto pool = ctx->device->createDescriptorPool(poolInfo);
            snap::rhi::DescriptorSetCreateInfo dsci{}; dsci.descriptorPool = pool.get(); dsci.descriptorSetLayout = dsLayout.get();
            auto ds = ctx->device->createDescriptorSet(dsci);
            ds->bindUniformBuffer(0, uniformBuffer.get());

            // Vertex buffer: two full-screen quads at different depths
            // First (green) at z=0.0 (near), second (red) at z=0.5 (farther)
            // With depth test Less, green (drawn first) should win where they overlap.
            snap::rhi::BufferCreateInfo vbInfo{};
            vbInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
            vbInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            vbInfo.size = 12 * 24;
            auto vertexBuffer = ctx->device->createBuffer(vbInfo);
            {
                float verts[] = {
                    // Green quad at z=0.0 (near plane — this should win)
                    -1, -1,  0, 1, 0, 1,   1, -1,  0, 1, 0, 1,   1,  1,  0, 1, 0, 1,
                    -1, -1,  0, 1, 0, 1,   1,  1,  0, 1, 0, 1,  -1,  1,  0, 1, 0, 1,
                    // Red quad at z=0.5 (farther — should be occluded)
                    -1, -1,  1, 0, 0, 1,   1, -1,  1, 0, 0, 1,   1,  1,  1, 0, 0, 1,
                    -1, -1,  1, 0, 0, 1,   1,  1,  1, 0, 0, 1,  -1,  1,  1, 0, 0, 1,
                };
                auto* m = vertexBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(verts));
                std::memcpy(m, verts, sizeof(verts));
                vertexBuffer->unmap();
            }

            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            auto* enc = cmdBuf->getRenderCommandEncoder();

            snap::rhi::RenderPassBeginInfo rpBegin{};
            rpBegin.renderPass = renderPass.get();
            rpBegin.framebuffer = framebuffer.get();
            snap::rhi::ClearValue colorClear{};
            colorClear.color.float32 = {0, 0, 0, 1};
            snap::rhi::ClearValue depthClear{};
            depthClear.depthStencil = {1.0f, 0};
            rpBegin.clearValues.push_back(colorClear);
            rpBegin.clearValues.push_back(depthClear);

            enc->beginEncoding(rpBegin);
            enc->setViewport({0, 0, 64, 64, 0.0f, 1.0f});
            enc->bindRenderPipeline(pipeline.get());
            enc->bindVertexBuffer(0, vertexBuffer.get(), 0);
            enc->bindDescriptorSet(0, ds.get(), {});
            enc->draw(12, 0, 1); // draw both quads
            enc->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

            auto pixels = test_harness::readbackTexture(
                ctx->device.get(), ctx->commandQueue, colorTex.get(), 64, 64);
            REQUIRE(pixels.size() == 64u * 64u);

            // The green quad was drawn first at z=0, the red quad at z=0.5.
            // With depth Less, the green should be visible (the red is farther).
            auto center = test_harness::getPixel(pixels, 64, 32, 32);
            CHECK(center.g > 200); // should be green
            CHECK(center.r < 50);  // should NOT be red

            ctx->commandQueue->waitIdle();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Back-face culling — CW triangle with CCW winding should be culled
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("RenderCommandEncoder — Back-face culling", "[api][rendering][culling]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {

        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        auto shaders = test_harness::createPassthroughShaders(ctx->device.get());
        if (!shaders) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            // DS layout + pipeline layout
            snap::rhi::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
            dsLayoutInfo.bindings.push_back({
                .binding = 0,
                .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
            });
            auto dsLayout = ctx->device->createDescriptorSetLayout(dsLayoutInfo);
            snap::rhi::DescriptorSetLayout* dslayouts[] = {dsLayout.get()};
            snap::rhi::PipelineLayoutCreateInfo plInfo{};
            plInfo.setLayouts = dslayouts;
            auto pipelineLayout = ctx->device->createPipelineLayout(plInfo);

            // Render pass + RT
            snap::rhi::RenderPassCreateInfo rpInfo{};
            rpInfo.attachments[0].format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            rpInfo.attachments[0].samples = snap::rhi::SampleCount::Count1;
            rpInfo.attachments[0].loadOp = snap::rhi::AttachmentLoadOp::Clear;
            rpInfo.attachments[0].storeOp = snap::rhi::AttachmentStoreOp::Store;
            rpInfo.attachmentCount = 1;
            rpInfo.subpasses[0].colorAttachments[0] = {.attachment = 0};
            rpInfo.subpasses[0].colorAttachmentCount = 1;
            rpInfo.subpassCount = 1;
            auto renderPass = ctx->device->createRenderPass(rpInfo);

            snap::rhi::TextureCreateInfo texInfo{};
            texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            texInfo.textureType = snap::rhi::TextureType::Texture2D;
            texInfo.textureUsage = snap::rhi::TextureUsage::ColorAttachment |
                                   snap::rhi::TextureUsage::TransferSrc |
                                   snap::rhi::TextureUsage::Sampled;
            texInfo.sampleCount = snap::rhi::SampleCount::Count1;
            texInfo.mipLevels = 1;
            texInfo.size = {64, 64, 1};
            auto renderTarget = ctx->device->createTexture(texInfo);

            snap::rhi::FramebufferCreateInfo fbInfo{};
            fbInfo.renderPass = renderPass.get();
            fbInfo.attachments = {renderTarget.get()};
            fbInfo.width = 64;
            fbInfo.height = 64;
            auto framebuffer = ctx->device->createFramebuffer(fbInfo);

            // Pipeline with back-face culling (CCW front, cull Back)
            snap::rhi::RenderPipelineCreateInfo pipeInfo{};
            pipeInfo.stages = {shaders->vertexShader.get(), shaders->fragmentShader.get()};
            pipeInfo.pipelineLayout = pipelineLayout.get();
            pipeInfo.renderPass = renderPass.get();
            pipeInfo.subpass = 0;
            pipeInfo.vertexInputState.attributesCount = 2;
            pipeInfo.vertexInputState.attributeDescription[0] = {
                .location = 0, .binding = 0, .offset = 0, .format = snap::rhi::VertexAttributeFormat::Float2};
            pipeInfo.vertexInputState.attributeDescription[1] = {
                .location = 1, .binding = 0, .offset = 8, .format = snap::rhi::VertexAttributeFormat::Float4};
            pipeInfo.vertexInputState.bindingsCount = 1;
            pipeInfo.vertexInputState.bindingDescription[0] = {.binding = 0, .stride = 24};
            pipeInfo.rasterizationState.rasterizationEnabled = true;
            pipeInfo.rasterizationState.cullMode = snap::rhi::CullMode::Back;
            pipeInfo.rasterizationState.windingMode = snap::rhi::Winding::CCW;
            pipeInfo.colorBlendState.colorAttachmentsCount = 1;
            pipeInfo.colorBlendState.colorAttachmentsBlendState[0].colorWriteMask = snap::rhi::ColorMask::All;

            if (shaders->glPipelineInfo) pipeInfo.glRenderPipelineInfo = shaders->glPipelineInfo;
            if (shaders->mtlRenderPipelineInfo) pipeInfo.mtlRenderPipelineInfo = shaders->mtlRenderPipelineInfo;

            auto pipeline = ctx->device->createRenderPipeline(pipeInfo);
            REQUIRE(pipeline != nullptr);

            // UBO + DS
            snap::rhi::BufferCreateInfo ubInfo{};
            ubInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
            ubInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            ubInfo.size = 16;
            auto ubo = ctx->device->createBuffer(ubInfo);
            {
                float d[] = {1,1,1,1};
                auto* m = ubo->map(snap::rhi::MemoryAccess::Write,0,16);
                std::memcpy(m,d,16);
                ubo->unmap();
            }

            snap::rhi::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.maxSets = 1;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] = 1;
            auto pool = ctx->device->createDescriptorPool(poolInfo);
            snap::rhi::DescriptorSetCreateInfo dsci{}; dsci.descriptorPool = pool.get(); dsci.descriptorSetLayout = dsLayout.get();
            auto ds = ctx->device->createDescriptorSet(dsci);
            ds->bindUniformBuffer(0, ubo.get());

            // Vertex buffer: a back-facing full-screen triangle (should be culled with CCW front-face + Back cull).
            // In Vulkan the clip-space Y-axis is inverted compared to OpenGL/Metal, which flips
            // the apparent winding order of triangles. We therefore use opposite vertex winding
            // for Vulkan so the triangle is back-facing (CW in screen-space) in all APIs.
            const bool isVulkan = api == snap::rhi::API::Vulkan;
            snap::rhi::BufferCreateInfo vbInfo{};
            vbInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
            vbInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            vbInfo.size = 3 * 24;
            auto vertexBuffer = ctx->device->createBuffer(vbInfo);
            {
                // OpenGL/Metal: CW in screen-space → (-1,-1), (0,1), (1,-1)
                // Vulkan:       CCW in clip-space becomes CW after Y-flip → (-1,-1), (1,-1), (0,1)
                float verts_ogl[] = {
                    -1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                     0.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                     1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                };
                float verts_vk[] = {
                    -1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                     1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                     0.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                };
                const float* verts = isVulkan ? verts_vk : verts_ogl;
                auto* m = vertexBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(verts_ogl));
                std::memcpy(m, verts, sizeof(verts_ogl));
                vertexBuffer->unmap();
            }

            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            auto* enc = cmdBuf->getRenderCommandEncoder();

            snap::rhi::RenderPassBeginInfo rpBegin{};
            rpBegin.renderPass = renderPass.get();
            rpBegin.framebuffer = framebuffer.get();
            snap::rhi::ClearValue cv{};
            cv.color.float32 = {0.0f, 0.0f, 1.0f, 1.0f}; // Clear to blue
            rpBegin.clearValues.push_back(cv);

            enc->beginEncoding(rpBegin);
            enc->setViewport({0, 0, 64, 64, 0.0f, 1.0f});
            enc->bindRenderPipeline(pipeline.get());
            enc->bindVertexBuffer(0, vertexBuffer.get(), 0);
            enc->bindDescriptorSet(0, ds.get(), {});
            enc->draw(3, 0, 1);
            enc->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

            auto pixels = test_harness::readbackTexture(
                ctx->device.get(), ctx->commandQueue, renderTarget.get(), 64, 64);
            REQUIRE(pixels.size() == 64u * 64u);

            // The CW-wound triangle should have been culled.
            // All pixels should remain the clear color (blue).
            auto center = test_harness::getPixel(pixels, 64, 32, 32);
            test_harness::Pixel blue{0, 0, 255, 255};
            CHECK(test_harness::pixelApproxEqual(center, blue, 5));
            // Entire image should be the clear color
            float blueCoverage = test_harness::colorCoverage(pixels, blue, 5);
            CHECK(blueCoverage > 0.99f);

            ctx->commandQueue->waitIdle();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Viewport clipping — draw a full-screen quad but with a small viewport;
// only the viewport region should contain the triangle color.
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("RenderCommandEncoder — Viewport clipping", "[api][rendering][viewport]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {

        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            auto res = createRenderResources(ctx->device.get(), *ctx);
            if (!res) {
                WARN("Skipping viewport clipping test for " << ctx->apiName);
                break;
            }

            // Use a full-screen quad vertex buffer (6 verts)
            snap::rhi::BufferCreateInfo vbInfo{};
            vbInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
            vbInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            vbInfo.size = 6 * 24;
            auto fullscreenVB = ctx->device->createBuffer(vbInfo);
            {
                float verts[] = {
                    -1, -1,  0, 1, 0, 1,   1, -1,  0, 1, 0, 1,   1,  1,  0, 1, 0, 1,
                    -1, -1,  0, 1, 0, 1,   1,  1,  0, 1, 0, 1,  -1,  1,  0, 1, 0, 1,
                };
                auto* m = fullscreenVB->map(snap::rhi::MemoryAccess::Write, 0, sizeof(verts));
                std::memcpy(m, verts, sizeof(verts));
                fullscreenVB->unmap();
            }

            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            auto* enc = cmdBuf->getRenderCommandEncoder();

            snap::rhi::RenderPassBeginInfo rpBegin{};
            rpBegin.renderPass = res->renderPass.get();
            rpBegin.framebuffer = res->framebuffer.get();
            snap::rhi::ClearValue cv{};
            cv.color.float32 = {0.0f, 0.0f, 0.0f, 1.0f}; // Clear to black
            rpBegin.clearValues.push_back(cv);

            enc->beginEncoding(rpBegin);
            // Small viewport: only top-left 32×32 of the 64×64 RT
            enc->setViewport({0, 0, 32, 32, 0.0f, 1.0f});
            enc->bindRenderPipeline(res->pipeline.get());
            enc->bindVertexBuffer(0, fullscreenVB.get(), 0);
            enc->bindDescriptorSet(0, res->descriptorSet.get(), {});
            enc->draw(6, 0, 1);
            enc->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

            auto pixels = test_harness::readbackTexture(
                ctx->device.get(), ctx->commandQueue, res->renderTarget.get(), 64, 64);
            REQUIRE(pixels.size() == 64u * 64u);

            // Pixel at (16,16) is inside the viewport — should be green
            auto insidePixel = test_harness::getPixel(pixels, 64, 16, 16);
            CHECK(insidePixel.g > 200);

            // Pixel at (48,48) is outside the viewport — should remain black (clear color)
            auto outsidePixel = test_harness::getPixel(pixels, 64, 48, 48);
            test_harness::Pixel black{0, 0, 0, 255};
            CHECK(test_harness::pixelApproxEqual(outsidePixel, black, 5));

            ctx->commandQueue->waitIdle();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Color write mask — draw but only write to the red channel
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("RenderCommandEncoder — Color write mask", "[api][rendering][writemask]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {

        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        auto shaders = test_harness::createPassthroughShaders(ctx->device.get());
        if (!shaders) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            snap::rhi::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
            dsLayoutInfo.bindings.push_back({
                .binding = 0,
                .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
            });
            auto dsLayout = ctx->device->createDescriptorSetLayout(dsLayoutInfo);
            snap::rhi::DescriptorSetLayout* dslayouts[] = {dsLayout.get()};
            snap::rhi::PipelineLayoutCreateInfo plInfo{};
            plInfo.setLayouts = dslayouts;
            auto pipelineLayout = ctx->device->createPipelineLayout(plInfo);

            snap::rhi::RenderPassCreateInfo rpInfo{};
            rpInfo.attachments[0].format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            rpInfo.attachments[0].samples = snap::rhi::SampleCount::Count1;
            rpInfo.attachments[0].loadOp = snap::rhi::AttachmentLoadOp::Clear;
            rpInfo.attachments[0].storeOp = snap::rhi::AttachmentStoreOp::Store;
            rpInfo.attachmentCount = 1;
            rpInfo.subpasses[0].colorAttachments[0] = {.attachment = 0};
            rpInfo.subpasses[0].colorAttachmentCount = 1;
            rpInfo.subpassCount = 1;
            auto renderPass = ctx->device->createRenderPass(rpInfo);

            snap::rhi::TextureCreateInfo texInfo{};
            texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            texInfo.textureType = snap::rhi::TextureType::Texture2D;
            texInfo.textureUsage = snap::rhi::TextureUsage::ColorAttachment |
                                   snap::rhi::TextureUsage::TransferSrc |
                                   snap::rhi::TextureUsage::Sampled;
            texInfo.sampleCount = snap::rhi::SampleCount::Count1;
            texInfo.mipLevels = 1;
            texInfo.size = {64, 64, 1};
            auto renderTarget = ctx->device->createTexture(texInfo);

            snap::rhi::FramebufferCreateInfo fbInfo{};
            fbInfo.renderPass = renderPass.get();
            fbInfo.attachments = {renderTarget.get()};
            fbInfo.width = 64;
            fbInfo.height = 64;
            auto framebuffer = ctx->device->createFramebuffer(fbInfo);

            // Pipeline: only write Red channel
            snap::rhi::RenderPipelineCreateInfo pipeInfo{};
            pipeInfo.stages = {shaders->vertexShader.get(), shaders->fragmentShader.get()};
            pipeInfo.pipelineLayout = pipelineLayout.get();
            pipeInfo.renderPass = renderPass.get();
            pipeInfo.subpass = 0;
            pipeInfo.vertexInputState.attributesCount = 2;
            pipeInfo.vertexInputState.attributeDescription[0] = {
                .location = 0, .binding = 0, .offset = 0, .format = snap::rhi::VertexAttributeFormat::Float2};
            pipeInfo.vertexInputState.attributeDescription[1] = {
                .location = 1, .binding = 0, .offset = 8, .format = snap::rhi::VertexAttributeFormat::Float4};
            pipeInfo.vertexInputState.bindingsCount = 1;
            pipeInfo.vertexInputState.bindingDescription[0] = {.binding = 0, .stride = 24};
            pipeInfo.rasterizationState.rasterizationEnabled = true;
            pipeInfo.rasterizationState.cullMode = snap::rhi::CullMode::None;
            pipeInfo.colorBlendState.colorAttachmentsCount = 1;
            pipeInfo.colorBlendState.colorAttachmentsBlendState[0].colorWriteMask =
                snap::rhi::ColorMask::R; // only red

            if (shaders->glPipelineInfo) pipeInfo.glRenderPipelineInfo = shaders->glPipelineInfo;
            if (shaders->mtlRenderPipelineInfo) pipeInfo.mtlRenderPipelineInfo = shaders->mtlRenderPipelineInfo;

            auto pipeline = ctx->device->createRenderPipeline(pipeInfo);
            REQUIRE(pipeline != nullptr);

            // UBO + DS
            snap::rhi::BufferCreateInfo ubInfo{};
            ubInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
            ubInfo.memoryProperties = snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            ubInfo.size = 16;
            auto ubo = ctx->device->createBuffer(ubInfo);
            { float d[] = {1,1,1,1}; auto* m = ubo->map(snap::rhi::MemoryAccess::Write,0,16); std::memcpy(m,d,16); ubo->unmap(); }

            snap::rhi::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.maxSets = 1;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] = 1;
            auto pool = ctx->device->createDescriptorPool(poolInfo);
            snap::rhi::DescriptorSetCreateInfo dsci{}; dsci.descriptorPool = pool.get(); dsci.descriptorSetLayout = dsLayout.get();
            auto ds = ctx->device->createDescriptorSet(dsci);
            ds->bindUniformBuffer(0, ubo.get());

            // Full-screen white quad
            snap::rhi::BufferCreateInfo vbInfo{};
            vbInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
            vbInfo.memoryProperties = snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            vbInfo.size = 6 * 24;
            auto vb = ctx->device->createBuffer(vbInfo);
            {
                float v[] = {
                    -1,-1, 1,1,1,1,  1,-1, 1,1,1,1,  1,1, 1,1,1,1,
                    -1,-1, 1,1,1,1,  1,1, 1,1,1,1,  -1,1, 1,1,1,1,
                };
                auto* m = vb->map(snap::rhi::MemoryAccess::Write, 0, sizeof(v));
                std::memcpy(m, v, sizeof(v));
                vb->unmap();
            }

            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            auto* enc = cmdBuf->getRenderCommandEncoder();

            snap::rhi::RenderPassBeginInfo rpBegin{};
            rpBegin.renderPass = renderPass.get();
            rpBegin.framebuffer = framebuffer.get();
            snap::rhi::ClearValue cv{};
            cv.color.float32 = {0, 0, 0, 0}; // Clear to transparent black
            rpBegin.clearValues.push_back(cv);

            enc->beginEncoding(rpBegin);
            enc->setViewport({0, 0, 64, 64, 0.0f, 1.0f});
            enc->bindRenderPipeline(pipeline.get());
            enc->bindVertexBuffer(0, vb.get(), 0);
            enc->bindDescriptorSet(0, ds.get(), {});
            enc->draw(6, 0, 1);
            enc->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

            auto pixels = test_harness::readbackTexture(
                ctx->device.get(), ctx->commandQueue, renderTarget.get(), 64, 64);
            REQUIRE(pixels.size() == 64u * 64u);

            // The white quad was drawn but only R channel was written.
            // Result: R=255, G=0 (masked), B=0 (masked), A=0 (masked, clear was 0)
            auto center = test_harness::getPixel(pixels, 64, 32, 32);
            CHECK(center.r > 200);
            CHECK(center.g < 10);
            CHECK(center.b < 10);
            CHECK(center.a < 10);

            ctx->commandQueue->waitIdle();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// MSAA — render a triangle to a 4× multi-sampled target, resolve to a
// single-sample texture, and verify the output contains the triangle.
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("RenderCommandEncoder — MSAA rendering", "[api][rendering][msaa]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {

        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        // Check that 4× MSAA is supported for RGBA8 as a framebuffer attachment
        const auto& caps = ctx->device->getCapabilities();
        const auto formatIdx = static_cast<uint32_t>(snap::rhi::PixelFormat::R8G8B8A8Unorm);
        if (caps.formatProperties[formatIdx].framebufferSampleCounts < snap::rhi::SampleCount::Count4) continue;

        auto shaders = test_harness::createPassthroughShaders(ctx->device.get());
        if (!shaders) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            // DS layout + pipeline layout
            snap::rhi::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
            dsLayoutInfo.bindings.push_back({
                .binding = 0,
                .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
            });
            auto dsLayout = ctx->device->createDescriptorSetLayout(dsLayoutInfo);
            snap::rhi::DescriptorSetLayout* layouts[] = {dsLayout.get()};
            snap::rhi::PipelineLayoutCreateInfo plInfo{};
            plInfo.setLayouts = layouts;
            auto pipelineLayout = ctx->device->createPipelineLayout(plInfo);

            // Render pass with MSAA resolve
            snap::rhi::RenderPassCreateInfo rpInfo{};
            rpInfo.attachments[0].format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            rpInfo.attachments[0].samples = snap::rhi::SampleCount::Count4;
            rpInfo.attachments[0].loadOp = snap::rhi::AttachmentLoadOp::Clear;
            rpInfo.attachments[0].storeOp = snap::rhi::AttachmentStoreOp::DontCare;
            rpInfo.attachments[1].format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            rpInfo.attachments[1].samples = snap::rhi::SampleCount::Count1;
            rpInfo.attachments[1].loadOp = snap::rhi::AttachmentLoadOp::DontCare;
            rpInfo.attachments[1].storeOp = snap::rhi::AttachmentStoreOp::Store;
            rpInfo.attachmentCount = 2;
            rpInfo.subpasses[0].colorAttachments[0] = {.attachment = 0};
            rpInfo.subpasses[0].resolveAttachments[0] = {.attachment = 1};
            rpInfo.subpasses[0].colorAttachmentCount = 1;
            rpInfo.subpasses[0].resolveAttachmentCount = 1;
            rpInfo.subpassCount = 1;
            auto renderPass = ctx->device->createRenderPass(rpInfo);
            REQUIRE(renderPass != nullptr);

            // MSAA render target (4× samples)
            snap::rhi::TextureCreateInfo msaaTexInfo{};
            msaaTexInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            msaaTexInfo.textureType = snap::rhi::TextureType::Texture2D;
            msaaTexInfo.textureUsage = snap::rhi::TextureUsage::ColorAttachment;
            msaaTexInfo.sampleCount = snap::rhi::SampleCount::Count4;
            msaaTexInfo.mipLevels = 1;
            msaaTexInfo.size = {64, 64, 1};
            auto msaaTarget = ctx->device->createTexture(msaaTexInfo);
            REQUIRE(msaaTarget != nullptr);

            // Resolve target (1× sample)
            snap::rhi::TextureCreateInfo resolveTexInfo{};
            resolveTexInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            resolveTexInfo.textureType = snap::rhi::TextureType::Texture2D;
            resolveTexInfo.textureUsage = snap::rhi::TextureUsage::ColorAttachment |
                                          snap::rhi::TextureUsage::TransferSrc |
                                          snap::rhi::TextureUsage::Sampled;
            resolveTexInfo.sampleCount = snap::rhi::SampleCount::Count1;
            resolveTexInfo.mipLevels = 1;
            resolveTexInfo.size = {64, 64, 1};
            auto resolveTarget = ctx->device->createTexture(resolveTexInfo);
            REQUIRE(resolveTarget != nullptr);

            // Framebuffer with both MSAA and resolve attachments
            snap::rhi::FramebufferCreateInfo fbInfo{};
            fbInfo.renderPass = renderPass.get();
            fbInfo.attachments = {msaaTarget.get(), resolveTarget.get()};
            fbInfo.width = 64;
            fbInfo.height = 64;
            auto framebuffer = ctx->device->createFramebuffer(fbInfo);
            REQUIRE(framebuffer != nullptr);

            // Pipeline for MSAA
            snap::rhi::RenderPipelineCreateInfo pipeInfo{};
            pipeInfo.stages = {shaders->vertexShader.get(), shaders->fragmentShader.get()};
            pipeInfo.pipelineLayout = pipelineLayout.get();
            pipeInfo.renderPass = renderPass.get();
            pipeInfo.subpass = 0;
            pipeInfo.multisampleState.samples = snap::rhi::SampleCount::Count4;
            pipeInfo.vertexInputState.attributesCount = 2;
            pipeInfo.vertexInputState.attributeDescription[0] = {
                .location = 0, .binding = 0, .offset = 0, .format = snap::rhi::VertexAttributeFormat::Float2};
            pipeInfo.vertexInputState.attributeDescription[1] = {
                .location = 1, .binding = 0, .offset = 8, .format = snap::rhi::VertexAttributeFormat::Float4};
            pipeInfo.vertexInputState.bindingsCount = 1;
            pipeInfo.vertexInputState.bindingDescription[0] = {.binding = 0, .stride = 24};
            pipeInfo.rasterizationState.rasterizationEnabled = true;
            pipeInfo.rasterizationState.cullMode = snap::rhi::CullMode::None;
            pipeInfo.colorBlendState.colorAttachmentsCount = 1;
            pipeInfo.colorBlendState.colorAttachmentsBlendState[0].colorWriteMask = snap::rhi::ColorMask::All;
            if (shaders->glPipelineInfo) pipeInfo.glRenderPipelineInfo = shaders->glPipelineInfo;
            if (shaders->mtlRenderPipelineInfo) pipeInfo.mtlRenderPipelineInfo = shaders->mtlRenderPipelineInfo;
            auto pipeline = ctx->device->createRenderPipeline(pipeInfo);
            REQUIRE(pipeline != nullptr);

            // Vertex buffer (same triangle as other tests)
            snap::rhi::BufferCreateInfo vbInfo{};
            vbInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
            vbInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            vbInfo.size = 3 * 24;
            auto vertexBuffer = ctx->device->createBuffer(vbInfo);
            {
                float verts[] = {
                    0.0f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f,
                   -0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f,
                    0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
                };
                auto* m = vertexBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(verts));
                std::memcpy(m, verts, sizeof(verts));
                vertexBuffer->unmap();
            }

            // UBO
            snap::rhi::BufferCreateInfo ubInfo{};
            ubInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
            ubInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            ubInfo.size = 16;
            auto ubo = ctx->device->createBuffer(ubInfo);
            { float d[] = {1,1,1,1}; auto* m = ubo->map(snap::rhi::MemoryAccess::Write,0,16); std::memcpy(m,d,16); ubo->unmap(); }

            snap::rhi::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.maxSets = 1;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] = 1;
            auto pool = ctx->device->createDescriptorPool(poolInfo);
            snap::rhi::DescriptorSetCreateInfo dsci{};
            dsci.descriptorPool = pool.get();
            dsci.descriptorSetLayout = dsLayout.get();
            auto ds = ctx->device->createDescriptorSet(dsci);
            ds->bindUniformBuffer(0, ubo.get());

            // Draw
            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            auto* enc = cmdBuf->getRenderCommandEncoder();

            snap::rhi::RenderPassBeginInfo rpBegin{};
            rpBegin.renderPass = renderPass.get();
            rpBegin.framebuffer = framebuffer.get();
            snap::rhi::ClearValue cv{};
            cv.color.float32 = {0, 0, 0, 1};
            rpBegin.clearValues.push_back(cv);
            rpBegin.clearValues.push_back(cv); // resolve attachment clear

            enc->beginEncoding(rpBegin);
            enc->setViewport({0, 0, 64, 64, 0, 1});
            enc->bindRenderPipeline(pipeline.get());
            enc->bindVertexBuffer(0, vertexBuffer.get(), 0);
            enc->bindDescriptorSet(0, ds.get(), {});
            enc->draw(3, 0, 1);
            enc->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

            // Readback the resolved (single-sample) target
            auto pixels = test_harness::readbackTexture(
                ctx->device.get(), ctx->commandQueue, resolveTarget.get(), 64, 64);
            REQUIRE(pixels.size() == 64u * 64u);

            // Corner should be clear color
            test_harness::Pixel black{0, 0, 0, 255};
            CHECK(test_harness::pixelApproxEqual(
                test_harness::getPixel(pixels, 64, 0, 0), black, 5));

            // Triangle must have produced non-black pixels
            CHECK(test_harness::hasNonBlackPixel(pixels));

            // MSAA edge quality: sample edge pixels near the triangle boundary.
            // With 4× MSAA, edge pixels should have anti-aliased (intermediate) values
            // rather than hard on/off transitions. Check that at least one pixel along
            // the triangle edge has a channel value between 20 and 235 (partial coverage).
            bool foundAntiAliasedPixel = false;
            // Scan a horizontal line through the middle of the render target
            for (uint32_t x = 0; x < 64; ++x) {
                auto p = test_harness::getPixel(pixels, 64, x, 32);
                // A partially-covered pixel will have non-zero non-max color
                if ((p.r > 20 && p.r < 235) || (p.g > 20 && p.g < 235) || (p.b > 20 && p.b < 235)) {
                    foundAntiAliasedPixel = true;
                    break;
                }
            }
            CHECK(foundAntiAliasedPixel);

            ctx->commandQueue->waitIdle();
        }
    }
}

#endif
