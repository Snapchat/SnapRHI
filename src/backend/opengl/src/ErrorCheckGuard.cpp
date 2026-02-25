#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"

#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Exception.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/Profile.hpp"

namespace {
constexpr std::string_view getMessageFromCode(GLenum code) {
    switch (code) {
        case GL_NO_ERROR:
            return "{NoError}";
        case GL_INVALID_ENUM:
            return "{InvalidEnum}";
        case GL_INVALID_VALUE:
            return "{InvalidValue}";
        case GL_INVALID_OPERATION:
            return "{InvalidOperation}";
        case GL_STACK_OVERFLOW:
            return "{StackOverflow}";
        case GL_STACK_UNDERFLOW:
            return "{StackUnderflow}";
        case GL_OUT_OF_MEMORY:
            return "{OutOfMemory}";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "{InvalidFramebufferOperation}";

        default:
            return "{Undefined Error}";
    }
}

std::pair<GLenum, std::string> getGLErrorAndMessage() {
    std::pair<GLenum, std::string> result{GL_NONE, ""};

    constexpr int MaxErrorCheckIterations = 64; // Serves to prevent infinite loop on certain gpus
    GLenum glError = GL_NO_ERROR;
    int errorCheckIterations = 0;
    for (errorCheckIterations = 0, glError = glGetError();
         (errorCheckIterations < MaxErrorCheckIterations) && (glError != GL_NO_ERROR);
         errorCheckIterations++, glError = glGetError()) {
        if (result.first != GL_OUT_OF_MEMORY) {
            result.first = glError;
        }
        result.second += getMessageFromCode(glError);
    }

    return result;
}

void checkGLError(const std::function<void()>& onErrorCallback) noexcept(false) {
    std::pair<GLenum, std::string> errorWithMessage = getGLErrorAndMessage();
    if (errorWithMessage.first != GL_NO_ERROR) {
        onErrorCallback();
    }

    int count = std::uncaught_exceptions();
    if (count) {
        return;
    }

    if (errorWithMessage.first == GL_OUT_OF_MEMORY) {
        snap::rhi::common::throwException<snap::rhi::backend::opengl::OpenGLOutOfMemoryException>(
            errorWithMessage.second);
    } else if (errorWithMessage.first != GL_NO_ERROR) {
        snap::rhi::common::throwException<snap::rhi::backend::opengl::OpenGLErrorException>(errorWithMessage.first,
                                                                                            errorWithMessage.second);
    }
}

void handleOpenGLErrors(snap::rhi::backend::opengl::Device* device,
                        const std::function<void()>& onErrorCallback) noexcept(false) {
    if (!device) {
        return;
    }

    checkGLError(onErrorCallback);
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
ErrorCheckGuard::ErrorCheckGuard(snap::rhi::backend::opengl::Device* glDevice,
                                 bool isEnabled,
                                 std::function<void()>&& callback)
    : device(isEnabled ? glDevice : nullptr), onErrorCallback(std::move(callback)) {
    handleOpenGLErrors(device, onErrorCallback);
}

ErrorCheckGuard::~ErrorCheckGuard() noexcept(false) {
    handleOpenGLErrors(device, onErrorCallback);
}
} // namespace snap::rhi::backend::opengl
