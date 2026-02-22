#include "snap/rhi/backend/opengl/Buffer.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include <cassert>

namespace {
bool canUseNativeBuffer(const snap::rhi::BufferCreateInfo& info,
                        const snap::rhi::backend::opengl::Features& features,
                        snap::rhi::backend::opengl::GLPBOOptions pboOptions) {
    const bool isDeviceLocalMemory =
        static_cast<bool>(info.memoryProperties & snap::rhi::MemoryProperties::DeviceLocal);
    const bool isVbIb = static_cast<bool>(info.bufferUsage & snap::rhi::BufferUsage::VertexBuffer) ||
                        static_cast<bool>(info.bufferUsage & snap::rhi::BufferUsage::IndexBuffer);
    if (isVbIb)
        return isDeviceLocalMemory || features.isMapUnmapAvailable;

    const bool isSsbo = ((info.bufferUsage & snap::rhi::BufferUsage::StorageBuffer) != snap::rhi::BufferUsage::None);
    if (isSsbo)
        return features.isSSBOSupported;

    const bool isUbo = ((info.bufferUsage & snap::rhi::BufferUsage::UniformBuffer) != snap::rhi::BufferUsage::None);
    if (isUbo)
        return features.isNativeUBOSupported && isDeviceLocalMemory;

    const bool isPboTransferSrc =
        (info.bufferUsage & (snap::rhi::BufferUsage::TransferSrc)) != snap::rhi::BufferUsage::None;
    if (isPboTransferSrc)
        return (pboOptions & snap::rhi::backend::opengl::GLPBOOptions::AllowNativePboAsTransferSrc) !=
                   snap::rhi::backend::opengl::GLPBOOptions::None &&
               features.isMapUnmapAvailable;

    const bool isPboDst = (info.bufferUsage & (snap::rhi::BufferUsage::TransferDst)) != snap::rhi::BufferUsage::None;
    if (isPboDst)
        return features.isMapUnmapAvailable;

    // In read-only mode, we can replace glMapRange with glReadBufferData
    const bool isBufferCopyDst =
        ((info.bufferUsage & (snap::rhi::BufferUsage::CopyDst)) != snap::rhi::BufferUsage::None);
    if (isBufferCopyDst)
        return features.isMapUnmapAvailable;

    const bool isBufferCopySrc =
        ((info.bufferUsage & (snap::rhi::BufferUsage::CopySrc)) != snap::rhi::BufferUsage::None);
    if (isBufferCopySrc)
        return isDeviceLocalMemory;

    return false;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
Buffer::Buffer(Device* device, const snap::rhi::BufferCreateInfo& info)
    : snap::rhi::Buffer(device, info),
      device(device),
      gl(device->getOpenGL()),
      validationLayer(device->getValidationLayer()) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[Buffer](bufferData)");

    SNAP_RHI_VALIDATE(validationLayer,
                      info.size > 0,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[snap::rhi::backend::opengl::Buffer] Invalid buffer size: 0 bytes.");

    const bool useNativeBuffer = canUseNativeBuffer(info, gl.getFeatures(), device->getPboOptions());
    if (useNativeBuffer) {
        nativeBuffer = std::make_unique<NativeBuffer>(device, info, std::span<const std::byte>{});
    } else {
        logicalBuffer = std::make_unique<LogicalBuffer>(device, info);
    }
}

Buffer::Buffer(Device* device, const BufferCreateInfo& info, const std::shared_ptr<std::byte>& bufferData)
    : snap::rhi::Buffer(device, info),
      device(device),
      gl(device->getOpenGL()),
      validationLayer(device->getValidationLayer()) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[Buffer](bufferData)");
    SNAP_RHI_VALIDATE(validationLayer,
                      info.size > 0,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[snap::rhi::backend::opengl::Buffer] Invalid buffer size: 0 bytes.");
    logicalBuffer = std::make_unique<LogicalBuffer>(device, info, bufferData);
}

std::byte* Buffer::map(const snap::rhi::MemoryAccess access,
                       const uint64_t offset,
                       const uint64_t size,
                       DeviceContext* dc) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[Buffer][map]");
    if (logicalBuffer) {
        return logicalBuffer->map() + offset;
    }

    if (nativeBuffer) {
        return nativeBuffer->map(access, offset, size, dc);
    }

    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::BufferOp,
                    "[snap::rhi::backend::opengl::Buffer] Invalid buffer");
    return nullptr;
}

void Buffer::unmap(DeviceContext* dc) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[Buffer][unmap]");

    if (logicalBuffer) {
        logicalBuffer->unmap();
        return;
    }

    if (nativeBuffer) {
        nativeBuffer->unmap(dc);
        return;
    }

    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::BufferOp,
                    "[snap::rhi::backend::opengl::Buffer] Invalid buffer");
}

void Buffer::flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges, DeviceContext* dc) {
    if (logicalBuffer) {
        return;
    }

    if (nativeBuffer) {
        nativeBuffer->flushMappedMemoryRanges(ranges, dc);
        return;
    }
}

void Buffer::invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges, DeviceContext* dc) {
    if (logicalBuffer) {
        return;
    }

    if (nativeBuffer) {
        nativeBuffer->invalidateMappedMemoryRanges(ranges, dc);
        return;
    }
}

GLuint Buffer::getOrAllocateGLBuffer(DeviceContext* dc) {
    if (!nativeBuffer) {
        assert(logicalBuffer);

        nativeBuffer =
            std::make_unique<NativeBuffer>(device, info, std::span<const std::byte>{logicalBuffer->map(), info.size});

        logicalBuffer->unmap();
        logicalBuffer.reset();
    }

    return getGLBuffer(dc);
}

void Buffer::uploadData(const uint64_t offset, const std::span<const std::byte>& rawData, DeviceContext* dc) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[Buffer][uploadData]");

    if (logicalBuffer) {
        logicalBuffer->uploadData(offset, rawData);

        return;
    }

    if (nativeBuffer) {
        nativeBuffer->uploadData(offset, rawData, dc);

        return;
    }

    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::BufferOp,
                    "[snap::rhi::backend::opengl::Buffer] Invalid buffer");
}
} // namespace snap::rhi::backend::opengl
