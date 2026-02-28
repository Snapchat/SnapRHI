//
//  ComputeCommandEncoderTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::ComputeCommandEncoder — pipeline creation, dispatch, SSBO readback.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "TestHarness/ShaderHelper.h"
#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/ComputeCommandEncoder.hpp"
#include "snap/rhi/ComputePipeline.hpp"
#include "snap/rhi/DescriptorPool.hpp"
#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/DescriptorSetLayout.hpp"
#include "snap/rhi/PipelineLayout.hpp"
#include <catch2/catch.hpp>
#include <cstring>
#include <vector>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("ComputeCommandEncoder — Pipeline creation", "[api][compute][pipeline]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        // Compute requires StorageBuffer support
        if (!ctx->device->getCapabilities().isShaderStorageBufferSupported) {
            WARN("StorageBuffer not supported on " << ctx->apiName << ", skipping compute tests");
            continue;
        }

        auto shaders = test_harness::createComputeShaders(ctx->device.get());
        if (!shaders) {
            WARN("Skipping compute pipeline test for " << ctx->apiName);
            continue;
        }

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            snap::rhi::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
            dsLayoutInfo.bindings.push_back({
                .binding = 0,
                .descriptorType = snap::rhi::DescriptorType::StorageBuffer,
                .stageBits = snap::rhi::ShaderStageBits::ComputeShaderBit,
            });
            auto dsLayout = ctx->device->createDescriptorSetLayout(dsLayoutInfo);

            snap::rhi::DescriptorSetLayout* layouts[] = {dsLayout.get()};
            snap::rhi::PipelineLayoutCreateInfo plInfo{};
            plInfo.setLayouts = layouts;
            auto pipelineLayout = ctx->device->createPipelineLayout(plInfo);

            snap::rhi::ComputePipelineCreateInfo cpInfo{};
            cpInfo.pipelineLayout = pipelineLayout.get();
            cpInfo.stage = shaders->computeShader.get();
            cpInfo.localThreadGroupSize = {64, 1, 1};
            if (shaders->glPipelineInfo) {
                cpInfo.glPipelineInfo = shaders->glPipelineInfo;
            }

            auto pipeline = ctx->device->createComputePipeline(cpInfo);
            REQUIRE(pipeline != nullptr);
        }
    }
}

TEST_CASE("ComputeCommandEncoder — Dispatch and SSBO readback", "[api][compute][dispatch]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        // Compute requires StorageBuffer support
        if (!ctx->device->getCapabilities().isShaderStorageBufferSupported) {
            WARN("StorageBuffer not supported on " << ctx->apiName << ", skipping compute tests");
            continue;
        }

        auto shaders = test_harness::createComputeShaders(ctx->device.get());
        if (!shaders) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            constexpr uint32_t kElements = 64;
            constexpr uint32_t kBufSize = kElements * sizeof(uint32_t);

            // DS layout + pipeline layout
            snap::rhi::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
            dsLayoutInfo.bindings.push_back({
                .binding = 0,
                .descriptorType = snap::rhi::DescriptorType::StorageBuffer,
                .stageBits = snap::rhi::ShaderStageBits::ComputeShaderBit,
            });
            auto dsLayout = ctx->device->createDescriptorSetLayout(dsLayoutInfo);

            snap::rhi::DescriptorSetLayout* layouts[] = {dsLayout.get()};
            snap::rhi::PipelineLayoutCreateInfo plInfo{};
            plInfo.setLayouts = layouts;
            auto pipelineLayout = ctx->device->createPipelineLayout(plInfo);

            // Compute pipeline
            snap::rhi::ComputePipelineCreateInfo cpInfo{};
            cpInfo.pipelineLayout = pipelineLayout.get();
            cpInfo.stage = shaders->computeShader.get();
            cpInfo.localThreadGroupSize = {64, 1, 1};
            if (shaders->glPipelineInfo) {
                cpInfo.glPipelineInfo = shaders->glPipelineInfo;
            }
            auto pipeline = ctx->device->createComputePipeline(cpInfo);
            REQUIRE(pipeline != nullptr);

            // SSBO
            snap::rhi::BufferCreateInfo ssboInfo{};
            ssboInfo.bufferUsage = snap::rhi::BufferUsage::StorageBuffer;
            ssboInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            ssboInfo.size = kBufSize;
            auto ssbo = ctx->device->createBuffer(ssboInfo);

            // Descriptor pool + set
            snap::rhi::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.maxSets = 1;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::StorageBuffer)] = 1;
            auto pool = ctx->device->createDescriptorPool(poolInfo);

            snap::rhi::DescriptorSetCreateInfo dsInfo{};
            dsInfo.descriptorPool = pool.get();
            dsInfo.descriptorSetLayout = dsLayout.get();
            auto ds = ctx->device->createDescriptorSet(dsInfo);
            ds->bindStorageBuffer(0, ssbo.get());

            // Dispatch
            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            auto* computeEncoder = cmdBuf->getComputeCommandEncoder();
            computeEncoder->beginEncoding();
            computeEncoder->bindComputePipeline(pipeline.get());
            computeEncoder->bindDescriptorSet(0, ds.get(), {});
            computeEncoder->dispatch(1, 1, 1); // 1 workgroup of 64 threads
            computeEncoder->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

            // Verify: data[idx] = idx * 42 + 7
            {
                auto* mapped = reinterpret_cast<const uint32_t*>(
                    ssbo->map(snap::rhi::MemoryAccess::Read, 0, kBufSize));
                REQUIRE(mapped != nullptr);
                for (uint32_t i = 0; i < kElements; ++i) {
                    uint32_t expected = i * 42u + 7u;
                    if (mapped[i] != expected) {
                        ssbo->unmap();
                        FAIL("SSBO mismatch at index " << i << ": expected " << expected << " got " << mapped[i]);
                    }
                }
                ssbo->unmap();
            }
        }
    }
}

#endif
