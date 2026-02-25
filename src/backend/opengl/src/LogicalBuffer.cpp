#include "snap/rhi/backend/opengl/LogicalBuffer.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/opengl/Device.hpp"

namespace snap::rhi::backend::opengl {
LogicalBuffer::LogicalBuffer(Device* device,
                             const snap::rhi::BufferCreateInfo& info,
                             const std::shared_ptr<std::byte>& bufferData)
    : validationLayer(device->getValidationLayer()), data(bufferData), info(info) {}

LogicalBuffer::LogicalBuffer(Device* device, const snap::rhi::BufferCreateInfo& info)
    : validationLayer(device->getValidationLayer()), info(info) {
    data = std::shared_ptr<std::byte>(new std::byte[info.size], std::default_delete<std::byte[]>());
}

LogicalBuffer::~LogicalBuffer() {
    if constexpr (snap::rhi::backend::common::enableSlowSafetyChecks()) {
        int32_t cnt = mapCounter.load();
        SNAP_RHI_VALIDATE(validationLayer,
                          cnt == 0,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::BufferOp,
                          "[snap::rhi::backend::opengl::LogicalBuffer][~LogicalBuffer] wrong buffer map count: %d",
                          cnt);
    }
}

std::byte* LogicalBuffer::map() {
    if constexpr (snap::rhi::backend::common::enableSlowSafetyChecks()) {
        int32_t cnt = mapCounter.load();
        SNAP_RHI_VALIDATE(validationLayer,
                          cnt >= 0,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::BufferOp,
                          "[snap::rhi::backend::opengl::LogicalBuffer][map] wrong buffer map count: %d",
                          cnt);
        ++mapCounter;
    }

    assert(data);
    return data.get();
}

void LogicalBuffer::unmap() {
    if constexpr (snap::rhi::backend::common::enableSlowSafetyChecks()) {
        int32_t cnt = mapCounter.load();
        SNAP_RHI_VALIDATE(validationLayer,
                          cnt > 0,
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::BufferOp,
                          "[snap::rhi::backend::opengl::LogicalBuffer][unmap] wrong buffer map count: %d",
                          cnt);
        --mapCounter;
    }
}

void LogicalBuffer::uploadData(const uint32_t offset, const std::span<const std::byte>& rawData) {
    SNAP_RHI_VALIDATE(
        validationLayer,
        info.size >= offset + rawData.size(),
        snap::rhi::ReportLevel::Error,
        snap::rhi::ValidationTag::BufferOp,
        "[snap::rhi::backend::opengl::LogicalBuffer][uploadData] Buffer{size: %d} out of memory{size: %d}",
        info.size,
        offset + rawData.size());

    memcpy(data.get() + offset, rawData.data(), rawData.size());
}
} // namespace snap::rhi::backend::opengl
