//
//  SynchronizationTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::Fence and snap::rhi::Semaphore — creation, wait, status, reset.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/BlitCommandEncoder.hpp"
#include "snap/rhi/Fence.hpp"
#include "snap/rhi/Semaphore.hpp"
#include <catch2/catch.hpp>
#include <cstring>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("Fence — Creation and lifecycle", "[api][sync][fence]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Create fence") {
                snap::rhi::FenceCreateInfo fenceInfo{};
                auto fence = ctx->device->createFence(fenceInfo);
                REQUIRE(fence != nullptr);
            }

            SECTION("Fence status before signal is NotReady") {
                snap::rhi::FenceCreateInfo fenceInfo{};
                auto fence = ctx->device->createFence(fenceInfo);
                REQUIRE(fence != nullptr);

                // Before any submission, fence should be not ready
                auto status = fence->getStatus();
                CHECK(status == snap::rhi::FenceStatus::NotReady);
            }

            SECTION("Submit work with fence and wait") {

                snap::rhi::FenceCreateInfo fenceInfo{};
                auto fence = ctx->device->createFence(fenceInfo);
                REQUIRE(fence != nullptr);

                // Create a small buffer copy to generate GPU work
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::CopySrc | snap::rhi::BufferUsage::CopyDst |
                                     snap::rhi::BufferUsage::TransferSrc | snap::rhi::BufferUsage::TransferDst;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 256;

                auto srcBuf = ctx->device->createBuffer(bufInfo);
                auto dstBuf = ctx->device->createBuffer(bufInfo);
                REQUIRE(srcBuf != nullptr);
                REQUIRE(dstBuf != nullptr);

                // Write data to source
                {
                    auto* mapped = reinterpret_cast<uint8_t*>(
                        srcBuf->map(snap::rhi::MemoryAccess::Write, 0, snap::rhi::WholeSize));
                    std::memset(mapped, 0xAB, 256);
                    srcBuf->unmap();
                }

                // Record and submit copy
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                {
                    auto* blit = cmdBuf->getBlitCommandEncoder();
                    blit->beginEncoding();
                    snap::rhi::BufferCopy copy{.srcOffset = 0, .dstOffset = 0, .size = 256};
                    blit->copyBuffer(srcBuf.get(), dstBuf.get(), std::span<snap::rhi::BufferCopy>{&copy, 1});
                    blit->endEncoding();
                }

                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::DoNotWait, fence.get());

                // Wait for completion
                fence->waitForComplete();
                CHECK(fence->getStatus() == snap::rhi::FenceStatus::Completed);
            }

            SECTION("Fence reset and reuse") {
                snap::rhi::FenceCreateInfo fenceInfo{};
                auto fence = ctx->device->createFence(fenceInfo);
                REQUIRE(fence != nullptr);

                // Reset should not crash
                fence->reset();
                auto status = fence->getStatus();
                CHECK(status == snap::rhi::FenceStatus::NotReady);
            }

            SECTION("Fence exportPlatformSyncHandle") {
                snap::rhi::FenceCreateInfo fenceInfo{};
                auto fence = ctx->device->createFence(fenceInfo);
                REQUIRE(fence != nullptr);

                // Export — may return nullptr on backends that don't support platform sync handles
                auto handle = fence->exportPlatformSyncHandle();
                // Just verify no crash; handle may be nullptr on some backends
                (void)handle;
            }
        }
    }
}

TEST_CASE("Semaphore — Creation", "[api][sync][semaphore]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {

            SECTION("Create semaphore") {
                snap::rhi::SemaphoreCreateInfo semInfo{};
                auto semaphore = ctx->device->createSemaphore(semInfo);
                REQUIRE(semaphore != nullptr);
            }

            SECTION("Signal and wait across submits") {

                snap::rhi::SemaphoreCreateInfo semInfo{};
                auto semaphore = ctx->device->createSemaphore(semInfo);
                REQUIRE(semaphore != nullptr);

                // Create two command buffers: first signals, second waits
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::CopySrc | snap::rhi::BufferUsage::CopyDst |
                                     snap::rhi::BufferUsage::TransferSrc | snap::rhi::BufferUsage::TransferDst;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 64;

                auto buf1 = ctx->device->createBuffer(bufInfo);
                auto buf2 = ctx->device->createBuffer(bufInfo);

                // First submit: signal the semaphore
                auto cmd1 = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                {
                    auto* blit = cmd1->getBlitCommandEncoder();
                    blit->beginEncoding();
                    snap::rhi::BufferCopy copy{.srcOffset = 0, .dstOffset = 0, .size = 64};
                    blit->copyBuffer(buf1.get(), buf2.get(), std::span<snap::rhi::BufferCopy>{&copy, 1});
                    blit->endEncoding();
                }

                snap::rhi::Semaphore* signalSems[] = {semaphore.get()};
                snap::rhi::CommandBuffer* bufs1[] = {cmd1.get()};
                ctx->commandQueue->submitCommands(
                    {}, {}, bufs1, signalSems,
                    snap::rhi::CommandBufferWaitType::DoNotWait, nullptr);

                // Second submit: wait on the semaphore
                auto cmd2 = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                {
                    auto* blit = cmd2->getBlitCommandEncoder();
                    blit->beginEncoding();
                    snap::rhi::BufferCopy copy{.srcOffset = 0, .dstOffset = 0, .size = 64};
                    blit->copyBuffer(buf1.get(), buf2.get(), std::span<snap::rhi::BufferCopy>{&copy, 1});
                    blit->endEncoding();
                }

                snap::rhi::Semaphore* waitSems[] = {semaphore.get()};
                snap::rhi::PipelineStageBits waitStages[] = {snap::rhi::PipelineStageBits::TransferBit};
                snap::rhi::CommandBuffer* bufs2[] = {cmd2.get()};
                ctx->commandQueue->submitCommands(
                    waitSems, waitStages, bufs2, {},
                    snap::rhi::CommandBufferWaitType::WaitUntilCompleted, nullptr);
            }
        }
    }
}

#endif
