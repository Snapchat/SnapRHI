//
//  ShaderModuleInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 31.03.2022.
//

#pragma once

#include "snap/rhi/reflection/Info.hpp"

#include <vector>

namespace snap::rhi::reflection {
/**
 * Description of all possible attributes/uniforms, only after compiling the program unused attributes/uniforms will be
 * removed.
 */
struct ShaderModuleInfo {
    std::vector<VertexAttributeInfo> vertexAttributes;
};
} // namespace snap::rhi::reflection
