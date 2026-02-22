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

#include "snap/rhi/backend/opengl/CommandAllocator.hpp"
#include "snap/rhi/backend/common/Math.hpp"
#include <algorithm>
#include <cassert>

// The memory after the ID will contain the following:
//   - the current ID
//   - padding to align the command, maximum kMaxSupportedAlignment
//   - the command of size commandSize
//   - padding to align the next ID, maximum alignof(uint32_t)
//   - the next ID of size sizeof(uint32_t)

namespace snap::rhi::backend::opengl {
CommandAllocator::CommandAllocator() {
    data.resize(lastAllocationSize);
    currentPtr = snap::rhi::backend::common::alignPtr(data.data(), alignof(uint32_t));
    endPtr = currentPtr + data.size();
    isFinished = false;
}

uint8_t* CommandAllocator::allocate(uint32_t commandId, size_t commandSize, size_t commandAlignment) {
    assert(currentPtr != nullptr);
    assert(endPtr != nullptr);
    assert(commandId != EndOfBlock);

    // It should always be possible to allocate one id, for kEndOfBlock tagging,
    assert(snap::rhi::backend::common::isPtrAligned(currentPtr, alignof(uint32_t)));
    assert(endPtr >= currentPtr);
    assert(static_cast<size_t>(endPtr - currentPtr) >= sizeof(uint32_t));

    size_t remainingSize = static_cast<size_t>(endPtr - currentPtr);

    if ((remainingSize >= WorstCaseAdditionalSize) && (remainingSize - WorstCaseAdditionalSize >= commandSize)) {
        uint32_t* idAlloc = reinterpret_cast<uint32_t*>(currentPtr);
        *idAlloc = commandId;

        // we should align memory by commandAlignment
        uint8_t* commandAlloc = snap::rhi::backend::common::alignPtr(currentPtr + sizeof(uint32_t), commandAlignment);

        // we should move currentPtr after place command to memory
        currentPtr = snap::rhi::backend::common::alignPtr(commandAlloc + commandSize, alignof(uint32_t));

        return commandAlloc;
    }
    return allocateInNewMemory(commandId, commandSize, commandAlignment);
}

uint8_t* CommandAllocator::allocateInNewMemory(uint32_t commandId, size_t commandSize, size_t commandAlignment) {
    size_t requestedBlockSize = commandSize + WorstCaseAdditionalSize;
    addMemory(requestedBlockSize);
    return allocate(commandId, commandSize, commandAlignment);
}

void CommandAllocator::finish() {
    assert(isFinished == false);
    assert(currentPtr != nullptr && endPtr != nullptr);
    assert(snap::rhi::backend::common::isPtrAligned(currentPtr, alignof(uint32_t)));
    assert(currentPtr + sizeof(uint32_t) <= endPtr);
    *reinterpret_cast<uint32_t*>(currentPtr) = EndOfBlock;
    currentPtr = nullptr;
    endPtr = nullptr;
    isFinished = true;
}

void CommandAllocator::reset() {
    currentPtr = snap::rhi::backend::common::alignPtr(data.data(), alignof(uint32_t));
    endPtr = currentPtr + data.size();
    isFinished = false;
}

void CommandAllocator::addMemory(size_t minimumSize) {
    size_t offset = currentPtr - data.data();
    // Allocate blocks doubling sizes each time, to a maximum of 16k (or at least minimumSize).
    lastAllocationSize = std::max(minimumSize, std::min(lastAllocationSize * 2, MaxAllocationSize));
    data.resize(data.size() + lastAllocationSize);

    currentPtr = snap::rhi::backend::common::alignPtr(data.data() + offset, alignof(uint32_t));
    endPtr = data.data() + data.size();
}
} // namespace snap::rhi::backend::opengl
