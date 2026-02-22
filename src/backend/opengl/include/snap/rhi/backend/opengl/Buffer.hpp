//
//  Buffer.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 5/26/22.
//

#pragma once

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/backend/opengl/LogicalBuffer.hpp"
#include "snap/rhi/backend/opengl/NativeBuffer.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"

namespace snap::rhi {
class ValidationLayer;
} // namespace snap::rhi

namespace snap::rhi::backend::opengl {
class Device;
class DeviceContext;
class Profile;

class Buffer : public snap::rhi::Buffer {
public:
    /**
     * This is only optimization for staging buffer
     * User have to provide proper lifetime of @bufferData
     */
    explicit Buffer(Device* device, const BufferCreateInfo& info, const std::shared_ptr<std::byte>& bufferData);
    explicit Buffer(Device* device, const snap::rhi::BufferCreateInfo& info);
    ~Buffer() override = default;

    void uploadData(const uint64_t offset, const std::span<const std::byte>& rawData, DeviceContext* dc);

    std::byte* map(const snap::rhi::MemoryAccess access, const uint64_t offset, const uint64_t size, DeviceContext* dc);
    void unmap(DeviceContext* dc);

    void flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges, DeviceContext* dc);
    void invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges, DeviceContext* dc);

    GLuint getOrAllocateGLBuffer(DeviceContext* dc);
    GLuint getGLBuffer(DeviceContext* dc) const {
        if (nativeBuffer) {
            return nativeBuffer->getGLBuffer(dc);
        }

        return GL_NONE;
    }

private:
    std::byte* map(const snap::rhi::MemoryAccess access, const uint64_t offset, const uint64_t size) override {
        return map(access, offset, size, nullptr);
    }

    void unmap() override {
        unmap(nullptr);
    }

    void flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) override {
        flushMappedMemoryRanges(ranges, nullptr);
    }

    void invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges) override {
        invalidateMappedMemoryRanges(ranges, nullptr);
    }

    Device* device = nullptr;
    snap::rhi::backend::opengl::Profile& gl;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    std::unique_ptr<LogicalBuffer> logicalBuffer = nullptr;
    std::unique_ptr<NativeBuffer> nativeBuffer = nullptr;
};
} // namespace snap::rhi::backend::opengl
