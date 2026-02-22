#pragma once

#include "../Window.h"

#include <snap/rhi/backend/vulkan/Device.h>
#include <vector>

namespace snap::rhi::backend::vulkan {
class Device;
}

namespace snap::rhi::demo {

/**
 * @class VkWindow
 * @brief Vulkan-backed window implementation for the SnapRHI demo.
 *
 * This class demonstrates how to integrate SnapRHI with a native Vulkan swapchain.
 *
 * ## Architecture Overview
 *
 * This implementation uses a **frame-based synchronization model** that separates:
 * - **Frame Index**: A rotating index [0, maxFramesInFlight) that tracks CPU-side frame submission.
 * - **Image Index**: The swapchain image index returned by vkAcquireNextImageKHR (unpredictable).
 *
 * This separation is essential because we cannot know which swapchain image we'll acquire
 * until after calling vkAcquireNextImageKHR, but we need a semaphore to pass to that call.
 *
 * ## Synchronization Strategy
 *
 * Each frame (not each image) owns synchronization primitives:
 * - `imageAvailableSemaphores[frameIndex]`: Passed to vkAcquireNextImageKHR, signaled when
 *    the acquired image is ready for rendering.
 * - `renderFinishedSemaphores[frameIndex]`: Signaled when rendering completes, waited on by present.
 * - `inFlightFences[frameIndex]`: CPU-GPU sync to prevent overwriting in-use command buffers.
 *
 * The fence is waited on at the START of getNextSwapchainImage() to ensure the frame's
 * semaphores are no longer in use before reusing them for a new acquisition.
 *
 * ## Key Design Decisions
 *
 * 1. **Default Layout Strategy**: Swapchain textures are created with
 *    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as the default layout. This tells SnapRHI
 *    to automatically transition images to presentation-ready layout after rendering,
 *    eliminating the need for manual layout transitions in the presentation path.
 *
 * 2. **External Ownership**: Swapchain images are wrapped with isOwner=false since
 *    the swapchain owns the actual VkImage objects.
 *
 * 3. **Frame-Based vs Image-Based Sync**: Using frame-based synchronization is the
 *    standard Vulkan pattern that correctly handles the asynchronous nature of
 *    swapchain image acquisition.
 *
 * ## References
 * - Vulkan Tutorial: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Frames_in_flight
 * - Vulkan Spec: VUID-vkAcquireNextImageKHR-semaphore-01779
 */
class VkWindow final : public Window {
public:
    VkWindow(const std::string& title, int width, int height);
    ~VkWindow();

private:
    FrameGuard acquireFrame() override;

    void createSwapchain();
    void cleanupSwapchain();
    void recreateSwapchain();
    void createSyncObjects();

    /// Performs one-time layout transition of all swapchain images from UNDEFINED to PRESENT_SRC_KHR.
    /// Called after swapchain creation to ensure images match the default layout we told SnapRHI about.
    void transitionSwapchainImagesToPresent();

private:
    snap::rhi::backend::vulkan::Device* vkDevice = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<std::shared_ptr<snap::rhi::Texture>> swapchainTextures;
    VkExtent2D swapchainExtent{};
    VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;

    // =========================================================================
    // Frame-Based Synchronization Primitives
    // =========================================================================
    // These are indexed by currentFrameIndex, NOT by swapchain image index.
    // This is critical: we need to wait on a frame's fence before reusing its
    // semaphores, and we don't know the image index until after acquisition.

    /// Semaphores signaled by vkAcquireNextImageKHR when image is ready for rendering.
    std::vector<std::shared_ptr<snap::rhi::Semaphore>> imageAvailableSemaphores;

    /// Semaphores signaled when rendering completes, waited on by vkQueuePresentKHR.
    std::vector<std::shared_ptr<snap::rhi::Semaphore>> renderFinishedSemaphores;

    /// Fences for CPU-GPU synchronization. Waited on before reusing frame resources.
    std::vector<std::shared_ptr<snap::rhi::Fence>> inFlightFences;

    /// Tracks whether each frame's fence has been submitted (avoids waiting on unsignaled fence).
    std::vector<bool> fenceSubmitted;

    /// Current frame index [0, maxFramesInFlight). Rotates independently of image index.
    uint32_t currentFrameIndex = 0;

    /// The swapchain image index returned by vkAcquireNextImageKHR for the current frame.
    uint32_t currentImageIndex = 0;

    /// Maximum number of frames that can be in-flight simultaneously.
    uint32_t maxFramesInFlight = 0;
};
} // namespace snap::rhi::demo
