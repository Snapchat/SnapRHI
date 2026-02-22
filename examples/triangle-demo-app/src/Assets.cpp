#include "Assets.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>

#include <fstream>
#include <string>
#include <vector>

namespace snap::rhi::demo::assets {
namespace {
static std::string joinPath(const std::string& base, const std::string& rel) {
    if (base.empty()) {
        return rel;
    }
    if (rel.empty()) {
        return base;
    }

    const bool baseEndsWithSep = (base.back() == '/' || base.back() == '\\');
    const bool relStartsWithSep = (rel.front() == '/' || rel.front() == '\\');

    if (baseEndsWithSep && relStartsWithSep) {
        return base + rel.substr(1);
    }
    if (!baseEndsWithSep && !relStartsWithSep) {
        return base + "/" + rel;
    }
    return base + rel;
}

static std::optional<std::vector<std::byte>> readFileBytesFromDisk(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        return std::nullopt;
    }

    f.seekg(0, std::ios::end);
    const std::streamoff size = f.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    f.seekg(0, std::ios::beg);

    std::vector<std::byte> out;
    out.resize(static_cast<size_t>(size));
    if (size > 0) {
        f.read(reinterpret_cast<char*>(out.data()), size);
        if (!f) {
            return std::nullopt;
        }
    }
    return out;
}

#if defined(__ANDROID__)
static std::optional<std::vector<std::byte>> readFileBytesFromAndroidAssets(const char* relativePath) {
    // SDL on Android can open APK assets via SDL_IOFromFile().
    SDL_IOStream* io = SDL_IOFromFile(relativePath, "rb");
    if (!io) {
        return std::nullopt;
    }

    // Determine size.
    const Sint64 end = SDL_SeekIO(io, 0, SDL_IO_SEEK_END);
    if (end < 0) {
        SDL_CloseIO(io);
        return std::nullopt;
    }
    if (SDL_SeekIO(io, 0, SDL_IO_SEEK_SET) < 0) {
        SDL_CloseIO(io);
        return std::nullopt;
    }

    std::vector<std::byte> out;
    out.resize(static_cast<size_t>(end));
    if (!out.empty()) {
        const size_t read = SDL_ReadIO(io, out.data(), out.size());
        SDL_CloseIO(io);
        if (read != out.size()) {
            return std::nullopt;
        }
        return out;
    }

    SDL_CloseIO(io);
    return out;
}
#endif

static std::optional<std::vector<std::byte>> readFromPackagedAssetsViaBasePath(const char* relativePath) {
    const char* basePathRaw = SDL_GetBasePath();
    if (!basePathRaw) {
        return std::nullopt;
    }

    std::string basePath(basePathRaw);

    const std::string rel(relativePath);

    // Candidate 1: <base>/assets/<rel>
    const std::string candidate1 = joinPath(joinPath(basePath, "assets"), rel);

    // Candidate 2 (macOS bundle): <bundle>/Contents/Resources/assets/<rel>
    std::string candidate2;
    {
        const std::string needle = "/Contents/MacOS/";
        const auto pos = basePath.find(needle);
        if (pos != std::string::npos) {
            std::string resourcesBase = basePath;
            resourcesBase.replace(pos, needle.size(), "/Contents/Resources/");
            candidate2 = joinPath(joinPath(resourcesBase, "assets"), rel);
        }
    }
    const std::string candidate3 = joinPath(basePath, rel);

    if (auto bytes = readFileBytesFromDisk(candidate1)) {
        return bytes;
    }
    if (!candidate2.empty()) {
        if (auto bytes = readFileBytesFromDisk(candidate2)) {
            return bytes;
        }
    }
    if (auto bytes = readFileBytesFromDisk(candidate3)) {
        return bytes;
    }

    return std::nullopt;
}
} // namespace

std::optional<std::vector<std::byte>> readRawFile(const char* relativePath) {
    if (!relativePath || relativePath[0] == '\0') {
        return std::nullopt;
    }

#if defined(__ANDROID__)
    // Android: assets are packaged into the APK and should be accessed via SDL's IO layer.
    // Our Gradle setup places demo assets at the APK root.
    if (auto bytes = readFileBytesFromAndroidAssets(relativePath)) {
        return bytes;
    }
#endif

    // Desktop + Apple platforms: assets are copied into a real folder and readable via normal file I/O.
    return readFromPackagedAssetsViaBasePath(relativePath);
}
} // namespace snap::rhi::demo::assets
