//
//  RenderPipelineTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::RenderPipeline — shader creation, pipeline creation, render pass and framebuffer.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "TestHarness/ShaderHelper.h"
#include "snap/rhi/DescriptorPool.hpp"
#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/DescriptorSetLayout.hpp"
#include "snap/rhi/Framebuffer.hpp"
#include "snap/rhi/PipelineLayout.hpp"
#include "snap/rhi/RenderPass.hpp"
#include "snap/rhi/RenderPipeline.hpp"
#include "snap/rhi/ShaderLibrary.hpp"
#include "snap/rhi/ShaderModule.hpp"
#include "snap/rhi/Texture.hpp"
#include <catch2/catch.hpp>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("RenderPipeline — Shader library and module creation", "[api][rendering][shader]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            auto shaders = test_harness::createPassthroughShaders(ctx->device.get());
            if (!shaders) {
                // Backend may not support runtime shader compilation (e.g., Vulkan without SPIR-V)
                WARN("Skipping shader test for " << ctx->apiName);
                continue;
            }

            SECTION("Shader library created successfully") {
                CHECK(shaders->library != nullptr);
            }

            SECTION("Vertex shader module created") {
                CHECK(shaders->vertexShader != nullptr);
            }

            SECTION("Fragment shader module created") {
                CHECK(shaders->fragmentShader != nullptr);
            }
        }
    }
}

TEST_CASE("RenderPipeline — RenderPass and Framebuffer creation", "[api][rendering]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Create render pass with single color attachment") {
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
                REQUIRE(renderPass != nullptr);
            }

            SECTION("Create framebuffer") {
                // Create render pass
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
                REQUIRE(renderPass != nullptr);

                // Create render target texture
                snap::rhi::TextureCreateInfo texInfo{};
                texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
                texInfo.textureType = snap::rhi::TextureType::Texture2D;
                texInfo.textureUsage = snap::rhi::TextureUsage::ColorAttachment | snap::rhi::TextureUsage::Sampled;
                texInfo.sampleCount = snap::rhi::SampleCount::Count1;
                texInfo.mipLevels = 1;
                texInfo.size = {64, 64, 1};

                auto texture = ctx->device->createTexture(texInfo);
                REQUIRE(texture != nullptr);

                // Create framebuffer
                snap::rhi::FramebufferCreateInfo fbInfo{};
                fbInfo.renderPass = renderPass.get();
                fbInfo.attachments = {texture.get()};
                fbInfo.width = 64;
                fbInfo.height = 64;
                fbInfo.layers = 1;

                auto framebuffer = ctx->device->createFramebuffer(fbInfo);
                REQUIRE(framebuffer != nullptr);
            }
        }
    }
}

TEST_CASE("RenderPipeline — Full pipeline creation", "[api][rendering][pipeline]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        auto shaders = test_harness::createPassthroughShaders(ctx->device.get());
        if (!shaders) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            // Create descriptor set layout for UBO
            snap::rhi::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
            dsLayoutInfo.bindings.push_back({
                .binding = 0,
                .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
            });
            auto dsLayout = ctx->device->createDescriptorSetLayout(dsLayoutInfo);

            // Create pipeline layout
            snap::rhi::DescriptorSetLayout* layouts[] = {dsLayout.get()};
            snap::rhi::PipelineLayoutCreateInfo plInfo{};
            plInfo.setLayouts = layouts;
            auto pipelineLayout = ctx->device->createPipelineLayout(plInfo);

            // Create render pass
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

            // Create render pipeline
            snap::rhi::RenderPipelineCreateInfo pipeInfo{};
            pipeInfo.stages = {shaders->vertexShader.get(), shaders->fragmentShader.get()};
            pipeInfo.pipelineLayout = pipelineLayout.get();
            pipeInfo.renderPass = renderPass.get();
            pipeInfo.subpass = 0;

            // Vertex input state (pos: vec2, color: vec4)
            pipeInfo.vertexInputState.attributesCount = 2;
            pipeInfo.vertexInputState.attributeDescription[0] = {
                .location = 0,
                .binding = 0,
                .offset = 0,
                .format = snap::rhi::VertexAttributeFormat::Float2,
            };
            pipeInfo.vertexInputState.attributeDescription[1] = {
                .location = 1,
                .binding = 0,
                .offset = 8,
                .format = snap::rhi::VertexAttributeFormat::Float4,
            };
            pipeInfo.vertexInputState.bindingsCount = 1;
            pipeInfo.vertexInputState.bindingDescription[0] = {
                .binding = 0,
                .divisor = 1,
                .stride = 24, // vec2 + vec4 = 8 + 16 = 24
                .inputRate = snap::rhi::VertexInputRate::PerVertex,
            };

            // Rasterization
            pipeInfo.rasterizationState.rasterizationEnabled = true;
            pipeInfo.rasterizationState.cullMode = snap::rhi::CullMode::None;

            // Color blend
            pipeInfo.colorBlendState.colorAttachmentsCount = 1;
            pipeInfo.colorBlendState.colorAttachmentsBlendState[0].colorWriteMask =
                snap::rhi::ColorMask::R | snap::rhi::ColorMask::G |
                snap::rhi::ColorMask::B | snap::rhi::ColorMask::A;

            // GL-specific pipeline info
            if (shaders->glPipelineInfo) {
                pipeInfo.glRenderPipelineInfo = *shaders->glPipelineInfo;
            }
            // Metal-specific pipeline info
            if (shaders->mtlRenderPipelineInfo) {
                pipeInfo.mtlRenderPipelineInfo = *shaders->mtlRenderPipelineInfo;
            }

            auto pipeline = ctx->device->createRenderPipeline(pipeInfo);
            REQUIRE(pipeline != nullptr);
        }
    }
}

#endif
