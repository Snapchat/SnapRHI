#include "snap/rhi/backend/opengl/CommandRecorderUtils.hpp"

#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/RenderPipeline.hpp"

#include "snap/rhi/Compare.hpp"
#include "snap/rhi/common/Throw.h"

namespace snap::rhi::backend::opengl {
GLenum convertToGLWindingMode(const snap::rhi::Winding winding) {
    switch (winding) {
        case snap::rhi::Winding::CW:
            return GL_CW;

        case snap::rhi::Winding::CCW:
            return GL_CCW;

        default:
            snap::rhi::common::throwException("invalid winding mode");
    }
}

GLenum convertToGLCullFace(const snap::rhi::CullMode cullMode) {
    switch (cullMode) {
        case snap::rhi::CullMode::Front:
            return GL_FRONT;

        case snap::rhi::CullMode::Back:
            return GL_BACK;

        default:
            snap::rhi::common::throwException("invalid cull mode");
    }
}

GLenum convertToGLPolygonMode(const snap::rhi::PolygonMode polygonMode) {
    switch (polygonMode) {
        case snap::rhi::PolygonMode::Fill:
            return GL_FILL;

        case snap::rhi::PolygonMode::Line:
            return GL_LINE;

        default:
            snap::rhi::common::throwException("invalid polygon mode");
    }
}

GLenum convertToGLStencilOp(const snap::rhi::StencilOp stencilOp) {
    switch (stencilOp) {
        case snap::rhi::StencilOp::Keep:
            return GL_KEEP;
        case snap::rhi::StencilOp::Zero:
            return GL_ZERO;
        case snap::rhi::StencilOp::Replace:
            return GL_REPLACE;
        case snap::rhi::StencilOp::IncAndClamp:
            return GL_INCR;
        case snap::rhi::StencilOp::DecAndClamp:
            return GL_DECR;
        case snap::rhi::StencilOp::Invert:
            return GL_INVERT;
        case snap::rhi::StencilOp::IncAndWrap:
            return GL_INCR_WRAP;
        case snap::rhi::StencilOp::DecAndWrap:
            return GL_DECR_WRAP;
        default:
            snap::rhi::common::throwException("invalid stencil operation");
    }
}

GLenum convertToGLBlendOperation(const snap::rhi::BlendOp blendOp) {
    switch (blendOp) {
        case snap::rhi::BlendOp::Add:
            return GL_FUNC_ADD;
        case snap::rhi::BlendOp::Subtract:
            return GL_FUNC_SUBTRACT;
        case snap::rhi::BlendOp::ReverseSubtract:
            return GL_FUNC_REVERSE_SUBTRACT;

        // should support with extension
        case snap::rhi::BlendOp::Min:
            return GL_MIN;
        case snap::rhi::BlendOp::Max:
            return GL_MAX;

        default:
            snap::rhi::common::throwException("invalid blend operation");
    }
}

GLenum convertToGLBlendFactor(const snap::rhi::BlendFactor blendFactor) {
    switch (blendFactor) {
        case snap::rhi::BlendFactor::Zero:
            return GL_ZERO;
        case snap::rhi::BlendFactor::One:
            return GL_ONE;
        case snap::rhi::BlendFactor::SrcColor:
            return GL_SRC_COLOR;
        case snap::rhi::BlendFactor::OneMinusSrcColor:
            return GL_ONE_MINUS_SRC_COLOR;
        case snap::rhi::BlendFactor::DstColor:
            return GL_DST_COLOR;
        case snap::rhi::BlendFactor::OneMinusDstColor:
            return GL_ONE_MINUS_DST_COLOR;
        case snap::rhi::BlendFactor::SrcAlpha:
            return GL_SRC_ALPHA;
        case snap::rhi::BlendFactor::OneMinusSrcAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case snap::rhi::BlendFactor::DstAlpha:
            return GL_DST_ALPHA;
        case snap::rhi::BlendFactor::OneMinusDstAlpha:
            return GL_ONE_MINUS_DST_ALPHA;
        case snap::rhi::BlendFactor::ConstantColor:
            return GL_CONSTANT_COLOR;
        case snap::rhi::BlendFactor::OneMinusConstantColor:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case snap::rhi::BlendFactor::ConstantAlpha:
            return GL_CONSTANT_ALPHA;
        case snap::rhi::BlendFactor::OneMinusConstantAlpha:
            return GL_ONE_MINUS_CONSTANT_ALPHA;
        case snap::rhi::BlendFactor::SrcAlphaSaturate:
            return GL_SRC_ALPHA_SATURATE;

        default:
            snap::rhi::common::throwException("invalid blend factor");
    }
}

GLenum convertToGLCompareFunc(const snap::rhi::CompareFunction function) {
    switch (function) {
        case snap::rhi::CompareFunction::Never:
            return GL_NEVER;
        case snap::rhi::CompareFunction::Less:
            return GL_LESS;
        case snap::rhi::CompareFunction::Equal:
            return GL_EQUAL;
        case snap::rhi::CompareFunction::LessEqual:
            return GL_LEQUAL;
        case snap::rhi::CompareFunction::Greater:
            return GL_GREATER;
        case snap::rhi::CompareFunction::NotEqual:
            return GL_NOTEQUAL;
        case snap::rhi::CompareFunction::GreaterEqual:
            return GL_GEQUAL;
        case snap::rhi::CompareFunction::Always:
            return GL_ALWAYS;

        default:
            snap::rhi::common::throwException("invalid depth compare function");
    }
}

GLenum convertToGLPrimitiveType(const snap::rhi::Topology topology) {
    switch (topology) {
        case snap::rhi::Topology::Triangles:
            return GL_TRIANGLES;

        case snap::rhi::Topology::TriangleStrip:
            return GL_TRIANGLE_STRIP;

        case snap::rhi::Topology::Lines:
            return GL_LINES;

        case snap::rhi::Topology::LineStrip:
            return GL_LINE_STRIP;

        case snap::rhi::Topology::Points:
            return GL_POINTS;

        default:
            snap::rhi::common::throwException("invalid topology");
    }
}

GLenum convertToGLIndexType(const snap::rhi::IndexType indexType) {
    switch (indexType) {
        case snap::rhi::IndexType::UInt16:
            return GL_UNSIGNED_SHORT;

        case snap::rhi::IndexType::UInt32:
            return GL_UNSIGNED_INT;

        case snap::rhi::IndexType::None:
            return GL_NONE;

        default:
            snap::rhi::common::throwException("invalid index type");
    }
}

uint32_t getGLIndexTypeByteSize(const GLenum indexType) {
    switch (indexType) {
        case GL_UNSIGNED_SHORT:
            return 2;

        case GL_UNSIGNED_INT:
            return 4;

        default:
            snap::rhi::common::throwException("invalid index type");
    }
}

void setupDepthSettings(const Profile& gl,
                        const snap::rhi::DepthStencilStateCreateInfo& depthStencilState,
                        DeviceContext* dc) {
    depthStencilState.depthTest ? gl.enable(GL_DEPTH_TEST, dc) : gl.disable(GL_DEPTH_TEST, dc);
    gl.depthMask(depthStencilState.depthWrite ? GL_TRUE : GL_FALSE);
    gl.depthFunc(convertToGLCompareFunc(depthStencilState.depthFunc), dc);
}

void setupStencilSettings(const Profile& gl,
                          const snap::rhi::DepthStencilStateCreateInfo& depthStencilState,
                          const std::array<uint32_t, 2>& stencilReference,
                          DeviceContext* dc) {
    depthStencilState.stencilEnable ? gl.enable(GL_STENCIL_TEST, dc) : gl.disable(GL_STENCIL_TEST, dc);
    if (depthStencilState.stencilEnable) {
        gl.stencilFuncSeparate(GL_FRONT,
                               convertToGLCompareFunc(depthStencilState.stencilFront.stencilFunc),
                               stencilReference[static_cast<uint32_t>(snap::rhi::StencilFace::Front)],
                               static_cast<uint32_t>(depthStencilState.stencilFront.readMask),
                               dc);
        gl.stencilMaskSeparate(GL_FRONT, static_cast<uint32_t>(depthStencilState.stencilFront.writeMask), dc);
        gl.stencilOpSeparate(GL_FRONT,
                             convertToGLStencilOp(depthStencilState.stencilFront.failOp),
                             convertToGLStencilOp(depthStencilState.stencilFront.depthFailOp),
                             convertToGLStencilOp(depthStencilState.stencilFront.passOp),
                             dc);
        gl.stencilFuncSeparate(GL_BACK,
                               convertToGLCompareFunc(depthStencilState.stencilBack.stencilFunc),
                               stencilReference[static_cast<uint32_t>(snap::rhi::StencilFace::Back)],
                               static_cast<uint32_t>(depthStencilState.stencilBack.readMask),
                               dc);
        gl.stencilMaskSeparate(GL_BACK, static_cast<uint32_t>(depthStencilState.stencilBack.writeMask), dc);
        gl.stencilOpSeparate(GL_BACK,
                             convertToGLStencilOp(depthStencilState.stencilBack.failOp),
                             convertToGLStencilOp(depthStencilState.stencilBack.depthFailOp),
                             convertToGLStencilOp(depthStencilState.stencilBack.passOp),
                             dc);
    }
}

void setStencilReference(
    const Profile& gl, RenderInfo& renderInfo, const StencilFace face, const uint32_t reference, DeviceContext* dc) {
    if (face == snap::rhi::StencilFace::FrontAndBack) {
        renderInfo.stencilReference[static_cast<uint32_t>(snap::rhi::StencilFace::Front)] = reference;
        renderInfo.stencilReference[static_cast<uint32_t>(snap::rhi::StencilFace::Back)] = reference;

        if (renderInfo.renderPipeline) {
            const auto& depthStencilState = renderInfo.renderPipeline->getCreateInfo().depthStencilState;
            gl.stencilFunc(
                snap::rhi::backend::opengl::convertToGLCompareFunc(depthStencilState.stencilFront.stencilFunc),
                reference,
                static_cast<uint32_t>(depthStencilState.stencilFront.readMask),
                dc);
        }
    } else {
        renderInfo.stencilReference[static_cast<uint32_t>(face)] = reference;

        if (renderInfo.renderPipeline) {
            const auto& depthStencilState = renderInfo.renderPipeline->getCreateInfo().depthStencilState;

            if (face == snap::rhi::StencilFace::Front) {
                gl.stencilFuncSeparate(
                    GL_FRONT,
                    snap::rhi::backend::opengl::convertToGLCompareFunc(depthStencilState.stencilFront.stencilFunc),
                    reference,
                    static_cast<uint32_t>(depthStencilState.stencilFront.readMask),
                    dc);
            } else {
                gl.stencilFuncSeparate(
                    GL_BACK,
                    snap::rhi::backend::opengl::convertToGLCompareFunc(depthStencilState.stencilBack.stencilFunc),
                    reference,
                    static_cast<uint32_t>(depthStencilState.stencilBack.readMask),
                    dc);
            }
        }
    }
}

void setupMultisampleState(const Profile& gl,
                           const snap::rhi::MultisampleStateCreateInfo& multisampleState,
                           DeviceContext* dc) {
    if (multisampleState.alphaToCoverageEnable) {
        gl.enable(GL_SAMPLE_ALPHA_TO_COVERAGE, dc);
        //        gl.sampleCoverage(1.0f, GL_FALSE);
    } else {
        gl.disable(GL_SAMPLE_ALPHA_TO_COVERAGE, dc);
    }

#if SNAP_RHI_GL_ES
    if (multisampleState.alphaToOneEnable == true) {
        // GL_SAMPLE_ALPHA_TO_ONE only for desktop
        snap::rhi::common::throwException<snap::rhi::UnsupportedOperationException>(
            "[setupMultisampleState] OpenGL ES doesn't support GL_SAMPLE_ALPHA_TO_ONE");
    }
#else
    if (multisampleState.alphaToOneEnable) {
        gl.enable(GL_SAMPLE_ALPHA_TO_ONE, dc);
    } else {
        gl.disable(GL_SAMPLE_ALPHA_TO_ONE, dc);
    }
#endif
}

void setupColorBlendState(const Profile& gl,
                          const snap::rhi::ColorBlendStateCreateInfo& colorBlendState,
                          DeviceContext* dc) {
    const auto& features = gl.getFeatures();
    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                      colorBlendState.colorAttachmentsCount < 2 ||
                          (colorBlendState.colorAttachmentsCount > 1 && features.isMRTSupported),
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandBufferOp,
                      "[setupColorBlendState] OpenGL ES 2.0 doesn't supported multiple attachments.");

    if (features.isDifferentBlendSettingsSupported) {
        for (uint32_t i = 0; i < colorBlendState.colorAttachmentsCount; ++i) {
            const auto& state = colorBlendState.colorAttachmentsBlendState[i];

            gl.colorMaski(i,
                          (state.colorWriteMask & snap::rhi::ColorMask::R) != snap::rhi::ColorMask::None,
                          (state.colorWriteMask & snap::rhi::ColorMask::G) != snap::rhi::ColorMask::None,
                          (state.colorWriteMask & snap::rhi::ColorMask::B) != snap::rhi::ColorMask::None,
                          (state.colorWriteMask & snap::rhi::ColorMask::A) != snap::rhi::ColorMask::None,
                          dc);

            state.blendEnable ? gl.enablei(GL_BLEND, i, dc) : gl.disablei(GL_BLEND, i, dc);
            gl.blendEquationSeparatei(i,
                                      snap::rhi::backend::opengl::convertToGLBlendOperation(state.colorBlendOp),
                                      snap::rhi::backend::opengl::convertToGLBlendOperation(state.alphaBlendOp),
                                      dc);
            gl.blendFuncSeparatei(i,
                                  snap::rhi::backend::opengl::convertToGLBlendFactor(state.srcColorBlendFactor),
                                  snap::rhi::backend::opengl::convertToGLBlendFactor(state.dstColorBlendFactor),
                                  snap::rhi::backend::opengl::convertToGLBlendFactor(state.srcAlphaBlendFactor),
                                  snap::rhi::backend::opengl::convertToGLBlendFactor(state.dstAlphaBlendFactor),
                                  dc);
        }
    } else if (colorBlendState.colorAttachmentsCount > 0) {
        for (uint32_t i = 1; i < colorBlendState.colorAttachmentsCount; ++i) {
            SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                              colorBlendState.colorAttachmentsBlendState[0] ==
                                  colorBlendState.colorAttachmentsBlendState[i],
                              snap::rhi::ReportLevel::Error,
                              snap::rhi::ValidationTag::CommandBufferOp,
                              "[setupColorBlendState] All colorAttachmentsBlendState should have same settings, "
                              "but attachment [%d] is different from attachment [0].",
                              i);
        }

        const auto& state = colorBlendState.colorAttachmentsBlendState[0];

        gl.colorMask((state.colorWriteMask & snap::rhi::ColorMask::R) != snap::rhi::ColorMask::None,
                     (state.colorWriteMask & snap::rhi::ColorMask::G) != snap::rhi::ColorMask::None,
                     (state.colorWriteMask & snap::rhi::ColorMask::B) != snap::rhi::ColorMask::None,
                     (state.colorWriteMask & snap::rhi::ColorMask::A) != snap::rhi::ColorMask::None,
                     dc);

        state.blendEnable ? gl.enable(GL_BLEND, dc) : gl.disable(GL_BLEND, dc);
        gl.blendEquationSeparate(snap::rhi::backend::opengl::convertToGLBlendOperation(state.colorBlendOp),
                                 snap::rhi::backend::opengl::convertToGLBlendOperation(state.alphaBlendOp),
                                 dc);
        gl.blendFuncSeparate(snap::rhi::backend::opengl::convertToGLBlendFactor(state.srcColorBlendFactor),
                             snap::rhi::backend::opengl::convertToGLBlendFactor(state.dstColorBlendFactor),
                             snap::rhi::backend::opengl::convertToGLBlendFactor(state.srcAlphaBlendFactor),
                             snap::rhi::backend::opengl::convertToGLBlendFactor(state.dstAlphaBlendFactor),
                             dc);
    }
}

void setupInputAssemblyState(const Profile& gl,
                             const snap::rhi::InputAssemblyStateCreateInfo& inputAssemblyState,
                             DeviceContext* dc) {
    // Do nothing
}

void setupRasterizationState(const Profile& gl,
                             const snap::rhi::RasterizationStateCreateInfo& rasterizationState,
                             DeviceContext* dc) {
    const auto& caps = gl.getDevice()->getCapabilities();
    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                      rasterizationState.clipDistancePlaneCount <= caps.maxClipDistances,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandBufferOp,
                      "[setupRasterizationState] clipDistancePlaneCount(%d) > Capabilities::maxClipDistances(%d)",
                      rasterizationState.clipDistancePlaneCount,
                      caps.maxClipDistances);

    SNAP_RHI_VALIDATE(gl.getDevice()->getValidationLayer(),
                      rasterizationState.rasterizationEnabled || caps.isRasterizerDisableSupported,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CommandBufferOp,
                      "[setupRasterizationState] RasterizerDisabled isn't supported",
                      rasterizationState.clipDistancePlaneCount,
                      caps.maxClipDistances);

    rasterizationState.rasterizationEnabled ? gl.disable(GL_RASTERIZER_DISCARD, dc) :
                                              gl.enable(GL_RASTERIZER_DISCARD, dc);

    gl.polygonMode(GL_FRONT_AND_BACK,
                   snap::rhi::backend::opengl::convertToGLPolygonMode(rasterizationState.polygonMode));

    if (rasterizationState.cullMode != snap::rhi::CullMode::None) {
        gl.enable(GL_CULL_FACE, dc);
        gl.cullFace(snap::rhi::backend::opengl::convertToGLCullFace(rasterizationState.cullMode), dc);
    } else {
        gl.disable(GL_CULL_FACE, dc);
    }

    for (uint32_t i = 0; i < rasterizationState.clipDistancePlaneCount; ++i) {
        gl.enable(GL_CLIP_DISTANCE0 + i, dc);
    }

    gl.frontFace(snap::rhi::backend::opengl::convertToGLWindingMode(rasterizationState.windingMode));

    if (rasterizationState.polygonMode == snap::rhi::PolygonMode::Fill) {
        rasterizationState.depthBiasEnable ? gl.enable(GL_POLYGON_OFFSET_FILL, dc) :
                                             gl.disable(GL_POLYGON_OFFSET_FILL, dc);
    } else if (rasterizationState.polygonMode == snap::rhi::PolygonMode::Line) {
        rasterizationState.depthBiasEnable ? gl.enable(GL_POLYGON_OFFSET_LINE, dc) :
                                             gl.disable(GL_POLYGON_OFFSET_LINE, dc);
    } else {
        snap::rhi::common::throwException<snap::rhi::InvalidArgumentException>(
            "[setupRasterizationState] invalid PolygonMode");
    }
}
} // namespace snap::rhi::backend::opengl
