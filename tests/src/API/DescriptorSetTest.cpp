//
//  DescriptorSetTest.cpp
//  unitest-catch2
//
//  Tests for snap::rhi::DescriptorSetLayout, DescriptorPool, DescriptorSet, PipelineLayout.
//  Covers: creation, binding UBO/SSBO/textures/samplers, reset, updateDescriptorSet.
//  Copyright © 2026 Snapchat. All rights reserved.
//

#include "TestHarness/DeviceTestFixture.h"
#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/Descriptor.h"
#include "snap/rhi/DescriptorPool.hpp"
#include "snap/rhi/DescriptorSet.hpp"
#include "snap/rhi/DescriptorSetLayout.hpp"
#include "snap/rhi/PipelineLayout.hpp"
#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/Texture.hpp"
#include <catch2/catch.hpp>

#if SNAP_RHI_OS_MACOS() || SNAP_RHI_OS_WINDOWS() || SNAP_RHI_OS_LINUX_BASED() || SNAP_RHI_OS_IOS() || SNAP_RHI_OS_ANDROID()

TEST_CASE("DescriptorSetLayout — Creation", "[api][descriptor]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Empty layout") {
                snap::rhi::DescriptorSetLayoutCreateInfo layoutInfo{};
                auto layout = ctx->device->createDescriptorSetLayout(layoutInfo);
                REQUIRE(layout != nullptr);
            }

            SECTION("Layout with uniform buffer binding") {
                snap::rhi::DescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.bindings.push_back({
                    .binding = 0,
                    .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                    .stageBits = snap::rhi::ShaderStageBits::VertexShaderBit |
                                 snap::rhi::ShaderStageBits::FragmentShaderBit,
                });
                auto layout = ctx->device->createDescriptorSetLayout(layoutInfo);
                REQUIRE(layout != nullptr);
            }

            SECTION("Layout with multiple binding types") {
                snap::rhi::DescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.bindings.push_back({
                    .binding = 0,
                    .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                    .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
                });
                layoutInfo.bindings.push_back({
                    .binding = 1,
                    .descriptorType = snap::rhi::DescriptorType::SampledTexture,
                    .stageBits = snap::rhi::ShaderStageBits::FragmentShaderBit,
                });
                layoutInfo.bindings.push_back({
                    .binding = 2,
                    .descriptorType = snap::rhi::DescriptorType::Sampler,
                    .stageBits = snap::rhi::ShaderStageBits::FragmentShaderBit,
                });
                auto layout = ctx->device->createDescriptorSetLayout(layoutInfo);
                REQUIRE(layout != nullptr);
            }

            SECTION("Layout with storage buffer") {
                snap::rhi::DescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.bindings.push_back({
                    .binding = 0,
                    .descriptorType = snap::rhi::DescriptorType::StorageBuffer,
                    .stageBits = snap::rhi::ShaderStageBits::ComputeShaderBit,
                });
                auto layout = ctx->device->createDescriptorSetLayout(layoutInfo);
                REQUIRE(layout != nullptr);
            }

            SECTION("Layout with storage texture") {
                snap::rhi::DescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.bindings.push_back({
                    .binding = 0,
                    .descriptorType = snap::rhi::DescriptorType::StorageTexture,
                    .stageBits = snap::rhi::ShaderStageBits::ComputeShaderBit,
                });
                auto layout = ctx->device->createDescriptorSetLayout(layoutInfo);
                REQUIRE(layout != nullptr);
            }
        }
    }
}

TEST_CASE("PipelineLayout — Creation", "[api][descriptor]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Single set layout") {
                snap::rhi::DescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.bindings.push_back({
                    .binding = 0,
                    .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                    .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
                });
                auto setLayout = ctx->device->createDescriptorSetLayout(layoutInfo);

                snap::rhi::DescriptorSetLayout* layouts[] = {setLayout.get()};
                snap::rhi::PipelineLayoutCreateInfo pipeLayoutInfo{};
                pipeLayoutInfo.setLayouts = layouts;

                auto pipelineLayout = ctx->device->createPipelineLayout(pipeLayoutInfo);
                REQUIRE(pipelineLayout != nullptr);
            }

            SECTION("Multiple set layouts with gaps") {
                snap::rhi::DescriptorSetLayoutCreateInfo emptyLayoutInfo{};
                auto emptyLayout = ctx->device->createDescriptorSetLayout(emptyLayoutInfo);

                snap::rhi::DescriptorSetLayoutCreateInfo uboLayoutInfo{};
                uboLayoutInfo.bindings.push_back({
                    .binding = 0,
                    .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                    .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
                });
                auto uboLayout = ctx->device->createDescriptorSetLayout(uboLayoutInfo);

                // Set 0 = empty, Set 1 = UBO
                snap::rhi::DescriptorSetLayout* layouts[] = {emptyLayout.get(), uboLayout.get()};
                snap::rhi::PipelineLayoutCreateInfo pipeLayoutInfo{};
                pipeLayoutInfo.setLayouts = layouts;

                auto pipelineLayout = ctx->device->createPipelineLayout(pipeLayoutInfo);
                REQUIRE(pipelineLayout != nullptr);
            }
        }
    }
}

TEST_CASE("DescriptorPool — Creation and allocation", "[api][descriptor]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            SECTION("Create pool and allocate descriptor set") {
                snap::rhi::DescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.bindings.push_back({
                    .binding = 0,
                    .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                    .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
                });
                auto layout = ctx->device->createDescriptorSetLayout(layoutInfo);

                snap::rhi::DescriptorPoolCreateInfo poolInfo{};
                poolInfo.maxSets = 4;
                poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] = 4;
                auto pool = ctx->device->createDescriptorPool(poolInfo);
                REQUIRE(pool != nullptr);

                snap::rhi::DescriptorSetCreateInfo dsInfo{};
                dsInfo.descriptorPool = pool.get();
                dsInfo.descriptorSetLayout = layout.get();
                auto ds = ctx->device->createDescriptorSet(dsInfo);
                REQUIRE(ds != nullptr);
            }

            SECTION("Allocate multiple descriptor sets") {
                snap::rhi::DescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.bindings.push_back({
                    .binding = 0,
                    .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                    .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
                });
                auto layout = ctx->device->createDescriptorSetLayout(layoutInfo);

                snap::rhi::DescriptorPoolCreateInfo poolInfo{};
                poolInfo.maxSets = 8;
                poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] = 8;
                auto pool = ctx->device->createDescriptorPool(poolInfo);

                std::vector<std::shared_ptr<snap::rhi::DescriptorSet>> sets;
                for (int i = 0; i < 8; ++i) {
                    snap::rhi::DescriptorSetCreateInfo dsInfo{};
                    dsInfo.descriptorPool = pool.get();
                    dsInfo.descriptorSetLayout = layout.get();
                    auto ds = ctx->device->createDescriptorSet(dsInfo);
                    REQUIRE(ds != nullptr);
                    sets.push_back(ds);
                }
                CHECK(sets.size() == 8);
            }
        }
    }
}

TEST_CASE("DescriptorSet — Bind resources", "[api][descriptor]") {
    auto apis = test_harness::getAvailableAPIs();
    for (const auto& api : apis) {
        auto ctx = test_harness::createTestContext(api);
        if (!ctx) continue;

        SNAP_RHI_TEST_BACKEND_SECTION(ctx->apiName) {
            // Create a layout with all resource types
            snap::rhi::DescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.bindings.push_back({
                .binding = 0,
                .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                .stageBits = snap::rhi::ShaderStageBits::AllGraphics,
            });
            layoutInfo.bindings.push_back({
                .binding = 1,
                .descriptorType = snap::rhi::DescriptorType::StorageBuffer,
                .stageBits = snap::rhi::ShaderStageBits::ComputeShaderBit,
            });
            layoutInfo.bindings.push_back({
                .binding = 2,
                .descriptorType = snap::rhi::DescriptorType::SampledTexture,
                .stageBits = snap::rhi::ShaderStageBits::FragmentShaderBit,
            });
            layoutInfo.bindings.push_back({
                .binding = 3,
                .descriptorType = snap::rhi::DescriptorType::Sampler,
                .stageBits = snap::rhi::ShaderStageBits::FragmentShaderBit,
            });
            auto layout = ctx->device->createDescriptorSetLayout(layoutInfo);

            snap::rhi::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.maxSets = 4;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] = 4;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::StorageBuffer)] = 4;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::SampledTexture)] = 4;
            poolInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::Sampler)] = 4;
            auto pool = ctx->device->createDescriptorPool(poolInfo);

            snap::rhi::DescriptorSetCreateInfo dsInfo{};
            dsInfo.descriptorPool = pool.get();
            dsInfo.descriptorSetLayout = layout.get();
            auto ds = ctx->device->createDescriptorSet(dsInfo);
            REQUIRE(ds != nullptr);

            SECTION("bindUniformBuffer") {
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 64;
                auto ubo = ctx->device->createBuffer(bufInfo);
                ds->bindUniformBuffer(0, ubo.get());
            }

            SECTION("bindStorageBuffer") {
                if (ctx->device->getCapabilities().isShaderStorageBufferSupported) {
                    snap::rhi::BufferCreateInfo bufInfo{};
                    bufInfo.bufferUsage = snap::rhi::BufferUsage::StorageBuffer;
                    bufInfo.memoryProperties =
                        snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                    bufInfo.size = 256;
                    auto ssbo = ctx->device->createBuffer(bufInfo);
                    ds->bindStorageBuffer(1, ssbo.get());
                }
            }

            SECTION("bindTexture") {
                snap::rhi::TextureCreateInfo texInfo{};
                texInfo.format = snap::rhi::PixelFormat::R8G8B8A8Unorm;
                texInfo.textureType = snap::rhi::TextureType::Texture2D;
                texInfo.textureUsage = snap::rhi::TextureUsage::Sampled;
                texInfo.sampleCount = snap::rhi::SampleCount::Count1;
                texInfo.mipLevels = 1;
                texInfo.size = {32, 32, 1};
                auto tex = ctx->device->createTexture(texInfo);
                ds->bindTexture(2, tex.get());
            }

            SECTION("bindSampler") {
                snap::rhi::SamplerCreateInfo sampInfo{};
                auto sampler = ctx->device->createSampler(sampInfo);
                ds->bindSampler(3, sampler.get());
            }

            SECTION("updateDescriptorSet batch") {
                snap::rhi::BufferCreateInfo bufInfo{};
                bufInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
                bufInfo.memoryProperties =
                    snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
                bufInfo.size = 64;
                auto ubo = ctx->device->createBuffer(bufInfo);

                snap::rhi::SamplerCreateInfo sampInfo{};
                auto sampler = ctx->device->createSampler(sampInfo);

                std::vector<snap::rhi::Descriptor> writes = {
                    {.binding = 0,
                     .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
                     .bufferInfo = {.buffer = ubo.get(), .offset = 0, .range = snap::rhi::WholeSize}},
                    {.binding = 3,
                     .descriptorType = snap::rhi::DescriptorType::Sampler,
                     .samplerInfo = {.sampler = sampler.get()}},
                };
                ds->updateDescriptorSet(writes);
            }

            SECTION("reset") {
                ds->reset();
                // Should not crash and set should be reusable
            }
        }
    }
}

#endif
