#include "snap/rhi/backend/metal/Utils.h"
#include "snap/rhi/Common.h"
#include "snap/rhi/RenderPass.hpp"
#include "snap/rhi/backend/common/PipelineLayout.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/RenderPipeline.h"
#include "snap/rhi/backend/metal/ShaderModule.h"
#include "snap/rhi/backend/metal/Texture.h"
#include "snap/rhi/common/Throw.h"

namespace {
MTLLoadAction convertToMtlLoadAction(const snap::rhi::AttachmentLoadOp loadOp) {
    switch (loadOp) {
        case snap::rhi::AttachmentLoadOp::Load:
            return MTLLoadActionLoad;

        case snap::rhi::AttachmentLoadOp::Clear:
            return MTLLoadActionClear;

        case snap::rhi::AttachmentLoadOp::DontCare:
            return MTLLoadActionDontCare;

        default:
            snap::rhi::common::throwException("invalid AttachmentLoadOp");
    }
}

MTLStoreAction convertToMtlStoreAction(snap::rhi::AttachmentStoreOp storeOp) {
    switch (storeOp) {
        case snap::rhi::AttachmentStoreOp::DontCare:
            return MTLStoreActionDontCare;

        case snap::rhi::AttachmentStoreOp::Store:
            return MTLStoreActionStore;

        default:
            snap::rhi::common::throwException("invalid AttachmentStoreOp");
    }
}

NSUInteger getTextureSlice(const snap::rhi::TextureType textureType, const uint32_t slice, const uint32_t depth) {
    switch (textureType) {
        case snap::rhi::TextureType::Texture2D:
        // Metal makes a distinction between slices and depth planes for 3D textures and requires that the slice is
        // always 0 for 3D textures.
        case snap::rhi::TextureType::Texture3D:
            return 0;

        case snap::rhi::TextureType::TextureCubemap: {
            assert(slice < 6);
            return slice;
        }

        case snap::rhi::TextureType::Texture2DArray: {
            assert(slice < depth);
            return slice;
        }

        default:
            snap::rhi::common::throwException("invalid texture slice");
    }
}

NSUInteger getTextureDepthPlane(const snap::rhi::TextureType textureType, const uint32_t slice, const uint32_t depth) {
    switch (textureType) {
        case snap::rhi::TextureType::Texture2D:
        case snap::rhi::TextureType::Texture2DArray:
        case snap::rhi::TextureType::TextureCubemap:
            return 0;

        case snap::rhi::TextureType::Texture3D: {
            assert(slice < depth);
            return slice;
        }

        default:
            snap::rhi::common::throwException("invalid DepthPlane");
    }
}

snap::rhi::reflection::UniformFormat convertToUniformFormat(MTLDataType dataType) {
    switch (dataType) {
        case MTLDataTypeBool:
            return snap::rhi::reflection::UniformFormat::Bool;
        case MTLDataTypeBool2:
            return snap::rhi::reflection::UniformFormat::Bool2;
        case MTLDataTypeBool3:
            return snap::rhi::reflection::UniformFormat::Bool3;
        case MTLDataTypeBool4:
            return snap::rhi::reflection::UniformFormat::Bool4;

        case MTLDataTypeInt:
            return snap::rhi::reflection::UniformFormat::Int;
        case MTLDataTypeInt2:
            return snap::rhi::reflection::UniformFormat::Int2;
        case MTLDataTypeInt3:
            return snap::rhi::reflection::UniformFormat::Int3;
        case MTLDataTypeInt4:
            return snap::rhi::reflection::UniformFormat::Int4;

        case MTLDataTypeUInt:
            return snap::rhi::reflection::UniformFormat::UInt;
        case MTLDataTypeUInt2:
            return snap::rhi::reflection::UniformFormat::UInt2;
        case MTLDataTypeUInt3:
            return snap::rhi::reflection::UniformFormat::UInt3;
        case MTLDataTypeUInt4:
            return snap::rhi::reflection::UniformFormat::UInt4;

        case MTLDataTypeFloat:
            return snap::rhi::reflection::UniformFormat::Float;
        case MTLDataTypeFloat2:
            return snap::rhi::reflection::UniformFormat::Float2;
        case MTLDataTypeFloat3:
            return snap::rhi::reflection::UniformFormat::Float3;
        case MTLDataTypeFloat4:
            return snap::rhi::reflection::UniformFormat::Float4;

        case MTLDataTypeFloat2x2:
            return snap::rhi::reflection::UniformFormat::Float2x2;
        case MTLDataTypeFloat3x3:
            return snap::rhi::reflection::UniformFormat::Float3x3;
        case MTLDataTypeFloat4x4:
            return snap::rhi::reflection::UniformFormat::Float4x4;

        default:
            snap::rhi::common::throwException("[convertToUniformFormat] unsupported data format");
    }
}

uint32_t computeStride(MTLDataType dataType) {
    // https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf#page12
    // Table 2.3. Size and alignment of vector data types
    // Table 2.5. Size and alignment of matrix data types
    switch (dataType) {
        case MTLDataTypeBool:
            return sizeof(uint8_t);
        case MTLDataTypeBool2:
            return sizeof(uint8_t) * 2;
        case MTLDataTypeBool3:
            return sizeof(uint8_t) * 4;
        case MTLDataTypeBool4:
            return sizeof(uint8_t) * 4;

        case MTLDataTypeInt:
            return sizeof(int32_t);
        case MTLDataTypeInt2:
            return sizeof(int32_t) * 2;
        case MTLDataTypeInt3:
            return sizeof(int32_t) * 4;
        case MTLDataTypeInt4:
            return sizeof(int32_t) * 4;

        case MTLDataTypeUInt:
            return sizeof(uint32_t);
        case MTLDataTypeUInt2:
            return sizeof(uint32_t) * 2;
        case MTLDataTypeUInt3:
            return sizeof(uint32_t) * 4;
        case MTLDataTypeUInt4:
            return sizeof(uint32_t) * 4;

        case MTLDataTypeFloat:
            return sizeof(float);
        case MTLDataTypeFloat2:
            return sizeof(float) * 2;
        case MTLDataTypeFloat3:
            return sizeof(float) * 4; // not a typo
        case MTLDataTypeFloat4:
            return sizeof(float) * 4;

        case MTLDataTypeFloat2x2:
            return sizeof(float) * 2 * 2;
        case MTLDataTypeFloat3x3:
            return sizeof(float) * 3 * 4; // not a typo
        case MTLDataTypeFloat3x4:
            return sizeof(float) * 3 * 4;
        case MTLDataTypeFloat4x3:
            return sizeof(float) * 4 * 4; // not a typo
        case MTLDataTypeFloat4x4:
            return sizeof(float) * 4 * 4;

        default:
            snap::rhi::common::throwException("[computeStride] unsupported data format");
    }
}

struct BufferDescription {
    std::vector<snap::rhi::reflection::BufferMemberInfo> members;
    uint32_t size = 0;
};

void processAtomic(const std::string& parentName,
                   int parentOffset,
                   MTLDataType dataType,
                   std::vector<snap::rhi::reflection::BufferMemberInfo>& membersInfo) {
    snap::rhi::reflection::BufferMemberInfo info{};
    info.name = parentName;
    info.offset = parentOffset;
    info.stride = computeStride(dataType);
    info.format = convertToUniformFormat(dataType);
    info.arraySize = 0;
    membersInfo.push_back(info);
}

void processAtomic(const std::string& parentName,
                   int parentOffset,
                   MTLStructMember* member,
                   std::vector<snap::rhi::reflection::BufferMemberInfo>& membersInfo) {
    std::string key = parentName + snap::rhi::backend::metal::getString(member.name);
    processAtomic(key, parentOffset + static_cast<int>(member.offset), member.dataType, membersInfo);
}

void processArrayUniform(const std::string& parentName,
                         int parentOffset,
                         MTLArrayType* arrayinfo,
                         std::vector<snap::rhi::reflection::BufferMemberInfo>& membersInfo);
void processArrayUniform(const std::string& parentName,
                         int parentOffset,
                         MTLStructMember* member,
                         std::vector<snap::rhi::reflection::BufferMemberInfo>& membersInfo);

void processStructUniform(const std::string& parentName,
                          int parentOffset,
                          MTLStructType* member,
                          std::vector<snap::rhi::reflection::BufferMemberInfo>& membersInfo) {
    for (MTLStructMember* structmember in member.members) {
        std::string key = parentName;

        if (structmember.dataType == MTLDataTypeArray) {
            processArrayUniform(parentName, parentOffset, structmember, membersInfo);
        } else {
            processAtomic(parentName, parentOffset, structmember, membersInfo);
        }
    }
}

void processArrayUniform(const std::string& parentName,
                         int parentOffset,
                         MTLArrayType* arrayinfo,
                         std::vector<snap::rhi::reflection::BufferMemberInfo>& membersInfo) {
    if (arrayinfo.elementType != MTLDataTypeStruct) {
        snap::rhi::reflection::BufferMemberInfo info{};
        info.name = parentName;
        info.offset = parentOffset;
        info.stride = computeStride(arrayinfo.elementType);
        info.format = convertToUniformFormat(arrayinfo.elementType);
        info.arraySize = static_cast<uint32_t>(arrayinfo.arrayLength);
        membersInfo.push_back(info);
    }

    for (NSUInteger a = 0; a < arrayinfo.arrayLength; a++) {
        std::string key = parentName;
        key += "[" + std::to_string(a) + "]";
        if (arrayinfo.elementType == MTLDataTypeStruct) {
            processStructUniform(key + ".",
                                 static_cast<int>(parentOffset + arrayinfo.stride * a),
                                 arrayinfo.elementStructType,
                                 membersInfo);
        } else {
            processAtomic(
                key, static_cast<int>(parentOffset + arrayinfo.stride * a), arrayinfo.elementType, membersInfo);
        }
    }
}

void processStructUniform(const std::string& parentName,
                          int parentOffset,
                          MTLStructMember* member,
                          std::vector<snap::rhi::reflection::BufferMemberInfo>& membersInfo) {
    MTLStructType* structinfo = member.structType;
    processStructUniform(parentName + snap::rhi::backend::metal::getString(member.name) + ".",
                         static_cast<int>(parentOffset + member.offset),
                         structinfo,
                         membersInfo);
}

void processArrayUniform(const std::string& parentName,
                         int parentOffset,
                         MTLStructMember* member,
                         std::vector<snap::rhi::reflection::BufferMemberInfo>& membersInfo) {
    MTLArrayType* arrayinfo = member.arrayType;
    processArrayUniform(parentName + snap::rhi::backend::metal::getString(member.name),
                        parentOffset + static_cast<int>(member.offset),
                        arrayinfo,
                        membersInfo);
}

BufferDescription buildBufferDescription(MTLPointerType* pointerType) {
    BufferDescription result{};

    result.size = static_cast<uint32_t>(pointerType.dataSize);
    if (pointerType.elementType == MTLDataTypeStruct) {
        for (MTLStructMember* structmember in pointerType.elementStructType.members) {
            if (structmember.dataType == MTLDataTypeStruct) {
                processStructUniform("", 0, structmember, result.members);
            } else if (structmember.dataType == MTLDataTypeArray) {
                processArrayUniform("", 0, structmember, result.members);
            } else {
                processAtomic("", 0, structmember, result.members);
            }
        }
    } else if (pointerType.elementType == MTLDataTypeArray) {
        processArrayUniform("", 0, pointerType.elementArrayType, result.members);
    } else {
        // A plain data pointer should not be processed.
    }

    return result;
}

snap::rhi::reflection::DescriptorSetInfo buildDescriptorSetInfo(
    MTLArgument* arg, const std::vector<std::unordered_set<uint32_t>>& writableResources) {
    snap::rhi::reflection::DescriptorSetInfo dsInfo{};
    dsInfo.binding = static_cast<uint32_t>(arg.index);

    for (MTLStructMember* uniform in arg.bufferStructType.members) {
        const std::string uniformName = snap::rhi::backend::metal::getString(uniform.name);
        const uint32_t argIdx = static_cast<uint32_t>(uniform.argumentIndex);
        const bool isWritable =
            dsInfo.binding < writableResources.size() && writableResources[dsInfo.binding].count(argIdx);

        if (uniform.dataType == MTLDataTypePointer) {
            BufferDescription bufferDesc = buildBufferDescription(uniform.pointerType);

            if (uniform.pointerType.access == MTLArgumentAccessReadOnly && !isWritable) {
                snap::rhi::reflection::UniformBufferInfo info{};

                info.name = uniformName;
                info.binding = argIdx;
                info.size = bufferDesc.size;
                info.membersInfo = std::move(bufferDesc.members);

                dsInfo.uniformBuffers.push_back(info);
            } else {
                snap::rhi::reflection::StorageBufferInfo info{};

                info.name = uniformName;
                info.binding = argIdx;
                info.size = bufferDesc.size;
                info.membersInfo = std::move(bufferDesc.members);

                dsInfo.storageBuffers.push_back(info);
            }
        } else if (uniform.dataType == MTLDataTypeSampler) {
            snap::rhi::reflection::SamplerInfo info{};

            info.name = uniformName;
            info.binding = argIdx;
            info.type = snap::rhi::reflection::SamplerType::Undefined;

            dsInfo.samplers.push_back(info);
        } else if (uniform.dataType == MTLDataTypeTexture) {
            if (uniform.textureReferenceType.access == MTLArgumentAccessReadOnly && !isWritable) {
                snap::rhi::reflection::SampledTextureInfo info{};

                info.name = uniformName;
                info.binding = argIdx;
                info.type =
                    snap::rhi::backend::metal::convertToReflectionTexType(uniform.textureReferenceType.textureType);

                dsInfo.sampledTextures.push_back(info);
            } else {
                snap::rhi::reflection::StorageTextureInfo info{};

                info.name = uniformName;
                info.binding = argIdx;
                info.type =
                    snap::rhi::backend::metal::convertToReflectionTexType(uniform.textureReferenceType.textureType);
                info.access = snap::rhi::backend::metal::convertToImageAccess(uniform.textureReferenceType.access);

                dsInfo.storageTextures.push_back(info);
            }
        }
    }

    return dsInfo;
}

template<typename T>
void verifyBufferResources(std::vector<T>& dst, const std::vector<T>& str) {
    for (const auto& info : str) {
        const auto itr = std::find_if(dst.begin(), dst.end(), [&info](const T& val) { return val.name == info.name; });

        if (itr == dst.end()) {
            snap::rhi::common::throwException("[verifyBufferResources] incompatible shader buffers");
        } else if (*itr != info) {
            if (itr->binding != info.binding) {
                snap::rhi::common::throwException("[verifyBufferResources] wrong shader buffer description");
            }
            itr->size = std::max(itr->size, info.size);

            if (itr->membersInfo != info.membersInfo) {
                for (const auto& srcMemberInfo : info.membersInfo) {
                    const auto membersItr =
                        std::find_if(itr->membersInfo.begin(),
                                     itr->membersInfo.end(),
                                     [&srcMemberInfo](const snap::rhi::reflection::BufferMemberInfo& val) {
                                         return val.name == srcMemberInfo.name;
                                     });
                    if (membersItr == itr->membersInfo.end()) {
                        /**
                         * It's ok since original uniforms offset should be the same
                         */
                        itr->membersInfo.push_back(srcMemberInfo);
                    } else if (*membersItr != srcMemberInfo) {
                        snap::rhi::common::throwException("[verifyBufferResources] incompatible shader buffers layout");
                    }
                }
            }
        }
    }
}

template<typename T>
void verifyTextureResources(std::vector<T>& dst, const std::vector<T>& str) {
    for (const auto& info : str) {
        const auto itr = std::find_if(dst.begin(), dst.end(), [&info](const T& val) { return val.name == info.name; });

        if (itr == dst.end() || *itr != info) {
            snap::rhi::common::throwException("[verifyTextureResources] wrong shader texture/image bindings");
        }
    }
}

void verifySamplerResources(std::vector<snap::rhi::reflection::SamplerInfo>& dst,
                            const std::vector<snap::rhi::reflection::SamplerInfo>& src) {
    for (const auto& info : src) {
        const auto itr = std::find_if(dst.begin(), dst.end(), [&info](const snap::rhi::reflection::SamplerInfo& val) {
            return val.name == info.name;
        });

        if (itr == dst.end() || *itr != info) {
            snap::rhi::common::throwException("[verifySamplerResources] wrong shader sampler bindings");
        }
    }
}

void processArguments(NSArray<MTLArgument*>* arguments,
                      std::vector<snap::rhi::reflection::DescriptorSetInfo>& descriptorSetInfos,
                      const std::vector<std::unordered_set<uint32_t>>& writableResources) {
    for (MTLArgument* arg in arguments) {
        if (arg.type == MTLArgumentTypeBuffer && arg.bufferPointerType.elementIsArgumentBuffer == YES) {
            snap::rhi::reflection::DescriptorSetInfo dsInfo = buildDescriptorSetInfo(arg, writableResources);

            auto dsItr = std::find_if(descriptorSetInfos.begin(),
                                      descriptorSetInfos.end(),
                                      [dsID = dsInfo.binding](const snap::rhi::reflection::DescriptorSetInfo& info) {
                                          return info.binding == dsID;
                                      });
            if (dsItr == descriptorSetInfos.end()) {
                descriptorSetInfos.push_back(dsInfo);
            } else {
                if (*dsItr != dsInfo) {
                    verifyTextureResources(dsItr->sampledTextures, dsInfo.sampledTextures);
                    verifyTextureResources(dsItr->storageTextures, dsInfo.storageTextures);

                    verifySamplerResources(dsItr->samplers, dsInfo.samplers);

                    verifyBufferResources(dsItr->uniformBuffers, dsInfo.uniformBuffers);
                    verifyBufferResources(dsItr->storageBuffers, dsInfo.storageBuffers);
                }
            }
        }
    }
}

std::vector<std::unordered_set<uint32_t>> prepareWritableResourcesIds(const snap::rhi::PipelineLayout* pipelineLayout) {
    if (!pipelineLayout) {
        return {};
    }

    const auto* pipelineLayoutImpl =
        snap::rhi::backend::common::smart_cast<const snap::rhi::backend::common::PipelineLayout>(pipelineLayout);
    const auto& info = pipelineLayoutImpl->getSetLayouts();

    std::vector<std::unordered_set<uint32_t>> result(info.size());
    for (size_t i = 0; i < result.size(); ++i) {
        auto& dsWritableResources = result[i];
        const auto& dsInfo = info[i];

        if (!dsInfo) {
            continue;
        }

        for (const auto& descriptorInfo : dsInfo->bindings) {
            if (descriptorInfo.descriptorType == snap::rhi::DescriptorType::StorageTexture ||
                descriptorInfo.descriptorType == snap::rhi::DescriptorType::StorageBuffer) {
                dsWritableResources.insert(descriptorInfo.binding);
            }
        }
    }

    return result;
}

std::vector<snap::rhi::reflection::DescriptorSetInfo> buildReflection(const snap::rhi::PipelineLayout* pipelineLayout) {
    if (!pipelineLayout) {
        snap::rhi::common::throwException("[prepareWritableResourcesIds] pipelineLayout is nullptr");
    }

    const auto* pipelineLayoutImpl =
        snap::rhi::backend::common::smart_cast<const snap::rhi::backend::common::PipelineLayout>(pipelineLayout);
    const auto& info = pipelineLayoutImpl->getSetLayouts();

    std::vector<snap::rhi::reflection::DescriptorSetInfo> result(info.size());
    for (size_t i = 0; i < result.size(); ++i) {
        auto& dsResources = result[i];
        const auto& dsInfo = info[i];

        dsResources.binding = static_cast<uint32_t>(i);
        if (!dsInfo) {
            continue;
        }

        for (const auto& descriptorInfo : dsInfo->bindings) {
            switch (descriptorInfo.descriptorType) {
                case snap::rhi::DescriptorType::StorageTexture: {
                    dsResources.storageTextures.push_back({.binding = descriptorInfo.binding});
                } break;

                case snap::rhi::DescriptorType::SampledTexture: {
                    dsResources.sampledTextures.push_back({.binding = descriptorInfo.binding});
                } break;

                case snap::rhi::DescriptorType::StorageBuffer: {
                    dsResources.storageBuffers.push_back({.binding = descriptorInfo.binding});
                } break;

                case snap::rhi::DescriptorType::UniformBuffer: {
                    dsResources.uniformBuffers.push_back({.binding = descriptorInfo.binding});
                } break;

                case snap::rhi::DescriptorType::Sampler: {
                    dsResources.samplers.push_back({.binding = descriptorInfo.binding});
                } break;

                default:
                    snap::rhi::common::throwException("[buildReflection] unexpected descriptor type");
            }
        }
    }

    return result;
}

MTLVertexFormat convertToMtlVertexFormat(const snap::rhi::VertexAttributeFormat vertexFormat) {
    switch (vertexFormat) {
        case snap::rhi::VertexAttributeFormat::Byte2:
            return MTLVertexFormatChar2;
        case snap::rhi::VertexAttributeFormat::Byte3:
            return MTLVertexFormatChar3;
        case snap::rhi::VertexAttributeFormat::Byte4:
            return MTLVertexFormatChar4;

        case snap::rhi::VertexAttributeFormat::Byte2Normalized:
            return MTLVertexFormatChar2Normalized;
        case snap::rhi::VertexAttributeFormat::Byte3Normalized:
            return MTLVertexFormatChar3Normalized;
        case snap::rhi::VertexAttributeFormat::Byte4Normalized:
            return MTLVertexFormatChar4Normalized;

        case snap::rhi::VertexAttributeFormat::UnsignedByte2:
            return MTLVertexFormatUChar2;
        case snap::rhi::VertexAttributeFormat::UnsignedByte3:
            return MTLVertexFormatUChar3;
        case snap::rhi::VertexAttributeFormat::UnsignedByte4:
            return MTLVertexFormatUChar4;

        case snap::rhi::VertexAttributeFormat::UnsignedByte2Normalized:
            return MTLVertexFormatUChar2Normalized;
        case snap::rhi::VertexAttributeFormat::UnsignedByte3Normalized:
            return MTLVertexFormatUChar3Normalized;
        case snap::rhi::VertexAttributeFormat::UnsignedByte4Normalized:
            return MTLVertexFormatUChar4Normalized;

        case snap::rhi::VertexAttributeFormat::Short2:
            return MTLVertexFormatShort2;
        case snap::rhi::VertexAttributeFormat::Short3:
            return MTLVertexFormatShort3;
        case snap::rhi::VertexAttributeFormat::Short4:
            return MTLVertexFormatShort4;

        case snap::rhi::VertexAttributeFormat::Short2Normalized:
            return MTLVertexFormatShort2Normalized;
        case snap::rhi::VertexAttributeFormat::Short3Normalized:
            return MTLVertexFormatShort3Normalized;
        case snap::rhi::VertexAttributeFormat::Short4Normalized:
            return MTLVertexFormatShort4Normalized;

        case snap::rhi::VertexAttributeFormat::UnsignedShort2:
            return MTLVertexFormatUShort2;
        case snap::rhi::VertexAttributeFormat::UnsignedShort3:
            return MTLVertexFormatUShort3;
        case snap::rhi::VertexAttributeFormat::UnsignedShort4:
            return MTLVertexFormatUShort4;

        case snap::rhi::VertexAttributeFormat::UnsignedShort2Normalized:
            return MTLVertexFormatUShort2Normalized;
        case snap::rhi::VertexAttributeFormat::UnsignedShort3Normalized:
            return MTLVertexFormatUShort3Normalized;
        case snap::rhi::VertexAttributeFormat::UnsignedShort4Normalized:
            return MTLVertexFormatUShort4Normalized;

        case snap::rhi::VertexAttributeFormat::HalfFloat2:
            return MTLVertexFormatHalf2;
        case snap::rhi::VertexAttributeFormat::HalfFloat3:
            return MTLVertexFormatHalf3;
        case snap::rhi::VertexAttributeFormat::HalfFloat4:
            return MTLVertexFormatHalf4;

        case snap::rhi::VertexAttributeFormat::Float:
            return MTLVertexFormatFloat;
        case snap::rhi::VertexAttributeFormat::Float2:
            return MTLVertexFormatFloat2;
        case snap::rhi::VertexAttributeFormat::Float3:
            return MTLVertexFormatFloat3;
        case snap::rhi::VertexAttributeFormat::Float4:
            return MTLVertexFormatFloat4;

        case snap::rhi::VertexAttributeFormat::Int:
            return MTLVertexFormatInt;
        case snap::rhi::VertexAttributeFormat::Int2:
            return MTLVertexFormatInt2;
        case snap::rhi::VertexAttributeFormat::Int3:
            return MTLVertexFormatInt3;
        case snap::rhi::VertexAttributeFormat::Int4:
            return MTLVertexFormatInt4;

        case snap::rhi::VertexAttributeFormat::UInt:
            return MTLVertexFormatUInt;
        case snap::rhi::VertexAttributeFormat::UInt2:
            return MTLVertexFormatUInt2;
        case snap::rhi::VertexAttributeFormat::UInt3:
            return MTLVertexFormatUInt3;
        case snap::rhi::VertexAttributeFormat::UInt4:
            return MTLVertexFormatUInt4;

        default:
            snap::rhi::common::throwException("invalid format");
    }
    return MTLVertexFormatInvalid;
}

MTLVertexStepFunction convertToMtlStepFunction(const snap::rhi::VertexInputRate inputRate) {
    switch (inputRate) {
        case snap::rhi::VertexInputRate::PerVertex:
            return MTLVertexStepFunctionPerVertex;

        case snap::rhi::VertexInputRate::PerInstance:
            return MTLVertexStepFunctionPerInstance;

        default:
            snap::rhi::common::throwException("invalid input rate");
    }
}

MTLBlendFactor convertToMtlBlendFactor(const snap::rhi::BlendFactor blendFactor) {
    switch (blendFactor) {
        case snap::rhi::BlendFactor::Zero:
            return MTLBlendFactorZero;
        case snap::rhi::BlendFactor::One:
            return MTLBlendFactorOne;
        case snap::rhi::BlendFactor::SrcColor:
            return MTLBlendFactorSourceColor;
        case snap::rhi::BlendFactor::OneMinusSrcColor:
            return MTLBlendFactorOneMinusSourceColor;
        case snap::rhi::BlendFactor::DstColor:
            return MTLBlendFactorDestinationColor;
        case snap::rhi::BlendFactor::OneMinusDstColor:
            return MTLBlendFactorOneMinusDestinationColor;
        case snap::rhi::BlendFactor::SrcAlpha:
            return MTLBlendFactorSourceAlpha;
        case snap::rhi::BlendFactor::OneMinusSrcAlpha:
            return MTLBlendFactorOneMinusSourceAlpha;
        case snap::rhi::BlendFactor::DstAlpha:
            return MTLBlendFactorDestinationAlpha;
        case snap::rhi::BlendFactor::OneMinusDstAlpha:
            return MTLBlendFactorOneMinusDestinationAlpha;
        case snap::rhi::BlendFactor::ConstantColor:
            return MTLBlendFactorBlendColor;
        case snap::rhi::BlendFactor::OneMinusConstantColor:
            return MTLBlendFactorOneMinusBlendColor;
        case snap::rhi::BlendFactor::ConstantAlpha:
            return MTLBlendFactorBlendAlpha;
        case snap::rhi::BlendFactor::OneMinusConstantAlpha:
            return MTLBlendFactorOneMinusBlendAlpha;
        case snap::rhi::BlendFactor::SrcAlphaSaturate:
            return MTLBlendFactorSourceAlphaSaturated;

        default:
            snap::rhi::common::throwException("invalid blend factor");
    }
}

MTLBlendOperation convertToMtlBlendOperation(const snap::rhi::BlendOp blendOp) {
    switch (blendOp) {
        case snap::rhi::BlendOp::Add:
            return MTLBlendOperationAdd;
        case snap::rhi::BlendOp::Subtract:
            return MTLBlendOperationSubtract;
        case snap::rhi::BlendOp::ReverseSubtract:
            return MTLBlendOperationReverseSubtract;
        case snap::rhi::BlendOp::Min:
            return MTLBlendOperationMin;
        case snap::rhi::BlendOp::Max:
            return MTLBlendOperationMax;

        default:
            snap::rhi::common::throwException("invalid blend operation");
    }
}

void fillVertexDescriptor(MTLRenderPipelineDescriptor* pipelineDescriptor,
                          const snap::rhi::RenderPipelineCreateInfo& info) {
    /**
     * vertex buffer => max supported 8
     * uniform buffer => max supported per stage 23, and we can use slots [0; 22]
     */
    MTLVertexDescriptor* vertexDescriptor = [[MTLVertexDescriptor alloc] init];

    const auto& vertexInputState = info.vertexInputState;

    assert(info.mtlRenderPipelineInfo != std::nullopt);
    const auto& mtlRenderPipelineInfo = *info.mtlRenderPipelineInfo;
    for (uint32_t i = 0; i < vertexInputState.attributesCount; ++i) {
        const auto& attribInfo = vertexInputState.attributeDescription[i];

        vertexDescriptor.attributes[attribInfo.location].bufferIndex =
            mtlRenderPipelineInfo.vertexBufferBindingBase + attribInfo.binding;
        vertexDescriptor.attributes[attribInfo.location].offset = attribInfo.offset;
        vertexDescriptor.attributes[attribInfo.location].format = convertToMtlVertexFormat(attribInfo.format);
    }

    for (uint32_t i = 0; i < vertexInputState.bindingsCount; ++i) {
        const uint32_t binding =
            mtlRenderPipelineInfo.vertexBufferBindingBase + vertexInputState.bindingDescription[i].binding;

        if (vertexInputState.bindingDescription[i].stride == snap::rhi::Undefined) {
            snap::rhi::common::throwException("[bindVertexBuffers] buffer stride Undefined!");
        }

        if (vertexInputState.bindingDescription[i].inputRate == snap::rhi::VertexInputRate::Constant) {
            /**
             * If stepFunction is MTLVertexStepFunctionConstant, the function fetches attribute data just once,
             * and that attribute data is used for every vertex. In this case,stepRate must be set to 0.
             */
            vertexDescriptor.layouts[binding].stride = 0;
            vertexDescriptor.layouts[binding].stepRate = 0;
            vertexDescriptor.layouts[binding].stepFunction = MTLVertexStepFunctionConstant;
        } else {
            vertexDescriptor.layouts[binding].stepRate = vertexInputState.bindingDescription[i].divisor;
            vertexDescriptor.layouts[binding].stride = vertexInputState.bindingDescription[i].stride;
            vertexDescriptor.layouts[binding].stepFunction =
                convertToMtlStepFunction(vertexInputState.bindingDescription[i].inputRate);
        }
    }

    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
}

void fillInputAssembly(MTLRenderPipelineDescriptor* pipelineDescriptor,
                       const snap::rhi::InputAssemblyStateCreateInfo& info) {
    if (@available(macOS 10.11, ios 12.0, *)) {
        pipelineDescriptor.inputPrimitiveTopology =
            snap::rhi::backend::metal::convertToMtlPrimitiveTopologyClass(info.primitiveTopology);
    }
    // info.primitiveTopology used in RenderCommandEncoder
}

void fillRasterizationState(MTLRenderPipelineDescriptor* pipelineDescriptor,
                            const snap::rhi::RasterizationStateCreateInfo& info) {
    pipelineDescriptor.rasterizationEnabled = info.rasterizationEnabled;
    //    CullMode cullMode = CullMode::Back;  // used in RenderCommandEncoder
    //    Winding windingMode = Winding::CCW;  // used in RenderCommandEncoder
    //    bool depthBiasEnable = false; // ???
}

void fillMultisampleState(MTLRenderPipelineDescriptor* pipelineDescriptor,
                          const snap::rhi::MultisampleStateCreateInfo& info) {
    pipelineDescriptor.sampleCount = static_cast<uint32_t>(info.samples);
    pipelineDescriptor.alphaToCoverageEnabled = info.alphaToCoverageEnable;
    pipelineDescriptor.alphaToOneEnabled = info.alphaToOneEnable;
}

void fillColorBlendState(MTLRenderPipelineDescriptor* pipelineDescriptor,
                         const snap::rhi::ColorBlendStateCreateInfo& info) {
    for (uint32_t i = 0; i < info.colorAttachmentsCount; ++i) {
        const auto& state = info.colorAttachmentsBlendState[i];

        pipelineDescriptor.colorAttachments[i].blendingEnabled = state.blendEnable;
        pipelineDescriptor.colorAttachments[i].sourceRGBBlendFactor =
            convertToMtlBlendFactor(state.srcColorBlendFactor);
        pipelineDescriptor.colorAttachments[i].sourceAlphaBlendFactor =
            convertToMtlBlendFactor(state.srcAlphaBlendFactor);
        pipelineDescriptor.colorAttachments[i].destinationRGBBlendFactor =
            convertToMtlBlendFactor(state.dstColorBlendFactor);
        pipelineDescriptor.colorAttachments[i].destinationAlphaBlendFactor =
            convertToMtlBlendFactor(state.dstAlphaBlendFactor);
        pipelineDescriptor.colorAttachments[i].rgbBlendOperation = convertToMtlBlendOperation(state.colorBlendOp);
        pipelineDescriptor.colorAttachments[i].alphaBlendOperation = convertToMtlBlendOperation(state.alphaBlendOp);
        pipelineDescriptor.colorAttachments[i].writeMask =
            (((state.colorWriteMask & snap::rhi::ColorMask::R) != snap::rhi::ColorMask::None) ? MTLColorWriteMaskRed :
                                                                                                MTLColorWriteMaskNone) |
            (((state.colorWriteMask & snap::rhi::ColorMask::G) != snap::rhi::ColorMask::None) ? MTLColorWriteMaskGreen :
                                                                                                MTLColorWriteMaskNone) |
            (((state.colorWriteMask & snap::rhi::ColorMask::B) != snap::rhi::ColorMask::None) ? MTLColorWriteMaskBlue :
                                                                                                MTLColorWriteMaskNone) |
            (((state.colorWriteMask & snap::rhi::ColorMask::A) != snap::rhi::ColorMask::None) ? MTLColorWriteMaskAlpha :
                                                                                                MTLColorWriteMaskNone);
    }
}

void fillAttachmentsInfo(MTLRenderPipelineDescriptor* pipelineDescriptor,
                         const snap::rhi::RenderPassCreateInfo& renderPassCreateInfo,
                         const bool stencilEnable) {
    if (renderPassCreateInfo.subpassCount > 1) {
        snap::rhi::common::throwException("[SnapRHI Metal] support only 1 subpasss");
    }
    const auto& subpassDescription = renderPassCreateInfo.subpasses[0];

    for (uint32_t i = 0; i < subpassDescription.colorAttachmentCount; ++i) {
        const uint32_t attachmentIdx = subpassDescription.colorAttachments[i].attachment;
        assert(attachmentIdx < renderPassCreateInfo.attachmentCount);

        const auto& attachmentInfo = renderPassCreateInfo.attachments[attachmentIdx];

        pipelineDescriptor.colorAttachments[i].pixelFormat =
            snap::rhi::backend::metal::convertToMtlPixelFormat(attachmentInfo.format);
    }

    snap::rhi::PixelFormat depthStencilFormat = snap::rhi::PixelFormat::Undefined;
    const uint32_t depthAttachmentIdx = subpassDescription.depthStencilAttachment.attachment;
    if (depthAttachmentIdx != snap::rhi::AttachmentUnused) {
        const auto& attachmentInfo = renderPassCreateInfo.attachments[depthAttachmentIdx];
        depthStencilFormat = attachmentInfo.format;
        pipelineDescriptor.depthAttachmentPixelFormat =
            snap::rhi::backend::metal::convertToMtlPixelFormat(depthStencilFormat);
    }

    pipelineDescriptor.stencilAttachmentPixelFormat =
        (stencilEnable || snap::rhi::hasStencilAspect(depthStencilFormat)) ?
            pipelineDescriptor.depthAttachmentPixelFormat :
            MTLPixelFormatInvalid;
}

void fillAttachmentsInfo(MTLRenderPipelineDescriptor* pipelineDescriptor,
                         const snap::rhi::AttachmentFormatsCreateInfo& attachmentFormatsCreateInfo) {
    for (uint32_t i = 0; i < attachmentFormatsCreateInfo.colorAttachmentFormats.size(); ++i) {
        const auto& attachmentFormat = attachmentFormatsCreateInfo.colorAttachmentFormats[i];

        pipelineDescriptor.colorAttachments[i].pixelFormat =
            snap::rhi::backend::metal::convertToMtlPixelFormat(attachmentFormat);
    }

    if (attachmentFormatsCreateInfo.depthAttachmentFormat != snap::rhi::PixelFormat::Undefined) {
        pipelineDescriptor.depthAttachmentPixelFormat =
            snap::rhi::backend::metal::convertToMtlPixelFormat(attachmentFormatsCreateInfo.depthAttachmentFormat);
    }

    if (attachmentFormatsCreateInfo.stencilAttachmentFormat != snap::rhi::PixelFormat::Undefined) {
        pipelineDescriptor.stencilAttachmentPixelFormat =
            snap::rhi::backend::metal::convertToMtlPixelFormat(attachmentFormatsCreateInfo.stencilAttachmentFormat);
    }
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
MTLPixelFormat convertToMtlPixelFormat(const snap::rhi::PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case snap::rhi::PixelFormat::Undefined:
            return MTLPixelFormatInvalid;

        case snap::rhi::PixelFormat::R8Unorm:
            return MTLPixelFormatR8Unorm;

        case snap::rhi::PixelFormat::R8G8Unorm:
            return MTLPixelFormatRG8Unorm;

        case snap::rhi::PixelFormat::R8G8B8A8Unorm:
            return MTLPixelFormatRGBA8Unorm;

        case snap::rhi::PixelFormat::R16Unorm:
            return MTLPixelFormatR16Unorm;

        case snap::rhi::PixelFormat::R16G16Unorm:
            return MTLPixelFormatRG16Unorm;

        case snap::rhi::PixelFormat::R16G16B16A16Unorm:
            return MTLPixelFormatRGBA16Unorm;

        case snap::rhi::PixelFormat::R8Snorm:
            return MTLPixelFormatR8Snorm;

        case snap::rhi::PixelFormat::R8G8Snorm:
            return MTLPixelFormatRG8Snorm;

        case snap::rhi::PixelFormat::R8G8B8A8Snorm:
            return MTLPixelFormatRGBA8Snorm;

        case snap::rhi::PixelFormat::R16Snorm:
            return MTLPixelFormatR16Snorm;

        case snap::rhi::PixelFormat::R16G16Snorm:
            return MTLPixelFormatRG16Snorm;

        case snap::rhi::PixelFormat::R16G16B16A16Snorm:
            return MTLPixelFormatRGBA16Snorm;

        case snap::rhi::PixelFormat::R8Uint:
            return MTLPixelFormatR8Uint;

        case snap::rhi::PixelFormat::R8G8Uint:
            return MTLPixelFormatRG8Uint;

        case snap::rhi::PixelFormat::R8G8B8A8Uint:
            return MTLPixelFormatRGBA8Uint;

        case snap::rhi::PixelFormat::R16Uint:
            return MTLPixelFormatR16Uint;

        case snap::rhi::PixelFormat::R16G16Uint:
            return MTLPixelFormatRG16Uint;

        case snap::rhi::PixelFormat::R16G16B16A16Uint:
            return MTLPixelFormatRGBA16Uint;

        case snap::rhi::PixelFormat::R32Uint:
            return MTLPixelFormatR32Uint;

        case snap::rhi::PixelFormat::R32G32Uint:
            return MTLPixelFormatRG32Uint;

        case snap::rhi::PixelFormat::R32G32B32A32Uint:
            return MTLPixelFormatRGBA32Uint;

        case snap::rhi::PixelFormat::R8Sint:
            return MTLPixelFormatR8Sint;

        case snap::rhi::PixelFormat::R8G8Sint:
            return MTLPixelFormatRG8Sint;

        case snap::rhi::PixelFormat::R8G8B8A8Sint:
            return MTLPixelFormatRGBA8Sint;

        case snap::rhi::PixelFormat::R16Sint:
            return MTLPixelFormatR16Sint;

        case snap::rhi::PixelFormat::R16G16Sint:
            return MTLPixelFormatRG16Sint;

        case snap::rhi::PixelFormat::R16G16B16A16Sint:
            return MTLPixelFormatRGBA16Sint;

        case snap::rhi::PixelFormat::R32Sint:
            return MTLPixelFormatR32Sint;

        case snap::rhi::PixelFormat::R32G32Sint:
            return MTLPixelFormatRG32Sint;

        case snap::rhi::PixelFormat::R32G32B32A32Sint:
            return MTLPixelFormatRGBA32Sint;

        case snap::rhi::PixelFormat::R16Float:
            return MTLPixelFormatR16Float;

        case snap::rhi::PixelFormat::R16G16Float:
            return MTLPixelFormatRG16Float;

        case snap::rhi::PixelFormat::R16G16B16A16Float:
            return MTLPixelFormatRGBA16Float;

        case snap::rhi::PixelFormat::R32Float:
            return MTLPixelFormatR32Float;

        case snap::rhi::PixelFormat::R32G32Float:
            return MTLPixelFormatRG32Float;

        case snap::rhi::PixelFormat::R32G32B32A32Float:
            return MTLPixelFormatRGBA32Float;

        case snap::rhi::PixelFormat::Grayscale:
            return MTLPixelFormatR8Unorm;

        case snap::rhi::PixelFormat::B8G8R8A8Unorm:
            return MTLPixelFormatBGRA8Unorm;

        case snap::rhi::PixelFormat::R8G8B8A8Srgb:
            if (@available(macOS 11.0, ios 12.0, *)) {
                return MTLPixelFormatR8Unorm_sRGB;
            } else {
                return MTLPixelFormatInvalid;
            }

        case snap::rhi::PixelFormat::R10G10B10A2Unorm:
            return MTLPixelFormatRGB10A2Unorm;

        case snap::rhi::PixelFormat::R10G10B10A2Uint:
            return MTLPixelFormatRGB10A2Uint;

        case snap::rhi::PixelFormat::R11G11B10Float:
            return MTLPixelFormatRG11B10Float;

        case snap::rhi::PixelFormat::Depth16Unorm:
            return MTLPixelFormatDepth32Float; // MTLPixelFormatDepth16Unorm ?

        case snap::rhi::PixelFormat::DepthFloat:
            return MTLPixelFormatDepth32Float;

        case snap::rhi::PixelFormat::DepthStencil:
            return MTLPixelFormatDepth32Float_Stencil8;

        case snap::rhi::PixelFormat::ETC_R8G8B8_Unorm:
        case snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm: {
            MTLPixelFormat retFormat = MTLPixelFormatInvalid;
            if (@available(macOS 11.0, ios 8.0, *)) {
                retFormat = MTLPixelFormatETC2_RGB8;
            }
            return retFormat;
        }

        case snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB: {
            MTLPixelFormat retFormat = MTLPixelFormatInvalid;
            if (@available(macOS 11.0, ios 8.0, *)) {
                retFormat = MTLPixelFormatETC2_RGB8_sRGB;
            }
            return retFormat;
        }

        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm: {
            MTLPixelFormat retFormat = MTLPixelFormatInvalid;
            if (@available(macOS 11.0, ios 8.0, *)) {
                retFormat = MTLPixelFormatETC2_RGB8A1;
            }
            return retFormat;
        }

        case snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB: {
            MTLPixelFormat retFormat = MTLPixelFormatInvalid;
            if (@available(macOS 11.0, ios 8.0, *)) {
                retFormat = MTLPixelFormatETC2_RGB8A1_sRGB;
            }
            return retFormat;
        }

        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm: {
            MTLPixelFormat retFormat = MTLPixelFormatInvalid;
            if (@available(macOS 11.0, ios 8.0, *)) {
                retFormat = MTLPixelFormatEAC_RGBA8;
            }
            return retFormat;
        }

        case snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB: {
            MTLPixelFormat retFormat = MTLPixelFormatInvalid;
            if (@available(macOS 11.0, ios 8.0, *)) {
                retFormat = MTLPixelFormatEAC_RGBA8_sRGB;
            }
            return retFormat;
        }

        case snap::rhi::PixelFormat::ASTC_4x4_Unorm: {
            MTLPixelFormat retFormat = MTLPixelFormatInvalid;
            if (@available(macOS 11.0, ios 8.0, *)) {
                retFormat = MTLPixelFormatASTC_4x4_LDR;
            }
            return retFormat;
        }

        case snap::rhi::PixelFormat::ASTC_4x4_sRGB: {
            MTLPixelFormat retFormat = MTLPixelFormatInvalid;
            if (@available(macOS 11.0, ios 8.0, *)) {
                retFormat = MTLPixelFormatASTC_4x4_sRGB;
            }
            return retFormat;
        }

        default:
            return MTLPixelFormatInvalid;
    }
}

snap::rhi::PixelFormat convertMtlPixelFormatToRHI(const MTLPixelFormat pixelFormat) {
    switch (pixelFormat) {
        case MTLPixelFormatR8Unorm:
            return snap::rhi::PixelFormat::R8Unorm;

        case MTLPixelFormatRG8Unorm:
            return snap::rhi::PixelFormat::R8G8Unorm;

        case MTLPixelFormatRGBA8Unorm:
            return snap::rhi::PixelFormat::R8G8B8A8Unorm;

        case MTLPixelFormatR16Unorm:
            return snap::rhi::PixelFormat::R16Unorm;

        case MTLPixelFormatRG16Unorm:
            return snap::rhi::PixelFormat::R16G16Unorm;

        case MTLPixelFormatRGBA16Unorm:
            return snap::rhi::PixelFormat::R16G16B16A16Unorm;

        case MTLPixelFormatR8Snorm:
            return snap::rhi::PixelFormat::R8Snorm;

        case MTLPixelFormatRG8Snorm:
            return snap::rhi::PixelFormat::R8G8Snorm;

        case MTLPixelFormatRGBA8Snorm:
            return snap::rhi::PixelFormat::R8G8B8A8Snorm;

        case MTLPixelFormatR16Snorm:
            return snap::rhi::PixelFormat::R16Snorm;

        case MTLPixelFormatRG16Snorm:
            return snap::rhi::PixelFormat::R16G16Snorm;

        case MTLPixelFormatRGBA16Snorm:
            return snap::rhi::PixelFormat::R16G16B16A16Snorm;

        case MTLPixelFormatR8Uint:
            return snap::rhi::PixelFormat::R8Uint;

        case MTLPixelFormatRG8Uint:
            return snap::rhi::PixelFormat::R8G8Uint;

        case MTLPixelFormatRGBA8Uint:
            return snap::rhi::PixelFormat::R8G8B8A8Uint;

        case MTLPixelFormatR16Uint:
            return snap::rhi::PixelFormat::R16Uint;

        case MTLPixelFormatRG16Uint:
            return snap::rhi::PixelFormat::R16G16Uint;

        case MTLPixelFormatRGBA16Uint:
            return snap::rhi::PixelFormat::R16G16B16A16Uint;

        case MTLPixelFormatR32Uint:
            return snap::rhi::PixelFormat::R32Uint;

        case MTLPixelFormatRG32Uint:
            return snap::rhi::PixelFormat::R32G32Uint;

        case MTLPixelFormatRGBA32Uint:
            return snap::rhi::PixelFormat::R32G32B32A32Uint;

        case MTLPixelFormatR8Sint:
            return snap::rhi::PixelFormat::R8Sint;

        case MTLPixelFormatRG8Sint:
            return snap::rhi::PixelFormat::R8G8Sint;

        case MTLPixelFormatRGBA8Sint:
            return snap::rhi::PixelFormat::R8G8B8A8Sint;

        case MTLPixelFormatR16Sint:
            return snap::rhi::PixelFormat::R16Sint;

        case MTLPixelFormatRG16Sint:
            return snap::rhi::PixelFormat::R16G16Sint;

        case MTLPixelFormatRGBA16Sint:
            return snap::rhi::PixelFormat::R16G16B16A16Sint;

        case MTLPixelFormatR32Sint:
            return snap::rhi::PixelFormat::R32Sint;

        case MTLPixelFormatRG32Sint:
            return snap::rhi::PixelFormat::R32G32Sint;

        case MTLPixelFormatRGBA32Sint:
            return snap::rhi::PixelFormat::R32G32B32A32Sint;

        case MTLPixelFormatR16Float:
            return snap::rhi::PixelFormat::R16Float;

        case MTLPixelFormatRG16Float:
            return snap::rhi::PixelFormat::R16G16Float;

        case MTLPixelFormatRGBA16Float:
            return snap::rhi::PixelFormat::R16G16B16A16Float;

        case MTLPixelFormatR32Float:
            return snap::rhi::PixelFormat::R32Float;

        case MTLPixelFormatRG32Float:
            return snap::rhi::PixelFormat::R32G32Float;

        case MTLPixelFormatRGBA32Float:
            return snap::rhi::PixelFormat::R32G32B32A32Float;

        case MTLPixelFormatBGRA8Unorm:
            return snap::rhi::PixelFormat::B8G8R8A8Unorm;

        case MTLPixelFormatR8Unorm_sRGB:
            return snap::rhi::PixelFormat::R8G8B8A8Srgb;

        case MTLPixelFormatRGB10A2Unorm:
            return snap::rhi::PixelFormat::R10G10B10A2Unorm;

        case MTLPixelFormatRGB10A2Uint:
            return snap::rhi::PixelFormat::R10G10B10A2Uint;

        case MTLPixelFormatRG11B10Float:
            return snap::rhi::PixelFormat::R11G11B10Float;

        case MTLPixelFormatDepth32Float:
            return snap::rhi::PixelFormat::DepthFloat;

        case MTLPixelFormatDepth32Float_Stencil8:
            return snap::rhi::PixelFormat::DepthStencil;

        case MTLPixelFormatETC2_RGB8:
            return snap::rhi::PixelFormat::ETC2_R8G8B8_Unorm;

        case MTLPixelFormatETC2_RGB8_sRGB:
            return snap::rhi::PixelFormat::ETC2_R8G8B8_sRGB;

        case MTLPixelFormatETC2_RGB8A1:
            return snap::rhi::PixelFormat::ETC2_R8G8B8A1_Unorm;

        case MTLPixelFormatETC2_RGB8A1_sRGB:
            return snap::rhi::PixelFormat::ETC2_R8G8B8A1_sRGB;

        case MTLPixelFormatEAC_RGBA8:
            return snap::rhi::PixelFormat::ETC2_R8G8B8A8_Unorm;

        case MTLPixelFormatEAC_RGBA8_sRGB:
            return snap::rhi::PixelFormat::ETC2_R8G8B8A8_sRGB;

        case MTLPixelFormatASTC_4x4_LDR:
            return snap::rhi::PixelFormat::ASTC_4x4_Unorm;

        case MTLPixelFormatASTC_4x4_sRGB:
            return snap::rhi::PixelFormat::ASTC_4x4_sRGB;

        default:
            return snap::rhi::PixelFormat::Undefined;
    }
}

MTLCompareFunction convertToMtlCompareFunc(const snap::rhi::CompareFunction depthCompareFunction) {
    switch (depthCompareFunction) {
        case snap::rhi::CompareFunction::Never:
            return MTLCompareFunctionNever;
        case snap::rhi::CompareFunction::Less:
            return MTLCompareFunctionLess;
        case snap::rhi::CompareFunction::Equal:
            return MTLCompareFunctionEqual;
        case snap::rhi::CompareFunction::LessEqual:
            return MTLCompareFunctionLessEqual;
        case snap::rhi::CompareFunction::Greater:
            return MTLCompareFunctionGreater;
        case snap::rhi::CompareFunction::NotEqual:
            return MTLCompareFunctionNotEqual;
        case snap::rhi::CompareFunction::GreaterEqual:
            return MTLCompareFunctionGreaterEqual;
        case snap::rhi::CompareFunction::Always:
            return MTLCompareFunctionAlways;

        default:
            snap::rhi::common::throwException("invalid depth compare function");
    }
}

MTLPrimitiveType convertToMtlPrimitiveType(const snap::rhi::Topology topology) {
    switch (topology) {
        case snap::rhi::Topology::Triangles:
            return MTLPrimitiveTypeTriangle;

        case snap::rhi::Topology::TriangleStrip:
            return MTLPrimitiveTypeTriangleStrip;

        case snap::rhi::Topology::Lines:
            return MTLPrimitiveTypeLine;

        case snap::rhi::Topology::LineStrip:
            return MTLPrimitiveTypeLineStrip;

        case snap::rhi::Topology::Points:
            return MTLPrimitiveTypePoint;

        default:
            snap::rhi::common::throwException("invalid topology");
    }
}

API_AVAILABLE(macos(10.11), ios(12.0))
MTLPrimitiveTopologyClass convertToMtlPrimitiveTopologyClass(const snap::rhi::Topology topology) {
    switch (topology) {
        case snap::rhi::Topology::Triangles:
        case snap::rhi::Topology::TriangleStrip:
            return MTLPrimitiveTopologyClassTriangle;

        case snap::rhi::Topology::Lines:
        case snap::rhi::Topology::LineStrip:
            return MTLPrimitiveTopologyClassLine;

        case snap::rhi::Topology::Points:
            return MTLPrimitiveTopologyClassPoint;

        default:
            snap::rhi::common::throwException("invalid topology");
    }
}

std::string getString(NSString* str) {
    const size_t size = static_cast<size_t>([str length]);
    const char* strData = [str cStringUsingEncoding:NSASCIIStringEncoding];

    return std::string(strData, size);
}

NSString* getString(const std::string_view& str) {
    return [[NSString alloc] initWithBytes:reinterpret_cast<const void*>(str.data())
                                    length:str.size()
                                  encoding:NSUTF8StringEncoding];
}

Boolean getBoolean(const std::string& value) {
    auto from = 0;
    auto len = value.size() - from;
    if (value.compare(from, len, "true") == 0) {
        return YES;
    }
    if (value.compare(from, len, "false") == 0) {
        return NO;
    }
    if (value.compare(from, len, "1") == 0) {
        return YES;
    }
    if (value.compare(from, len, "0") == 0) {
        return NO;
    }

    snap::rhi::common::throwException("only true/false or 0/1 support for bool values");
}

NSInteger getInt(const std::string& value) {
    return std::stoi(value);
}

float getFloat(const std::string& value) {
    return std::stof(value);
}

MTLDataType getDataType(const snap::rhi::DescriptorType descriptorType) {
    if (@available(macOS 10.13, ios 11.0, *)) {
        switch (descriptorType) {
            case snap::rhi::DescriptorType::Sampler:
                return MTLDataTypeSampler;

            case snap::rhi::DescriptorType::SampledTexture:
            case snap::rhi::DescriptorType::StorageTexture:
                return MTLDataTypeTexture;

            case snap::rhi::DescriptorType::UniformBuffer:
            case snap::rhi::DescriptorType::StorageBuffer:
                return MTLDataTypePointer;

            default: {
                snap::rhi::common::throwException("[getDataType] invalid data type");
            }
        }
    }

    assert(false);
    return MTLDataTypeNone;
}

snap::rhi::VertexAttributeFormat convertToVtxFormat(MTLDataType dataType) {
    switch (dataType) {
        case MTLDataTypeFloat:
            return snap::rhi::VertexAttributeFormat::Float;
        case MTLDataTypeFloat2:
            return snap::rhi::VertexAttributeFormat::Float2;
        case MTLDataTypeFloat3:
            return snap::rhi::VertexAttributeFormat::Float3;
        case MTLDataTypeFloat4:
            return snap::rhi::VertexAttributeFormat::Float4;

        case MTLDataTypeInt:
            return snap::rhi::VertexAttributeFormat::Int;
        case MTLDataTypeInt2:
            return snap::rhi::VertexAttributeFormat::Int2;
        case MTLDataTypeInt3:
            return snap::rhi::VertexAttributeFormat::Int3;
        case MTLDataTypeInt4:
            return snap::rhi::VertexAttributeFormat::Int4;

        case MTLDataTypeUInt:
            return snap::rhi::VertexAttributeFormat::UInt;
        case MTLDataTypeUInt2:
            return snap::rhi::VertexAttributeFormat::UInt2;
        case MTLDataTypeUInt3:
            return snap::rhi::VertexAttributeFormat::UInt3;
        case MTLDataTypeUInt4:
            return snap::rhi::VertexAttributeFormat::UInt4;

        default:
            return snap::rhi::VertexAttributeFormat::Undefined;
    }
}

snap::rhi::reflection::ImageAccess convertToImageAccess(const MTLArgumentAccess access) {
    switch (access) {
        case MTLArgumentAccessReadOnly:
            return snap::rhi::reflection::ImageAccess::ReadOnly;

        case MTLArgumentAccessReadWrite:
            return snap::rhi::reflection::ImageAccess::ReadWrite;

        case MTLArgumentAccessWriteOnly:
            return snap::rhi::reflection::ImageAccess::WriteOnly;

        default:
            return snap::rhi::reflection::ImageAccess::Undefined;
    }
}

snap::rhi::reflection::TextureType convertToReflectionTexType(const MTLTextureType type) {
    switch (type) {
        case MTLTextureTypeTextureBuffer:
            return snap::rhi::reflection::TextureType::TextureBuffer;
        case MTLTextureType2D:
            return snap::rhi::reflection::TextureType::Texture2D;
        case MTLTextureType2DArray:
            return snap::rhi::reflection::TextureType::Texture2DArray;
        case MTLTextureTypeCube:
            return snap::rhi::reflection::TextureType::TextureCube;
        case MTLTextureType3D:
            return snap::rhi::reflection::TextureType::Texture3D;

        default:
            snap::rhi::common::throwException("[convertToReflectionTexType] invalid type");
    }
}

std::vector<snap::rhi::reflection::VertexAttributeInfo> buildVertexReflection(const id<MTLFunction>& vertexFunction) {
    std::vector<snap::rhi::reflection::VertexAttributeInfo> info{};

    for (MTLVertexAttribute* arg in vertexFunction.vertexAttributes) {
        if (arg.isActive == NO) {
            continue;
        }

        snap::rhi::reflection::VertexAttributeInfo attribInfo{};
        attribInfo.name = snap::rhi::backend::metal::getString(arg.name);
        attribInfo.location = static_cast<uint32_t>(arg.attributeIndex);
        attribInfo.format = convertToVtxFormat(arg.attributeType);

        info.push_back(attribInfo);
    }

    return info;
}

std::vector<snap::rhi::reflection::DescriptorSetInfo> buildDescriptorSetReflection(
    MTLRenderPipelineReflection* reflection, const snap::rhi::PipelineLayout* pipelineLayout) {
    std::vector<snap::rhi::reflection::DescriptorSetInfo> descriptorSetInfos{};
    if (reflection == nil) {
        descriptorSetInfos = buildReflection(pipelineLayout);
    } else {
        auto writableResources = prepareWritableResourcesIds(pipelineLayout);
        processArguments(reflection.vertexArguments, descriptorSetInfos, writableResources);
        processArguments(reflection.fragmentArguments, descriptorSetInfos, writableResources);
    }
    return descriptorSetInfos;
}

std::vector<snap::rhi::reflection::DescriptorSetInfo> buildDescriptorSetReflection(
    MTLComputePipelineReflection* reflection, const snap::rhi::PipelineLayout* pipelineLayout) {
    std::vector<snap::rhi::reflection::DescriptorSetInfo> descriptorSetInfos{};
    if (reflection == nil) {
        descriptorSetInfos = buildReflection(pipelineLayout);
    } else {
        auto writableResources = prepareWritableResourcesIds(pipelineLayout);
        processArguments(reflection.arguments, descriptorSetInfos, writableResources);
    }
    return descriptorSetInfos;
}

MTLRenderPipelineDescriptor* createRenderPipelineDescriptor(const snap::rhi::RenderPipelineCreateInfo& info,
                                                            const id<MTLFunction>& vertex,
                                                            const id<MTLFunction>& fragment) {
    MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];

    pipelineDescriptor.vertexFunction = vertex;
    pipelineDescriptor.fragmentFunction = fragment;

    fillVertexDescriptor(pipelineDescriptor, info);
    fillInputAssembly(pipelineDescriptor, info.inputAssemblyState);
    fillRasterizationState(pipelineDescriptor, info.rasterizationState);
    fillMultisampleState(pipelineDescriptor, info.multisampleState);
    fillColorBlendState(pipelineDescriptor, info.colorBlendState);

    if (info.renderPass) {
        const auto& renderPassInfo = info.renderPass->getCreateInfo();
        fillAttachmentsInfo(pipelineDescriptor, renderPassInfo, info.depthStencilState.stencilEnable);
    } else {
        fillAttachmentsInfo(pipelineDescriptor, info.attachmentFormatsCreateInfo);
    }

    return pipelineDescriptor;
}

id<MTLFunction> getFunction(std::span<snap::rhi::ShaderModule* const> shaders, const snap::rhi::ShaderStage stage) {
    for (const auto& shader : shaders) {
        if (shader->getShaderStage() == stage) {
            const snap::rhi::backend::metal::ShaderModule* mtlShaderModule =
                snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::ShaderModule>(shader);
            return mtlShaderModule->getFunction();
        }
    }

    return nil;
}

#if SNAP_RHI_OS_MACOS()
MTLRenderStages convertTo(snap::rhi::PipelineStageBits stage) {
    MTLRenderStages stages = 0;
    if ((stage & snap::rhi::PipelineStageBits::VertexShaderBit) == snap::rhi::PipelineStageBits::VertexShaderBit) {
        stages |= MTLRenderStageVertex;
    }

    if ((stage & snap::rhi::PipelineStageBits::FragmentShaderBit) == snap::rhi::PipelineStageBits::FragmentShaderBit) {
        stages |= MTLRenderStageFragment;
    }

    return stages;
}
#endif

MTLRenderPassDescriptor* createFramebuffer(const snap::rhi::RenderPassCreateInfo& renderPassInfo,
                                           std::span<snap::rhi::Texture* const> attachments,
                                           std::span<const ClearValue> clearValues) {
    MTLRenderPassDescriptor* framebuffer = [MTLRenderPassDescriptor new];

    if (renderPassInfo.subpassCount > 1) {
        snap::rhi::common::throwException("[SnapRHI Metal] support only 1 subpasss");
    }
    const auto& subpassDescription = renderPassInfo.subpasses[0];

    // https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/2879276-rendertargetwidth
    // https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/2879279-rendertargetheight
    // https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/1437975-rendertargetarraylength
    for (uint32_t i = 0; i < subpassDescription.colorAttachmentCount; ++i) {
        const uint32_t attachmentIdx = subpassDescription.colorAttachments[i].attachment;
        assert(attachmentIdx < renderPassInfo.attachmentCount);

        auto* mtlTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::metal::Texture>(
            attachments[attachmentIdx]);

        const auto& textureInfo = mtlTexture->getCreateInfo();
        const auto& attachmentInfo = renderPassInfo.attachments[attachmentIdx];

        framebuffer.colorAttachments[i].storeAction = convertToMtlStoreAction(attachmentInfo.storeOp);
        framebuffer.colorAttachments[i].loadAction = convertToMtlLoadAction(attachmentInfo.loadOp);

        if (attachmentInfo.loadOp == snap::rhi::AttachmentLoadOp::Clear) {
            assert(attachmentIdx < clearValues.size());
            const auto& clearValue = clearValues[attachmentIdx].color;
            if (isIntFormat(attachmentInfo.format)) {
                framebuffer.colorAttachments[i].clearColor = MTLClearColorMake(
                    clearValue.int32[0], clearValue.int32[1], clearValue.int32[2], clearValue.int32[3]);
            } else if (isUintFormat(attachmentInfo.format)) {
                framebuffer.colorAttachments[i].clearColor = MTLClearColorMake(
                    clearValue.uint32[0], clearValue.uint32[1], clearValue.uint32[2], clearValue.uint32[3]);
            } else {
                framebuffer.colorAttachments[i].clearColor = MTLClearColorMake(
                    clearValue.float32[0], clearValue.float32[1], clearValue.float32[2], clearValue.float32[3]);
            }
        }
        framebuffer.colorAttachments[i].texture = mtlTexture->getTexture();
        framebuffer.colorAttachments[i].level = attachmentInfo.mipLevel;
        framebuffer.colorAttachments[i].slice =
            getTextureSlice(textureInfo.textureType, attachmentInfo.layer, textureInfo.size.depth);
        framebuffer.colorAttachments[i].depthPlane =
            getTextureDepthPlane(textureInfo.textureType, attachmentInfo.layer, textureInfo.size.depth);

        if (subpassDescription.resolveAttachmentCount) {
            const uint32_t resolveAttachmentIdx = subpassDescription.resolveAttachments[i].attachment;
            assert(resolveAttachmentIdx < renderPassInfo.attachmentCount);

            auto* mtlResolveTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::metal::Texture>(
                attachments[resolveAttachmentIdx]);

            const auto& resolveTextureInfo = mtlResolveTexture->getCreateInfo();
            const auto& resolveAttachmentInfo = renderPassInfo.attachments[resolveAttachmentIdx];

            framebuffer.colorAttachments[i].resolveTexture = mtlResolveTexture->getTexture();
            framebuffer.colorAttachments[i].resolveLevel = resolveAttachmentInfo.mipLevel;
            framebuffer.colorAttachments[i].resolveSlice = getTextureSlice(
                resolveTextureInfo.textureType, resolveAttachmentInfo.layer, resolveTextureInfo.size.depth);
            framebuffer.colorAttachments[i].resolveDepthPlane = getTextureDepthPlane(
                resolveTextureInfo.textureType, resolveAttachmentInfo.layer, resolveTextureInfo.size.depth);

            if (resolveAttachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::DontCare) {
                snap::rhi::common::throwException(
                    "[Framebuffer] invalid resolveAttachment storeOp, should be AttachmentStoreOp::Store");
            }

            if (attachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::DontCare) {
                framebuffer.colorAttachments[i].storeAction = MTLStoreActionMultisampleResolve;
            } else if (attachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::Store) {
                if (@available(iOS 10.0, macOS 10.12, *)) {
                    framebuffer.colorAttachments[i].storeAction = MTLStoreActionStoreAndMultisampleResolve;
                } else {
                    snap::rhi::common::throwException("[Framebuffer] unsupported storeOp");
                }
            }
        }
    }

    const uint32_t depthAttachmentIdx = subpassDescription.depthStencilAttachment.attachment;
    if (depthAttachmentIdx != snap::rhi::AttachmentUnused) {
        assert(depthAttachmentIdx < renderPassInfo.attachmentCount);

        auto* mtlTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::metal::Texture>(
            attachments[depthAttachmentIdx]);

        const auto& textureInfo = mtlTexture->getCreateInfo();
        const auto& attachmentInfo = renderPassInfo.attachments[depthAttachmentIdx];

        framebuffer.depthAttachment.texture = mtlTexture->getTexture();
        framebuffer.depthAttachment.level = attachmentInfo.mipLevel;
        framebuffer.depthAttachment.slice =
            getTextureSlice(textureInfo.textureType, attachmentInfo.layer, textureInfo.size.depth);
        framebuffer.depthAttachment.depthPlane =
            getTextureDepthPlane(textureInfo.textureType, attachmentInfo.layer, textureInfo.size.depth);
        framebuffer.depthAttachment.loadAction = convertToMtlLoadAction(attachmentInfo.loadOp);
        framebuffer.depthAttachment.storeAction = convertToMtlStoreAction(attachmentInfo.storeOp);

        if (attachmentInfo.loadOp == snap::rhi::AttachmentLoadOp::Clear) {
            assert(depthAttachmentIdx < clearValues.size());
            framebuffer.depthAttachment.clearDepth = clearValues[depthAttachmentIdx].depthStencil.depth;
        }
        if (hasStencilAspect(textureInfo.format)) {
            framebuffer.stencilAttachment.texture = framebuffer.depthAttachment.texture;
            framebuffer.stencilAttachment.level = framebuffer.depthAttachment.level;
            framebuffer.stencilAttachment.slice = framebuffer.depthAttachment.slice;
            framebuffer.stencilAttachment.depthPlane = framebuffer.depthAttachment.depthPlane;
            framebuffer.stencilAttachment.loadAction = convertToMtlLoadAction(attachmentInfo.stencilLoadOp);
            framebuffer.stencilAttachment.storeAction = convertToMtlStoreAction(attachmentInfo.stencilStoreOp);

            if (attachmentInfo.stencilLoadOp == snap::rhi::AttachmentLoadOp::Clear) {
                assert(depthAttachmentIdx < clearValues.size());
                framebuffer.stencilAttachment.clearStencil = clearValues[depthAttachmentIdx].depthStencil.stencil;
            }
        }

        const uint32_t resolveDepthAttachmentIdx = subpassDescription.resolveDepthStencilAttachment.attachment;
        if (resolveDepthAttachmentIdx != snap::rhi::AttachmentUnused) {
            assert(resolveDepthAttachmentIdx < renderPassInfo.attachmentCount);

            auto* mtlResolveTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::metal::Texture>(
                attachments[resolveDepthAttachmentIdx]);

            const auto& resolveTextureInfo = mtlResolveTexture->getCreateInfo();
            const auto& resolveAttachmentInfo = renderPassInfo.attachments[resolveDepthAttachmentIdx];

            framebuffer.depthAttachment.resolveTexture = mtlResolveTexture->getTexture();
            framebuffer.depthAttachment.resolveLevel = resolveAttachmentInfo.mipLevel;
            framebuffer.depthAttachment.resolveSlice = getTextureSlice(
                resolveTextureInfo.textureType, resolveAttachmentInfo.layer, resolveTextureInfo.size.depth);
            framebuffer.depthAttachment.resolveDepthPlane = getTextureDepthPlane(
                resolveTextureInfo.textureType, resolveAttachmentInfo.layer, resolveTextureInfo.size.depth);

            if (hasStencilAspect(resolveTextureInfo.format)) {
                framebuffer.stencilAttachment.resolveTexture = framebuffer.depthAttachment.resolveTexture;
                framebuffer.stencilAttachment.resolveLevel = framebuffer.depthAttachment.resolveLevel;
                framebuffer.stencilAttachment.resolveSlice = framebuffer.depthAttachment.resolveSlice;
                framebuffer.stencilAttachment.resolveDepthPlane = framebuffer.depthAttachment.resolveDepthPlane;

                if (resolveAttachmentInfo.stencilStoreOp == snap::rhi::AttachmentStoreOp::DontCare) {
                    snap::rhi::common::throwException(
                        "[Framebuffer] invalid resolveAttachment stencilStoreOp, should be AttachmentStoreOp::Store");
                }

                if (attachmentInfo.stencilStoreOp == snap::rhi::AttachmentStoreOp::DontCare) {
                    framebuffer.stencilAttachment.storeAction = MTLStoreActionMultisampleResolve;
                } else if (attachmentInfo.stencilStoreOp == snap::rhi::AttachmentStoreOp::Store) {
                    if (@available(iOS 10.0, macOS 10.12, *)) {
                        framebuffer.stencilAttachment.storeAction = MTLStoreActionStoreAndMultisampleResolve;
                    } else {
                        snap::rhi::common::throwException("[Framebuffer] unsupported storeOp");
                    }
                }
            }

            if (resolveAttachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::DontCare) {
                snap::rhi::common::throwException(
                    "[Framebuffer] invalid resolveAttachment storeOp, should be AttachmentStoreOp::Store");
            }

            if (attachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::DontCare) {
                framebuffer.depthAttachment.storeAction = MTLStoreActionMultisampleResolve;
            } else if (attachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::Store) {
                if (@available(iOS 10.0, macOS 10.12, *)) {
                    framebuffer.depthAttachment.storeAction = MTLStoreActionStoreAndMultisampleResolve;
                } else {
                    snap::rhi::common::throwException("[Framebuffer] unsupported storeOp");
                }
            }
        }
    }

    return framebuffer;
}

MTLRenderPassDescriptor* createFramebuffer(const snap::rhi::RenderingInfo& renderingInfo) {
    MTLRenderPassDescriptor* framebuffer = [MTLRenderPassDescriptor new];

    // https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/2879276-rendertargetwidth
    // https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/2879279-rendertargetheight
    // https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/1437975-rendertargetarraylength
    for (uint32_t i = 0; i < renderingInfo.colorAttachments.size(); ++i) {
        const auto& attachmentInfo = renderingInfo.colorAttachments[i];
        auto* mtlTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::metal::Texture>(
            attachmentInfo.attachment.texture);
        const auto& textureInfo = mtlTexture->getCreateInfo();

        framebuffer.colorAttachments[i].storeAction = convertToMtlStoreAction(attachmentInfo.storeOp);
        framebuffer.colorAttachments[i].loadAction = convertToMtlLoadAction(attachmentInfo.loadOp);

        if (attachmentInfo.loadOp == snap::rhi::AttachmentLoadOp::Clear) {
            const auto& clearValue = attachmentInfo.clearValue.color;
            if (isIntFormat(textureInfo.format)) {
                framebuffer.colorAttachments[i].clearColor = MTLClearColorMake(
                    clearValue.int32[0], clearValue.int32[1], clearValue.int32[2], clearValue.int32[3]);
            } else if (isUintFormat(textureInfo.format)) {
                framebuffer.colorAttachments[i].clearColor = MTLClearColorMake(
                    clearValue.uint32[0], clearValue.uint32[1], clearValue.uint32[2], clearValue.uint32[3]);
            } else {
                framebuffer.colorAttachments[i].clearColor = MTLClearColorMake(
                    clearValue.float32[0], clearValue.float32[1], clearValue.float32[2], clearValue.float32[3]);
            }
        }
        framebuffer.colorAttachments[i].texture = mtlTexture->getTexture();
        framebuffer.colorAttachments[i].level = attachmentInfo.attachment.mipLevel;
        framebuffer.colorAttachments[i].slice =
            getTextureSlice(textureInfo.textureType, attachmentInfo.attachment.layer, textureInfo.size.depth);
        framebuffer.colorAttachments[i].depthPlane =
            getTextureDepthPlane(textureInfo.textureType, attachmentInfo.attachment.layer, textureInfo.size.depth);

        if (attachmentInfo.resolveAttachment.texture) {
            const auto& resolveAttachmentInfo = attachmentInfo.resolveAttachment;
            auto* mtlResolveTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::metal::Texture>(
                resolveAttachmentInfo.texture);
            const auto& resolveTextureInfo = mtlResolveTexture->getCreateInfo();

            framebuffer.colorAttachments[i].resolveTexture = mtlResolveTexture->getTexture();
            framebuffer.colorAttachments[i].resolveLevel = resolveAttachmentInfo.mipLevel;
            framebuffer.colorAttachments[i].resolveSlice = getTextureSlice(
                resolveTextureInfo.textureType, resolveAttachmentInfo.layer, resolveTextureInfo.size.depth);
            framebuffer.colorAttachments[i].resolveDepthPlane = getTextureDepthPlane(
                resolveTextureInfo.textureType, resolveAttachmentInfo.layer, resolveTextureInfo.size.depth);

            if (attachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::DontCare) {
                framebuffer.colorAttachments[i].storeAction = MTLStoreActionMultisampleResolve;
            } else if (attachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::Store) {
                if (@available(iOS 10.0, macOS 10.12, *)) {
                    framebuffer.colorAttachments[i].storeAction = MTLStoreActionStoreAndMultisampleResolve;
                } else {
                    snap::rhi::common::throwException("[Framebuffer] unsupported storeOp");
                }
            }
        }
    }

    if (renderingInfo.depthAttachment.attachment.texture) {
        auto* mtlTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::metal::Texture>(
            renderingInfo.depthAttachment.attachment.texture);

        const auto& textureInfo = mtlTexture->getCreateInfo();
        const auto& attachmentInfo = renderingInfo.depthAttachment;

        framebuffer.depthAttachment.texture = mtlTexture->getTexture();
        framebuffer.depthAttachment.level = attachmentInfo.attachment.mipLevel;
        framebuffer.depthAttachment.slice =
            getTextureSlice(textureInfo.textureType, attachmentInfo.attachment.layer, textureInfo.size.depth);
        framebuffer.depthAttachment.depthPlane =
            getTextureDepthPlane(textureInfo.textureType, attachmentInfo.attachment.layer, textureInfo.size.depth);
        framebuffer.depthAttachment.loadAction = convertToMtlLoadAction(attachmentInfo.loadOp);
        framebuffer.depthAttachment.storeAction = convertToMtlStoreAction(attachmentInfo.storeOp);

        if (attachmentInfo.loadOp == snap::rhi::AttachmentLoadOp::Clear) {
            framebuffer.depthAttachment.clearDepth = attachmentInfo.clearValue.depthStencil.depth;
        }

        if (renderingInfo.depthAttachment.resolveAttachment.texture) {
            const auto& resolveAttachmentInfo = renderingInfo.depthAttachment.resolveAttachment;
            auto* mtlResolveTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::metal::Texture>(
                resolveAttachmentInfo.texture);
            const auto& resolveTextureInfo = mtlResolveTexture->getCreateInfo();

            framebuffer.depthAttachment.resolveTexture = mtlResolveTexture->getTexture();
            framebuffer.depthAttachment.resolveLevel = resolveAttachmentInfo.mipLevel;
            framebuffer.depthAttachment.resolveSlice = getTextureSlice(
                resolveTextureInfo.textureType, resolveAttachmentInfo.layer, resolveTextureInfo.size.depth);
            framebuffer.depthAttachment.resolveDepthPlane = getTextureDepthPlane(
                resolveTextureInfo.textureType, resolveAttachmentInfo.layer, resolveTextureInfo.size.depth);

            if (attachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::DontCare) {
                framebuffer.depthAttachment.storeAction = MTLStoreActionMultisampleResolve;
            } else if (attachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::Store) {
                if (@available(iOS 10.0, macOS 10.12, *)) {
                    framebuffer.depthAttachment.storeAction = MTLStoreActionStoreAndMultisampleResolve;
                } else {
                    snap::rhi::common::throwException("[Framebuffer] unsupported storeOp");
                }
            }
        }
    }

    if (renderingInfo.stencilAttachment.attachment.texture) {
        auto* mtlTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::metal::Texture>(
            renderingInfo.stencilAttachment.attachment.texture);

        const auto& textureInfo = mtlTexture->getCreateInfo();
        const auto& attachmentInfo = renderingInfo.stencilAttachment;

        framebuffer.stencilAttachment.texture = mtlTexture->getTexture();
        framebuffer.stencilAttachment.level = attachmentInfo.attachment.mipLevel;
        framebuffer.stencilAttachment.slice =
            getTextureSlice(textureInfo.textureType, attachmentInfo.attachment.layer, textureInfo.size.depth);
        framebuffer.stencilAttachment.depthPlane =
            getTextureDepthPlane(textureInfo.textureType, attachmentInfo.attachment.layer, textureInfo.size.depth);
        framebuffer.stencilAttachment.loadAction = convertToMtlLoadAction(attachmentInfo.loadOp);
        framebuffer.stencilAttachment.storeAction = convertToMtlStoreAction(attachmentInfo.storeOp);

        if (attachmentInfo.loadOp == snap::rhi::AttachmentLoadOp::Clear) {
            framebuffer.stencilAttachment.clearStencil = attachmentInfo.clearValue.depthStencil.stencil;
        }

        if (renderingInfo.stencilAttachment.resolveAttachment.texture) {
            const auto& resolveAttachmentInfo = renderingInfo.stencilAttachment.resolveAttachment;
            auto* mtlResolveTexture = snap::rhi::backend::common::smart_cast<const snap::rhi::backend::metal::Texture>(
                resolveAttachmentInfo.texture);
            const auto& resolveTextureInfo = mtlResolveTexture->getCreateInfo();

            framebuffer.stencilAttachment.resolveTexture = mtlResolveTexture->getTexture();
            framebuffer.stencilAttachment.resolveLevel = resolveAttachmentInfo.mipLevel;
            framebuffer.stencilAttachment.resolveSlice = getTextureSlice(
                resolveTextureInfo.textureType, resolveAttachmentInfo.layer, resolveTextureInfo.size.depth);
            framebuffer.stencilAttachment.resolveDepthPlane = getTextureDepthPlane(
                resolveTextureInfo.textureType, resolveAttachmentInfo.layer, resolveTextureInfo.size.depth);

            if (attachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::DontCare) {
                framebuffer.stencilAttachment.storeAction = MTLStoreActionMultisampleResolve;
            } else if (attachmentInfo.storeOp == snap::rhi::AttachmentStoreOp::Store) {
                if (@available(iOS 10.0, macOS 10.12, *)) {
                    framebuffer.stencilAttachment.storeAction = MTLStoreActionStoreAndMultisampleResolve;
                } else {
                    snap::rhi::common::throwException("[Framebuffer] unsupported storeOp");
                }
            }
        }
    }

    return framebuffer;
}

MTLResourceUsage convertToResourceUsage(snap::rhi::ShaderStageBits stageBits,
                                        snap::rhi::DescriptorType descriptorType) {
    MTLResourceUsage usage = MTLResourceUsageRead;
    if (descriptorType == snap::rhi::DescriptorType::StorageBuffer ||
        descriptorType == snap::rhi::DescriptorType::StorageTexture) {
        usage |= MTLResourceUsageWrite;
    }

    return usage;
}

MTLRenderStages convertToRenderStages(snap::rhi::ShaderStageBits stageBits) {
    MTLRenderStages stages = 0;

    if ((stageBits & snap::rhi::ShaderStageBits::VertexShaderBit) == snap::rhi::ShaderStageBits::VertexShaderBit) {
        stages |= MTLRenderStageVertex;
    }

    if ((stageBits & snap::rhi::ShaderStageBits::FragmentShaderBit) == snap::rhi::ShaderStageBits::FragmentShaderBit) {
        stages |= MTLRenderStageFragment;
    }

    return stages;
}

MTLArgumentAccess getArgumentAccess(const snap::rhi::DescriptorType descriptorType) {
    switch (descriptorType) {
        case snap::rhi::DescriptorType::UniformBuffer:
        case snap::rhi::DescriptorType::SampledTexture:
        case snap::rhi::DescriptorType::Sampler:
            return MTLArgumentAccessReadOnly;

        case snap::rhi::DescriptorType::StorageTexture:
        case snap::rhi::DescriptorType::StorageBuffer:
            return MTLArgumentAccessReadWrite;

        default: {
            assert(false);
            return MTLArgumentAccessReadOnly;
        }
    }
}

id<MTLArgumentEncoder> createArgumentEncoder(Device* device, const snap::rhi::DescriptorSetLayoutCreateInfo& info) {
    NSMutableArray<MTLArgumentDescriptor*>* descriptors = [[NSMutableArray<MTLArgumentDescriptor*> alloc] init];
    for (const auto& descInfo : info.bindings) {
        MTLArgumentDescriptor* descriptor = [MTLArgumentDescriptor new];
        [descriptor setDataType:getDataType(descInfo.descriptorType)];
        [descriptor setIndex:descInfo.binding];
        [descriptor setAccess:getArgumentAccess(descInfo.descriptorType)];

        [descriptors addObject:descriptor];
    }

    return [device->getMtlDevice() newArgumentEncoderWithArguments:descriptors];
}

MTLTextureType toMtlTextureType(const snap::rhi::TextureType textureType, const uint32_t sampleCount) {
    switch (textureType) {
        case snap::rhi::TextureType::Texture2DArray:
            // iOS doesn't support multisample texture array
            assert(sampleCount == 1);
            return MTLTextureType2DArray;

        case snap::rhi::TextureType::Texture2D:
            return sampleCount == 1 ? MTLTextureType2D : MTLTextureType2DMultisample;

        case snap::rhi::TextureType::Texture3D:
            assert(sampleCount == 1);
            return MTLTextureType3D;

        case snap::rhi::TextureType::TextureCubemap:
            assert(sampleCount == 1);
            return MTLTextureTypeCube;

        default:
            snap::rhi::common::throwException("[Metal::TextureViewCache] invalid TextureType");
    }
}

MTLTextureSwizzle toMtlSwizzle(const snap::rhi::ComponentSwizzle s) {
    switch (s) {
        case snap::rhi::ComponentSwizzle::Zero:
            return MTLTextureSwizzleZero;
        case snap::rhi::ComponentSwizzle::One:
            return MTLTextureSwizzleOne;
        case snap::rhi::ComponentSwizzle::R:
            return MTLTextureSwizzleRed;
        case snap::rhi::ComponentSwizzle::G:
            return MTLTextureSwizzleGreen;
        case snap::rhi::ComponentSwizzle::B:
            return MTLTextureSwizzleBlue;
        case snap::rhi::ComponentSwizzle::A:
            return MTLTextureSwizzleAlpha;
        default:
            return MTLTextureSwizzleRed;
    }
}
} // namespace snap::rhi::backend::metal
