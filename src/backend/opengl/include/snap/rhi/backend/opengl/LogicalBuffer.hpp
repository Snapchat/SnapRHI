#pragma once

#include "snap/rhi/Buffer.hpp"

// vector.resize is too slow, because vector.resize will initialize the value
#include <atomic>
#include <memory>
#include <span>

#include "snap/rhi/backend/common/ValidationLayer.hpp"

namespace snap::rhi::backend::opengl {
class Device;
class Profile;

class LogicalBuffer final {
public:
    /**
     * This is only optimization for staging buffer
     * User have to provide proper lifetime of @bufferData
     */
    explicit LogicalBuffer(Device* device,
                           const snap::rhi::BufferCreateInfo& info,
                           const std::shared_ptr<std::byte>& bufferData);
    explicit LogicalBuffer(Device* device, const snap::rhi::BufferCreateInfo& info);
    ~LogicalBuffer();

    std::byte* map();
    void unmap();
    void uploadData(const uint32_t offset, const std::span<const std::byte>& rawData);

private:
    const snap::rhi::backend::common::ValidationLayer& validationLayer;

    std::atomic_int32_t mapCounter = 0;
    std::shared_ptr<std::byte> data = nullptr;
    snap::rhi::BufferCreateInfo info{};
};
} // namespace snap::rhi::backend::opengl
