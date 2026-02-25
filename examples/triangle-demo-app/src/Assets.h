#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace snap::rhi::demo::assets {
// Reads a file from the packaged demo-app assets.
//
// Call with paths relative to the assets root, e.g. `hello.txt` or `shaders/tri.vert`.
//
// Expected runtime layout:
// - Desktop: <exe_dir>/assets/<relativePath>
// - Apple bundle: <bundle>/Contents/Resources/assets/<relativePath> (macOS)
//               or <bundle>/.../Resources/assets/<relativePath> (iOS)
// - Android: APK assets/<relativePath> (SDL base path maps to the app's internal storage; Gradle copies these)
//
// This function is platform-independent: it resolves the base path via SDL and uses std::ifstream.
std::optional<std::vector<std::byte>> readRawFile(const char* relativePath);
} // namespace snap::rhi::demo::assets
