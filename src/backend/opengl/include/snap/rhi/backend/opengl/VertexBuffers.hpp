//
//  VertexBuffers.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 2/6/23.
//

#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/Limits.h"

#include <array>
#include <bitset>

namespace snap::rhi::backend::opengl {
class Buffer;
class RenderPipeline;
class Profile;
class DeviceContext;

class VertexBuffers final {
public:
    VertexBuffers(const Profile& gl);
    ~VertexBuffers() = default;

    void setBuffer(const uint32_t binding, Buffer* buffer, const uint32_t offset);
    void setPipeline(const RenderPipeline* renderPipeline);

    void bind(DeviceContext* dc);
    void clear();

private:
    const Profile& gl;

    std::array<Buffer*, snap::rhi::MaxVertexBuffers> buffers;
    std::array<uint32_t, snap::rhi::MaxVertexBuffers> offsets;

    const RenderPipeline* renderPipeline = nullptr;
    std::bitset<snap::rhi::MaxVertexAttributes> isAttribEnabled;
    std::bitset<snap::rhi::MaxVertexBuffers> updateMask;
};
} // namespace snap::rhi::backend::opengl
