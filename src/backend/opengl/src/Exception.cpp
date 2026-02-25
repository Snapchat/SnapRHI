#include "snap/rhi/backend/opengl/Exception.h"
#include "snap/rhi/common/StringFormat.h"

namespace snap::rhi::backend::opengl {
OpenGLOutOfMemoryException::OpenGLOutOfMemoryException(std::string& message)
    : snap::rhi::Exception("[OpenGLOutOfMemoryException] OpenGL memory cannot be allocated, message: " + message) {}

OpenGLErrorException::OpenGLErrorException(GLenum code, std::string& message)
    : snap::rhi::Exception(
          snap::rhi::common::stringFormat("[OpenGLErrorException] OpenGL error: 0x%x ('%s'), message: {'%s'}\n",
                                          (int)code,
                                          getErrorStr(code).data(),
                                          message.data())) {}
} // namespace snap::rhi::backend::opengl
