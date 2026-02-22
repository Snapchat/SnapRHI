#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>

#include "snap/rhi/BufferCreateInfo.h"
#include "snap/rhi/Capabilities.h"
#include "snap/rhi/CommandBufferCreateInfo.h"
#include "snap/rhi/ComputePipelineCreateInfo.h"
#include "snap/rhi/DebugMessengerCreateInfo.h"
#include "snap/rhi/DescriptorPoolCreateInfo.h"
#include "snap/rhi/DescriptorSetCreateInfo.h"
#include "snap/rhi/DescriptorSetLayoutCreateInfo.h"
#include "snap/rhi/DeviceContext.hpp"
#include "snap/rhi/DeviceCreateInfo.h"
#include "snap/rhi/Enums.h"
#include "snap/rhi/FenceCreateInfo.h"
#include "snap/rhi/FramebufferCreateInfo.h"
#include "snap/rhi/PipelineCacheCreateInfo.h"
#include "snap/rhi/PipelineLayoutCreateInfo.h"
#include "snap/rhi/QueryPoolCreateInfo.h"
#include "snap/rhi/RenderPassCreateInfo.h"
#include "snap/rhi/RenderPipelineCreateInfo.h"
#include "snap/rhi/SamplerCreateInfo.h"
#include "snap/rhi/SemaphoreCreateInfo.h"
#include "snap/rhi/ShaderLibraryCreateInfo.h"
#include "snap/rhi/TextureCreateInfo.h"
#include "snap/rhi/TextureViewCreateInfo.h"
#include "snap/rhi/common/OS.h"
#include "snap/rhi/reflection/ComputePipelineInfo.h"
#include "snap/rhi/reflection/RenderPipelineInfo.h"
#include <span>

namespace snap::rhi {
class DeviceContext;
class CommandQueue;
class CommandBuffer;
class Texture;
class Buffer;
class Framebuffer;
class RenderPass;
class ShaderLibrary;
class ShaderModule;
class RenderPipeline;
class ComputePipeline;
class Sampler;
class Semaphore;
class Fence;
class PipelineCache;
class DescriptorPool;
class TextureInterop;
class DescriptorSet;
class DescriptorSetLayout;
class DebugMessenger;
class PipelineLayout;
class QueryPool;
class PlatformSyncHandle;

/**
 * @brief Graphics device interface.
 *
 * `snap::rhi::Device` is the main factory for GPU resources (buffers, textures, pipelines, etc.) and the entry point
 * for backend capabilities and validation.
 *
 * ## Thread-safety
 * The device object is generally safe to call from multiple threads for *resource creation* unless a method explicitly
 * states otherwise. The objects returned by this interface have their own thread-safety rules (for example,
 * `snap::rhi::CommandQueue` and `snap::rhi::DeviceContext` are not thread-safe).
 *
 * ## Lifetime / ownership
 * Resources created by the device are device-children and are tracked internally. All resources must be released
 * (all `std::shared_ptr` copies destroyed) before destroying the device.
 *
 * ## Error model
 * SnapRHI uses validation reporting extensively. In addition:
 * - Some methods may return `nullptr` on failure (notably descriptor set allocation and some pipeline creation modes).
 * - Some methods may throw on unsupported functionality on a given backend (for example, OpenGL `createTextureView`).
 *
 * Always consult the per-method documentation for backend-specific behavior.
 */
class Device : public std::enable_shared_from_this<Device> {
public:
    /**
     * @brief Constructs a device with the provided configuration.
     *
     * @param info Device creation parameters (API/backend selection, validation settings, etc.).
     */
    Device(const DeviceCreateInfo& info);

    /**
     * @brief Destroys the device.
     *
     * @warning All resources created from this device must be destroyed before destroying the device.
     * Backends validate this invariant and may report a critical error if resources are still alive.
     *
     * @note Destruction should be externally synchronized with any ongoing GPU work.
     */
    virtual ~Device() noexcept(false);

    /**
     * @brief Returns a command queue for the given queue family and index.
     *
     * @param queueFamilyIndex Queue family index.
     * @param queueIndex Queue index within the family.
     *
     * @return A pointer to a device-owned queue instance.
     *
     * @thread_safety This function is thread-safe. The returned `snap::rhi::CommandQueue` is thread-unsafe; callers
     * must synchronize queue submission and `waitIdle()` calls (see Vulkan "Host Synchronization" rules).
     *
     * @note Invalid indices are reported via validation.
     */
    virtual snap::rhi::CommandQueue* getCommandQueue(const uint32_t queueFamilyIndex, const uint32_t queueIndex) = 0;

    /**
     * @brief Returns the device main context (primarily relevant for OpenGL backends).
     *
     * @return A raw pointer to a device-owned context.
     *
     * @thread_safety Not thread-safe.
     */
    virtual snap::rhi::DeviceContext* getMainDeviceContext() = 0;

    /**
     * @brief Creates a new device context.
     *
     * This is typically used by OpenGL backends to create additional shared contexts for worker threads.
     *
     * @param dcCreateInfo Context creation info.
     * @return A shared pointer to the created context.
     *
     * @thread_safety Not thread-safe.
     * @note Avoid making the same context current on multiple threads.
     */
    virtual std::shared_ptr<snap::rhi::DeviceContext> createDeviceContext(
        const snap::rhi::DeviceContextCreateInfo& dcCreateInfo) = 0;

    /**
     * @brief Makes the provided context current for the calling thread and returns an RAII guard.
     *
     * When the returned guard is destroyed, the previous context is restored.
     *
     * @param ptr Context to make current, or `nullptr` to release the current context.
     * @return `snap::rhi::DeviceContext::Guard` that restores the previous current context on destruction.
     *
     * @thread_safety Not thread-safe. Context binding is typically per-thread and must be externally synchronized.
     */
    [[nodiscard]] virtual snap::rhi::DeviceContext::Guard setCurrent(DeviceContext* ptr) = 0;

    /**
     * @brief Returns the currently bound context for the calling thread.
     *
     * @return Pointer to the current context, or `nullptr` if no context is current.
     *
     * @thread_safety Not thread-safe.
     */
    virtual snap::rhi::DeviceContext* getCurrentDeviceContext() = 0;

    /**
     * @brief Returns whether a native context is currently attached to the calling thread.
     */
    [[nodiscard]] virtual bool isNativeContextAttached() const = 0;

    /**
     * @brief Creates a command buffer.
     *
     * Command buffers are used to record GPU commands for later submission.
     *
     * @param info Command buffer creation info.
     * @return A shared pointer to the created command buffer.
     *
     * @thread_safety Recording into a command buffer is not thread-safe. Use a separate command buffer per thread.
     */
    virtual std::shared_ptr<CommandBuffer> createCommandBuffer(const CommandBufferCreateInfo& info) = 0;

    /**
     * @brief Creates an immutable sampler object.
     *
     * @param info Sampler creation info.
     * @return A shared pointer to the created sampler.
     *
     * @thread_safety The created sampler is immutable and safe to share between threads.
     */
    virtual std::shared_ptr<Sampler> createSampler(const SamplerCreateInfo& info) = 0;

    /**
     * @brief Creates a query pool for timestamp queries.
     *
     * @param info Query pool creation info.
     * @return A shared pointer to the created query pool instance.
     */
    virtual std::shared_ptr<QueryPool> createQueryPool(const QueryPoolCreateInfo& info) = 0;

    /**
     * @brief Creates a semaphore, optionally importing a platform synchronization handle.
     *
     * @param info Semaphore creation info.
     * @param platformSyncHandle Optional platform handle to import/synchronize with.
     * @return A shared pointer to the created semaphore.
     *
     * @thread_safety Semaphore objects are thread-unsafe.
     *
     * @warning A semaphore acts like a single-use handoff: one signal completes one wait. Ensure submissions are
     * ordered correctly (signal before wait) or the GPU may hang.
     *
     * @note On some backends/platforms, providing a `platformSyncHandle` may cause the implementation to wait on an
     * associated CPU fence before creating/returning the semaphore.
     */
    virtual std::shared_ptr<Semaphore> createSemaphore(
        const SemaphoreCreateInfo& info,
        const std::shared_ptr<snap::rhi::PlatformSyncHandle>& platformSyncHandle = nullptr) = 0;

    /**
     * @brief Creates a fence for CPU↔GPU synchronization.
     *
     * @param info Fence creation info.
     * @return A shared pointer to the created fence.
     *
     * @thread_safety Fence objects are thread-unsafe.
     */
    virtual std::shared_ptr<Fence> createFence(const FenceCreateInfo& info) = 0;

    /**
     * @brief Creates a buffer.
     *
     * @param info Buffer creation info (size, usage, memory properties, etc.).
     * @return A shared pointer to the created buffer.
     *
     * @thread_safety The returned buffer object is not inherently thread-safe. The host must synchronize map/unmap
     * operations and CPU-side access. The application must also synchronize GPU access across different command
     * buffers/queues.
     *
     * @note Backends may validate certain usage constraints (for example, OpenGL validates SSBO support and UBO size
     * alignment).
     */
    virtual std::shared_ptr<Buffer> createBuffer(const BufferCreateInfo& info) = 0;

    /**
     * @brief Creates a new device-local texture.
     *
     * @param info Texture creation info.
     * @return A shared pointer to the created texture.
     *
     * @thread_safety The application must synchronize texture usage across different command buffers
     * (draw/read/upload).
     * @note Unsupported formats are reported via validation.
     */
    virtual std::shared_ptr<Texture> createTexture(const TextureCreateInfo& info) = 0;

    /**
     * @brief Creates a texture from a platform/native interop object.
     *
     * @param textureInterop Interop wrapper containing a backend/platform-specific texture.
     * @return A shared pointer to the created texture.
     *
     * @note Synchronization between the external producer/consumer and SnapRHI is the caller’s responsibility.
     */
    virtual std::shared_ptr<Texture> createTexture(const std::shared_ptr<TextureInterop>& textureInterop) = 0;

    /**
     * @brief Creates a texture view (alias) into an existing texture.
     *
     * @param info Texture view creation info.
     * @return A shared pointer to the created view.
     *
     * @warning Backend support differs. On OpenGL this operation is not supported and the implementation may throw.
     */
    virtual std::shared_ptr<Texture> createTextureView(const TextureViewCreateInfo& info) = 0;

    /**
     * @brief Creates a render pass object.
     *
     * @param info Render pass creation info.
     * @return A shared pointer to the created render pass.
     */
    virtual std::shared_ptr<RenderPass> createRenderPass(const RenderPassCreateInfo& info) = 0;

    /**
     * @brief Creates a framebuffer.
     *
     * @param info Framebuffer creation info.
     * @return A shared pointer to the created framebuffer.
     *
     * @warning The framebuffer does not retain references to the attachment textures. The caller must keep the
     * attachment textures alive while the framebuffer is used.
     *
     * @warning The render pass must outlive any framebuffer created from it.
     */
    virtual std::shared_ptr<Framebuffer> createFramebuffer(const FramebufferCreateInfo& info) = 0;

    /**
     * @brief Creates a shader library.
     *
     * A shader library can contain multiple shader permutations / variants depending on backend.
     *
     * @param info Shader library creation info.
     * @return A shared pointer to the created shader library.
     *
     * @thread_safety Shader libraries are generally not thread-safe (backend-dependent).
     */
    virtual std::shared_ptr<ShaderLibrary> createShaderLibrary(const ShaderLibraryCreateInfo& info) = 0;

    /**
     * @brief Creates an immutable shader module.
     *
     * @param info Shader module creation info.
     * @return A shared pointer to the created shader module.
     *
     * @thread_safety Shader modules are immutable after creation and safe to share across threads.
     */
    virtual std::shared_ptr<ShaderModule> createShaderModule(const ShaderModuleCreateInfo& info) = 0;

    /**
     * @brief Creates a descriptor set layout.
     *
     * @param info Descriptor set layout creation info.
     * @return A shared pointer to the created layout.
     */
    virtual std::shared_ptr<DescriptorSetLayout> createDescriptorSetLayout(
        const DescriptorSetLayoutCreateInfo& info) = 0;

    /**
     * @brief Creates a descriptor pool.
     *
     * @param info Descriptor pool creation info.
     * @return A shared pointer to the created pool.
     */
    virtual std::shared_ptr<DescriptorPool> createDescriptorPool(const DescriptorPoolCreateInfo& info) = 0;

    /**
     * @brief Allocates a descriptor set.
     *
     * @param info Descriptor set allocation info.
     * @return The allocated descriptor set, or `nullptr` if allocation failed.
     *
     * @note Some backends return `nullptr` when native allocation fails (for example, Vulkan `VkDescriptorSet ==
     * VK_NULL_HANDLE` or Metal argument-buffer allocation failure).
     */
    virtual std::shared_ptr<DescriptorSet> createDescriptorSet(const DescriptorSetCreateInfo& info) = 0;

    /**
     * @brief Creates a pipeline layout.
     *
     * Pipeline layouts define the ordered descriptor set layouts used by a pipeline.
     *
     * @param info Pipeline layout creation info.
     * @return A shared pointer to the created pipeline layout.
     *
     * @warning All descriptor-set indices referenced by shader modules must be present in the layout.
     * If a shader uses `set = N`, then `pSetLayouts[N]` must exist.
     */
    virtual std::shared_ptr<snap::rhi::PipelineLayout> createPipelineLayout(
        const snap::rhi::PipelineLayoutCreateInfo& info) = 0;

    /**
     * @brief Creates a pipeline cache used to speed up pipeline creation.
     *
     * @param info Pipeline cache creation info.
     * @return A shared pointer to the created pipeline cache.
     *
     * @thread_safety Pipeline caches are safe to use concurrently for pipeline creation.
     */
    virtual std::shared_ptr<PipelineCache> createPipelineCache(const PipelineCacheCreateInfo& info) = 0;

    /**
     * @brief Creates a render pipeline.
     *
     * @param info Render pipeline creation info.
     * @return The created pipeline, or `nullptr` if creation failed.
     *
     * @note Backend behavior differs. On Metal, when `PipelineCreateFlags::NullOnPipelineCacheMiss` is set and a
     * pipeline binary archive miss occurs, this function returns `nullptr`.
     */
    virtual std::shared_ptr<RenderPipeline> createRenderPipeline(const RenderPipelineCreateInfo& info) = 0;

    /**
     * @brief Creates a compute pipeline.
     *
     * @param info Compute pipeline creation info.
     * @return The created pipeline, or `nullptr` if creation failed.
     *
     * @note Backend behavior differs. On Metal, when `PipelineCreateFlags::NullOnPipelineCacheMiss` is set and a
     * pipeline binary archive miss occurs, this function returns `nullptr`.
     */
    virtual std::shared_ptr<ComputePipeline> createComputePipeline(const ComputePipelineCreateInfo& info) = 0;

    /**
     * @brief Blocks until all tracked submissions have completed.
     *
     * Use this to synchronize tracked work before resource destruction.
     *
     * @warning This does NOT wait for untracked/internal work.
     * @note Call this before destroying the device to ensure resources are no longer in use.
     */
    virtual void waitForTrackedSubmissions() = 0;

    /**
     * @brief Returns device capabilities determined at initialization.
     */
    const Capabilities& getCapabilities() const {
        return caps;
    }

    /**
     * @brief Returns the total CPU memory usage accounted to live device resources.
     *
     * @return Bytes of CPU memory used by currently live resources.
     * @note Some backends may not report CPU memory usage and return 0.
     */
    virtual uint64_t getCPUMemoryUsage() const = 0;

    /**
     * @brief Returns the total GPU memory usage accounted to live device resources.
     *
     * @return Bytes of GPU memory used by currently live resources.
     * @note Some backends may not report GPU memory usage and return 0.
     */
    virtual uint64_t getGPUMemoryUsage() const = 0;

    /**
     * @brief Returns a human-readable string describing the underlying platform device.
     */
    const std::string& getPlatformDeviceString() const;

    /**
     * @brief Returns the immutable creation info used to build this device.
     */
    const DeviceCreateInfo& getDeviceCreateInfo() const {
        return createInfo;
    }

    /**
     * @brief Captures the current state of all allocations and generates a memory report.
     *
     * @return A snapshot containing hierarchical memory usage data for CPU and GPU.
     * @note This operation may be expensive; avoid calling it every frame.
     */
    virtual DeviceMemorySnapshot captureMemorySnapshot() const = 0;

    /**
     * @brief Creates a debug messenger that receives backend debug messages.
     *
     * @param info Debug messenger configuration; ownership of callbacks is managed by the returned messenger.
     * @return A messenger instance, or `nullptr` if unsupported on the current backend.
     *
     * @thread_safety Callbacks may be invoked from arbitrary threads; the callback implementation must be thread-safe.
     */
    virtual std::shared_ptr<snap::rhi::DebugMessenger> createDebugMessenger(
        snap::rhi::DebugMessengerCreateInfo&& info) = 0;

    /**
     * @brief Queries format/usage-specific limits for textures.
     *
     * @param info Format query info (format, type, usage).
     * @return Properties supported by the backend for the given configuration.
     *
     * @note On Vulkan, this queries the physical device image-format properties and may report errors via validation.
     */
    virtual TextureFormatProperties getTextureFormatProperties(const TextureFormatInfo& info) = 0;

protected:
    std::string platformDeviceString;

    Capabilities caps{};

    DeviceCreateInfo createInfo{};
};
} // namespace snap::rhi
