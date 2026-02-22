//
//  GPUInfo.hpp
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 12/19/22.
//

#include <atomic>
#include <string_view>

#include <algorithm>
#include <array>

namespace snap::rhi::backend::opengl {
enum class GPUVendor : uint32_t {
    Unknown = 0,

    PowerVR,
    Apple,
    Mali,
    Adreno,
    Intel,
    ATI,
};

enum class GPUModel : uint32_t {
    Unknown = 0,

    PowerVR_SGX,
    PowerVR_Rogue_G6430,

    AppleA7, // iPhone 5S, iPad Air
    AppleA8, // iPhone 6
    AppleA9, // iPhone 6s

    Mali200,
    Mali300,
    Mali400,
    MaliT628, // Galaxy S5
    MaliT720,
    MaliT760,
    MaliT880,
    MaliG76,
    MaliG57MC2,

    Adreno225,
    Adreno306, // Samsung J3
    Adreno308, // Samsung J5
    Adreno505,
    Adreno506,
    Adreno509, // Xiaomi Redmi Note 5
    Adreno510, // redmi note 3
    Adreno512, // redmi note 5 pro
    Adreno530, // Pixel
    Adreno540, // Pixel 2
    Adreno630, // S9
    Adreno615, // Hermosa
    Adreno640, // S10, 1+7

    VivanteGC7000,

    Angle,
    Mesa,
    AMD,
    Radeon,
    AndroidEmulator,
};

GPUVendor getGPUVendor(std::string_view name);
GPUModel getGPUModel(std::string_view name);
} // namespace snap::rhi::backend::opengl
