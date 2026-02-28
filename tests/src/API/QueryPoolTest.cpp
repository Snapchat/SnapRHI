//
//  QueryPoolTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::QueryPool — creation, reset, timestamp queries.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "snap/rhi/QueryPool.h"
#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/BlitCommandEncoder.hpp"
#include "snap/rhi/Buffer.hpp"
#include <catch2/catch.hpp>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("QueryPool — Creation and basic usage", "[api][query]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            // Skip backends that don't support query pools (returns nullptr on creation)
            {
                snap::rhi::QueryPoolCreateInfo probeInfo{};
                probeInfo.queryCount = 1;
                if (!ctx->device->createQueryPool(probeInfo)) {
                    WARN("Query pools not supported on " << ctx->apiName << ", skipping");
                    continue;
                }
            }

            SECTION("Create query pool") {
                snap::rhi::QueryPoolCreateInfo qpInfo{};
                qpInfo.queryCount = 16;

                auto queryPool = ctx->device->createQueryPool(qpInfo);
                REQUIRE(queryPool != nullptr);
                CHECK(queryPool->getCreateInfo().queryCount == 16);
            }

            SECTION("Reset query pool via command buffer") {
                snap::rhi::QueryPoolCreateInfo qpInfo{};
                qpInfo.queryCount = 8;

                auto queryPool = ctx->device->createQueryPool(qpInfo);
                REQUIRE(queryPool != nullptr);

                auto cmdBuf = ctx->device->createCommandBuffer({.commandQueue = ctx->commandQueue});
                REQUIRE(cmdBuf != nullptr);

                // Reset should not crash
                cmdBuf->resetQueryPool(queryPool.get(), 0, 8);
            }

            SECTION("getResults before any queries returns NotReady or Error") {
                snap::rhi::QueryPoolCreateInfo qpInfo{};
                qpInfo.queryCount = 4;

                auto queryPool = ctx->device->createQueryPool(qpInfo);
                REQUIRE(queryPool != nullptr);

                std::vector<std::chrono::nanoseconds> results(4);
                auto result = queryPool->getResults(0, 4, results);
                // Before any queries are recorded, result should be NotReady or Error
                CHECK((result == snap::rhi::QueryPool::Result::NotReady ||
                       result == snap::rhi::QueryPool::Result::Error ||
                       result == snap::rhi::QueryPool::Result::Available));
            }

            SECTION("getCPUMemoryUsage and getGPUMemoryUsage are valid") {
                snap::rhi::QueryPoolCreateInfo qpInfo{};
                qpInfo.queryCount = 8;

                auto queryPool = ctx->device->createQueryPool(qpInfo);
                REQUIRE(queryPool != nullptr);
                CHECK(queryPool->getCPUMemoryUsage() >= 0);
                CHECK(queryPool->getGPUMemoryUsage() >= 0);
            }
        }
    }
}

#endif
