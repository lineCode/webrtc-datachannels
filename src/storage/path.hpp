#pragma once

#include <string>

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

namespace gloer {
namespace storage {

fs::path getThisBinaryPath();

/**
 * @brief Path utils
 *
 * Example usage:
 *
 * @code{.cpp}
 * const ::fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();
 * @endcode
 **/
fs::path getThisBinaryDirectoryPath();

std::string getFileContents(const fs::path& path);

} // namespace storage
} // namespace gloer
