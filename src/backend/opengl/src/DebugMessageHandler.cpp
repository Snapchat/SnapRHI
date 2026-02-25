#include "snap/rhi/backend/opengl/DebugMessageHandler.h"
#include <snap/rhi/backend/opengl/DebugMessenger.h>
#include <snap/rhi/backend/opengl/Device.hpp>
#include <snap/rhi/backend/opengl/Profile.hpp>

namespace {
bool isDebugCallbackSupported() {
#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    return false;
#elif defined(GL_KHR_debug)
    // old drivers have extension, but don't have functions...
    return GLAD_SNAP_RHI_GL_KHR_debug && glDebugMessageCallback && glDebugMessageControl;
#elif defined(EGL_KHR_debug)
    return GLAD_SNAP_RHI_EGL_KHR_debug && glDebugMessageCallback && glDebugMessageControl;
#else
    return false;
#endif
}

void GLAPIENTRY debugCallback(GLenum source,
                              GLenum type,
                              GLuint id,
                              GLenum severity,
                              GLsizei length,
                              const GLchar* message,
                              const void* userParam) {
    auto* debugMessenger = static_cast<const snap::rhi::backend::opengl::DebugMessageHandler*>(userParam);
    debugMessenger->report({.message = std::string(message, length)});
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
DebugMessageHandler::DebugMessageHandler(snap::rhi::backend::opengl::Device* device)
    : device(device), gl(device->getOpenGL()) {
    if (!static_cast<bool>(device->getDeviceCreateInfo().deviceCreateFlags &
                           snap::rhi::DeviceCreateFlags::EnableDebugCallback)) {
        return;
    }

    if (!isDebugCallbackSupported()) {
        return;
    }

    gl.enable(GL_DEBUG_OUTPUT, nullptr);
    // gl.disable(GL_DEBUG_OUTPUT_SYNCHRONOUS, nullptr); // default value
    gl.debugMessageCallback(debugCallback, this);

    gl.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
    gl.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
    debugMessageEnabled = true;
}

DebugMessageHandler::~DebugMessageHandler() {
    if (!debugMessageEnabled) {
        return;
    }

    gl.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
    gl.debugMessageCallback(nullptr, nullptr);
}

void DebugMessageHandler::addDebugMessenger(DebugMessenger* debugMessenger) {
    if (!debugMessageEnabled) {
        return;
    }

    std::lock_guard lock(accessMutex);
    messengers.insert(debugMessenger);
}

void DebugMessageHandler::removeDebugMessenger(DebugMessenger* debugMessenger) {
    if (!debugMessageEnabled) {
        return;
    }

    std::lock_guard lock(accessMutex);
    messengers.erase(debugMessenger);
}

void DebugMessageHandler::report(const snap::rhi::DebugCallbackInfo& info) const {
    std::lock_guard lock(accessMutex);
    for (auto* debugMessenger : messengers) {
        debugMessenger->report(info);
    }
}
} // namespace snap::rhi::backend::opengl
