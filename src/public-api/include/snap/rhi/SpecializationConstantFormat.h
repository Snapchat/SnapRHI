//
//  SpecializationConstantFormat.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 20.05.2021.
//

#pragma once

#include <cstdint>

namespace snap::rhi {

/**
 * @brief Data format of a specialization constant.
 *
 * Specialization constants are provided through `snap::rhi::SpecializationInfo` as a raw byte buffer.
 * Each `SpecializationMapEntry` specifies the constant's @ref SpecializationConstantFormat so backends can interpret
 * and upload the value correctly.
 *
 * Backend notes:
 * - Vulkan requires boolean specialization constants to use `VkBool32` storage (4 bytes).
 * - Float/Int32/UInt32 values are treated as 32-bit scalars across backends.
 * - OpenGL/WebGPU implementations typically convert these values into defines/constant entries.
 */
enum class SpecializationConstantFormat : uint32_t {
    /**
     * @brief The format is not specified.
     */
    Undefined = 0,

    /**
     * @brief 32-bits boolean value(32-bit unsigned integer value).
     *
     * Interpreted as a boolean scalar. For Vulkan, the value is expected to be stored as a 32-bit `VkBool32`.
     */
    Bool32,

    /**
     * @brief 32-bit IEEE-754 floating point value.
     */
    Float,

    /**
     * @brief 32-bit signed integer value.
     */
    Int32,

    /**
     * @brief 32-bit unsigned integer value.
     */
    UInt32,

    /**
     * @brief Sentinel value.
     */
    Count,
};
} // namespace snap::rhi
