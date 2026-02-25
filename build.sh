#!/bin/bash
# ============================================================================
# SnapRHI Build Script (macOS / Linux)
# ============================================================================
#
# Usage:
#   ./build.sh                                    # Metal (macOS default)
#   ./build.sh --with-demo                        # Metal with demo app
#   ./build.sh --vulkan --with-demo               # Vulkan with demo
#   ./build.sh --vulkan --with-demo --validation  # Full validation
#   ./build.sh --release                          # Optimized build
#   ./build.sh --xcode --with-demo                # Generate Xcode project
#   ./build.sh --clean                            # Clean rebuild
#   ./build.sh --help                             # Show all options
#
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_TYPE="Debug"
WITH_DEMO="OFF"
BACKEND="metal"
GENERATOR=""
CLEAN=false
VALIDATION_LAYERS="OFF"
ALL_VALIDATION="OFF"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --with-demo)
            WITH_DEMO="ON"
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --vulkan)
            BACKEND="vulkan"
            shift
            ;;
        --opengl)
            BACKEND="opengl"
            shift
            ;;
        --metal)
            BACKEND="metal"
            shift
            ;;
        --xcode)
            GENERATOR="-G Xcode"
            shift
            ;;
        --validation)
            ALL_VALIDATION="ON"
            shift
            ;;
        --validation-layers)
            VALIDATION_LAYERS="ON"
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [options]"
            echo ""
            echo "Backend:"
            echo "  --metal              Metal backend (default on macOS)"
            echo "  --vulkan             Vulkan backend"
            echo "  --opengl             OpenGL backend"
            echo ""
            echo "Build:"
            echo "  --with-demo          Include demo application"
            echo "  --release            Release build (default: Debug)"
            echo "  --clean              Delete build directory first"
            echo "  --xcode              Generate Xcode project (no build)"
            echo ""
            echo "Validation:"
            echo "  --validation         Enable ALL validation features"
            echo "  --validation-layers  Enable Vulkan validation layers only"
            echo ""
            echo "Examples:"
            echo "  $0 --with-demo                        # Metal demo (macOS)"
            echo "  $0 --vulkan --with-demo               # Vulkan demo"
            echo "  $0 --vulkan --with-demo --validation  # Full validation"
            echo "  $0 --release                          # Optimized library"
            echo "  $0 --xcode --with-demo                # Xcode project"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Run '$0 --help' for usage"
            exit 1
            ;;
    esac
done

# Validate backend on macOS
if [[ "$BACKEND" == "metal" && "$(uname)" != "Darwin" ]]; then
    echo "Error: Metal backend requires macOS"
    exit 1
fi

# Set backend CMake option
case $BACKEND in
    metal)
        BACKEND_OPTION="-DSNAP_RHI_ENABLE_METAL=ON"
        ;;
    vulkan)
        BACKEND_OPTION="-DSNAP_RHI_ENABLE_VULKAN=ON"
        ;;
    opengl)
        BACKEND_OPTION="-DSNAP_RHI_ENABLE_OPENGL=ON"
        ;;
esac

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "=== Cleaning build directory ==="
    rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

echo "=== SnapRHI Build Configuration ==="
echo "Backend:           ${BACKEND}"
echo "Build Type:        ${BUILD_TYPE}"
echo "Demo App:          ${WITH_DEMO}"
echo "All Validation:    ${ALL_VALIDATION}"
echo "Validation Layers: ${VALIDATION_LAYERS}"
echo "Generator:         ${GENERATOR:-default}"
echo "Build Dir:         ${BUILD_DIR}"
echo "==================================="
echo ""

# Configure
echo "=== Configuring ==="
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
    ${GENERATOR} \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    ${BACKEND_OPTION} \
    -DSNAP_RHI_BUILD_EXAMPLES="${WITH_DEMO}" \
    -DSNAP_RHI_DEMO_APP_API="${BACKEND}" \
    -DSNAP_RHI_VULKAN_ENABLE_LAYERS="${VALIDATION_LAYERS}" \
    -DSNAP_RHI_ENABLE_ALL_VALIDATION="${ALL_VALIDATION}"

# Build (skip if Xcode generator - user will build in Xcode)
if [[ -z "$GENERATOR" ]]; then
    echo ""
    echo "=== Building ==="
    cmake --build "${BUILD_DIR}" --parallel

    echo ""
    echo "=== Build Complete ==="
    echo "Libraries: ${BUILD_DIR}/src/"
    if [ "$WITH_DEMO" = "ON" ]; then
        if [[ "$(uname)" == "Darwin" ]]; then
            echo "Demo app:  ${BUILD_DIR}/examples/triangle-demo-app/snap_rhi_demo_app.app"
            echo ""
            echo "Run with:  open ${BUILD_DIR}/examples/triangle-demo-app/snap_rhi_demo_app.app"
        else
            echo "Demo app:  ${BUILD_DIR}/examples/triangle-demo-app/snap_rhi_demo_app"
        fi
    fi
else
    echo ""
    echo "=== Xcode Project Generated ==="
    echo "Open with: open ${BUILD_DIR}/snap-rhi.xcodeproj"
fi
