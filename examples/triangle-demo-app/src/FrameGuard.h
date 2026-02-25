#pragma once

#include <functional>
#include <memory>
#include <snap/rhi/CommandBuffer.hpp>
#include <snap/rhi/Texture.hpp>
#include <utility>

namespace snap::rhi::demo {

/**
 * @class FrameGuard
 * @brief RAII guard that manages a frame's command buffer and render target.
 *
 * This class encapsulates the lifetime of a single frame's rendering resources.
 * When the guard goes out of scope (or is explicitly released), it automatically
 * submits the command buffer and presents the frame.
 *
 * ## Design Principles
 *
 * 1. **RAII (Resource Acquisition Is Initialization)**: The frame resources are valid
 *    for the lifetime of the guard. Submission happens automatically on destruction.
 *
 * 2. **Move-Only Semantics**: FrameGuard is non-copyable but moveable, ensuring
 *    exactly one submission per acquired frame.
 *
 * 3. **Exception Safety**: If an exception occurs during rendering, the destructor
 *    still attempts to submit (or clean up) gracefully.
 *
 * ## Usage Example
 * ```cpp
 * {
 *     auto frame = window.acquireFrame();
 *     if (!frame) return; // Handle acquisition failure
 *
 *     renderer.render(frame.getCommandBuffer(), frame.getRenderTarget());
 *     // Automatic submission when 'frame' goes out of scope
 * }
 * ```
 *
 * ## Thread Safety
 * Not thread-safe. A single FrameGuard should only be used from one thread.
 */
class FrameGuard final {
public:
    /// Callback type for frame submission. Called when the guard is destroyed.
    using SubmitCallback =
        std::function<void(std::shared_ptr<snap::rhi::CommandBuffer>, std::shared_ptr<snap::rhi::Texture>)>;

    /**
     * @brief Constructs a FrameGuard with the given resources and submission callback.
     * @param commandBuffer The command buffer for recording rendering commands.
     * @param renderTarget The texture to render into (typically a swapchain image).
     * @param onSubmit Callback invoked on destruction to submit the frame.
     */
    FrameGuard(std::shared_ptr<snap::rhi::CommandBuffer> commandBuffer,
               std::shared_ptr<snap::rhi::Texture> renderTarget,
               SubmitCallback onSubmit)
        : commandBuffer_(std::move(commandBuffer)),
          renderTarget_(std::move(renderTarget)),
          onSubmit_(std::move(onSubmit)) {}

    /// Default constructor creates an invalid/empty guard.
    FrameGuard() = default;

    /// Move constructor - transfers ownership.
    FrameGuard(FrameGuard&& other) noexcept
        : commandBuffer_(std::move(other.commandBuffer_)),
          renderTarget_(std::move(other.renderTarget_)),
          onSubmit_(std::move(other.onSubmit_)) {
        other.onSubmit_ = nullptr; // Prevent moved-from object from submitting
    }

    /// Move assignment - transfers ownership.
    FrameGuard& operator=(FrameGuard&& other) noexcept {
        if (this != &other) {
            submit(); // Submit current frame if any
            commandBuffer_ = std::move(other.commandBuffer_);
            renderTarget_ = std::move(other.renderTarget_);
            onSubmit_ = std::move(other.onSubmit_);
            other.onSubmit_ = nullptr;
        }
        return *this;
    }

    /// Non-copyable
    FrameGuard(const FrameGuard&) = delete;
    FrameGuard& operator=(const FrameGuard&) = delete;

    /// Destructor - automatically submits the frame.
    ~FrameGuard() {
        submit();
    }

    /**
     * @brief Checks if this guard holds valid resources.
     * @return true if both command buffer and render target are valid.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return commandBuffer_ != nullptr && renderTarget_ != nullptr;
    }

    /**
     * @brief Gets the command buffer for recording rendering commands.
     * @return Shared pointer to the command buffer (may be null if guard is invalid).
     */
    [[nodiscard]] const std::shared_ptr<snap::rhi::CommandBuffer>& getCommandBuffer() const noexcept {
        return commandBuffer_;
    }

    /**
     * @brief Gets the render target texture.
     * @return Shared pointer to the render target (may be null if guard is invalid).
     */
    [[nodiscard]] const std::shared_ptr<snap::rhi::Texture>& getRenderTarget() const noexcept {
        return renderTarget_;
    }

    /**
     * @brief Explicitly submits the frame and releases resources.
     *
     * After calling this, the guard becomes invalid. This is automatically
     * called by the destructor, but can be called early if needed.
     */
    void submit() {
        if (onSubmit_ && commandBuffer_ && renderTarget_) {
            onSubmit_(std::move(commandBuffer_), std::move(renderTarget_));
        }
        onSubmit_ = nullptr;
        commandBuffer_ = nullptr;
        renderTarget_ = nullptr;
    }

private:
    std::shared_ptr<snap::rhi::CommandBuffer> commandBuffer_;
    std::shared_ptr<snap::rhi::Texture> renderTarget_;
    SubmitCallback onSubmit_;
};

} // namespace snap::rhi::demo
