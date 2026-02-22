#pragma once

#include "snap/rhi/backend/common/CommandBuffer.h"
#include <cassert>
#include <snap/rhi/backend/common/Device.hpp>
#include <snap/rhi/backend/common/ResourceResidencySet.h>
#include <snap/rhi/backend/common/ValidationLayer.hpp>

namespace snap::rhi {
class BlitCommandEncoder;
class RenderCommandEncoder;
class ComputeCommandEncoder;
} // namespace snap::rhi

namespace snap::rhi::backend::common {

/// @brief A mixin class for command encoders that provides common functionality for state switching.
/// @tparam CommandEncoderBase The base class for the command encoder, e.g, BlitCommandEncoder, RenderCommandEncoder, or
/// ComputeCommandEncoder.
template<typename CommandEncoderBase>
class CommandEncoder : public CommandEncoderBase {
public:
    /// @tparam CommandBufferImpl The type of the command buffer implementation, must be derived from
    /// snap::rhi::CommandBuffer and snap::rhi::backend::common::CommandBuffer<>
    template<typename CommandBufferImpl>
    CommandEncoder(snap::rhi::backend::common::Device* device, CommandBufferImpl* commandBuffer)
        : CommandEncoderBase(device, commandBuffer),
          commandBuffer(commandBuffer),
          validationLayer(device->getValidationLayer()),
          resourceResidencySet(commandBuffer->getResourceResidencySet()) {}

    ~CommandEncoder() noexcept override = default;

    constexpr bool isStatusOneOf(std::initializer_list<CommandBufferStatus> statuses) const {
        return commandBuffer->isStatusOneOf(statuses);
    }

    constexpr bool isEncodingTypeOneOf(std::initializer_list<EncodingType> types) const {
        return commandBuffer->isEncodingTypeOneOf(types);
    }

protected:
    static constexpr EncodingType getExpectedEncodingType() {
        if constexpr (std::is_base_of_v<snap::rhi::BlitCommandEncoder, CommandEncoderBase>) {
            return EncodingType::Blit;
        } else if constexpr (std::is_base_of_v<snap::rhi::RenderCommandEncoder, CommandEncoderBase>) {
            return EncodingType::Render;
        } else if constexpr (std::is_base_of_v<snap::rhi::ComputeCommandEncoder, CommandEncoderBase>) {
            return EncodingType::Compute;
        } else {
            assert(false && "Unsupported command encoder type");
            return EncodingType::None;
        }
    }

    /// @brief Notifies the command buffer that encoding has started, automatically determining the type of encoder.
    void onBeginEncoding() noexcept {
        SNAP_RHI_VALIDATE(validationLayer,
                          isEncodingTypeOneOf({EncodingType::None}),
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::CommandBufferOp,
                          "[onBeginEncoding] prior encoders should be closed before beginning a new encoding.");
        commandBuffer->onEncoderBegin(getExpectedEncodingType());
    }

    /// @brief Notifies the command buffer that encoding has ended.
    void onEndEncoding() noexcept {
        SNAP_RHI_VALIDATE(validationLayer,
                          isEncodingTypeOneOf({getExpectedEncodingType()}),
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::CommandBufferOp,
                          "[onEndEncoding] BlitCommandEncoder must be started.");
        commandBuffer->onEncoderEnd();
    }

protected:
    CommandBuffer* commandBuffer = nullptr;
    const snap::rhi::backend::common::ValidationLayer& validationLayer;
    snap::rhi::backend::common::ResourceResidencySet& resourceResidencySet;
};
} // namespace snap::rhi::backend::common
