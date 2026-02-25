#include "snap/rhi/backend/opengl/NativeBuffer.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include <snap/rhi/common/Scope.h>

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
#include <emscripten/bind.h>
#endif

namespace {
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
bool useJsBufferForRead = false;
#endif

GLenum getDefaultBufferTarget(const snap::rhi::BufferUsage bufferUsage) {
    if ((bufferUsage & snap::rhi::BufferUsage::VertexBuffer) == snap::rhi::BufferUsage::VertexBuffer) {
        return GL_ARRAY_BUFFER;
    }

    if ((bufferUsage & snap::rhi::BufferUsage::IndexBuffer) == snap::rhi::BufferUsage::IndexBuffer) {
        return GL_ELEMENT_ARRAY_BUFFER;
    }

    if ((bufferUsage & snap::rhi::BufferUsage::UniformBuffer) == snap::rhi::BufferUsage::UniformBuffer) {
        return GL_UNIFORM_BUFFER;
    }

    if ((bufferUsage & snap::rhi::BufferUsage::CopySrc) == snap::rhi::BufferUsage::CopySrc) {
        return GL_COPY_READ_BUFFER;
    }

    if ((bufferUsage & snap::rhi::BufferUsage::CopyDst) == snap::rhi::BufferUsage::CopyDst) {
        return GL_COPY_WRITE_BUFFER;
    }

    if ((bufferUsage & snap::rhi::BufferUsage::TransferSrc) == snap::rhi::BufferUsage::TransferSrc) {
        return GL_PIXEL_UNPACK_BUFFER;
    }

    if ((bufferUsage & snap::rhi::BufferUsage::TransferDst) == snap::rhi::BufferUsage::TransferDst) {
        return GL_PIXEL_PACK_BUFFER;
    }

    return GL_ARRAY_BUFFER;
}

GLenum getResourceUsage(const snap::rhi::BufferCreateInfo& bufferCreateInfo) {
    if ((bufferCreateInfo.bufferUsage & snap::rhi::BufferUsage::TransferSrc) == snap::rhi::BufferUsage::TransferSrc) {
        return GL_STREAM_DRAW;
    }

    if ((bufferCreateInfo.bufferUsage & snap::rhi::BufferUsage::TransferDst) == snap::rhi::BufferUsage::TransferDst) {
        return GL_STREAM_READ;
    }

    if ((bufferCreateInfo.memoryProperties & snap::rhi::MemoryProperties::DeviceLocal) !=
        snap::rhi::MemoryProperties::None) {
        return GL_STATIC_DRAW;
    }

    return GL_DYNAMIC_DRAW;
}

GLbitfield getAccessMask(const snap::rhi::MemoryAccess access, const snap::rhi::MemoryProperties memoryProperties) {
    /**
     * https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glMapBufferRange.xhtml
     * According to OpenGL specification for glMapBufferRange:
     * GL_INVALID_OPERATION is generated for any of the following conditions:
     *  - The buffer is already in a mapped state.
     *  - Neither GL_MAP_READ_BIT or GL_MAP_WRITE_BIT is set.
     *  - GL_MAP_READ_BIT is set and any of GL_MAP_INVALIDATE_RANGE_BIT, GL_MAP_INVALIDATE_BUFFER_BIT, or
     * GL_MAP_UNSYNCHRONIZED_BIT is set.
     *  - GL_MAP_FLUSH_EXPLICIT_BIT is set and GL_MAP_WRITE_BIT is not set.
     */

    if ((access & snap::rhi::MemoryAccess::Read) == snap::rhi::MemoryAccess::Read &&
        (access & snap::rhi::MemoryAccess::Write) == snap::rhi::MemoryAccess::Write) {
        /**
         * Cannot combine read access with unsynchronized access.
         * The driver assumes that if you are reading, you must be synchronized to ensure you read valid data.
         */
        return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
    }

    if ((access & snap::rhi::MemoryAccess::Read) == snap::rhi::MemoryAccess::Read) {
        /**
         * Cannot combine read access with unsynchronized access.
         * The driver assumes that if you are reading, you must be synchronized to ensure you read valid data.
         */
        return GL_MAP_READ_BIT;
    }

    if ((access & snap::rhi::MemoryAccess::Write) == snap::rhi::MemoryAccess::Write) {
        GLbitfield mask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;

        if ((memoryProperties & snap::rhi::MemoryProperties::HostCoherent) == snap::rhi::MemoryProperties::None) {
            mask |= GL_MAP_FLUSH_EXPLICIT_BIT;
        }
        return mask;
    }

    snap::rhi::common::throwException("Invalid access flags for glMapBufferRange");
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
void NativeBuffer::setUseJsBufferForRead(bool value) {
    useJsBufferForRead = value;
}
#endif

NativeBuffer::NativeBuffer(Device* device,
                           const snap::rhi::BufferCreateInfo& info,
                           const std::span<const std::byte>& rawData)
    : device(device),
      gl(device->getOpenGL()),
      validationLayer(device->getValidationLayer()),
      info(info),
      defaultTarget(getDefaultBufferTarget(info.bufferUsage)) {
    SNAP_RHI_VALIDATE(validationLayer,
                      info.size > 0,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[snap::rhi::backend::opengl::NativeBuffer] Invalid buffer size: 0 bytes.");

    if (!device->areResourcesLazyAllocationsEnabled()) {
        tryAllocateWithData(nullptr, rawData);
    } else if (rawData.data()) {
        assert(rawData.size() == info.size);
        data = std::shared_ptr<std::byte>(new std::byte[rawData.size()], std::default_delete<std::byte[]>());
        memcpy(data.get(), rawData.data(), rawData.size());
    }
}

NativeBuffer::~NativeBuffer() {
    try {
        auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);

        if (isMapped) {
            unmap(nullptr);
        }

        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~NativeBuffer] start destruction");
        SNAP_RHI_VALIDATE(validationLayer,
                          device->getCurrentDeviceContext(),
                          snap::rhi::ReportLevel::Warning,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~NativeBuffer] DeviceContext isn't attached to thread");
        SNAP_RHI_VALIDATE(validationLayer,
                          glDevice->isNativeContextAttached(),
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~NativeBuffer] OpenGL context isn't attached to thread");
        destroyGLBuffer();
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~NativeBuffer] end of destruction");
    } catch (const snap::rhi::common::Exception& e) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::NativeBuffer::~NativeBuffer] Caught: %s, (possible resource leak).",
                      e.what());
    } catch (...) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::NativeBuffer::~NativeBuffer] Caught unexpected error (possible "
                      "resource leak).");
    }
}

void NativeBuffer::tryAllocateWithData(DeviceContext* dc, const std::span<const std::byte>& rawData) {
    SNAP_RHI_VALIDATE(validationLayer,
                      glObject.isValid(),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[snap::rhi::backend::opengl::Buffer][tryAllocateWithData] Buffer data size(%d) doesn't match "
                      "the buffer size(%d)!",
                      rawData.size(),
                      info.size);
    if (buffer != GL_NONE) {
        return;
    }

    ErrorCheckGuard glErrorCheckGuard(device,
                                      static_cast<bool>(validationLayer.getValidationTags() & ValidationTag::BufferOp),
                                      [this]() { destroyGLBuffer(); });

    gl.genBuffers(1, &buffer);
    assert(buffer != GL_NONE);

    GLuint originalBuffer = GL_NONE;
    if (dc) {
        const auto& cache = dc->getGLStateCache();
        originalBuffer = cache.getBindBufferOrNone(defaultTarget);
    }

    gl.bindBuffer(defaultTarget, buffer, dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindBuffer(defaultTarget, originalBuffer, dc);
    };

    gl.bufferData(defaultTarget, info.size, rawData.data(), getResourceUsage(info));
}

void NativeBuffer::tryAllocate(DeviceContext* dc) {
    tryAllocateWithData(dc, {data.get(), static_cast<size_t>(info.size)});
}

void NativeBuffer::destroyGLBuffer() {
    if (buffer != GL_NONE) {
        gl.deleteBuffers(1, &buffer);
    }

    buffer = GL_NONE;
    glObject.markInvalid();
}

void NativeBuffer::uploadData(const uint64_t offset, const std::span<const std::byte>& rawData, DeviceContext* dc) {
    tryAllocate(dc);

    SNAP_RHI_VALIDATE(validationLayer,
                      info.size >= offset + rawData.size(),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[snap::rhi::backend::opengl::Buffer][uploadData] Buffer{size: %d} out of memory{size: %d}",
                      info.size,
                      offset + rawData.size());

    ErrorCheckGuard glErrorCheckGuard(
        device,
        static_cast<bool>(validationLayer.getValidationTags() & ValidationTag::BufferOp) &&
            (defaultTarget == GL_ELEMENT_ARRAY_BUFFER),
        [this]() { destroyGLBuffer(); });

    gl.bindBuffer(defaultTarget, buffer, dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindBuffer(defaultTarget, 0, dc);
    };

    if (offset == 0 && info.size == rawData.size()) {
        gl.bufferData(defaultTarget, info.size, rawData.data(), getResourceUsage(info));
    } else {
        gl.bufferSubData(defaultTarget, offset, rawData.size(), rawData.data());
    }
}

void NativeBuffer::flushMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> ranges, DeviceContext* dc) {
    if (ranges.empty()) {
        return;
    }

    if (!gl.getFeatures().isMapUnmapAvailable) {
        return;
    }
    // Only valid if mapping used GL_MAP_FLUSH_EXPLICIT_BIT.
    gl.bindBuffer(defaultTarget, buffer, dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindBuffer(defaultTarget, 0, dc);
    };
    for (const auto& r : ranges) {
        const uint64_t size = (r.size == WholeSize) ? (info.size - r.offset) : r.size;
        gl.flushMappedBufferRange(defaultTarget, r.offset, size);
    }
}

void NativeBuffer::invalidateMappedMemoryRanges(std::span<const snap::rhi::MappedMemoryRange> /*ranges*/,
                                                DeviceContext* /*dc*/) {
    // OpenGL doesn't provide a direct invalidate-to-host primitive for buffer mappings.
    // Callers should use fences/barriers before to read back(before map()).
}

std::byte* NativeBuffer::map(const snap::rhi::MemoryAccess access,
                             const uint64_t offset,
                             const uint64_t size,
                             DeviceContext* dc) {
    tryAllocate(dc);

    // Note: We cannot call mapRange multiple times without an intermediate unmap
    // https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glMapBufferRange.xhtml
    // https://developer.arm.com/documentation/101897/0200/cpu-overheads/opengl-es-cpu-memory-mapping?lang=en
    SNAP_RHI_VALIDATE(validationLayer,
                      !isMapped,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[snap::rhi::backend::opengl::NativeBuffer::map] Native buffer was mapped.");
    const uint64_t fixedSize = size == WholeSize ? (info.size - offset) : size;

    SNAP_RHI_VALIDATE(validationLayer,
                      (offset + fixedSize) <= info.size,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[snap::rhi::backend::opengl::NativeBuffer::map] invlalid buffer range [(offset{%lld} + "
                      "fixedSize{%lld}) <= info.size{%lld}].",
                      offset,
                      fixedSize,
                      info.size);
    ErrorCheckGuard glErrorCheckGuard(
        device,
        static_cast<bool>(validationLayer.getValidationTags() & ValidationTag::BufferOp) &&
            (defaultTarget == GL_ELEMENT_ARRAY_BUFFER),
        [this]() { destroyGLBuffer(); });

    gl.bindBuffer(defaultTarget, buffer, dc);
    SNAP_RHI_ON_SCOPE_EXIT {
        gl.bindBuffer(defaultTarget, 0, dc);
    };

    if (gl.getFeatures().isMapUnmapAvailable) {
        auto* ptr = static_cast<std::byte*>(
            gl.mapBufferRange(defaultTarget, offset, fixedSize, getAccessMask(access, info.memoryProperties)));
        SNAP_RHI_VALIDATE(validationLayer,
                          ptr,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::BufferOp,
                          "[snap::rhi::backend::opengl::NativeBuffer::map] glMapBufferRange return nullptr.");
        isMapped = ptr != nullptr;
        return ptr;
    }

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    if (!data) {
        data = std::shared_ptr<std::byte>(new std::byte[info.size], std::default_delete<std::byte[]>());
    }

    // Special handling since we want to pass an emscripten::val to getBufferSubData avoid extra copies
    emscripten::val dataAsTypedArray =
        emscripten::val(emscripten::typed_memory_view(fixedSize, reinterpret_cast<uint8_t*>(data.get())));
    auto bufferForRead = useJsBufferForRead ? emscripten::val::global("Uint8Array").new_(fixedSize) : dataAsTypedArray;
    auto glCtx = emscripten::val::module_property("ctx");
    glCtx.call<void>("getBufferSubData", defaultTarget, offset, bufferForRead);
    if (useJsBufferForRead) {
        dataAsTypedArray.call<void>("set", bufferForRead);
    }
    isMapped = data.get() != nullptr;
    return data.get();
#endif

    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::BufferOp,
                    "[snap::rhi::backend::opengl::NativeBuffer::map] Native buffer cannot be mapped.");
    return nullptr;
}

void NativeBuffer::unmap(DeviceContext* dc) {
    SNAP_RHI_VALIDATE(validationLayer,
                      isMapped,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::BufferOp,
                      "[snap::rhi::backend::opengl::NativeBuffer::unmap] Native buffer wasn't mapped.");
    SNAP_RHI_ON_SCOPE_EXIT {
        isMapped = false;
    };
    ErrorCheckGuard glErrorCheckGuard(
        device,
        static_cast<bool>(validationLayer.getValidationTags() & ValidationTag::BufferOp) &&
            (defaultTarget == GL_ELEMENT_ARRAY_BUFFER),
        [this]() { destroyGLBuffer(); });

    if (gl.getFeatures().isMapUnmapAvailable) {
        gl.bindBuffer(defaultTarget, buffer, dc);
        SNAP_RHI_ON_SCOPE_EXIT {
            gl.bindBuffer(defaultTarget, 0, dc);
        };

        GLboolean result = gl.unmapBuffer(defaultTarget);
        SNAP_RHI_VALIDATE(validationLayer,
                          result == GL_TRUE,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::BufferOp,
                          "[snap::rhi::backend::opengl::NativeBuffer::unmap] glUnmapBuffer return GL_FALSE");
        return;
    }

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    // It's ok, since WebAssembly used glGetBufferSubData
    return;
#endif

    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Error,
                    snap::rhi::ValidationTag::BufferOp,
                    "[snap::rhi::backend::opengl::NativeBuffer::unmap] Native buffer cannot be unmapped.");
}
} // namespace snap::rhi::backend::opengl
