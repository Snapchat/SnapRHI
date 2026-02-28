//
//  TextureTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::Texture — creation (2D, cubemap, 3D), formats, mip levels, texture views.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "snap/rhi/Texture.hpp"
#include <catch2/catch.hpp>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("Texture — 2D creation", "[api][texture]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Create RGBA8 2D texture") {
                snap::rhi::TextureCreateInfo texInfo{};
                texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
                texInfo.textureType = snap::rhi::TextureType::Texture2D;
                texInfo.textureUsage = snap::rhi::TextureUsage::Sampled | snap::rhi::TextureUsage::ColorAttachment;
                texInfo.sampleCount = snap::rhi::SampleCount::Count1;
                texInfo.mipLevels = 1;
                texInfo.size = {64, 64, 1};

                auto texture = ctx->device->createTexture(texInfo);
                REQUIRE(texture != nullptr);
                CHECK(texture->getCreateInfo().size.width == 64);
                CHECK(texture->getCreateInfo().size.height == 64);
                CHECK(texture->getCreateInfo().format == snap::rhi::PixelFormat::R8G8B8A8Unorm);
            }

            SECTION("Create R8 2D texture") {
                snap::rhi::TextureCreateInfo texInfo{};
                texInfo.format = snap::rhi::PixelFormat::R8Unorm;
                texInfo.textureType = snap::rhi::TextureType::Texture2D;
                texInfo.textureUsage = snap::rhi::TextureUsage::Sampled;
                texInfo.sampleCount = snap::rhi::SampleCount::Count1;
                texInfo.mipLevels = 1;
                texInfo.size = {32, 32, 1};

                auto texture = ctx->device->createTexture(texInfo);
                REQUIRE(texture != nullptr);
            }

            SECTION("Create R16Float 2D texture") {
                snap::rhi::TextureCreateInfo texInfo{};
                texInfo.format = snap::rhi::PixelFormat::R16Float;
                texInfo.textureType = snap::rhi::TextureType::Texture2D;
                texInfo.textureUsage = snap::rhi::TextureUsage::Sampled;
                texInfo.sampleCount = snap::rhi::SampleCount::Count1;
                texInfo.mipLevels = 1;
                texInfo.size = {32, 32, 1};

                auto texture = ctx->device->createTexture(texInfo);
                REQUIRE(texture != nullptr);
            }

            SECTION("Create texture with multiple mip levels") {
                snap::rhi::TextureCreateInfo texInfo{};
                texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
                texInfo.textureType = snap::rhi::TextureType::Texture2D;
                texInfo.textureUsage = snap::rhi::TextureUsage::Sampled | snap::rhi::TextureUsage::TransferDst |
                                      snap::rhi::TextureUsage::TransferSrc;
                texInfo.sampleCount = snap::rhi::SampleCount::Count1;
                texInfo.mipLevels = 4;
                texInfo.size = {128, 128, 1};

                auto texture = ctx->device->createTexture(texInfo);
                REQUIRE(texture != nullptr);
                CHECK(texture->getCreateInfo().mipLevels == 4);
            }

            SECTION("Create large texture (1024x1024)") {
                snap::rhi::TextureCreateInfo texInfo{};
                texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
                texInfo.textureType = snap::rhi::TextureType::Texture2D;
                texInfo.textureUsage = snap::rhi::TextureUsage::Sampled | snap::rhi::TextureUsage::ColorAttachment |
                                      snap::rhi::TextureUsage::TransferDst | snap::rhi::TextureUsage::TransferSrc;
                texInfo.sampleCount = snap::rhi::SampleCount::Count1;
                texInfo.mipLevels = 1;
                texInfo.size = {1024, 1024, 1};

                auto texture = ctx->device->createTexture(texInfo);
                REQUIRE(texture != nullptr);
            }

            SECTION("getGPUMemoryUsage is reasonable") {
                snap::rhi::TextureCreateInfo texInfo{};
                texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
                texInfo.textureType = snap::rhi::TextureType::Texture2D;
                texInfo.textureUsage = snap::rhi::TextureUsage::Sampled;
                texInfo.sampleCount = snap::rhi::SampleCount::Count1;
                texInfo.mipLevels = 1;
                texInfo.size = {256, 256, 1};

                auto texture = ctx->device->createTexture(texInfo);
                REQUIRE(texture != nullptr);
                // 256×256×4 = 262144 bytes minimum for RGBA8
                CHECK(texture->getGPUMemoryUsage() >= 256u * 256u * 4u);
            }
        }
    }
}

TEST_CASE("Texture — Cubemap creation", "[api][texture]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            snap::rhi::TextureCreateInfo texInfo{};
            texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            texInfo.textureType = snap::rhi::TextureType::TextureCubemap;
            texInfo.textureUsage = snap::rhi::TextureUsage::Sampled;
            texInfo.sampleCount = snap::rhi::SampleCount::Count1;
            texInfo.mipLevels = 1;
            texInfo.size = {64, 64, 1}; // cubemap faces are implicit

            auto texture = ctx->device->createTexture(texInfo);
            REQUIRE(texture != nullptr);
            CHECK(texture->getCreateInfo().textureType == snap::rhi::TextureType::TextureCubemap);
        }
    }
}

TEST_CASE("Texture — Texture view creation", "[api][texture]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        // Skip backends that don't support texture views
        if (!ctx->device->getCapabilities().isTextureViewSupported) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            // Create a source texture
            snap::rhi::TextureCreateInfo texInfo{};
            texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            texInfo.textureType = snap::rhi::TextureType::Texture2D;
            texInfo.textureUsage = snap::rhi::TextureUsage::Sampled;
            texInfo.sampleCount = snap::rhi::SampleCount::Count1;
            texInfo.mipLevels = 4;
            texInfo.size = {128, 128, 1};

            auto sourceTexture = ctx->device->createTexture(texInfo);
            REQUIRE(sourceTexture != nullptr);

            // Create a view of mip level 1
            snap::rhi::TextureViewCreateInfo viewCreateInfo{};
            viewCreateInfo.texture = sourceTexture.get();
            viewCreateInfo.viewInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
            viewCreateInfo.viewInfo.range.baseMipLevel = 1;
            viewCreateInfo.viewInfo.range.levelCount = 1;

            auto view = ctx->device->createTextureView(viewCreateInfo);
            CHECK(view != nullptr);
        }
    }
}

#endif
