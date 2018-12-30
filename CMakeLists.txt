
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(example-server)
set(CMAKE_CXX_FLAGS "-fno-rtti")
# -fno-rtti for webrtc
#set(CMAKE_CXX_FLAGS "-fno-rtti")
find_package(Threads REQUIRED)
#find_package(LibWebRTC REQUIRED)
#include(${LIBWEBRTC_USE_FILE})
#set(Boost_USE_STATIC_LIBS OFF) 
#set(Boost_USE_MULTITHREADED ON)  
#set(Boost_USE_STATIC_RUNTIME OFF) 
set(BOOST_ROOT /usr)
set(BOOST_INCLUDES /usr/include)
set(BOOST_LIBS /usr/lib)
FIND_PACKAGE( Boost 1.69.0 COMPONENTS program_options filesystem regex date_time system thread graph REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

#find_package(boost_asio)

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

add_executable(server_exe src/main.cpp src/observers.cpp src/observers.h src/dispatch_queue.cpp src/dispatch_queue.h)
target_include_directories(server_exe PUBLIC "src/")
target_include_directories(server_exe SYSTEM PUBLIC
  "lib/" "lib/boost/" 
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${WEBRTC_INCLUDES}
  ${WEBRTC_SRC_PATH}/out/src/third_party/abseil-cpp)
message(${Boost_INCLUDE_DIRS})
set_property(TARGET server_exe PROPERTY CXX_STANDARD 14)
#add_definitions(-DWEBRTC_POSIX)
target_compile_definitions(server_exe PUBLIC
  ${WEBRTC_DEFINITIONS})
set_target_properties(server_exe PROPERTIES
  OUTPUT_NAME example-server
  RUNTIME_OUTPUT_DIRECTORY /${CMAKE_BINARY_DIR}/bin/)
  #/sbin/ldconfig -p| grep libz.so
#link_libraries(${WEBRTC_LIBRARIES} boost_system pthread ssl crypto X11 dl expat libz z)
target_link_libraries(server_exe LINK_PUBLIC
  ${WEBRTC_LIBRARIES} boost_system pthread ssl crypto X11 dl expat ${Boost_LIBRARIES} z)
# https://stackoverflow.com/questions/6646405/how-do-you-add-boost-libraries-in-cmakelists-txt
#if(Boost_FOUND)
#add_executable(progname file1.cxx file2.cxx) 
#target_link_libraries(progname Boost::filesystem Boost::regex)
#endif()
# g++ -DWEBRTC_POSIX -std=gnu++11 -o simpleTest test1.cpp -Iinclude -Iinclude/webrtc -Ithird_party/abseil-cpp /home/denis/workspace/webrtc-checkout/src/    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}//libwebrtc_full.a 
# /home/denis/workspace/webrtc-checkout/src/    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}//obj/api/libjingle_peerconnection_api.a /home/denis/workspace/webrtc-checkout/src/    ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}//obj/third_party/boringssl/libboringssl.a -lssl -lcrypto -lpthread -lX11 -ldl
# -lexpat -Ithird_party/jsoncpp/source/include -Ithird_party/libyuv/include -fno-rtti -I/home/denis/workspace/webrtc-test/lib -lboost_system
