#include "snap/rhi/backend/opengl/BlitCommandEncoder.hpp"
#include "snap/rhi/backend/common/CommandBuffer.h"
#include "snap/rhi/backend/common/Utils.hpp"

#include "snap/rhi/Buffer.hpp"
#include "snap/rhi/Device.hpp"
#include "snap/rhi/Texture.hpp"
#include "snap/rhi/backend/opengl/CommandBuffer.hpp"
#include "snap/rhi/backend/opengl/Commands.h"
#include "snap/rhi/backend/opengl/Device.hpp"
#include <algorithm>
#include <sstream>

namespace snap::rhi::backend::opengl {
BlitCommandEncoder::BlitCommandEncoder(Device* device, snap::rhi::backend::opengl::CommandBuffer* commandBuffer)
    : BlitCommandEncoderBase(device, commandBuffer) {}

void BlitCommandEncoder::beginEncoding() {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCommandEncoder][beginEncoding]");
    BlitCommandEncoderBase::onBeginEncoding();
    commandAllocator.allocateCommand<BeginBlitPassCmd>();
}

void BlitCommandEncoder::copyBuffer(snap::rhi::Buffer* srcBuffer,
                                    snap::rhi::Buffer* dstBuffer,
                                    std::span<const BufferCopy> info) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCommandEncoder][copyBuffer]");

    assert(srcBuffer && dstBuffer);

    CopyBufferToBufferCmd* cmd = commandAllocator.allocateCommand<CopyBufferToBufferCmd>();

    cmd->srcBuffer = srcBuffer;
    cmd->dstBuffer = dstBuffer;

    if (info.size() > MaxCopyInfos) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "BufferCopy infos count exceeds maximum limit");
    }
    cmd->infoCount = std::min((uint32_t)info.size(), MaxCopyInfos);
    std::copy_n(info.begin(), cmd->infoCount, cmd->infos.begin());

    resourceResidencySet.track(srcBuffer);
    resourceResidencySet.track(dstBuffer);

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing CopyBufferToBufferCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        std::stringstream srcBufferStream;
        srcBufferStream << "\tsrcBuffer: " << static_cast<const void*>(cmd->srcBuffer);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        srcBufferStream.str());
        std::stringstream dstBufferStream;
        dstBufferStream << "\tdstBuffer: " << static_cast<const void*>(cmd->dstBuffer);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        dstBufferStream.str());
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "\tinfos: ");
        for (uint32_t idx = 0; idx < cmd->infoCount; ++idx) {
            auto& element = cmd->infos[idx];
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\tBufferCopy element " + std::to_string(idx));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tsrcOffset: " + std::to_string(element.srcOffset));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tdstOffset: " + std::to_string(element.dstOffset));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tsize: " + std::to_string(element.size));
        }
    }
}

void BlitCommandEncoder::copyBufferToTexture(snap::rhi::Buffer* srcBuffer,
                                             snap::rhi::Texture* dstTexture,
                                             std::span<const BufferTextureCopy> info) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCommandEncoder][copyBufferToTexture]");
    assert(srcBuffer && dstTexture);

    commandBuffer->tryPreserveInteropTexture(dstTexture);

    CopyBufferToTextureCmd* cmd = commandAllocator.allocateCommand<CopyBufferToTextureCmd>();

    cmd->srcBuffer = srcBuffer;
    cmd->dstTexture = dstTexture;

    if (info.size() > MaxCopyInfos) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "BufferTextureCopy count exceeds maximum limit");
    }
    cmd->infoCount = std::min((uint32_t)info.size(), MaxCopyInfos);
    std::copy_n(info.begin(), cmd->infoCount, cmd->infos.begin());

    resourceResidencySet.track(srcBuffer);
    resourceResidencySet.track(dstTexture);

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing CopyBufferToTextureCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        std::stringstream srcBufferStream;
        srcBufferStream << "\tsrcBuffer: " << static_cast<const void*>(cmd->srcBuffer);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        srcBufferStream.str());
        std::stringstream dstTextureStream;
        dstTextureStream << "\tdstTexture: " << static_cast<const void*>(cmd->dstTexture);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        dstTextureStream.str());
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "\tinfos: ");
        for (uint32_t idx = 0; idx < cmd->infoCount; ++idx) {
            auto& element = cmd->infos[idx];
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\tBufferTextureCopy element " + std::to_string(idx));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tbufferOffset: " + std::to_string(element.bufferOffset));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\ttextureSubresource: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\tmipLevel: " + std::to_string(element.textureSubresource.mipLevel));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\ttextureOffset: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\t(x, y, z): (" + std::to_string(element.textureOffset.x) + ", " +
                                std::to_string(element.textureOffset.y) + ", " +
                                std::to_string(element.textureOffset.z) + ")");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\ttextureExtent: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\t(width, height, depth): (" + std::to_string(element.textureExtent.width) + ", " +
                                std::to_string(element.textureExtent.height) + ", " +
                                std::to_string(element.textureExtent.depth) + ")");
        }
    }
}

void BlitCommandEncoder::copyTextureToBuffer(snap::rhi::Texture* srcTexture,
                                             snap::rhi::Buffer* dstBuffer,
                                             std::span<const BufferTextureCopy> info) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCommandEncoder][copyTextureToBuffer]");
    assert(srcTexture && dstBuffer);

    commandBuffer->tryPreserveInteropTexture(srcTexture);

    CopyTextureToBufferCmd* cmd = commandAllocator.allocateCommand<CopyTextureToBufferCmd>();

    cmd->srcTexture = srcTexture;
    cmd->dstBuffer = dstBuffer;

    if (info.size() > MaxCopyInfos) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "BufferTextureCopy count exceeds maximum limit");
    }
    cmd->infoCount = std::min((uint32_t)info.size(), MaxCopyInfos);
    std::copy_n(info.begin(), cmd->infoCount, cmd->infos.begin());

    resourceResidencySet.track(srcTexture);
    resourceResidencySet.track(dstBuffer);

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing CopyTextureToBufferCmd******");
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "BlitCommandEncoder label: ");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        std::stringstream srcTextureStream;
        srcTextureStream << "\tsrcTexture: " << static_cast<const void*>(cmd->srcTexture);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        srcTextureStream.str());
        std::stringstream dstTextureStream;
        dstTextureStream << "\tdstBuffer: " << static_cast<const void*>(cmd->dstBuffer);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        dstTextureStream.str());
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "\tinfos: ");
        for (uint32_t idx = 0; idx < cmd->infoCount; ++idx) {
            auto& element = cmd->infos[idx];
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\tBufferTextureCopy element " + std::to_string(idx));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tbufferOffset: " + std::to_string(element.bufferOffset));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\ttextureSubresource: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\tmipLevel: " + std::to_string(element.textureSubresource.mipLevel));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\ttextureOffset: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\t(x, y, z): (" + std::to_string(element.textureOffset.x) + ", " +
                                std::to_string(element.textureOffset.y) + ", " +
                                std::to_string(element.textureOffset.z) + ")");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\ttextureExtent: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\t(width, height, depth): (" + std::to_string(element.textureExtent.width) + ", " +
                                std::to_string(element.textureExtent.height) + ", " +
                                std::to_string(element.textureExtent.depth) + ")");
        }
    }
}

void BlitCommandEncoder::copyTexture(snap::rhi::Texture* srcTexture,
                                     snap::rhi::Texture* dstTexture,
                                     std::span<const TextureCopy> info) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCommandEncoder][copyTexture]");
    assert(srcTexture && dstTexture);

    commandBuffer->tryPreserveInteropTexture(srcTexture);
    commandBuffer->tryPreserveInteropTexture(dstTexture);

    CopyTextureToTextureCmd* cmd = commandAllocator.allocateCommand<CopyTextureToTextureCmd>();

    cmd->srcTexture = srcTexture;
    cmd->dstTexture = dstTexture;

    if (info.size() > MaxCopyInfos) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "TextureCopy count exceeds maximum limit");
    }
    cmd->infoCount = std::min((uint32_t)info.size(), MaxCopyInfos);
    std::copy_n(info.begin(), cmd->infoCount, cmd->infos.begin());

    resourceResidencySet.track(srcTexture);
    resourceResidencySet.track(dstTexture);

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing CopyTextureToTextureCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        std::stringstream srcTextureStream;
        srcTextureStream << "\tsrcTexture: " << static_cast<const void*>(cmd->srcTexture);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        std::string(srcTextureStream.str()));
        std::stringstream dstTextureStream;
        dstTextureStream << "\tdstTexture: " << static_cast<const void*>(cmd->dstTexture);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        dstTextureStream.str());
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "\tinfos: ");
        for (uint32_t idx = 0; idx < cmd->infoCount; ++idx) {
            auto& element = cmd->infos[idx];
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\tTextureCopy element " + std::to_string(idx));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tsrcSubresource: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\tmipLevel: " + std::to_string(element.srcSubresource.mipLevel));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tsrcOffset: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\t(x, y, z): (" + std::to_string(element.srcOffset.x) + ", " +
                                std::to_string(element.srcOffset.y) + ", " + std::to_string(element.srcOffset.z) + ")");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tdstSubresource: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\tmipLevel: " + std::to_string(element.dstSubresource.mipLevel));
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\tdstOffset: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\t(x, y, z): (" + std::to_string(element.dstOffset.x) + ", " +
                                std::to_string(element.dstOffset.y) + ", " + std::to_string(element.dstOffset.z) + ")");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\textent: ");
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Debug,
                            snap::rhi::ValidationTag::CommandBufferOp,
                            "\t\t\t(width, height, depth): (" + std::to_string(element.extent.width) + ", " +
                                std::to_string(element.extent.height) + ", " + std::to_string(element.extent.depth) +
                                ")");
        }
    }
}

void BlitCommandEncoder::generateMipmaps(snap::rhi::Texture* texture) {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCommandEncoder][generateMipmaps]");
    assert(texture);

    commandBuffer->tryPreserveInteropTexture(texture);

    GenerateMipmapCmd* cmd = commandAllocator.allocateCommand<GenerateMipmapCmd>();
    cmd->texture = texture;

    resourceResidencySet.track(texture);

    if (device->getDeviceCreateInfo().enabledReportLevel == snap::rhi::ReportLevel::Debug) {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        "******Executing GenerateMipmapCmd******");
        SNAP_RHI_REPORT(
            validationLayer, snap::rhi::ReportLevel::Debug, snap::rhi::ValidationTag::CommandBufferOp, "Parameters: ");
        std::stringstream textureStream;
        textureStream << "\ttexture: " << static_cast<const void*>(cmd->texture);
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Debug,
                        snap::rhi::ValidationTag::CommandBufferOp,
                        textureStream.str());
    }
}

void BlitCommandEncoder::endEncoding() {
    SNAP_RHI_DECLARE_CUSTOM_PROFILING_LABEL(device, "[BlitCommandEncoder][endEncoding]");

    BlitCommandEncoderBase::onEndEncoding();
    commandAllocator.allocateCommand<EndBlitPassCmd>();
}
} // namespace snap::rhi::backend::opengl
