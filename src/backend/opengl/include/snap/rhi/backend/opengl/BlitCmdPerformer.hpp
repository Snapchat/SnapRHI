//
//  BlitCmdPerformer.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 15.02.2022.
//

#pragma once

#include "snap/rhi/backend/opengl/CmdPerformer.hpp"
#include "snap/rhi/backend/opengl/Commands.h"
#include "snap/rhi/backend/opengl/OpenGL.h"
#include "snap/rhi/common/NonCopyable.h"

namespace snap::rhi::backend::opengl {
class Device;
class Profile;
} // namespace snap::rhi::backend::opengl

namespace snap::rhi::backend::opengl {
class Device;

class BlitCmdPerformer final : public CmdPerformer {
public:
    BlitCmdPerformer(snap::rhi::backend::opengl::Device* device);
    ~BlitCmdPerformer() = default;

    void copyBuffer(const snap::rhi::backend::opengl::CopyBufferToBufferCmd& cmd);
    void copyBufferToTexture(const snap::rhi::backend::opengl::CopyBufferToTextureCmd& cmd);
    void copyTextureToBuffer(const snap::rhi::backend::opengl::CopyTextureToBufferCmd& cmd);
    void copyTexture(const snap::rhi::backend::opengl::CopyTextureToTextureCmd& cmd);
    void generateMipmap(const snap::rhi::backend::opengl::GenerateMipmapCmd& cmd);

    void reset();

private:
};
} // namespace snap::rhi::backend::opengl
