#pragma once

#include <span>

#include "snap/rhi/Descriptor.h"
#include "snap/rhi/DescriptorSetCreateInfo.h"
#include "snap/rhi/DeviceChild.hpp"

namespace snap::rhi {
class Device;
class Buffer;
class Texture;
class Sampler;

/**
 * @brief Descriptor set object.
 *
 * A descriptor set binds resources (buffers, textures, samplers) to shader-visible bindings defined by an associated
 * @ref snap::rhi::DescriptorSetLayout.
 *
 * Descriptor sets are created from a @ref snap::rhi::DescriptorPool via `Device::createDescriptorSet()`.
 *
 * @warning Lifetime / resource retention
 * `DescriptorSet` does **not** guarantee retention of bound resources in the general case. Applications must ensure
 * that all resources referenced by a descriptor set remain alive while the set may be used by GPU work.
 *
 * Backends may optionally retain resources when DescriptorSet bind to command buffers (for example, Vulkan uses a
 * residency set to keep resources alive for the duration of GPU execution), but only for CommandBuffer that retain
 * refs.
 *
 * @note Backend behavior
 * - **Vulkan**: binding calls typically update the native `VkDescriptorSet` immediately via `vkUpdateDescriptorSets`.
 *   Textures may be tracked for expected image layout transitions which are applied when the set is bound to a command
 *   buffer.
 * - **Metal**: encodes resources into an argument buffer (either via `MTLArgumentEncoder` or direct GPU-address writes
 * on Tier-2 devices). Allocation uses the associated `DescriptorPool`.
 * - **OpenGL**: stores descriptor data and applies it when binding pipeline state (there is no native descriptor set).
 */
class DescriptorSet : public snap::rhi::DeviceChild {
public:
    DescriptorSet(Device* device, const DescriptorSetCreateInfo& info);
    ~DescriptorSet() override = default;

    /**
     * @brief Resets the descriptor set to its initial (unbound) state.
     *
     * This clears all previously bound resources.
     *
     * @note Backend mapping
     * - Vulkan implementations may free/recycle native descriptor set allocations depending on pool settings.
     * - Metal/OpenGL implementations typically clear tracked bindings / argument buffer contents.
     */
    virtual void reset() = 0;

    /**
     * @brief Binds a uniform buffer to the specified binding.
     *
     * Convenience overload that binds the full buffer range.
     */
    void bindUniformBuffer(uint32_t binding, snap::rhi::Buffer* buffer) {
        bindUniformBuffer(binding, buffer, 0u, WholeSize);
    }

    /**
     * @brief Binds a uniform buffer to the specified binding.
     *
     * @param binding Binding number as defined by the descriptor set layout.
     * @param buffer Buffer to bind.
     * @param offset Byte offset into @p buffer.
     * @param range Byte range of the bound region. Use `WholeSize` to indicate "until end of buffer" when supported.
     *
     * @note Alignment
     * `offset` must satisfy the backend's uniform buffer alignment requirements (e.g.
     * `Capabilities::minUniformBufferOffsetAlignment`).
     */
    virtual void bindUniformBuffer(uint32_t binding, snap::rhi::Buffer* buffer, uint64_t offset, uint64_t range) = 0;

    /**
     * @brief Binds a storage buffer to the specified binding.
     *
     * Convenience overload that binds the full buffer range.
     */
    void bindStorageBuffer(uint32_t binding, snap::rhi::Buffer* buffer) {
        bindStorageBuffer(binding, buffer, 0u, WholeSize);
    }

    /**
     * @brief Binds a storage buffer to the specified binding.
     *
     * @param binding Binding number as defined by the descriptor set layout.
     * @param buffer Buffer to bind.
     * @param offset Byte offset into @p buffer.
     * @param range Byte range of the bound region. Use `WholeSize` to indicate "until end of buffer" when supported.
     *
     * @note Alignment
     * `offset` must satisfy the backend's storage buffer alignment requirements (e.g.
     * `Capabilities::minShaderStorageBufferOffsetAlignment`).
     */
    virtual void bindStorageBuffer(uint32_t binding, snap::rhi::Buffer* buffer, uint64_t offset, uint64_t range) = 0;

    /**
     * @brief Binds a sampled texture to the specified binding.
     *
     * @param binding Binding number as defined by the descriptor set layout.
     * @param texture Texture to bind.
     */
    virtual void bindTexture(uint32_t binding, snap::rhi::Texture* texture) = 0;

    /**
     * @brief Binds a storage texture (image) to the specified binding.
     *
     * @param binding Binding number as defined by the descriptor set layout.
     * @param texture Texture to bind.
     * @param mipLevel Mip level to bind (backend-dependent).
     */
    virtual void bindStorageTexture(uint32_t binding, snap::rhi::Texture* texture, uint32_t mipLevel) = 0;

    /**
     * @brief Binds a sampler to the specified binding.
     *
     * @param binding Binding number as defined by the descriptor set layout.
     * @param sampler Sampler to bind.
     */
    virtual void bindSampler(uint32_t binding, snap::rhi::Sampler* sampler) = 0;

    /**
     * @brief Updates the descriptor set from a list of generic descriptor writes.
     *
     * This is a convenience API that applies a batch of descriptor writes described by `snap::rhi::Descriptor`.
     *
     * @param descriptorWrites Descriptor write entries.
     *
     * @note Backend mapping
     * - Vulkan maps this to `vkUpdateDescriptorSets`.
     * - Metal updates the argument buffer.
     * - OpenGL stores the bindings to be applied at draw/dispatch time.
     */
    virtual void updateDescriptorSet(std::span<snap::rhi::Descriptor> descriptorWrites) = 0;

    [[nodiscard]] uint64_t getCPUMemoryUsage() const override {
        return 0;
    }

    [[nodiscard]] uint64_t getGPUMemoryUsage() const override {
        return 0;
    }

protected:
    DescriptorSetCreateInfo info{};
};
} // namespace snap::rhi
