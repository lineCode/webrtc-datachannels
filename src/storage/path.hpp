#pragma once

#include <filesystem>
#include <string>

namespace gloer {
namespace storage {

std::filesystem::path getThisBinaryPath();

/**
 * @brief Path utils
 *
 * Example usage:
 *
 * @code{.cpp}
 * const ::fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();
 * @endcode
 **/
std::filesystem::path getThisBinaryDirectoryPath();

std::string getFileContents(const std::filesystem::path& path);

} // namespace storage
} // namespace gloer
