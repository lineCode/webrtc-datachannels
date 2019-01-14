# About

Based on https://github.com/brkho/client-server-webrtc-example
And https://github.com/llamerada-jp/webrtc-cpp-sample/blob/master/main.cpp
And https://github.com/shakandrew/AgarPlusPlus/blob/abbd548ab1d0e0d908778baa9366fc3a83182f88/src/webrtc_server.cpp
And https://github.com/search?p=2&q=%22CreateModularPeerConnectionFactory%22+-filename%3Apeerconnectionfactory_jni.cc+-filename%3Apeerconnectionfactory_jni.cc+-filename%3Acreatepeerconnectionfactory.cc+-filename%3Apeerconnectionfactory.cc+-filename%3Acreate_peerconnection_factory.cc+-filename%3ARTCPeerConnectionFactory.mm+-filename%3Apeerconnectioninterface.h+-filename%3Aobjccallclient.mm+-filename%3Aandroidcallclient.cc+-filename%3Apeerconnection_media_unittest.cc+-filename%3Apeerconnection_integrationtest.cc&type=Code
And https://github.com/shakandrew/AgarPlusPlus/blob/abbd548ab1d0e0d908778baa9366fc3a83182f88/src/network/WebRTCConnectionFactory.cpp
And https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp
And https://chromium.googlesource.com/external/webrtc/+/master/examples/peerconnection/server/
And https://gist.github.com/MatrixMuto/e37f50567e4b9b982dd8673a1e49dcbe
And https://github.com/notedit/webrtc-clone/tree/master

## Build webrtc && combine libs to libwebrtc_full

Based on https://docs.google.com/document/d/1J6rcqV5KWpYCZlhWv4vt8Ilrh_f08QC2KA1jbkSBo9s/edit?usp=sharing

Uses webrtc branch-heads/69, see https://chromium.googlesource.com/external/webrtc/+/branch-heads/69
Uses combine.sh from https://gist.github.com/blockspacer/6bee958df866670ae61e4340ce9b5938

mkdir -p ~/workspace/ && cd ~/workspace/
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=~/workspace/depot_tools:"$PATH"
mkdir webrtc-checkout
cd webrtc-checkout
fetch --nohooks webrtc
cd src
git checkout branch-heads/69
gclient sync
gclient runhooks
./build/install-build-deps.sh

export GYP_DEFINES="target_arch=x64 host_arch=x64 build_with_chromium=0 use_openssl=0 use_gtk=0 use_x11=0 include_examples=0 include_tests=0 fastbuild=1 remove_webcore_debug_symbols=1 include_pulse_audio=0 include_internal_video_render=0 clang=0"
rm -rf ./out/release
gn gen out/release --args='target_os="linux" enable_iterator_debugging=false is_component_build=false 
is_debug=false use_custom_libcxx=false proprietary_codecs=true use_custom_libcxx_for_host=false'
ninja -C ./out/release boringssl protobuf_lite p2p base jsoncpp -t clean
ninja -C ./out/release boringssl protobuf_lite p2p base jsoncpp
ninja -C ./out/release -t clean
ninja -C ./out/release
ls ./out/release
rm ./out/release/libwebrtc_full.a
rm -rf include
bash combine.sh
file ./out/release/libwebrtc_full.a
bash buildtest.sh

## Helpful Info

Signaling - The discovery and negotiation process of WebRTC peers is called signaling. For two devices in different networks to find each other they need to use a central service called a signaling server. Using the signaling server two devices can discover each other and exchange negotiation messages. WebRTC does not specify signaling; different technologies such as WebSockets can be used for it.

ICE Candidates - Two peers exchange ICE candidates until they find a method of communication that they both support. After the connection has been established ICE candidates can be traded again to upgrade to a better and faster communication method.

STUN Server - STUN servers are used to get an external network address and to pass firewalls.

Read https://www.scaledrone.com/blog/webrtc-chat-tutorial/

## CLONE

mkdir ~/workspace/
cd ~/workspace/
mkdir webrtc-test
cd webrtc-test/

## DEPENDENCIES

Build webrtc https://gist.github.com/blockspacer/6bee958df866670ae61e4340ce9b5938

Install latest cmake (remove old before):
version=3.13.2
mkdir ~/temp
cd ~/temp
wget https://github.com/Kitware/CMake/releases/download/v$version/cmake-$version-Linux-x86_64.sh
sudo mkdir /opt/cmake
sudo sh cmake-$version-Linux-x86_64.sh --prefix=/opt/cmake --skip-license
sudo ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake
cmake --version

Remove old boost:
sudo apt-get remove libboost-system-dev libboost-program-options-dev libboost-all-dev -y
sudo apt-get autoremove
sudo rm -rf /usr/local/lib/libboost*
sudo rm -rf /usr/local/include/boost

sudo apt install clang-format

Integrate with your IDE ( QT instructions http://doc.qt.io/qtcreator/creator-beautifier.html )
Import .clang-format rules to IDE settings.

NOTE: don`t forget to use clang-format!

Install new boost:
cd ~
wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.bz2 \
    && tar -xjf boost_1_69_0.tar.bz2 \
    && rm -rf boost_1_69_0.tar.bz2 \
    && cd boost_1_69_0 \
    && sudo ./bootstrap.sh --prefix=/usr/ \
    && sudo ./b2 link=shared install

cat /usr/include/boost/version.hpp

sudo apt-get install lua5.2 liblua5.2-dev 

# TODO USES OLD BOOST libluabind-dev

sudo apt install build-essential libreadline-dev

cd ~ && git clone https://github.com/LuaJIT/LuaJIT.git && cd LuaJIT && make && sudo make install
cd ~ && curl -R -O http://www.lua.org/ftp/lua-5.3.4.tar.gz && tar zxf lua-5.3.4.tar.gz && cd lua-5.3.4 && make linux test

## Build include-what-you-use (DEPENDENCY)
see https://github.com/include-what-you-use/include-what-you-use
see project submodules!

sudo apt-get install llvm-6.0-dev libclang-6.0-dev clang-6.0 -y

ls include-what-you-use
mkdir build-iwyu && cd build-iwyu
cmake ../include-what-you-use -DIWYU_LLVM_ROOT_PATH=/usr/lib/llvm-6.0
cmake --build .

NOTE: don`t use "bits/*" or "*/details/*" includes, add them to mappings file (.imp)

read https://llvm.org/devmtg/2010-11/Silverstein-IncludeWhatYouUse.pdf
read https://github.com/include-what-you-use/include-what-you-use/tree/master/docs
read https://github.com/hdclark/Ygor/blob/master/artifacts/20180225_include-what-you-use/iwyu_how-to.txt

## BUILD main project (from root project dir)

USE YOUR OWN WEBRTC_SRC_PATH at cmake configure step!

USE g++ (Ubuntu 8.2.0-7ubuntu1) 8.2.0

rm -rf build
mkdir build
cd build
cmake .. -DWEBRTC_SRC_PATH:STRING="/home/denis/workspace/webrtc-checkout/src" -DWEBRTC_TARGET_PATH:STRING="out/release" -DCMAKE_C_COMPILER="/usr/bin/clang-6.0" -DCMAKE_CXX_COMPILER="/usr/bin/clang++-6.0" -DBOOST_ROOT:STRING="/usr" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCLANG_PATH="/usr/lib/llvm-6.0/lib/clang/6.0.1/include" -DENABLE_IWYU=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --clean-first -- -j4
./bin/example-server

## Run client (from root project dir)

cd client
python -m SimpleHTTPServer 8081

## NOTE

You should set up your own STUN server for production use, the google server is only for development.
STUN is very simple so this shouldn’t be an issue.

WebSocket++, a header-only C++ WebSockets implementation for our pseudo-signaling server. WebSocket++ itself depends on Boost.Asio

Some corporate networks behind symmetric NAT devices cannot use STUN. This is because symmetric NAT offers additional security by not only associating a local IP with a port, but also with a destination. The NAT device will then only accept connections on the associated port from the original destination server. This means that while the STUN server can still discover the client’s NAT IP, that address is useless to other peers because only the STUN server can respond along it. NOTE: STUN and TURN are unnecessary. ICE has a concept of a host candidate

message sizes are limited in both Mozilla's and Google's implementations at the moment 

## Server-side

LUA:
sudo apt install lua5.3
yum install epel-release && yum install lua
dnf install lua

## TODO

* Cusom STUN https://webrtc.googlesource.com/src/+/master/examples/stunserver/stunserver_main.cc
* Support android https://blog.piasy.com/2017/09/03/Use-WebRTC-Static-Library/
* Support ios
* Support win64
* Send a message each 33 or 60 ms with user input data.
* Use Protocol Buffers
* Use message queue
* Run the WebSocket server as a separate thread so main process can handle the game loop.
* If the game server is not behind a NAT and it has a static IP address, STUN and TURN are unnecessary. ICE has a concept of a host candidate (https://tools.ietf.org/html/rfc5245#section-4.1.1.1), which will create a direct connection between peers using the address in the candidate with no STUN or TURN servers in between.
* Video/Audio? https://blog.discordapp.com/how-discord-handles-two-and-half-million-concurrent-voice-users-using-webrtc-ce01c3187429 && https://www.jianshu.com/p/1de3bacf9d3c
* Support large messages https://developer.mozilla.org/en-US/docs/Web/API/WebRTC_API/Using_data_channels

## TODO
SOCI https://github.com/SOCI/soci
rocksdb2 https://github.com/facebook/rocksdb
protobuf https://github.com/protocolbuffers/protobuf
folly https://trello.com/c/mCQza0wM/27-use-folly
abseil https://trello.com/c/xeTTnS0b/30-abseil-c-lib
JSON for Modern C++ https://github.com/nlohmann/json
NuDB https://github.com/vinniefalco/NuDB
Boost.System: for error codes (header only). https://github.com/Broekman/micro
spdlog: fast, header only, C++ logging library. https://github.com/Broekman/micro
receivedMessagesQ_ https://github.com/peterSweter/carSmashCpp/blob/master/networking/Session.cpp#L87
timeouts https://github.com/LeonineKing1199/foxy
coroutine? https://github.com/LeonineKing1199/foxy/blob/master/src/proxy.cpp#L176
ip spam https://github.com/zirconium-n/Tangerine/blob/master/Tangerine/sgk/general/Game.cpp#L25
IOD https://github.com/matt-42/iod
Use_the_Tools_Available! https://lefticus.gitbooks.io/cpp-best-practices/content/02-Use_the_Tools_Available.html
Astyle https://github.com/node-webrtc/node-webrtc/blob/develop/CMakeLists.txt#L447
Read https://blog.kitware.com/static-checks-with-cmake-cdash-iwyu-clang-tidy-lwyu-cpplint-and-cppcheck/
