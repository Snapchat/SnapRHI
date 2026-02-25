#include "snap/rhi/backend/metal/PipelineCache.h"
#include "snap/rhi/RenderPass.hpp"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Utils.h"

#include "snap/rhi/common/OS.h"
#include <snap/rhi/common/Scope.h>
#include <snap/rhi/common/Throw.h>

namespace snap::rhi::backend::metal {
PipelineCache::PipelineCache(Device* mtlDevice, const snap::rhi::PipelineCacheCreateInfo& info)
    : snap::rhi::PipelineCache(mtlDevice, info) {
    if (@available(macOS 11.0, ios 14.0, *)) {
        MTLBinaryArchiveDescriptor* archiveDescriptor = [[MTLBinaryArchiveDescriptor alloc] init];
        NSError* archiveError = nil;

        std::string pathStr = info.cachePath.string();
        NSString* nsStringPath = [NSString stringWithUTF8String:pathStr.c_str()];
        NSURL* fileURL = [NSURL fileURLWithPath:nsStringPath];

        NSError* fileError;
        if ([fileURL checkResourceIsReachableAndReturnError:&fileError]) {
            archiveDescriptor.url = fileURL;
        } else {
            archiveDescriptor.url = nil;
        }

        archive = [mtlDevice->getMtlDevice() newBinaryArchiveWithDescriptor:archiveDescriptor error:&archiveError];
        if (!archive) {
            snap::rhi::common::throwException(
                "[PipelineCache][available(macOS 11.0, ios 14.0)] Failed to create bin archive");
        }
    } else {
        // Do nothing since bin archive doesn't supported
    }
}

void PipelineCache::addRenderPipeline(const snap::rhi::RenderPipelineCreateInfo& info,
                                      const id<MTLFunction>& vertex,
                                      const id<MTLFunction>& fragment) {
    /**
     * Instead of retain RenderPass ref, SnapRHI convert RenderPass infor into AttachmentFormatsCreateInfo
     */
    snap::rhi::RenderPipelineCreateInfo createInfo = info;
    if (createInfo.renderPass) {
        createInfo.attachmentFormatsCreateInfo = snap::rhi::backend::common::convertToAttachmentFormatsCreateInfo(
            createInfo.renderPass->getCreateInfo(), info.depthStencilState.stencilEnable);
    }
    createInfo.renderPass = nullptr;
    renderPipelines.push_back({createInfo, vertex, fragment});
}

void PipelineCache::addComputePipeline(const snap::rhi::ComputePipelineCreateInfo& info,
                                       const id<MTLFunction>& compute) {
    computePipelines.push_back({info, compute});
}

const id<MTLBinaryArchive>& PipelineCache::getBinaryArchive() const {
    return archive;
}

Result PipelineCache::serializeToFile(const std::filesystem::path& cachePath) const {
    /**
     * SnapRHI postpone call addRenderPipelineFunctionsWithDescriptor/addComputePipelineFunctionsWithDescriptor
     * because we want avoid compilation as much as possible to improve performance
     * */
    SNAP_RHI_ON_SCOPE_EXIT {
        renderPipelines.clear();
        computePipelines.clear();
    };

    if (@available(macOS 11.0, ios 14.0, *)) {
        auto* mtlDevice = snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::Device>(device);
        const auto& validationLayer = mtlDevice->getValidationLayer();

        NSError* archiveError = nil;
        for (const auto& pipelineInfo : renderPipelines) {
            MTLRenderPipelineDescriptor* pipelineDescriptor = snap::rhi::backend::metal::createRenderPipelineDescriptor(
                pipelineInfo.info, pipelineInfo.vertex, pipelineInfo.fragment);
            BOOL result = [archive addRenderPipelineFunctionsWithDescriptor:pipelineDescriptor error:&archiveError];
            if (result == NO) {
                std::string description;
                if (archiveError != nil) {
                    description = getString(archiveError.description);
                }
                SNAP_RHI_REPORT(validationLayer,
                                snap::rhi::ReportLevel::Info,
                                snap::rhi::ValidationTag::PipelineCacheOp,
                                "[PipelineCache::serializeToFile] Failed to add render pipeline: %s\n",
                                description.c_str());
            }
        }

        for (const auto& pipelineInfo : computePipelines) {
            MTLComputePipelineDescriptor* pipelineDescriptor = [[MTLComputePipelineDescriptor alloc] init];
            pipelineDescriptor.computeFunction = pipelineInfo.compute;
            BOOL result = [archive addComputePipelineFunctionsWithDescriptor:pipelineDescriptor error:&archiveError];
            if (result == NO) {
                std::string description;
                if (archiveError != nil) {
                    description = getString(archiveError.description);
                }
                SNAP_RHI_REPORT(validationLayer,
                                snap::rhi::ReportLevel::Info,
                                snap::rhi::ValidationTag::PipelineCacheOp,
                                "[PipelineCache::serializeToFile] Failed to add compute pipeline: %s\n",
                                description.c_str());
            }
        }

        std::string pathStr = cachePath.string();
        NSString* nsStringPath = [NSString stringWithUTF8String:pathStr.c_str()];
        NSURL* fileURL = [NSURL fileURLWithPath:nsStringPath];

        BOOL result = [archive serializeToURL:fileURL error:&archiveError];
        if (result == NO) {
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Info,
                            snap::rhi::ValidationTag::PipelineCacheOp,
                            "[PipelineCache] Failed to save archive");
            return Result::ErrorUnknown;
        }
    }

    return snap::rhi::Result::Success;
}

void PipelineCache::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    archive.label = [NSString stringWithUTF8String:label.data()];
#endif
}

} // namespace snap::rhi::backend::metal
