#include "snap/rhi/backend/opengl/FramebufferPool.h"

#include "snap/rhi/backend/opengl/Device.hpp"
#include "snap/rhi/backend/opengl/DeviceContext.hpp"

#include "snap/rhi/common/HashCombine.h"
#include "snap/rhi/common/OS.h"
#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <snap/rhi/common/Platform.h>
#include <snap/rhi/common/Scope.h>

namespace {
size_t buildHash(const snap::rhi::backend::opengl::FramebufferAttachment& info) {
    size_t hash = 0;

    hash = snap::rhi::common::hash_combine(hash, info.texUUID);
    hash = snap::rhi::common::hash_combine(hash, info.target);
    hash = snap::rhi::common::hash_combine(hash, info.texId);
    hash = snap::rhi::common::hash_combine(hash, info.level);
    hash = snap::rhi::common::hash_combine(hash, info.firstLayer);
    hash = snap::rhi::common::hash_combine(hash, info.size.width);
    hash = snap::rhi::common::hash_combine(hash, info.size.height);
    hash = snap::rhi::common::hash_combine(hash, info.size.depth);
    hash = snap::rhi::common::hash_combine(hash, info.format);
    hash = snap::rhi::common::hash_combine(hash, info.sampleCount);
    hash = snap::rhi::common::hash_combine(hash, info.viewMask);
    return hash;
}

uint32_t getClearFBOPoolSize(snap::rhi::backend::opengl::DeviceContext* dc) {
    snap::rhi::Device* device = dc->getDevice();
    auto* glDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::opengl::Device>(device);
    if (!glDevice) {
        return 0;
    }

    snap::rhi::backend::opengl::GLFramebufferPoolOption fboPoolOptions = glDevice->getFramebufferPoolOptions();
    switch (fboPoolOptions) {
        case snap::rhi::backend::opengl::GLFramebufferPoolOption::FBOPool_64:
            return 64u;
        case snap::rhi::backend::opengl::GLFramebufferPoolOption::FBOPool_32:
            return 32u;
        case snap::rhi::backend::opengl::GLFramebufferPoolOption::FBOPool_16:
            return 16u;
        case snap::rhi::backend::opengl::GLFramebufferPoolOption::FBOPool_8:
            return 8u;

        default:
            return 0;
    }
}
} // unnamed namespace

namespace snap::rhi::backend::opengl {
FramebufferPool::FramebufferPool(DeviceContext* dc) noexcept
    : validationLayer(dc->getValidationLayer()),
      dc(dc),
      gl(dc->getOpenGL()),
      ClearFBOPoolSize(getClearFBOPoolSize(dc)) {}

FramebufferPool::~FramebufferPool() noexcept {
    assert(fbos.empty());
}

FramebufferPool::Key::Key(const FramebufferDescription& description) : description(description) {
    hash = snap::rhi::common::hash_combine(0xcc9e2d51, description.numColorAttachments);
    for (uint32_t i = 0; i < description.numColorAttachments; ++i) {
        hash = snap::rhi::common::hash_combine(hash, buildHash(description.colorAttachments[i]));
    }
    if (description.depthStencilAttachment.texId != snap::rhi::backend::opengl::TextureId::Null) {
        hash = snap::rhi::common::hash_combine(hash, buildHash(description.depthStencilAttachment));
    }
}

FramebufferPool::Value::Value(DeviceContext* dc) : useCount(1), fbo(std::make_unique<FBO>(dc)) {}

FramebufferId FramebufferPool::bindFramebuffer(FramebufferTarget target, const FramebufferDescription& description) {
    Key key{description};

    auto itr = std::find_if(fbos.begin(), fbos.end(), [&key](const auto& info) { return info.first == key; });
    if (itr == fbos.end()) {
        auto& res = fbos.emplace_back(key, dc);
        return res.second.fbo->assignAndBind(target, description);
    }

    auto& value = itr->second;
    ++value.useCount;
    return value.fbo->bind(target);
}

void FramebufferPool::freeUnusedFBOs() {
    if (fbos.size() > ClearFBOPoolSize) {
        std::sort(fbos.begin(), fbos.end(), [](const auto& l, const auto& r) {
            return l.second.useCount > r.second.useCount;
        });

        while (fbos.size() > ClearFBOPoolSize) {
            fbos.pop_back();
        }
    }
}

void FramebufferPool::clear() {
    fbos.clear();
}
} // namespace snap::rhi::backend::opengl
