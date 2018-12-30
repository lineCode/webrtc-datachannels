#include "filesystem/path.hpp"
#include <iostream>

namespace {

std::string getStringThisBinaryPath(std::size_t &directorySize) {
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

namespace utils {
namespace filesystem {

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

std::string getFileContents(const fs::path &path) {
  if (path.empty()) {
    // TODO: log
    std::cout << "getFileContents: path.empty for " << path.c_str() << "\n";
    return "";
  }

  if (!fs::exists(path)) {
    std::cout << "getFileContents: !fs::exists(path) for " << path.c_str()
              << "\n";
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

} // namespace filesystem
} // namespace utils
