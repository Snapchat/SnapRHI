#include "snap/rhi/backend/opengl/Device.hpp"

#include "PlatformSpecific/WebAssembly/ValBuffer.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include "snap/rhi/backend/opengl/Buffer.hpp"
#include "snap/rhi/backend/opengl/CommandQueue.hpp"
#include "snap/rhi/backend/opengl/ComputePipeline.hpp"
#include "snap/rhi/backend/opengl/DebugMessageHandler.h"
#include "snap/rhi/backend/opengl/DebugMessenger.h"
#include "snap/rhi/backend/opengl/DescriptorSet.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"
#include "snap/rhi/backend/opengl/Fence.hpp"
#include "snap/rhi/backend/opengl/Framebuffer.hpp"
#include "snap/rhi/backend/opengl/PipelineCache.hpp"
#include "snap/rhi/backend/opengl/RenderPipeline.hpp"
#include "snap/rhi/backend/opengl/Sampler.hpp"
#include "snap/rhi/backend/opengl/Semaphore.hpp"
#include "snap/rhi/backend/opengl/ShaderLibrary.hpp"

#if SNAP_RHI_OS_ANDROID()
#include "snap/rhi/backend/common/platform/android/SyncHandle.h"
#include "snap/rhi/backend/opengl/platform/android/Semaphore.h"
#endif

#include "snap/rhi/backend/common/PipelineLayout.h"
#include "snap/rhi/backend/opengl/QueryPool.h"
#include "snap/rhi/backend/opengl/ShaderModule.hpp"
#include "snap/rhi/backend/opengl/Texture.hpp"
#include <snap/rhi/backend/opengl/DebugMessenger.h>

#include "snap/rhi/common/Throw.h"
#include <OpenGL/API.h>
#include <string>

namespace snap::rhi::backend::opengl {
Device::Device(const snap::rhi::backend::opengl::DeviceCreateInfo& info)
    : snap::rhi::backend::common::Device(info.deviceInfo),
      glPipelineStatusCheckOptions(info.glPipelineStatusCheckOptions),
      framebufferPoolOptions(info.framebufferPoolOptions),
      deviceResourcesAllocateOption(info.deviceResourcesAllocateOption),
      commandQueueErrorCheckOptions(info.commandQueueErrorCheckOptions),
      framebufferDetachOnScopeExitOptions(info.framebufferDetachOnScopeExitOptions),
      textureBlitOptions(info.textureBlitOptions),
      loadOpDontCareBehavior(info.loadOpDontCareBehavior),
      resolveOpBehavior(info.resolveOpBehavior),
      glPBOOptions(info.glPBOOptions),
      commandBufferPool(this),
      glProfile(this) {
    SNAP_RHI_VALIDATE(
        validationLayer,
        info.apiVersion != gl::APIVersion::None,
        ReportLevel::CriticalError,
        snap::rhi::ValidationTag::DeviceOp,
        "[snap::rhi::backend::opengl::Device] Non-none GL version needs to be requested creating the device."
        "Use gl::APIVersion::Latest to create latest GL version context");

    auto glContext =
        info.glContext ?
            snap::rhi::backend::opengl::Context(info.glContext,
                                                snap::rhi::backend::opengl::Context::ContextMode::Own,
                                                static_cast<bool>(info.deviceInfo.deviceCreateFlags &
                                                                  snap::rhi::DeviceCreateFlags::EnableDebugCallback)) :
            snap::rhi::backend::opengl::Context(
                info.apiVersion,
                info.dcCreateInfo.isUserTheOwnerOfGLContext ?
                    snap::rhi::backend::opengl::Context::ConstructBehavior::UseBoundOrCreateNew :
                    snap::rhi::backend::opengl::Context::ConstructBehavior::CreateNewAlways,
                static_cast<bool>(info.deviceInfo.deviceCreateFlags &
                                  snap::rhi::DeviceCreateFlags::EnableDebugCallback));
    auto guard = glContext.makeCurrent();
    gl::loadAPIWithHooks();
#if (SNAP_RHI_OS_ANDROID()) && !SNAP_RHI_ENABLE_EGL_GLAD
    gladLoadEGLExtensionsSafe(gl::getDefaultDisplay());
#endif

    const auto version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    auto realApiVersion = snap::rhi::backend::opengl::computeVersion(version);

    SNAP_RHI_VALIDATE(
        validationLayer,
        info.apiVersion == gl::APIVersion::Latest ||
            (info.apiVersion <= realApiVersion && snap::rhi::backend::opengl::isApiVersionGLES(info.apiVersion) ==
                                                      snap::rhi::backend::opengl::isApiVersionGLES(realApiVersion)),
        ReportLevel::CriticalError,
        snap::rhi::ValidationTag::DeviceOp,
        "[snap::rhi::backend::opengl::Device] Real API version is below requested API version or GLES mismatch."
        " Requested %d, real %d",
        static_cast<int>(info.apiVersion),
        static_cast<int>(realApiVersion));
    auto requestedApiVersion = (info.apiVersion == gl::APIVersion::Latest) ? realApiVersion : info.apiVersion;

    const std::string_view vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const std::string_view renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

    { // Blocklist GPU features
        if (info.fboTileMemoryIssueGPUs.find(renderer) != std::string::npos) {
            framebufferDetachOnScopeExitOptions =
                snap::rhi::backend::opengl::FramebufferDetachOnScopeExitOptions::AlwaysDetach;
        }
    }

    platformDeviceString = std::string(vendor) + " " + std::string(renderer) + " " + std::string(version);
    SNAP_RHI_LOGI("[snap::rhi::backend::opengl::Device] platformDeviceString: %s", platformDeviceString.c_str());

    glProfile.loadOpenGLInstance(requestedApiVersion, realApiVersion, vendor, renderer);
    caps = glProfile.buildCapabilities();
    mainDC = std::make_shared<snap::rhi::backend::opengl::DeviceContext>(
        this, std::move(glContext), info.dcCreateInfo, dcUUIDGen.fetch_add(1));
#if SNAP_RHI_OS_WINDOWS()
    // On Windows, when shared context is created, the context it's shared from may not be current
    // on any thread other than current. That's way too serious limitation, and to work it around
    // we're creating a separate DeviceContext (which is never current) which we'll be using to share with.
    sharingDC = std::make_shared<snap::rhi::backend::opengl::DeviceContext>(
        this, std::nullopt, snap::rhi::backend::opengl::DeviceContextCreateInfo(), dcUUIDGen.fetch_add(1));
#endif
    debugMessageHandler = std::make_unique<DebugMessageHandler>(this);

    snap::rhi::backend::common::reportCapabilities(caps, validationLayer);
    commandQueues.resize(caps.queueFamilyProperties.size());
    for (size_t i = 0; i < caps.queueFamilyProperties.size(); ++i) {
        const auto& queueFamilyProperties = caps.queueFamilyProperties[i];
        commandQueues[i].resize(queueFamilyProperties.queueCount);
        for (uint32_t j = 0; j < queueFamilyProperties.queueCount; ++j) {
            commandQueues[i][j] = std::make_unique<snap::rhi::backend::opengl::CommandQueue>(this);
        }
    }
    fencePool = std::make_unique<common::FencePool>(
        [this](const snap::rhi::FenceCreateInfo& info) {
            return opengl::Device::createResourceNoDeviceRetain<snap::rhi::Fence, snap::rhi::backend::opengl::Fence>(
                this, info);
        },
        validationLayer);
    submissionTracker =
        std::make_unique<common::SubmissionTracker>(this, [this](const snap::rhi::DeviceContextCreateInfo& info) {
            DeviceContextUUID dcUUID = dcUUIDGen.fetch_add(1);

            snap::rhi::backend::opengl::DeviceContextCreateInfo glDCCreateInfo{info};
            glDCCreateInfo.isUserTheOwnerOfGLContext = false;
            return common::Device::createResourceNoDeviceRetain<snap::rhi::DeviceContext,
                                                                snap::rhi::backend::opengl::DeviceContext>(
                this, std::optional<snap::rhi::backend::opengl::Context>{}, glDCCreateInfo, dcUUID);
        });
}

Device::~Device() noexcept(false) {
    {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~Device] start destruction");
        auto guard = mainDC->makeCurrent();

        commandQueues.clear();
        submissionTracker.reset();
        fencePool.reset();
        clearLazyDestroyResources();
        {
            SNAP_RHI_VALIDATE(validationLayer,
                              commandBufferPool.isAllCommandBufferReleased(),
                              snap::rhi::ReportLevel::CriticalError,
                              snap::rhi::ValidationTag::DestroyOp,
                              "[snap::rhi::backend::opengl::~Device] Some commandBuffer is being used! All Snap "
                              "Graphics resources must "
                              "be destroyed before destroying snap::rhi::Device.");
        }
        debugMessageHandler.reset();
    }

    mainDC.reset();
    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::Debug,
                    snap::rhi::ValidationTag::DestroyOp,
                    "[snap::rhi::backend::opengl::~Device] end of destruction");
}

bool Device::isExternalSemaphoreSupported() const {
#if SNAP_RHI_OS_ANDROID()
    return glProfile.getFeatures().isANDROIDNativeFenceSyncSupported && glProfile.getFeatures().isFenceSupported;
#else
    return false;
#endif
}

std::shared_ptr<snap::rhi::DeviceContext> Device::createDeviceContext(
    const snap::rhi::DeviceContextCreateInfo& dcCreateInfo) {
    DeviceContextUUID dcUUID = dcUUIDGen.fetch_add(1);

    snap::rhi::backend::opengl::DeviceContextCreateInfo glDCCreateInfo{dcCreateInfo};
    glDCCreateInfo.isUserTheOwnerOfGLContext = false;
    return common::Device::createResource<snap::rhi::DeviceContext, snap::rhi::backend::opengl::DeviceContext>(
        this, std::optional<snap::rhi::backend::opengl::Context>{}, glDCCreateInfo, dcUUID);
}

snap::rhi::DeviceContext::Guard Device::setCurrent(snap::rhi::DeviceContext* ptr) {
    auto* dc = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::DeviceContext>(ptr);
    return dc->makeCurrent();
}

snap::rhi::DeviceContext* Device::getCurrentDeviceContext() {
    return snap::rhi::backend::opengl::DeviceContext::getCurrent();
}

void Device::clearLazyDestroyResources() {
    assert(isNativeContextAttached());

    std::lock_guard guard(lazyDestroyMutex);
    for (const auto* ptr : readyToDestroyResources) {
        delete ptr;
    }

    readyToDestroyResources.clear();
}

std::shared_ptr<snap::rhi::QueryPool> Device::createQueryPool(const snap::rhi::QueryPoolCreateInfo& info) {
    // TODO: Implement OpenGL query pool creation
    return nullptr;
    // return opengl::Device::createResource<snap::rhi::QueryPool, snap::rhi::backend::opengl::QueryPool>(this, info);
}

std::shared_ptr<snap::rhi::CommandBuffer> Device::createCommandBuffer(const snap::rhi::CommandBufferCreateInfo& info) {
    auto resource = std::shared_ptr<snap::rhi::CommandBuffer>(
        commandBufferPool.acquireCommandBuffer(info), [deviceBase = shared_from_this()](auto* ptr) {
            auto* device = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(deviceBase.get());
            /*
             * deviceResourceRegistry.erase(...) must be called before releasing command buffer back to pool,
             * because after commandBufferPool.releaseCommandBuffer(...) the ptr may be reused for another command
             * buffer.
             */
            device->deviceResourceRegistry.erase(ptr);
            device->commandBufferPool.releaseCommandBuffer(ptr);
        });
    deviceResourceRegistry.insert(resource);
    return resource;
}

std::shared_ptr<snap::rhi::Sampler> Device::createSampler(const snap::rhi::SamplerCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::Sampler, snap::rhi::backend::opengl::Sampler>(this, info);
}

std::shared_ptr<snap::rhi::Semaphore> Device::createSemaphore(
    const snap::rhi::SemaphoreCreateInfo& info,
    const std::shared_ptr<snap::rhi::PlatformSyncHandle>& platformSyncHandle) {
    if (platformSyncHandle) {
#if SNAP_RHI_OS_ANDROID()
        auto* androidHandle =
            dynamic_cast<snap::rhi::backend::common::platform::android::SyncHandle*>(platformSyncHandle.get());
        if (androidHandle && isExternalSemaphoreSupported()) {
            int32_t fenceFd = androidHandle->releaseFenceFd();
            if (fenceFd >= 0) {
                return opengl::Device::createResource<snap::rhi::Semaphore,
                                                      snap::rhi::backend::opengl::platform::android::Semaphore>(
                    this, info, fenceFd);
            }
        }
#endif
        auto* commonHandle = snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::PlatformSyncHandle>(
            platformSyncHandle.get());
        if (const auto& fence = commonHandle->getFence(); fence) {
            fence->waitForComplete();
        }
    }
    return opengl::Device::createResource<snap::rhi::Semaphore, snap::rhi::backend::opengl::Semaphore>(this, info);
}

std::shared_ptr<snap::rhi::Fence> Device::createFence(const snap::rhi::FenceCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::Fence, snap::rhi::backend::opengl::Fence>(this, info);
}

std::shared_ptr<snap::rhi::Buffer> Device::createBuffer(const BufferCreateInfo& info,
                                                        const std::shared_ptr<std::byte>& bufferData) {
    return opengl::Device::createResource<snap::rhi::Buffer, snap::rhi::backend::opengl::Buffer>(
        this, info, bufferData);
}

#if SNAP_RHI_PLATFORM_WEBASSEMBLY()
std::shared_ptr<snap::rhi::Buffer> Device::createBuffer(const uint32_t size, const emscripten::val& bufferData) {
    return opengl::Device::createResource<snap::rhi::Buffer, snap::rhi::backend::opengl::ValBuffer>(
        this, size, bufferData);
}
#endif

std::shared_ptr<snap::rhi::Buffer> Device::createBuffer(const snap::rhi::BufferCreateInfo& info) {
    SNAP_RHI_VALIDATE(validationLayer,
                      glProfile.getFeatures().isSSBOSupported ||
                          (info.bufferUsage & snap::rhi::BufferUsage::StorageBuffer) == snap::rhi::BufferUsage::None,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Device::createBuffer] StorageBuffer doesnt supported buffer type for current API.");
    SNAP_RHI_VALIDATE(validationLayer,
                      (info.bufferUsage & snap::rhi::BufferUsage::UniformBuffer) == snap::rhi::BufferUsage::None ||
                          (info.size & 0xf) == 0,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Device::createBuffer] sizeof UniformBuffer should be 16 byte aligned.");
    return opengl::Device::createResource<snap::rhi::Buffer, snap::rhi::backend::opengl::Buffer>(this, info);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const snap::rhi::TextureCreateInfo& info) {
    validateTextureCreation(info);
    TextureUUID textureUUID = acquireTextureUUID();
    return opengl::Device::createResource<snap::rhi::Texture, snap::rhi::backend::opengl::Texture>(
        this, info, textureUUID);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const snap::rhi::TextureCreateInfo& info,
                                                          const snap::rhi::backend::opengl::TextureId textureID,
                                                          const snap::rhi::backend::opengl::TextureTarget target,
                                                          const bool isTextureOwner) {
    validateTextureCreation(info);
    TextureUUID textureUUID = acquireTextureUUID();
    return opengl::Device::createResource<snap::rhi::Texture, snap::rhi::backend::opengl::Texture>(
        this, info, textureID, target, isTextureOwner, textureUUID);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(
    const std::shared_ptr<snap::rhi::TextureInterop>& textureInterop) {
    TextureUUID textureUUID = acquireTextureUUID();
    return opengl::Device::createResource<snap::rhi::Texture, snap::rhi::backend::opengl::Texture>(
        this, textureInterop, textureUUID);
}

std::shared_ptr<snap::rhi::Texture> Device::createTextureView(const snap::rhi::TextureViewCreateInfo& info) {
    snap::rhi::common::throwException("[Device::createTextureView] OpenGL 4.5 doesn't supported TextureView");
}

std::shared_ptr<RenderPass> Device::createRenderPass(const RenderPassCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::RenderPass, snap::rhi::RenderPass>(this, info);
}

std::shared_ptr<snap::rhi::Framebuffer> Device::createFramebuffer(const snap::rhi::FramebufferCreateInfo& info) {
    SNAP_RHI_VALIDATE(validationLayer,
                      info.renderPass,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[createFramebuffer] renderPass is nullptr");

    return opengl::Device::createResource<snap::rhi::Framebuffer, snap::rhi::backend::opengl::Framebuffer>(this, info);
}

std::shared_ptr<snap::rhi::ShaderLibrary> Device::createShaderLibrary(const snap::rhi::ShaderLibraryCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::ShaderLibrary, snap::rhi::backend::opengl::ShaderLibrary>(this,
                                                                                                               info);
}

std::shared_ptr<snap::rhi::ShaderModule> Device::createShaderModule(const ShaderStage shaderStage,
                                                                    std::string_view src,
                                                                    bool doesShaderContainUniformsOnly) {
    return opengl::Device::createResource<snap::rhi::ShaderModule, snap::rhi::backend::opengl::ShaderModule>(
        this, shaderStage, src, doesShaderContainUniformsOnly);
}

std::shared_ptr<snap::rhi::ShaderModule> Device::createShaderModule(const snap::rhi::ShaderModuleCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::ShaderModule, snap::rhi::backend::opengl::ShaderModule>(this,
                                                                                                             info);
}

std::shared_ptr<snap::rhi::DescriptorSetLayout> Device::createDescriptorSetLayout(
    const snap::rhi::DescriptorSetLayoutCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::DescriptorSetLayout, snap::rhi::DescriptorSetLayout>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorPool> Device::createDescriptorPool(
    const snap::rhi::DescriptorPoolCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::DescriptorPool, snap::rhi::DescriptorPool>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorSet> Device::createDescriptorSet(const snap::rhi::DescriptorSetCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::DescriptorSet, snap::rhi::backend::opengl::DescriptorSet>(this,
                                                                                                               info);
}

std::shared_ptr<snap::rhi::PipelineLayout> Device::createPipelineLayout(
    const snap::rhi::PipelineLayoutCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::PipelineLayout, snap::rhi::backend::common::PipelineLayout>(this,
                                                                                                                 info);
}

std::shared_ptr<snap::rhi::PipelineCache> Device::createPipelineCache(std::span<const uint8_t> serializedData) {
    return opengl::Device::createResource<snap::rhi::PipelineCache, snap::rhi::backend::opengl::PipelineCache>(
        this, serializedData);
}

std::shared_ptr<snap::rhi::PipelineCache> Device::createPipelineCache(const snap::rhi::PipelineCacheCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::PipelineCache, snap::rhi::backend::opengl::PipelineCache>(this,
                                                                                                               info);
}

std::shared_ptr<snap::rhi::RenderPipeline> Device::createRenderPipeline(
    const snap::rhi::RenderPipelineCreateInfo& info) {
    for (uint32_t i = 0; i < info.vertexInputState.attributesCount; ++i) {
        SNAP_RHI_VALIDATE(
            validationLayer,
            (info.vertexInputState.attributeDescription[i].offset & 0x3) == 0,
            snap::rhi::ReportLevel::Error,
            snap::rhi::ValidationTag::CreateOp,
            "[createRenderPipeline] info.vertexInputState.attributeDescription.offset must be a multiple of 4 "
            "bytes.");
    }

    RenderPipelineStatesUUID stateUUID = renderPipelineStates.getRenderPipelineState(info);
    return opengl::Device::createResource<snap::rhi::RenderPipeline, snap::rhi::backend::opengl::RenderPipeline>(
        this, info, stateUUID);
}

std::shared_ptr<snap::rhi::RenderPipeline> Device::createRenderPipeline(bool isLegacyPipeline, GLuint programID) {
    return opengl::Device::createResource<snap::rhi::RenderPipeline, snap::rhi::backend::opengl::RenderPipeline>(
        this, isLegacyPipeline, programID);
}

std::shared_ptr<snap::rhi::ComputePipeline> Device::createComputePipeline(
    const snap::rhi::ComputePipelineCreateInfo& info) {
    return opengl::Device::createResource<snap::rhi::ComputePipeline, snap::rhi::backend::opengl::ComputePipeline>(
        this, info);
}

TextureFormatProperties Device::getTextureFormatProperties(const TextureFormatInfo& info) {
    return {
        .maxExtent =
            {
                .width = caps.maxTextureDimension3D,
                .height = caps.maxTextureDimension3D,
                .depth = caps.maxTextureDimension3D,
            },
        .maxMipLevels = 12u,
        .maxArrayLayers = caps.maxTextureArrayLayers,
        .sampleCounts = caps.formatProperties[static_cast<uint32_t>(info.format)].framebufferSampleCounts,
        .maxResourceSize = caps.maxTextureDimension3D * caps.maxTextureDimension3D * caps.maxTextureDimension3D *
                           16u /* assume max 128bpp */,
    };
}

TextureUUID Device::acquireTextureUUID() {
    TextureUUID textureUUID = textureUUIDGen.fetch_add(1, std::memory_order_relaxed);
    return textureUUID;
}

const std::string_view& Device::getShaderVersionString() const {
    return glProfile.getFeatures().shaderVersionHeader;
}

snap::rhi::backend::opengl::Profile& Device::getOpenGL() noexcept {
    return glProfile;
}

const snap::rhi::backend::opengl::Profile& Device::getOpenGL() const noexcept {
    return glProfile;
}

std::shared_ptr<snap::rhi::DebugMessenger> Device::createDebugMessenger(snap::rhi::DebugMessengerCreateInfo&& info) {
    return opengl::Device::createResource<snap::rhi::DebugMessenger, snap::rhi::backend::opengl::DebugMessenger>(
        this, std::move(info));
}

bool Device::isNativeContextAttached() const {
    return gl::getActiveContext() != nullptr;
}

bool Device::areResourcesLazyAllocationsEnabled() {
    return !isNativeContextAttached() &&
           ((deviceResourcesAllocateOption & snap::rhi::backend::opengl::DeviceResourcesOptions::CreateResourcesLazy) !=
            snap::rhi::backend::opengl::DeviceResourcesOptions::None);
}

bool Device::shouldValidatePipelineOnCreation() const {
    return glPipelineStatusCheckOptions == GLPipelineStatusCheckOptions::OnCreation;
}
} // namespace snap::rhi::backend::opengl
