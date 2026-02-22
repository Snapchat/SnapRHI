#include "snap/rhi/backend/opengl/VertexBuffers.hpp"
#include "snap/rhi/backend/opengl/Buffer.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/RenderPipeline.hpp"
#include <snap/rhi/common/Scope.h>

namespace snap::rhi::backend::opengl {
VertexBuffers::VertexBuffers(const Profile& gl) : gl(gl) {
    clear();
}

void VertexBuffers::setBuffer(const uint32_t binding, Buffer* buffer, const uint32_t offset) {
    assert(binding < snap::rhi::MaxVertexBuffers);

    if ((buffers[binding] == buffer) && (offsets[binding] == offset)) {
        return;
    }

    buffers[binding] = buffer;
    offsets[binding] = offset;
    updateMask.set(binding);
}

void VertexBuffers::setPipeline(const RenderPipeline* renderPipeline) {
    if (this->renderPipeline != renderPipeline) {
        this->renderPipeline = renderPipeline;
        updateMask.set();
    }
}

void VertexBuffers::bind(DeviceContext* dc) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[VertexBuffers][bind]");

    if (updateMask.none()) {
        return;
    }

    const auto& validationLayer = gl.getDevice()->getValidationLayer();
    const auto& caps = renderPipeline->getDevice()->getCapabilities();
    const auto& vertexDescription = renderPipeline->getVertexDescriptor();
    const auto& info = renderPipeline->getCreateInfo();
    const auto& vertexLayout = info.vertexInputState;

    assert(updateMask.size() >= vertexLayout.bindingsCount);
    for (uint32_t i = 0; i < vertexLayout.bindingsCount; ++i) {
        const auto& bindingDesc = vertexLayout.bindingDescription[i];
        SNAP_RHI_VALIDATE(validationLayer,
                          buffers[bindingDesc.binding],
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::RenderPassOp,
                          "[VertexBuffers::bind] there is no buffer with binding: %d.",
                          bindingDesc.binding);

        if (!updateMask.test(i)) {
            continue;
        }

        Buffer* vertexBuffer = snap::rhi::backend::common::smart_cast<Buffer>(buffers[bindingDesc.binding]);

        const uint32_t divisor =
            bindingDesc.inputRate == snap::rhi::VertexInputRate::PerInstance ? bindingDesc.divisor : 0;
        const auto& bufferInfo = vertexDescription.bindings[bindingDesc.binding];
        for (uint32_t j = 0; j < bufferInfo.attributesCount; ++j) {
            const auto& attribInfo = bufferInfo.attributes[j];

            const uint32_t attributeOffset = offsets[bindingDesc.binding] + attribInfo.offset;

            const void* offset = nullptr;
            bool isVBMapped = false;

            if (vertexBuffer->getGLBuffer(dc) != GL_NONE) {
                offset = reinterpret_cast<const void*>(attributeOffset);
            } else {
                offset = vertexBuffer->map(snap::rhi::MemoryAccess::Read, 0, WholeSize, dc) + attributeOffset;
                isVBMapped = true;
            }

            SNAP_RHI_ON_SCOPE_EXIT {
                if (isVBMapped) {
                    vertexBuffer->unmap(dc);
                }
            };

            assert(attribInfo.location < isAttribEnabled.size());
            GLuint location = attribInfo.location;

            if (bindingDesc.inputRate == snap::rhi::VertexInputRate::Constant) {
                isAttribEnabled.set(location, false);
                gl.disableVertexAttribArray(location, dc);

                const void* bufferData = offset;
                if (!isVBMapped) {
                    bufferData = vertexBuffer->map(snap::rhi::MemoryAccess::Read, 0, WholeSize, dc) + attributeOffset;
                    isVBMapped = true;
                }

                if (attribInfo.type == GL_FLOAT) {
                    std::array<GLfloat, 4> attribData{0.0f, 0.0f, 0.0f, 0.0f};
                    memcpy(attribData.data(), bufferData, sizeof(float) * attribInfo.componentsCount);
                    gl.vertexAttrib4fv(location, attribData.data());
                } else if (attribInfo.type == GL_BYTE) {
                    std::array<GLint, 4> attribData{0, 0, 0, 0};

                    const int8_t* ptr = reinterpret_cast<const int8_t*>(bufferData);
                    for (GLint i = 0; i < attribInfo.componentsCount; ++i) {
                        attribData[i] = *(ptr + i);
                    }
                    gl.vertexAttribI4iv(location, attribData.data());
                } else if (attribInfo.type == GL_SHORT) {
                    std::array<GLint, 4> attribData{0, 0, 0, 0};

                    const int16_t* ptr = reinterpret_cast<const int16_t*>(bufferData);
                    for (GLint i = 0; i < attribInfo.componentsCount; ++i) {
                        attribData[i] = *(ptr + i);
                    }
                    gl.vertexAttribI4iv(location, attribData.data());
                } else if (attribInfo.type == GL_UNSIGNED_BYTE) {
                    std::array<GLuint, 4> attribData{0, 0, 0, 0};

                    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(bufferData);
                    for (GLint i = 0; i < attribInfo.componentsCount; ++i) {
                        attribData[i] = *(ptr + i);
                    }
                    gl.vertexAttribI4uiv(location, attribData.data());
                } else if (attribInfo.type == GL_UNSIGNED_SHORT) {
                    std::array<GLuint, 4> attribData{0, 0, 0, 0};

                    const uint16_t* ptr = reinterpret_cast<const uint16_t*>(bufferData);
                    for (GLint i = 0; i < attribInfo.componentsCount; ++i) {
                        attribData[i] = *(ptr + i);
                    }
                    gl.vertexAttribI4uiv(location, attribData.data());
                } else {
                    SNAP_RHI_REPORT(gl.getDevice()->getValidationLayer(),
                                    snap::rhi::ReportLevel::Error,
                                    snap::rhi::ValidationTag::RenderPassOp,
                                    "[bindVertexBuffers] unsupported attribute type: %d.",
                                    attribInfo.type);
                }
            } else {
                isAttribEnabled.set(location, true);
                gl.enableVertexAttribArray(location, dc);

                /**
                 * We should use glVertexAttribFormat/glVertexAttribBinding/glVertexBindingDivisor +
                 * glBindVertexBuffer for OpenGL ES 3.1+
                 */
                gl.vertexAttrib(location,
                                attribInfo.componentsCount,
                                attribInfo.type,
                                attribInfo.normalized,
                                attribInfo.stride,
                                offset,
                                vertexBuffer->getGLBuffer(dc),
                                dc);
                gl.vertexAttribDivisor(location, divisor, dc);
            }
        }
    }

    /**
     * It is necessary to turn off unused attributes, as this can greatly affect performance.
     */
    uint32_t maxAttribs = std::min(static_cast<uint32_t>(isAttribEnabled.size()), caps.maxVertexInputAttributes);
    for (uint32_t i = 0; i < maxAttribs; ++i) {
        if (!isAttribEnabled.test(i)) {
            gl.disableVertexAttribArray(static_cast<GLuint>(i), dc);
        }
    }

    updateMask.reset();
}

void VertexBuffers::clear() {
    buffers.fill(nullptr);
    offsets.fill(0);

    renderPipeline = nullptr;
    isAttribEnabled.reset();
    updateMask.reset();
}
} // namespace snap::rhi::backend::opengl
// TODO(vdeviatkov): We should use glVertexAttribFormat/glVertexAttribBinding/glVertexBindingDivisor for gles 3.1+
// https://www.khronos.org/opengl/wiki/Vertex_Specification#Vertex_format
// void glVertexAttrib*Pointer(GLuint index​, GLint size​, GLenum type​, {GLboolean normalized​,} GLsizei
// stride​, const GLvoid * pointer​)
//{
//  glVertexAttrib*Format(index, size, type, {normalized,} 0);
//  glVertexAttribBinding(index, index);
//
//  GLuint buffer;
//  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, buffer);
//
//  if(buffer == 0)
//    glErrorOut(GL_INVALID_OPERATION); // Give an error.
//
//  if(stride == 0)
//    stride = CalcStride(size, type);
//
//  GLintptr offset = reinterpret_cast<GLintptr>(pointer);
//  glBindVertexBuffer(index, buffer, offset, stride);
//}
