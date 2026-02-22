//
//  Exception.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/29/23.
//

#pragma once

#include "snap/rhi/Exception.h"

#include "snap/rhi/backend/opengl/OpenGL.h"

namespace snap::rhi::backend::opengl {
struct OpenGLOutOfMemoryException final : public snap::rhi::Exception {
    using Exception::Exception;

    OpenGLOutOfMemoryException(std::string& message);
};

struct OpenGLErrorException final : public snap::rhi::Exception {
    using Exception::Exception;

    OpenGLErrorException(GLenum code, std::string& message);
};
} // namespace snap::rhi::backend::opengl
