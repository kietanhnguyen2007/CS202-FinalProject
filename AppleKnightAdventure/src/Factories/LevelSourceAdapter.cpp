#include "Factories/LevelSourceAdapter.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {
std::string LowercaseExtension(const std::string& filepath) {
    std::string extension = std::filesystem::path(filepath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return extension;
}
} // namespace

bool LegacyLevelAdapter::CanLoad(const std::string& filepath) const {
    const std::string extension = LowercaseExtension(filepath);
    return extension.empty() || extension == ".lvl";
}

bool LDtkLevelAdapter::CanLoad(const std::string& filepath) const {
    return LowercaseExtension(filepath) == ".ldtk";
}
