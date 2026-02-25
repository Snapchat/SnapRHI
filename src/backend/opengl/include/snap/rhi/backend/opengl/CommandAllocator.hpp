// Copyright 2017 The Dawn Authors
// Modifications Copyright 2025 Snap Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "snap/rhi/backend/opengl/Commands.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

namespace snap::rhi::backend::opengl {
class CommandAllocator final {
    friend class CommandIterator;

public:
    CommandAllocator();
    ~CommandAllocator() = default;

    CommandAllocator(const CommandAllocator& other) = delete;
    CommandAllocator& operator=(const CommandAllocator& other) = delete;

    CommandAllocator(CommandAllocator&& other) = default;
    CommandAllocator& operator=(CommandAllocator&& other) = default;

    template<typename T>
    T* allocateCommand() = delete;

    void finish();
    void reset();

private:
    template<typename T, typename E>
    T* allocate(E commandId) {
        assert(isFinished == false);
        static_assert(sizeof(E) == sizeof(uint32_t), "invalid command type size");
        static_assert(alignof(E) == alignof(uint32_t), "invalid command type alignment");
        static_assert(alignof(T) <= MaxSupportedAlignment, "invalid command alignment");
        T* result = reinterpret_cast<T*>(allocate(static_cast<uint32_t>(commandId), sizeof(T), alignof(T)));
        if (!result) {
            return nullptr;
        }
        new (result) T;
        return result;
    }

    uint8_t* allocate(uint32_t commandId, size_t commandSize, size_t commandAlignment);
    uint8_t* allocateInNewMemory(uint32_t commandId, size_t commandSize, size_t commandAlignment);
    void addMemory(size_t minimumSize);

    static constexpr uint32_t EndOfBlock = std::numeric_limits<uint32_t>::max(); // identify end of command list
    static constexpr size_t MaxSupportedAlignment = std::max(size_t{8}, alignof(InvokeCustomCallbackCmd));
    static constexpr size_t MaxAllocationSize = 16384;
    static constexpr size_t WorstCaseAdditionalSize =
        sizeof(uint32_t) + MaxSupportedAlignment + alignof(uint32_t) + sizeof(uint32_t);

    size_t lastAllocationSize = 2048;
    uint8_t* currentPtr = nullptr;
    uint8_t* endPtr = nullptr;
    bool isFinished = false;

    std::vector<uint8_t> data{};
};

#define SNAP_RHI_DECLARE_COMMAND(CommandName)                                                                          \
    template<>                                                                                                         \
    inline CommandName##Cmd* CommandAllocator::allocateCommand<CommandName##Cmd>() {                                   \
        return allocate<CommandName##Cmd, snap::rhi::backend::opengl::Command>(Command::CommandName);                  \
    }

SNAP_RHI_DECLARE_COMMAND(BeginComputePass)
SNAP_RHI_DECLARE_COMMAND(BindComputePipeline)
SNAP_RHI_DECLARE_COMMAND(Dispatch)
SNAP_RHI_DECLARE_COMMAND(EndComputePass)

SNAP_RHI_DECLARE_COMMAND(BeginRenderPass)
SNAP_RHI_DECLARE_COMMAND(BeginRenderPass1)
SNAP_RHI_DECLARE_COMMAND(SetRenderPipeline)
SNAP_RHI_DECLARE_COMMAND(SetViewport)
SNAP_RHI_DECLARE_COMMAND(BindDescriptorSet)
SNAP_RHI_DECLARE_COMMAND(SetVertexBuffers)
SNAP_RHI_DECLARE_COMMAND(SetVertexBuffer)
SNAP_RHI_DECLARE_COMMAND(SetIndexBuffer)
SNAP_RHI_DECLARE_COMMAND(SetDepthBias)
SNAP_RHI_DECLARE_COMMAND(SetStencilReference)
SNAP_RHI_DECLARE_COMMAND(SetBlendConstants)
SNAP_RHI_DECLARE_COMMAND(Draw)
SNAP_RHI_DECLARE_COMMAND(DrawIndexed)
SNAP_RHI_DECLARE_COMMAND(InvokeCustomCallback)
SNAP_RHI_DECLARE_COMMAND(PipelineBarrier)
SNAP_RHI_DECLARE_COMMAND(EndRenderPass)

SNAP_RHI_DECLARE_COMMAND(BeginBlitPass)
SNAP_RHI_DECLARE_COMMAND(CopyBufferToBuffer)
SNAP_RHI_DECLARE_COMMAND(CopyBufferToTexture)
SNAP_RHI_DECLARE_COMMAND(CopyTextureToBuffer)
SNAP_RHI_DECLARE_COMMAND(CopyTextureToTexture)
SNAP_RHI_DECLARE_COMMAND(GenerateMipmap)
SNAP_RHI_DECLARE_COMMAND(EndBlitPass)

SNAP_RHI_DECLARE_COMMAND(BeginDebugGroup)
SNAP_RHI_DECLARE_COMMAND(EndDebugGroup)

#undef SNAP_RHI_DECLARE_COMMAND

} // namespace snap::rhi::backend::opengl
