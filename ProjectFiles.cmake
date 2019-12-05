# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

## Search source files in folders
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_NAME} "" ) # add main.cpp manually
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src ${PROJECT_NAME} "" ) # add main.cpp manually
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/storage ${PROJECT_NAME} "" )
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/config ${PROJECT_NAME} "" )
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/lua ${PROJECT_NAME} "" )
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/log ${PROJECT_NAME} "" )
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net ${PROJECT_NAME} "" )
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/algo ${PROJECT_NAME} "" )
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc ${PROJECT_NAME} "" )
# addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws ${PROJECT_NAME} "" )

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
  ${LIBIBERTY_INCLUDE_DIR} # used by folly
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
  CACHE INTERNAL "THIRDPARTY_SOURCES")

set(SOURCE_FILES ${THIRDPARTY_SOURCES} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HEADERS})

list(APPEND SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/algo/CallbackManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/algo/CallbackManager.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/algo/DispatchQueue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/algo/DispatchQueue.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/algo/NetworkOperation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/algo/NetworkOperation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/algo/StringUtils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/algo/StringUtils.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/algo/TickManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/algo/TickManager.hpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/src/config/ServerConfig.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/config/ServerConfig.hpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/src/log/Logger.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/log/Logger.hpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lua/LuaScript.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lua/LuaScript.hpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/path.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/path.hpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/NetworkManagerBase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/NetworkManagerBase.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/SessionBase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/SessionBase.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/SessionManagerBase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/SessionManagerBase.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ConnectionManagerBase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ConnectionManagerBase.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/SessionPair.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/SessionPair.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/core.hpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/SessionManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/SessionManager.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/Observers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/Observers.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/PeerConnectivityChecker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/PeerConnectivityChecker.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/Timer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/Timer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/WRTCServer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/WRTCServer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/WRTCSession.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/WRTCSession.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/SessionGUID.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/Callbacks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/Callbacks.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/wrtc/wrtc.hpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/client/ClientSession.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/client/ClientSession.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/client/ClientConnectionManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/client/ClientConnectionManager.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/client/ClientSessionManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/client/ClientSessionManager.hpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/server/Listener.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/server/Listener.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/server/ServerConnectionManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/server/ServerConnectionManager.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/server/ServerSession.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/server/ServerSession.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/server/ServerSessionManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/server/ServerSessionManager.hpp
  #
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/SessionGUID.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/Callbacks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ws/Callbacks.hpp

)

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
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "examples/server/assets/*.*;examples/server/assets/*/*.*" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "examples/webclient/*.*;examples/webclient/*/*.*" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "docs/*.*;docs/*/*.*;docs/Doxyfile*" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "scripts/*.sh;scripts/*/*.sh" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "*.md;*.yml;*.json;*.cmake;*.in;*.txt;*.py" )

# Group source files in folders (IDE filters)
#assign_source_group("${SOURCE_FILES};${${PROJECT_NAME}_OTHER_IDE_FILES_EXTRA")

# Set project test source files.
#set(TEST_SRC
#  "${TEST_SRC_PATH}/testCppbase.cpp"
#  "${TEST_SRC_PATH}/testFactorial.cpp"
#)
