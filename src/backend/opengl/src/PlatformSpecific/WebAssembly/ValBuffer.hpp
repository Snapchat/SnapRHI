#pragma once

#include "snap/rhi/backend/opengl/Buffer.hpp"

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
#include <emscripten/bind.h>

namespace snap::rhi::backend::opengl {
class Device;
class Profile;

class ValBuffer : public snap::rhi::backend::opengl::Buffer {
public:
    std::byte* map(const snap::rhi::MemoryAccess access,
                   const uint64_t offset = 0,
                   const uint64_t size = snap::rhi::WholeSize) override;
    void unmap() override;

    explicit ValBuffer(Device* device, const uint32_t size, const emscripten::val& srcData);
    const emscripten::val& getJsBuffer();

protected:
    emscripten::val dataJs = emscripten::val::null();
};
} // namespace snap::rhi::backend::opengl
#endif
