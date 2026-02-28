//
//  CommandBufferTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::CommandBuffer and snap::rhi::CommandQueue.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "snap/rhi/BlitCommandEncoder.hpp"
#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/ComputeCommandEncoder.hpp"
#include "snap/rhi/RenderCommandEncoder.hpp"
#include <catch2/catch.hpp>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("CommandBuffer — Creation and encoder access", "[api][command-buffer]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Create command buffer") {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                REQUIRE(cmdBuf != nullptr);
            }

            SECTION("getRenderCommandEncoder returns non-null") {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* re = cmdBuf->getRenderCommandEncoder();
                CHECK(re != nullptr);
            }

            SECTION("getBlitCommandEncoder returns non-null") {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* blit = cmdBuf->getBlitCommandEncoder();
                CHECK(blit != nullptr);
            }

            SECTION("getComputeCommandEncoder returns non-null") {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* compute = cmdBuf->getComputeCommandEncoder();
                CHECK(compute != nullptr);
            }

            SECTION("getCommandQueue returns associated queue") {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                CHECK(cmdBuf->getCommandQueue() == ctx->commandQueue);
            }

            SECTION("Newly created command buffer has a defined initial status") {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto status = cmdBuf->getStatus();
                // Status must be one of the defined enum values (not undefined/garbage)
                CHECK(static_cast<uint32_t>(status) < static_cast<uint32_t>(snap::rhi::CommandBufferStatus::Count));
            }

            SECTION("Create with UnretainedResources flag") {
                snap::rhi::CommandBufferCreateInfo cbInfo{};
                cbInfo.commandQueue = ctx->commandQueue;
                cbInfo.commandBufferCreateFlags = snap::rhi::CommandBufferCreateFlags::UnretainedResources;
                auto cmdBuf = ctx->device->createCommandBuffer(cbInfo);
                REQUIRE(cmdBuf != nullptr);
            }
        }
    }
}

TEST_CASE("CommandQueue — waitIdle and waitUntilScheduled", "[api][command-queue]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("waitIdle does not crash") {
                ctx->commandQueue->waitIdle();
            }

            SECTION("waitUntilScheduled does not crash") {
                ctx->commandQueue->waitUntilScheduled();
            }

            SECTION("Submit and waitIdle") {
                // Create a small buffer so we have a real blit operation.
                // Metal debug layer asserts on empty blit encoders ("endEncoding without use").
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::TransferSrc | snap::rhi::BufferUsage::TransferDst;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 16;
                auto buf = ctx->device->createBuffer(bufInfo);

                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* blit = cmdBuf->getBlitCommandEncoder();
                blit->beginEncoding();
                snap::rhi::BufferCopy region{.srcOffset = 0, .dstOffset = 0, .size = 16};
                blit->copyBuffer(buf.get(), buf.get(), {&region, 1});
                blit->endEncoding();

                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::DoNotWait);
                ctx->commandQueue->waitIdle();
            }
        }
    }
}

#endif
