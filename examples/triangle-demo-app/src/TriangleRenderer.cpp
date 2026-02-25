//
// TriangleRenderer.cpp
// SnapRHI Demo Application
//
// Copyright (c) 2026 Snap Inc. All rights reserved.
//
// Implementation of the TriangleRenderer demonstration class.
// See TriangleRenderer.h for detailed documentation.
//

#include "TriangleRenderer.h"
#include "Assets.h"

// SnapRHI API headers
#include <snap/rhi/BufferCreateInfo.h>
#include <snap/rhi/DescriptorPoolCreateInfo.h>
#include <snap/rhi/DescriptorSetCreateInfo.h>
#include <snap/rhi/DescriptorSetLayoutCreateInfo.h>
#include <snap/rhi/FramebufferCreateInfo.h>
#include <snap/rhi/PipelineLayoutCreateInfo.h>
#include <snap/rhi/RenderCommandEncoder.hpp>
#include <snap/rhi/RenderPassCreateInfo.h>
#include <snap/rhi/ShaderLibraryCreateInfo.h>
#include <snap/rhi/ShaderModuleCreateInfo.h>
#include <snap/rhi/TextureCreateInfo.h>

// GLM for matrix math
#include <glm/gtc/matrix_transform.hpp>

// Standard library
#include <array>
#include <cassert>
#include <cstring>
#include <stdexcept>

namespace snap::rhi::demo {

// =============================================================================
// Construction / Destruction
// =============================================================================

TriangleRenderer::TriangleRenderer(std::shared_ptr<snap::rhi::Device> device)
    : device_(std::move(device)), animationStartTime_(std::chrono::steady_clock::now()) {
    // Validate input parameters
    if (!device_) {
        throw std::invalid_argument("TriangleRenderer: device cannot be null");
    }

    // Initialize all GPU resources in dependency order:
    // 1. Shaders (no dependencies)
    // 2. Descriptor set layout (no dependencies)
    // 3. Pipeline layout (depends on descriptor set layout)
    // 4. Descriptor pool (depends on descriptor set layout for sizing)
    // 5. Vertex buffer (no dependencies)
    // Pipeline is created lazily on first render (depends on render target format)

    initializeShaders();
    initializeDescriptorSetLayout();
    initializePipelineLayout();
    initializeDescriptorPool();
    initializeVertexBuffer();
}

TriangleRenderer::~TriangleRenderer() {
    // All resources are reference-counted via shared_ptr.
    // They will be released when this object is destroyed.
    // The actual GPU memory will be reclaimed by the device's deferred deletion queue.
}

// =============================================================================
// Rendering
// =============================================================================

void TriangleRenderer::render(const std::shared_ptr<snap::rhi::CommandBuffer>& commandBuffer,
                              const std::shared_ptr<snap::rhi::Texture>& renderTarget) {
    // --- Validate inputs ---
    if (!commandBuffer || !renderTarget) {
        return; // Silent early-out for null inputs (production code should log a warning)
    }

    // --- Get the render command encoder ---
    // The encoder provides methods for recording graphics commands (draw calls, state changes).
    snap::rhi::RenderCommandEncoder* const renderEncoder = commandBuffer->getRenderCommandEncoder();
    if (!renderEncoder) {
        return; // Command buffer doesn't support render encoding
    }

    // --- Query device capabilities to choose rendering path ---
    // Dynamic rendering (VK_KHR_dynamic_rendering, Metal) is preferred as it eliminates
    // the need for render pass / framebuffer objects. Fall back to traditional path
    // on older OpenGL backends.
    const snap::rhi::Capabilities& deviceCapabilities = device_->getCapabilities();
    const bool useDynamicRendering = deviceCapabilities.isDynamicRenderingSupported;

    // --- Allocate per-frame resources ---
    // Each frame needs its own descriptor set and uniform buffer to avoid GPU/CPU
    // synchronization issues. In production, these would come from a ring buffer.
    std::shared_ptr<snap::rhi::DescriptorSet> frameDescriptorSet;
    std::shared_ptr<snap::rhi::Buffer> frameUniformBuffer;

    if (!allocatePerFrameResources(frameDescriptorSet, frameUniformBuffer)) {
        return; // Allocation failed (pool exhausted or out of memory)
    }

    // --- Update uniform buffer with current transformation ---
    updateUniformBuffer(frameUniformBuffer.get());

    // --- Get render target info for pipeline creation ---
    const snap::rhi::TextureCreateInfo& renderTargetInfo = renderTarget->getCreateInfo();
    const snap::rhi::PixelFormat renderTargetFormat = renderTargetInfo.format;

    // --- Ensure we have a valid pipeline for this format ---
    if (!ensurePipelineForFormat(renderTargetFormat, !useDynamicRendering)) {
        return; // Pipeline creation failed
    }

    // --- Record rendering commands using the appropriate path ---
    if (useDynamicRendering) {
        renderWithDynamicRendering(renderEncoder, renderTarget.get(), frameDescriptorSet.get());
    } else {
        renderWithRenderPass(renderEncoder, renderTarget.get(), frameDescriptorSet.get());
    }
}

// =============================================================================
// Initialization Helpers
// =============================================================================

void TriangleRenderer::initializeShaders() {
    // Query device capabilities to determine which shader format to use.
    // Metal uses .msl (Metal Shading Language), OpenGL uses .glsl.
    const snap::rhi::Capabilities& capabilities = device_->getCapabilities();
    const snap::rhi::APIDescription& apiDescription = capabilities.apiDescription;

    // Determine shader asset path based on active graphics API
    const char* shaderAssetPath = nullptr;
    snap::rhi::ShaderLibraryCreateFlag compileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromSource;
    if (apiDescription.isAnyMetal()) {
        shaderAssetPath = "draw_color.msl";
    } else if (apiDescription.isAnyOpenGL()) {
        shaderAssetPath = "draw_color.glsl";
    } else if (apiDescription.isAnyVulkan()) {
        compileFlag = snap::rhi::ShaderLibraryCreateFlag::CompileFromBinary;
        shaderAssetPath = "draw_color.spv";
    } else {
        throw std::runtime_error("TriangleRenderer: Unsupported graphics API");
    }

    // Load shader source from assets
    std::optional<std::vector<std::byte>> shaderSourceBytes = assets::readRawFile(shaderAssetPath);
    if (!shaderSourceBytes || shaderSourceBytes->empty()) {
        throw std::runtime_error(std::string("TriangleRenderer: Failed to load shader asset: ") + shaderAssetPath);
    }

    // Create shader library from source code
    // A shader library is a compiled collection of shader functions (entry points).
    snap::rhi::ShaderLibraryCreateInfo libraryCreateInfo{};
    libraryCreateInfo.libCompileFlag = compileFlag;
    libraryCreateInfo.code = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(shaderSourceBytes->data()),
                                                      shaderSourceBytes->size());

    shaderLibrary_ = device_->createShaderLibrary(libraryCreateInfo);
    if (!shaderLibrary_) {
        throw std::runtime_error("TriangleRenderer: Failed to create shader library");
    }

    // Create vertex shader module
    // The entry point name must match the function name in the shader source.
    snap::rhi::ShaderModuleCreateInfo vertexShaderCreateInfo{};
    vertexShaderCreateInfo.createFlags = snap::rhi::ShaderModuleCreateFlags::None;
    vertexShaderCreateInfo.shaderStage = snap::rhi::ShaderStage::Vertex;
    vertexShaderCreateInfo.name = "snap_rhi_demo_triangle_vs";
    vertexShaderCreateInfo.shaderLibrary = shaderLibrary_.get();

    vertexShader_ = device_->createShaderModule(vertexShaderCreateInfo);
    if (!vertexShader_) {
        throw std::runtime_error("TriangleRenderer: Failed to create vertex shader module");
    }

    // Create fragment shader module
    snap::rhi::ShaderModuleCreateInfo fragmentShaderCreateInfo{};
    fragmentShaderCreateInfo.createFlags = snap::rhi::ShaderModuleCreateFlags::None;
    fragmentShaderCreateInfo.shaderStage = snap::rhi::ShaderStage::Fragment;
    fragmentShaderCreateInfo.name = "snap_rhi_demo_triangle_fs";
    fragmentShaderCreateInfo.shaderLibrary = shaderLibrary_.get();

    fragmentShader_ = device_->createShaderModule(fragmentShaderCreateInfo);
    if (!fragmentShader_) {
        throw std::runtime_error("TriangleRenderer: Failed to create fragment shader module");
    }
}

void TriangleRenderer::initializeDescriptorSetLayout() {
    // A descriptor set layout defines the types and bindings of resources
    // that can be bound to a shader. Our shader has one uniform buffer
    // at binding slot 2, visible to both vertex and fragment stages.

    snap::rhi::DescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.bindings.push_back(snap::rhi::DescriptorSetLayoutBinding{
        .binding = kUniformBufferBinding,
        .descriptorType = snap::rhi::DescriptorType::UniformBuffer,
        .stageBits = snap::rhi::ShaderStageBits::VertexShaderBit | snap::rhi::ShaderStageBits::FragmentShaderBit,
    });

    descriptorSetLayout_ = device_->createDescriptorSetLayout(layoutCreateInfo);
    if (!descriptorSetLayout_) {
        throw std::runtime_error("TriangleRenderer: Failed to create descriptor set layout");
    }
}

void TriangleRenderer::initializePipelineLayout() {
    // A pipeline layout defines how descriptor sets are arranged across set indices.
    // We bind our descriptor set at index 3 to demonstrate non-trivial layout configuration.
    // Sets 0-2 use an empty descriptor set layout (zero bindings), set 3 contains our uniform buffer layout.
    //
    // Note: All entries must be non-null. Vulkan and other backends require valid descriptor set layouts
    // for all indices. We create an empty layout (with no bindings) for unused set indices.

    // Create an empty descriptor set layout for unused slots
    snap::rhi::DescriptorSetLayoutCreateInfo emptyLayoutCreateInfo{};
    // No bindings - empty layout
    emptyDescriptorSetLayout_ = device_->createDescriptorSetLayout(emptyLayoutCreateInfo);
    if (!emptyDescriptorSetLayout_) {
        throw std::runtime_error("TriangleRenderer: Failed to create empty descriptor set layout");
    }

    // Note: The array size must be at least (kDescriptorSetIndex + 1)
    constexpr size_t kRequiredSetCount = kDescriptorSetIndex + 1;
    std::array<snap::rhi::DescriptorSetLayout*, kRequiredSetCount> setLayouts{};

    // Fill all slots with the empty layout by default
    std::fill(setLayouts.begin(), setLayouts.end(), emptyDescriptorSetLayout_.get());

    // Place our actual layout at the designated index
    setLayouts[kDescriptorSetIndex] = descriptorSetLayout_.get();

    snap::rhi::PipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.setLayouts = setLayouts;

    pipelineLayout_ = device_->createPipelineLayout(layoutCreateInfo);
    if (!pipelineLayout_) {
        throw std::runtime_error("TriangleRenderer: Failed to create pipeline layout");
    }
}

void TriangleRenderer::initializeDescriptorPool() {
    // A descriptor pool manages memory for descriptor set allocations.
    // We size it to support multiple frames in flight.

    snap::rhi::DescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.maxSets = kDescriptorPoolMaxSets;

    // Specify how many of each descriptor type can be allocated
    poolCreateInfo.descriptorCount[static_cast<size_t>(snap::rhi::DescriptorType::UniformBuffer)] =
        kDescriptorPoolMaxSets; // One UBO per set

    descriptorPool_ = device_->createDescriptorPool(poolCreateInfo);
    if (!descriptorPool_) {
        throw std::runtime_error("TriangleRenderer: Failed to create descriptor pool");
    }
}

void TriangleRenderer::initializeVertexBuffer() {
    // Define the triangle vertices in normalized device coordinates (NDC).
    // Each vertex has a 2D position and an RGBA color.
    // The triangle is colored with red, green, and blue at each corner.

    static constexpr std::array<Vertex, kTriangleVertexCount> triangleVertices = {{
        // Bottom-left vertex: position (-0.8, -0.8), color red
        {.position = {-0.8f, -0.8f}, .color = {1.0f, 0.0f, 0.0f, 1.0f}},

        // Bottom-right vertex: position (0.8, -0.8), color green
        {.position = {0.8f, -0.8f}, .color = {0.0f, 1.0f, 0.0f, 1.0f}},

        // Top-center vertex: position (0.0, 0.8), color blue
        {.position = {0.0f, 0.8f}, .color = {0.0f, 0.0f, 1.0f, 1.0f}},
    }};

    // Create a host-visible, host-coherent buffer for simplicity.
    // Host-coherent memory doesn't require explicit flush operations after CPU writes.
    snap::rhi::BufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.bufferUsage = snap::rhi::BufferUsage::VertexBuffer;
    bufferCreateInfo.memoryProperties =
        snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
    bufferCreateInfo.size = static_cast<uint32_t>(sizeof(triangleVertices));

    vertexBuffer_ = device_->createBuffer(bufferCreateInfo);
    if (!vertexBuffer_) {
        throw std::runtime_error("TriangleRenderer: Failed to create vertex buffer");
    }

    // Map the buffer and upload vertex data
    std::byte* const mappedMemory = vertexBuffer_->map(snap::rhi::MemoryAccess::Write, 0, bufferCreateInfo.size);

    if (!mappedMemory) {
        throw std::runtime_error("TriangleRenderer: Failed to map vertex buffer");
    }

    std::memcpy(mappedMemory, triangleVertices.data(), sizeof(triangleVertices));
    vertexBuffer_->unmap();
}

// =============================================================================
// Per-Frame Helpers
// =============================================================================

bool TriangleRenderer::allocatePerFrameResources(std::shared_ptr<snap::rhi::DescriptorSet>& outDescriptorSet,
                                                 std::shared_ptr<snap::rhi::Buffer>& outUniformBuffer) {
    // --- Allocate descriptor set from pool ---
    snap::rhi::DescriptorSetCreateInfo descriptorSetCreateInfo{};
    descriptorSetCreateInfo.descriptorPool = descriptorPool_.get();
    descriptorSetCreateInfo.descriptorSetLayout = descriptorSetLayout_.get();

    outDescriptorSet = device_->createDescriptorSet(descriptorSetCreateInfo);
    if (!outDescriptorSet) {
        return false; // Pool may be exhausted
    }

    // --- Create uniform buffer ---
    snap::rhi::BufferCreateInfo uniformBufferCreateInfo{};
    uniformBufferCreateInfo.bufferUsage = snap::rhi::BufferUsage::UniformBuffer;
    uniformBufferCreateInfo.memoryProperties =
        snap::rhi::MemoryProperties::HostVisible | snap::rhi::MemoryProperties::HostCoherent;
    uniformBufferCreateInfo.size = sizeof(UniformData);

    outUniformBuffer = device_->createBuffer(uniformBufferCreateInfo);
    if (!outUniformBuffer) {
        return false;
    }

    // --- Bind uniform buffer to descriptor set ---
    // This associates the buffer with binding slot 2 in the descriptor set.
    outDescriptorSet->bindUniformBuffer(kUniformBufferBinding, outUniformBuffer.get());

    return true;
}

void TriangleRenderer::updateUniformBuffer(snap::rhi::Buffer* uniformBuffer) {
    assert(uniformBuffer != nullptr);

    // Calculate elapsed time for animation
    const auto currentTime = std::chrono::steady_clock::now();
    const float elapsedSeconds = std::chrono::duration<float>(currentTime - animationStartTime_).count();

    // Build transformation matrix: rotation around Y-axis
    // The rotation speed is 1 radian per second.
    UniformData uniformData{};
    uniformData.transformMatrix = glm::rotate(glm::mat4(1.0f),            // Start with identity matrix
                                              elapsedSeconds,             // Rotation angle in radians
                                              glm::vec3(0.0f, 1.0f, 0.0f) // Rotation axis (Y)
    );

    /**
     * @brief Constructs a perspective projection matrix compatible with Vulkan's coordinate system.
     *
     * This function generates a standard perspective projection matrix (e.g., via GLM or similar math library)
     * and applies a specific correction to adapt it for Vulkan's Normalized Device Coordinates (NDC).
     *
     * @details
     * **The Coordinate System Mismatch:**
     * - **Standard/OpenGL:** Clip Space Y-axis points **UP** [-1 (bottom) to +1 (top)].
     * - **Vulkan:** Clip Space Y-axis points **DOWN** [-1 (top) to +1 (bottom)].
     *
     * **The Correction:**
     * To align standard mathematical projection (which assumes Y-Up) with Vulkan's Y-Down requirement,
     * the `(1,1)` element of the projection matrix is negated:
     * @code
     * projection[1][1] *= -1;
     * @endcode
     * This effectively flips the Y-axis of the clip-space coordinates.
     *
     * @warning **Critical Side Effect: Winding Order**
     * Flipping the Y-axis mirrors the geometry, which reverses the triangle winding order.
     * - **Counter-Clockwise (CCW)** triangles will become **Clockwise (CW)** after this transform.
     * - If your pipeline uses backface culling (`VK_CULL_MODE_BACK_BIT`), you must ensure your
     * Front Face definition matches this new winding.
     * - **Recommended:** Set `VkPipelineRasterizationStateCreateInfo::frontFace` to
     * `VK_FRONT_FACE_COUNTER_CLOCKWISE` if your source data is CCW, effectively treating
     * the "flipped" result as correct.
     **/
    if (device_->getCapabilities().apiDescription.isAnyVulkan()) {
        uniformData.transformMatrix[1][1] *= -1;
    }
    // Color multiplier is white (no tinting) - the per-vertex colors will show through
    uniformData.colorMultiplier = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    // Map buffer and write data
    std::byte* const mappedMemory = uniformBuffer->map(snap::rhi::MemoryAccess::Write, 0, sizeof(UniformData));

    if (mappedMemory) {
        std::memcpy(mappedMemory, &uniformData, sizeof(UniformData));
        uniformBuffer->unmap();
    }
}

bool TriangleRenderer::ensurePipelineForFormat(snap::rhi::PixelFormat colorFormat, bool useRenderPass) {
    // Check if we need to recreate the pipeline (format changed or first creation)
    const bool needsRecreation = !renderPipeline_ || cachedRenderTargetFormat_ != colorFormat;

    if (!needsRecreation) {
        return true; // Existing pipeline is valid
    }

    // Store format for cache invalidation check
    cachedRenderTargetFormat_ = colorFormat;

    // If using render pass mode, create/update render pass and framebuffer
    if (useRenderPass) {
        // Create render pass describing the attachment operations
        snap::rhi::RenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.attachments[0].format = colorFormat;
        renderPassCreateInfo.attachments[0].loadOp = snap::rhi::AttachmentLoadOp::Clear;
        renderPassCreateInfo.attachments[0].storeOp = snap::rhi::AttachmentStoreOp::Store;
        renderPassCreateInfo.attachments[0].samples = snap::rhi::SampleCount::Count1;

        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.subpasses[0].colorAttachmentCount = 1;
        renderPassCreateInfo.subpasses[0].colorAttachments[0].attachment = 0;

        cachedRenderPass_ = device_->createRenderPass(renderPassCreateInfo);
        if (!cachedRenderPass_) {
            return false;
        }
    }

    // Reset pipeline to trigger recreation
    renderPipeline_.reset();

    // --- Build pipeline create info ---
    snap::rhi::RenderPipelineCreateInfo pipelineCreateInfo{};

    // Request native reflection data for debugging/validation
    pipelineCreateInfo.pipelineCreateFlags = snap::rhi::PipelineCreateFlags::AcquireNativeReflection;

    // Set pipeline layout and shader stages
    pipelineCreateInfo.pipelineLayout = pipelineLayout_.get();
    pipelineCreateInfo.stages = {vertexShader_.get(), fragmentShader_.get()};

    // --- Input assembly state ---
    // Defines how vertices are assembled into primitives
    pipelineCreateInfo.inputAssemblyState.primitiveTopology = snap::rhi::Topology::Triangles;

    // --- Rasterization state ---
    // Controls how triangles are converted to fragments
    pipelineCreateInfo.rasterizationState.rasterizationEnabled = true;
    pipelineCreateInfo.rasterizationState.cullMode = snap::rhi::CullMode::None; // Draw both sides
    pipelineCreateInfo.rasterizationState.windingMode = snap::rhi::Winding::CCW;

    // --- Color blend state ---
    // Controls how fragment outputs are blended with existing framebuffer contents
    pipelineCreateInfo.colorBlendState.colorAttachmentsCount = 1;
    pipelineCreateInfo.colorBlendState.colorAttachmentsBlendState[0].blendEnable = false;
    pipelineCreateInfo.colorBlendState.colorAttachmentsBlendState[0].colorWriteMask = snap::rhi::ColorMask::All;

    // --- Vertex input state ---
    // Describes how vertex data is organized in buffers

    // One vertex buffer binding at slot 0
    pipelineCreateInfo.vertexInputState.bindingsCount = 1;
    pipelineCreateInfo.vertexInputState.bindingDescription[0].binding = kVertexBufferBinding;
    pipelineCreateInfo.vertexInputState.bindingDescription[0].inputRate = snap::rhi::VertexInputRate::PerVertex;
    pipelineCreateInfo.vertexInputState.bindingDescription[0].stride = sizeof(Vertex);

    // Two vertex attributes: position (vec2) and color (vec4)
    pipelineCreateInfo.vertexInputState.attributesCount = 2;

    // Attribute 0: position (vec2 at offset 0)
    pipelineCreateInfo.vertexInputState.attributeDescription[0].location = 0;
    pipelineCreateInfo.vertexInputState.attributeDescription[0].binding = kVertexBufferBinding;
    pipelineCreateInfo.vertexInputState.attributeDescription[0].format = snap::rhi::VertexAttributeFormat::Float2;
    pipelineCreateInfo.vertexInputState.attributeDescription[0].offset = offsetof(Vertex, position);

    // Attribute 1: color (vec4 at offset after position)
    pipelineCreateInfo.vertexInputState.attributeDescription[1].location = 1;
    pipelineCreateInfo.vertexInputState.attributeDescription[1].binding = kVertexBufferBinding;
    pipelineCreateInfo.vertexInputState.attributeDescription[1].format = snap::rhi::VertexAttributeFormat::Float4;
    pipelineCreateInfo.vertexInputState.attributeDescription[1].offset = offsetof(Vertex, color);

    // --- Backend-specific configuration ---
    if (useRenderPass) {
        // Traditional render pass path
        pipelineCreateInfo.renderPass = cachedRenderPass_.get();
    } else {
        // Dynamic rendering path - specify attachment formats directly
        pipelineCreateInfo.renderPass = nullptr;
        pipelineCreateInfo.attachmentFormatsCreateInfo.colorAttachmentFormats.resize(1);
        pipelineCreateInfo.attachmentFormatsCreateInfo.colorAttachmentFormats[0] = colorFormat;
    }

    // Metal-specific: provide empty info to enable Metal backend path
    pipelineCreateInfo.mtlRenderPipelineInfo.emplace(snap::rhi::metal::RenderPipelineInfo{});

    // OpenGL-specific: provide resource name mappings
    pipelineCreateInfo.glRenderPipelineInfo = buildOpenGLPipelineInfo();

    // --- Create the pipeline ---
    renderPipeline_ = device_->createRenderPipeline(pipelineCreateInfo);

    return renderPipeline_ != nullptr;
}

// =============================================================================
// Render Path Implementations
// =============================================================================

void TriangleRenderer::renderWithDynamicRendering(snap::rhi::RenderCommandEncoder* encoder,
                                                  snap::rhi::Texture* renderTarget,
                                                  snap::rhi::DescriptorSet* descriptorSet) {
    const snap::rhi::TextureCreateInfo& renderTargetInfo = renderTarget->getCreateInfo();

    // --- Configure dynamic rendering info ---
    // This replaces the need for explicit RenderPass and Framebuffer objects.
    snap::rhi::RenderingInfo renderingInfo{};
    renderingInfo.layers = 1;

    // Configure single color attachment
    renderingInfo.colorAttachments.resize(1);
    snap::rhi::RenderingAttachmentInfo& colorAttachment = renderingInfo.colorAttachments[0];

    colorAttachment.attachment.texture = renderTarget;
    colorAttachment.attachment.mipLevel = 0;
    colorAttachment.attachment.layer = 0;
    colorAttachment.loadOp = snap::rhi::AttachmentLoadOp::Clear;
    colorAttachment.storeOp = snap::rhi::AttachmentStoreOp::Store;
    colorAttachment.clearValue.color.float32 = kClearColor;

    // --- Begin encoding ---
    encoder->beginEncoding(renderingInfo);

    // --- Set viewport to cover entire render target ---
    const snap::rhi::Viewport viewport =
        createFullscreenViewport(renderTargetInfo.size.width, renderTargetInfo.size.height);
    encoder->setViewport(viewport);

    // --- Bind pipeline state ---
    encoder->bindRenderPipeline(renderPipeline_.get());

    // --- Bind vertex buffer ---
    encoder->bindVertexBuffer(kVertexBufferBinding, vertexBuffer_.get(), 0);

    // --- Bind descriptor set containing uniform buffer ---
    encoder->bindDescriptorSet(kDescriptorSetIndex, descriptorSet, {});

    // --- Issue draw call ---
    encoder->draw(kTriangleVertexCount, 0, 1);

    // --- End encoding ---
    encoder->endEncoding();
}

void TriangleRenderer::renderWithRenderPass(snap::rhi::RenderCommandEncoder* encoder,
                                            snap::rhi::Texture* renderTarget,
                                            snap::rhi::DescriptorSet* descriptorSet) {
    const snap::rhi::TextureCreateInfo& renderTargetInfo = renderTarget->getCreateInfo();

    // --- Create framebuffer if needed ---
    // The framebuffer binds render targets to render pass attachments.
    std::shared_ptr<snap::rhi::Framebuffer> framebuffer = nullptr;
    {
        snap::rhi::FramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.width = renderTargetInfo.size.width;
        framebufferCreateInfo.height = renderTargetInfo.size.height;
        framebufferCreateInfo.layers = 1;
        framebufferCreateInfo.renderPass = cachedRenderPass_.get();
        framebufferCreateInfo.attachments = {renderTarget};

        framebuffer = device_->createFramebuffer(framebufferCreateInfo);
        if (!framebuffer) {
            return;
        }
    }

    // --- Configure render pass begin info ---
    snap::rhi::RenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.renderPass = cachedRenderPass_.get();
    renderPassBeginInfo.framebuffer = framebuffer.get();

    // Set clear value for the color attachment
    snap::rhi::ClearValue clearValue{};
    clearValue.color.float32 = kClearColor;
    renderPassBeginInfo.clearValues.push_back(clearValue);

    // --- Begin encoding ---
    encoder->beginEncoding(renderPassBeginInfo);

    // --- Set viewport to cover entire render target ---
    const snap::rhi::Viewport viewport =
        createFullscreenViewport(renderTargetInfo.size.width, renderTargetInfo.size.height);
    encoder->setViewport(viewport);

    // --- Bind pipeline state ---
    encoder->bindRenderPipeline(renderPipeline_.get());

    // --- Bind vertex buffer ---
    encoder->bindVertexBuffer(kVertexBufferBinding, vertexBuffer_.get(), 0);

    // --- Bind descriptor set containing uniform buffer ---
    encoder->bindDescriptorSet(kDescriptorSetIndex, descriptorSet, {});

    // --- Issue draw call ---
    encoder->draw(kTriangleVertexCount, 0, 1);

    // --- End encoding ---
    encoder->endEncoding();
}

// =============================================================================
// Static Helpers
// =============================================================================

snap::rhi::opengl::RenderPipelineInfo TriangleRenderer::buildOpenGLPipelineInfo() {
    // OpenGL doesn't have native descriptor sets, so we provide explicit mappings
    // that tell the backend how to connect our logical bindings to GL uniforms.

    snap::rhi::opengl::RenderPipelineInfo pipelineInfo{};

    // --- Uniform buffer resource mapping ---
    // Maps our logical (set, binding) to the GLSL uniform block name.
    snap::rhi::opengl::PipelineResourceInfo uniformBufferInfo{};
    uniformBufferInfo.name = "TriangleSettings"; // Must match GLSL uniform block name
    uniformBufferInfo.descriptorType = snap::rhi::DescriptorType::UniformBuffer;
    uniformBufferInfo.binding.descriptorSet = kDescriptorSetIndex;
    uniformBufferInfo.binding.binding = kUniformBufferBinding;

    pipelineInfo.resources.push_back(std::move(uniformBufferInfo));

    // --- Vertex attribute mappings ---
    // Maps GLSL input variable names to attribute locations.
    // These names must match the shader source exactly.
    pipelineInfo.vertexAttributes.push_back(snap::rhi::opengl::VertexAttributeInfo{.name = "aPos", .location = 0});
    pipelineInfo.vertexAttributes.push_back(snap::rhi::opengl::VertexAttributeInfo{.name = "aColor", .location = 1});

    return pipelineInfo;
}

snap::rhi::Viewport TriangleRenderer::createFullscreenViewport(uint32_t width, uint32_t height) {
    snap::rhi::Viewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = width;
    viewport.height = height;
    viewport.znear = 0.0f;
    viewport.zfar = 1.0f;
    return viewport;
}

} // namespace snap::rhi::demo
