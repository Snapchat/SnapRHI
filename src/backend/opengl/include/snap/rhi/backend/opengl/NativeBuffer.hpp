// Copyright © 2024 Snap, Inc. All rights reserved.

#pragma once

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/opengl/GLObject.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include <snap/rhi/common/Platform.h>

// vector.resize is too slow, because vector.resize will initialize the value
#include <atomic>
#include <memory>
#include <span>

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
namespace emscripten {
class val;
}
#endif

namespace snap::rhi {
class ValidationLayer;
} // namespace snap::rhi

namespace snap::rhi::backend::opengl {
class Device;
class DeviceContext;
class Profile;

// https://developer.arm.com/documentation/101897/0200/cpu-overheads/opengl-es-cpu-memory-mapping?lang=en
class NativeBuffer final {
public:
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    static void setUseJsBufferForRead(bool value);
#endif

    explicit NativeBuffer(Device* device,
                          const snap::rhi::BufferCreateInfo& info,
                          const std::span<const std::byte>& rawData);
    ~NativeBuffer();

    void uploadData(const uint64_t offset, const std::span<const std::byte>& rawData, DeviceContext* dc);

    std::byte* map(const snap::rhi::MemoryAccess access, const uint64_t offset, const uint64_t size, DeviceContext* dc);
    void unmap(DeviceContext* dc);

    void flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges, DeviceContext* dc);
    void invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges, DeviceContext* dc);

    GLuint getGLBuffer(DeviceContext* dc) {
        tryAllocate(dc);
        return buffer;
    }

private:
    void tryAllocate(DeviceContext* dc);
    void tryAllocateWithData(DeviceContext* dc, const std::span<const std::byte>& rawData);

    void destroyGLBuffer();

    Device* device = nullptr;
    snap::rhi::backend::opengl::Profile& gl;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    snap::rhi::BufferCreateInfo info{};
    GLenum defaultTarget = GL_NONE;
    mutable GLuint buffer = GL_NONE;
    bool isMapped = false;

    std::shared_ptr<std::byte> data = nullptr;
    GLObject glObject{};
};
} // namespace snap::rhi::backend::opengl
