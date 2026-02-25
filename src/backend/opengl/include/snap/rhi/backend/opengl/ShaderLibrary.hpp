//
//  ShaderLibrary.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 08.02.2022.
//

#pragma once

#include "snap/rhi/ShaderLibrary.hpp"
#include "snap/rhi/backend/opengl/OpenGL.h"

#include <string>
#include <string_view>

namespace snap::rhi::backend::opengl {
class Device;

class ShaderLibrary final : public snap::rhi::ShaderLibrary {
public:
    explicit ShaderLibrary(Device* glesDevice, const snap::rhi::ShaderLibraryCreateInfo& info);
    ~ShaderLibrary() override = default;

    const std::string& getSource() const;

private:
    std::string src{};
};
} // namespace snap::rhi::backend::opengl
