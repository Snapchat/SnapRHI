//
//  ShaderLibrary.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 1/24/20.
//  Copyright © 2020 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/ShaderLibrary.hpp"

#include "snap/rhi/reflection/ShaderModuleInfo.h"

#include <Metal/Metal.h>
#include <string>

namespace snap::rhi::backend::metal {
class Device;
class ShaderModule;

class ShaderLibrary final : public snap::rhi::ShaderLibrary {
public:
    ShaderLibrary(Device* mtlDevice, const snap::rhi::ShaderLibraryCreateInfo& info);
    ~ShaderLibrary() override = default;

    const id<MTLLibrary>& getMtlLibrary() const;

    void setDebugLabel(std::string_view label) override;

private:
    id<MTLLibrary> library = nil;
};
} // namespace snap::rhi::backend::metal
