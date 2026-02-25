//
//  ShaderLibraryInfo.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 7/11/22.
//

#pragma once

#include "snap/rhi/reflection/Info.hpp"

#include <vector>

namespace snap::rhi::reflection {
struct ShaderLibraryInfo {
    std::vector<EntryPointInfo> entryPoints{};
};
} // namespace snap::rhi::reflection
