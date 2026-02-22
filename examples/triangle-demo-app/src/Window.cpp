#include "Window.h"

#include <cmath>
#include <snap/rhi/CommandQueue.hpp>
#include <stdexcept>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace snap::rhi::demo {
Window::Window(const std::string& title, int width, int height, Uint32 flags) : width(width), height(height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(std::string("Failed to init SDL: ") + SDL_GetError());
    }

    flags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    window = SDL_CreateWindow(title.c_str(), width, height, flags);
    if (!window) {
        throw std::runtime_error(std::string("Failed to create window: ") + SDL_GetError());
    }
}

Window::Window(const std::string& title, int width, int height, Uint32 flags, std::function<void()> preCreateCallback)
    : width(width), height(height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(std::string("Failed to init SDL: ") + SDL_GetError());
    }

    if (preCreateCallback) {
        preCreateCallback();
    }

    flags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    window = SDL_CreateWindow(title.c_str(), width, height, flags);
    if (!window) {
        throw std::runtime_error(std::string("Failed to create window: ") + SDL_GetError());
    }
}

Window::~Window() {
    if (window) {
        SDL_DestroyWindow(window);
    }

    SDL_Quit();
}

void Window::render() {
    auto guard = device->setCurrent(device->getMainDeviceContext());

    auto frame = acquireFrame();
    if (!frame) {
        return; // Frame acquisition failed (e.g., window minimized)
    }

    triangleRenderer->render(frame.getCommandBuffer(), frame.getRenderTarget());
    // Frame is automatically submitted when 'frame' goes out of scope
}

bool Window::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }
    }
    return true;
}
} // namespace snap::rhi::demo
