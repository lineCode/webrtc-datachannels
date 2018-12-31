
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

set(PROJECT_NAME "example-server")

project(${PROJECT_NAME})
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "-fno-rtti")

# Performs searching and adding of files to source list
# Appends source files to ${${PROJECT_NAME}_SRCS}
# Appends header files to ${${PROJECT_NAME}_HEADERS}
# Appends dir (argument) to ${${PROJECT_NAME}_DIRS}
macro(addFolder dir)
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
    )
    file(${globType} header_files ABSOLUTE
        ${dir}/*.h
        ${dir}/*.hpp
    )
    LIST(APPEND ${PROJECT_NAME}_SRCS ${src_files})
    LIST(APPEND ${PROJECT_NAME}_HEADERS ${header_files})
    LIST(APPEND ${PROJECT_NAME}_DIRS ${dir})
endmacro()

# Performs searching recursively and adding of files to source list
macro(addFolderRecursive dir)
    addFolder("${dir}" "RECURSIVE")
endmacro()

# This macro lets you find executable programs on the host system
# ToDo: Tested with emscripten on windows, need more tests
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
#set(Boost_USE_STATIC_LIBS OFF) 
#set(Boost_USE_MULTITHREADED ON)  
#set(Boost_USE_STATIC_RUNTIME OFF) 
set(BOOST_ROOT /usr)
set(BOOST_INCLUDES /usr/include)
set(BOOST_LIBS /usr/lib)
findPackageCrossPlatform( Boost 1.69.0 COMPONENTS program_options filesystem regex date_time system thread graph REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

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
# set(WEBRTC_SRC_PATH "/home/denis/workspace/webrtc-checkout/src")
# set(WEBRTC_TARGET_PATH "out/release"
set(WEBRTC_SRC_PATH "/home/denis/workspace/webrtc-checkout/src")
set(WEBRTC_TARGET_PATH "out/release")
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
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/filesystem )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/config )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/lua )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net )
#addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/webrtc )
#addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/websockets )

set(THIRDPARTY_FILES
  "lib/"
  "lib/boost/"
  "lib/whereami/"
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LUA_INCLUDE_DIR}
  ${WEBRTC_INCLUDES}
  ${WEBRTC_SRC_PATH}/out/src/third_party
  ${WEBRTC_SRC_PATH}/out/src/third_party/abseil-cpp)

set(THIRDPARTY_SOURCES
  "lib/whereami/whereami.c")

set(SOURCE_FILES ${THIRDPARTY_SOURCES} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HEADERS})
message(${${PROJECT_NAME}_SRCS})
## Set global variables
#SET( ${PROJECT_NAME}_SRCS  "${${PROJECT_NAME}_SRCS}" PARENT_SCOPE )
#SET( ${PROJECT_NAME}_HEADERS  "${${PROJECT_NAME}_HEADERS}" PARENT_SCOPE )
#SET( ${PROJECT_NAME}_DIRS  "${${PROJECT_NAME}_DIRS}" PARENT_SCOPE )

# OTHER_FILES for IDE even if they have no build rules.
set(OTHER_FILES
  ".clang-format"
  "README.md"
  "assets/conf.lua")

# Group source files in folders (IDE filters)
#assign_source_group("${SOURCE_FILES};${OTHER_FILES}")

add_executable(server_target
  ${SOURCE_FILES}
  ${OTHER_FILES})

target_include_directories(server_target PUBLIC "src/")
target_include_directories(server_target SYSTEM PRIVATE
  ${THIRDPARTY_FILES})

set_source_files_properties(
  ${OTHER_FILES}
  PROPERTIES
  HEADER_FILE_ONLY TRUE
)

set_property(TARGET server_target PROPERTY CXX_STANDARD 14)
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

