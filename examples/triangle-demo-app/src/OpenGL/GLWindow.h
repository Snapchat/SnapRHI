#pragma once

#include "../Window.h"
#include <snap/rhi/backend/opengl/Device.hpp>

namespace snap::rhi::demo {
class GLWindow final : public Window {
public:
    GLWindow(const std::string& title, int width, int height);
    ~GLWindow();

private:
    FrameGuard acquireFrame() override;

private:
    SDL_GLContext glContext = nullptr;
    GLint screenFbo = 0;
};
} // namespace snap::rhi::demo
