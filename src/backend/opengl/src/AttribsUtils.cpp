#include "snap/rhi/backend/opengl/AttribsUtils.hpp"

#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Utils.hpp"

#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ranges>
#include <vector>

namespace {
GLboolean isNormalizedFormat(const snap::rhi::VertexAttributeFormat vertexFormat) {
    switch (vertexFormat) {
        case snap::rhi::VertexAttributeFormat::Byte2Normalized:
        case snap::rhi::VertexAttributeFormat::Byte3Normalized:
        case snap::rhi::VertexAttributeFormat::Byte4Normalized:

        case snap::rhi::VertexAttributeFormat::UnsignedByte2Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedByte3Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedByte4Normalized:

        case snap::rhi::VertexAttributeFormat::Short2Normalized:
        case snap::rhi::VertexAttributeFormat::Short3Normalized:
        case snap::rhi::VertexAttributeFormat::Short4Normalized:

        case snap::rhi::VertexAttributeFormat::UnsignedShort2Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedShort3Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedShort4Normalized:
            return GL_TRUE;

        case snap::rhi::VertexAttributeFormat::Byte2:
        case snap::rhi::VertexAttributeFormat::Byte3:
        case snap::rhi::VertexAttributeFormat::Byte4:

        case snap::rhi::VertexAttributeFormat::UnsignedByte2:
        case snap::rhi::VertexAttributeFormat::UnsignedByte3:
        case snap::rhi::VertexAttributeFormat::UnsignedByte4:

        case snap::rhi::VertexAttributeFormat::Short2:
        case snap::rhi::VertexAttributeFormat::Short3:
        case snap::rhi::VertexAttributeFormat::Short4:

        case snap::rhi::VertexAttributeFormat::UnsignedShort2:
        case snap::rhi::VertexAttributeFormat::UnsignedShort3:
        case snap::rhi::VertexAttributeFormat::UnsignedShort4:

        case snap::rhi::VertexAttributeFormat::HalfFloat2:
        case snap::rhi::VertexAttributeFormat::HalfFloat3:
        case snap::rhi::VertexAttributeFormat::HalfFloat4:

        case snap::rhi::VertexAttributeFormat::Float:
        case snap::rhi::VertexAttributeFormat::Float2:
        case snap::rhi::VertexAttributeFormat::Float3:
        case snap::rhi::VertexAttributeFormat::Float4:
            return GL_FALSE;

        default:
            snap::rhi::common::throwException("invalid format");
    }
    return GL_FALSE;
}

GLint convertFormatToComponentCount(const snap::rhi::VertexAttributeFormat vertexFormat) {
    switch (vertexFormat) {
        case snap::rhi::VertexAttributeFormat::Float:
            return 1;

        case snap::rhi::VertexAttributeFormat::Byte2:
        case snap::rhi::VertexAttributeFormat::Byte2Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedByte2:
        case snap::rhi::VertexAttributeFormat::UnsignedByte2Normalized:
        case snap::rhi::VertexAttributeFormat::Short2:
        case snap::rhi::VertexAttributeFormat::Short2Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedShort2:
        case snap::rhi::VertexAttributeFormat::UnsignedShort2Normalized:
        case snap::rhi::VertexAttributeFormat::HalfFloat2:
        case snap::rhi::VertexAttributeFormat::Float2:
            return 2;

        case snap::rhi::VertexAttributeFormat::Byte3:
        case snap::rhi::VertexAttributeFormat::Byte3Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedByte3:
        case snap::rhi::VertexAttributeFormat::UnsignedByte3Normalized:
        case snap::rhi::VertexAttributeFormat::Short3:
        case snap::rhi::VertexAttributeFormat::Short3Normalized:
        case snap::rhi::VertexAttributeFormat::HalfFloat3:
        case snap::rhi::VertexAttributeFormat::UnsignedShort3:
        case snap::rhi::VertexAttributeFormat::UnsignedShort3Normalized:
        case snap::rhi::VertexAttributeFormat::Float3:
            return 3;

        case snap::rhi::VertexAttributeFormat::Byte4:
        case snap::rhi::VertexAttributeFormat::Byte4Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedByte4:
        case snap::rhi::VertexAttributeFormat::UnsignedByte4Normalized:
        case snap::rhi::VertexAttributeFormat::Short4:
        case snap::rhi::VertexAttributeFormat::Short4Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedShort4:
        case snap::rhi::VertexAttributeFormat::UnsignedShort4Normalized:
        case snap::rhi::VertexAttributeFormat::HalfFloat4:
        case snap::rhi::VertexAttributeFormat::Float4:
            return 4;

        default:
            snap::rhi::common::throwException("invalid format");
    }
    return -1;
}

GLenum convertToGLAttributeType(const snap::rhi::VertexAttributeFormat vertexFormat) {
    switch (vertexFormat) {
        case snap::rhi::VertexAttributeFormat::Byte2:
        case snap::rhi::VertexAttributeFormat::Byte3:
        case snap::rhi::VertexAttributeFormat::Byte4:

        case snap::rhi::VertexAttributeFormat::Byte2Normalized:
        case snap::rhi::VertexAttributeFormat::Byte3Normalized:
        case snap::rhi::VertexAttributeFormat::Byte4Normalized:
            return GL_BYTE;

        case snap::rhi::VertexAttributeFormat::UnsignedByte2:
        case snap::rhi::VertexAttributeFormat::UnsignedByte3:
        case snap::rhi::VertexAttributeFormat::UnsignedByte4:

        case snap::rhi::VertexAttributeFormat::UnsignedByte2Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedByte3Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedByte4Normalized:
            return GL_UNSIGNED_BYTE;

        case snap::rhi::VertexAttributeFormat::Short2:
        case snap::rhi::VertexAttributeFormat::Short3:
        case snap::rhi::VertexAttributeFormat::Short4:

        case snap::rhi::VertexAttributeFormat::Short2Normalized:
        case snap::rhi::VertexAttributeFormat::Short3Normalized:
        case snap::rhi::VertexAttributeFormat::Short4Normalized:
            return GL_SHORT;

        case snap::rhi::VertexAttributeFormat::UnsignedShort2:
        case snap::rhi::VertexAttributeFormat::UnsignedShort3:
        case snap::rhi::VertexAttributeFormat::UnsignedShort4:

        case snap::rhi::VertexAttributeFormat::UnsignedShort2Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedShort3Normalized:
        case snap::rhi::VertexAttributeFormat::UnsignedShort4Normalized:
            return GL_UNSIGNED_SHORT;

        case snap::rhi::VertexAttributeFormat::HalfFloat2:
        case snap::rhi::VertexAttributeFormat::HalfFloat3:
        case snap::rhi::VertexAttributeFormat::HalfFloat4:
            return GL_HALF_FLOAT; // OpenGL ES 3.0+

        case snap::rhi::VertexAttributeFormat::Float:
        case snap::rhi::VertexAttributeFormat::Float2:
        case snap::rhi::VertexAttributeFormat::Float3:
        case snap::rhi::VertexAttributeFormat::Float4:
            return GL_FLOAT;

        default:
            snap::rhi::common::throwException("invalid format");
    }
    return -1;
}

constexpr uint32_t InvalidLocation = std::numeric_limits<uint32_t>::max();

snap::rhi::backend::opengl::VertexDescriptor createVertexDescriptor(
    const std::vector<snap::rhi::backend::opengl::PipelineNativeInfo::AttribInfo>& attribsInfo,
    const std::optional<snap::rhi::opengl::RenderPipelineInfo>& glRenderPipelineInfo,
    const snap::rhi::VertexInputStateCreateInfo& layout,
    const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    snap::rhi::backend::opengl::VertexDescriptor vertexDescription{};
    const auto& attribs = layout.attributeDescription;
    const auto& bindings = layout.bindingDescription;

    if (!layout.attributesCount) {
        return vertexDescription;
    }

    for (const auto& attribInfo : attribsInfo) {
        uint32_t attributeID = InvalidLocation;

        /**
         * If reflection has been provided, SnapRHI can locate the attribute's location in the layout via
         * reflection by name. If no reflection has been provided, SnapRHI will assume that the attribute's
         * location is the attribute's native location.
         */
        if (!glRenderPipelineInfo) {
            attributeID = attribInfo.location;
        } else {
            auto attribItr = std::find_if(glRenderPipelineInfo->vertexAttributes.begin(),
                                          glRenderPipelineInfo->vertexAttributes.end(),
                                          [name = attribInfo.name](const auto& info) { return info.name == name; });
            SNAP_RHI_VALIDATE(validationLayer,
                              attribItr != glRenderPipelineInfo->vertexAttributes.end(),
                              snap::rhi::ReportLevel::Error,
                              snap::rhi::ValidationTag::RenderPipelineOp,
                              "[buildVertexDescriptor] Attribute not found");
            attributeID = attribItr->location;
        }

        assert(attributeID != InvalidLocation);
        auto attribItr = std::find_if(attribs.begin(),
                                      attribs.begin() + layout.attributesCount,
                                      [attributeID](const snap::rhi::VertexInputAttributeDescription& attrib) {
                                          return attrib.location == attributeID;
                                      });
        SNAP_RHI_VALIDATE(validationLayer,
                          attribItr != (attribs.begin() + layout.attributesCount),
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::RenderPipelineOp,
                          "[buildVertexDescriptor] Attribute not found");

        const auto& attribDesc = *attribItr;
        assert(attribDesc.binding < snap::rhi::MaxVertexBuffers);
        auto& bufferBindingInfo = vertexDescription.bindings[attribDesc.binding];
        const uint32_t attributeIdx = bufferBindingInfo.attributesCount++;

        assert(attributeIdx < snap::rhi::MaxVertexAttributesPerBuffer);
        auto& currentAttrib = bufferBindingInfo.attributes[attributeIdx];

        auto bindingItr =
            std::find_if(bindings.begin(),
                         bindings.begin() + layout.bindingsCount,
                         [bindingID = attribDesc.binding](const snap::rhi::VertexInputBindingDescription& bindingDesc) {
                             return bindingDesc.binding == bindingID;
                         });
        SNAP_RHI_VALIDATE(validationLayer,
                          bindingItr != bindings.begin() + layout.bindingsCount,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::RenderPipelineOp,
                          "[buildVertexDescriptor] invalid binding");
        SNAP_RHI_VALIDATE(validationLayer,
                          bindingItr->stride != snap::rhi::Undefined,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::RenderPipelineOp,
                          "[buildVertexDescriptor] buffer stride Undefined");

        currentAttrib.location = attribInfo.location;
        currentAttrib.normalized = isNormalizedFormat(attribDesc.format);
        currentAttrib.componentsCount = convertFormatToComponentCount(attribDesc.format);
        currentAttrib.type = convertToGLAttributeType(attribDesc.format);
        currentAttrib.offset = attribDesc.offset;
        currentAttrib.stride = bindingItr->stride;
    }
    return vertexDescription;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
VertexDescriptor buildVertexDescriptor(
    const PipelineNativeInfo& pipelineNativeInfo,
    const RenderPipelineConfigurationInfo& renderPipelineConfigInfo,
    std::vector<snap::rhi::reflection::VertexAttributeInfo>& vertexAttributesReflection,
    const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    if (renderPipelineConfigInfo.renderPipelineInfo) { // Shader with reflection
        const auto& vertexAttributes = renderPipelineConfigInfo.renderPipelineInfo->vertexAttributes;
        const auto& layout = renderPipelineConfigInfo.layout;

        assert(layout.bindingsCount <= snap::rhi::MaxVertexBuffers);
        std::unordered_map<std::string, size_t> attributesNameToId =
            snap::rhi::backend::common::buildNameToId(vertexAttributes);

        for (const auto& attribInfo : pipelineNativeInfo.attribsInfo) {
            auto itr = attributesNameToId.find(attribInfo.name);
            SNAP_RHI_VALIDATE(validationLayer,
                              itr != attributesNameToId.end(),
                              snap::rhi::ReportLevel::Error,
                              snap::rhi::ValidationTag::DeviceOp,
                              "[buildAttribsLocation] insufficient reflection, attribute \"%s\" not found.",
                              attribInfo.name.data());
            const auto& logicalInfo = vertexAttributes[itr->second];

            const std::span<const snap::rhi::VertexInputAttributeDescription> attribs{
                layout.attributeDescription.data(), layout.attributesCount};
            auto attribItr =
                std::ranges::find_if(attribs, [&](const snap::rhi::VertexInputAttributeDescription& attrib) {
                    return attrib.location == static_cast<uint32_t>(logicalInfo.location);
                });
            SNAP_RHI_VALIDATE(validationLayer,
                              attribItr != attribs.end(),
                              snap::rhi::ReportLevel::Error,
                              snap::rhi::ValidationTag::DeviceOp,
                              "[buildAttribsLocation] attribute location %d not found in vertex input layout.",
                              logicalInfo.location);

            vertexAttributesReflection.push_back(snap::rhi::reflection::VertexAttributeInfo{
                .name = logicalInfo.name,
                .location = attribItr->location,
                .format = attribItr->format,
            });
        }
    } else { // Shader without reflection
        for (const auto& attribInfo : pipelineNativeInfo.attribsInfo) {
            snap::rhi::reflection::VertexAttributeInfo info{};
            info.name = attribInfo.name;
            info.location = static_cast<uint32_t>(attribInfo.location);
            info.format = attribInfo.format;

            vertexAttributesReflection.push_back(info);
        }
    }

    return createVertexDescriptor(pipelineNativeInfo.attribsInfo,
                                  renderPipelineConfigInfo.renderPipelineInfo,
                                  renderPipelineConfigInfo.layout,
                                  validationLayer);
}
} // namespace snap::rhi::backend::opengl
