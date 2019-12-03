
cmake_minimum_required( VERSION 3.13.3 FATAL_ERROR )

set( PROJECT_NAME "gloer" )
set( PROJECT_DESCRIPTION "gloer: C++17 server with WebRTC & WebSockets" )

project( ${PROJECT_NAME}
  VERSION ${${PROJECT_NAME}_VERSION}
  DESCRIPTION ${PROJECT_DESCRIPTION}
  #HOMEPAGE_URL #TODO
)

# --------------------------- conan configuration ------------------------------

# Conan package manager List of packages used by the project is container in
# conanfile.py
option(CONAN_AUTO_INSTALL "Let CMake call conan install automatically" OFF)
if (CONAN_AUTO_INSTALL)
  set(CONAN_PROFILE
      "default"
      CACHE STRING "Conan profile to use during installation")
  include(cmake/conan-auto-install.cmake)
  conan_auto_install(${CONAN_PROFILE})
endif()

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  include(${CMAKE_BINARY_DIR}/conan_paths.cmake OPTIONAL)
  conan_basic_setup(
    # prevent conan_basic_setup from resetting cmake variables
    TARGETS
    KEEP_RPATHS
    # see https://github.com/conan-io/conan/issues/6012
    NO_OUTPUT_DIRS)
else()
  message (FATAL_ERROR "must use conan")
endif()

set( ${PROJECT_NAME}_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "${PROJECT_NAME}_PROJECT_DIR" )
set( ROOT_PROJECT_NAME ${PROJECT_NAME} CACHE INTERNAL "ROOT_PROJECT_NAME" )
set( ROOT_PROJECT_DIR ${${PROJECT_NAME}_PROJECT_DIR} CACHE INTERNAL "ROOT_PROJECT_DIR" )

# Generate clang compilation database
# see https://stackoverflow.com/a/31086619/10904212
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check target architecture
#if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
# message(FATAL_ERROR "${PROJECT_NAME} requires a 64bit target architecture.")
#endif()

include( cmake/Utils.cmake )
set_cmake_module_paths( ${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ) # from Utils.cmake

include( useGoldLinker ) # option USE_GOLD_LINKER

include( CheckCXXCompilerFlag ) # https://cmake.org/cmake/help/v3.13/module/CheckCXXCompilerFlag.html

message(STATUS "Compiler ${CMAKE_CXX_COMPILER}, version: ${CMAKE_CXX_COMPILER_VERSION}")

set_project_version(0 0 1) # from Utils.cmake

check_cmake_build_type_selected() # from Utils.cmake

enable_colored_diagnostics() # from Utils.cmake

print_cmake_system_info() # from Utils.cmake

check_supported_os() # from Utils.cmake

add_subdirectory( submodules )

include( Dependencies )

include( ProjectFiles )

findPackageCrossPlatform(Sanitizers) # see https://github.com/arsenm/sanitizers-cmake
# sanitizer_add_blacklist_file("blacklist.txt") # https://github.com/arsenm/sanitizers-cmake#build-targets-with-sanitizer-support

add_library(${PROJECT_NAME}_lib STATIC
  ${SOURCE_FILES}
  ${${PROJECT_NAME}_OTHER_IDE_FILES_EXTRA}
)

set (CPPCHECK_SOURCE_FILES ${SOURCE_FILES})
include( CppCheck )

add_library( ${PROJECT_NAME}::core ALIAS ${PROJECT_NAME}_lib )

# IWYU detects superfluous includes and when the include can be replaced with a forward declaration.
# It can be obtained using "apt-get install iwyu" or from "github.com/include-what-you-use".
# make sure it can find Clang built-in headers (stdarg.h and friends.)
# see https://stackoverflow.com/a/30951493/10904212
option(ENABLE_IWYU "ENABLE IWYU" OFF)
if (ENABLE_IWYU)
  set(IWYU_IMP "${ROOT_PROJECT_DIR}/cmake/iwyu/iwyu.imp")
  findPackageCrossPlatform(PythonInterp) # Required by iwyu
  add_iwyu( ${PROJECT_NAME}_lib ) # from Utils.cmake
else()
  message(WARNING "iwyu turned off")
endif()

set_source_files_properties( ${OTHER_FILES} PROPERTIES
  HEADER_FILE_ONLY TRUE )

# ensure that dependencies build before <target> does.
set( USED_ABSL_LIBS
  absl::base
  absl::flat_hash_map
  absl::algorithm
  absl::container
  absl::debugging
  absl::hash
  absl::memory
  absl::meta
  absl::numeric
  absl::strings
  absl::synchronization
  absl::time
  absl::utility
  CACHE INTERNAL "USED_ABSL_LIBS" )

set( USED_BOOST_LIBS
  Boost::boost
  Boost::atomic
  Boost::chrono
  Boost::date_time
  Boost::filesystem
  Boost::graph
  # Boost::log
  Boost::program_options
  Boost::regex
  Boost::system
  Boost::thread
  CACHE INTERNAL "USED_BOOST_LIBS" )

option( GLTFPP_ENABLE_MODULES "Enable clang modules to increase compilation speed - only tested with clang 5.0!" OFF )
if( GLTFPP_ENABLE_MODULES AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND NOT EMSCRIPTEN )
  target_compile_options( ${PROJECT_NAME}_lib
    PRIVATE -fmodules )
endif()

# @see https://stackoverflow.com/a/46132078/10904212
message( "CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}" )
if( UNIX OR MINGW )
  target_compile_options( ${PROJECT_NAME}_lib PRIVATE
##   "$<$<CONFIG:RELEASE>:-Werror>"
   -Wall
   -Wextra
   -Wno-unused-variable
   -Wno-unused-parameter
   -Wno-deprecated
   -Wno-reorder
##    "$<$<CONFIG:RELEASE>:-Wpedantic>"
##    "$<$<CONFIG:RELEASE>:-O3>"
    #"$<$<CONFIG:DEBUG>:-fsanitize-address-use-after-scope>" # https://kristerw.blogspot.com/2018/06/useful-gcc-address-sanitizer-checks-not.html
    #"$<$<CONFIG:DEBUG>:-fsanitize=pointer-compare>"
    -fdiagnostics-color=always
##    "$<$<CONFIG:DEBUG>:-g>"
##    "$<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>" # https://github.com/google/sanitizers/wiki/AddressSanitizer#using-addresssanitizer
    )
  message( "detected compiler: UNIX OR MINGW" )
elseif(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang|AppleClang" )
  target_compile_options( ${PROJECT_NAME}_lib PRIVATE
##    "$<$<CONFIG:RELEASE>:-Werror>"
    -Wall
    -Wextra
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-deprecated
    -Wno-reorder
    #"$<$<CONFIG:DEBUG>:-fsanitize-address-use-after-scope>" # https://kristerw.blogspot.com/2018/06/useful-gcc-address-sanitizer-checks-not.html
    #"$<$<CONFIG:DEBUG>:-fsanitize=pointer-compare>"
##    "$<$<CONFIG:RELEASE>:-pedantic>"
##    "$<$<CONFIG:RELEASE>:-O3>"
    )
  message( "detected compiler: Clang" )
elseif( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
  target_compile_options( ${PROJECT_NAME}_lib PRIVATE /W4 /WX )
else()
  message( FATAL_ERROR "unsupported compiler" )
endif()

set_target_properties( ${PROJECT_NAME}_lib PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  CMAKE_CXX_STANDARD_REQUIRED ON
  CMAKE_CXX_FLAGS "-fno-rtti /std:c++latest"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${PROJECT_NAME}_lib
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>/${PROJECT_NAME}_lib )

set_property(TARGET ${PROJECT_NAME}_lib PROPERTY CXX_STANDARD 17)

if (NOT CMAKE_BUILD_TYPE MATCHES "Debug" )
##  add_sanitizers( ${PROJECT_NAME} ) # https://github.com/arsenm/sanitizers-cmake#build-targets-with-sanitizer-support
endif()

get_target_property (gtest_LOCATION gtest INTERFACE_INCLUDE_DIRECTORIES)
message( "gtest_LOCATION=${gtest_LOCATION}")

get_target_property (glog_LOCATION glog INTERFACE_INCLUDE_DIRECTORIES)
message( "glog_LOCATION=${glog_LOCATION}")

#message( FATAL_ERROR "LIBIBERTY_LIBRARY=${LIBIBERTY_LIBRARY}")

set( USED_3DPARTY_LIBS
  # custom openssl version
  ${OPENSSL_LIBRARIES}
  #
  ${LUA_LIBRARIES}
  ${G3LOG_LIBRARIES}
  ${FOLLY_LIBRARIES}
#  folly
  gflags
  pthread
#  thrift
  double-conversion
  gtest
  glog
  ${WEBRTC_LIBRARIES} # NOTE: place wrtc libs before system libs to prefer wrtc libs
  # boost libs
  ${USED_BOOST_LIBS}
  # absl libs
  ${USED_ABSL_LIBS}
  # @note: Order matters https://stackoverflow.com/a/10269201/10904212
  ${LIBIBERTY_LIBRARY} # used by folly
  CACHE INTERNAL "USED_3DPARTY_LIBS" )

set( USED_SYSTEM_LIBS
  Threads::Threads # pthread, https://cmake.org/cmake/help/v3.13/module/FindThreads.html
  ${X11_LIBRARIES} # https://cmake.org/cmake/help/v3.13/module/FindX11.html
  ${CMAKE_DL_LIBS} # https://cmake.org/cmake/help/v3.13/variable/CMAKE_DL_LIBS.html
  EXPAT::EXPAT # https://cmake.org/cmake/help/v3.13/module/FindEXPAT.html
  ZLIB::ZLIB # https://cmake.org/cmake/help/v3.13/module/FindZLIB.html
  # Custom libs
  stdc++fs # C++17 std::filesystem
  CACHE INTERNAL "USED_SYSTEM_LIBS" )

#get_target_property (OPENSSL_CRYPTO_IMPORTED_LOCATION OpenSSL::Crypto INTERFACE_INCLUDE_DIRECTORIES)
#message( "OPENSSL_CRYPTO_IMPORTED_LOCATION=${OPENSSL_CRYPTO_IMPORTED_LOCATION}")
##
#get_target_property (OPENSSL_IMPORTED_LOCATION OpenSSL::SSL INTERFACE_INCLUDE_DIRECTORIES)
#message( "OPENSSL_IMPORTED_LOCATION=${OPENSSL_IMPORTED_LOCATION}")

target_link_libraries( ${PROJECT_NAME}_lib PUBLIC
  # https://cmake.org/cmake/help/v3.10/module/FindOpenSSL.html
  # 3dparty libs
  ${USED_3DPARTY_LIBS}
  # system libs
  ${USED_SYSTEM_LIBS}
)

target_link_directories( ${PROJECT_NAME}_lib PUBLIC ${WEBRTC_LIB_PATHS} )

target_link_libraries( ${PROJECT_NAME}_lib PUBLIC
  sol2::sol2_single Catch boost_beast FakeIt nlohmann_json microsoft_gsl boost_outcome better_enums )

add_dependencies( ${PROJECT_NAME}_lib
  ${USED_ABSL_LIBS} )

get_target_property( ABSEIL_BASE_IMPORTED_LOCATION absl::base INCLUDE_DIRECTORIES )

message( "ABSEIL_BASE_IMPORTED_LOCATION=${ABSEIL_BASE_IMPORTED_LOCATION}" )

target_include_directories( ${PROJECT_NAME}_lib SYSTEM PUBLIC
  # NOTE: make sure that include dirs from conan are used first
  ${Boost_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIRS}
  ${ABSEIL_BASE_IMPORTED_LOCATION} )

target_include_directories( ${PROJECT_NAME}_lib
  PRIVATE "${ROOT_PROJECT_DIR}/src/"
  # @see about SYSTEM INTERFACE foonathan.net/blog/2018/10/17/cmake-warnings.html
  SYSTEM INTERFACE "${ROOT_PROJECT_DIR}/src/" )

target_include_directories( ${PROJECT_NAME}_lib SYSTEM PUBLIC
  ${THIRDPARTY_FILES} ) # from ProjectFiles.cmake

target_compile_definitions( ${PROJECT_NAME}_lib PUBLIC
  ${WEBRTC_DEFINITIONS} ${RAPIDJSON_DEFINITIONS} )

# Make found targets globally available.
if ( Boost_FOUND )
  set_target_properties(
    ${USED_BOOST_LIBS}
  PROPERTIES IMPORTED_GLOBAL TRUE )
endif()

option(${ROOT_PROJECT_NAME}_BUILD_TESTS "Enable tests" ON)
if(${ROOT_PROJECT_NAME}_BUILD_TESTS)
  add_subdirectory( tests )
else()
  message( WARNING "Tests turned off")
endif()

option(${ROOT_PROJECT_NAME}_BUILD_EXAMPLES "Build examples" OFF)
if(${ROOT_PROJECT_NAME}_BUILD_EXAMPLES)
  add_subdirectory( examples )
else()
  message( WARNING "Examples turned off")
endif()

option(BUILD_DOXY_DOC "build doxygen documentation" OFF)
if(BUILD_DOXY_DOC)
  add_subdirectory( docs )
endif()

#add_custom_target(ctest-cleanup
#  COMMAND ${CMAKE_COMMAND} -E remove Tests/ctest.log
#  COMMAND ${CMAKE_COMMAND} -E remove_directory Tests
#  COMMAND ${CMAKE_COMMAND} -E remove_directory Testing
#  COMMAND ${CMAKE_COMMAND} -E remove_directory test
#)

# TODO https://github.com/mmha/gltfpp/blob/master/CMakeLists.txt#L30
# option(BUILD_DOCS "Build documentation using Doxygen" ON)

# TODO GenerateClangTidyTarget https://github.com/mmha/gltfpp/blob/master/cmake/GenerateClangTidyTarget.cmake

# TODO FindSanitizers https://github.com/mmha/gltfpp/blob/master/cmake/FindSanitizers.cmake
