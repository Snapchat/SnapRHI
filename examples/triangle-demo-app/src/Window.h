#pragma once
#include "FrameGuard.h"
#include "TriangleRenderer.h"
#include <SDL3/SDL.h>
#include <functional>
#include <memory>
#include <string>

namespace snap::rhi::demo {
class Window {
public:
    Window(const std::string& title, int width, int height, Uint32 flags);

    /// @brief Constructor with a pre-creation callback.
    /// The callback is invoked after SDL_Init but before SDL_CreateWindow.
    /// This is useful for platform-specific setup like SDL_Vulkan_LoadLibrary.
    Window(const std::string& title, int width, int height, Uint32 flags, std::function<void()> preCreateCallback);

    ~Window();

    void render();
    bool pollEvents();

protected:
    /**
     * @brief Acquires a frame for rendering.
     *
     * This method returns a FrameGuard that contains both the command buffer
     * and render target. The frame is automatically submitted when the guard
     * is destroyed.
     *
     * @return FrameGuard containing the frame resources. Check validity with operator bool().
     */
    virtual FrameGuard acquireFrame() = 0;

    SDL_Window* window = nullptr;
    int width = 0;
    int height = 0;

    std::shared_ptr<snap::rhi::Device> device = nullptr;
    snap::rhi::CommandQueue* commandQueue = nullptr;

    std::unique_ptr<TriangleRenderer> triangleRenderer = nullptr;
};
} // namespace snap::rhi::demo
