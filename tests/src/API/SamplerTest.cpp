//
//  SamplerTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::Sampler — filter modes, address modes, anisotropy, comparison.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "snap/rhi/Sampler.hpp"
#include <catch2/catch.hpp>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("Sampler — Creation with various configurations", "[api][sampler]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Default sampler (nearest, clamp-to-edge)") {
                snap::rhi::SamplerCreateInfo samplerInfo{};
                auto sampler = ctx->device->createSampler(samplerInfo);
                REQUIRE(sampler != nullptr);
            }

            SECTION("Linear min/mag filter") {
                snap::rhi::SamplerCreateInfo samplerInfo{};
                samplerInfo.minFilter = snap::rhi::SamplerMinMagFilter::Linear;
                samplerInfo.magFilter = snap::rhi::SamplerMinMagFilter::Linear;
                auto sampler = ctx->device->createSampler(samplerInfo);
                REQUIRE(sampler != nullptr);
            }

            SECTION("Mipmapped sampler") {
                snap::rhi::SamplerCreateInfo samplerInfo{};
                samplerInfo.minFilter = snap::rhi::SamplerMinMagFilter::Linear;
                samplerInfo.magFilter = snap::rhi::SamplerMinMagFilter::Linear;
                samplerInfo.mipFilter = snap::rhi::SamplerMipFilter::Linear;
                auto sampler = ctx->device->createSampler(samplerInfo);
                REQUIRE(sampler != nullptr);
            }

            SECTION("Repeat wrap mode") {
                snap::rhi::SamplerCreateInfo samplerInfo{};
                samplerInfo.wrapU = snap::rhi::WrapMode::Repeat;
                samplerInfo.wrapV = snap::rhi::WrapMode::Repeat;
                samplerInfo.wrapW = snap::rhi::WrapMode::Repeat;
                auto sampler = ctx->device->createSampler(samplerInfo);
                REQUIRE(sampler != nullptr);
            }

            SECTION("Mirror repeat wrap mode") {
                snap::rhi::SamplerCreateInfo samplerInfo{};
                samplerInfo.wrapU = snap::rhi::WrapMode::MirrorRepeat;
                samplerInfo.wrapV = snap::rhi::WrapMode::MirrorRepeat;
                auto sampler = ctx->device->createSampler(samplerInfo);
                REQUIRE(sampler != nullptr);
            }

            SECTION("Anisotropic filtering") {
                const auto& caps = ctx->device->getCapabilities();
                if (caps.maxAnisotropic >= snap::rhi::AnisotropicFiltering::Count4) {
                    snap::rhi::SamplerCreateInfo samplerInfo{};
                    samplerInfo.minFilter = snap::rhi::SamplerMinMagFilter::Linear;
                    samplerInfo.magFilter = snap::rhi::SamplerMinMagFilter::Linear;
                    samplerInfo.mipFilter = snap::rhi::SamplerMipFilter::Linear;
                    samplerInfo.anisotropyEnable = true;
                    samplerInfo.maxAnisotropy = snap::rhi::AnisotropicFiltering::Count4;
                    auto sampler = ctx->device->createSampler(samplerInfo);
                    REQUIRE(sampler != nullptr);
                }
            }

            SECTION("Comparison sampler (shadow mapping)") {
                snap::rhi::SamplerCreateInfo samplerInfo{};
                samplerInfo.minFilter = snap::rhi::SamplerMinMagFilter::Linear;
                samplerInfo.magFilter = snap::rhi::SamplerMinMagFilter::Linear;
                samplerInfo.compareEnable = true;
                samplerInfo.compareFunction = snap::rhi::CompareFunction::LessEqual;
                auto sampler = ctx->device->createSampler(samplerInfo);
                REQUIRE(sampler != nullptr);
            }

            SECTION("LOD clamping") {
                snap::rhi::SamplerCreateInfo samplerInfo{};
                samplerInfo.lodMin = 0.0f;
                samplerInfo.lodMax = 4.0f;
                auto sampler = ctx->device->createSampler(samplerInfo);
                REQUIRE(sampler != nullptr);
            }
        }
    }
}

#endif
