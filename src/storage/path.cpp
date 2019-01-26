#include "storage/path.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <whereami/whereami.h>

namespace fs = std::filesystem;

namespace {

std::string getStringThisBinaryPath(std::size_t& directorySize) {
  int pathSize = 0;
  int dirPathSize = 0;
  std::string result;

  // Get length of path.
  pathSize = wai_getExecutablePath(NULL, 0, &dirPathSize);

  // Get path again.
  if (pathSize > 0) {
    result.resize(pathSize);
    wai_getExecutablePath(&result[0], pathSize, &dirPathSize);
  }

  directorySize = dirPathSize;
  return result;
}

} // anonymous namespace

namespace gloer {
namespace storage {

/**
 * Get absolute path of currently running binary.
 *
 * @return absolute path
 */
fs::path getThisBinaryPath() {
  std::size_t dirPathSize = 0;
  std::string path = getStringThisBinaryPath(dirPathSize);

  return fs::path{path};
}

/**
 * Get absolute path to directory of currently running binary.
 *
 * @return absolute path
 */
fs::path getThisBinaryDirectoryPath() {
  std::size_t dirPathSize = 0;
  std::string path = getStringThisBinaryPath(dirPathSize);

  // Remove file from path.
  if (!path.empty()) {
    path.erase(dirPathSize + 1);
  }

  return fs::path{path};
}

std::string getFileContents(const fs::path& path) {
  if (path.empty()) {
    // TODO: log
    LOG(WARNING) << "getFileContents: path.empty for " << path.c_str();
    return "";
  }

  if (!fs::exists(path)) {
    LOG(WARNING) << "getFileContents: !fs::exists(path) for " << path.c_str();
    // TODO: log
    return "";
  }

  // Open the stream to 'lock' the file.
  std::ifstream f{path};

  // Obtain the size of the file.
  const auto sz = fs::file_size(path);

  // Create a buffer.
  std::string result(sz, ' ');

  // Read the whole file into the buffer.
  f.read(result.data(), sz);

  return result;
}

std::string getFileContentsByStream(const fs::path& path) {
  if (!fs::exists(path)) {
    LOG(WARNING) << "fileContents: invalid path " << path;
    return "";
  }

  std::ifstream fileStream = std::ifstream{path.c_str()};
  return std::string{std::istreambuf_iterator<char>{fileStream}, std::istreambuf_iterator<char>{}};
}

} // namespace storage
} // namespace gloer
