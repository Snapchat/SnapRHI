// Copyright 2017 The Dawn Authors
// Modifications Copyright 2026 Snap Inc.
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

#include <cassert>
#include <cstdint>
#include <vector>

namespace snap::rhi::backend::opengl {
class CommandAllocator;

class CommandIterator final {
public:
    explicit CommandIterator(CommandAllocator& allocator);

    template<typename E>
    bool nextCommandId(E* commandId) {
        static_assert(sizeof(E) == sizeof(uint32_t), "invalid command type size");
        static_assert(alignof(E) == alignof(uint32_t), "invalid command type alignment");

        return nextCommandId(reinterpret_cast<uint32_t*>(commandId));
    }

    template<typename T>
    const T* nextCommand() {
        return static_cast<const T*>(nextCommand(sizeof(T), alignof(T)));
    }

    void rewind();

private:
    bool nextCommandId(uint32_t* commandId);
    const void* nextCommand(size_t commandSize, size_t commandAlignment);

    const uint8_t* currentPtr = nullptr;
    const CommandAllocator& allocator;
};
} // namespace snap::rhi::backend::opengl
