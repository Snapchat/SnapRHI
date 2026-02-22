#include "snap/rhi/backend/metal/ShaderModule.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/ShaderLibrary.h"
#include "snap/rhi/backend/metal/Utils.h"
#include <snap/rhi/common/Throw.h>

namespace {
snap::rhi::reflection::ShaderModuleInfo buildShaderReflection(const id<MTLFunction>& func) {
    snap::rhi::reflection::ShaderModuleInfo info{};

    for (size_t i = 0; i < func.vertexAttributes.count; ++i) {
        const auto& attrib = func.vertexAttributes[i];

        if (attrib.isActive == YES) {
            snap::rhi::reflection::VertexAttributeInfo vertexInfo{};
            vertexInfo.name = snap::rhi::backend::metal::getString(attrib.name);
            vertexInfo.location = static_cast<uint32_t>(attrib.attributeIndex);
            vertexInfo.format = snap::rhi::backend::metal::convertToVtxFormat(attrib.attributeType);

            info.vertexAttributes.push_back(vertexInfo);
        }
    }

    return info;
}

MTLFunctionConstantValues* prepareFunctionConstants(const snap::rhi::SpecializationInfo& info) {
    MTLFunctionConstantValues* constantValues = [MTLFunctionConstantValues new];
    for (uint32_t i = 0; i < info.mapEntryCount; ++i) {
        const auto* pData = static_cast<const uint8_t*>(info.pData) + info.pMapEntries[i].offset;
        switch (info.pMapEntries[i].format) {
            case snap::rhi::SpecializationConstantFormat::Bool32: {
                assert(info.pMapEntries[i].offset + sizeof(uint32_t) <= info.dataSize);
                const bool boolValue = *reinterpret_cast<const uint32_t*>(pData);
                [constantValues setConstantValue:&boolValue
                                            type:MTLDataTypeBool
                                         atIndex:info.pMapEntries[i].constantID];
            } break;

            case snap::rhi::SpecializationConstantFormat::Float: {
                assert(info.pMapEntries[i].offset + sizeof(float) <= info.dataSize);
                [constantValues setConstantValue:pData type:MTLDataTypeFloat atIndex:info.pMapEntries[i].constantID];
            } break;

            case snap::rhi::SpecializationConstantFormat::Int32: {
                assert(info.pMapEntries[i].offset + sizeof(int32_t) <= info.dataSize);
                [constantValues setConstantValue:pData type:MTLDataTypeInt atIndex:info.pMapEntries[i].constantID];
            } break;

            case snap::rhi::SpecializationConstantFormat::UInt32: {
                assert(info.pMapEntries[i].offset + sizeof(uint32_t) <= info.dataSize);
                [constantValues setConstantValue:pData type:MTLDataTypeUInt atIndex:info.pMapEntries[i].constantID];
            } break;

            default:
                snap::rhi::common::throwException("[prepareFunctionConstants] invalid SpecializationConstantFormat");
        }
    }
    return constantValues;
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
ShaderModule::ShaderModule(Device* mtlDevice, const snap::rhi::ShaderModuleCreateInfo& info)
    : snap::rhi::ShaderModule(mtlDevice, info) {
    auto mtlShaderLibrary =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::metal::ShaderLibrary>(info.shaderLibrary);

    const id<MTLLibrary>& library = mtlShaderLibrary->getMtlLibrary();
    MTLFunctionConstantValues* constantValues = prepareFunctionConstants(info.specializationInfo);
    NSString* entryName = snap::rhi::backend::metal::getString(info.name);

    auto onCompleteFunc = [this, mtlDevice](id<MTLFunction> function, NSError* error) {
        [[maybe_unused]] const auto& validationLayer = mtlDevice->getValidationLayer();

        assert(function != nil);

        if (error != nil) {
            std::string description = [error.description UTF8String];
            SNAP_RHI_REPORT(validationLayer,
                            snap::rhi::ReportLevel::Error,
                            snap::rhi::ValidationTag::CreateOp,
                            "[ShaderModule] Failed to created shader module, error: %s",
                            description.data());
        }

        this->function = function;
        this->shaderReflection = buildShaderReflection(function);
        this->asyncPromise.set_value();
    };

    asyncFuture = asyncPromise.get_future();

    bool isAsync =
        (info.createFlags & ShaderModuleCreateFlags::AllowAsyncCreation) == ShaderModuleCreateFlags::AllowAsyncCreation;
    if (isAsync) {
        [library newFunctionWithName:entryName constantValues:constantValues completionHandler:onCompleteFunc];
    } else {
        NSError* error = nil;
        function = [library newFunctionWithName:entryName constantValues:constantValues error:&error];
        onCompleteFunc(function, error);
    }
}

ShaderModule::~ShaderModule() {
    sync();
}

void ShaderModule::sync() const {
    asyncFuture.wait();
}

const std::optional<reflection::ShaderModuleInfo>& ShaderModule::getReflectionInfo() const {
    sync();
    return snap::rhi::ShaderModule::getReflectionInfo();
}

const id<MTLFunction>& ShaderModule::getFunction() const {
    sync();
    return function;
}

void ShaderModule::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    function.label = [NSString stringWithUTF8String:label.data()];
#endif
}

} // namespace snap::rhi::backend::metal
