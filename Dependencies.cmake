# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

# Include script to build external libraries with CMake.
include(ExternalProject)

add_subdirectory( submodules )

set(USE_QT OFF CACHE BOOL "use Qt")
  if(USE_QT)
  # The AUTOMOC target property controls whether cmake(1) inspects the C++ files in the target to determine if they require moc to be run, and to create rules to execute moc at the appropriate time. If a Q_OBJECT or Q_GADGET macro is found in a header file, moc will be run on the file.
  set(CMAKE_AUTOMOC ON)
  # Find includes in corresponding build directories
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  # Create code from a list of Qt designer ui files
  set(CMAKE_AUTOUIC ON)

  set(USE_QT4 OFF CACHE BOOL "use Qt4 or Qt5")

  # make sure correct version installed!
  if(USE_QT4)
    set(QT_VERSION_MAJOR 4.7)
    set(QT_VERSION_MINOR .0)
  else(USE_QT4)
    set(QT_VERSION_MAJOR 5.12)
    set(QT_VERSION_MINOR .0)
  endif(USE_QT4)

  # The easiest way to use CMake is to set the CMAKE_PREFIX_PATH environment variable to the install prefix of Qt 5.
  # https://stackoverflow.com/a/31223997/10904212
  prepend(CMAKE_PREFIX_PATH /opt/Qt${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/gcc_64)
  message("CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
  add_qt() # from Utils.cmake
endif(USE_QT)

findPackageCrossPlatform(Threads REQUIRED)

set( BOOST_VERSION 1.69.0 )

# If you have built boost statically you will need to set the Boost_USE_STATIC_LIBS CMake variable to ON
# set( Boost_USE_STATIC_LIBS FALSE )
# set( Boost_USE_STATIC_RUNTIME FALSE )
# set( Boost_USE_MULTITHREADED TRUE )
set( BOOST_ROOT CACHE STRING /usr )
set( Boost_ADDITIONAL_VERSIONS "${BOOST_VERSION}" )
set( BOOST_LIBS CACHE STRING ${BOOST_ROOT}/lib )
# set( Boost_COMPILER "-gcc" )
findPackageCrossPlatform( Boost 1.69.0
  COMPONENTS program_options filesystem regex date_time system thread graph
  EXACT REQUIRED )

# Make found targets globally available.
if ( Boost_FOUND )
  set_target_properties(
    Boost::boost
    Boost::filesystem
    Boost::regex
    Boost::date_time
    Boost::system
    Boost::thread
    Boost::graph
  PROPERTIES IMPORTED_GLOBAL TRUE )
endif ()

option(USE_LUA "Use Lua (also called 'C' Lua) includes (default)" OFF)
option(USE_LUAJIT "Use LuaJIT includes instead of 'C' Lua ones (recommended, if you're using LuaJIT, but disabled by default)" ON)
set(USE_LUA_VERSION 5.1 CACHE STRING "Set the Lua version to use (default: 5.1)")

if(USE_LUAJIT)
  findPackageCrossPlatform( LuaJIT REQUIRED )
  set(USE_LUA OFF)
endif()

option(USE_FOLLY "Use facebook/folly library (Apache License 2.0)" ON)
if(USE_FOLLY)
  findPackageCrossPlatform( Folly REQUIRED )
endif()

if(USE_LUA)
  findPackageCrossPlatform( Lua ${USE_LUA_VERSION} EXACT REQUIRED )
endif()

option(USE_G3LOG "Use g3log logger" ON)
add_g3log() # from Utils.cmake

option(USE_ABSEIL "Use abseil" ON)
add_abseil() # from Utils.cmake

option(USE_RANG "Use RANG for coloring terminal" ON)
add_rang() # from Utils.cmake

# TODO https://github.com/shakandrew/AgarPlusPlus/blob/abbd548ab1d0e0d908778baa9366fc3a83182f88/CMake/FindWebRTC.cmake
set(WEBRTC_SRC_PATH CACHE STRING "WEBRTC_SRC_PATH_HERE")
set(WEBRTC_TARGET_PATH CACHE STRING "WEBRTC_TARGET_PATH_HERE")
add_webrtc() # from Utils.cmake

## -------------------------------
#
## https://computing.llnl.gov/tutorials/pthreads
## Required for GTest.
#
#if(TEST_FRAMEWORK STREQUAL "GTest")
#  find_package (Threads REQUIRED)
#endif(TEST_FRAMEWORK STREQUAL "GTest")
#
## -------------------------------
#
## -------------------------------
#
## Set up test framework
## Frameworks available: Catch2, GTest
## Catch2: https://github.com/catchorg/Catch2
## GTest: hhttps://github.com/google/googletest
#
#
#if(BUILD_TESTS)
#  if(NOT BUILD_DEPENDENCIES)
#    if(TEST_FRAMEWORK STREQUAL "Catch2")
#      find_package(Catch2)
#      set(TEST_FRAMEWORK_INCLUDE_DIRS ${CATCH2_INCLUDE_DIRS})
#      set(TEST_FRAMEWORK_LIB Catch2::Catch2)
#    elseif(TEST_FRAMEWORK STREQUAL "GTest")
#      set(GTEST_ROOT "${PROJECT_PATH};${PROJECT_PATH}/googletest;${PROJECT_PATH}/googletest/googletest;${PROJECT_PATH}/..;${PROJECT_PATH}/../googletest;${PROJECT_PATH}/../googletest/googletest"
#          CACHE PATH "Path to googletest")
#      find_package(GTest)
#      set(TEST_FRAMEWORK_INCLUDE_DIRS ${GTEST_INCLUDE_DIRS})
#      set(TEST_FRAMEWORK_LIB GTest::Main)
#    endif(TEST_FRAMEWORK STREQUAL "Catch2")
#  endif(NOT BUILD_DEPENDENCIES)
#
#  if((TEST_FRAMEWORK STREQUAL "Catch2") AND (NOT CATCH2_FOUND))
#    message(STATUS "Catch2 will be downloaded when ${CMAKE_PROJECT_NAME} is built")
#    ExternalProject_Add(catch2-lib
#      PREFIX ${EXTERNAL_PATH}/Catch2
#      #--Download step--------------
#      URL https://github.com/catchorg/Catch2/archive/master.zip
#      TIMEOUT 30
#      #--Update/Patch step----------
#      UPDATE_COMMAND ""
#      PATCH_COMMAND ""
#      #--Configure step-------------
#      CONFIGURE_COMMAND ""
#      #--Build step-----------------
#      BUILD_COMMAND ""
#      #--Install step---------------
#      INSTALL_COMMAND ""
#      #--Output logging-------------
#      LOG_DOWNLOAD ON
#    )
#    ExternalProject_Get_Property(catch2-lib source_dir)
#    set(TEST_FRAMEWORK_INCLUDE_DIRS ${source_dir}/single_include CACHE INTERNAL "Path to include folder for Catch2")
#    set(TEST_FRAMEWORK_LIB catch2-lib)
#  endif((TEST_FRAMEWORK STREQUAL "Catch2") AND (NOT CATCH2_FOUND))
#
#  if((TEST_FRAMEWORK STREQUAL "GTest") AND (NOT GTEST_FOUND))
#    message(STATUS "GTest will be downloaded when ${CMAKE_PROJECT_NAME} is built")
#    ExternalProject_Add(gtest-lib
#      PREFIX ${EXTERNAL_PATH}/GTest
#      #--Download step--------------
#      URL https://github.com/google/googletest/archive/master.zip
#      TIMEOUT 30
#      #--Update/Patch step----------
#      UPDATE_COMMAND ""
#      PATCH_COMMAND ""
#      #--Configure step-------------
#      SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/gtest
#      CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} ${CMAKE_CURRENT_BINARY_DIR}/gtest/googletest
#      #--Build step-----------------
#      BUILD_IN_SOURCE 1
#      #--Install step---------------
#      INSTALL_COMMAND ""
#      #--Output logging-------------
#      LOG_DOWNLOAD ON
#    )
#    ExternalProject_Get_Property(gtest-lib source_dir binary_dir)
#    add_library(gtest_static INTERFACE)
#    add_dependencies(gtest_static gtest-lib)
#    target_link_libraries(gtest_static
#        INTERFACE "${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main.a"
#                  "${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a")
#    target_include_directories(gtest_static INTERFACE "${source_dir}/googletest/include")
#    set(TEST_FRAMEWORK_INCLUDE_DIRS ${source_dir}/googletest/include CACHE INTERNAL "Path to include folder for GTest")
#    set(TEST_FRAMEWORK_LIB gtest_static)
#    link_directories("${binary_dir}/lib")
#  endif((TEST_FRAMEWORK STREQUAL "GTest") AND (NOT GTEST_FOUND))
#
#  if(NOT APPLE)
#    include_directories(SYSTEM AFTER "${TEST_FRAMEWORK_INCLUDE_DIRS}")
#  else(APPLE)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${TEST_FRAMEWORK_INCLUDE_DIRS}\"")
#  endif(NOT APPLE)
#endif(BUILD_TESTS)
#
## -------------------------------
