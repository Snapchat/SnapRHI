//
// VkWindow.cpp
// SnapRHI Demo Application - Vulkan Window Implementation
//
// Copyright (c) 2026 Snap Inc. All rights reserved.
//
// This file implements a Vulkan-backed window for the SnapRHI demo application.
// It demonstrates how to integrate SnapRHI with a native Vulkan swapchain.
//
// Key Concepts:
// 1. Swapchain Management: Creates and manages a Vulkan swapchain for presentation.
// 2. External Texture Wrapping: Wraps swapchain VkImages as snap::rhi::Texture using the
//    Device::createTexture(info, VkImage, isOwner, layout) API.
// 3. Layout Management: By specifying VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as the default
//    layout when creating swapchain textures, SnapRHI will automatically transition
//    images to presentation-ready layout after rendering. This eliminates the need for
//    manual layout transitions in the presentation path.
// 4. Frame-Based Synchronization: Uses a rotating frame index independent of the swapchain
//    image index. This is essential because vkAcquireNextImageKHR requires a semaphore
//    with no pending operations, but we don't know which image we'll get until after
//    acquisition completes. By waiting on the FRAME's fence BEFORE acquisition, we
//    guarantee the semaphore is free for reuse.
//
// References:
// - Vulkan Tutorial: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Frames_in_flight
// - VUID-vkAcquireNextImageKHR-semaphore-01779
//

#include "VKWindow.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <snap/rhi/CommandQueue.hpp>
#include <snap/rhi/backend/common/Utils.hpp>
#include <snap/rhi/backend/vulkan/CommandQueue.h>
#include <snap/rhi/backend/vulkan/Device.h>
#include <snap/rhi/backend/vulkan/DeviceFactory.h>
#include <snap/rhi/backend/vulkan/Fence.h>
#include <snap/rhi/backend/vulkan/Semaphore.h>
#include <snap/rhi/backend/vulkan/Texture.h>

#include <algorithm>
#include <limits>
#include <stdexcept>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace snap::rhi::demo {
namespace {

// =============================================================================
// Constants
// =============================================================================

/// Preferred swapchain format. Falls back to the first available format if not supported.
constexpr VkFormat kSwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;

// =============================================================================
// SDL Vulkan Library Initialization
// =============================================================================

/**
 * @brief Ensures SDL loads the same Vulkan library that GLAD uses.
 *
 * When SDL creates a window with SDL_WINDOW_VULKAN, it auto-loads a Vulkan library
 * and caches the vkGetInstanceProcAddr from it. On macOS, SDL first tries
 * dlsym(RTLD_DEFAULT, "vkGetInstanceProcAddr") which may find MoltenVK's version
 * if MoltenVK is statically linked.
 *
 * However, GLAD's loader dynamically loads the Vulkan library and creates the
 * VkInstance through it. The Vulkan Loader wraps the VkInstance in a dispatch object.
 * If SDL uses MoltenVK's vkGetInstanceProcAddr with a Loader-created VkInstance,
 * MoltenVK will try to dereference the Loader's dispatch table as if it were its own
 * internal structure, causing EXC_BAD_ACCESS.
 *
 * By calling SDL_Vulkan_LoadLibrary with the same library name that GLAD uses
 * before creating the window, we ensure both GLAD and SDL resolve Vulkan functions
 * through the same library, avoiding the mismatch.
 *
 * Platform-specific behavior (must match glad_vulkan_dlopen_handle in vulkan.cpp):
 * - macOS:   Loads "libvulkan.1.dylib" (Vulkan Loader from Vulkan SDK / Homebrew)
 * - iOS (framework mode): Loads "vulkan.framework/vulkan" — the Vulkan Loader framework
 *            embedded in the app bundle. Enables validation layers via the loader.
 * - iOS (static mode): Passes NULL — MoltenVK is statically linked, SDL uses RTLD_DEFAULT
 *            to find vkGetInstanceProcAddr in the current process image.
 * - Windows: Loads "vulkan-1.dll"
 * - Android: Loads "libvulkan.so" (system Vulkan driver, no .1 suffix)
 * - Linux:   Loads "libvulkan.so.1"
 */
void ensureSDLVulkanLibraryLoaded() {
    // Load the same Vulkan library that GLAD uses (see glad_vulkan_dlopen_handle).
    // This must be called before SDL_CreateWindow with SDL_WINDOW_VULKAN flag,
    // otherwise SDL will auto-detect and may pick a different Vulkan implementation.
#if defined(__APPLE__) && TARGET_OS_IOS
#if SNAP_RHI_IOS_VULKAN_USE_FRAMEWORKS
    // iOS framework mode: Load the embedded Vulkan Loader framework.
    const char* vulkanLibrary = "@rpath/vulkan.framework/vulkan";
#else
    // iOS static mode: MoltenVK is statically linked. Pass NULL so SDL resolves
    // Vulkan symbols from the current process (RTLD_DEFAULT).
    const char* vulkanLibrary = nullptr;
#endif
#elif defined(__APPLE__)
    const char* vulkanLibrary = "libvulkan.1.dylib";
#elif defined(_WIN32)
    const char* vulkanLibrary = "vulkan-1.dll";
#elif defined(__ANDROID__)
    const char* vulkanLibrary = "libvulkan.so";
#else
    const char* vulkanLibrary = "libvulkan.so.1";
#endif

    if (!SDL_Vulkan_LoadLibrary(vulkanLibrary)) {
        throw std::runtime_error(std::string("Failed to load Vulkan library for SDL: ") + SDL_GetError());
    }
}

// =============================================================================
// Swapchain Configuration Helpers
// =============================================================================

/**
 * @brief Selects the best available surface format for the swapchain.
 * @param availableFormats List of formats supported by the surface.
 * @return The preferred format (B8G8R8A8_UNORM with SRGB color space) or the first available.
 */
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& format : availableFormats) {
        if (format.format == kSwapchainFormat && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return availableFormats[0];
}

/**
 * @brief Selects the best available presentation mode.
 * @param availableModes List of presentation modes supported by the surface.
 * @return MAILBOX if available (low-latency triple buffering), otherwise FIFO (vsync).
 */
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes) {
    for (const auto& mode : availableModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

/**
 * @brief Determines the optimal swapchain extent (resolution).
 * @param capabilities Surface capabilities containing min/max extent constraints.
 * @param width Requested width in pixels.
 * @param height Requested height in pixels.
 * @return The clamped extent within the surface's supported range.
 */
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    VkExtent2D extent = {width, height};
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return extent;
}

/**
 * @brief Selects a supported composite alpha mode for the swapchain.
 * @param capabilities Surface capabilities containing supported composite alpha flags.
 * @return The first supported mode from the preferred order (OPAQUE > PRE_MULTIPLIED > POST_MULTIPLIED > INHERIT).
 * @note Some platforms (e.g., Android) may only support INHERIT.
 */
VkCompositeAlphaFlagBitsKHR chooseCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities) {
    const std::array<VkCompositeAlphaFlagBitsKHR, 4> preferredOrder = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (auto alpha : preferredOrder) {
        if (capabilities.supportedCompositeAlpha & alpha) {
            return alpha;
        }
    }
    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

/**
 * @brief Converts a Vulkan format to SnapRHI PixelFormat.
 * @param format The Vulkan format to convert.
 * @return The corresponding snap::rhi::PixelFormat, or B8G8R8A8Unorm as fallback.
 */
snap::rhi::PixelFormat vkFormatToPixelFormat(VkFormat format) {
    switch (format) {
        case VK_FORMAT_B8G8R8A8_UNORM:
            return snap::rhi::PixelFormat::B8G8R8A8Unorm;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return snap::rhi::PixelFormat::R8G8B8A8Unorm;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return snap::rhi::PixelFormat::R8G8B8A8Srgb;
        default:
            return snap::rhi::PixelFormat::B8G8R8A8Unorm;
    }
}
} // namespace

// =============================================================================
// VkWindow Implementation
// =============================================================================

VkWindow::VkWindow(const std::string& title, int width, int height)
    : Window(title, width, height, SDL_WINDOW_VULKAN, ensureSDLVulkanLibraryLoaded) {
    // --- Create SnapRHI Device ---
    // Configure the Vulkan device with validation enabled and swapchain extension.
    snap::rhi::backend::vulkan::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.apiVersion = snap::rhi::backend::vulkan::APIVersion::Vulkan_1_1;
    deviceCreateInfo.enabledReportLevel = snap::rhi::ReportLevel::All;
    deviceCreateInfo.enabledTags = snap::rhi::ValidationTag::All;

    // Get required instance extensions from SDL for surface creation
    uint32_t extensionCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    deviceCreateInfo.enabledInstanceExtensions = std::span<const char* const>{sdlExtensions, extensionCount};

    // Enable swapchain extension for presentation
    std::array enabledDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    deviceCreateInfo.enabledDeviceExtensions = enabledDeviceExtensions;

    device = snap::rhi::backend::vulkan::createDevice(deviceCreateInfo);
    vkDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Device>(device.get());

    // Cache native Vulkan handles for direct Vulkan API calls
    instance = vkDevice->getVkInstance();
    physicalDevice = vkDevice->getVkPhysicalDevice();
    logicalDevice = vkDevice->getVkLogicalDevice();

    // --- Create Surface ---
    // SDL creates the platform-specific surface (Wayland, X11, Android, etc.)
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        throw std::runtime_error(std::string("Failed to create Vulkan surface: ") + SDL_GetError());
    }

    // --- Get Command Queue ---
    commandQueue = device->getCommandQueue(0, 0);

    // --- Create Swapchain and Synchronization Objects ---
    createSwapchain();
    createSyncObjects();

    // --- Create Demo Renderer ---
    triangleRenderer = std::make_unique<TriangleRenderer>(device);
}

VkWindow::~VkWindow() {
    // Wait for all GPU work to complete before destroying resources
    vkDeviceWaitIdle(logicalDevice);

    // Release SnapRHI resources first (they may reference Vulkan objects)
    triangleRenderer.reset();

    // Cleanup swapchain, textures, and per-image sync objects
    cleanupSwapchain();

    // Surface must be destroyed before the device (which owns the instance)
    vkDestroySurfaceKHR(instance, surface, nullptr);

    // Release the SnapRHI device (destroys Vulkan instance/device)
    device.reset();
}

// =============================================================================
// Swapchain Management
// =============================================================================

void VkWindow::createSwapchain() {
    // --- Query Surface Capabilities ---
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    // --- Query Supported Formats ---
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

    // --- Query Supported Present Modes ---
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

    // --- Select Optimal Configuration ---
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
    swapchainExtent = chooseSwapExtent(capabilities, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    swapchainFormat = surfaceFormat.format;

    // Request one more image than minimum for triple buffering
    maxFramesInFlight = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && maxFramesInFlight > capabilities.maxImageCount) {
        maxFramesInFlight = capabilities.maxImageCount;
    }

    // --- Create Swapchain ---
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = maxFramesInFlight;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = chooseCompositeAlpha(capabilities);
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain");
    }

    // --- Get Swapchain Images ---
    vkGetSwapchainImagesKHR(logicalDevice, swapchain, &maxFramesInFlight, nullptr);
    swapchainImages.resize(maxFramesInFlight);
    vkGetSwapchainImagesKHR(logicalDevice, swapchain, &maxFramesInFlight, swapchainImages.data());

    // --- Wrap Swapchain Images as SnapRHI Textures ---
    // We use createTexture with external VkImage to wrap swapchain images.
    // Parameters:
    //   - isOwner=false: SnapRHI won't destroy the VkImage (swapchain owns them)
    //   - defaultLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: This is the key insight!
    //     By telling SnapRHI that the default layout is PRESENT_SRC_KHR,
    //     it will automatically transition images to this layout after rendering.
    //     This eliminates the need for manual layout transitions before presentation.
    //
    // Note: Swapchain images start in VK_IMAGE_LAYOUT_UNDEFINED, so we must perform
    // an initial transition to PRESENT_SRC_KHR before first use.
    snap::rhi::TextureCreateInfo textureDesc{
        .size = {swapchainExtent.width, swapchainExtent.height, 1},
        .mipLevels = 1,
        .textureType = snap::rhi::TextureType::Texture2D,
        .textureUsage = snap::rhi::TextureUsage::ColorAttachment | snap::rhi::TextureUsage::TransferDst,
        .sampleCount = snap::rhi::SampleCount::Count1,
        .format = vkFormatToPixelFormat(swapchainFormat),
    };

    swapchainTextures.reserve(maxFramesInFlight);
    for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
        swapchainTextures.push_back(
            vkDevice->createTexture(textureDesc, swapchainImages[i], false, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
    }

    // --- Initial Layout Transition ---
    // Swapchain images are created in UNDEFINED layout. We must transition them to
    // PRESENT_SRC_KHR to match the default layout we told SnapRHI about.
    // This is a one-time operation per swapchain creation.
    transitionSwapchainImagesToPresent();
}

void VkWindow::transitionSwapchainImagesToPresent() {
    // Create a temporary command buffer for the initial layout transitions
    auto* vkCommandQueue =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandQueue>(commandQueue);
    VkQueue queue = vkCommandQueue->getQueue();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = 0;

    VkCommandPool tempPool;
    if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &tempPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create temporary command pool");
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = tempPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(logicalDevice, &allocInfo, &cmdBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    // Transition all swapchain images from UNDEFINED to PRESENT_SRC_KHR
    std::vector<VkImageMemoryBarrier> barriers(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); ++i) {
        barriers[i] = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = swapchainImages[i],
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };
    }

    vkCmdPipelineBarrier(cmdBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         static_cast<uint32_t>(barriers.size()),
                         barriers.data());

    vkEndCommandBuffer(cmdBuffer);

    // Submit and wait for completion
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    // Cleanup temporary resources
    vkDestroyCommandPool(logicalDevice, tempPool, nullptr);
}

void VkWindow::cleanupSwapchain() {
    // Release SnapRHI texture wrappers first (they reference swapchain images)
    swapchainTextures.clear();

    // Clear per-frame sync objects (they're recreated with the swapchain)
    imageAvailableSemaphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();
    fenceSubmitted.clear();

    // Destroy the swapchain (this releases the VkImages)
    if (swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }
    swapchainImages.clear();
}

void VkWindow::recreateSwapchain() {
    // Handle minimization - wait until window has non-zero size
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    while (w == 0 || h == 0) {
        SDL_GetWindowSizeInPixels(window, &w, &h);
        SDL_WaitEvent(nullptr);
    }
    width = w;
    height = h;

    // Wait for GPU to finish all operations before recreating swapchain
    vkDeviceWaitIdle(logicalDevice);
    cleanupSwapchain();
    createSwapchain();
    createSyncObjects();
}

// =============================================================================
// Synchronization
// =============================================================================

void VkWindow::createSyncObjects() {
    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(maxFramesInFlight);
    inFlightFences.resize(maxFramesInFlight);
    fenceSubmitted.resize(maxFramesInFlight, false);

    snap::rhi::SemaphoreCreateInfo semaphoreInfo{};
    snap::rhi::FenceCreateInfo fenceInfo{};

    for (size_t i = 0; i < maxFramesInFlight; ++i) {
        imageAvailableSemaphores[i] = device->createSemaphore(semaphoreInfo, nullptr);
        renderFinishedSemaphores[i] = device->createSemaphore(semaphoreInfo, nullptr);
        inFlightFences[i] = device->createFence(fenceInfo);
    }

    // Reset frame index on swapchain recreation
    currentFrameIndex = 0;
}

// =============================================================================
// Frame Acquisition and Presentation
// =============================================================================

FrameGuard VkWindow::acquireFrame() {
    // =========================================================================
    // Frame-Based Image Acquisition
    // =========================================================================
    //
    // Critical ordering:
    // 1. Wait on THIS FRAME's fence BEFORE calling vkAcquireNextImageKHR
    //    - This ensures this frame's semaphores are no longer in use
    //    - We're waiting for frame (currentFrameIndex - maxFramesInFlight) to complete
    //
    // 2. Reset the fence AFTER waiting (makes it available for the new submission)
    //
    // 3. Call vkAcquireNextImageKHR with this frame's imageAvailableSemaphore
    //    - Now we know the semaphore has no pending operations (per Vulkan spec)
    //
    // 4. The acquired image index may be ANY valid swapchain image
    //    - This is why we use frame-based sync, not image-based
    //
    // Note: On first few frames, fences haven't been submitted yet, so we skip waiting.

    // Step 1: Wait for this frame slot to be free
    if (fenceSubmitted[currentFrameIndex]) {
        inFlightFences[currentFrameIndex]->waitForComplete();
        inFlightFences[currentFrameIndex]->reset();
        fenceSubmitted[currentFrameIndex] = false;
    }

    // Step 2: Get this frame's semaphore (now guaranteed to have no pending operations)
    auto* vkSemaphore = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Semaphore>(
        imageAvailableSemaphores[currentFrameIndex].get());

    // Step 3: Acquire the next swapchain image
    VkResult result = vkAcquireNextImageKHR(logicalDevice,
                                            swapchain,
                                            std::numeric_limits<uint64_t>::max(),
                                            vkSemaphore->getSemaphore(),
                                            VK_NULL_HANDLE,
                                            &currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return acquireFrame(); // Retry with new swapchain
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return {}; // Return invalid guard on failure
    }

    // Step 4: Create command buffer for this frame
    auto commandBuffer = device->createCommandBuffer({
        .commandQueue = commandQueue,
    });

    auto renderTarget = swapchainTextures[currentImageIndex];

    // Capture the frame index for the submit callback (it may change by the time submit is called)
    const uint32_t frameIndex = currentFrameIndex;
    const uint32_t imageIndex = currentImageIndex;

    // Create the submit callback that will be invoked when the FrameGuard is destroyed
    auto submitCallback = [this, frameIndex, imageIndex](std::shared_ptr<snap::rhi::CommandBuffer> cmdBuffer,
                                                         std::shared_ptr<snap::rhi::Texture> /*target*/) {
        // =====================================================================
        // Frame Submission and Presentation
        // =====================================================================
        //
        // Synchronization flow for this frame:
        // 1. GPU waits on imageAvailableSemaphore[frameIndex] before rendering
        //    - This semaphore was signaled by vkAcquireNextImageKHR
        // 2. GPU signals renderFinishedSemaphore[frameIndex] after rendering
        // 3. GPU signals inFlightFences[frameIndex] after rendering
        //    - CPU will wait on this fence before reusing this frame's resources
        // 4. vkQueuePresentKHR waits on renderFinishedSemaphore[frameIndex]
        // 5. Advance currentFrameIndex for the next frame
        //
        // Note: SnapRHI automatically transitions the swapchain image to
        // PRESENT_SRC_KHR because we specified that as the default layout.

        // Submit rendering commands with this frame's synchronization primitives
        snap::rhi::Semaphore* waitSemaphores[] = {imageAvailableSemaphores[frameIndex].get()};
        snap::rhi::Semaphore* signalSemaphores[] = {renderFinishedSemaphores[frameIndex].get()};
        snap::rhi::CommandBuffer* cmdBuffers[] = {cmdBuffer.get()};
        snap::rhi::PipelineStageBits waitStages[] = {snap::rhi::PipelineStageBits::ColorAttachmentOutputBit};

        commandQueue->submitCommands(waitSemaphores,
                                     waitStages,
                                     cmdBuffers,
                                     signalSemaphores,
                                     snap::rhi::CommandBufferWaitType::DoNotWait,
                                     inFlightFences[frameIndex].get());

        // Mark this frame's fence as submitted (so we'll wait on it when this slot is reused)
        fenceSubmitted[frameIndex] = true;

        // Present the rendered image
        auto* vkSignalSemaphore = snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::Semaphore>(
            renderFinishedSemaphores[frameIndex].get());
        VkSemaphore signalSemaphore = vkSignalSemaphore->getSemaphore();

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &signalSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;

        auto* vkCommandQueue =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::CommandQueue>(commandQueue);
        VkQueue queue = vkCommandQueue->getQueue();

        VkResult presentResult = vkQueuePresentKHR(queue, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            // Swapchain needs recreation - will be handled on next acquireFrame()
        } else if (presentResult != VK_SUCCESS) {
            // Log error but don't throw from callback
        }

        // Advance to the next frame slot (wraps around)
        currentFrameIndex = (currentFrameIndex + 1) % maxFramesInFlight;
    };

    return FrameGuard(std::move(commandBuffer), std::move(renderTarget), std::move(submitCallback));
}
} // namespace snap::rhi::demo
