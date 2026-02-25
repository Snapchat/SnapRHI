#include "ValBuffer.hpp"

#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Profile.hpp"

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()

namespace {
snap::rhi::BufferCreateInfo buildCPUStagingBufferInfo(const uint32_t size) {
    snap::rhi::BufferCreateInfo info{};

    info.size = size;
    info.bufferUsage =
        snap::rhi::BufferUsage::TransferSrc | snap::rhi::BufferUsage::CopySrc | snap::rhi::BufferUsage::CopyDst;
    info.memoryProperties = snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;

    return info;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
std::byte* ValBuffer::map([[maybe_unused]] const snap::rhi::MemoryAccess access,
                          const uint64_t offset,
                          const uint64_t size) {
    return nullptr;
}

void ValBuffer::unmap() {}

ValBuffer::ValBuffer(Device* device, const uint32_t size, const emscripten::val& bufferData)
    : snap::rhi::backend::opengl::Buffer(device, buildCPUStagingBufferInfo(size), nullptr), dataJs(bufferData) {}

const emscripten::val& ValBuffer::getJsBuffer() {
    return dataJs;
}
} // namespace snap::rhi::backend::opengl
#endif
