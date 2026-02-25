//
//  ShaderModule.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 1/24/20.
//  Copyright © 2020 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/ShaderModule.hpp"
#include <Metal/Metal.h>

#include <future>

namespace snap::rhi {
class ShaderLibrary;
} // namespace snap::rhi

namespace snap::rhi::backend::metal {
class Device;

class ShaderModule final : public snap::rhi::ShaderModule {
public:
    explicit ShaderModule(Device* mtlDevice, const snap::rhi::ShaderModuleCreateInfo& info);
    ~ShaderModule() override;

    const std::optional<reflection::ShaderModuleInfo>& getReflectionInfo() const override;
    const id<MTLFunction>& getFunction() const;

private:
    void sync() const;

    mutable std::future<void> asyncFuture;
    std::promise<void> asyncPromise;

    void setDebugLabel(std::string_view label) override;

private:
    id<MTLFunction> function = nil;
};
} // namespace snap::rhi::backend::metal
