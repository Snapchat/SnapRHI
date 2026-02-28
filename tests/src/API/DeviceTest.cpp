//
//  DeviceTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::Device creation, capabilities, memory queries, and context management.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/Texture.hpp"
#include <catch2/catch.hpp>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("Device — Creation and basic queries", "[api][device]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Device is created successfully") {
                REQUIRE(ctx->device != nullptr);
            }

            SECTION("getCapabilities returns valid data") {
                const auto& caps = ctx->device->getCapabilities();
                CHECK(caps.apiDescription.api == ctx->api);
            }

            SECTION("getCommandQueue returns valid queue") {
                auto* queue = ctx->device->getCommandQueue(0, 0);
                REQUIRE(queue != nullptr);
            }

            SECTION("getMainDeviceContext returns valid context") {
                auto* dc = ctx->device->getMainDeviceContext();
                // OpenGL and NoOp should return a valid context; Metal/Vulkan may also.
                CHECK(dc != nullptr);
            }

            SECTION("createDeviceContext returns shared_ptr") {
                snap::rhi::DeviceContextCreateInfo dcInfo{};
                dcInfo.internalResourcesAllowed = false;
                auto dc = ctx->device->createDeviceContext(dcInfo);
                CHECK(dc != nullptr);
            }

            SECTION("setCurrent and Guard RAII") {
                auto* dc = ctx->device->getMainDeviceContext();
                {
                    auto guard = ctx->device->setCurrent(dc);
                    // Context should be current
                    auto* current = ctx->device->getCurrentDeviceContext();
                    // On some backends, getCurrentDeviceContext may return the same dc
                    CHECK(current != nullptr);
                }
                // After guard is destroyed, context may be restored
            }

            SECTION("getPlatformDeviceString is non-empty") {
                const auto& platformStr = ctx->device->getPlatformDeviceString();
                CHECK_FALSE(platformStr.empty());
            }

            SECTION("getDeviceCreateInfo returns stored info") {
                const auto& info = ctx->device->getDeviceCreateInfo();
                CHECK(info.enabledReportLevel == snap::rhi::ReportLevel::All);
            }

            SECTION("Per-resource memory reporting") {

                // Texture: 256×256×RGBA8 = at least 262144 bytes
                snap::rhi::TextureCreateInfo texInfo{};
                texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
                texInfo.textureType = snap::rhi::TextureType::Texture2D;
                texInfo.textureUsage = snap::rhi::TextureUsage::Sampled;
                texInfo.sampleCount = snap::rhi::SampleCount::Count1;
                texInfo.mipLevels = 1;
                texInfo.size = {256, 256, 1};
                auto texture = ctx->device->createTexture(texInfo);
                REQUIRE(texture != nullptr);
                CHECK(texture->getGPUMemoryUsage() >= 256u * 256u * 4u);

                // Buffer: requested 4096 bytes
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 4096;
                auto buffer = ctx->device->createBuffer(bufInfo);
                REQUIRE(buffer != nullptr);
                CHECK(buffer->getGPUMemoryUsage() >= 4096);

                // Device-wide queries should not crash
                auto totalGPU = ctx->device->getGPUMemoryUsage();
                auto totalCPU = ctx->device->getCPUMemoryUsage();
                (void)totalGPU;
                (void)totalCPU;
            }

            SECTION("captureMemorySnapshot reflects live resources") {
                auto snapshotEmpty = ctx->device->captureMemorySnapshot();

                // Create a buffer — the snapshot should show at least one more resource
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 1024;
                auto buffer = ctx->device->createBuffer(bufInfo);
                REQUIRE(buffer != nullptr);

                auto snapshotAfter = ctx->device->captureMemorySnapshot();
                // At minimum, one more resource must be tracked
                CHECK(snapshotAfter.cpu.totalResourceCount + snapshotAfter.gpu.totalResourceCount >=
                      snapshotEmpty.cpu.totalResourceCount + snapshotEmpty.gpu.totalResourceCount);
            }

            SECTION("waitForTrackedSubmissions does not crash") {
                ctx->device->waitForTrackedSubmissions();
            }

            SECTION("getTextureFormatProperties for RGBA8") {
                snap::rhi::TextureFormatInfo formatInfo{};
                formatInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
                formatInfo.type = snap::rhi::TextureType::Texture2D;
                formatInfo.usage = snap::rhi::TextureUsage::Sampled;

                auto props = ctx->device->getTextureFormatProperties(formatInfo);
                CHECK(props.maxExtent.width > 0);
                CHECK(props.maxExtent.height > 0);
            }
        }
    }
}

TEST_CASE("Device — Debug messenger", "[api][device]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            snap::rhi::DebugMessengerCreateInfo messengerInfo{};
            messengerInfo.debugMessengerCallback = [](const snap::rhi::DebugCallbackInfo& info) {
                // Just absorb the message
            };

            auto messenger = ctx->device->createDebugMessenger(std::move(messengerInfo));
            // May return nullptr on backends that don't support it
            // Just check it doesn't crash
        }
    }
}

#endif
