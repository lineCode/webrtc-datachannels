
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

set(PROJECT_NAME "example-server")

project(${PROJECT_NAME})

# The version number.
set (${PROJECT_NAME}_VERSION_MAJOR 1)
set (${PROJECT_NAME}_VERSION_MINOR 0)
message("Building ${PROJECT_NAME} project v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}")
#set(CMAKE_CXX_FLAGS "-fno-rtti")

# Performs searching and adding of files to source list
# Appends source files to ${${PROJECT_NAME}_SRCS}
# Appends header files to ${${PROJECT_NAME}_HEADERS}
# Appends dir (argument) to ${${PROJECT_NAME}_DIRS}
# Appends extra_patterns (argument) to ${${PROJECT_NAME}_EXTRA}
# Example of extra_patterns: "cmake/*.cmake;cmake/*.imp"
macro(addFolder dir prefix extra_patterns)
    set(src_files "")
    set(header_files "")
    set(globType GLOB)
    if(${ARGC} GREATER 1 AND "${ARGV1}" STREQUAL "RECURSIVE")
        set(globType GLOB_RECURSE)
    endif()
    # Note: Certain IDEs will only display files that belong to a target, so add .h files too.
    file(${globType} src_files ABSOLUTE
        ${dir}/*.c
        ${dir}/*.cc
        ${dir}/*.cpp
        ${dir}/*.asm
        ${extra_patterns}
    )
    file(${globType} header_files ABSOLUTE
        ${dir}/*.h
        ${dir}/*.hpp
        ${extra_patterns}
    )
    file(${globType} extra_files ABSOLUTE
        ${extra_patterns}
    )
    LIST(APPEND ${prefix}_SRCS ${src_files})
    LIST(APPEND ${prefix}_HEADERS ${header_files})
    LIST(APPEND ${prefix}_EXTRA ${extra_files})
    LIST(APPEND ${prefix}_DIRS ${dir})
endmacro()

# Performs searching recursively and adding of files to source list
macro(addFolderRecursive dir prefix)
    addFolder("${dir}" "${prefix}" "" "RECURSIVE")
endmacro()

# This macro lets you find executable programs on the host system
# Usefull for emscripten
macro(find_host_package)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
    find_package(${ARGN})
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
endmacro(find_host_package)

# Useful cause some systems don`t allow easy package finding
macro(findPackageCrossPlatform)
    if(EMSCRIPTEN)
        find_host_package(${ARGN})
    elseif(ANDROID)
        find_host_package(${ARGN})
    elseif(CMAKE_HOST_WIN32)
        find_package(${ARGN})
    elseif(CMAKE_HOST_UNIX)
      find_package(${ARGN})
    else()
        message( "Unknown platform, using find_package" )
        find_package(${ARGN})
    endif()
endmacro(findPackageCrossPlatform)

# Group source files in folders (IDE filters)
function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "../../" "" _source_path "${_source_path}")
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

# Enable solution folders (in IDE)
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

# New feature in CMake 3.6 lets us define the Visual Studio StartUp Project
set_property(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}
)

# Checks if PROPERTY VS_STARTUP_PROJECT works (for Visual Studio)
if ( ${CMAKE_VERSION} VERSION_LESS "3.6" AND     # feature introduced in 3.6
        ${CMAKE_GENERATOR} MATCHES "Visual Studio") # we only care if generating for VS
        message("\n")
        message(WARNING
            "Visual Studio's \"StartUp Project\" cannot be set automatically by CMake ${CMAKE_VERSION}.\n"
            "Try upgrading to CMake 3.6 or use Visual Studio's solution explorer to manually set "
            "\"${PROJECT_NAME}\" as the \"StartUp Project\".")
endif()

# Path to root dir (absolute)
get_filename_component(ABSOLUTE_ROOT_DIR "${EXA_ROOT}" ABSOLUTE)

# -fno-rtti for webrtc
#set(CMAKE_CXX_FLAGS "-fno-rtti")
findPackageCrossPlatform(Threads REQUIRED)
#findPackageCrossPlatform(LibWebRTC REQUIRED)
#include(${LIBWEBRTC_USE_FILE})
# If you have built boost statically you will need to set the Boost_USE_STATIC_LIBS CMake variable to ON
#set(Boost_USE_STATIC_LIBS OFF) 
#set(Boost_USE_MULTITHREADED ON)  
#set(Boost_USE_STATIC_RUNTIME OFF)
set(BOOST_ROOT CACHE STRING /usr)
# The FindBoost.cmake module in versions of CMake v2.8.6 and older do not know about versions of boost newer than v.1.46.1 so you need to tell the FindBoost.cmake about these versions. You do this by setting the Boost_ADDITIONAL_VERSIONS CMake variable.
set(Boost_ADDITIONAL_VERSIONS="1.69.0")
#set(BOOST_INCLUDES CACHE STRING ${BOOST_ROOT}/include)
set(BOOST_LIBS CACHE STRING ${BOOST_ROOT}/lib)
findPackageCrossPlatform( Boost 1.69.0 COMPONENTS program_options filesystem regex date_time system thread graph REQUIRED )
# INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

option(USE_LUA "Use Lua (also called 'C' Lua) includes (default)" OFF)
option(USE_LUAJIT "Use LuaJIT includes instead of 'C' Lua ones (recommended, if you're using LuaJIT, but disabled by default)" ON)
set(USE_LUA_VERSION 5.1 CACHE STRING "Set the Lua version to use (default: 5.1)")

if(USE_LUAJIT)
# Find luajit
  findPackageCrossPlatform(LuaJIT REQUIRED)
  set(USE_LUA OFF)
# / Find lua
endif()

if(USE_LUA)
# Find lua
  findPackageCrossPlatform(Lua ${USE_LUA_VERSION} EXACT REQUIRED)
# / Find lua
endif()

#findPackageCrossPlatform(boost_asio)

#if (NOT boost_asio_FOUND)
#	message (FATAL_ERROR "boost::asio not found")
#else ()
#	message (STATUS "boost::asio found")
#endif ()

# g++ -DWEBRTC_POSIX -o simpleTest test2.cpp 
# TODO https://github.com/shakandrew/AgarPlusPlus/blob/abbd548ab1d0e0d908778baa9366fc3a83182f88/CMake/FindWebRTC.cmake
set(WEBRTC_SRC_PATH CACHE STRING "/home/denis/workspace/webrtc-checkout/src")
set(WEBRTC_TARGET_PATH CACHE STRING "out/release")
message("WEBRTC_SRC_PATH=${WEBRTC_SRC_PATH}")
message("WEBRTC_TARGET_PATH=${WEBRTC_TARGET_PATH}")
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin"
  OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux"
  OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")

  set(WEBRTC_DEFINITIONS "-DWEBRTC_POSIX" CACHE INTERNAL "" FORCE)
  set(WEBRTC_LIBRARIES ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/libwebrtc_full.a
    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/obj/libwebrtc.a
    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/obj/p2p/librtc_p2p.a
    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/obj/api/libjingle_peerconnection_api.a
    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/obj/third_party/boringssl/libboringssl.a
    CACHE INTERNAL "" FORCE)

else()
# TODO
# #define WEBRTC_ANDROID 1
# #define WEBRTC_IOS 1
# #define WEBRTC_LINUX 1
# #define WEBRTC_MAC 1
# #define WEBRTC_WIN 1
# 
# #define WEBRTC_POSIX 1
message(FATAL_ERROR "System ${CMAKE_SYSTEM_NAME} not supported")

endif()

set(WEBRTC_INCLUDES ${WEBRTC_SRC_PATH}/include
  ${WEBRTC_SRC_PATH}/include/webrtc
  ${WEBRTC_SRC_PATH}/third_party/abseil-cpp
  ${WEBRTC_SRC_PATH}/third_party/jsoncpp/source/include
  ${WEBRTC_SRC_PATH}/third_party/libyuv/include)

file(GLOB PROJECT_CONFIGS CMakeLists.txt
                         .gitattributes
                         .gitignore
                         .gitmodules)

## Search source files in folders
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_NAME} "")
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src ${PROJECT_NAME} "")
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/filesystem ${PROJECT_NAME} "")
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/config ${PROJECT_NAME} "")
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/lua ${PROJECT_NAME} "")
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net ${PROJECT_NAME} "")
#addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/webrtc )
#addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/websockets )

set(CLANG_PATH CACHE STRING "/usr/lib/llvm-6.0/lib/clang/6.0.1")

set(THIRDPARTY_FILES
  "lib/"
#  "lib/boost/"
  "lib/whereami/"
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LUA_INCLUDE_DIR}
  ${WEBRTC_INCLUDES}
  ${WEBRTC_SRC_PATH}/out/src/third_party
  ${WEBRTC_SRC_PATH}/out/src/third_party/abseil-cpp
  ${CLANG_PATH}/include)

set(THIRDPARTY_SOURCES
  "lib/whereami/whereami.c")

set(SOURCE_FILES ${THIRDPARTY_SOURCES} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HEADERS})
#message(${${PROJECT_NAME}_SRCS})
## Set global variables
#SET( ${PROJECT_NAME}_SRCS  "${${PROJECT_NAME}_SRCS}" PARENT_SCOPE )
#SET( ${PROJECT_NAME}_HEADERS  "${${PROJECT_NAME}_HEADERS}" PARENT_SCOPE )
#SET( ${PROJECT_NAME}_DIRS  "${${PROJECT_NAME}_DIRS}" PARENT_SCOPE )

# OTHER_IDE_FILES for IDE even if they have no build rules.
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/cmake "${PROJECT_NAME}_OTHER_IDE_FILES" "cmake/*.cmake;cmake/*.imp")
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/assets "${PROJECT_NAME}_OTHER_IDE_FILES" "assets/*.lua")
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/client "${PROJECT_NAME}_OTHER_IDE_FILES" "client/*.js;client/*.html")
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "*.md")
set(OTHER_IDE_FILES
  ${${PROJECT_NAME}_OTHER_IDE_FILES_EXTRA}
  ".clang-format"
  ".gitignore")

# Group source files in folders (IDE filters)
#assign_source_group("${SOURCE_FILES};${OTHER_IDE_FILES}")

add_executable(server_target
  ${SOURCE_FILES}
  ${OTHER_IDE_FILES})

target_include_directories(server_target PUBLIC "src/")
target_include_directories(server_target SYSTEM PRIVATE
  ${THIRDPARTY_FILES})

set_source_files_properties(
  ${OTHER_FILES}
  PROPERTIES
  HEADER_FILE_ONLY TRUE
)

# IWYU detects superfluous includes and when the include can be replaced with a forward declaration.
# It can be obtained using "apt-get install iwyu" or from "github.com/include-what-you-use".
# make sure it can find Clang built-in headers (stdarg.h and friends.)
# see https://stackoverflow.com/a/30951493/10904212
option(ENABLE_IWYU "ENABLE IWYU" ON) # same as: set(ENABLE_IWYU ON CACHE BOOL "ENABLE IWYU")
# TODO: https://github.com/darktable-org/rawspeed/blob/d30d600bb0ec3d4a771135a459f5fb0a7b6e7649/cmake/iwyu.imp
if (ENABLE_IWYU)
  set(IWYU_IMP "${CMAKE_SOURCE_DIR}/cmake/iwyu.imp")
  message("iwyu mapping_file at ${IWYU_IMP}")

  # Generate clang compilation database
  # see https://stackoverflow.com/a/31086619/10904212
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  find_package(PythonInterp) # Required by iwyu
  find_program(iwyu_path
    NAMES include-what-you-use iwyu
    PATHS
    ${CMAKE_SOURCE_DIR}/build-iwyu
    ${CMAKE_SOURCE_DIR}/include-what-you-use/build
    ~/Library/Frameworks
    /Library/Frameworks
    /sw # Fink
    /opt/local # DarwinPorts
    /usr/local/include
    /usr/local
    /usr
    /usr/bin/
    /opt/csw # Blastwave
    /opt)
  if(NOT iwyu_path)
    message(WARNING "Could not find the program include-what-you-use")
  else()
    # see https://github.com/aferrero2707/PhotoFlow/blob/master/src/external/rawspeed/cmake/iwyu.cmake#L7
    set(iwyu_path_and_options
        ${iwyu_path}
#        -Xiwyu --transitive_includes_only
        -Xiwyu --no_comments
        -Xiwyu --mapping_file=${IWYU_IMP}
        -Xiwyu --max_line_length=120
        -Xiwyu --check_also=${CMAKE_SOURCE_DIR}/src/*
        -Xiwyu --check_also=${CMAKE_SOURCE_DIR}/src/*/*
        -Xiwyu --check_also=${CMAKE_SOURCE_DIR}/src/*/*/*
        -Xiwyu --check_also=${CMAKE_SOURCE_DIR}/src/*/*/*/*)
    #set_property(TARGET server_target PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options})
    set_property(TARGET server_target PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options})
    message("iwyu FOUND at ${iwyu_path}")
  endif()

  # run with --target 'iwyu'
  #find_program(iwyu_tool_path NAMES iwyu_tool.py)
  #if (iwyu_tool_path AND PYTHONINTERP_FOUND)
  #  add_custom_target(iwyu
  #    ALL      # Remove ALL if you don't iwyu to be run by default.
  #    COMMAND "${PYTHON_EXECUTABLE}" "${iwyu_tool_path}" -p "${CMAKE_BINARY_DIR}" -- --mapping_file=${IWYU_IMP}
  #    COMMENT "Running include-what-you-use tool"
  #    VERBATIM
  #  )
  #endif()
endif()

#add_definitions(-DWEBRTC_POSIX)
target_compile_definitions(server_target PUBLIC
  ${WEBRTC_DEFINITIONS})
set_target_properties(server_target PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  CMAKE_CXX_STANDARD_REQUIRED ON
  CMAKE_CXX_FLAGS "-fno-rtti /std:c++latest"
  RUNTIME_OUTPUT_DIRECTORY /${CMAKE_BINARY_DIR}/bin/)
  #/sbin/ldconfig -p| grep libz.so
#link_libraries(${WEBRTC_LIBRARIES} boost_system pthread ssl crypto X11 dl expat libz z)
target_link_libraries(server_target LINK_PUBLIC
  ${WEBRTC_LIBRARIES} boost_system pthread ssl crypto X11 dl expat z stdc++fs ${Boost_LIBRARIES} ${LUA_LIBRARIES})
# https://stackoverflow.com/questions/6646405/how-do-you-add-boost-libraries-in-cmakelists-txt
#if(Boost_FOUND)
#add_executable(progname file1.cxx file2.cxx) 
#target_link_libraries(progname Boost::filesystem Boost::regex)
#endif()
# g++ -DWEBRTC_POSIX -std=gnu++11 -o simpleTest test1.cpp -Iinclude -Iinclude/webrtc -Ithird_party/abseil-cpp /home/denis/workspace/webrtc-checkout/src/    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}//libwebrtc_full.a 
# /home/denis/workspace/webrtc-checkout/src/    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}//obj/api/libjingle_peerconnection_api.a /home/denis/workspace/webrtc-checkout/src/    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}//obj/third_party/boringssl/libboringssl.a -lssl -lcrypto -lpthread -lX11 -ldl
# -lexpat -Ithird_party/jsoncpp/source/include -Ithird_party/libyuv/include -fno-rtti -I/home/denis/workspace/webrtc-test/lib -lboost_system

add_custom_command(TARGET server_target PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/ $<TARGET_FILE_DIR:server_target>/assets)

#add_custom_target(project-related-files SOURCES .clang-format)

