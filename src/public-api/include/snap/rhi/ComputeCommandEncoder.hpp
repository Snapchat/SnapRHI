//
//  ComputeCommandEncoder.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 12/6/22.
//

#pragma once

#include "snap/rhi/CommandEncoder.h"

namespace snap::rhi {
/**
 * @brief Encoder for compute commands.
 *
 * A compute command encoder records compute pipeline state, descriptor bindings, and dispatches into an owning
 * `snap::rhi::CommandBuffer`.
 *
 * ## Typical usage
 * 1) `beginEncoding()`
 * 2) `bindComputePipeline()`
 * 3) `bindDescriptorSet()` (as needed)
 * 4) `dispatch()`
 * 5) `endEncoding()` (inherited from `snap::rhi::CommandEncoder`)
 *
 * ## Backend notes
 * - **Vulkan**: binds a compute pipeline with `vkCmdBindPipeline`, binds descriptor sets at dispatch time, then calls
 *   `vkCmdDispatch`.
 * - **Metal**: uses `MTLComputeCommandEncoder`; the per-dispatch `threadsPerThreadgroup` is derived from the pipeline's
 *   `localThreadGroupSize`.
 * - **OpenGL**: records commands that are later replayed by the command queue.
 * - **NoOp**: validates inputs (in debug) and performs no work.
 */
class ComputeCommandEncoder : public snap::rhi::CommandEncoder {
public:
    ComputeCommandEncoder(Device* device, CommandBuffer* commandBuffer);
    ~ComputeCommandEncoder() override = default;

    /**
     * @brief Begins a compute encoding scope.
     *
     * After this call, compute commands can be recorded until `endEncoding()` is called.
     *
     * @note Backends may reset internal cached state when beginning encoding (e.g., Vulkan resets its descriptor set
     * encoder state).
     */
    virtual void beginEncoding() = 0;

    /**
     * @brief Binds a descriptor set to the compute pipeline at a given set index.
     *
     * @param binding Descriptor set index (set number) within the pipeline layout.
     * @param descriptorSet Descriptor set to bind. Must not be null.
     * @param dynamicOffsets A span of byte offsets for all dynamic buffers in this set.
     * @warning The size of this span **must exactly match** the total count of dynamic descriptors
     * in the set layout. If the set has 0 dynamic buffers, pass an empty span.
     *
     * @pre `binding < snap::rhi::MaxBoundDescriptorSets`
     */
    virtual void bindDescriptorSet(uint32_t binding,
                                   DescriptorSet* descriptorSet,
                                   std::span<const uint32_t> dynamicOffsets) = 0;

    /**
     * @brief Binds the compute pipeline state.
     *
     * @param pipeline Compute pipeline to bind. Must not be null.
     *
     * @note This must be called before `dispatch()` for the dispatch to have defined behavior.
     */
    virtual void bindComputePipeline(ComputePipeline* pipeline) = 0;

    /**
     * @brief Dispatches compute work.
     *
     * @param groupSizeX Number of workgroups to dispatch in X.
     * @param groupSizeY Number of workgroups to dispatch in Y.
     * @param groupSizeZ Number of workgroups to dispatch in Z.
     *
     * @note The *local* workgroup size (threads-per-threadgroup / local size) is defined by the bound compute pipeline.
     */
    virtual void dispatch(uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ) = 0;

protected:
};
} // namespace snap::rhi
