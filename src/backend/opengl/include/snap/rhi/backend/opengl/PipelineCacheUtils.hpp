// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include "snap/rhi/Enums.h"
#include "snap/rhi/backend/opengl/HashUtils.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"

#include <span>
#include <unordered_map>
#include <variant>

namespace snap::rhi::backend::opengl {
using PipelineCacheStorage =
    std::unordered_map<snap::rhi::backend::opengl::hash64, std::pair<GLenum, std::vector<GLubyte>>>;

PipelineCacheStorage deserialize(std::span<const uint8_t> storage);
bool serialize(const PipelineCacheStorage& src, std::vector<uint8_t>& dst);
} // namespace snap::rhi::backend::opengl
