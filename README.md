WORK IN PROGRESS! This library is not finished yet.

# About

Based on https://github.com/brkho/client-server-webrtc-example
And http://www.stormbrewers.com/blog/webrtc-data-channels-without-signaling-aka-hacking-the-crap-out-of-webrtc/
And https://github.com/llamerada-jp/webrtc-cpp-sample/blob/master/main.cpp
And https://github.com/shakandrew/AgarPlusPlus/blob/abbd548ab1d0e0d908778baa9366fc3a83182f88/src/webrtc_server.cpp
And https://github.com/search?p=2&q=%22CreateModularPeerConnectionFactory%22+-filename%3Apeerconnectionfactory_jni.cc+-filename%3Apeerconnectionfactory_jni.cc+-filename%3Acreatepeerconnectionfactory.cc+-filename%3Apeerconnectionfactory.cc+-filename%3Acreate_peerconnection_factory.cc+-filename%3ARTCPeerConnectionFactory.mm+-filename%3Apeerconnectioninterface.h+-filename%3Aobjccallclient.mm+-filename%3Aandroidcallclient.cc+-filename%3Apeerconnection_media_unittest.cc+-filename%3Apeerconnection_integrationtest.cc&type=Code
And https://github.com/shakandrew/AgarPlusPlus/blob/abbd548ab1d0e0d908778baa9366fc3a83182f88/src/network/WebRTCConnectionFactory.cpp
And https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp
And https://chromium.googlesource.com/external/webrtc/+/master/examples/peerconnection/server/
And https://gist.github.com/MatrixMuto/e37f50567e4b9b982dd8673a1e49dcbe
And https://github.com/notedit/webrtc-clone/tree/master

TODO http://www.stormbrewers.com/blog/webrtc-data-channels-without-signaling-aka-hacking-the-crap-out-of-webrtc/

# Supported server platforms

Ubuntu

TODO: https://github.com/hamfirst/StormWebrtc

TODO: WATCHDOG: RESTARTS ON CRASH/HANG

TODO: http://ithare.com/64-network-dos-and-donts-for-multi-player-game-developers-part-viia-security-tls-ssl/ & https://gamedev.stackexchange.com/questions/20529/using-the-dtls-protocol-for-multiplayer-games

TODO: encrypt JWT? http://ithare.com/udp-for-games-security-encryption-and-ddos-protection/

TODO: backup http://ithare.com and https://www.gamedev.net/profile/229878-no-bugs-hare/

FIND INFO FROM https://www.amazon.com/Massively-Multiplayer-Development-Charles-River/dp/1584503904

BOOKS
https://www.amazon.com/Distributed-Systems-Maarten-van-Steen/dp/1543057381/ref=sr_1_5?ie=UTF8&qid=1547678003&sr=8-5&keywords=Andrew+Tannenbaum
https://www.amazon.com/Computer-Networks-Tanenbaum-International-2010-01-09/dp/B01FJ1CHEG/ref=sr_1_18?ie=UTF8&qid=1547678050&sr=8-18&keywords=Andrew+Tannenbaum

# Supported client platforms

TODO: godot https://github.com/godotengine/godot/pull/14888 & http://docs.godotengine.org/en/latest/classes/class_websocketclient.html#description

# Clone server

mkdir ~/workspace/
cd ~/workspace/
mkdir webrtc-test
cd webrtc-test/

git clone https://gitlab.com/derofim/webrtc-test.git
git submodule update --init --recursive --depth 50

TODO: rename from 'webrtc-test' and change in license 'webrtc-test'
TODO: update THIRD_PARTY_LICENSES.md
TODO: add license on top of src files

TODO: remove “copy left” deps https://en.wikipedia.org/wiki/Comparison_of_free_and_open-source_software_licenses

NOTE:
WEBRTC - Some software frameworks, voice and video codecs require end-users, distributors and manufacturers to pay patent royalties to use the intellectual property within the software technology and/or codec. Google is not charging royalties for WebRTC and its components including the codecs it supports (VP8 for video and iSAC and iLBC for audio). For more information, see the License page.
LUA - MIT
LuaJIT - MIT

TODO: remove QT
QT - If you dynamically link the library, you do not need to share any of your source code
https://stackoverflow.com/a/41642717

## Build webrtc && combine libs to libwebrtc_full

Based on https://docs.google.com/document/d/1J6rcqV5KWpYCZlhWv4vt8Ilrh_f08QC2KA1jbkSBo9s/edit?usp=sharing

Uses webrtc branch-heads/69, see https://chromium.googlesource.com/external/webrtc/+/branch-heads/69

sudo apt-get update && sudo apt-get -y install git python
sudo apt-get install clang-4.0 build-essential
sudo apt-get install libssl-dev libcurl4

NOTE: change git config to YOURS:
git config --global user.name "John Doe"
git config --global user.email "jdoe@email.com"
git config --global core.autocrlf false
git config --global core.filemode false
git config --global color.ui true

bash scripts/clone_fresh_webrtc.sh

bash scripts/build_fresh_webrtc.sh

bash scripts/combine_webrtc_libs.sh

bash scripts/test_webrtc_p2p.sh

## Helpful Info

Signaling - The discovery and negotiation process of WebRTC peers is called signaling. For two devices in different networks to find each other they need to use a central service called a signaling server. Using the signaling server two devices can discover each other and exchange negotiation messages. WebRTC does not specify signaling; different technologies such as WebSockets can be used for it.

ICE Candidates - Two peers exchange ICE candidates until they find a method of communication that they both support. After the connection has been established ICE candidates can be traded again to upgrade to a better and faster communication method.

STUN Server - STUN servers are used to get an external network address and to pass firewalls.

Read https://www.scaledrone.com/blog/webrtc-chat-tutorial/

## DEPENDENCIES

### cmake

Install latest cmake (remove old before):
bash scripts/install_cmake.sh

sudo apt install clang-format

Integrate with your IDE ( QT instructions http://doc.qt.io/qtcreator/creator-beautifier.html )
Import .clang-format rules to IDE settings.

NOTE: don`t forget to use clang-format!

TODO: add helper scripts for clang-format

### boost

Remove old boost:
sudo apt-get remove libboost-system-dev libboost-program-options-dev libboost-all-dev -y
sudo apt-get autoremove
sudo rm -rf /usr/local/lib/libboost*
sudo rm -rf /usr/local/include/boost

Install new boost:
cd ~
wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.bz2 \
    && tar -xjf boost_1_69_0.tar.bz2 \
    && rm -rf boost_1_69_0.tar.bz2 \
    && cd boost_1_69_0 \
    && sudo ./bootstrap.sh --prefix=/usr/ \
    && sudo ./b2 link=shared install

cat /usr/include/boost/version.hpp

### lua and LuaJIT

sudo apt-get install lua5.3 liblua5.3-dev

# TODO USES OLD BOOST libluabind-dev

sudo apt install build-essential libreadline-dev

cd ~ && git clone https://github.com/LuaJIT/LuaJIT.git && cd LuaJIT && make && sudo make install
cd ~ && curl -R -O http://www.lua.org/ftp/lua-5.3.4.tar.gz && tar zxf lua-5.3.4.tar.gz && cd lua-5.3.4 && make linux test

LUA:
sudo apt install lua5.3
OR
yum install epel-release && yum install lua
OR
dnf install lua

### include-what-you-use

see https://github.com/include-what-you-use/include-what-you-use
see project submodules!

sudo apt-get install llvm-6.0-dev libclang-6.0-dev clang-6.0 -y

bash scripts/build_iwyu.sh

NOTE: change -DIWYU_LLVM_ROOT_PATH=/usr/lib/llvm-6.0 in build_iwyu.sh

NOTE: For Clang on Windows read https://metricpanda.com/rival-fortress-update-27-compiling-with-clang-on-windows

NOTE: don`t use "bits/*" or "*/details/*" includes, add them to mappings file (.imp)

read https://llvm.org/devmtg/2010-11/Silverstein-IncludeWhatYouUse.pdf
read https://github.com/include-what-you-use/include-what-you-use/tree/master/docs
read https://github.com/hdclark/Ygor/blob/master/artifacts/20180225_include-what-you-use/iwyu_how-to.txt

### QT

bash scripts/setup_qt5.sh

Install at /opt/Qt5.12.0 (paste your version) and check "Qt" checkbox!

# After the installation is complete update the database used by locate so that CMake can immediately find QT.
sudo updatedb

sudo apt-get install libfontconfig1 mesa-common-dev

# ls /opt/Qt5.12.0/
# export PATH=/opt/Qt5.12.0/5.12.0/gcc_64/bin/:$PATH

# TODO USE sudo ./qt-opensource-linux-x64-5.12.0.run --verbose -platform minimal --script https://github.com/mjoppich/bioGUI/blob/master/silent_qt_install.qs

(OPTIONALLY) fix CMAKE_PREFIX_PATH:
set(CMAKE_PREFIX_PATH "/home/qt-everywhere-opensource-src-5.6.0/qtbase")

### g3log

bash scripts/install_g3log.sh

For windows: https://github.com/KjellKod/g3log#building-on-windows

### folly

READ: https://github.com/facebook/folly#build-notes

NOTE: folly requires gcc 5.1+ and a version of boost compiled with C++14 support.

Install Gtest:
bash scripts/setup_gtest.sh

NOTE: we use custom boost and cmake versions (see above)

sudo apt-get install \
    libevent-dev \
    libdouble-conversion-dev \
    libgoogle-glog-dev \
    libgflags-dev \
    libiberty-dev \
    liblz4-dev \
    liblzma-dev \
    libsnappy-dev \
    zlib1g-dev \
    binutils-dev \
    libjemalloc-dev \
    libssl-dev \
    pkg-config \
    libunwind8-dev \
    libelf-dev \
    libdwarf-dev

From root project dir:

bash scripts/install_folly.sh

## BUILD main project (from root project dir)

USE YOUR OWN WEBRTC_SRC_PATH at cmake configure step!

Tested on g++ (Ubuntu 8.2.0-7ubuntu1) 8.2.0

bash scripts/rebuild_clean.sh

OR

bash scripts/build_project.sh

OR

bash scripts/release_project.sh

## RUN main project (from root project dir)

./build/bin/gloer

# GDB (from root project dir)

Read https://metricpanda.com/tips-for-productive-debugging-with-gdb

gdb build/bin/gloer -iex 'add-auto-load-safe-path .'

Then type:
r

See http://www.yolinux.com/TUTORIALS/GDB-Commands.html

## Run client (from root project dir)

bash scripts/run_html_client.sh

open http://localhost:8081/example-client.html

## NOTE about STUN server

You should set up your own STUN server for production use, the google server is only for development.
STUN is very simple so this shouldn’t be an issue.

WebSocket++, a header-only C++ WebSockets implementation for our pseudo-signaling server. WebSocket++ itself depends on Boost.Asio

Some corporate networks behind symmetric NAT devices cannot use STUN. This is because symmetric NAT offers additional security by not only associating a local IP with a port, but also with a destination. The NAT device will then only accept connections on the associated port from the original destination server. This means that while the STUN server can still discover the client’s NAT IP, that address is useless to other peers because only the STUN server can respond along it. NOTE: STUN and TURN are unnecessary. ICE has a concept of a host candidate

message sizes are limited in both Mozilla's and Google's implementations at the moment 

## Unit testing

We use Catch2 and [BDD-Style](https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#bdd-style)

Read https://helpercode.com/2017/08/10/getting-started-with-c-unit-testing/

Note:
* Currently assertions in Catch are not thread safe https://github.com/catchorg/Catch2/blob/master/docs/assertions.md#thread-safety

We use FakeIt, see https://github.com/eranpeer/FakeIt#limitations and https://github.com/eranpeer/FakeIt/wiki/Quickstart

Note:
* FakeIt can't mock classes with multiple inheritance.
* FakeIt can't mock classes with virtual inheritance.
* FakeIt currently mocks are not thread safe.
* Prefer
When(Method(mock,foo)).AlwaysReturn(1);
over
Method(mock,foo) = 1;

## Versioning

This library follows [Semantic Versioning](http://semver.org/). Please note it
is currently under active development. Any release versioned 0.x.y is subject to
backwards-incompatible changes at any time.

**GA**: Libraries defined at a GA quality level are expected to be stable and
all updates in the libraries are guaranteed to be backwards-compatible. Any
backwards-incompatible changes will lead to the major version increment
(1.x.y -> 2.0.0).

**Beta**: Libraries defined at a Beta quality level are expected to be mostly
stable and we're working towards their release candidate. We will address issues
and requests with a higher priority.

**Alpha**: Libraries defined at an Alpha quality level are still a
work-in-progress and are more likely to get backwards-incompatible updates.
Additionally, it's possible for Alpha libraries to get deprecated and deleted
before ever being promoted to Beta or GA.

## Contributing changes

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for details on how to contribute to
this project, including how to build and test your changes as well as how to
properly format your code.

