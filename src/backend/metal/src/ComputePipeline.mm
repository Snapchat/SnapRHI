#include "snap/rhi/backend/metal/ComputePipeline.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/common/ValidationLayer.hpp"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/PipelineCache.h"
#include "snap/rhi/backend/metal/ShaderModule.h"
#include "snap/rhi/backend/metal/Utils.h"

#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

namespace {
} // unnamed namespace

namespace snap::rhi::backend::metal {
ComputePipeline::ComputePipeline(Device* mtlDevice, const snap::rhi::ComputePipelineCreateInfo& info)
    : snap::rhi::ComputePipeline(mtlDevice, info), validationLayer(mtlDevice->getValidationLayer()) {
    const snap::rhi::backend::metal::ShaderModule* mtlShaderModule =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::ShaderModule>(info.stage);
    computeFunction = mtlShaderModule->getFunction();

    MTLComputePipelineDescriptor* pipelineDescriptor = [[MTLComputePipelineDescriptor alloc] init];
    pipelineDescriptor.computeFunction = computeFunction;

    if (@available(macOS 11.0, ios 14.0, *)) {
        if (info.pipelineCache) {
            auto* mtlPipelineCache = snap::rhi::backend::common::smart_cast<PipelineCache>(info.pipelineCache);
            mtlPipelineCache->addComputePipeline(info, computeFunction);

            const id<MTLBinaryArchive>& binaryArchive = mtlPipelineCache->getBinaryArchive();
            pipelineDescriptor.binaryArchives = [NSArray arrayWithObject:binaryArchive];
        }
    }

    auto onCompleteFunc =
        [this](id<MTLComputePipelineState> state, MTLComputePipelineReflection* reflection, NSError* error) {
            try {
                if ((error != nil) || (state == nil)) {
                    std::string description = "[ComputePipeline] Failed to create pipeline state, error:";
                    if (error != nil) {
                        description += getString(error.description);
                    }

                    snap::rhi::common::throwException(description);
                }

                this->pipelineState = state;

                this->reflection = snap::rhi::reflection::ComputePipelineInfo{
                    .descriptorSetInfos =
                        snap::rhi::backend::metal::buildDescriptorSetReflection(reflection, this->info.pipelineLayout)};

                this->asyncPromise.set_value();
            } catch (...) {
                this->asyncPromise.set_exception(std::current_exception());
            }
        };

    bool isAsync =
        (info.pipelineCreateFlags & PipelineCreateFlags::AllowAsyncCreation) == PipelineCreateFlags::AllowAsyncCreation;

    asyncFuture = asyncPromise.get_future();
    MTLPipelineOption options = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;

    if (isAsync) {
        [mtlDevice->getMtlDevice() newComputePipelineStateWithDescriptor:pipelineDescriptor
                                                                 options:options
                                                       completionHandler:onCompleteFunc];
    } else {
        MTLComputePipelineReflection* reflectionObj = nil;
        NSError* error = nil;
        pipelineState = [mtlDevice->getMtlDevice() newComputePipelineStateWithDescriptor:pipelineDescriptor
                                                                                 options:options
                                                                              reflection:&reflectionObj
                                                                                   error:&error];
        onCompleteFunc(pipelineState, reflectionObj, error);
    }
}

ComputePipeline::ComputePipeline(Device* mtlDevice,
                                 const snap::rhi::ComputePipelineCreateInfo& info,
                                 const std::optional<reflection::ComputePipelineInfo>& reflectionInfo,
                                 const id<MTLFunction>& computeFunction,
                                 const id<MTLComputePipelineState>& pipelineState)
    : snap::rhi::ComputePipeline(mtlDevice, info),
      validationLayer(mtlDevice->getValidationLayer()),
      computeFunction(computeFunction),
      pipelineState(pipelineState) {
    reflection = reflectionInfo;
    asyncFuture = asyncPromise.get_future();
    asyncPromise.set_value();
}

void ComputePipeline::sync() const {
    std::call_once(syncFlag, [this]() {
        if (asyncFuture.valid()) {
            asyncFuture.get();
        }
    });

    SNAP_RHI_VALIDATE(validationLayer,
                      pipelineState != nil,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::ComputePipelineOp,
                      "[RenderPipeline] Failed to create compute pipeline");
}

const id<MTLFunction>& ComputePipeline::getComputeFunction() const {
    return computeFunction;
}

const id<MTLComputePipelineState>& ComputePipeline::getComputePipeline() const {
    sync();
    return pipelineState;
}

const std::optional<reflection::ComputePipelineInfo>& ComputePipeline::getReflectionInfo() const {
    sync();
    return snap::rhi::ComputePipeline::getReflectionInfo();
}

void ComputePipeline::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    computeFunction.label = [NSString stringWithUTF8String:label.data()];
    // .label is a readonly property of the MTLComputePipelineState object
#endif
}

} // namespace snap::rhi::backend::metal
