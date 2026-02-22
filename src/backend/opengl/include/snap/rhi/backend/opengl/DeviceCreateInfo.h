#pragma once

#include "snap/rhi/DeviceCreateInfo.h"
#include "snap/rhi/EnumOps.h"
#include "snap/rhi/backend/opengl/DeviceContextCreateInfo.h"
#include <OpenGL/runtime.h>

namespace snap::rhi::backend::opengl {
constexpr bool isApiVersionGLES(gl::APIVersion version) {
    return version >= gl::APIVersion::GL_ES_START && version <= gl::APIVersion::GL_ES_END;
}

enum class GLFramebufferPoolOption {
    None = 0,

    FBOPool_64,
    FBOPool_32,
    FBOPool_16,
    FBOPool_8,
};
SNAP_RHI_DEFINE_ENUM_OPS(GLFramebufferPoolOption);

enum class GLCommandQueueErrorCheckOptions {
    EnableAllGLErrorChecks = 0,
    DisableAllGLErrorChecks,
};

enum class GLPipelineStatusCheckOptions { OnCreation = 0, OnFirstUsage };

enum class FramebufferDetachOnScopeExitOptions { DoNotDetach = 0, AlwaysDetach };

enum class TextureBlitOptions {
    // Blit approach is determined by implementation.
    Default = 0,

    // Forces usage of glBlitFramebuffer fot texute blit if supported by device
    // or fallbacks to other best available approach.
    BlitFramebuffer,

    // Forces usage of copyImageSubData fot texute blit if supported by device
    // or fallbacks to other best available approach.
    CopyImageSubData,

    // Forces usage of glCopyTexSubImage2D/3D() which is supported on all devices.
    CopyTexSubImage,
};

enum class LoadOpDontCareBehavior : uint32_t { DoNothing = 0, Clear, Discard };

enum class ResolveOpBehavior : uint32_t { DoNothing = 0, FlushBeforeResolve };

enum class GLPBOOptions {
    None = 0,
    AllowNativePboAsTransferSrc = 1 << 0,
};
SNAP_RHI_DEFINE_ENUM_OPS(GLPBOOptions);

enum class DeviceResourcesOptions {
    None = 0,

    CreateResourcesLazy = 1 << 0,
    DestroyResourcesLazy = 1 << 1,
};
SNAP_RHI_DEFINE_ENUM_OPS(DeviceResourcesOptions);

struct DeviceCreateInfo {
    snap::rhi::DeviceCreateInfo deviceInfo;
    snap::rhi::backend::opengl::DeviceContextCreateInfo dcCreateInfo;

    gl::APIVersion apiVersion = gl::APIVersion::Latest;
    gl::Context glContext = nullptr;

    snap::rhi::backend::opengl::DeviceResourcesOptions deviceResourcesAllocateOption =
        snap::rhi::backend::opengl::DeviceResourcesOptions::None;
    snap::rhi::backend::opengl::GLFramebufferPoolOption framebufferPoolOptions =
        snap::rhi::backend::opengl::GLFramebufferPoolOption::None;
    snap::rhi::backend::opengl::GLCommandQueueErrorCheckOptions commandQueueErrorCheckOptions =
        snap::rhi::backend::opengl::GLCommandQueueErrorCheckOptions::EnableAllGLErrorChecks;
    snap::rhi::backend::opengl::GLPBOOptions glPBOOptions = snap::rhi::backend::opengl::GLPBOOptions::None;
    snap::rhi::backend::opengl::GLPipelineStatusCheckOptions glPipelineStatusCheckOptions =
        snap::rhi::backend::opengl::GLPipelineStatusCheckOptions::OnCreation;
    snap::rhi::backend::opengl::FramebufferDetachOnScopeExitOptions framebufferDetachOnScopeExitOptions =
        snap::rhi::backend::opengl::FramebufferDetachOnScopeExitOptions::DoNotDetach;
    snap::rhi::backend::opengl::LoadOpDontCareBehavior loadOpDontCareBehavior =
        snap::rhi::backend::opengl::LoadOpDontCareBehavior::DoNothing;
    snap::rhi::backend::opengl::ResolveOpBehavior resolveOpBehavior =
        snap::rhi::backend::opengl::ResolveOpBehavior::DoNothing;
    snap::rhi::backend::opengl::TextureBlitOptions textureBlitOptions =
        snap::rhi::backend::opengl::TextureBlitOptions::Default;

    std::string fboTileMemoryIssueGPUs;
};
} // namespace snap::rhi::backend::opengl
