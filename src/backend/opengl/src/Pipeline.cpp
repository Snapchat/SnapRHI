#include "snap/rhi/backend/opengl/Pipeline.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/opengl/AttribsUtils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/ErrorCheckGuard.hpp"
#include "snap/rhi/backend/opengl/PipelineCache.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/ProgramUtils.hpp"
#include "snap/rhi/backend/opengl/ShaderModule.hpp"
#include "snap/rhi/backend/opengl/UniformUtils.hpp"
#include "snap/rhi/common/Throw.h"

namespace {
snap::rhi::backend::opengl::hash64 buildPipelineSrcHash(const std::span<snap::rhi::ShaderModule*>& shaderStages) {
    snap::rhi::backend::opengl::hash64 result = 0;

    for (const auto& shaderModule : shaderStages) {
        if (shaderModule) {
            auto* glShaderModule =
                snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::ShaderModule>(shaderModule);

            result = snap::rhi::backend::opengl::singleHashCombine(result, glShaderModule->getShaderSrcHash());
        }
    }

    return result;
}

template<typename T>
const T* toVec3(const T* data, GLsizei count, std::vector<uint32_t>& dataAlignmentCache) {
    static_assert(std::is_same<GLfloat, T>() || std::is_same<GLint, T>() || std::is_same<GLuint, T>(),
                  "Wrong typename");

    size_t storageSize = count * 3 * sizeof(T);
    if (dataAlignmentCache.size() * sizeof(uint32_t) < storageSize) {
        size_t alignedSize = ((storageSize + sizeof(uint32_t) - 1) / sizeof(uint32_t)) * sizeof(uint32_t);
        dataAlignmentCache.resize(alignedSize);
    }

    for (GLsizei i = 0; i < count; ++i) {
        T* dstPtr = reinterpret_cast<T*>(dataAlignmentCache.data()) + i * 3;
        memcpy(dstPtr, data + i * 4, 3 * sizeof(T));
    }
    return reinterpret_cast<T*>(dataAlignmentCache.data());
}

template<typename T>
const T* toMat3(const T* data, GLsizei count, std::vector<uint32_t>& dataAlignmentCache) {
    static_assert(std::is_same<GLfloat, T>() || std::is_same<GLint, T>(), "Wrong typename");

    size_t storageSize = count * 9 * sizeof(T);
    if (dataAlignmentCache.size() * sizeof(uint32_t) < storageSize) {
        size_t alignedSize = ((storageSize + sizeof(uint32_t) - 1) / sizeof(uint32_t)) * sizeof(uint32_t);
        dataAlignmentCache.resize(alignedSize);
    }

    // For UBO, We will add padding into mat3 to become mat3x4. The total number is 12 not 16
    for (GLsizei i = 0; i < count; ++i) {
        T* dstPtr = reinterpret_cast<T*>(dataAlignmentCache.data()) + i * 9 + 0;
        memcpy(dstPtr, data + i * 12 + 0, 3 * sizeof(T)); // first column

        dstPtr = reinterpret_cast<T*>(dataAlignmentCache.data()) + i * 9 + 3;
        memcpy(dstPtr, data + i * 12 + 4, 3 * sizeof(T)); // second column

        dstPtr = reinterpret_cast<T*>(dataAlignmentCache.data()) + i * 9 + 6;
        memcpy(dstPtr, data + i * 12 + 8, 3 * sizeof(T)); // third column
    }
    return reinterpret_cast<T*>(dataAlignmentCache.data());
}

void setUniform(const snap::rhi::backend::opengl::Profile& gl,
                const GLint location,
                const GLenum type,
                const uint8_t* data,
                GLsizei count,
                std::vector<uint32_t>& dataAlignmentCache) {
    // glGetUniformLocation returns -1 for inactive/optimized-out uniforms.
    if (location == snap::rhi::backend::opengl::InvalidLocation) {
        // The glGetUniformLocation returns -1 if name does not correspond to anactive uniform variable in program
        return;
    }

    switch (type) {
        // SnapRHI doesn't supported bool uniform, since from OpenGL side bool is int(4-byte), but for Metal bool is
        // bool(1 byte) SnapRHI suggests that int should be used instead of bool. So SnapRHI assumed that user fill UBO,
        // with bool like int.
        case GL_BOOL: {
            gl.uniform1iv(location, count, reinterpret_cast<const GLint*>(data));
        } break;
        case GL_BOOL_VEC2: {
            gl.uniform2iv(location, count, reinterpret_cast<const GLint*>(data));
        } break;
        case GL_BOOL_VEC3: {
            const GLint* ptr = toVec3(reinterpret_cast<const GLint*>(data), count, dataAlignmentCache);
            gl.uniform3iv(location, count, ptr);
        } break;
        case GL_BOOL_VEC4: {
            gl.uniform4iv(location, count, reinterpret_cast<const GLint*>(data));
        } break;

        case GL_FLOAT: {
            gl.uniform1fv(location, count, reinterpret_cast<const GLfloat*>(data));
        } break;
        case GL_FLOAT_VEC2: {
            gl.uniform2fv(location, count, reinterpret_cast<const GLfloat*>(data));
        } break;
        case GL_FLOAT_VEC3: {
            const GLfloat* ptr = toVec3(reinterpret_cast<const GLfloat*>(data), count, dataAlignmentCache);
            gl.uniform3fv(location, count, ptr);
        } break;
        case GL_FLOAT_VEC4: {
            gl.uniform4fv(location, count, reinterpret_cast<const GLfloat*>(data));
        } break;

        case GL_INT: {
            gl.uniform1iv(location, count, reinterpret_cast<const GLint*>(data));
        } break;
        case GL_INT_VEC2: {
            gl.uniform2iv(location, count, reinterpret_cast<const GLint*>(data));
        } break;
        case GL_INT_VEC3: {
            const GLint* ptr = toVec3(reinterpret_cast<const GLint*>(data), count, dataAlignmentCache);
            gl.uniform3iv(location, count, ptr);
        } break;
        case GL_INT_VEC4: {
            gl.uniform4iv(location, count, reinterpret_cast<const GLint*>(data));
        } break;

        case GL_UNSIGNED_INT: {
            gl.uniform1uiv(location, count, reinterpret_cast<const GLuint*>(data));
        } break;
        case GL_UNSIGNED_INT_VEC2: {
            gl.uniform2uiv(location, count, reinterpret_cast<const GLuint*>(data));
        } break;
        case GL_UNSIGNED_INT_VEC3: {
            const GLuint* ptr = toVec3(reinterpret_cast<const GLuint*>(data), count, dataAlignmentCache);
            gl.uniform3uiv(location, count, ptr);
        } break;
        case GL_UNSIGNED_INT_VEC4: {
            gl.uniform4uiv(location, count, reinterpret_cast<const GLuint*>(data));
        } break;

        case GL_FLOAT_MAT2: {
            gl.uniformMatrix2fv(location, count, GL_FALSE, reinterpret_cast<const GLfloat*>(data));
        } break;

        case GL_FLOAT_MAT3: {
            const GLfloat* ptr = toMat3(reinterpret_cast<const GLfloat*>(data), count, dataAlignmentCache);
            gl.uniformMatrix3fv(location, count, GL_FALSE, ptr);
        } break;
        case GL_FLOAT_MAT4: {
            gl.uniformMatrix4fv(location, count, GL_FALSE, reinterpret_cast<const GLfloat*>(data));
        } break;

        default:
            snap::rhi::common::throwException("invalid uniform type");
    }
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
Pipeline::Pipeline(Device* device,
                   const snap::rhi::PipelineCreateFlags pipelineCreateFlags,
                   const bool doesShaderContainUniformsOnly,
                   const PipelineConfigurationInfo& pipelineInfo,
                   const std::span<snap::rhi::ShaderModule*>& shaderStages,
                   const Pipeline* basePipeline,
                   snap::rhi::PipelineCache* pipelineCache)
    : gl(device->getOpenGL()),
      device(device),
      pipelineInfo(pipelineInfo),
      pipelineSrcHash(buildPipelineSrcHash(shaderStages)),
      isLegacyFlag(doesShaderContainUniformsOnly),
      pipelineCreateFlags(pipelineCreateFlags) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[Pipeline] creation");

    if (basePipeline && basePipeline->pipelineSrcHash == pipelineSrcHash) {
        programState = basePipeline->programState;
        isLegacyFlag = basePipeline->isLegacyFlag;
    }

    init(shaderStages, pipelineCache);
}

Pipeline::Pipeline(Device* device, const bool isLegacy, GLuint programID)
    : gl(device->getOpenGL()),
      device(device),
      pipelineInfo(RenderPipelineConfigurationInfo{}),
      pipelineSrcHash(0),
      isLegacyFlag(isLegacy) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[Pipeline] legacy creation from cache");
    SNAP_RHI_VALIDATE(device->getValidationLayer(),
                      programID != GL_NONE,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::RenderPipelineOp,
                      "[snap::rhi::backend::opengl::Pipeline] Cannot create render pipeline from cache");
    this->programState = std::make_shared<ProgramState>(gl, programID, true, true, isLegacyFlag);
}

Pipeline::~Pipeline() {
    try {
        auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);

        SNAP_RHI_REPORT(device->getValidationLayer(),
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~Pipeline] start destruction");
        SNAP_RHI_VALIDATE(device->getValidationLayer(),
                          device->getCurrentDeviceContext(),
                          snap::rhi::ReportLevel::Warning,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~Pipeline] DeviceContext isn't attached to thread");
        SNAP_RHI_VALIDATE(device->getValidationLayer(),
                          glDevice->isNativeContextAttached(),
                          snap::rhi::ReportLevel::CriticalError,
                          snap::rhi::ValidationTag::DestroyOp,
                          "[snap::rhi::backend::opengl::~Pipeline] GLES context isn't attached to thread");
        programState.reset();
        SNAP_RHI_REPORT(device->getValidationLayer(),
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::DestroyOp,
                        "[snap::rhi::backend::opengl::~RenderProgram] end of destruction");
    } catch (const snap::rhi::common::Exception& e) {
        SNAP_RHI_LOGE("[snap::rhi::backend::opengl::Pipeline::~Pipeline] Caught: %s, (possible resource leak).",
                      e.what());
    } catch (...) {
        SNAP_RHI_LOGE(
            "[snap::rhi::backend::opengl::Pipeline::~Pipeline] Caught unexpected error (possible resource leak).");
    }
}

void Pipeline::init(const std::span<snap::rhi::ShaderModule*>& shaderStages, snap::rhi::PipelineCache* pipelineCache) {
    if (!device->areResourcesLazyAllocationsEnabled()) {
        tryToCompilePipeline(shaderStages, pipelineCache);
    } else {
        if (pipelineCache) {
            lazyInitRetainedRefs.pipelineCacheRef = device->resolveResource(pipelineCache);
        }

        for (const auto& shader : shaderStages) {
            if (shader) {
                lazyInitRetainedRefs.shaderStagesRefs.push_back(device->resolveResource(shader));
            }
        }
    }
}

void Pipeline::tryToCompilePipeline(const std::span<snap::rhi::ShaderModule*>& shaderStages,
                                    snap::rhi::PipelineCache* pipelineCache) const {
    if (programState) {
        return;
    }

    GLuint programID = tryLoadFromCache(gl, pipelineCache, pipelineSrcHash);
    std::string errorMessage;

    if (programID == GL_NONE) {
        GLuint createdProgramID = snap::rhi::backend::opengl::createProgramFromShaders(gl, shaderStages, pipelineCache);
        std::variant<GLuint, std::string> program;

        if (common::smart_cast<Device>(gl.getDevice())->shouldValidatePipelineOnCreation()) {
            program = validateProgramLinkStatus(gl, createdProgramID, shaderStages);
        } else {
            program = createdProgramID;
        }

        const GLuint* programIDPtr = std::get_if<GLuint>(&program);
        const std::string* programErrorMessagePtr = std::get_if<std::string>(&program);

        if ((programIDPtr != nullptr) && (*programIDPtr != GL_NONE)) {
            programID = *programIDPtr;
        } else if (programErrorMessagePtr != nullptr) {
            errorMessage = *programErrorMessagePtr;
        }
    }

    this->programState = std::make_shared<ProgramState>(
        gl,
        programID,
        common::smart_cast<Device>(gl.getDevice())->shouldValidatePipelineOnCreation(),
        static_cast<bool>(pipelineCreateFlags & snap::rhi::PipelineCreateFlags::AcquireNativeReflection),
        isLegacyFlag);

    if (pipelineCache && (programID != GL_NONE)) {
        SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[Pipeline][storeCache]");
        auto* glPipelineCache =
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::PipelineCache>(pipelineCache);
        glPipelineCache->store(pipelineSrcHash, programState);
    }

    SNAP_RHI_VALIDATE(device->getValidationLayer(),
                      programID != GL_NONE,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::GLProgramValidationOp,
                      "[tryToCompilePipeline] Pipeline compile log: %s\n",
                      errorMessage.c_str());

    lazyInitRetainedRefs.release();
}

void Pipeline::tryToCompilePipeline() const {
    if (programState) {
        return;
    }

    snap::rhi::PipelineCache* pipelineCache = nullptr;
    if (lazyInitRetainedRefs.pipelineCacheRef) {
        pipelineCache = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::PipelineCache>(
            lazyInitRetainedRefs.pipelineCacheRef.get());
    }

    std::vector<snap::rhi::ShaderModule*> shaderStages{};
    for (const auto& shader : lazyInitRetainedRefs.shaderStagesRefs) {
        shaderStages.push_back(
            snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::ShaderModule>(shader.get()));
    }

    tryToCompilePipeline(shaderStages, pipelineCache);
}

void Pipeline::syncImpl() const {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(gl.getDevice(), "[Pipeline][syncImpl]");

    const GLuint programID = programState->getProgramID();
    if (programID == GL_NONE) {
        return;
    }

    programState->initPipelineInfo(pipelineInfo);
    const auto& nativeInfo = programState->getPipelineNativeInfo();

    const snap::rhi::opengl::PipelineInfo* pipelineInfoPtr = nullptr;
    const RenderPipelineConfigurationInfo* renderPipelineConfigInfoPtr = nullptr;

    if (const auto* computePipelineInfoPtr = std::get_if<ComputePipelineConfigurationInfo>(&pipelineInfo)) {
        pipelineInfoPtr = computePipelineInfoPtr->computePipelineInfo ?
                              &computePipelineInfoPtr->computePipelineInfo.value() :
                              nullptr;
    } else if (const auto* renderPipelineInfoPtr = std::get_if<RenderPipelineConfigurationInfo>(&pipelineInfo)) {
        renderPipelineConfigInfoPtr = renderPipelineInfoPtr;
        pipelineInfoPtr =
            renderPipelineInfoPtr->renderPipelineInfo ? &renderPipelineInfoPtr->renderPipelineInfo.value() : nullptr;
    } else {
        snap::rhi::common::throwException("[Pipeline::syncImpl] invalid Pipeline configuration info");
    }

    { // Initialize Pipeline resources descriptions(logical to physical bindings info)
        ssbo = buildSSBODesc(nativeInfo, pipelineInfoPtr, descriptorSetInfos);
        image = buildImageDesc(nativeInfo, pipelineInfoPtr, descriptorSetInfos);

        texture = buildTextureDesc(nativeInfo, pipelineInfoPtr, descriptorSetInfos);

        if (renderPipelineConfigInfoPtr) {
            sampler = buildSamplerDesc(nativeInfo, *renderPipelineConfigInfoPtr, descriptorSetInfos, texture);
        }

        switch (nativeInfo.pipelineUniformManagmentType) {
            case PipelineUniformManagmentType::Default: {
                ubo = buildUBODesc(nativeInfo, pipelineInfoPtr, descriptorSetInfos);
            } break;

            case PipelineUniformManagmentType::Compatible: {
                compatibleUBO = buildCompatibleUBODesc(nativeInfo, pipelineInfoPtr, descriptorSetInfos);
            } break;

            default: {
                legacyDescription = buildLegacyUBODesc(nativeInfo, descriptorSetInfos);
            } break;
        }
    }

    if (renderPipelineConfigInfoPtr) {
        vertexDescription = buildVertexDescriptor(
            nativeInfo, *renderPipelineConfigInfoPtr, vertexAttributeInfos, gl.getDevice()->getValidationLayer());
    }
}

void Pipeline::bindLegacyUBO(DeviceContext* dc, std::span<const uint8_t> data) const {
    sync();

    /**
     * SnapRHI does not support legacy render pipeline for multiple threads !!!
     */

    assert(dc);
    std::vector<uint32_t>& dataAlignmentCache = dc->getDataAlignmentCache();
    auto& uniformsCache = programState->getLegacyUBOState(dc->getDeviceContextUUID());
    const auto& uniformsInfo = legacyDescription.uniforms;

    assert(data.size() >= legacyDescription.size);

    if (uniformsCache.empty()) {
        uniformsCache.assign(data.begin(), data.begin() + legacyDescription.size);

        for (const auto& info : uniformsInfo) {
            const uint8_t* dataPtr = data.data() + info.uboOffset;
            setUniform(gl, info.location, info.type, dataPtr, info.arraySize, dataAlignmentCache);
        }
    } else {
        for (const auto& info : uniformsInfo) {
            assert((info.byteSize & 3) == 0);

            uint8_t* cache = uniformsCache.data() + info.uboOffset;
            const uint8_t* value = data.data() + info.uboOffset;
            const uint32_t size = info.byteSize;

            if (memcmp(cache, value, size)) {
                setUniform(gl, info.location, info.type, value, info.arraySize, dataAlignmentCache);
                memcpy(cache, value, size);
            }
        }
    }
}

void Pipeline::useProgram(snap::rhi::backend::opengl::DeviceContext* dc) const {
    sync();

    assert(programState);
    return programState->useProgram(dc);
}
} // namespace snap::rhi::backend::opengl
