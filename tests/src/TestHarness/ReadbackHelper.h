#pragma once
#include "DeviceTestFixture.h"
#include "snap/rhi/BlitCommandEncoder.hpp"
#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/CopyInfo.h"
#include "snap/rhi/Texture.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace test_harness {

struct Pixel {
    uint8_t r = 0, g = 0, b = 0, a = 0;
    bool operator==(const Pixel& o) const = default;
};

inline std::vector<Pixel> readbackTexture(
    snap::rhi::Device* device, snap::rhi::CommandQueue* queue,
    snap::rhi::Texture* texture, uint32_t width, uint32_t height) {
    const uint32_t dataSize = width * height * 4;
    snap::rhi::BufferCreateInfo bufInfo{};
    bufInfo.bufferUsage = snap::rhi::BufferUsage::CopyDst | snap::rhi::BufferUsage::TransferDst;
    bufInfo.memoryProperties = snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
    bufInfo.size = dataSize;
    auto readbackBuf = device->createBuffer(bufInfo);
    auto cmdBuf = device->createCommandBuffer({.commandQueue = queue});
    auto* blit = cmdBuf->getBlitCommandEncoder();
    blit->beginEncoding();
    snap::rhi::BufferTextureCopy region{};
    region.textureExtent = {width, height, 1};
    blit->copyTextureToBuffer(texture, readbackBuf.get(), std::span<snap::rhi::BufferTextureCopy>{&region, 1});
    blit->endEncoding();
    queue->submitCommandBuffer(cmdBuf.get(), snap::rhi::CommandBufferWaitType::WaitUntilCompleted);
    std::vector<Pixel> pixels(width * height);
    auto* mapped = readbackBuf->map(snap::rhi::MemoryAccess::Read, 0, dataSize);
    std::memcpy(pixels.data(), mapped, dataSize);
    readbackBuf->unmap();
    return pixels;
}

inline bool pixelApproxEqual(const Pixel& a, const Pixel& b, uint8_t tolerance = 2) {
    auto diff = [](uint8_t x, uint8_t y) { return static_cast<uint8_t>(std::abs(int(x) - int(y))); };
    return diff(a.r, b.r) <= tolerance && diff(a.g, b.g) <= tolerance &&
           diff(a.b, b.b) <= tolerance && diff(a.a, b.a) <= tolerance;
}

inline Pixel getPixel(const std::vector<Pixel>& pixels, uint32_t w, uint32_t x, uint32_t y) {
    return pixels[y * w + x];
}

inline Pixel averageColor(const std::vector<Pixel>& pixels) {
    if (pixels.empty()) return {};
    uint64_t sR = 0, sG = 0, sB = 0, sA = 0;
    for (const auto& p : pixels) { sR += p.r; sG += p.g; sB += p.b; sA += p.a; }
    auto n = static_cast<uint64_t>(pixels.size());
    return {uint8_t(sR/n), uint8_t(sG/n), uint8_t(sB/n), uint8_t(sA/n)};
}

inline float colorCoverage(const std::vector<Pixel>& pixels, const Pixel& expected, uint8_t tol = 2) {
    if (pixels.empty()) return 0.0f;
    uint32_t m = 0;
    for (const auto& p : pixels) if (pixelApproxEqual(p, expected, tol)) ++m;
    return float(m) / float(pixels.size());
}

inline bool hasNonBlackPixel(const std::vector<Pixel>& pixels) {
    Pixel opaqueBlack{0, 0, 0, 255};
    for (const auto& p : pixels) {
        if (!pixelApproxEqual(p, opaqueBlack, 2)) return true;
    }
    return false;
}

} // namespace test_harness
