//
//  VertexDescriptor.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 16.10.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include <unordered_map>
#include <vector>

namespace snap::rhi::backend::opengl {
constexpr GLint InvalidAttribLocation = -1;

struct AttributeDescription {
    GLint location = InvalidAttribLocation;
    GLint componentsCount = 0;
    GLenum type = GL_NONE;
    GLboolean normalized = GL_FALSE;
    GLsizei stride = 0;
    uint32_t offset = 0;
};

struct BufferAttributes {
    std::array<AttributeDescription, snap::rhi::MaxVertexAttributesPerBuffer> attributes;
    uint32_t attributesCount = 0;
};

struct VertexDescriptor {
    std::array<BufferAttributes, snap::rhi::MaxVertexBuffers> bindings{};
};
} // namespace snap::rhi::backend::opengl
