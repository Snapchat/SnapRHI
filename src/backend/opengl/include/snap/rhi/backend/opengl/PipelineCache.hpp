//
//  PipelineCache.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 6/13/22.
//

#pragma once

#include "snap/rhi/Enums.h"

#include "snap/rhi/PipelineCache.hpp"
#include "snap/rhi/backend/opengl/HashUtils.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/backend/opengl/PipelineCacheUtils.hpp"
#include "snap/rhi/backend/opengl/Profile.hpp"
#include "snap/rhi/backend/opengl/ProgramState.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace snap::rhi::backend::opengl {
class Device;

struct PipelineCacheValue {
    GLenum binaryFormat = GL_NONE;
    std::span<const GLubyte> data{};
};

// https://www.khronos.org/opengl/wiki/Shader_Compilation
class PipelineCache final : public snap::rhi::PipelineCache {
public:
    explicit PipelineCache(Device* device, const snap::rhi::PipelineCacheCreateInfo& info);
    explicit PipelineCache(Device* device, std::span<const uint8_t> serializedData);
    ~PipelineCache() override = default;

    Result serializeToFile(const std::filesystem::path& cachePath) const override;

    PipelineCacheValue load(const snap::rhi::backend::opengl::hash64 key) const;
    void store(const snap::rhi::backend::opengl::hash64 key, const std::shared_ptr<ProgramState>& program);

private:
    void prepareCache() const;

    const Profile& gl;
    mutable std::mutex accessMutex;

    mutable std::unordered_map<snap::rhi::backend::opengl::hash64, std::shared_ptr<ProgramState>> pipelines;
    mutable PipelineCacheStorage cache;
};
} // namespace snap::rhi::backend::opengl
