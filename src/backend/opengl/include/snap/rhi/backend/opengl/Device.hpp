#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "snap/rhi/Compare.hpp"
#include "snap/rhi/Sampler.hpp"
#include "snap/rhi/backend/common/Device.hpp"

#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/backend/opengl/CommandBufferPool.hpp"
#include "snap/rhi/backend/opengl/CommandQueue.hpp"
#include "snap/rhi/backend/opengl/Context.h"
#include "snap/rhi/backend/opengl/DebugMessageHandler.h"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/DeviceCreateInfo.h"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/RenderPipelineState.hpp"

#include <optional>
#include <string_view>

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
namespace emscripten {
class val;
} // namespace emscripten
#endif

// https://developer.android.com/about/dashboards
namespace snap::rhi::backend::opengl {
namespace Debug {
class DebugMessageHandler;
}
class Device final : public snap::rhi::backend::common::Device {
    // https://www.khronos.org/registry/OpenGL/extensions/OES/OES_texture_view.txt
public:
    explicit Device(const snap::rhi::backend::opengl::DeviceCreateInfo& info);
    ~Device() noexcept(false) override;

    snap::rhi::backend::opengl::Profile& getOpenGL() noexcept;
    const snap::rhi::backend::opengl::Profile& getOpenGL() const noexcept;

    const std::string_view& getShaderVersionString() const;

    snap::rhi::DeviceContext* getMainDeviceContext() override {
        return mainDC.get();
    }

    snap::rhi::backend::opengl::DeviceContext* sharingDeviceContext() {
        return sharingDC ? sharingDC.get() : mainDC.get();
    }

    std::shared_ptr<snap::rhi::DeviceContext> createDeviceContext(
        const snap::rhi::DeviceContextCreateInfo& dcCreateInfo) override;
    std::shared_ptr<snap::rhi::CommandBuffer> createCommandBuffer(
        const snap::rhi::CommandBufferCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Sampler> createSampler(const snap::rhi::SamplerCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Semaphore> createSemaphore(
        const snap::rhi::SemaphoreCreateInfo& info,
        const std::shared_ptr<snap::rhi::PlatformSyncHandle>& platformSyncHandle) override;
    std::shared_ptr<snap::rhi::Fence> createFence(const snap::rhi::FenceCreateInfo& info) override;

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
    std::shared_ptr<snap::rhi::Buffer> createBuffer(const uint32_t size, const emscripten::val& bufferData);
#endif
    std::shared_ptr<snap::rhi::Buffer> createBuffer(const snap::rhi::BufferCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Buffer> createBuffer(const snap::rhi::BufferCreateInfo& info,
                                                    const std::shared_ptr<std::byte>& bufferData);
    std::shared_ptr<snap::rhi::Texture> createTexture(const TextureCreateInfo& info,
                                                      const snap::rhi::backend::opengl::TextureId textureID,
                                                      const snap::rhi::backend::opengl::TextureTarget target,
                                                      const bool isTextureOwner);
    std::shared_ptr<snap::rhi::Texture> createTexture(
        const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop) final;
    std::shared_ptr<snap::rhi::Texture> createTexture(const snap::rhi::TextureCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Texture> createTextureView(const snap::rhi::TextureViewCreateInfo& info) override;
    std::shared_ptr<RenderPass> createRenderPass(const RenderPassCreateInfo& info) override;
    std::shared_ptr<snap::rhi::Framebuffer> createFramebuffer(const snap::rhi::FramebufferCreateInfo& info) override;
    std::shared_ptr<snap::rhi::ShaderLibrary> createShaderLibrary(
        const snap::rhi::ShaderLibraryCreateInfo& info) override;
    std::shared_ptr<snap::rhi::ShaderModule> createShaderModule(const snap::rhi::ShaderModuleCreateInfo& info) override;
    std::shared_ptr<snap::rhi::ShaderModule> createShaderModule(const ShaderStage shaderStage,
                                                                std::string_view src,
                                                                bool doesShaderContainUniformsOnly);
    std::shared_ptr<snap::rhi::DescriptorSetLayout> createDescriptorSetLayout(
        const snap::rhi::DescriptorSetLayoutCreateInfo& info) override;
    std::shared_ptr<snap::rhi::DescriptorPool> createDescriptorPool(
        const snap::rhi::DescriptorPoolCreateInfo& info) override;
    std::shared_ptr<snap::rhi::DescriptorSet> createDescriptorSet(
        const snap::rhi::DescriptorSetCreateInfo& info) override;
    std::shared_ptr<snap::rhi::PipelineLayout> createPipelineLayout(
        const snap::rhi::PipelineLayoutCreateInfo& info) override;
    std::shared_ptr<snap::rhi::PipelineCache> createPipelineCache(std::span<const uint8_t> serializedData);
    std::shared_ptr<snap::rhi::PipelineCache> createPipelineCache(
        const snap::rhi::PipelineCacheCreateInfo& info) override;
    std::shared_ptr<snap::rhi::RenderPipeline> createRenderPipeline(bool isLegacyPipeline, GLuint programID);
    std::shared_ptr<snap::rhi::RenderPipeline> createRenderPipeline(
        const snap::rhi::RenderPipelineCreateInfo& info) override;
    std::shared_ptr<snap::rhi::ComputePipeline> createComputePipeline(
        const snap::rhi::ComputePipelineCreateInfo& info) override;
    std::shared_ptr<snap::rhi::QueryPool> createQueryPool(const snap::rhi::QueryPoolCreateInfo& info) override;
    TextureFormatProperties getTextureFormatProperties(const TextureFormatInfo& info) override;

    bool isNativeContextAttached() const override;

    snap::rhi::DeviceContext::Guard setCurrent(snap::rhi::DeviceContext* ptr) override;
    snap::rhi::DeviceContext* getCurrentDeviceContext() override;

    std::shared_ptr<snap::rhi::DebugMessenger> createDebugMessenger(
        snap::rhi::DebugMessengerCreateInfo&& info) override;

    GLFramebufferPoolOption getFramebufferPoolOptions() const noexcept {
        return framebufferPoolOptions;
    }

    snap::rhi::backend::opengl::TextureBlitOptions getTextureBlitOptions() const noexcept {
        return textureBlitOptions;
    }

    GLCommandQueueErrorCheckOptions getGLCommandQueueErrorCheckOptions() const noexcept {
        return commandQueueErrorCheckOptions;
    }

    TextureUUID acquireTextureUUID();

    GLPBOOptions getPboOptions() const {
        return glPBOOptions;
    }

    DeviceResourcesOptions getDeviceResourcesOptions() {
        return deviceResourcesAllocateOption;
    }

    bool shouldValidatePipelineOnCreation() const;
    bool areResourcesLazyAllocationsEnabled();

    void clearLazyDestroyResources();

    bool shouldDetachFramebufferOnScopeExit() const {
        return framebufferDetachOnScopeExitOptions ==
               snap::rhi::backend::opengl::FramebufferDetachOnScopeExitOptions::AlwaysDetach;
    }

    snap::rhi::backend::opengl::ResolveOpBehavior getResolveOpBehavior() const {
        return resolveOpBehavior;
    }

    snap::rhi::backend::opengl::LoadOpDontCareBehavior getLoadOpDontCareBehavior() const {
        return loadOpDontCareBehavior;
    }

    const std::unique_ptr<snap::rhi::backend::opengl::DebugMessageHandler>& getDebugMessageHandler() {
        return debugMessageHandler;
    }

    bool isExternalSemaphoreSupported() const;

private:
    template<typename PublicType, typename ImplType, typename... Args>
    std::shared_ptr<PublicType> createResource(Args&&... args) {
        auto resource = std::shared_ptr<PublicType>(
            new ImplType(std::forward<Args>(args)...), [deviceBase = shared_from_this()](auto* ptr) {
                auto* device =
                    snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(deviceBase.get());
                device->deviceResourceRegistry.erase(ptr);

                SNAP_RHI_VALIDATE_RESOURCE_LIFETIMES(device);

                if (!device->isNativeContextAttached()) {
                    std::lock_guard guard(device->lazyDestroyMutex);
                    device->readyToDestroyResources.push_back(ptr);
                } else {
                    delete ptr;
                }
            });
        deviceResourceRegistry.insert(resource);
        return std::move(resource);
    }

    template<typename PublicType, typename ImplType, typename... Args>
    std::shared_ptr<PublicType> createResourceNoDeviceRetain(Args&&... args) {
        auto resource =
            std::shared_ptr<PublicType>(new ImplType(std::forward<Args>(args)...), [device = this](auto* ptr) {
                auto* deviceImpl = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);
                deviceImpl->deviceResourceRegistry.erase(ptr);

                SNAP_RHI_VALIDATE_RESOURCE_LIFETIMES(deviceImpl);

                if (!deviceImpl->isNativeContextAttached()) {
                    std::lock_guard guard(deviceImpl->lazyDestroyMutex);
                    deviceImpl->readyToDestroyResources.push_back(ptr);
                } else {
                    delete ptr;
                }
            });
        deviceResourceRegistry.insert(resource);
        return std::move(resource);
    }

private:
    std::mutex contextMutex;
    std::optional<snap::rhi::backend::opengl::Context> mainGLContext = std::nullopt;

    GLPipelineStatusCheckOptions glPipelineStatusCheckOptions = GLPipelineStatusCheckOptions::OnCreation;
    GLFramebufferPoolOption framebufferPoolOptions;
    DeviceResourcesOptions deviceResourcesAllocateOption;
    GLCommandQueueErrorCheckOptions commandQueueErrorCheckOptions;
    snap::rhi::backend::opengl::FramebufferDetachOnScopeExitOptions framebufferDetachOnScopeExitOptions;
    snap::rhi::backend::opengl::TextureBlitOptions textureBlitOptions =
        snap::rhi::backend::opengl::TextureBlitOptions::Default;
    snap::rhi::backend::opengl::LoadOpDontCareBehavior loadOpDontCareBehavior =
        snap::rhi::backend::opengl::LoadOpDontCareBehavior::DoNothing;
    snap::rhi::backend::opengl::ResolveOpBehavior resolveOpBehavior =
        snap::rhi::backend::opengl::ResolveOpBehavior::DoNothing;
    GLPBOOptions glPBOOptions;

    CommandBufferPool commandBufferPool;

    Profile glProfile;

    std::atomic<DeviceContextUUID> dcUUIDGen = 1;
    std::atomic<TextureUUID> textureUUIDGen = 1;

    RenderPipelineStates renderPipelineStates{};

    std::mutex lazyDestroyMutex;
    std::vector<snap::rhi::DeviceChild*> readyToDestroyResources;

    std::shared_ptr<snap::rhi::backend::opengl::DeviceContext> mainDC;
    std::shared_ptr<snap::rhi::backend::opengl::DeviceContext> sharingDC;

    std::unique_ptr<snap::rhi::backend::opengl::DebugMessageHandler> debugMessageHandler;
};
} // namespace snap::rhi::backend::opengl
