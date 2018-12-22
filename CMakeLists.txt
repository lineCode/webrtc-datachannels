cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(example-server)
# -fno-rtti for webrtc
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
find_package(Threads REQUIRED)
#find_package(LibWebRTC REQUIRED)
#include(${LIBWEBRTC_USE_FILE})
FIND_PACKAGE( Boost 1.67.0 COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

# TODO https://github.com/shakandrew/AgarPlusPlus/blob/abbd548ab1d0e0d908778baa9366fc3a83182f88/CMake/FindWebRTC.cmake
# set(WEBRTC_SRC_PATH "/home/denis/workspace/webrtc-checkout/src")
# set(WEBRTC_TARGET_PATH "out/release")
message("WEBRTC_SRC_PATH=${WEBRTC_SRC_PATH}")
message("WEBRTC_TARGET_PATH=${WEBRTC_TARGET_PATH}")
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin"
  OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux"
  OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")

  set(WEBRTC_DEFINITIONS "-DWEBRTC_POSIX")
  set(WEBRTC_LIBRARIES
    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/libwebrtc_full.a
    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/obj/api/libjingle_peerconnection_api.a
    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/obj/third_party/boringssl/libboringssl.a)

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

add_executable(server_exe src/main.cpp src/observers.h)
target_include_directories(server_exe PRIVATE "src/")
target_include_directories(server_exe SYSTEM PRIVATE
  "lib/" 
  ${CMAKE_CURRENT_BINARY_DIR}
  ${WEBRTC_INCLUDES})
set_property(TARGET server_exe PROPERTY CXX_STANDARD 14)
#add_definitions(-DWEBRTC_POSIX)
target_compile_definitions(server_exe PRIVATE
  ${WEBRTC_DEFINITIONS})
set_target_properties(server_exe PROPERTIES
  OUTPUT_NAME example-server
  RUNTIME_OUTPUT_DIRECTORY /${CMAKE_BINARY_DIR}/bin/)
#link_libraries(${WEBRTC_LIBRARIES} boost_system pthread ssl crypto X11 dl expat)
target_link_libraries(server_exe LINK_PUBLIC
  ${WEBRTC_LIBRARIES} boost_system pthread ssl crypto X11 dl expat ${Boost_LIBRARIES})

# g++ -DWEBRTC_POSIX -std=gnu++11 -o simpleTest test1.cpp -Iinclude -Iinclude/webrtc -Ithird_party/abseil-cpp ./out/release/libwebrtc_full.a 
# ./out/release/obj/api/libjingle_peerconnection_api.a ./out/release/obj/third_party/boringssl/libboringssl.a -lssl -lcrypto -lpthread -lX11 -ldl
# -lexpat -Ithird_party/jsoncpp/source/include -Ithird_party/libyuv/include -fno-rtti -I/home/denis/workspace/webrtc-test/lib -lboost_system
