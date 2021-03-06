#include "storage/path.hpp" // IWYU pragma: associated
#include "log/Logger.hpp"

#include <boost/asio.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>

// beast must be after boost includes
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

using namespace boost;

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

/**
 * Get absolute path of currently running binary.
 *
 * @return absolute path
 */
::fs::path getThisBinaryPath() { return boost::dll::program_location().c_str(); }

/**
 * Get absolute path to directory of currently running binary.
 *
 * @return absolute path
 */
::fs::path getThisBinaryDirectoryPath() {
  // may be not thread-safe boost.org/doc/libs/1_62_0/doc/html/boost_dll/f_a_q_.html
  return boost::dll::program_location().parent_path().c_str();
}

std::string getFileContents(const ::fs::path& path) {
  if (path.empty()) {
    // TODO: log
    LOG(WARNING) << "getFileContents: path.empty for " << path.c_str();
    return "";
  }

  if (!::fs::exists(path)) {
    LOG(WARNING) << "getFileContents: !::fs::exists(path) for " << path.c_str();
    // TODO: log
    return "";
  }

  // Open the stream to 'lock' the file.
  std::ifstream f{path};

  // Obtain the size of the file.
  const auto sz = ::fs::file_size(path);

  // Create a buffer.
  std::string result(sz, ' ');

  // Read the whole file into the buffer.
  f.read(result.data(), sz);

  return result;
}

} // namespace storage
} // namespace gloer
