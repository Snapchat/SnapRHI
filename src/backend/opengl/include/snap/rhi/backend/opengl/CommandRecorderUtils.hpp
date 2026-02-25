//
//  CommandRecorderUtils.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 01.11.2021.
//

#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/RenderPipelineCreateInfo.h"

#include "snap/rhi/ClearValue.h"
#include "snap/rhi/Device.hpp"
#include "snap/rhi/Limits.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/FramebufferAttachmentTarget.h"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/RenderPipelineState.hpp"
#include "snap/rhi/backend/opengl/VertexBuffers.hpp"

#include <array>
#include <vector>

namespace snap::rhi::backend::opengl {
class Buffer;
class Framebuffer;
class RenderPipeline;

GLenum convertToGLWindingMode(const snap::rhi::Winding winding);
GLenum convertToGLCullFace(const snap::rhi::CullMode cullMode);
GLenum convertToGLPolygonMode(const snap::rhi::PolygonMode polygonMode);
GLenum convertToGLStencilOp(const snap::rhi::StencilOp stencilOp);
GLenum convertToGLBlendOperation(const snap::rhi::BlendOp blendOp);
GLenum convertToGLBlendFactor(const snap::rhi::BlendFactor blendFactor);
GLenum convertToGLCompareFunc(const snap::rhi::CompareFunction function);
GLenum convertToGLPrimitiveType(const snap::rhi::Topology topology);
GLenum convertToGLIndexType(const snap::rhi::IndexType indexType);

uint32_t getGLIndexTypeByteSize(const GLenum indexType);

void setupColorBlendState(const Profile& gl,
                          const snap::rhi::ColorBlendStateCreateInfo& colorBlendState,
                          DeviceContext* dc);
void setupMultisampleState(const Profile& gl,
                           const snap::rhi::MultisampleStateCreateInfo& multisampleState,
                           DeviceContext* dc);
void setupInputAssemblyState(const Profile& gl,
                             const snap::rhi::InputAssemblyStateCreateInfo& inputAssemblyState,
                             DeviceContext* dc);
void setupRasterizationState(const Profile& gl,
                             const snap::rhi::RasterizationStateCreateInfo& rasterizationState,
                             DeviceContext* dc);
void setupDepthSettings(const Profile& gl,
                        const snap::rhi::DepthStencilStateCreateInfo& depthStencilState,
                        DeviceContext* dc);
void setupStencilSettings(const Profile& gl,
                          const snap::rhi::DepthStencilStateCreateInfo& depthStencilState,
                          const std::array<uint32_t, 2>& stencilReference,
                          DeviceContext* dc);

struct RenderInfo {
    GLenum primitiveType = GL_NONE;
    GLenum indexType = GL_NONE;

    snap::rhi::backend::opengl::Buffer* indexBuffer = nullptr;
    uint32_t indexBufferOffset = 0;

    RenderingInfo renderingInfo{};
    snap::rhi::backend::opengl::FramebufferId fboID = snap::rhi::backend::opengl::FramebufferId::CurrSurfaceBackbuffer;

    /**
     * In order to clear FBO attachments OpenGL have to override color/depth/stencil masks,
     * but since RenderPipelineStatesUUID store active state, SnapRHI have to restore original states in order to
     * support caching. If SnapRHI will change depth/stencil/color states, than SnapRHI may ignore original
     * states, else SnapRHI have to restore original states.
     */
    GLbitfield restoreMask = GL_NONE;
    bool isFBOBound = false;

    std::vector<FramebufferAttachmentTarget> drawBuffers;
    FramebufferTarget target = FramebufferTarget::Detached;
    snap::rhi::backend::opengl::RenderPipeline* renderPipeline = nullptr;

    /**
     * [0] => StencilFace::Front
     * [1] => StencilFace::Back
     */
    std::array<uint32_t, 2> stencilReference{0, 0};

    VertexBuffers vertexBuffers;
    RenderPipelineStatesUUID renderPipelineStatesUUID{};

public:
    void clear() {
        primitiveType = GL_NONE;
        indexType = GL_NONE;

        indexBuffer = nullptr;
        indexBufferOffset = 0;

        renderingInfo = {};
        fboID = snap::rhi::backend::opengl::FramebufferId::CurrSurfaceBackbuffer;
        isFBOBound = false;

        drawBuffers.clear();
        target = FramebufferTarget::Detached;
        renderPipeline = nullptr;

        stencilReference.fill(0);

        vertexBuffers.clear();
        renderPipelineStatesUUID = {};
    }

    RenderInfo(const Profile& gl) : vertexBuffers(gl) {
        clear();
    }
};

void setStencilReference(
    const Profile& gl, RenderInfo& renderInfo, const StencilFace face, const uint32_t reference, DeviceContext* dc);
} // namespace snap::rhi::backend::opengl
