#include "snap/rhi/backend/metal/RenderPipeline.h"

#include "snap/rhi/backend/common/ValidationLayer.hpp"

#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Utils.h"

#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/PipelineCache.h"
#include "snap/rhi/backend/metal/ShaderModule.h"

#include "snap/rhi/common/Throw.h"
#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

namespace {
MTLStencilOperation convertToMtlStencilOp(const snap::rhi::StencilOp stencilOp) {
    switch (stencilOp) {
        case snap::rhi::StencilOp::Keep:
            return MTLStencilOperationKeep;
        case snap::rhi::StencilOp::Zero:
            return MTLStencilOperationZero;
        case snap::rhi::StencilOp::Replace:
            return MTLStencilOperationReplace;
        case snap::rhi::StencilOp::IncAndClamp:
            return MTLStencilOperationIncrementClamp;
        case snap::rhi::StencilOp::DecAndClamp:
            return MTLStencilOperationDecrementClamp;
        case snap::rhi::StencilOp::Invert:
            return MTLStencilOperationInvert;
        case snap::rhi::StencilOp::IncAndWrap:
            return MTLStencilOperationIncrementWrap;
        case snap::rhi::StencilOp::DecAndWrap:
            return MTLStencilOperationDecrementWrap;
        default:
            snap::rhi::common::throwException("invalid stencil operation");
    }
}

MTLStencilDescriptor* buildStencilState(const snap::rhi::StencilOpState& state) {
    MTLStencilDescriptor* frontFaceStencil = [MTLStencilDescriptor new];
    frontFaceStencil.depthFailureOperation = convertToMtlStencilOp(state.depthFailOp);
    frontFaceStencil.depthStencilPassOperation = convertToMtlStencilOp(state.passOp);
    frontFaceStencil.stencilFailureOperation = convertToMtlStencilOp(state.failOp);
    frontFaceStencil.stencilCompareFunction = snap::rhi::backend::metal::convertToMtlCompareFunc(state.stencilFunc);
    frontFaceStencil.writeMask = static_cast<uint32_t>(state.writeMask);
    frontFaceStencil.readMask = static_cast<uint32_t>(state.readMask);

    return frontFaceStencil;
}

id<MTLDepthStencilState> buildDepthStencilState(snap::rhi::backend::metal::Device* device,
                                                const snap::rhi::DepthStencilStateCreateInfo& info) {
    assert(!(info.depthWrite && !info.depthTest)); // invalid usage

    MTLDepthStencilDescriptor* depthStencilDescriptor = [[MTLDepthStencilDescriptor alloc] init];
    if (info.depthTest) {
        depthStencilDescriptor.depthWriteEnabled = info.depthWrite;
        depthStencilDescriptor.depthCompareFunction =
            snap::rhi::backend::metal::convertToMtlCompareFunc(info.depthFunc);
    }
    if (info.stencilEnable) {
        depthStencilDescriptor.backFaceStencil = buildStencilState(info.stencilBack);
        depthStencilDescriptor.frontFaceStencil = buildStencilState(info.stencilFront);
    }
    return [device->getMtlDevice() newDepthStencilStateWithDescriptor:depthStencilDescriptor];
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
RenderPipeline::RenderPipeline(Device* mtlDevice, const snap::rhi::RenderPipelineCreateInfo& info)
    : snap::rhi::RenderPipeline(mtlDevice, info), validationLayer(mtlDevice->getValidationLayer()) {
    id<MTLFunction> vertexFunction = getFunction(info.stages, snap::rhi::ShaderStage::Vertex);
    id<MTLFunction> fragmentFunction = getFunction(info.stages, snap::rhi::ShaderStage::Fragment);
    MTLRenderPipelineDescriptor* pipelineDescriptor =
        snap::rhi::backend::metal::createRenderPipelineDescriptor(info, vertexFunction, fragmentFunction);

    if (@available(macOS 11.0, ios 14.0, *)) {
        if (info.pipelineCache) {
            auto* mtlPipelineCache = snap::rhi::backend::common::smart_cast<PipelineCache>(info.pipelineCache);
            mtlPipelineCache->addRenderPipeline(info, vertexFunction, fragmentFunction);

            const id<MTLBinaryArchive>& binaryArchive = mtlPipelineCache->getBinaryArchive();
            pipelineDescriptor.binaryArchives = [NSArray arrayWithObject:binaryArchive];
        }
    }

    auto onCompleteFunc = [this, vertexFunction](id<MTLRenderPipelineState> state,
                                                 MTLRenderPipelineReflection* reflection,
                                                 NSError* error) {
        try {
            if ((error != nil) || (state == nil)) {
                std::string description = "[RenderPipeline] Failed to create pipeline state, error:";
                if (error != nil) {
                    description += getString(error.description);
                }

                snap::rhi::common::throwException(description);
            }

            this->pipelineState = state;

            if (reflection != nil) {
                this->reflection = snap::rhi::reflection::RenderPipelineInfo{
                    .vertexAttributes = snap::rhi::backend::metal::buildVertexReflection(vertexFunction),
                    .descriptorSetInfos =
                        snap::rhi::backend::metal::buildDescriptorSetReflection(reflection, this->info.pipelineLayout)};
            }
            this->asyncPromise.set_value();
        } catch (...) {
            this->asyncPromise.set_exception(std::current_exception());
        }
    };

    const bool isNativeReflectionAcquired = (info.pipelineCreateFlags & PipelineCreateFlags::AcquireNativeReflection) ==
                                            PipelineCreateFlags::AcquireNativeReflection;
    const bool isAsync =
        (info.pipelineCreateFlags & PipelineCreateFlags::AllowAsyncCreation) == PipelineCreateFlags::AllowAsyncCreation;

    asyncFuture = asyncPromise.get_future();
    MTLPipelineOption options = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;

    if (isAsync) {
        if (isNativeReflectionAcquired) {
            [mtlDevice->getMtlDevice() newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                                    options:options
                                                          completionHandler:onCompleteFunc];
        } else {
            auto handler = [onCompleteFunc](id<MTLRenderPipelineState> state, NSError* error) {
                onCompleteFunc(state, nil, error);
            };

            [mtlDevice->getMtlDevice() newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                          completionHandler:handler];
        }
    } else {
        MTLRenderPipelineReflection* reflectionObj = nil;
        NSError* error = nil;

        if (isNativeReflectionAcquired) {
            pipelineState = [mtlDevice->getMtlDevice() newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                                                    options:options
                                                                                 reflection:&reflectionObj
                                                                                      error:&error];
        } else {
            pipelineState = [mtlDevice->getMtlDevice() newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                                                      error:&error];
        }

        onCompleteFunc(pipelineState, reflectionObj, error);
    }

    initDepthStencilState(mtlDevice, info.basePipeline);
}

RenderPipeline::RenderPipeline(Device* mtlDevice,
                               const snap::rhi::RenderPipelineCreateInfo& info,
                               const snap::rhi::reflection::RenderPipelineInfo& reflectionInfo,
                               const id<MTLRenderPipelineState>& state)
    : snap::rhi::RenderPipeline(mtlDevice, info),
      validationLayer(mtlDevice->getValidationLayer()),
      pipelineState(state) {
    initDepthStencilState(mtlDevice, info.basePipeline);
    reflection = reflectionInfo;
    asyncFuture = asyncPromise.get_future();
    asyncPromise.set_value();
}

void RenderPipeline::initDepthStencilState(Device* mtlDevice, snap::rhi::RenderPipeline* basePipeline) {
    if (basePipeline && basePipeline->getCreateInfo().depthStencilState == info.depthStencilState) {
        RenderPipeline* mtlRenderPipeline = snap::rhi::backend::common::smart_cast<RenderPipeline>(basePipeline);
        depthStencilState = mtlRenderPipeline->depthStencilState;
    }

    if (depthStencilState == nil) {
        depthStencilState = buildDepthStencilState(mtlDevice, info.depthStencilState);
    }
}

void RenderPipeline::sync() const {
    std::call_once(syncFlag, [this]() {
        if (asyncFuture.valid()) {
            asyncFuture.get();
        }
    });

    SNAP_RHI_VALIDATE(validationLayer,
                      pipelineState != nil,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::RenderPipelineOp,
                      "[RenderPipeline] Failed to create render pipeline");
}

const id<MTLDepthStencilState>& RenderPipeline::getDepthStencilState() const {
    return depthStencilState;
}

const id<MTLRenderPipelineState>& RenderPipeline::getRenderPipeline() const {
    sync();
    return pipelineState;
}

const std::optional<reflection::RenderPipelineInfo>& RenderPipeline::getReflectionInfo() const {
    sync();
    return snap::rhi::RenderPipeline::getReflectionInfo();
}
} // namespace snap::rhi::backend::metal
