#pragma once

#include "snap/rhi/backend/opengl/OpenGL.h"

#include <cassert>
#include <cstdint>
#include <string_view>

namespace snap::rhi::backend::opengl {

enum class TextureId : uint32_t { Null, CurrentSurfaceBackBuffer = std::numeric_limits<uint32_t>::max() };

enum class TextureTarget : uint32_t {
    Detached = 0,
    Renderbuffer = GL_RENDERBUFFER,
    Texture2D = GL_TEXTURE_2D,
    Texture2DArray = GL_TEXTURE_2D_ARRAY,
    Texture3D = GL_TEXTURE_3D,
    TextureCubeMap = GL_TEXTURE_CUBE_MAP,
    TextureCubeMapSidePosX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    TextureCubeMapSideNegX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    TextureCubeMapSidePosY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    TextureCubeMapSideNegY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    TextureCubeMapSidePosZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    TextureCubeMapSideNegZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    TextureRectangle = GL_TEXTURE_RECTANGLE,
    Texture2DMS = GL_TEXTURE_2D_MULTISAMPLE,
    Texture2DMSArray = GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
    TextureExternal = GL_TEXTURE_EXTERNAL_OES,
    TextureCubeMapFirstSide = TextureCubeMapSidePosX,
    TextureCubeMapLastSide = TextureCubeMapSideNegZ,
    TextureCubeMapNumSides = TextureCubeMapLastSide - TextureCubeMapFirstSide + 1,
};

static_assert(uint32_t(TextureTarget::TextureCubeMapNumSides) == 6u);

static inline constexpr uint32_t TextureCubeMapNumSides = uint32_t(TextureTarget::TextureCubeMapNumSides);

template<typename T = TextureTarget>
static inline constexpr T getCubeMapSideTextureTarget(uint8_t i) {
    return T(uint32_t(TextureTarget::TextureCubeMapFirstSide) + i);
}

[[nodiscard]] static inline constexpr std::string_view toString(TextureTarget target) noexcept {
    switch (target) {
        case TextureTarget::Detached:
            return "Detached";
        case TextureTarget::Renderbuffer:
            return "Renderbuffer";
        case TextureTarget::Texture2D:
            return "Texture2D";
        case TextureTarget::Texture2DArray:
            return "Texture2DArray";
        case TextureTarget::Texture3D:
            return "Texture3D";
        case TextureTarget::TextureCubeMap:
            return "TextureCubeMap";
        case TextureTarget::TextureCubeMapSidePosX:
            return "TextureCubeMapSidePosX";
        case TextureTarget::TextureCubeMapSideNegX:
            return "TextureCubeMapSideNegX";
        case TextureTarget::TextureCubeMapSidePosY:
            return "TextureCubeMapSidePosY";
        case TextureTarget::TextureCubeMapSideNegY:
            return "TextureCubeMapSideNegY";
        case TextureTarget::TextureCubeMapSidePosZ:
            return "TextureCubeMapSidePosZ";
        case TextureTarget::TextureCubeMapSideNegZ:
            return "TextureCubeMapSideNegZ";
        case TextureTarget::TextureRectangle:
            return "TextureRectangle";
        case TextureTarget::TextureExternal:
            return "TextureExternal";
        case TextureTarget::TextureCubeMapNumSides:
            return "TextureCubeMapNumSides";
        default:
            assert(false && "Caught unknown texture target.");
            return "<Error>";
    }
}

enum class TextureTargetIndex : uint8_t {
    Detached = 0,
    Renderbuffer,
    Texture2D,
    Texture2DArray,
    Texture3D,
    TextureCubeMap,
    TextureCubeMapSidePosX,
    TextureCubeMapSideNegX,
    TextureCubeMapSidePosY,
    TextureCubeMapSideNegY,
    TextureCubeMapSidePosZ,
    TextureCubeMapSideNegZ,
    TextureRectangle,
    Texture2DMS,
    Texture2DMSArray,
    TextureExternal,
    Error,
};

[[nodiscard]] static inline constexpr TextureTargetIndex toIndex(TextureTarget target) noexcept {
    switch (target) {
        case TextureTarget::Detached:
            return TextureTargetIndex::Detached;
        case TextureTarget::Renderbuffer:
            return TextureTargetIndex::Renderbuffer;
        case TextureTarget::Texture2D:
            return TextureTargetIndex::Texture2D;
        case TextureTarget::Texture2DArray:
            return TextureTargetIndex::Texture2DArray;
        case TextureTarget::Texture3D:
            return TextureTargetIndex::Texture3D;
        case TextureTarget::TextureCubeMap:
            return TextureTargetIndex::TextureCubeMap;
        case TextureTarget::TextureCubeMapSidePosX:
            return TextureTargetIndex::TextureCubeMapSidePosX;
        case TextureTarget::TextureCubeMapSideNegX:
            return TextureTargetIndex::TextureCubeMapSideNegX;
        case TextureTarget::TextureCubeMapSidePosY:
            return TextureTargetIndex::TextureCubeMapSidePosY;
        case TextureTarget::TextureCubeMapSideNegY:
            return TextureTargetIndex::TextureCubeMapSideNegY;
        case TextureTarget::TextureCubeMapSidePosZ:
            return TextureTargetIndex::TextureCubeMapSidePosZ;
        case TextureTarget::TextureCubeMapSideNegZ:
            return TextureTargetIndex::TextureCubeMapSideNegZ;
        case TextureTarget::TextureRectangle:
            return TextureTargetIndex::TextureRectangle;
        case TextureTarget::Texture2DMS:
            return TextureTargetIndex::Texture2DMS;
        case TextureTarget::Texture2DMSArray:
            return TextureTargetIndex::Texture2DMSArray;
        case TextureTarget::TextureExternal:
            return TextureTargetIndex::TextureExternal;
        default:
            assert(false && "Caught unknown texture target.");
            return TextureTargetIndex::Error;
    }
}

} // namespace snap::rhi::backend::opengl
