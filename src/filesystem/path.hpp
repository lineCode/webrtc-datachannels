#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <whereami/whereami.h>

namespace fs = std::filesystem;

namespace utils {
namespace filesystem {

fs::path getThisBinaryPath();
fs::path getThisBinaryDirectoryPath();
std::string getFileContents(const fs::path &path);

} // namespace filesystem
} // namespace utils
