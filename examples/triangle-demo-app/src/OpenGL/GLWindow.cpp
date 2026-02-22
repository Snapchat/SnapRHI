#include "GLWindow.h"

#include <SDL3/SDL.h>

#include <snap/rhi/CommandQueue.hpp>

#include <snap/rhi/backend/common/Utils.hpp>
#include <snap/rhi/backend/opengl/Device.hpp>
#include <snap/rhi/backend/opengl/DeviceFactory.h>
#include <snap/rhi/backend/opengl/Texture.hpp>

#include <SDL3/SDL_opengl.h>
#include <stdexcept>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace {
void blitTextureToScreen(GLuint textureId, int width, int height, GLuint screenFbo) {
    GLuint readFbo = 0;
    glGenFramebuffers(1, &readFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, screenFbo);

    glBlitFramebuffer(0,
                      0,
                      static_cast<GLint>(width),
                      static_cast<GLint>(height),
                      0,
                      0,
                      static_cast<GLint>(width),
                      static_cast<GLint>(height),
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &readFbo);
}
} // unnamed namespace

namespace snap::rhi::demo {
GLWindow::GLWindow(const std::string& title, int width, int height) : Window(title, width, height, SDL_WINDOW_OPENGL) {
#if defined(ANDROID)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
#if TARGET_OS_IOS
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
#else
    // Windows/Linux: request a reasonably modern core profile.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#endif

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        throw std::runtime_error(std::string("Failed to create GL context: ") + SDL_GetError());
    }
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);

    snap::rhi::backend::opengl::DeviceCreateInfo deviceCreateInfo{};

    deviceCreateInfo.apiVersion = gl::APIVersion::Latest;
    deviceCreateInfo.deviceInfo.enabledReportLevel = snap::rhi::ReportLevel::All;
    deviceCreateInfo.deviceInfo.enabledTags = snap::rhi::ValidationTag::All;
    deviceCreateInfo.dcCreateInfo.isUserTheOwnerOfGLContext = true;
    deviceCreateInfo.glContext = glContext;

    device = snap::rhi::backend::opengl::createDevice(deviceCreateInfo);
    commandQueue = device->getCommandQueue(0, 0);

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &screenFbo);

    triangleRenderer = std::make_unique<TriangleRenderer>(device);
}

GLWindow::~GLWindow() {
    triangleRenderer.reset();
    device.reset();
    if (glContext) {
        SDL_GL_DestroyContext(glContext);
    }
}

FrameGuard GLWindow::acquireFrame() {
    SDL_GetWindowSizeInPixels(window, &width, &height);

    // Create render target texture
    snap::rhi::TextureCreateInfo textureDesc{
        .size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        .mipLevels = 1,
        .textureType = snap::rhi::TextureType::Texture2D,
        .textureUsage = snap::rhi::TextureUsage::ColorAttachment | snap::rhi::TextureUsage::Sampled |
                        snap::rhi::TextureUsage::TransferDst | snap::rhi::TextureUsage::TransferSrc,
        .sampleCount = snap::rhi::SampleCount::Count1,
        .format = snap::rhi::PixelFormat::R8G8B8A8Unorm,

    };
    auto renderTarget = device->createTexture(textureDesc);

    // Create command buffer for this frame
    auto commandBuffer = device->createCommandBuffer({
        .commandQueue = commandQueue,
    });

    // Capture necessary state for the submit callback
    const int frameWidth = width;
    const int frameHeight = height;
    const GLint fbo = screenFbo;

    auto submitCallback = [this, frameWidth, frameHeight, fbo](std::shared_ptr<snap::rhi::CommandBuffer> cmdBuffer,
                                                               std::shared_ptr<snap::rhi::Texture> target) {
        commandQueue->submitCommandBuffer(cmdBuffer.get(), snap::rhi::CommandBufferWaitType::DoNotWait, nullptr);

        auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device.get());
        if (!glDevice) {
            return;
        }

        auto* glTexture = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Texture>(target.get());
        if (!glTexture) {
            return;
        }

        const GLuint srcTex = static_cast<GLuint>(glTexture->getTextureID(nullptr));

        blitTextureToScreen(srcTex, frameWidth, frameHeight, fbo);

        SDL_GL_SwapWindow(window);
    };

    return FrameGuard(std::move(commandBuffer), std::move(renderTarget), std::move(submitCallback));
}
} // namespace snap::rhi::demo
