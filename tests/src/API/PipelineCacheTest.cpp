//
//  PipelineCacheTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::PipelineCache — creation and usage with pipelines.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "snap/rhi/PipelineCache.hpp"
#include <catch2/catch.hpp>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("PipelineCache — Creation", "[api][pipeline-cache]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Create empty pipeline cache") {
                snap::rhi::PipelineCacheCreateInfo cacheInfo{};
                auto cache = ctx->device->createPipelineCache(cacheInfo);
                REQUIRE(cache != nullptr);
            }

            SECTION("Create multiple pipeline caches") {
                snap::rhi::PipelineCacheCreateInfo cacheInfo{};
                auto cache1 = ctx->device->createPipelineCache(cacheInfo);
                auto cache2 = ctx->device->createPipelineCache(cacheInfo);
                REQUIRE(cache1 != nullptr);
                REQUIRE(cache2 != nullptr);
            }
        }
    }
}

#endif
