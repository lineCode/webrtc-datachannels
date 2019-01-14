#pragma once

#include <filesystem>
#include <string>

namespace utils {
namespace filesystem {

std::filesystem::path getThisBinaryPath();
std::filesystem::path getThisBinaryDirectoryPath();
std::string getFileContents(const std::filesystem::path& path);

} // namespace filesystem
} // namespace utils
