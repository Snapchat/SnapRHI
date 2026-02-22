#pragma once

#include "snap/rhi/Texture.hpp"
#include <snap/rhi/CommandBuffer.hpp>
#include <snap/rhi/backend/common/Device.hpp>
#include <snap/rhi/backend/common/ResourceResidencySet.h>
#include <snap/rhi/backend/common/ValidationLayer.hpp>

#include <cassert>
#include <vector>

namespace snap::rhi {
class TextureInterop;
} // namespace snap::rhi

namespace snap::rhi::backend::common {
class CommandBuffer : public snap::rhi::CommandBuffer {
public:
    CommandBuffer(Device* device, const snap::rhi::CommandBufferCreateInfo& info)
        : snap::rhi::CommandBuffer(device, info),
          descriptorPoolResourceResidencySet(
              device,
              static_cast<uint32_t>(info.commandBufferCreateFlags &
                                    snap::rhi::CommandBufferCreateFlags::UnretainedResources) ?
                  snap::rhi::backend::common::ResourceResidencySet::RetentionMode::NonRetainableReferences :
                  snap::rhi::backend::common::ResourceResidencySet::RetentionMode::RetainReferences),
          resourceResidencySet(
              device,
              static_cast<uint32_t>(info.commandBufferCreateFlags &
                                    snap::rhi::CommandBufferCreateFlags::UnretainedResources) ?
                  snap::rhi::backend::common::ResourceResidencySet::RetentionMode::NonRetainableReferences :
                  snap::rhi::backend::common::ResourceResidencySet::RetentionMode::RetainReferences) {}
    ~CommandBuffer() override {
        assert(isEncodingTypeOneOf({EncodingType::None}));
    }

    [[nodiscard]] CommandBufferStatus getStatusImpl() const noexcept {
        return status_;
    }

    [[nodiscard]] EncodingType getEncodingTypeImpl() const noexcept {
        return encodingType_;
    }

    [[nodiscard]] bool isStatusOneOf(std::initializer_list<CommandBufferStatus> statuses) const noexcept {
        return std::find(statuses.begin(), statuses.end(), getStatusImpl()) != statuses.end();
    }

    [[nodiscard]] bool isEncodingTypeOneOf(std::initializer_list<EncodingType> types) const noexcept {
        return std::find(types.begin(), types.end(), getEncodingTypeImpl()) != types.end();
    }

    /// @brief Resets the command buffer to its initial state.
    void onReset(const snap::rhi::CommandBufferCreateInfo& info) noexcept {
        interopTextures_.clear();

        this->info = info;
        descriptorPoolResourceResidencySet.clear();
        resourceResidencySet.clear();

        const auto retentionMode =
            static_cast<uint32_t>(info.commandBufferCreateFlags &
                                  snap::rhi::CommandBufferCreateFlags::UnretainedResources) ?
                snap::rhi::backend::common::ResourceResidencySet::RetentionMode::NonRetainableReferences :
                snap::rhi::backend::common::ResourceResidencySet::RetentionMode::RetainReferences;
        descriptorPoolResourceResidencySet.setRetentionMode(retentionMode);
        resourceResidencySet.setRetentionMode(retentionMode);

        assert(isStatusOneOf({CommandBufferStatus::Initial, CommandBufferStatus::Submitted}));
        status_ = CommandBufferStatus::Initial;
    }

    /// @brief Sets the command buffer to the submitted state.
    /// @note Ensure that the last encoder is closed before calling this function, @see onEncoderEnd().
    void onSubmitted() noexcept {
        assert(isEncodingTypeOneOf({EncodingType::None}));
        status_ = CommandBufferStatus::Submitted;
    }

    /// @brief Sets the command buffer to the submitted state.
    /// @note Ensure that the last encoder is closed before calling this function, @see onEncoderEnd().
    void onSubmitted(const ValidationLayer& validationLayer) noexcept {
        SNAP_RHI_VALIDATE(validationLayer,
                          isEncodingTypeOneOf({EncodingType::None}),
                          snap::rhi::ReportLevel::Error,
                          snap::rhi::ValidationTag::CommandBufferOp,
                          "[onSubmitted] prior encoders should be closed before submitting the command buffer");
        return onSubmitted();
    }

    /// @brief Notifies the command buffer that a new encoder is being started.
    /// @note Ensure that the last encoder (if any) is closed before calling this function, @see onEncoderEnd().
    void onEncoderBegin(EncodingType type) noexcept {
        assert(isStatusOneOf({CommandBufferStatus::Initial, CommandBufferStatus::Recording}));
        status_ = CommandBufferStatus::Recording;
        assert(isEncodingTypeOneOf({EncodingType::None}));
        encodingType_ = type;
    }

    /// @brief Notifies the that command buffer is done encoding with the last encoder.
    void onEncoderEnd() noexcept {
        assert(isEncodingTypeOneOf({EncodingType::Blit, EncodingType::Render, EncodingType::Compute}));
        encodingType_ = EncodingType::None;
    }

    SNAP_RHI_ALWAYS_INLINE void preserveInteropTextures(std::span<snap::rhi::TextureInterop* const> textures) noexcept {
        if (textures.empty()) {
            return;
        }

        interopTextures_.insert(interopTextures_.end(), textures.begin(), textures.end());
    }

    SNAP_RHI_ALWAYS_INLINE void tryPreserveInteropTexture(snap::rhi::Texture* texture) noexcept {
        if (!texture) {
            return;
        }

        const auto& textureInterop = texture->getTextureInterop();
        if (textureInterop) {
            interopTextures_.push_back(textureInterop.get());
        }
    }

    SNAP_RHI_ALWAYS_INLINE const std::vector<snap::rhi::TextureInterop*>& getInteropTextures() const noexcept {
        return interopTextures_;
    }

#if SNAP_RHI_ENABLE_SLOW_VALIDATIONS()
    void validateResourceLifetimes() const {
        descriptorPoolResourceResidencySet.validateResourceLifetimes();
        resourceResidencySet.validateResourceLifetimes();
    }
#endif

    snap::rhi::backend::common::ResourceResidencySet& getResourceResidencySet() {
        return resourceResidencySet;
    }

    snap::rhi::backend::common::ResourceResidencySet& getDescriptorPoolResourceResidencySet() {
        return descriptorPoolResourceResidencySet;
    }

protected:
    /**
     *  Separate ResourceResidencySet to hold descriptor pools,
     *  since they must be alive while descriptor set is alive.
     *
     *  Please do not change the order of declaration of these members,
     *  @descriptorPoolResourceResidencySet must be destroyed after @resourceResidencySet.
     **/
    snap::rhi::backend::common::ResourceResidencySet descriptorPoolResourceResidencySet;
    snap::rhi::backend::common::ResourceResidencySet resourceResidencySet;

private:
    /// @brief Partially implements the snap::rhi::CommandBuffer interface.
    /// @see getStatusImpl() for querying the command buffer status, do not use this virtual function.
    [[nodiscard]] CommandBufferStatus getStatus() const final {
        return getStatusImpl();
    }

private:
    CommandBufferStatus status_ = CommandBufferStatus::Initial;
    EncodingType encodingType_ = EncodingType::None;

    std::vector<snap::rhi::TextureInterop*> interopTextures_;
};
} // namespace snap::rhi::backend::common
