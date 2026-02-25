#pragma once

#include "../Window.h"

namespace snap::rhi::demo {
class MtlWindow final : public Window {
public:
    MtlWindow(const std::string& title, int width, int height);
    ~MtlWindow();

private:
    FrameGuard acquireFrame() override;

private:
    void* metalLayer = nullptr;
    void* drawable = nullptr;
};
} // namespace snap::rhi::demo
