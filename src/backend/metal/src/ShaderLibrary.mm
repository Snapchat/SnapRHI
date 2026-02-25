#include "snap/rhi/backend/metal/ShaderLibrary.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Utils.h"
#include "snap/rhi/common/Throw.h"

#include <array>
#include <string_view>

namespace {
id<MTLLibrary> createLibraryFromSource(const snap::rhi::backend::common::ValidationLayer& validationLayer,
                                       id<MTLDevice> mtlDevice,
                                       const uint8_t* pCode,
                                       const size_t codeSize) {
    NSError* error = nil;
    MTLCompileOptions* compileOptions = nil;

    if (@available(ios 11.0, *)) {
        // by spec __METAL_IOS__  should be defined, but that's not true for ios 10
    } else {
        compileOptions = [MTLCompileOptions new];
        NSDictionary* dictionary = @{@"__METAL_IOS__" : @1};
        compileOptions.preprocessorMacros = dictionary;
    }

    NSString* source =
        snap::rhi::backend::metal::getString(std::string_view{reinterpret_cast<const char*>(pCode), codeSize});
    id<MTLLibrary> library = [mtlDevice newLibraryWithSource:source options:compileOptions error:&error];
    if (error != nil) {
        std::string description = [error.description UTF8String];
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::CreateOp,
                        "[createLibraryFromSource] erors: %s",
                        description.c_str());
    }

    if (library == nil) {
        std::string description = [error.description UTF8String];
        snap::rhi::common::throwException("Failed to create library from pCode: " + description);
    }

    return library;
}

id<MTLLibrary> createLibraryFromBinary(const snap::rhi::backend::common::ValidationLayer& validationLayer,
                                       id<MTLDevice> mtlDevice,
                                       const uint8_t* pCode,
                                       const size_t codeSize) {
    NSError* error = nil;
    dispatch_data_t lib_data =
        dispatch_data_create(pCode, codeSize, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);

    id<MTLLibrary> library = [mtlDevice newLibraryWithData:lib_data error:&error];
    if (error != nil) {
        std::string description = [error.description UTF8String];
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Warning,
                        snap::rhi::ValidationTag::CreateOp,
                        "[createLibraryFromBinary] erors: %s",
                        description.c_str());
    }

    if (library == nil) {
        std::string description = [error.description UTF8String];
        snap::rhi::common::throwException("[createLibraryFromBinary] Failed to create library from pCode: " +
                                          description);
    }

    return library;
}

snap::rhi::SpecializationConstantFormat convertToSpecConstFormat(const MTLDataType type) {
    switch (type) {
        case MTLDataTypeBool:
            return snap::rhi::SpecializationConstantFormat::Bool32;
        case MTLDataTypeFloat:
            return snap::rhi::SpecializationConstantFormat::Float;
        case MTLDataTypeInt:
            return snap::rhi::SpecializationConstantFormat::Int32;
        case MTLDataTypeUInt:
            return snap::rhi::SpecializationConstantFormat::UInt32;

        default: {
            snap::rhi::common::throwException("[convertToSpecConstFormat] invalid MTLDataType");
        }
    }
}

void buildSpecConstsList(const id<MTLFunction>& function,
                         std::vector<snap::rhi::reflection::SpecializationInfo>& specConstsInfo) {
    for (NSString* key in function.functionConstantsDictionary.allKeys) {
        auto value = function.functionConstantsDictionary[key];

        snap::rhi::reflection::SpecializationInfo info{};
        info.name = snap::rhi::backend::metal::getString(key);
        info.format = convertToSpecConstFormat(value.type);
        info.constantID = static_cast<int32_t>(value.index);
        info.required = value.required ? true : false;

        specConstsInfo.push_back(info);
    }
}

snap::rhi::ShaderStage convertToShaderType(const MTLFunctionType funcType) {
    switch (funcType) {
        case MTLFunctionTypeVertex:
            return snap::rhi::ShaderStage::Vertex;
        case MTLFunctionTypeFragment:
            return snap::rhi::ShaderStage::Fragment;
        case MTLFunctionTypeKernel:
            return snap::rhi::ShaderStage::Compute;

        default: {
            snap::rhi::common::throwException("[convertToShaderType] invalid MTLFunctionType");
        }
    }
}

snap::rhi::reflection::ShaderLibraryInfo buildShaderLibraryInfo(const id<MTLLibrary>& library) {
    snap::rhi::reflection::ShaderLibraryInfo result{};
    result.entryPoints.resize(library.functionNames.count);
    for (size_t i = 0; i < library.functionNames.count; ++i) {
        result.entryPoints[i].name = snap::rhi::backend::metal::getString(library.functionNames[i]);

        id<MTLFunction> function = [library newFunctionWithName:library.functionNames[i]];
        result.entryPoints[i].stage = convertToShaderType(function.functionType);
        buildSpecConstsList(function, result.entryPoints[i].specializationsInfo);
    }

    return result;
}
} // unnamed namespace

namespace snap::rhi::backend::metal {
ShaderLibrary::ShaderLibrary(Device* mtlDevice, const snap::rhi::ShaderLibraryCreateInfo& info)
    : snap::rhi::ShaderLibrary(mtlDevice, info) {
    const auto& validationLayer = mtlDevice->getValidationLayer();

    if (info.libCompileFlag == ShaderLibraryCreateFlag::CompileFromSource) {
        library = createLibraryFromSource(
            mtlDevice->getValidationLayer(), mtlDevice->getMtlDevice(), info.code.data(), info.code.size());
    } else if (info.libCompileFlag == ShaderLibraryCreateFlag::CompileFromBinary) {
        library = createLibraryFromBinary(
            mtlDevice->getValidationLayer(), mtlDevice->getMtlDevice(), info.code.data(), info.code.size());
    } else {
        SNAP_RHI_REPORT(validationLayer,
                        snap::rhi::ReportLevel::Error,
                        snap::rhi::ValidationTag::CreateOp,
                        "[ShaderLibrary] wrong libCompileFlag{%d}, only \'CompileFromSource\' and "
                        "\'CompileFromBinary\' supported for Metal",
                        info.libCompileFlag);
    }

    shaderLibraryInfo = buildShaderLibraryInfo(library);
}

const id<MTLLibrary>& ShaderLibrary::getMtlLibrary() const {
    return library;
}

void ShaderLibrary::setDebugLabel(std::string_view label) {
#if SNAP_RHI_ENABLE_DEBUG_LABELS
    ObjectDebugMarkers::setDebugLabel(label);
    library.label = [NSString stringWithUTF8String:label.data()];
#endif
}

} // namespace snap::rhi::backend::metal
