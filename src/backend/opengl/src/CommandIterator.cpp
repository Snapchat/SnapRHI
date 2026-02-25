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

#include "snap/rhi/backend/opengl/CommandIterator.hpp"
#include "snap/rhi/backend/common/Math.hpp"
#include "snap/rhi/backend/opengl/CommandAllocator.hpp"
#include "snap/rhi/common/Throw.h"

namespace snap::rhi::backend::opengl {
CommandIterator::CommandIterator(CommandAllocator& allocator) : allocator(allocator) {
    if (!allocator.isFinished) {
        snap::rhi::common::throwException("[CommandIterator] command allocator should be finished");
        // allocator.finish();
    }
    rewind();
}

void CommandIterator::rewind() {
    currentPtr = snap::rhi::backend::common::alignPtr(allocator.data.data(), alignof(uint32_t));
}

bool CommandIterator::nextCommandId(uint32_t* commandId) {
    assert(currentPtr != nullptr);

    const uint8_t* idPtr = snap::rhi::backend::common::alignPtr(currentPtr, alignof(uint32_t));
    assert(idPtr + sizeof(uint32_t) <= allocator.data.data() + allocator.data.size());

    uint32_t id = *reinterpret_cast<const uint32_t*>(idPtr);
    if (id != CommandAllocator::EndOfBlock) {
        currentPtr = idPtr + sizeof(uint32_t);
        *commandId = id;
        return true;
    }
    return false;
}

const void* CommandIterator::nextCommand(size_t commandSize, size_t commandAlignment) {
    assert(currentPtr != nullptr);

    const uint8_t* commandPtr = snap::rhi::backend::common::alignPtr(currentPtr, commandAlignment);
    assert(commandPtr + sizeof(commandSize) <= allocator.data.data() + allocator.data.size());

    currentPtr = commandPtr + commandSize;
    return commandPtr;
}
} // namespace snap::rhi::backend::opengl
