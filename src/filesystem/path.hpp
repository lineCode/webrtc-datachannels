#pragma once

#include <filesystem>

//#include <bits/fs_fwd.h> // for path, filesystem
//#include <string>        // for string

namespace utils {
namespace filesystem {

std::filesystem::path getThisBinaryPath();
std::filesystem::path getThisBinaryDirectoryPath();
std::string getFileContents(const std::filesystem::path& path);

} // namespace filesystem
} // namespace utils
