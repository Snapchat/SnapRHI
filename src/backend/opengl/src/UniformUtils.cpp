#include "snap/rhi/backend/opengl/UniformUtils.hpp"
#include "snap/rhi/Enums.h"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/Pipeline.hpp"
#include "snap/rhi/backend/opengl/Utils.hpp"
#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
constexpr std::string_view LegacyUBOName{""};
constexpr uint32_t LegacyBindingOffset = 32;

constexpr uint32_t LegacyUBOsBindingBase = 0;
constexpr uint32_t LegacySSBOsBindingBase = LegacyUBOsBindingBase + LegacyBindingOffset;
constexpr uint32_t LegacyTexturesBindingBase = LegacySSBOsBindingBase + LegacyBindingOffset;
constexpr uint32_t LegacySamplersBindingBase = LegacyTexturesBindingBase + LegacyBindingOffset;
constexpr uint32_t LegacyImagesBindingBase = LegacySamplersBindingBase + LegacyBindingOffset;

snap::rhi::backend::opengl::TextureTarget getTextureTarget(const GLenum type) {
    switch (type) {
        case GL_SAMPLER_2D_ARRAY:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_2D_ARRAY_SHADOW:
            return snap::rhi::backend::opengl::TextureTarget::Texture2DArray;

        case GL_SAMPLER_3D:
        case GL_INT_SAMPLER_3D:
        case GL_UNSIGNED_INT_SAMPLER_3D:
            return snap::rhi::backend::opengl::TextureTarget::Texture3D;

        case GL_SAMPLER_EXTERNAL_OES:
            return snap::rhi::backend::opengl::TextureTarget::TextureExternal;

        case GL_SAMPLER_CUBE:
        case GL_INT_SAMPLER_CUBE:
        case GL_SAMPLER_CUBE_SHADOW:
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
            return snap::rhi::backend::opengl::TextureTarget::TextureCubeMap;

        case GL_SAMPLER_2D_RECT:
            return snap::rhi::backend::opengl::TextureTarget::TextureRectangle;

        case GL_SAMPLER_2D_SHADOW:
        case GL_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_SAMPLER_2D:
            return snap::rhi::backend::opengl::TextureTarget::Texture2D;

        default:
            return snap::rhi::backend::opengl::TextureTarget::Detached;
    }
}

void prepareDefaultDS(std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection) {
    if (physicalReflection.empty()) {
        snap::rhi::reflection::DescriptorSetInfo dstDSInfo{};
        dstDSInfo.binding = snap::rhi::backend::opengl::DefaultDescriptorSetID;
        physicalReflection.push_back(dstDSInfo);
    } else {
        assert(physicalReflection[0].binding == snap::rhi::backend::opengl::DefaultDescriptorSetID);
    }
}

snap::rhi::reflection::UniformFormat convertToUniformType(const GLenum type) {
    switch (type) {
        // Use int for bool
        case GL_BOOL:
            return snap::rhi::reflection::UniformFormat::Bool;
        case GL_BOOL_VEC2:
            return snap::rhi::reflection::UniformFormat::Bool2;
        case GL_BOOL_VEC3:
            return snap::rhi::reflection::UniformFormat::Bool3;
        case GL_BOOL_VEC4:
            return snap::rhi::reflection::UniformFormat::Bool4;

        case GL_FLOAT:
            return snap::rhi::reflection::UniformFormat::Float;
        case GL_FLOAT_VEC2:
            return snap::rhi::reflection::UniformFormat::Float2;
        case GL_FLOAT_VEC3:
            return snap::rhi::reflection::UniformFormat::Float3;
        case GL_FLOAT_VEC4:
            return snap::rhi::reflection::UniformFormat::Float4;

        case GL_INT:
            return snap::rhi::reflection::UniformFormat::Int;
        case GL_INT_VEC2:
            return snap::rhi::reflection::UniformFormat::Int2;
        case GL_INT_VEC3:
            return snap::rhi::reflection::UniformFormat::Int3;
        case GL_INT_VEC4:
            return snap::rhi::reflection::UniformFormat::Int4;

        case GL_UNSIGNED_INT:
            return snap::rhi::reflection::UniformFormat::UInt;
        case GL_UNSIGNED_INT_VEC2:
            return snap::rhi::reflection::UniformFormat::UInt2;
        case GL_UNSIGNED_INT_VEC3:
            return snap::rhi::reflection::UniformFormat::UInt3;
        case GL_UNSIGNED_INT_VEC4:
            return snap::rhi::reflection::UniformFormat::UInt4;

        case GL_FLOAT_MAT2:
            return snap::rhi::reflection::UniformFormat::Float2x2;
        case GL_FLOAT_MAT3:
            return snap::rhi::reflection::UniformFormat::Float3x3;
        case GL_FLOAT_MAT4:
            return snap::rhi::reflection::UniformFormat::Float4x4;

        default:
            SNAP_RHI_LOGE("[convertToUniformType] invalid type: %d", static_cast<int>(type));
            return snap::rhi::reflection::UniformFormat::Undefined;
    }
}

[[maybe_unused]] GLenum toGLAccessFormat(const snap::rhi::reflection::ImageAccess imageAccess) {
    switch (imageAccess) {
        case snap::rhi::reflection::ImageAccess::ReadOnly:
            return GL_READ_ONLY;

        case snap::rhi::reflection::ImageAccess::WriteOnly:
            return GL_WRITE_ONLY;

        case snap::rhi::reflection::ImageAccess::ReadWrite:
            return GL_READ_WRITE;

        default:
            return GL_NONE;
    }
}

snap::rhi::reflection::TextureType convertToTextureType(const GLenum format) {
    switch (format) {
        case GL_SAMPLER_2D_ARRAY:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_2D_ARRAY_SHADOW:
            return snap::rhi::reflection::TextureType::Texture2DArray;

        case GL_UNSIGNED_INT_SAMPLER_CUBE:
        case GL_INT_SAMPLER_CUBE:
        case GL_SAMPLER_CUBE_SHADOW:
        case GL_SAMPLER_CUBE:
            return snap::rhi::reflection::TextureType::TextureCube;

        case GL_UNSIGNED_INT_SAMPLER_3D:
        case GL_INT_SAMPLER_3D:
        case GL_SAMPLER_3D:
            return snap::rhi::reflection::TextureType::Texture3D;

        case GL_SAMPLER_EXTERNAL_OES:
            return snap::rhi::reflection::TextureType::TextureExternal;

        case GL_SAMPLER_BUFFER:
        case GL_INT_SAMPLER_BUFFER:
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            return snap::rhi::reflection::TextureType::TextureBuffer;

        case GL_SAMPLER_2D:
        case GL_SAMPLER_2D_SHADOW:
        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_INT_SAMPLER_2D:
        case GL_SAMPLER_2D_RECT:
            return snap::rhi::reflection::TextureType::Texture2D;

        default: {
            return snap::rhi::reflection::TextureType::Undefined;
        }
    }
}

[[maybe_unused]] GLenum convertToGLTextureType(const snap::rhi::reflection::TextureType format) {
    switch (format) {
        case snap::rhi::reflection::TextureType::Texture2DArray:
            return GL_SAMPLER_2D_ARRAY;

        case snap::rhi::reflection::TextureType::TextureCube:
            return GL_SAMPLER_CUBE;

        case snap::rhi::reflection::TextureType::Texture3D:
            return GL_SAMPLER_3D;

        case snap::rhi::reflection::TextureType::TextureExternal:
            return GL_SAMPLER_EXTERNAL_OES;

        case snap::rhi::reflection::TextureType::TextureBuffer:
            return GL_SAMPLER_BUFFER;

        case snap::rhi::reflection::TextureType::Texture2D:
            return GL_SAMPLER_2D;

        default: {
            assert(false);
            return GL_NONE;
        }
    }
}

void buildPipelineNativeUniformsInfo(const snap::rhi::backend::opengl::Profile& gl,
                                     const GLuint program,
                                     snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                     GLint uniformMaxLength,
                                     std::vector<char>& uniformNameBuffer) {
    GLint activeUniformCount = 0;
    gl.getProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniformCount);

    for (GLint i = 0; i < activeUniformCount; ++i) {
        snap::rhi::backend::opengl::PipelineNativeInfo::UniformInfo uniformInfo{};
        uniformInfo.index = static_cast<GLuint>(i);

        GLsizei receivedBuffSize = 0;
        gl.getActiveUniform(program,
                            uniformInfo.index,
                            uniformMaxLength,
                            &receivedBuffSize,
                            &uniformInfo.arraySize,
                            &uniformInfo.type,
                            uniformNameBuffer.data());
        std::string uniformName{uniformNameBuffer.data()};
        uniformInfo.location = gl.getUniformLocation(program, uniformName.data());

        if (snap::rhi::backend::opengl::isSystemResource(uniformName)) {
            continue;
        }

        if (uniformInfo.arraySize > 1 || snap::rhi::backend::common::hasEnding(uniformName, "[0]")) {
            // array case
        } else {
            uniformInfo.arraySize = 0;
        }

        uniformInfo.name = snap::rhi::backend::common::getNonArrayName(uniformName);
        pipelineNativeInfo.uniformsInfo.push_back(uniformInfo);
    }
}

void buildPipelineNativeSSBOsInfo(const snap::rhi::backend::opengl::Profile& gl,
                                  const GLuint program,
                                  snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                  GLint uniformMaxLength,
                                  std::vector<char>& uniformNameBuffer) {
    if (!gl.getFeatures().isSSBOSupported) {
        return;
    }

    GLint uniformBlockMaxLength = 0;
    gl.getProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, &uniformBlockMaxLength);
    std::vector<char> uniformBlockNameBuffer(uniformBlockMaxLength + 1, 0);

    GLint count = 0;
    gl.getProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &count);

    for (GLint i = 0; i < count; ++i) {
        snap::rhi::backend::opengl::PipelineNativeInfo::SSBOInfo info{};
        info.index = i; // index = gl.getProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK,
                        // uniformBlockNameBuffer.data());

        gl.getProgramResourceName(program,
                                  GL_SHADER_STORAGE_BLOCK,
                                  info.index,
                                  static_cast<GLsizei>(uniformBlockNameBuffer.size()),
                                  &uniformBlockMaxLength,
                                  uniformBlockNameBuffer.data());
        info.name = std::string{uniformBlockNameBuffer.data()};

        if (snap::rhi::backend::opengl::isSystemResource(info.name)) {
            continue;
        }

        const GLenum propsBinding[] = {GL_BUFFER_BINDING};
        gl.getProgramResourceiv(
            program, GL_SHADER_STORAGE_BLOCK, info.index, 1, propsBinding, 1, nullptr, &info.binding);

        const GLenum propsSize[] = {GL_BUFFER_DATA_SIZE};
        gl.getProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, info.index, 1, propsSize, 1, nullptr, &info.size);

        pipelineNativeInfo.ssbosInfo.emplace_back(info);
    }

    std::sort(pipelineNativeInfo.ssbosInfo.begin(),
              pipelineNativeInfo.ssbosInfo.end(),
              [](const auto& l, const auto& r) { return l.name < r.name; });
}

void buildPipelineNativeUBOsInfo(const snap::rhi::backend::opengl::Profile& gl,
                                 const GLuint program,
                                 snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                 GLint uniformMaxLength,
                                 std::vector<char>& uniformNameBuffer) {
    if (!gl.getFeatures().isNativeUBOSupported) {
        return;
    }

    GLint uniformBlockMaxLength = 0;
    gl.getProgramiv(program, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &uniformBlockMaxLength);

    uniformMaxLength = std::max(uniformMaxLength, uniformBlockMaxLength);

    GLint activeUniformBlockCount = 0;
    gl.getProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &activeUniformBlockCount);

    ++uniformMaxLength; // null terminated string
    std::vector<char> uniformBlockNameBuffer(uniformMaxLength, 0);

    for (GLint i = 0; i < activeUniformBlockCount; ++i) {
        GLsizei receivedBuffSize = 0;
        gl.getActiveUniformBlockName(
            program, static_cast<GLuint>(i), uniformMaxLength, &receivedBuffSize, uniformBlockNameBuffer.data());
        std::string uniformName{uniformBlockNameBuffer.data()};

        if (snap::rhi::backend::opengl::isSystemResource(uniformName)) {
            continue;
        }

        const GLuint blockIndex = i;
        //        GLuint blockIndex = gl.getUniformBlockIndex(program, uniformBlockNameBuffer.data());
        //        assert(blockIndex != GL_INVALID_INDEX)

        GLint blockSize = 0;
        gl.getActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);

        snap::rhi::reflection::UniformBufferInfo resultInfo{};
        resultInfo.size = static_cast<uint32_t>(blockSize);
        resultInfo.name = uniformName;
        resultInfo.binding = static_cast<uint32_t>(blockIndex);

        /**
         * Build UBO members info
         */
        {
            GLint numUniformsInBlock;
            gl.getActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUniformsInBlock);

            std::vector<GLint> uniformIndices(numUniformsInBlock);
            gl.getActiveUniformBlockiv(
                program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, uniformIndices.data());

            std::vector<GLint> uniformOffsets(uniformIndices.size());
            gl.getActiveUniformsiv(program,
                                   static_cast<GLsizei>(uniformIndices.size()),
                                   reinterpret_cast<GLuint*>(uniformIndices.data()),
                                   GL_UNIFORM_OFFSET,
                                   uniformOffsets.data());
            std::vector<GLint> arrayStrides(uniformIndices.size());
            gl.getActiveUniformsiv(program,
                                   static_cast<GLsizei>(uniformIndices.size()),
                                   reinterpret_cast<GLuint*>(uniformIndices.data()),
                                   GL_UNIFORM_ARRAY_STRIDE,
                                   arrayStrides.data());

            for (size_t i = 0; i < uniformIndices.size(); ++i) {
                GLint uniformIndex = uniformIndices[i];

                if (uniformIndex < 0) {
                    continue;
                }

                /**
                 * The size of the uniform variable will be returned in size. Uniform variables other than arrays
                 * will have a size of 1. Structures and arrays of structures will be reduced as described earlier,
                 * such that each of the names returned will be a data type in the earlier list. If this reduction
                 * results in an array, the size returned will be as described for uniform arrays; otherwise, the
                 * size returned will be 1.
                 *
                 * https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glGetActiveUniform.xhtml
                 */
                const auto& uniformInfo = pipelineNativeInfo.uniformsInfo[uniformIndex];

                GLint uniformSize = uniformInfo.arraySize;
                GLenum uniformType = uniformInfo.type;
                std::string uboUniformName = uniformInfo.name;

                if (uboUniformName.size() > uniformName.size() &&
                    uboUniformName.substr(0, uniformName.size()) == uniformName) {
                    uboUniformName =
                        uboUniformName.substr(uniformName.size() + 1); // + 1 since "StructName + '.'" has to be removed
                }

                GLint uniformOffset = uniformOffsets[i];

                snap::rhi::reflection::BufferMemberInfo resultMemberInfo{};
                resultMemberInfo.offset = static_cast<uint32_t>(uniformOffset);
                resultMemberInfo.format = convertToUniformType(uniformType);

                if (resultMemberInfo.format == snap::rhi::reflection::UniformFormat::Undefined) {
                    /**
                     * android-x64 build has issue with UBO reflection
                     * driver may report wrong unifroms like => GL_SAMPLER_2D
                     */
                    continue;
                }

                SNAP_RHI_LOGI("  Uniform: %s", uniformBlockNameBuffer.data());
                SNAP_RHI_LOGI("    Type: 0x%x", uniformType);
                SNAP_RHI_LOGI("    Size: %d (Array elements: %d)", uniformSize, uniformSize > 1 ? uniformSize : 1);
                SNAP_RHI_LOGI("    Offset: %d bytes", uniformOffset);

                if (uniformSize > 0) {
                    GLint arrayStride = arrayStrides[i];
                    assert(arrayStride > 0);

                    resultMemberInfo.name = uboUniformName;
                    resultMemberInfo.arraySize = static_cast<uint32_t>(uniformSize);
                    resultMemberInfo.stride = static_cast<uint32_t>(arrayStride);
                    resultInfo.membersInfo.push_back(resultMemberInfo);

                    for (GLint i = 0; i < uniformSize; ++i) {
                        resultMemberInfo.name = uboUniformName + "[" + std::to_string(i) + "]";
                        resultMemberInfo.arraySize = 0;
                        resultMemberInfo.offset = static_cast<uint32_t>(uniformOffset + i * arrayStride);
                        resultInfo.membersInfo.push_back(resultMemberInfo);

                        SNAP_RHI_LOGI("      Index %d Offset: %d bytes", i, uniformOffset + i * arrayStride);
                    }
                } else {
                    resultMemberInfo.name = uboUniformName;
                    resultMemberInfo.arraySize = 0;
                    resultMemberInfo.stride = snap::rhi::backend::opengl::getAlignedUniformSize(uniformType);

                    resultInfo.membersInfo.push_back(resultMemberInfo);
                }
            }
        }

        pipelineNativeInfo.nativeUBOsInfo.emplace_back(std::move(resultInfo));
    }

    std::sort(pipelineNativeInfo.nativeUBOsInfo.begin(),
              pipelineNativeInfo.nativeUBOsInfo.end(),
              [](const auto& l, const auto& r) { return l.name < r.name; });
    {
        /**
         * Before these code nativeUBOInfo[i].binding store UBO location,
         * After these code nativeUBOInfo[i].binding will store UBO binding.
         */
        for (size_t i = 0; i < pipelineNativeInfo.nativeUBOsInfo.size(); ++i) {
            gl.uniformBlockBinding(
                program, static_cast<GLint>(pipelineNativeInfo.nativeUBOsInfo[i].binding), static_cast<GLuint>(i));
            pipelineNativeInfo.nativeUBOsInfo[i].binding = static_cast<uint32_t>(i);
        }
    }
}

void buildPipelineNativeCompatibleUBOsInfo(const snap::rhi::backend::opengl::Profile& gl,
                                           const GLuint program,
                                           snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                           GLint uniformMaxLength,
                                           std::vector<char>& uniformNameBuffer) {
    for (const auto& uniformInfo : pipelineNativeInfo.uniformsInfo) {
        switch (uniformInfo.type) {
            case GL_FLOAT_VEC4:
            case GL_INT_VEC4: {
                assert(uniformInfo.arraySize > 0);

                snap::rhi::backend::opengl::PipelineNativeInfo::CompatibleUBOInfo info{};
                info.index = uniformInfo.index;
                info.name = uniformInfo.name;
                info.location = uniformInfo.location;
                info.type = uniformInfo.type;
                info.arraySize = uniformInfo.arraySize;

                pipelineNativeInfo.compatibleUBOsInfo.emplace_back(info);
            } break;
        }
    }
    std::sort(pipelineNativeInfo.compatibleUBOsInfo.begin(),
              pipelineNativeInfo.compatibleUBOsInfo.end(),
              [](const auto& l, const auto& r) { return l.name < r.name; });

    for (size_t i = 0; i < pipelineNativeInfo.compatibleUBOsInfo.size(); ++i) {
        pipelineNativeInfo.compatibleUBOsInfo[i].binding = static_cast<uint32_t>(i);
    }
}

void buildPipelineNativeLegacyUBOsInfo(snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo) {
    std::vector<snap::rhi::backend::opengl::PipelineNativeInfo::UniformInfo> uniformsInfo{};

    // result.uniforms should be sorted based on "offset" order
    for (const auto& uniformInfo : pipelineNativeInfo.uniformsInfo) {
        switch (uniformInfo.type) {
            case GL_BOOL:
            case GL_BOOL_VEC2:
            case GL_BOOL_VEC3:
            case GL_BOOL_VEC4:

            case GL_FLOAT:
            case GL_FLOAT_VEC2:
            case GL_FLOAT_VEC3:
            case GL_FLOAT_VEC4:

            case GL_INT:
            case GL_INT_VEC2:
            case GL_INT_VEC3:
            case GL_INT_VEC4:

            case GL_UNSIGNED_INT:
            case GL_UNSIGNED_INT_VEC2:
            case GL_UNSIGNED_INT_VEC3:
            case GL_UNSIGNED_INT_VEC4:

            case GL_FLOAT_MAT2:
            case GL_FLOAT_MAT3:
            case GL_FLOAT_MAT4: {
                uniformsInfo.push_back(uniformInfo);
            } break;
        }
    }
    std::sort(uniformsInfo.begin(), uniformsInfo.end(), [](const auto& l, const auto& r) { return l.name < r.name; });

    if (!uniformsInfo.empty()) {
        uint32_t uboOffset = 0;
        // uniforms should be sorted based on "offset" order
        pipelineNativeInfo.legacyUBOInfo.uniforms.resize(uniformsInfo.size());
        for (size_t i = 0; i < uniformsInfo.size(); ++i) {
            const auto& uniformInfo = uniformsInfo[i];

            snap::rhi::reflection::BufferMemberInfo memberInfo{};
            memberInfo.name = uniformInfo.name;
            memberInfo.offset = uboOffset;
            memberInfo.format = convertToUniformType(uniformInfo.type);
            memberInfo.arraySize = uniformInfo.arraySize;
            memberInfo.stride = snap::rhi::backend::opengl::getAlignedUniformSize(uniformInfo.type);

            pipelineNativeInfo.legacyUBOInfo.uboInfo.membersInfo.push_back(memberInfo);
            if (memberInfo.arraySize > 0) {
                memberInfo.arraySize = 0;
                for (uint32_t idx = 0; idx < uniformInfo.arraySize; ++idx) {
                    memberInfo.name = uniformInfo.name + "[" + std::to_string(idx) + "]";
                    memberInfo.offset = uboOffset + idx * memberInfo.stride;

                    pipelineNativeInfo.legacyUBOInfo.uboInfo.membersInfo.push_back(memberInfo);
                }
            }

            auto& uniformLogicalInfo = pipelineNativeInfo.legacyUBOInfo.uniforms[i];
            uniformLogicalInfo.type = uniformInfo.type;
            uniformLogicalInfo.location = uniformInfo.location;
            uniformLogicalInfo.arraySize = std::max(1, uniformInfo.arraySize);
            uniformLogicalInfo.uboOffset = uboOffset;
            uniformLogicalInfo.byteSize =
                uniformLogicalInfo.arraySize * snap::rhi::backend::opengl::getAlignedUniformSize(uniformInfo.type);

            uboOffset += uniformLogicalInfo.byteSize;
        }

        pipelineNativeInfo.legacyUBOInfo.uboInfo.size = ((uboOffset + 15) >> 4) << 4;
        pipelineNativeInfo.legacyUBOInfo.uboInfo.name = std::string(LegacyUBOName);
        pipelineNativeInfo.legacyUBOInfo.uboInfo.binding =
            LegacyUBOsBindingBase + snap::rhi::backend::opengl::LegacyUBOBinding;
    }
}

void buildPipelineNativeSampledTexturesInfo(const snap::rhi::backend::opengl::Profile& gl,
                                            const GLuint program,
                                            snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                            GLint uniformMaxLength,
                                            std::vector<char>& uniformNameBuffer) {
    for (const auto& uniformInfo : pipelineNativeInfo.uniformsInfo) {
        switch (uniformInfo.type) {
            case GL_SAMPLER_2D_ARRAY:
            case GL_INT_SAMPLER_2D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_2D_ARRAY_SHADOW:

            case GL_SAMPLER_2D_SHADOW:
            case GL_SAMPLER_CUBE_SHADOW:

            case GL_SAMPLER_EXTERNAL_OES:

            case GL_INT_SAMPLER_2D:
            case GL_INT_SAMPLER_3D:
            case GL_INT_SAMPLER_CUBE:

            case GL_UNSIGNED_INT_SAMPLER_2D:
            case GL_UNSIGNED_INT_SAMPLER_3D:
            case GL_UNSIGNED_INT_SAMPLER_CUBE:

            case GL_SAMPLER_2D_RECT:
            case GL_SAMPLER_2D:
            case GL_SAMPLER_3D:
            case GL_SAMPLER_CUBE: {
                if (uniformInfo.arraySize > 1) {
                    snap::rhi::common::throwException(
                        "[buildTextureSamplerDescription] arraySize > 1 for textures is unsupported");
                }
                snap::rhi::backend::opengl::PipelineNativeInfo::SampledTextureInfo info{};
                info.index = uniformInfo.index;
                info.name = uniformInfo.name;
                info.location = uniformInfo.location;
                info.type = uniformInfo.type;

                pipelineNativeInfo.sampledTexturesInfo.emplace_back(info);
            } break;
        }
    }

    std::sort(pipelineNativeInfo.sampledTexturesInfo.begin(),
              pipelineNativeInfo.sampledTexturesInfo.end(),
              [](const auto& l, const auto& r) { return l.name < r.name; });

    {
        GLuint activeProgram = GL_NONE;
        gl.getIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&activeProgram));

        gl.useProgram(program, nullptr); // need to setup texture units for texture locations
        snap::rhi::common::ScopeGuard _guard([&]() {
            if (gl.isProgram(activeProgram) == GL_TRUE) {
                GLint isFlaggedForDeletion = GL_NONE;
                gl.getProgramiv(activeProgram, GL_DELETE_STATUS, &isFlaggedForDeletion);

                if (isFlaggedForDeletion == GL_FALSE) {
                    gl.useProgram(activeProgram, nullptr);
                }
            } else if (activeProgram == GL_NONE) {
                gl.useProgram(GL_NONE, nullptr);
            }
        });

        for (size_t i = 0; i < pipelineNativeInfo.sampledTexturesInfo.size(); ++i) {
            gl.uniform1i(pipelineNativeInfo.sampledTexturesInfo[i].location, static_cast<int32_t>(i));
            pipelineNativeInfo.sampledTexturesInfo[i].binding = static_cast<uint32_t>(i);
        }
    }
}

void buildPipelineNativeImagesInfo(const snap::rhi::backend::opengl::Profile& gl,
                                   const GLuint program,
                                   snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                   GLint uniformMaxLength,
                                   std::vector<char>& uniformNameBuffer) {
    for (const auto& uniformInfo : pipelineNativeInfo.uniformsInfo) {
        switch (uniformInfo.type) {
            case GL_IMAGE_2D:
            case GL_IMAGE_3D:
            case GL_IMAGE_CUBE:
            case GL_IMAGE_2D_ARRAY:
            case GL_IMAGE_CUBE_MAP_ARRAY:

            case GL_INT_IMAGE_2D:
            case GL_INT_IMAGE_3D:
            case GL_INT_IMAGE_CUBE:
            case GL_INT_IMAGE_2D_ARRAY:
            case GL_INT_IMAGE_CUBE_MAP_ARRAY:

            case GL_UNSIGNED_INT_IMAGE_2D:
            case GL_UNSIGNED_INT_IMAGE_3D:
            case GL_UNSIGNED_INT_IMAGE_CUBE:
            case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
            case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY: {
                if (uniformInfo.arraySize > 1) {
                    snap::rhi::common::throwException(
                        "[buildTextureSamplerDescription] arraySize > 1 for textures is unsupported");
                }

                GLint imgUnit = -1;
                gl.getUniformiv(program, uniformInfo.location, &imgUnit);

                snap::rhi::backend::opengl::PipelineNativeInfo::ImageInfo info{};
                info.index = uniformInfo.index;
                info.name = uniformInfo.name;
                info.unit = imgUnit;

                pipelineNativeInfo.imagesInfo.emplace_back(info);
            } break;
        }
    }

    std::sort(pipelineNativeInfo.imagesInfo.begin(),
              pipelineNativeInfo.imagesInfo.end(),
              [](const auto& l, const auto& r) { return l.name < r.name; });
    for (size_t i = 0; i < pipelineNativeInfo.imagesInfo.size(); ++i) {
        pipelineNativeInfo.imagesInfo[i].binding = static_cast<uint32_t>(i);
    }
}

void buildPipelineNativeUBOsInfo(const snap::rhi::backend::opengl::Profile& gl,
                                 const GLuint program,
                                 snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                 const snap::rhi::opengl::PipelineInfo& pipelineInfo) {
    if (!gl.getFeatures().isNativeUBOSupported) {
        return;
    }

    for (const auto& resourceInfo : pipelineInfo.resources) {
        if (resourceInfo.descriptorType == snap::rhi::DescriptorType::UniformBuffer) {
            GLuint blockIndex = gl.getUniformBlockIndex(program, resourceInfo.name.c_str());

            if (blockIndex == GL_INVALID_INDEX) {
                continue;
            }

            snap::rhi::reflection::UniformBufferInfo info{};
            info.name = resourceInfo.name;

            /**
             * This wil be native binding after glUniformBlockBinding call
             */
            info.binding = blockIndex;

            pipelineNativeInfo.nativeUBOsInfo.emplace_back(info);
        }
    }

    std::sort(pipelineNativeInfo.nativeUBOsInfo.begin(),
              pipelineNativeInfo.nativeUBOsInfo.end(),
              [](const auto& l, const auto& r) { return l.name < r.name; });
    {
        /**
         * Before these code nativeUBOInfo[i].binding store UBO index,
         * After these code nativeUBOInfo[i].binding will store UBO binding.
         */
        for (size_t i = 0; i < pipelineNativeInfo.nativeUBOsInfo.size(); ++i) {
            gl.uniformBlockBinding(
                program, static_cast<GLint>(pipelineNativeInfo.nativeUBOsInfo[i].binding), static_cast<GLuint>(i));
            pipelineNativeInfo.nativeUBOsInfo[i].binding = static_cast<uint32_t>(i);
        }
    }
}

void buildPipelineNativeSampledTexturesInfo(const snap::rhi::backend::opengl::Profile& gl,
                                            const GLuint program,
                                            snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                            const snap::rhi::opengl::PipelineInfo& pipelineInfo) {
    for (const auto& resourceInfo : pipelineInfo.resources) {
        if (resourceInfo.descriptorType == snap::rhi::DescriptorType::SampledTexture) {
            GLint location = gl.getUniformLocation(program, resourceInfo.name.c_str());

            if (location == -1) {
                continue;
            }

            snap::rhi::backend::opengl::PipelineNativeInfo::SampledTextureInfo info{};
            // info.index; // In this case index should be invalid
            info.name = resourceInfo.name;
            info.location = location;

            // Type info is not available through SnapRHI API without full reflection reading
            info.type = GL_NONE;

            pipelineNativeInfo.sampledTexturesInfo.emplace_back(info);
        }
    }

    std::sort(pipelineNativeInfo.sampledTexturesInfo.begin(),
              pipelineNativeInfo.sampledTexturesInfo.end(),
              [](const auto& l, const auto& r) { return l.name < r.name; });

    {
        GLuint activeProgram = GL_NONE;
        gl.getIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&activeProgram));

        gl.useProgram(program, nullptr); // need to set up texture units for texture locations
        snap::rhi::common::ScopeGuard _guard([&]() {
            if (gl.isProgram(activeProgram) == GL_TRUE) {
                GLint isFlaggedForDeletion = GL_NONE;
                gl.getProgramiv(activeProgram, GL_DELETE_STATUS, &isFlaggedForDeletion);

                if (isFlaggedForDeletion == GL_FALSE) {
                    gl.useProgram(activeProgram, nullptr);
                }
            } else if (activeProgram == GL_NONE) {
                gl.useProgram(GL_NONE, nullptr);
            }
        });

        for (size_t i = 0; i < pipelineNativeInfo.sampledTexturesInfo.size(); ++i) {
            gl.uniform1i(pipelineNativeInfo.sampledTexturesInfo[i].location, static_cast<int32_t>(i));
            pipelineNativeInfo.sampledTexturesInfo[i].binding = static_cast<uint32_t>(i);
        }
    }
}

void buildPipelineNativeSSBOsInfo(const snap::rhi::backend::opengl::Profile& gl,
                                  const GLuint program,
                                  snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                  const snap::rhi::opengl::PipelineInfo& pipelineInfo) {
    if (!gl.getFeatures().isSSBOSupported) {
        return;
    }

    for (const auto& resourceInfo : pipelineInfo.resources) {
        if (resourceInfo.descriptorType == snap::rhi::DescriptorType::StorageBuffer) {
            snap::rhi::backend::opengl::PipelineNativeInfo::SSBOInfo info{};
            info.name = resourceInfo.name;

            // Size info is not available through SnapRHI API without full reflection reading
            info.size = 0;
            info.index = gl.getProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, resourceInfo.name.c_str());

            if (info.index == GL_INVALID_INDEX) {
                continue;
            }

            const GLenum propsBinding[] = {GL_BUFFER_BINDING};
            gl.getProgramResourceiv(
                program, GL_SHADER_STORAGE_BLOCK, info.index, 1, propsBinding, 1, nullptr, &info.binding);
            pipelineNativeInfo.ssbosInfo.emplace_back(info);
        }
    }

    std::sort(pipelineNativeInfo.ssbosInfo.begin(),
              pipelineNativeInfo.ssbosInfo.end(),
              [](const auto& l, const auto& r) { return l.name < r.name; });
}

void buildPipelineNativeImagesInfo(const snap::rhi::backend::opengl::Profile& gl,
                                   const GLuint program,
                                   snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                   const snap::rhi::opengl::PipelineInfo& pipelineInfo) {
    for (const auto& resourceInfo : pipelineInfo.resources) {
        if (resourceInfo.descriptorType == snap::rhi::DescriptorType::StorageTexture) {
            GLint location = gl.getUniformLocation(program, resourceInfo.name.c_str());

            if (location == -1) {
                continue;
            }

            GLint imgUnit = -1;
            gl.getUniformiv(program, location, &imgUnit);

            snap::rhi::backend::opengl::PipelineNativeInfo::ImageInfo info{};
            // info.index; // In this case index should be invalid
            info.name = resourceInfo.name;
            info.unit = imgUnit;

            pipelineNativeInfo.imagesInfo.emplace_back(info);
        }
    }

    std::sort(pipelineNativeInfo.imagesInfo.begin(),
              pipelineNativeInfo.imagesInfo.end(),
              [](const auto& l, const auto& r) { return l.name < r.name; });
    for (size_t i = 0; i < pipelineNativeInfo.imagesInfo.size(); ++i) {
        pipelineNativeInfo.imagesInfo[i].binding = static_cast<uint32_t>(i);
    }
}

snap::rhi::VertexAttributeFormat convertToAttributeFormat(const GLenum format) {
    // https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glBindAttribLocation.xml
    /**
     * If name refers to a matrix attribute variable, index refers to the first column of the matrix.
     * Other matrix columns are then automatically bound to locations index+1 for a matrix of type mat2;
     * index+1 and index+2 for a matrix of type mat3;
     * and index+1, index+2, and index+3 for a matrix of type mat4.
     */
    switch (format) {
        case GL_FLOAT:
            return snap::rhi::VertexAttributeFormat::Float;

        case GL_FLOAT_MAT2:
        case GL_FLOAT_VEC2:
            return snap::rhi::VertexAttributeFormat::Float2;

        case GL_FLOAT_MAT3:
        case GL_FLOAT_VEC3:
            return snap::rhi::VertexAttributeFormat::Float3;

        case GL_FLOAT_MAT4:
        case GL_FLOAT_VEC4:
            return snap::rhi::VertexAttributeFormat::Float4;

        case GL_INT:
            return snap::rhi::VertexAttributeFormat::Int;

        case GL_INT_VEC2:
            return snap::rhi::VertexAttributeFormat::Int2;

        case GL_INT_VEC3:
            return snap::rhi::VertexAttributeFormat::Int3;

        case GL_INT_VEC4:
            return snap::rhi::VertexAttributeFormat::Int4;

        case GL_UNSIGNED_INT:
            return snap::rhi::VertexAttributeFormat::UInt;

        case GL_UNSIGNED_INT_VEC2:
            return snap::rhi::VertexAttributeFormat::UInt2;

        case GL_UNSIGNED_INT_VEC3:
            return snap::rhi::VertexAttributeFormat::UInt3;

        case GL_UNSIGNED_INT_VEC4:
            return snap::rhi::VertexAttributeFormat::UInt4;

        default:
            snap::rhi::common::throwException("invalid format");
    }
}

void buildPipelineAttributesInfo(const snap::rhi::backend::opengl::Profile& gl,
                                 const GLuint program,
                                 snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo) {
    GLint attributeMaxLength = 0;
    gl.getProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attributeMaxLength);
    ++attributeMaxLength;
    std::string attributeNameBuffer(attributeMaxLength, 0);

    GLint activeAttributesCount = 0;
    gl.getProgramiv(program, GL_ACTIVE_ATTRIBUTES, &activeAttributesCount);

    for (GLint i = 0; i < activeAttributesCount; ++i) {
        snap::rhi::backend::opengl::PipelineNativeInfo::AttribInfo attribInfo{};

        GLint attributeSize = 0;
        GLenum type = GL_NONE;

        GLsizei receivedBuffSize = 0;
        gl.getActiveAttrib(program,
                           static_cast<GLuint>(i),
                           attributeMaxLength,
                           &receivedBuffSize,
                           &attributeSize,
                           &type,
                           attributeNameBuffer.data());
        attribInfo.name = std::string{attributeNameBuffer.data()};
        attribInfo.format = convertToAttributeFormat(type);

        if (snap::rhi::backend::opengl::isSystemResource(attribInfo.name)) {
            continue;
        }

        attribInfo.location = gl.getAttribLocation(program, attribInfo.name.data());
        pipelineNativeInfo.attribsInfo.emplace_back(attribInfo);
    }
}

void buildPipelineAttributesInfo(const snap::rhi::backend::opengl::Profile& gl,
                                 const GLuint program,
                                 const std::vector<snap::rhi::opengl::VertexAttributeInfo>& vertexAttributes,
                                 const snap::rhi::VertexInputStateCreateInfo& layout,
                                 snap::rhi::backend::opengl::PipelineNativeInfo& pipelineNativeInfo,
                                 const snap::rhi::backend::common::ValidationLayer& validationLayer) {
    const std::span<const snap::rhi::VertexInputAttributeDescription> attribs{layout.attributeDescription.data(),
                                                                              layout.attributesCount};

    for (const auto& vertexAttribLogicalInfo : vertexAttributes) {
        auto attribItr = std::ranges::find_if(attribs, [&](const snap::rhi::VertexInputAttributeDescription& attrib) {
            return attrib.location == static_cast<uint32_t>(vertexAttribLogicalInfo.location);
        });
        SNAP_RHI_VALIDATE(validationLayer,
                          attribItr != attribs.end(),
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::DeviceOp,
                          "[buildPipelineAttributesInfo] attribute location %d not found in vertex input layout.",
                          vertexAttribLogicalInfo.location);

        snap::rhi::backend::opengl::PipelineNativeInfo::AttribInfo attribInfo{};
        attribInfo.name = vertexAttribLogicalInfo.name;
        attribInfo.format = attribItr->format;
        attribInfo.location = gl.getAttribLocation(program, attribInfo.name.data());

        if (attribInfo.location == -1) {
            continue;
        }

        pipelineNativeInfo.attribsInfo.emplace_back(attribInfo);
    }
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
uint32_t getAlignedUniformSize(const GLenum type) {
    switch (type) {
        // Use int for bool
        case GL_BOOL:
            return sizeof(uint32_t);
        case GL_BOOL_VEC2:
            return sizeof(uint32_t) * 2;
        case GL_BOOL_VEC3:
            return sizeof(uint32_t) * 4;
        case GL_BOOL_VEC4:
            return sizeof(uint32_t) * 4;

        case GL_FLOAT:
            return sizeof(float);
        case GL_FLOAT_VEC2:
            return sizeof(float) * 2;
        case GL_FLOAT_VEC3:
            return sizeof(float) * 4; // vec3 -> vec4
        case GL_FLOAT_VEC4:
            return sizeof(float) * 4;

        case GL_INT:
            return sizeof(int32_t);
        case GL_INT_VEC2:
            return sizeof(int32_t) * 2;
        case GL_INT_VEC3:
            return sizeof(int32_t) * 4;
        case GL_INT_VEC4:
            return sizeof(int32_t) * 4;

        case GL_UNSIGNED_INT:
            return sizeof(uint32_t);
        case GL_UNSIGNED_INT_VEC2:
            return sizeof(uint32_t) * 2;
        case GL_UNSIGNED_INT_VEC3:
            return sizeof(uint32_t) * 4;
        case GL_UNSIGNED_INT_VEC4:
            return sizeof(uint32_t) * 4;

        case GL_FLOAT_MAT2:
            return sizeof(float) * 2 * 2;
        case GL_FLOAT_MAT3:
            return sizeof(float) * 3 * 4; // mat3 -> mat3x4
        case GL_FLOAT_MAT4:
            return sizeof(float) * 4 * 4;

        default:
            snap::rhi::common::throwException("[getAlignedUniformSize] invalid type");
    }
}

PipelineNativeInfo buildPipelineNativeInfo(const Profile& gl,
                                           const GLuint program,
                                           const bool acquireNativeReflection,
                                           bool isLegacyPipeline,
                                           const PipelineConfigurationInfo& info) {
    PipelineNativeInfo pipelineNativeInfo{};
    auto UniformNameBufferAllocator = [&]() -> std::pair<std::vector<char>, GLint> {
        GLint uniformMaxLength = 0;
        gl.getProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformMaxLength);
        ++uniformMaxLength;
        return {std::vector<char>(uniformMaxLength, 0), uniformMaxLength};
    };

    if (acquireNativeReflection) {
        auto [uniformNameBuffer, uniformMaxLength] = UniformNameBufferAllocator();

        buildPipelineNativeUniformsInfo(gl, program, pipelineNativeInfo, uniformMaxLength, uniformNameBuffer);
        { // build pipeline info
            buildPipelineNativeSSBOsInfo(gl, program, pipelineNativeInfo, uniformMaxLength, uniformNameBuffer);
            if (!isLegacyPipeline) {
                buildPipelineNativeUBOsInfo(gl, program, pipelineNativeInfo, uniformMaxLength, uniformNameBuffer);
                pipelineNativeInfo.pipelineUniformManagmentType = PipelineUniformManagmentType::Default;
                if (pipelineNativeInfo.nativeUBOsInfo.empty()) {
                    buildPipelineNativeCompatibleUBOsInfo(
                        gl, program, pipelineNativeInfo, uniformMaxLength, uniformNameBuffer);
                    pipelineNativeInfo.pipelineUniformManagmentType = PipelineUniformManagmentType::Compatible;
                }
            } else {
                buildPipelineNativeLegacyUBOsInfo(pipelineNativeInfo);
                pipelineNativeInfo.pipelineUniformManagmentType = PipelineUniformManagmentType::Legacy;
            }
            buildPipelineNativeSampledTexturesInfo(
                gl, program, pipelineNativeInfo, uniformMaxLength, uniformNameBuffer);
            buildPipelineNativeImagesInfo(gl, program, pipelineNativeInfo, uniformMaxLength, uniformNameBuffer);
        }
    } else {
        std::optional<snap::rhi::opengl::PipelineInfo> pipelineInfo;
        if (const auto* computePipelineInfoPtr = std::get_if<ComputePipelineConfigurationInfo>(&info)) {
            pipelineInfo = computePipelineInfoPtr->computePipelineInfo;
        } else if (const auto* renderPipelineInfoPtr = std::get_if<RenderPipelineConfigurationInfo>(&info)) {
            pipelineInfo = renderPipelineInfoPtr->renderPipelineInfo;
        }

        if (!pipelineInfo) {
            snap::rhi::common::throwException(
                "[buildPipelineNativeInfo] In order to init OpenGL pipeline propely, or "
                "NativeReflection should be acquired or PipelineLayout should be provided!");
        }

        const auto& info = *pipelineInfo;
        buildPipelineNativeSampledTexturesInfo(gl, program, pipelineNativeInfo, info);
        buildPipelineNativeUBOsInfo(gl, program, pipelineNativeInfo, info);
        pipelineNativeInfo.pipelineUniformManagmentType = PipelineUniformManagmentType::Default;
        if (pipelineNativeInfo.nativeUBOsInfo.empty()) {
            auto [uniformNameBuffer, uniformMaxLength] = UniformNameBufferAllocator();
            buildPipelineNativeUniformsInfo(gl, program, pipelineNativeInfo, uniformMaxLength, uniformNameBuffer);
            buildPipelineNativeCompatibleUBOsInfo(gl, program, pipelineNativeInfo, uniformMaxLength, uniformNameBuffer);
            pipelineNativeInfo.pipelineUniformManagmentType = PipelineUniformManagmentType::Compatible;
        }
        buildPipelineNativeSSBOsInfo(gl, program, pipelineNativeInfo, info);
        buildPipelineNativeImagesInfo(gl, program, pipelineNativeInfo, info);
    }

    if (const auto* renderPipelineInfoPtr = std::get_if<RenderPipelineConfigurationInfo>(&info)) {
        if (acquireNativeReflection) {
            buildPipelineAttributesInfo(gl, program, pipelineNativeInfo);
        } else {
            if (!renderPipelineInfoPtr->renderPipelineInfo) {
                snap::rhi::common::throwException(
                    "[buildPipelineNativeInfo] In order to init OpenGL pipeline attribs propely, or "
                    "NativeReflection should be acquired or PipelineLayout should be provided!");
            }

            assert(!isLegacyPipeline);
            buildPipelineAttributesInfo(gl,
                                        program,
                                        renderPipelineInfoPtr->renderPipelineInfo->vertexAttributes,
                                        renderPipelineInfoPtr->layout,
                                        pipelineNativeInfo,
                                        gl.getDevice()->getValidationLayer());
        }
    }

    return pipelineNativeInfo;
}

PipelineSSBODescription buildSSBODesc(const PipelineNativeInfo& pipelineNativeInfo,
                                      const snap::rhi::opengl::PipelineInfo* pipelineInfo,
                                      std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection) {
    PipelineSSBODescription result{};

    if (!pipelineInfo) {
        // Shader without reflection

        prepareDefaultDS(physicalReflection);
        for (const auto& ssboNativeInfo : pipelineNativeInfo.ssbosInfo) {
            SSBOInfo resInfo{};
            resInfo.binding = ssboNativeInfo.binding;
            resInfo.size = ssboNativeInfo.size;
            resInfo.logicalBinding = LegacySSBOsBindingBase + ssboNativeInfo.binding;
            result[DefaultDescriptorSetID].push_back(resInfo);

            snap::rhi::reflection::StorageBufferInfo ssboInfo{};
            ssboInfo.size = static_cast<uint32_t>(ssboNativeInfo.size);
            ssboInfo.name = ssboNativeInfo.name;
            ssboInfo.binding = resInfo.logicalBinding;

            physicalReflection[0].storageBuffers.push_back(ssboInfo);
        }
    } else {
        // Shader with reflection
        std::unordered_map<std::string, snap::rhi::opengl::ResourceBinding> ssboNameInfoMap;
        for (const auto& resourceInfo : pipelineInfo->resources) {
            if (resourceInfo.descriptorType == snap::rhi::DescriptorType::StorageBuffer) {
                ssboNameInfoMap[resourceInfo.name] = resourceInfo.binding;
            }
        }

        std::unordered_map<uint32_t, size_t> dstDSIdToIdxMap;
        for (size_t i = 0; i < physicalReflection.size(); ++i) {
            dstDSIdToIdxMap[physicalReflection[i].binding] = i;
            assert(physicalReflection[i].storageBuffers.empty());
        }

        for (const auto& ssboInfo : pipelineNativeInfo.ssbosInfo) {
            const auto& itr = ssboNameInfoMap.find(ssboInfo.name);
            if (itr == ssboNameInfoMap.end()) {
                snap::rhi::common::throwException("[buildSSBODesc] incomplete reflections");
            }

            const auto& ssboBinding = itr->second;
            snap::rhi::reflection::StorageBufferInfo info{};
            info.size = static_cast<uint32_t>(ssboInfo.size);
            info.name = ssboInfo.name;
            info.binding = ssboBinding.binding;

            const auto& dstDSItr = dstDSIdToIdxMap.find(ssboBinding.descriptorSet);
            if (dstDSItr == dstDSIdToIdxMap.end()) {
                snap::rhi::reflection::DescriptorSetInfo dstDSInfo{};
                dstDSInfo.binding = ssboBinding.descriptorSet;
                dstDSInfo.storageBuffers.push_back(info);
                dstDSIdToIdxMap[ssboBinding.descriptorSet] = physicalReflection.size();
                physicalReflection.push_back(dstDSInfo);
            } else {
                snap::rhi::reflection::DescriptorSetInfo& dstDSInfo = physicalReflection[dstDSItr->second];
                dstDSInfo.storageBuffers.push_back(info);
            }

            SSBOInfo resInfo{};
            resInfo.binding = ssboInfo.binding;
            resInfo.size = ssboInfo.size;
            resInfo.logicalBinding = info.binding;
            result[ssboBinding.descriptorSet].push_back(resInfo);
        }
    }

    for (auto& dsInfo : result) {
        if (!dsInfo.empty()) {
            std::sort(dsInfo.begin(), dsInfo.end(), [](const auto& l, const auto& r) {
                return l.logicalBinding < r.logicalBinding;
            });
        }
    }

    return result;
}

PipelineUBODescription buildUBODesc(const PipelineNativeInfo& pipelineNativeInfo,
                                    const snap::rhi::opengl::PipelineInfo* pipelineInfo,
                                    std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection) {
    PipelineUBODescription result{};
    if (!pipelineInfo) {
        // Shader without reflection

        prepareDefaultDS(physicalReflection);
        for (const auto& nativeUBOsInfo : pipelineNativeInfo.nativeUBOsInfo) {
            UBOInfo resInfo{};
            resInfo.binding = nativeUBOsInfo.binding;
            resInfo.size = nativeUBOsInfo.size;
            resInfo.logicalBinding = LegacyUBOsBindingBase + resInfo.binding;
            result[DefaultDescriptorSetID].push_back(resInfo);

            snap::rhi::reflection::UniformBufferInfo info{};
            info.size = nativeUBOsInfo.size;
            info.name = nativeUBOsInfo.name;
            info.binding = resInfo.logicalBinding;
            info.membersInfo = nativeUBOsInfo.membersInfo;

            physicalReflection[0].uniformBuffers.push_back(info);
        }
    } else {
        // Shader with reflection
        std::unordered_map<std::string, snap::rhi::opengl::ResourceBinding> uboNameInfoMap;
        for (const auto& resourceInfo : pipelineInfo->resources) {
            if (resourceInfo.descriptorType == snap::rhi::DescriptorType::UniformBuffer) {
                uboNameInfoMap[resourceInfo.name] = resourceInfo.binding;
            }
        }

        std::unordered_map<uint32_t, size_t> dstDSIdToIdxMap;
        for (size_t i = 0; i < physicalReflection.size(); ++i) {
            dstDSIdToIdxMap[physicalReflection[i].binding] = i;
            assert(physicalReflection[i].uniformBuffers.empty());
        }

        for (const auto& nativeUBOsInfo : pipelineNativeInfo.nativeUBOsInfo) {
            const auto& itr = uboNameInfoMap.find(nativeUBOsInfo.name);
            if (itr == uboNameInfoMap.end()) {
                snap::rhi::common::throwException("[buildUBODesc] incomplete reflections");
            }

            const auto& uboBinding = itr->second;
            snap::rhi::reflection::UniformBufferInfo info = nativeUBOsInfo;
            info.binding = uboBinding.binding;
            info.size = nativeUBOsInfo.size;
            info.name = nativeUBOsInfo.name;
            info.membersInfo = nativeUBOsInfo.membersInfo;

            const auto& dstDSItr = dstDSIdToIdxMap.find(uboBinding.descriptorSet);
            if (dstDSItr == dstDSIdToIdxMap.end()) {
                snap::rhi::reflection::DescriptorSetInfo dstDSInfo{};
                dstDSInfo.binding = uboBinding.descriptorSet;
                dstDSInfo.uniformBuffers.push_back(info);
                dstDSIdToIdxMap[uboBinding.descriptorSet] = physicalReflection.size();
                physicalReflection.push_back(dstDSInfo);
            } else {
                snap::rhi::reflection::DescriptorSetInfo& dstDSInfo = physicalReflection[dstDSItr->second];
                dstDSInfo.uniformBuffers.push_back(info);
            }

            UBOInfo resInfo{};
            resInfo.binding = nativeUBOsInfo.binding;
            resInfo.size = nativeUBOsInfo.size;
            resInfo.logicalBinding = info.binding;
            result[uboBinding.descriptorSet].push_back(resInfo);
        }
    }

    for (auto& dsInfo : result) {
        if (!dsInfo.empty()) {
            std::sort(dsInfo.begin(), dsInfo.end(), [](const auto& l, const auto& r) {
                return l.logicalBinding < r.logicalBinding;
            });
        }
    }

    return result;
}

PipelineCompatibleUBODescription buildCompatibleUBODesc(
    const PipelineNativeInfo& pipelineNativeInfo,
    const snap::rhi::opengl::PipelineInfo* pipelineInfo,
    std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection) {
    PipelineCompatibleUBODescription result{};

    if (!pipelineInfo) {
        // Shader without reflection

        prepareDefaultDS(physicalReflection);
        for (const auto& nativeCompatibleUBOInfo : pipelineNativeInfo.compatibleUBOsInfo) {
            CompatibleUBOInfo resInfo{};
            resInfo.location = nativeCompatibleUBOInfo.location;
            resInfo.arraySize = nativeCompatibleUBOInfo.arraySize;
            resInfo.bufferType =
                nativeCompatibleUBOInfo.type == GL_FLOAT_VEC4 ? UniformBufferType::Float : UniformBufferType::Int;

            resInfo.logicalBinding = LegacyUBOsBindingBase + nativeCompatibleUBOInfo.binding;
            result[DefaultDescriptorSetID].push_back(resInfo);

            snap::rhi::reflection::UniformBufferInfo info{};
            info.size = nativeCompatibleUBOInfo.arraySize * sizeof(float) * 4;
            info.name = nativeCompatibleUBOInfo.name;
            info.binding = resInfo.logicalBinding;

            physicalReflection[0].uniformBuffers.push_back(info);
        }
    } else {
        // Shader with reflection
        std::unordered_map<std::string, snap::rhi::opengl::ResourceBinding> uboNameInfoMap;
        for (const auto& resourceInfo : pipelineInfo->resources) {
            if (resourceInfo.descriptorType == snap::rhi::DescriptorType::UniformBuffer) {
                uboNameInfoMap[resourceInfo.name] = resourceInfo.binding;
            }
        }

        std::unordered_map<uint32_t, size_t> dstDSIdToIdxMap;
        for (size_t i = 0; i < physicalReflection.size(); ++i) {
            dstDSIdToIdxMap[physicalReflection[i].binding] = i;
            assert(physicalReflection[i].uniformBuffers.empty());
        }

        for (const auto& nativeComatibleUBOInfo : pipelineNativeInfo.compatibleUBOsInfo) {
            const auto& itr = uboNameInfoMap.find(nativeComatibleUBOInfo.name);
            if (itr == uboNameInfoMap.end()) {
                snap::rhi::common::throwException("[buildUBODesc] incomplete reflections");
            }

            const auto& uboBinding = itr->second;
            snap::rhi::reflection::UniformBufferInfo info{};
            info.size = nativeComatibleUBOInfo.arraySize * sizeof(float) * 4;
            info.name = nativeComatibleUBOInfo.name;
            info.binding = uboBinding.binding;

            const auto& dstDSItr = dstDSIdToIdxMap.find(uboBinding.descriptorSet);
            if (dstDSItr == dstDSIdToIdxMap.end()) {
                snap::rhi::reflection::DescriptorSetInfo dstDSInfo{};
                dstDSInfo.binding = uboBinding.descriptorSet;
                dstDSInfo.uniformBuffers.push_back(info);
                dstDSIdToIdxMap[uboBinding.descriptorSet] = physicalReflection.size();
                physicalReflection.push_back(dstDSInfo);
            } else {
                snap::rhi::reflection::DescriptorSetInfo& dstDSInfo = physicalReflection[dstDSItr->second];
                dstDSInfo.uniformBuffers.push_back(info);
            }

            CompatibleUBOInfo resInfo{};
            resInfo.location = nativeComatibleUBOInfo.location;
            resInfo.arraySize = nativeComatibleUBOInfo.arraySize;
            resInfo.bufferType =
                nativeComatibleUBOInfo.type == GL_FLOAT_VEC4 ? UniformBufferType::Float : UniformBufferType::Int;

            resInfo.logicalBinding = info.binding;
            result[uboBinding.descriptorSet].push_back(resInfo);
        }
    }

    for (auto& dsInfo : result) {
        if (!dsInfo.empty()) {
            std::sort(dsInfo.begin(), dsInfo.end(), [](const auto& l, const auto& r) {
                return l.logicalBinding < r.logicalBinding;
            });
        }
    }
    return result;
}

PipelineTextureDescription buildTextureDesc(const PipelineNativeInfo& pipelineNativeInfo,
                                            const snap::rhi::opengl::PipelineInfo* pipelineInfo,
                                            std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection) {
    PipelineTextureDescription result{};

    if (!pipelineInfo) {
        // Shader without reflection

        prepareDefaultDS(physicalReflection);
        for (const auto& nativeSampledTextureInfo : pipelineNativeInfo.sampledTexturesInfo) {
            SampledTextureInfo resInfo{};
            resInfo.target = getTextureTarget(nativeSampledTextureInfo.type);
            resInfo.binding = nativeSampledTextureInfo.binding;
            resInfo.name = nativeSampledTextureInfo.name;

            resInfo.logicalBinding = LegacyTexturesBindingBase + nativeSampledTextureInfo.binding;
            result[DefaultDescriptorSetID].push_back(resInfo);

            snap::rhi::reflection::SampledTextureInfo info{};
            info.name = nativeSampledTextureInfo.name;
            info.binding = resInfo.logicalBinding;
            info.type = convertToTextureType(nativeSampledTextureInfo.type);

            physicalReflection[0].sampledTextures.push_back(info);
        }
    } else {
        // Shader with reflection
        std::unordered_map<std::string, snap::rhi::opengl::ResourceBinding> texNameInfoMap;
        for (const auto& resourceInfo : pipelineInfo->resources) {
            if (resourceInfo.descriptorType == snap::rhi::DescriptorType::SampledTexture) {
                texNameInfoMap[resourceInfo.name] = resourceInfo.binding;
            }
        }

        std::unordered_map<uint32_t, size_t> dstDSIdToIdxMap;
        for (size_t i = 0; i < physicalReflection.size(); ++i) {
            dstDSIdToIdxMap[physicalReflection[i].binding] = i;
            assert(physicalReflection[i].sampledTextures.empty());
        }

        for (const auto& nativeSampledTextureInfo : pipelineNativeInfo.sampledTexturesInfo) {
            const auto& itr = texNameInfoMap.find(nativeSampledTextureInfo.name);
            if (itr == texNameInfoMap.end()) {
                snap::rhi::common::throwException("[buildUBODesc] incomplete reflections");
            }

            const auto& sampledTextureBinding = itr->second;
            snap::rhi::reflection::SampledTextureInfo info{};
            info.binding = sampledTextureBinding.binding;
            info.name = nativeSampledTextureInfo.name;
            info.type = convertToTextureType(nativeSampledTextureInfo.type);

            const auto& dstDSItr = dstDSIdToIdxMap.find(sampledTextureBinding.descriptorSet);
            if (dstDSItr == dstDSIdToIdxMap.end()) {
                snap::rhi::reflection::DescriptorSetInfo dstDSInfo{};
                dstDSInfo.binding = sampledTextureBinding.descriptorSet;
                dstDSInfo.sampledTextures.push_back(info);
                dstDSIdToIdxMap[sampledTextureBinding.descriptorSet] = physicalReflection.size();
                physicalReflection.push_back(dstDSInfo);
            } else {
                snap::rhi::reflection::DescriptorSetInfo& dstDSInfo = physicalReflection[dstDSItr->second];
                dstDSInfo.sampledTextures.push_back(info);
            }

            SampledTextureInfo resInfo{};
            resInfo.target = getTextureTarget(nativeSampledTextureInfo.type);
            resInfo.binding = nativeSampledTextureInfo.binding;
            resInfo.name = nativeSampledTextureInfo.name;

            resInfo.logicalBinding = info.binding;
            result[sampledTextureBinding.descriptorSet].push_back(resInfo);
        }
    }

    for (auto& dsInfo : result) {
        if (!dsInfo.empty()) {
            std::sort(dsInfo.begin(), dsInfo.end(), [](const auto& l, const auto& r) {
                return l.logicalBinding < r.logicalBinding;
            });
        }
    }

    return result;
}

PipelineSamplerDescription buildSamplerDesc(const PipelineNativeInfo& pipelineNativeInfo,
                                            const RenderPipelineConfigurationInfo& renderPipelineInfo,
                                            std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection,
                                            const PipelineTextureDescription& texInfo) {
    std::unordered_map<uint32_t, std::map<uint32_t, std::vector<GLint>>> resultInfo{};
    if (!renderPipelineInfo.renderPipelineInfo) {
        // Shader without reflection
        prepareDefaultDS(physicalReflection);
        for (size_t i = 0; i < texInfo.size(); ++i) {
            const auto& texDsInfo = texInfo[i];
            const uint32_t dsId = static_cast<uint32_t>(i);

            for (const auto& texInfo : texDsInfo) {
                assert(dsId == DefaultDescriptorSetID);

                const uint32_t smplBinding = legacyTextureBindingToSamplerBinding(texInfo.logicalBinding);
                resultInfo[dsId][smplBinding].push_back(texInfo.binding);

                snap::rhi::reflection::SamplerInfo smplInfo{};
                smplInfo.name = texInfo.name + std::string(snap::rhi::backend::common::SamplerDefaultSuffix.data());
                smplInfo.binding = smplBinding;

                physicalReflection[0].samplers.push_back(smplInfo);
            }
        }
    } else {
        // Shader with reflection
        const auto& textureSamplerInfos = renderPipelineInfo.renderPipelineInfo->combinedTextureSamplerInfos;
        std::set<snap::rhi::opengl::ResourceBinding> activeSamplers;
        for (const auto& resourceInfo : renderPipelineInfo.renderPipelineInfo->resources) {
            if (resourceInfo.descriptorType == snap::rhi::DescriptorType::SampledTexture) {
                const auto& textureBinding = resourceInfo.binding;
                const auto& nativeTexInfo = texInfo[textureBinding.descriptorSet];

                auto nativeTextureItr =
                    std::find_if(nativeTexInfo.begin(),
                                 nativeTexInfo.end(),
                                 [binding = textureBinding.binding](const SampledTextureInfo& info) {
                                     return info.logicalBinding == binding;
                                 });
                if (nativeTextureItr == nativeTexInfo.end()) {
                    continue; // there are no such texture in descriptor set
                }

                // This texture used in GL program and we should add sampler for this texture to physicalReflection
                auto textureSamplerItr = std::find_if(
                    textureSamplerInfos.begin(),
                    textureSamplerInfos.end(),
                    [dsID = textureBinding.descriptorSet,
                     binding = textureBinding.binding](const snap::rhi::opengl::CombinedTextureSamplerInfo& info) {
                        return info.textureBinding.binding == binding && info.textureBinding.descriptorSet == dsID;
                    });
                if (textureSamplerItr == textureSamplerInfos.end()) {
                    snap::rhi::common::throwException("[buildSamplerDesc] texture not found!");
                }

                activeSamplers.insert({.descriptorSet = textureSamplerItr->samplerBinding.descriptorSet,
                                       .binding = textureSamplerItr->samplerBinding.binding});

                resultInfo[textureSamplerItr->samplerBinding.descriptorSet][textureSamplerItr->samplerBinding.binding]
                    .push_back(nativeTextureItr->binding);
            }
        }

        std::unordered_map<uint32_t, size_t> dstDSIdToIdxMap;
        for (size_t i = 0; i < physicalReflection.size(); ++i) {
            dstDSIdToIdxMap[physicalReflection[i].binding] = i;
            assert(physicalReflection[i].samplers.empty());
        }

        for (const auto& resourceInfo : renderPipelineInfo.renderPipelineInfo->resources) {
            if (resourceInfo.descriptorType == snap::rhi::DescriptorType::Sampler) {
                const auto& samplerBinding = resourceInfo.binding;
                snap::rhi::reflection::SamplerInfo info{};
                info.name = resourceInfo.name;
                info.binding = samplerBinding.binding;

                if (activeSamplers.count(samplerBinding)) {
                    const auto& dstDSItr = dstDSIdToIdxMap.find(samplerBinding.descriptorSet);
                    if (dstDSItr == dstDSIdToIdxMap.end()) {
                        snap::rhi::reflection::DescriptorSetInfo dstDSInfo{};
                        dstDSInfo.binding = samplerBinding.descriptorSet;
                        dstDSInfo.samplers.push_back(info);
                        dstDSIdToIdxMap[samplerBinding.descriptorSet] = physicalReflection.size();
                        physicalReflection.push_back(dstDSInfo);
                    } else {
                        snap::rhi::reflection::DescriptorSetInfo& dstDSInfo = physicalReflection[dstDSItr->second];
                        dstDSInfo.samplers.push_back(info);
                    }
                }
            }
        }
    }

    PipelineSamplerDescription result{};
    for (const auto& [dsId, dsInfo] : resultInfo) {
        for (const auto& [bindingId, smplBindings] : dsInfo) {
            assert(dsId < result.size());

            SamplerInfo info{};
            info.binding = smplBindings;
            info.logicalBinding = bindingId;

            result[dsId].push_back(info);
        }
    }
    return result;
}

PipelineImageDescription buildImageDesc(const PipelineNativeInfo& pipelineNativeInfo,
                                        const snap::rhi::opengl::PipelineInfo* pipelineInfo,
                                        std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection) {
    std::unordered_map<uint32_t, size_t> dstDSIdToIdxMap;
    for (size_t i = 0; i < physicalReflection.size(); ++i) {
        dstDSIdToIdxMap[physicalReflection[i].binding] = i;
        assert(physicalReflection[i].storageTextures.empty());
    }

    PipelineImageDescription result{};
    if (!pipelineInfo) {
        // Shader without reflection

        prepareDefaultDS(physicalReflection);
        for (const auto& nativeImageInfo : pipelineNativeInfo.imagesInfo) {
            ImageInfo resInfo{};
            resInfo.unit = nativeImageInfo.unit;
            resInfo.access = GL_READ_WRITE;
            resInfo.logicalBinding = LegacyImagesBindingBase + nativeImageInfo.binding;
            result[DefaultDescriptorSetID].push_back(resInfo);

            snap::rhi::reflection::StorageTextureInfo imageInfo{};
            imageInfo.type = snap::rhi::reflection::TextureType::Undefined;
            imageInfo.name = nativeImageInfo.name;
            imageInfo.binding = resInfo.logicalBinding;
            imageInfo.access = snap::rhi::reflection::ImageAccess::Undefined;

            physicalReflection[0].storageTextures.push_back(imageInfo);
        }
    } else {
        // Shader with reflection

        std::unordered_map<std::string, snap::rhi::opengl::ResourceBinding> imgNameInfoMap;
        for (const auto& resourceInfo : pipelineInfo->resources) {
            if (resourceInfo.descriptorType == snap::rhi::DescriptorType::StorageTexture) {
                imgNameInfoMap[resourceInfo.name] = resourceInfo.binding;
            }
        }

        for (const auto& nativeImageInfo : pipelineNativeInfo.imagesInfo) {
            const auto& itr = imgNameInfoMap.find(nativeImageInfo.name);
            if (itr == imgNameInfoMap.end()) {
                snap::rhi::common::throwException("[buildImageDesc] incomplete reflections");
            }

            const auto& imageBinding = itr->second;
            snap::rhi::reflection::StorageTextureInfo info{};
            info.type = snap::rhi::reflection::TextureType::Undefined;
            info.name = nativeImageInfo.name;
            info.binding = imageBinding.binding;
            info.access = snap::rhi::reflection::ImageAccess::Undefined;

            const auto& dstDSItr = dstDSIdToIdxMap.find(imageBinding.descriptorSet);
            if (dstDSItr == dstDSIdToIdxMap.end()) {
                snap::rhi::reflection::DescriptorSetInfo dstDSInfo{};
                dstDSInfo.binding = imageBinding.descriptorSet;
                dstDSInfo.storageTextures.push_back(info);
                dstDSIdToIdxMap[imageBinding.descriptorSet] = physicalReflection.size();
                physicalReflection.push_back(dstDSInfo);
            } else {
                snap::rhi::reflection::DescriptorSetInfo& dstDSInfo = physicalReflection[dstDSItr->second];
                dstDSInfo.storageTextures.push_back(info);
            }

            ImageInfo resInfo{};
            resInfo.unit = nativeImageInfo.unit;
            resInfo.access = GL_READ_WRITE;
            resInfo.logicalBinding = info.binding;
            result[imageBinding.descriptorSet].push_back(resInfo);
        }
    }

    for (auto& dsInfo : result) {
        if (!dsInfo.empty()) {
            std::sort(dsInfo.begin(), dsInfo.end(), [](const auto& l, const auto& r) {
                return l.logicalBinding < r.logicalBinding;
            });
        }
    }

    return result;
}

PipelineLegacyUBODescription buildLegacyUBODesc(
    const PipelineNativeInfo& pipelineNativeInfo,
    std::vector<snap::rhi::reflection::DescriptorSetInfo>& physicalReflection) {
    prepareDefaultDS(physicalReflection);
    if (!pipelineNativeInfo.legacyUBOInfo.uboInfo.size) {
        // skip default UBO
        return {};
    }

    physicalReflection[0].uniformBuffers.push_back(pipelineNativeInfo.legacyUBOInfo.uboInfo);

    PipelineLegacyUBODescription result{};
    result.uniforms = pipelineNativeInfo.legacyUBOInfo.uniforms;
    result.size = pipelineNativeInfo.legacyUBOInfo.uboInfo.size;
    return result;
}

uint32_t legacyTextureBindingToSamplerBinding(const uint32_t binding) {
    const uint32_t originalBinding = binding - LegacyTexturesBindingBase;
    return originalBinding + LegacySamplersBindingBase;
}
} // namespace snap::rhi::backend::opengl
