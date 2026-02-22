#include "snap/rhi/backend/opengl/PipelineCache.hpp"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"
#include <fstream>
#include <iostream>

#include "snap/rhi/common/HashCombine.h"
#include <snap/rhi/common/Scope.h>
#include <span>

namespace {
snap::rhi::backend::opengl::PipelineCacheStorage deserializeFromFile(const snap::rhi::PipelineCacheCreateInfo& info) {
    if (info.cachePath.empty() || !std::filesystem::exists(info.cachePath)) {
        return {};
    }

    const size_t fileSize = std::filesystem::file_size(info.cachePath);
    std::vector<uint8_t> serializedData(fileSize, 0);

    {
        std::ifstream fin(info.cachePath, std::ios::binary);
        SNAP_RHI_ON_SCOPE_EXIT {
            fin.close();
        };

        fin.read(reinterpret_cast<char*>(serializedData.data()), fileSize);
        assert(fin);
    }
    snap::rhi::backend::opengl::PipelineCacheStorage result = snap::rhi::backend::opengl::deserialize(serializedData);

    return result;
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
PipelineCache::PipelineCache(Device* device, const snap::rhi::PipelineCacheCreateInfo& info)
    : snap::rhi::PipelineCache(device, info), gl(device->getOpenGL()), cache(deserializeFromFile(info)) {}

PipelineCache::PipelineCache(Device* device, std::span<const uint8_t> serializedData)
    : snap::rhi::PipelineCache(device, {}),
      gl(device->getOpenGL()),
      cache(snap::rhi::backend::opengl::deserialize(serializedData)) {}

PipelineCacheValue PipelineCache::load(const snap::rhi::backend::opengl::hash64 key) const {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[PipelineCache][load]");

    auto func = [this](const snap::rhi::backend::opengl::hash64 key) {
        PipelineCacheValue result{};

        if (!cache.empty()) {
            if (key == 0) {
                const auto& itr = cache.begin();
                result.data = itr->second.second;
                result.binaryFormat = itr->second.first;
            } else {
                const auto& itr = cache.find(key);
                if (itr != cache.end()) {
                    result.data = itr->second.second;
                    result.binaryFormat = itr->second.first;
                }
            }
        }

        return result;
    };

    if ((createFlags & snap::rhi::PipelineCacheCreateFlags::ExternallySynchronized) !=
        snap::rhi::PipelineCacheCreateFlags::None) {
        return func(key);
    }

    std::lock_guard<std::mutex> lock(accessMutex);
    return func(key);
}

void PipelineCache::store(const snap::rhi::backend::opengl::hash64 key, const std::shared_ptr<ProgramState>& program) {
    if ((createFlags & snap::rhi::PipelineCacheCreateFlags::ExternallySynchronized) !=
        snap::rhi::PipelineCacheCreateFlags::None) {
        pipelines[key] = program;
    } else {
        std::lock_guard<std::mutex> lock(accessMutex);
        pipelines[key] = program;
    }
}

void PipelineCache::prepareCache() const {
    const auto& features = gl.getFeatures();
    if (!features.isProgramBinarySupported) {
        return;
    }

    for (const auto& [srcHash, shader] : pipelines) {
        GLuint programID = shader->getProgramID();
        if (programID == GL_NONE) {
            continue;
        }

        GLint binaryLength = 0;
        gl.getProgramiv(programID, GL_PROGRAM_BINARY_LENGTH, &binaryLength);

        if (binaryLength > 0) {
            GLint length = 0;
            GLenum binaryFormat = GL_NONE;

            std::vector<GLubyte> binaryData{};
            binaryData.resize(binaryLength);

            gl.getProgramBinary(programID, binaryLength, &length, &binaryFormat, binaryData.data());
            cache.try_emplace(srcHash, binaryFormat, std::move(binaryData));
        }
    }
    pipelines.clear();
}

Result PipelineCache::serializeToFile(const std::filesystem::path& cachePath) const {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[PipelineCache][serializeToFile]");
    auto func = [this](const std::filesystem::path& cachePath) {
        prepareCache();

        if (cache.empty()) {
            return Result::Success;
        }

        std::vector<uint8_t> serializedData{};
        const bool serializationResult = serialize(cache, serializedData);

        if (serializationResult) {
            const bool success = snap::rhi::backend::common::writeBinToFile(
                cachePath,
                std::span<const std::byte>(reinterpret_cast<const std::byte*>(serializedData.data()),
                                           serializedData.size()));
            return success ? Result::Success : Result::ErrorUnknown;
        }

        return Result::Incomplete;
    };

    if ((createFlags & snap::rhi::PipelineCacheCreateFlags::ExternallySynchronized) !=
        snap::rhi::PipelineCacheCreateFlags::None) {
        return func(cachePath);
    }

    std::lock_guard<std::mutex> lock(accessMutex);
    return func(cachePath);
}
} // namespace snap::rhi::backend::opengl
