
cmake_minimum_required( VERSION 3.13.3 FATAL_ERROR )

set( PROJECT_NAME "gloer_server_example" )
set( PROJECT_DESCRIPTION "gloer_server_example: C++17 server with WebRTC & WebSockets" )
set( ${PROJECT_NAME}_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "${PROJECT_NAME}_PROJECT_DIR" )

set ( SERVER_EXAMPLE_TARGET_EXE "gloer_server_example" )

# Generate clang compilation database
# see https://stackoverflow.com/a/31086619/10904212
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check target architecture
#if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
# message(FATAL_ERROR "${PROJECT_NAME} requires a 64bit target architecture.")
#endif()

include( ${ROOT_PROJECT_DIR}/cmake/Utils.cmake )
set_cmake_module_paths( ${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR};${${ROOT_PROJECT_NAME}_CMAKE_MODULE_PATH}" ) # from Utils.cmake

include( useGoldLinker )

include( CheckCXXCompilerFlag ) # https://cmake.org/cmake/help/v3.13/module/CheckCXXCompilerFlag.html

message(STATUS "Compiler ${CMAKE_CXX_COMPILER}, version: ${CMAKE_CXX_COMPILER_VERSION}")

set_project_version(0 0 1) # from Utils.cmake

check_cmake_build_type_selected() # from Utils.cmake

enable_colored_diagnostics() # from Utils.cmake

project( ${PROJECT_NAME}
  VERSION ${${PROJECT_NAME}_VERSION}
  DESCRIPTION ${PROJECT_DESCRIPTION}
)

print_cmake_system_info() # from Utils.cmake

check_supported_os() # from Utils.cmake

#add_subdirectory( submodules )

#include( Dependencies )

#include( ProjectFiles )

findPackageCrossPlatform(Sanitizers) # see https://github.com/arsenm/sanitizers-cmake
# sanitizer_add_blacklist_file("blacklist.txt") # https://github.com/arsenm/sanitizers-cmake#build-targets-with-sanitizer-support

add_executable( ${SERVER_EXAMPLE_TARGET_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp )

target_include_directories( ${SERVER_EXAMPLE_TARGET_EXE} PUBLIC "src/" )

target_link_libraries(${SERVER_EXAMPLE_TARGET_EXE} PRIVATE ${ROOT_PROJECT_NAME}_lib)

# IWYU detects superfluous includes and when the include can be replaced with a forward declaration.
# It can be obtained using "apt-get install iwyu" or from "github.com/include-what-you-use".
# make sure it can find Clang built-in headers (stdarg.h and friends.)
# see https://stackoverflow.com/a/30951493/10904212
option(ENABLE_IWYU "ENABLE IWYU" OFF)
if (ENABLE_IWYU)
  set(IWYU_IMP "${ROOT_PROJECT_DIR}/cmake/iwyu/iwyu.imp")
  findPackageCrossPlatform(PythonInterp) # Required by iwyu
  add_iwyu( ${SERVER_EXAMPLE_TARGET_EXE} ) # from Utils.cmake
else()
  message(WARNING "iwyu turned off")
endif()

if(USE_QT)
  if(USE_QT4)
      target_link_libraries( ${SERVER_EXAMPLE_TARGET_EXE} PRIVATE Qt4::QtGui )
  else(USE_QT4)
      target_link_libraries( ${SERVER_EXAMPLE_TARGET_EXE} PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets )
      # TODO Qt5::Core Qt5::Network Qt5::Sql Qt5::Qml Qt5::Xml
  endif(USE_QT4)

  get_target_property( QT_CORE_IMPORTED_LOCATION Qt5::Core LOCATION )
  message( "QT_CORE_IMPORTED_LOCATION=${QT_CORE_IMPORTED_LOCATION}" )
endif(USE_QT)

set_target_properties( ${SERVER_EXAMPLE_TARGET_EXE} PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  CMAKE_CXX_STANDARD_REQUIRED ON
  CMAKE_CXX_FLAGS "-fno-rtti /std:c++latest"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${PROJECT_NAME} )

if (NOT CMAKE_BUILD_TYPE MATCHES "Debug" )
  add_sanitizers( ${SERVER_EXAMPLE_TARGET_EXE} ) # https://github.com/arsenm/sanitizers-cmake#build-targets-with-sanitizer-support
endif()

add_custom_command( TARGET ${SERVER_EXAMPLE_TARGET_EXE} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/examples/server/assets/ $<TARGET_FILE_DIR:${SERVER_EXAMPLE_TARGET_EXE}>/assets )
