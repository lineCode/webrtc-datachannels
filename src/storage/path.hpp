#pragma once

#include <filesystem>
#include <string>

namespace gloer {
namespace storage {

std::filesystem::path getThisBinaryPath();
std::filesystem::path getThisBinaryDirectoryPath();
std::string getFileContents(const std::filesystem::path& path);
std::string fileContents(const std::filesystem::path& path);

} // namespace storage
} // namespace gloer
