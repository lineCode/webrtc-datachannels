# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

## Search source files in folders
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_NAME} "" ) # add main.cpp manually
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/storage ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/config ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/lua ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/log ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/algo ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net/webrtc ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net/websockets ${PROJECT_NAME} "" )

set_vs_startup_project(${PROJECT_NAME}) # from Utils.cmake

set(CLANG_PATH CACHE STRING "/usr/lib/llvm-6.0/lib/clang/6.0.1")

set( WEBRTC_INCLUDES ${WEBRTC_SRC_PATH}/include
  ${WEBRTC_SRC_PATH}/include/webrtc
  #${WEBRTC_SRC_PATH}/third_party/abseil-cpp # we use custom abseil version
  ${WEBRTC_SRC_PATH}/third_party/jsoncpp/source/include
  ${WEBRTC_SRC_PATH}/third_party/libyuv/include
  CACHE INTERNAL "WEBRTC_INCLUDES" )

set( THIRDPARTY_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/"
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/whereami/"
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LUA_INCLUDE_DIR}
  ${RANG_INCLUDE_DIR}
  ${G3LOG_INCLUDE_DIR}
  ${WEBRTC_INCLUDES}
  ${FOLLY_INCLUDE_DIR}
  ${CLANG_PATH}/include
  CACHE INTERNAL "THIRDPARTY_FILES" )

set(THIRDPARTY_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/whereami/whereami.c"
  CACHE INTERNAL "THIRDPARTY_SOURCES")

set(SOURCE_FILES ${THIRDPARTY_SOURCES} ${${PROJECT_NAME}_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ${${PROJECT_NAME}_HEADERS})

## Set global variables
#SET( ${PROJECT_NAME}_SRCS  "${${PROJECT_NAME}_SRCS}" PARENT_SCOPE )
#SET( ${PROJECT_NAME}_HEADERS  "${${PROJECT_NAME}_HEADERS}" PARENT_SCOPE )
#SET( ${PROJECT_NAME}_DIRS  "${${PROJECT_NAME}_DIRS}" PARENT_SCOPE )

# OTHER_IDE_FILES for IDE even if they have no build rules.
set(${PROJECT_NAME}_OTHER_IDE_FILES_EXTRA
  ".editorconfig"
  ".clang-format"
  ".gitignore"
  ".dockerignore"
  ".gdbinit")
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "cmake/*.cmake;cmake/*/*.*" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "assets/*.*;assets/*/*.*" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "client/*.*;client/*/*.*" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "docs/*.*;docs/*/*.*" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "scripts/*.sh;scripts/*/*.sh" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "*.md;*.yml;*.json;*.cmake;*.in;*.txt" )

# Group source files in folders (IDE filters)
#assign_source_group("${SOURCE_FILES};${${PROJECT_NAME}_OTHER_IDE_FILES_EXTRA")

# Set project test source files.
#set(TEST_SRC
#  "${TEST_SRC_PATH}/testCppbase.cpp"
#  "${TEST_SRC_PATH}/testFactorial.cpp"
#)
