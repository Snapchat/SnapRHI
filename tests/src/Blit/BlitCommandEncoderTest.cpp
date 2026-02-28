//
//  BlitCommandEncoderTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::BlitCommandEncoder — buffer copy, texture copy, mipmap generation.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "snap/rhi/BlitCommandEncoder.hpp"
#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/Texture.hpp"
#include <catch2/catch.hpp>
#include <cstring>
#include <vector>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("BlitCommandEncoder — Buffer to buffer copy", "[api][blit][buffer]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            constexpr uint32_t kSize = 256;

            snap::rhi::BufferCreateInfo srcInfo{};
            srcInfo.bufferUsage = snap::rhi::BufferUsage::CopySrc | snap::rhi::BufferUsage::TransferSrc;
            srcInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            srcInfo.size = kSize;
            auto srcBuf = ctx->device->createBuffer(srcInfo);
            REQUIRE(srcBuf != nullptr);

            snap::rhi::BufferCreateInfo dstInfo{};
            dstInfo.bufferUsage = snap::rhi::BufferUsage::CopyDst | snap::rhi::BufferUsage::TransferDst;
            dstInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            dstInfo.size = kSize;
            auto dstBuf = ctx->device->createBuffer(dstInfo);
            REQUIRE(dstBuf != nullptr);

            // Fill source buffer with pattern
            {
                auto* mapped = reinterpret_cast<uint8_t*>(
                    srcBuf->map(snap::rhi::MemoryAccess::Write, 0, kSize));
                for (uint32_t i = 0; i < kSize; ++i) mapped[i] = static_cast<uint8_t>(i & 0xFF);
                srcBuf->unmap();
            }

            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            REQUIRE(cmdBuf != nullptr);

            auto* blitEncoder = cmdBuf->getBlitCommandEncoder();
            REQUIRE(blitEncoder != nullptr);
            blitEncoder->beginEncoding();

            snap::rhi::BufferCopy copyRegion{.srcOffset = 0, .dstOffset = 0, .size = kSize};
            blitEncoder->copyBuffer(srcBuf.get(), dstBuf.get(), std::span<snap::rhi::BufferCopy>{&copyRegion, 1});
            blitEncoder->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);

            // Verify
            {
                auto* mapped = reinterpret_cast<const uint8_t*>(
                    dstBuf->map(snap::rhi::MemoryAccess::Read, 0, kSize));
                REQUIRE(mapped != nullptr);
                for (uint32_t i = 0; i < kSize; ++i) {
                    if (mapped[i] != static_cast<uint8_t>(i & 0xFF)) {
                        FAIL("Buffer copy mismatch at byte " << i
                             << ": expected " << static_cast<int>(i & 0xFF)
                             << " got " << static_cast<int>(mapped[i]));
                    }
                }
                dstBuf->unmap();
            }
        }
    }
}

TEST_CASE("BlitCommandEncoder — Buffer to texture copy", "[api][blit][texture]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            constexpr uint32_t kWidth = 16;
            constexpr uint32_t kHeight = 16;
            constexpr uint32_t kBpp = 4; // RGBA8
            constexpr uint32_t kDataSize = kWidth * kHeight * kBpp;

            // Staging buffer
            snap::rhi::BufferCreateInfo bufInfo{};
            bufInfo.bufferUsage = snap::rhi::BufferUsage::CopySrc | snap::rhi::BufferUsage::TransferSrc;
            bufInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            bufInfo.size = kDataSize;
            auto staging = ctx->device->createBuffer(bufInfo);
            {
                auto* mapped = reinterpret_cast<uint8_t*>(
                    staging->map(snap::rhi::MemoryAccess::Write, 0, kDataSize));
                for (uint32_t i = 0; i < kDataSize; ++i) mapped[i] = static_cast<uint8_t>(i & 0xFF);
                staging->unmap();
            }

            // Texture
            snap::rhi::TextureCreateInfo texInfo{};
            texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            texInfo.textureType = snap::rhi::TextureType::Texture2D;
            texInfo.textureUsage = snap::rhi::TextureUsage::TransferDst | snap::rhi::TextureUsage::Sampled;
            texInfo.sampleCount = snap::rhi::SampleCount::Count1;
            texInfo.mipLevels = 1;
            texInfo.size = {kWidth, kHeight, 1};
            auto texture = ctx->device->createTexture(texInfo);

            auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
            auto* blit = cmdBuf->getBlitCommandEncoder();
            blit->beginEncoding();

            snap::rhi::BufferTextureCopy region{};
            region.bufferOffset = 0;
            region.textureExtent = {kWidth, kHeight, 1};
            blit->copyBufferToTexture(staging.get(), texture.get(),
                                      std::span<snap::rhi::BufferTextureCopy>{&region, 1});
            blit->endEncoding();

            ctx->commandQueue->submitCommandBuffer(
                cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
        }
    }
}

TEST_CASE("BlitCommandEncoder — Texture to buffer readback", "[api][blit][readback]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            constexpr uint32_t kWidth = 8;
            constexpr uint32_t kHeight = 8;
            constexpr uint32_t kBpp = 4;
            constexpr uint32_t kDataSize = kWidth * kHeight * kBpp;

            // Upload staging buffer
            snap::rhi::BufferCreateInfo uploadBufInfo{};
            uploadBufInfo.bufferUsage = snap::rhi::BufferUsage::CopySrc | snap::rhi::BufferUsage::TransferSrc;
            uploadBufInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            uploadBufInfo.size = kDataSize;
            auto uploadBuf = ctx->device->createBuffer(uploadBufInfo);
            {
                auto* mapped = reinterpret_cast<uint8_t*>(
                    uploadBuf->map(snap::rhi::MemoryAccess::Write, 0, kDataSize));
                for (uint32_t i = 0; i < kDataSize; ++i) mapped[i] = static_cast<uint8_t>((i * 7) & 0xFF);
                uploadBuf->unmap();
            }

            // Texture
            snap::rhi::TextureCreateInfo texInfo{};
            texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            texInfo.textureType = snap::rhi::TextureType::Texture2D;
            texInfo.textureUsage = snap::rhi::TextureUsage::TransferDst | snap::rhi::TextureUsage::TransferSrc |
                                   snap::rhi::TextureUsage::Sampled;
            texInfo.sampleCount = snap::rhi::SampleCount::Count1;
            texInfo.mipLevels = 1;
            texInfo.size = {kWidth, kHeight, 1};
            auto texture = ctx->device->createTexture(texInfo);

            // Upload
            {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* blit = cmdBuf->getBlitCommandEncoder();
                blit->beginEncoding();
                snap::rhi::BufferTextureCopy region{};
                region.textureExtent = {kWidth, kHeight, 1};
                blit->copyBufferToTexture(uploadBuf.get(), texture.get(),
                                          std::span<snap::rhi::BufferTextureCopy>{&region, 1});
                blit->endEncoding();
                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
            }

            // Readback buffer
            snap::rhi::BufferCreateInfo readbackBufInfo{};
            readbackBufInfo.bufferUsage = snap::rhi::BufferUsage::CopyDst | snap::rhi::BufferUsage::TransferDst;
            readbackBufInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            readbackBufInfo.size = kDataSize;
            auto readback = ctx->device->createBuffer(readbackBufInfo);

            // Read back
            {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* blit = cmdBuf->getBlitCommandEncoder();
                blit->beginEncoding();
                snap::rhi::BufferTextureCopy region{};
                region.textureExtent = {kWidth, kHeight, 1};
                blit->copyTextureToBuffer(texture.get(), readback.get(),
                                          std::span<snap::rhi::BufferTextureCopy>{&region, 1});
                blit->endEncoding();
                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
            }

            // Verify readback data matches upload
            {
                auto* mapped = reinterpret_cast<const uint8_t*>(
                    readback->map(snap::rhi::MemoryAccess::Read, 0, kDataSize));
                REQUIRE(mapped != nullptr);
                for (uint32_t i = 0; i < kDataSize; ++i) {
                    uint8_t expected = static_cast<uint8_t>((i * 7) & 0xFF);
                    if (mapped[i] != expected) {
                        FAIL("Texture readback mismatch at byte " << i
                             << ": expected " << static_cast<int>(expected)
                             << " got " << static_cast<int>(mapped[i]));
                    }
                }
                readback->unmap();
            }
        }
    }
}

TEST_CASE("BlitCommandEncoder — Generate mipmaps", "[api][blit][mipmap]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            constexpr uint32_t kBaseDim = 64;
            constexpr uint32_t kMipLevels = 4; // 64, 32, 16, 8
            constexpr uint32_t kBpp = 4;       // RGBA8

            // Create texture with 4 mip levels
            snap::rhi::TextureCreateInfo texInfo{};
            texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            texInfo.textureType = snap::rhi::TextureType::Texture2D;
            texInfo.textureUsage = snap::rhi::TextureUsage::Sampled |
                                   snap::rhi::TextureUsage::TransferSrc |
                                   snap::rhi::TextureUsage::TransferDst;
            texInfo.sampleCount = snap::rhi::SampleCount::Count1;
            texInfo.mipLevels = kMipLevels;
            texInfo.size = {kBaseDim, kBaseDim, 1};

            auto texture = ctx->device->createTexture(texInfo);
            REQUIRE(texture != nullptr);

            // Upload solid red (255, 0, 0, 255) to mip level 0
            constexpr uint32_t kBaseDataSize = kBaseDim * kBaseDim * kBpp;
            snap::rhi::BufferCreateInfo uploadBufInfo{};
            uploadBufInfo.bufferUsage = snap::rhi::BufferUsage::CopySrc | snap::rhi::BufferUsage::TransferSrc;
            uploadBufInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            uploadBufInfo.size = kBaseDataSize;
            auto uploadBuf = ctx->device->createBuffer(uploadBufInfo);
            REQUIRE(uploadBuf != nullptr);

            {
                auto* mapped = reinterpret_cast<uint8_t*>(
                    uploadBuf->map(snap::rhi::MemoryAccess::Write, 0, kBaseDataSize));
                REQUIRE(mapped != nullptr);
                for (uint32_t pixel = 0; pixel < kBaseDim * kBaseDim; ++pixel) {
                    mapped[pixel * 4 + 0] = 255; // R
                    mapped[pixel * 4 + 1] = 0;   // G
                    mapped[pixel * 4 + 2] = 0;   // B
                    mapped[pixel * 4 + 3] = 255; // A
                }
                uploadBuf->unmap();
            }

            // Upload to mip 0
            {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* blit = cmdBuf->getBlitCommandEncoder();
                blit->beginEncoding();
                snap::rhi::BufferTextureCopy region{};
                region.textureSubresource.mipLevel = 0;
                region.textureExtent = {kBaseDim, kBaseDim, 1};
                blit->copyBufferToTexture(uploadBuf.get(), texture.get(),
                                          std::span<snap::rhi::BufferTextureCopy>{&region, 1});
                blit->endEncoding();
                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
            }

            // Generate mipmaps from mip 0
            {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* blit = cmdBuf->getBlitCommandEncoder();
                blit->beginEncoding();
                blit->generateMipmaps(texture.get());
                blit->endEncoding();
                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
            }

            // Read back mip level 1 (32×32) and verify it's still solid red
            constexpr uint32_t kMip1Dim = kBaseDim / 2; // 32
            constexpr uint32_t kMip1DataSize = kMip1Dim * kMip1Dim * kBpp;

            snap::rhi::BufferCreateInfo readbackBufInfo{};
            readbackBufInfo.bufferUsage = snap::rhi::BufferUsage::CopyDst | snap::rhi::BufferUsage::TransferDst;
            readbackBufInfo.memoryProperties =
                snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
            readbackBufInfo.size = kMip1DataSize;
            auto readbackBuf = ctx->device->createBuffer(readbackBufInfo);
            REQUIRE(readbackBuf != nullptr);

            {
                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                auto* blit = cmdBuf->getBlitCommandEncoder();
                blit->beginEncoding();
                snap::rhi::BufferTextureCopy region{};
                region.textureSubresource.mipLevel = 1;
                region.textureExtent = {kMip1Dim, kMip1Dim, 1};
                blit->copyTextureToBuffer(texture.get(), readbackBuf.get(),
                                          std::span<snap::rhi::BufferTextureCopy>{&region, 1});
                blit->endEncoding();
                ctx->commandQueue->submitCommandBuffer(
                    cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
            }

            // Verify every pixel of mip 1 is solid red
            {
                auto* mapped = reinterpret_cast<const uint8_t*>(
                    readbackBuf->map(snap::rhi::MemoryAccess::Read, 0, kMip1DataSize));
                REQUIRE(mapped != nullptr);
                for (uint32_t pixel = 0; pixel < kMip1Dim * kMip1Dim; ++pixel) {
                    uint8_t r = mapped[pixel * 4 + 0];
                    uint8_t g = mapped[pixel * 4 + 1];
                    uint8_t b = mapped[pixel * 4 + 2];
                    uint8_t a = mapped[pixel * 4 + 3];
                    if (r != 255 || g != 0 || b != 0 || a != 255) {
                        FAIL("Mip 1 pixel " << pixel
                             << " expected (255,0,0,255) got ("
                             << static_cast<int>(r) << ","
                             << static_cast<int>(g) << ","
                             << static_cast<int>(b) << ","
                             << static_cast<int>(a) << ")");
                    }
                }
                readbackBuf->unmap();
            }
        }
    }
}

#endif
