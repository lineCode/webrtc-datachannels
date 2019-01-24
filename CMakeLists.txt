
cmake_minimum_required( VERSION 3.13.2 FATAL_ERROR )

set( PROJECT_NAME "gloer" )
set( PROJECT_TARGET_EXE "gloer" )
set( PROJECT_DESCRIPTION "gloer: C++17 server with WebRTC & WebSockets" )
set( ROOT_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "ROOT_PROJECT_DIR" )

include( cmake/Utils.cmake )

# @see http://www.brianlheim.com/2018/04/09/cmake-cheat-sheet.html
if ( NOT CMAKE_BUILD_TYPE )
  message( FATAL_ERROR "No build type selected, do: cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..." )
  set( CMAKE_BUILD_TYPE "Release" )
else()
  message( "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}" )
endif()

set_project_version(0 0 1) # from Utils.cmake

project( ${PROJECT_NAME}
  VERSION ${${PROJECT_NAME}_VERSION}
  DESCRIPTION ${PROJECT_DESCRIPTION}
  #HOMEPAGE_URL #TODO
)

set_cmake_module_paths( "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ) # from Utils.cmake

print_cmake_system_info() # from Utils.cmake

check_supported_os() # from Utils.cmake

add_subdirectory( submodules )

add_subdirectory( submodules/abseil )

include( Dependencies.cmake )

include( ProjectFiles.cmake )

add_executable( ${PROJECT_TARGET_EXE} )

# IWYU detects superfluous includes and when the include can be replaced with a forward declaration.
# It can be obtained using "apt-get install iwyu" or from "github.com/include-what-you-use".
# make sure it can find Clang built-in headers (stdarg.h and friends.)
# see https://stackoverflow.com/a/30951493/10904212
option(ENABLE_IWYU "ENABLE IWYU" OFF) # same as: set(ENABLE_IWYU ON CACHE BOOL "ENABLE IWYU")
if (ENABLE_IWYU)
  set(IWYU_IMP "${CMAKE_SOURCE_DIR}/cmake/iwyu/iwyu.imp")

  # Generate clang compilation database
  # see https://stackoverflow.com/a/31086619/10904212
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  findPackageCrossPlatform(PythonInterp) # Required by iwyu
  add_iwyu( ${PROJECT_TARGET_EXE} ) # from Utils.cmake
else()
  message(WARNING "iwyu turned off")
endif()

target_sources( ${PROJECT_TARGET_EXE} PRIVATE
  ${SOURCE_FILES}
  ${${PROJECT_NAME}_OTHER_IDE_FILES_EXTRA} )

target_include_directories( ${PROJECT_TARGET_EXE} PRIVATE "src/" )

target_include_directories( ${PROJECT_TARGET_EXE} SYSTEM PRIVATE
  ${THIRDPARTY_FILES} ) # from ProjectFiles.cmake

set_source_files_properties( ${OTHER_FILES} PROPERTIES
  HEADER_FILE_ONLY TRUE )

if(USE_QT)
  if(USE_QT4)
      target_link_libraries( ${PROJECT_TARGET_EXE} PRIVATE Qt4::QtGui )
  else(USE_QT4)
      target_link_libraries( ${PROJECT_TARGET_EXE} PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets )
      # TODO Qt5::Core Qt5::Network Qt5::Sql Qt5::Qml Qt5::Xml
  endif(USE_QT4)

  get_target_property( QT_CORE_IMPORTED_LOCATION Qt5::Core LOCATION )
  message( "QT_CORE_IMPORTED_LOCATION=${QT_CORE_IMPORTED_LOCATION}" )
endif(USE_QT)

target_link_libraries( ${PROJECT_TARGET_EXE} PRIVATE
  Catch nlohmann_json microsoft_gsl boost_outcome better_enums )

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

add_dependencies( ${PROJECT_TARGET_EXE}
  ${USED_ABSL_LIBS} )

get_target_property( ABSEIL_BASE_IMPORTED_LOCATION absl::base INCLUDE_DIRECTORIES )
message( "ABSEIL_BASE_IMPORTED_LOCATION=${ABSEIL_BASE_IMPORTED_LOCATION}" )
target_include_directories( ${PROJECT_TARGET_EXE} SYSTEM PRIVATE
  ${ABSEIL_BASE_IMPORTED_LOCATION} )

set( USED_BOOST_LIBS
  Boost::boost
  Boost::filesystem
  Boost::regex
  Boost::date_time
  Boost::system
  Boost::thread
  Boost::graph
  CACHE INTERNAL "USED_BOOST_LIBS" )

option( GLTFPP_ENABLE_MODULES "Enable clang modules to increase compilation speed - only tested with clang 5.0!" OFF )
if( GLTFPP_ENABLE_MODULES AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND NOT EMSCRIPTEN )
  target_compile_options( ${PROJECT_TARGET_EXE}
    PRIVATE -fmodules )
endif()

# @see https://stackoverflow.com/a/46132078/10904212
message( "CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}" )
if( UNIX OR MINGW )
  target_compile_options( ${PROJECT_TARGET_EXE} PRIVATE
    -Wall -Wextra "$<$<CONFIG:RELEASE>:-Wpedantic>" "$<$<CONFIG:RELEASE>:-O3>" -fdiagnostics-color=always "$<$<CONFIG:DEBUG>:-g>" )
  message( "detected compiler: UNIX OR MINGW" )
elseif(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang|AppleClang" )
  target_compile_options( ${PROJECT_TARGET_EXE} PRIVATE
    -Werror -Wall -Wextra "$<$<CONFIG:RELEASE>:-pedantic>" "$<$<CONFIG:RELEASE>:-O3>" )
  message( "detected compiler: Clang" )
elseif( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
  target_compile_options( ${PROJECT_TARGET_EXE} PRIVATE /W4 /WX )
else()
  message( FATAL_ERROR "unsupported compiler" )
endif()

target_compile_definitions( ${PROJECT_TARGET_EXE} PRIVATE
  ${WEBRTC_DEFINITIONS} ${RAPIDJSON_DEFINITIONS} )

set_target_properties( ${PROJECT_TARGET_EXE} PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  CMAKE_CXX_STANDARD_REQUIRED ON
  CMAKE_CXX_FLAGS "-fno-rtti /std:c++latest"
  #IMPORTED_LOCATION "/opt/Qt${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/gcc_64/lib"
  #INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/libbar"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${PROJECT_NAME} )
# TODO ARCHIVE_OUTPUT_DIRECTORY
# TODO LIBRARY_OUTPUT_DIRECTORY

target_link_directories( ${PROJECT_TARGET_EXE} PUBLIC ${WEBRTC_LIB_PATHS} )

# linking for C++17 std::filesystem
# TODO target_link_libraries(${PROJECT_TARGET_EXE} PRIVATE $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)

set( USED_3DPARTY_LIBS
  ${LUA_LIBRARIES}
  ${G3LOG_LIBRARIES}
  ${FOLLY_LIBRARIES}
  ${WEBRTC_LIBRARIES} # NOTE: place wrtc libs before system libs to prefer wrtc libs
  # boost libs
  ${USED_BOOST_LIBS}
  # absl libs
  ${USED_ABSL_LIBS}
  CACHE INTERNAL "USED_3DPARTY_LIBS" )

set( USED_SYSTEM_LIBS
  Threads::Threads # pthread, https://cmake.org/cmake/help/v3.13/module/FindThreads.html
  OpenSSL::SSL # https://cmake.org/cmake/help/v3.10/module/FindOpenSSL.html
  OpenSSL::Crypto # https://cmake.org/cmake/help/v3.10/module/FindOpenSSL.html
  ${X11_LIBRARIES} # https://cmake.org/cmake/help/v3.13/module/FindX11.html
  ${CMAKE_DL_LIBS} # https://cmake.org/cmake/help/v3.13/variable/CMAKE_DL_LIBS.html
  EXPAT::EXPAT # https://cmake.org/cmake/help/v3.13/module/FindEXPAT.html
  ZLIB::ZLIB # https://cmake.org/cmake/help/v3.13/module/FindZLIB.html
  # Custom libs
  stdc++fs # C++17 std::filesystem
  CACHE INTERNAL "USED_SYSTEM_LIBS" )

target_link_libraries( ${PROJECT_TARGET_EXE} PUBLIC
  # 3dparty libs
  ${USED_3DPARTY_LIBS}
  # system libs
  ${USED_SYSTEM_LIBS}
)

# Make found targets globally available.
if ( Boost_FOUND )
  set_target_properties(
    ${USED_BOOST_LIBS}
  PROPERTIES IMPORTED_GLOBAL TRUE )
endif ()

# if(USE_QT)
# TODO: copy QT shared libs
# @SEE https://github.com/mjoppich/bioGUI/blob/master/CMakeLists.txt#L453
# endif(USE_QT)

add_custom_command( TARGET ${PROJECT_TARGET_EXE} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/ $<TARGET_FILE_DIR:${PROJECT_TARGET_EXE}>/assets )

add_subdirectory( tests )

#option(BUILD_TESTS "Enable tests" ON)
#if(BUILD_TESTS)
#  message( "${PROJECT_NAME} testing enabled" )
#  #enable_testing()
#  add_subdirectory( tests )
#
#  # Force running test after build (after each change in source files)
#  #add_custom_command(TARGET ${PROJECT_NAME}
#  #  POST_BUILD
#  #  COMMAND  ${CMAKE_COMMAND} --build . --target ctest-cleanup
#  #  DEPENDS ${PROJECT_NAME}
#  #)
#
#  # testing binary
#  #add_executable(cpp_test tests/main.cpp)
#  #target_link_libraries(cpp_test Catch)
#  #
#  ## enable testing functionality
#  #enable_testing()
#  #
#  ## define test
#  #add_test(
#  #  NAME catch_test
#  #  COMMAND $<TARGET_FILE:cpp_test> --success
#  #  )
#
#endif()

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
