#!/bin/bash
# Compile Vulkan GLSL shaders to SPIR-V and generate C++ header with embedded byte arrays.
set -e

SHADER_DIR="$(cd "$(dirname "$0")/../shaders/vulkan" && pwd)"
OUT_DIR="$(cd "$(dirname "$0")/.." && pwd)/generated"
HEADER="$OUT_DIR/VulkanSPIRV.h"
SPV_DIR="/tmp/snap_rhi_spirv"

mkdir -p "$OUT_DIR" "$SPV_DIR"

echo "Compiling Vulkan shaders from $SHADER_DIR..."

glslc --target-env=vulkan1.0 -o "$SPV_DIR/passthrough_vert.spv" "$SHADER_DIR/passthrough.vert"
glslc --target-env=vulkan1.0 -o "$SPV_DIR/passthrough_frag.spv" "$SHADER_DIR/passthrough.frag"
glslc --target-env=vulkan1.0 -o "$SPV_DIR/fill_vert.spv"        "$SHADER_DIR/fill.vert"
glslc --target-env=vulkan1.0 -o "$SPV_DIR/fill_frag.spv"        "$SHADER_DIR/fill.frag"
glslc --target-env=vulkan1.0 -o "$SPV_DIR/compute_fill_comp.spv" "$SHADER_DIR/compute_fill.comp"

echo "All shaders compiled. Generating $HEADER ..."

cat > "$HEADER" <<'HEADER_TOP'
//
// VulkanSPIRV.h — Auto-generated. Do not edit.
//
// Embedded SPIR-V binaries compiled from Vulkan GLSL test shaders.
// Regenerate with: tests/scripts/compile_vulkan_shaders.sh
//

#pragma once

#include <cstdint>
#include <span>

#define SNAP_RHI_TEST_VULKAN_SPIRV_AVAILABLE 1

namespace test_shaders { namespace spirv {

HEADER_TOP

for spv in "$SPV_DIR"/*.spv; do
    name="$(basename "$spv" .spv)"
    echo "  Embedding $name ..."
    echo "// --- $name ---" >> "$HEADER"
    echo "constexpr uint8_t k_${name}[] = {" >> "$HEADER"
    xxd -i < "$spv" | sed 's/^/    /' >> "$HEADER"
    echo "};" >> "$HEADER"
    echo "" >> "$HEADER"
    echo "constexpr std::span<const uint8_t> ${name}(k_${name}, sizeof(k_${name}));" >> "$HEADER"
    echo "" >> "$HEADER"
done

echo "}} // namespace test_shaders::spirv" >> "$HEADER"

echo "Done: $HEADER"
