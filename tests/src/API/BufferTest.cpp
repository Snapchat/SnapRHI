//
//  BufferTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::Buffer — creation, map/unmap, flush/invalidate, usage flags.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "snap/rhi/Buffer.hpp"
#include <catch2/catch.hpp>
#include <cstring>
#include <numeric>
#include <vector>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("Buffer — Creation and lifecycle", "[api][buffer]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Create host-visible buffer") {
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer | snap::rhi::BufferUsage::CopyDst;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 256;

                auto buffer = ctx->device->createBuffer(bufInfo);
                REQUIRE(buffer != nullptr);
                CHECK(buffer->getCreateInfo().size == 256);
            }

            SECTION("Create uniform buffer") {
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 64;

                auto buffer = ctx->device->createBuffer(bufInfo);
                REQUIRE(buffer != nullptr);
            }

            SECTION("Create storage buffer") {
                if (ctx->device->getCapabilities().isShaderStorageBufferSupported) {
                    snap::rhi::BufferCreateInfo bufInfo{};
                    bufInfo.bufferUsage = snap::rhi::BufferUsage::StorageBuffer | snap::rhi::BufferUsage::CopySrc;
                    bufInfo.memoryProperties =
                        snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                    bufInfo.size = 1024;

                    auto buffer = ctx->device->createBuffer(bufInfo);
                    REQUIRE(buffer != nullptr);
                }
            }

            SECTION("Create index buffer") {
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::IndexBuffer;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 128;

                auto buffer = ctx->device->createBuffer(bufInfo);
                REQUIRE(buffer != nullptr);
            }

            SECTION("Create transfer src/dst buffer") {
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::TransferSrc | snap::rhi::BufferUsage::TransferDst |
                                     snap::rhi::BufferUsage::CopySrc | snap::rhi::BufferUsage::CopyDst;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 512;

                auto buffer = ctx->device->createBuffer(bufInfo);
                REQUIRE(buffer != nullptr);
            }

            SECTION("getGPUMemoryUsage reports at least the buffer size") {
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 1024;

                auto buffer = ctx->device->createBuffer(bufInfo);
                REQUIRE(buffer != nullptr);
                CHECK(buffer->getGPUMemoryUsage() >= 1024);
            }
        }
    }
}

TEST_CASE("Buffer — Map / Unmap / Read-back", "[api][buffer]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Write and read back data") {
                constexpr uint32_t kBufferSize = 256;
                std::vector<uint8_t> testData(kBufferSize);
                std::iota(testData.begin(), testData.end(), 0);

                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer |
                                     snap::rhi::BufferUsage::CopySrc | snap::rhi::BufferUsage::CopyDst;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = kBufferSize;

                auto buffer = ctx->device->createBuffer(bufInfo);
                REQUIRE(buffer != nullptr);

                // Write
                {
                    auto* mapped = reinterpret_cast<uint8_t*>(
                        buffer->map(snap::rhi::MemoryAccess::Write, 0, snap::rhi::WholeSize));
                    REQUIRE(mapped != nullptr);
                    std::memcpy(mapped, testData.data(), kBufferSize);
                    buffer->unmap();
                }

                // Read back
                {
                    auto* mapped = reinterpret_cast<const uint8_t*>(
                        buffer->map(snap::rhi::MemoryAccess::Read, 0, snap::rhi::WholeSize));
                    REQUIRE(mapped != nullptr);
                    for (uint32_t i = 0; i < kBufferSize; ++i) {
                        if (mapped[i] != testData[i]) {
                            FAIL("Buffer read-back mismatch at index " << i);
                            break;
                        }
                    }
                    buffer->unmap();
                }
            }

            SECTION("Map with offset — multi-region write and full verification") {
                constexpr uint32_t kBufferSize = 1024;

                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = kBufferSize;

                auto buffer = ctx->device->createBuffer(bufInfo);
                REQUIRE(buffer != nullptr);

                // Zero the entire buffer first
                {
                    auto* mapped = reinterpret_cast<uint8_t*>(
                        buffer->map(snap::rhi::MemoryAccess::Write, 0, snap::rhi::WholeSize));
                    REQUIRE(mapped != nullptr);
                    std::memset(mapped, 0, kBufferSize);
                    buffer->unmap();
                }

                // Write distinct patterns at three non-overlapping offsets:
                //   Region A: [64..128)   filled with 0xAA
                //   Region B: [256..384)  filled with 0xBB
                //   Region C: [512..768)  filled with 0xCC
                struct Region {
                    uint32_t offset;
                    uint32_t size;
                    uint8_t pattern;
                };
                const Region regions[] = {
                    {64,  64,  0xAA},
                    {256, 128, 0xBB},
                    {512, 256, 0xCC},
                };

                for (const auto& r : regions) {
                    auto* mapped = reinterpret_cast<uint8_t*>(
                        buffer->map(snap::rhi::MemoryAccess::Write, 0, snap::rhi::WholeSize));
                    REQUIRE(mapped != nullptr);
                    std::memset(mapped + r.offset, r.pattern, r.size);
                    buffer->unmap();
                }

                // Read back the ENTIRE buffer and verify every byte
                {
                    auto* mapped = reinterpret_cast<const uint8_t*>(
                        buffer->map(snap::rhi::MemoryAccess::Read, 0, snap::rhi::WholeSize));
                    REQUIRE(mapped != nullptr);

                    for (uint32_t i = 0; i < kBufferSize; ++i) {
                        uint8_t expected = 0x00;
                        for (const auto& r : regions) {
                            if (i >= r.offset && i < r.offset + r.size) {
                                expected = r.pattern;
                                break;
                            }
                        }
                        if (mapped[i] != expected) {
                            FAIL("Buffer mismatch at byte " << i
                                 << ": expected 0x" << std::hex << static_cast<int>(expected)
                                 << " got 0x" << static_cast<int>(mapped[i]) << std::dec);
                        }
                    }
                    buffer->unmap();
                }
            }

            SECTION("Multiple buffers with different usage flags") {
                auto createAndVerify = [&](snap::rhi::BufferUsage usage, uint32_t size) {
                    snap::rhi::BufferCreateInfo bufInfo{};
                    bufInfo.bufferUsage = usage;
                    bufInfo.memoryProperties =
                        snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                    bufInfo.size = size;

                    auto buffer = ctx->device->createBuffer(bufInfo);
                    REQUIRE(buffer != nullptr);
                    return buffer;
                };

                auto vb = createAndVerify(snap::rhi::BufferUsage::VertexBuffer, 256);
                auto ib = createAndVerify(snap::rhi::BufferUsage::IndexBuffer, 128);
                auto ub = createAndVerify(snap::rhi::BufferUsage::UniformBuffer, 64);

                // StorageBuffer may not be supported on all backends (e.g. macOS OpenGL 4.1)
                if (ctx->device->getCapabilities().isShaderStorageBufferSupported) {
                    auto sb = createAndVerify(snap::rhi::BufferUsage::StorageBuffer, 512);
                }
            }
        }
    }
}

#endif
