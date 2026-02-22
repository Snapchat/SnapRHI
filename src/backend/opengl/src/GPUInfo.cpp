#include "snap/rhi/backend/opengl/GPUInfo.h"

namespace {
struct VendorModelPair {
    snap::rhi::backend::opengl::GPUVendor vendor = snap::rhi::backend::opengl::GPUVendor::Unknown;
    snap::rhi::backend::opengl::GPUModel model = snap::rhi::backend::opengl::GPUModel::Unknown;
};

struct ModelKeyPair {
    snap::rhi::backend::opengl::GPUModel model = snap::rhi::backend::opengl::GPUModel::Unknown;
    std::string_view key;
};

// clang-format off
constexpr ModelKeyPair modelKeyPairs[] = {
    {snap::rhi::backend::opengl::GPUModel::PowerVR_SGX, "PowerVR SGX"},
    {snap::rhi::backend::opengl::GPUModel::PowerVR_Rogue_G6430, "PowerVR Rogue G6430"},
    {snap::rhi::backend::opengl::GPUModel::AppleA7, "Apple A7"},
    {snap::rhi::backend::opengl::GPUModel::AppleA8, "Apple A8"},
    {snap::rhi::backend::opengl::GPUModel::AppleA9, "Apple A9"},
    {snap::rhi::backend::opengl::GPUModel::Mali200, "Mali-200"},
    {snap::rhi::backend::opengl::GPUModel::Mali300, "Mali-300"},
    {snap::rhi::backend::opengl::GPUModel::Mali400, "Mali-4"},
    {snap::rhi::backend::opengl::GPUModel::MaliT628, "Mali-T628"},
    {snap::rhi::backend::opengl::GPUModel::MaliT720, "Mali-T720"},
    {snap::rhi::backend::opengl::GPUModel::MaliT760, "Mali-T760"},
    {snap::rhi::backend::opengl::GPUModel::MaliT880, "Mali-T880"},
    {snap::rhi::backend::opengl::GPUModel::MaliG76, "Mali-G76"},
    {snap::rhi::backend::opengl::GPUModel::MaliG57MC2, "Mali-G57 MC2"},
    {snap::rhi::backend::opengl::GPUModel::Adreno225, "Adreno (TM) 225"},
    {snap::rhi::backend::opengl::GPUModel::Adreno306, "Adreno (TM) 306"},
    {snap::rhi::backend::opengl::GPUModel::Adreno308, "Adreno (TM) 308"},
    {snap::rhi::backend::opengl::GPUModel::Adreno506, "Adreno (TM) 506"},
    {snap::rhi::backend::opengl::GPUModel::Adreno509, "Adreno (TM) 509"},
    {snap::rhi::backend::opengl::GPUModel::Adreno510, "Adreno (TM) 510"},
    {snap::rhi::backend::opengl::GPUModel::Adreno512, "Adreno (TM) 512"},
    {snap::rhi::backend::opengl::GPUModel::Adreno505, "Adreno (TM) 505"},
    {snap::rhi::backend::opengl::GPUModel::Adreno530, "Adreno (TM) 530"},
    {snap::rhi::backend::opengl::GPUModel::Adreno540, "Adreno (TM) 540"},
    {snap::rhi::backend::opengl::GPUModel::Adreno630, "Adreno (TM) 630"},
    {snap::rhi::backend::opengl::GPUModel::Adreno615, "Adreno (TM) 615"},
    {snap::rhi::backend::opengl::GPUModel::Adreno640, "Adreno (TM) 640"},
    {snap::rhi::backend::opengl::GPUModel::VivanteGC7000, "Vivante GC7000"},
    {snap::rhi::backend::opengl::GPUModel::Angle, "ANGLE"},
    {snap::rhi::backend::opengl::GPUModel::Mesa, "Mesa"},
    {snap::rhi::backend::opengl::GPUModel::AMD, "AMD"},
    {snap::rhi::backend::opengl::GPUModel::Radeon, "Radeon"},
    {snap::rhi::backend::opengl::GPUModel::AndroidEmulator, "Android Emulator OpenGL ES Translator"},
};
// clang-format on

constexpr std::array<std::pair<std::string_view, snap::rhi::backend::opengl::GPUVendor>, 6>
    modelNameSubstrToGpuVendorMap = {{
        {"PowerVR", snap::rhi::backend::opengl::GPUVendor::PowerVR},
        {"Apple", snap::rhi::backend::opengl::GPUVendor::Apple},
        {"Mali", snap::rhi::backend::opengl::GPUVendor::Mali},
        {"Adreno", snap::rhi::backend::opengl::GPUVendor::Adreno},
        {"Intel", snap::rhi::backend::opengl::GPUVendor::Intel},
        {"ATI", snap::rhi::backend::opengl::GPUVendor::ATI},
    }};
} // namespace

namespace snap::rhi::backend::opengl {
GPUVendor getGPUVendor(std::string_view modelName) {
    for (auto& el : modelNameSubstrToGpuVendorMap) {
        if (modelName.find(el.first) != std::string_view::npos)
            return el.second;
    }
    return GPUVendor::Unknown;
}

GPUModel getGPUModel(std::string_view name) {
    auto position = std::find_if(std::begin(modelKeyPairs), std::end(modelKeyPairs), [name](const ModelKeyPair& pair) {
        return name.find(pair.key) != std::string_view::npos;
    });
    return position == std::end(modelKeyPairs) ? GPUModel::Unknown : position->model;
}
} // namespace snap::rhi::backend::opengl
