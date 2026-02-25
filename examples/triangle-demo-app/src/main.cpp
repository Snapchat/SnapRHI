#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#if SNAP_RHI_DEMO_APP_API_OPENGL
#include "OpenGL/GLWindow.h"
#endif

#if SNAP_RHI_DEMO_APP_API_VULKAN
#include "Vulkan/VKWindow.h"
#endif

#if SNAP_RHI_DEMO_APP_API_METAL
#include "Metal/MtlWindow.h"
#endif

#include <cmath>

int main(int /*argc*/, char** /*argv*/) {
#if SNAP_RHI_DEMO_APP_API_OPENGL
    snap::rhi::demo::GLWindow window("SnapRHI OpenGL Demo App", 800, 600);
#elif SNAP_RHI_DEMO_APP_API_VULKAN
    snap::rhi::demo::VkWindow window("SnapRHI Vulkan Demo App", 800, 600);
#elif SNAP_RHI_DEMO_APP_API_METAL
    snap::rhi::demo::MtlWindow window("SnapRHI Metal Demo App", 800, 600);
#endif

    while (window.pollEvents()) {
        window.render();
    }
    return 0;
}
