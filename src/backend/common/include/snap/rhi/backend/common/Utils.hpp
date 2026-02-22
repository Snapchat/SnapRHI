//
//  Utils.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 22.09.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include "snap/rhi/Capabilities.h"
#include "snap/rhi/CommandBuffer.hpp"
#include "snap/rhi/Common.h"
#include "snap/rhi/Fence.hpp"
#include "snap/rhi/RenderPassCreateInfo.h"
#include "snap/rhi/RenderPipelineCreateInfo.h"
#include "snap/rhi/Semaphore.hpp"
#include "snap/rhi/TextureCreateInfo.h"
#include "snap/rhi/TextureViewCreateInfo.h"
#include "snap/rhi/backend/common/Math.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/common/Inlining.h"

#include <atomic>
#include <cassert>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

#if defined(NDEBUG)
#define SNAP_RHI_DEBUG() 0
#else
#define SNAP_RHI_DEBUG() 1
#endif

#define SNAP_RHI_ENABLE_SLOW_VALIDATIONS() (SNAP_RHI_ENABLE_SLOW_SAFETY_CHECKS)

namespace snap::rhi::backend::common {
static constexpr bool enableSlowSafetyChecks() {
    return SNAP_RHI_ENABLE_SLOW_VALIDATIONS();
}

template<typename T, typename U>
T* smart_cast(U* const ptr) {
    if constexpr (enableSlowSafetyChecks()) {
        auto result = dynamic_cast<T*>(ptr);
        assert(result);
        return result;
    } else {
        return static_cast<T*>(ptr);
    }
}

template<typename T, typename U>
std::shared_ptr<T> smart_cast(std::shared_ptr<U> const ptr) {
    if constexpr (enableSlowSafetyChecks()) {
        auto result = std::dynamic_pointer_cast<T>(ptr);
        assert(result);
        return result;
    } else {
        return std::static_pointer_cast<T>(ptr);
    }
}

template<typename T, typename U>
T* smart_dynamic_cast(U* const ptr) {
    auto result = dynamic_cast<T*>(ptr);
    assert(result);
    return result;
}

struct StringCmp {
    using is_transparent = void;
    bool operator()(std::string_view a, std::string_view b) const {
        return a < b;
    }
};

bool hasEnding(const std::string_view fullString, const std::string_view ending);

int32_t getIdxByName(const std::string_view name);

std::string getNonArrayName(const std::string_view name);

template<typename T, size_t Capacity>
struct FixedSingleItemAllocator {
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;
    template<typename U>
    struct rebind {
        typedef FixedSingleItemAllocator<U, Capacity> other;
    };

    using storage_type = std::aligned_storage_t<sizeof(T), alignof(T)>;
    constexpr static size_t MaxCapacity = 64;
    static_assert(Capacity && Capacity <= MaxCapacity, "Max capacity exceeded or empty.");

    FixedSingleItemAllocator() {}
    FixedSingleItemAllocator(const FixedSingleItemAllocator&) {}
    FixedSingleItemAllocator(FixedSingleItemAllocator&&) {}
    template<typename U>
    FixedSingleItemAllocator(const FixedSingleItemAllocator<U, Capacity>&) {}
    template<typename U>
    FixedSingleItemAllocator(FixedSingleItemAllocator<U, Capacity>&&) {}

    [[nodiscard]] constexpr T* allocate(std::size_t n) noexcept {
        if (n != 1) {
            return nullptr;
        }
        uint64_t firstUnsetMask = (~slots_) & (slots_ + 1);
        slots_ |= firstUnsetMask;
        uint64_t firstUnsetIndex = snap::rhi::backend::common::log2(firstUnsetMask);
        return (firstUnsetIndex < Capacity) ? reinterpret_cast<T*>(&items[firstUnsetIndex]) : nullptr;
    }

    [[nodiscard]] constexpr T* allocate(std::size_t n, [[maybe_unused]] const void* hint) noexcept {
        return allocate(n);
    }

    constexpr void deallocate(T* p, std::size_t n) noexcept {
        if (!p || n != 1) {
            return;
        }
        int64_t arrIndex = std::distance(&items[0], (storage_type*)p);
        if (arrIndex < 0 || arrIndex > Capacity) {
            return;
        }
        slots_ &= ~(1ull << arrIndex);
    }

    [[nodiscard]] constexpr size_type max_size() const noexcept {
        return Capacity;
    }
    template<typename U>
    constexpr bool operator==(const FixedSingleItemAllocator<U, Capacity>& rhs) noexcept {
        return this == &rhs;
    }
    template<typename U>
    constexpr bool operator!=(const FixedSingleItemAllocator<U, Capacity>& rhs) noexcept {
        return this != &rhs;
    }

protected:
    uint64_t slots_ = 0;
    storage_type items[Capacity];
};

template<typename T>
std::unordered_map<std::string, size_t> buildNameToId(const T& info) {
    std::unordered_map<std::string, size_t> nameToId;
    for (size_t i = 0; i < info.size(); ++i) {
        const auto& obj = info[i];
        nameToId[obj.name] = i;
    }
    return nameToId;
}

std::string computeSharedPrefix(const std::string& a, const std::string& b);

bool haveSamePrefix(const std::string& src, const std::string& prefix);

AttachmentFormatsCreateInfo convertToAttachmentFormatsCreateInfo(
    const snap::rhi::RenderPassCreateInfo& renderPassCreateInfo, const bool stencilEnable);

constexpr std::string_view SamplerDefaultSuffix{"Sampler"};

bool setEnvVar(const std::string& name, const std::string& value);

bool writeStringToFile(const std::filesystem::path& filePath, const std::string& fileData);
bool writeBinToFile(const std::filesystem::path& filePath, const std::span<const std::byte> data);

void reportCapabilities(const snap::rhi::Capabilities& caps,
                        const snap::rhi::backend::common::ValidationLayer& validationLayer);

/**
 * @brief Synchronization primitives required for Texture Interoperability.
 *
 * This structure encapsulates the synchronization objects needed to safely share
 * textures between different APIs.
 */
struct TextureInteropSyncInfo {
    /**
     * @brief The fence to signal when the submitted work is complete.
     * * This fence must be passed to the queue submission (e.g., `CommandQueue::submit`).
     * It will be signaled by the GPU once the command buffers associated with
     * this submission have finished executing, indicating that the texture is
     * ready for external API.
     */
    std::shared_ptr<snap::rhi::Fence> signalFence = nullptr;

    /**
     * @brief A list of semaphores that the queue must wait on before execution.
     * * These semaphores represent dependencies from previous operations (e.g.,
     * an external API releasing the texture). The GPU command queue will stall
     * execution of the new command buffers until all semaphores in this vector
     * are signaled.
     */
    std::vector<std::shared_ptr<snap::rhi::Semaphore>> waitSemaphores{};
};

TextureInteropSyncInfo processTextureInterop(std::span<snap::rhi::CommandBuffer*> buffers, snap::rhi::Fence*& fence);

/**
 * @brief User have to call device->drain() before destroying returned device.
 * But in order to simplify usage SnapRHI create a wrapper that calls waitIdle() in destructor.
 */
std::shared_ptr<snap::rhi::Device> createDeviceSafe(std::shared_ptr<snap::rhi::Device> device);

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
void validateResourceLifetimes(snap::rhi::Device* device);
#define SNAP_RHI_VALIDATE_RESOURCE_LIFETIMES(device) snap::rhi::backend::common::validateResourceLifetimes(device)
#else
#define SNAP_RHI_VALIDATE_RESOURCE_LIFETIMES(device)
#endif

snap::rhi::TextureCreateInfo buildTextureCreateInfoFromView(const snap::rhi::TextureViewCreateInfo& viewInfo);
snap::rhi::TextureViewInfo buildTextureViewInfo(const snap::rhi::TextureViewInfo& srcView,
                                                const snap::rhi::TextureViewInfo& targetView);
snap::rhi::TextureViewCreateInfo buildTextureViewCreateInfo(const snap::rhi::TextureViewCreateInfo& srcView,
                                                            const snap::rhi::TextureViewCreateInfo& targetView);
} // namespace snap::rhi::backend::common
