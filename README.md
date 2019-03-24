WORK IN PROGRESS! This library is not finished yet.

    # TODO: >>>>>>>>>
    set(WEBRTC_SRC_PATH "/home/denis/workspace/webrtc-checkout/src")

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

TODO: beast as submodule

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

```
git clone https://gitlab.com/derofim/webrtc-test.git
git submodule update --init --recursive --depth 50
```

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

## Add to your project

1. add as submodule
```
git submodule add https://gitlab.com/derofim/webrtc-test.git submodules/gloer
git submodule update --init --recursive --depth 50
```
2. add_subdirectory( submodules/gloer )
3. target_link_libraries( your_target gloer::core )

See examples folder

## Build webrtc && combine libs to libwebrtc_full

Based on https://docs.google.com/document/d/1J6rcqV5KWpYCZlhWv4vt8Ilrh_f08QC2KA1jbkSBo9s/edit?usp=sharing

Uses webrtc branch-heads/69, see https://chromium.googlesource.com/external/webrtc/+/branch-heads/69

```
sudo apt-get update && sudo apt-get -y install git python
sudo apt-get install clang-4.0 build-essential
sudo apt-get install libcurl4
```

NOTE: change git config to YOURS:
```
git config --global user.name "John Doe"
git config --global user.email "jdoe@email.com"
git config --global core.autocrlf false
git config --global core.filemode false
git config --global color.ui true

bash scripts/clone_fresh_webrtc.sh

bash scripts/build_fresh_webrtc.sh

bash scripts/combine_webrtc_libs.sh

bash scripts/test_webrtc_p2p.sh
```

## Helpful Info

Signaling - The discovery and negotiation process of WebRTC peers is called signaling. For two devices in different networks to find each other they need to use a central service called a signaling server. Using the signaling server two devices can discover each other and exchange negotiation messages. WebRTC does not specify signaling; different technologies such as WebSockets can be used for it.

ICE Candidates - Two peers exchange ICE candidates until they find a method of communication that they both support. After the connection has been established ICE candidates can be traded again to upgrade to a better and faster communication method.

STUN Server - STUN servers are used to get an external network address and to pass firewalls.

Read https://www.scaledrone.com/blog/webrtc-chat-tutorial/

## DEPENDENCIES

### cmake

Install latest cmake (remove old before):
```
bash scripts/install_cmake.sh

sudo apt install clang-format
```

Integrate with your IDE ( QT instructions http://doc.qt.io/qtcreator/creator-beautifier.html )
Import .clang-format rules to IDE settings.

NOTE: don`t forget to use clang-format!

TODO: add helper scripts for clang-format

### openssl

We use OpenSSL_1_1_1-stable. NOTE: Build webrtc with OpenSSL support, see scripts/build_fresh_webrtc.sh!

sudo apt-get remove libssl*-dev
cd submodules/openssl/
./config --prefix=/usr/
make
sudo make install
export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"
ldd $(type -p openssl)
openssl version
ldd /usr/bin/openssl
stat /usr/lib/libssl.so.*
stat /usr/lib/libcrypto.so.*

### boost

Remove old boost:
```
sudo apt-get remove libboost-system-dev libboost-program-options-dev libboost-all-dev -y
sudo apt-get autoremove
sudo rm -rf /usr/local/lib/libboost*
sudo rm -rf /usr/local/include/boost
```

Install new boost:
```
cd ~
wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.bz2 \
    && tar -xjf boost_1_69_0.tar.bz2 \
    && rm -rf boost_1_69_0.tar.bz2 \
    && cd boost_1_69_0 \
    && sudo ./bootstrap.sh --prefix=/usr/ \
    && sudo ./b2 link=shared install

cat /usr/include/boost/version.hpp
```

### lua and LuaJIT

```bash
sudo apt-get install lua5.3 liblua5.3-dev
```

# TODO USES OLD BOOST libluabind-dev

```bash
sudo apt install build-essential libreadline-dev

cd ~ && git clone https://github.com/LuaJIT/LuaJIT.git && cd LuaJIT && make && sudo make install
cd ~ && curl -R -O http://www.lua.org/ftp/lua-5.3.4.tar.gz && tar zxf lua-5.3.4.tar.gz && cd lua-5.3.4 && make linux test
```

LUA:

```bash
sudo apt install lua5.3
```

OR

```bash
yum install epel-release && yum install lua
```

OR

```bash
dnf install lua
```

### include-what-you-use

see https://github.com/include-what-you-use/include-what-you-use
see project submodules!

```bash
sudo apt-get install llvm-6.0-dev libclang-6.0-dev clang-6.0 -y

bash scripts/build_iwyu_submodule.sh
```

Usage:

```bash
# -i for iwyu
bash scripts/build_project.sh -c -i -b Debug
```

NOTE: change -DIWYU_LLVM_ROOT_PATH=/usr/lib/llvm-6.0 in scripts/build_project.sh

NOTE: For Clang on Windows read https://metricpanda.com/rival-fortress-update-27-compiling-with-clang-on-windows

NOTE: don`t use "bits/*" or "*/details/*" includes, add them to mappings file (.imp)

Read:
* https://llvm.org/devmtg/2010-11/Silverstein-IncludeWhatYouUse.pdf
* https://github.com/include-what-you-use/include-what-you-use/tree/master/docs
* https://github.com/hdclark/Ygor/blob/master/artifacts/20180225_include-what-you-use/iwyu_how-to.txt

### QT

```bash
bash scripts/setup_qt5.sh
```

Install at /opt/Qt5.12.0 (paste your version) and check "Qt" checkbox!

# After the installation is complete update the database used by locate so that CMake can immediately find QT.

```bash
sudo updatedb

sudo apt-get install libfontconfig1 mesa-common-dev

# ls /opt/Qt5.12.0/
# export PATH=/opt/Qt5.12.0/5.12.0/gcc_64/bin/:$PATH

# TODO USE sudo ./qt-opensource-linux-x64-5.12.0.run --verbose -platform minimal --script https://github.com/mjoppich/bioGUI/blob/master/silent_qt_install.qs
```

(OPTIONALLY) fix CMAKE_PREFIX_PATH:
set(CMAKE_PREFIX_PATH "/home/qt-everywhere-opensource-src-5.6.0/qtbase")

### g3log

We use g3log for logging

```bash
bash scripts/install_g3log.sh
```

For windows: https://github.com/KjellKod/g3log#building-on-windows

### folly

READ: https://github.com/facebook/folly#build-notes

NOTE: folly requires gcc 5.1+ and a version of boost compiled with C++14 support.

NOTE: we use custom boost and cmake versions (see above)

```bash
sudo apt-get install \
    libevent-dev \
    libdouble-conversion-dev \
    libgflags-dev \
    libiberty-dev \
    liblz4-dev \
    liblzma-dev \
    libsnappy-dev \
    zlib1g-dev \
    binutils-dev \
    libjemalloc-dev \
    pkg-config \
    libunwind8-dev \
    libelf-dev \
    libdwarf-dev
```

From root project dir:

```bash
bash scripts/install_folly.sh
```

## BUILD main project (from root project dir)

USE YOUR OWN WEBRTC_SRC_PATH at cmake configure step!

Tested on g++ (Ubuntu 8.2.0-7ubuntu1) 8.2.0

```bash
bash scripts/build_project.sh -c -t -b RelWithDebInfo
```

OR

```bash
bash scripts/build_project.sh -c -t -b Debug
```

OR

```bash
bash scripts/build_project.sh -c -t -b Release
```

## RUN example projects (from root project dir)

```bash
build/bin/Debug/clent_example/clent_example
```

OR

```bash
build/bin/Debug/server_example/server_example
```

# GDB (from root project dir)

To use with .gdbinit add 'add-auto-load-safe-path .'

Read https://metricpanda.com/tips-for-productive-debugging-with-gdb

To use with sanitizers (ASAN/MSAN/e.t.c) add ASAN_OPTIONS and compile with sanitizers support.

ASAN_OPTIONS=verbosity=1:detect_stack_use_after_return=1:symbolize=1:abort_on_error=1 MSAN_OPTIONS=verbosity=1:abort_on_error=1 gdb build/bin/Debug/server_example/server_example -iex 'add-auto-load-safe-path .'

About MSAN_OPTIONS and ASAN_OPTIONS:
* https://chromium.googlesource.com/chromium/src.git/+/62.0.3178.1/testing/libfuzzer/reproducing.md#debugging
* https://github.com/google/sanitizers/wiki/AddressSanitizerFlags#run-time-flags

After gdb started type:

```bash
r
```

See http://www.yolinux.com/TUTORIALS/GDB-Commands.html

## Run html client (from root project dir)

```bash
bash scripts/run_html_client.sh
```

open http://localhost:8082/example-client.html

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

# Code coverage

```bash
sudo apt-get install lcov gcovr

scripts/code_coverage.sh
```

open build/coverage/index.html

NOTE: coverage requires g++/gcc

## Clang Sanitizers

TODO: libFuzzer https://www.youtube.com/watch?v=FP8zFhB_cOo
TODO -fno-sanitize=vptr -flto -fsanitize=cfi -fsanitize=safe-stack

Read:

* clang.llvm.org/docs/AddressSanitizer.html
* clang.llvm.org/docs/ThreadSanitizer.html
* clang.llvm.org/docs/MemorySanitizer.html
* clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
* llvm.org/docs/LibFuzzer.html
* clang.llvm.org/docs/ControlFlowIntegrity.html
* clang.llvm.org/docs/SafeStack.html
* clang.llvm.org/docs/LeakSanitizer.html
* https://www.usenix.org/sites/default/files/conference/protected-files/enigma_slides_serebryany.pdf

Make sure you have symlink to /usr/bin/llvm-symbolizer (see https://stackoverflow.com/a/24821628/10904212):

sudo ln -s /usr/bin/llvm-symbolizer-6.0 /usr/bin/llvm-symbolizer

MemorySanitizer (without a dynamic component) requires that the entire program code including libraries, (except libc/libm/libpthread, to some extent), is instrumented. Compile libstdc++ with -fsanitize=memory / -fsanitize=thread e.t.c.

Linux is the only platform to support the thread, memory and undefined sanitizer by default. Address sanitizer works on all supported operating systems.

If sanitizers are supported by your compiler, the specified targets will be build with sanitizer support. If your compiler has no sanitizing capabilities (I asume intel compiler doesn't) you'll get a warning but CMake will continue processing and sanitizing will simply just be ignored.

Read http://btorpey.github.io/blog/2014/03/27/using-clangs-address-sanitizer/
Read https://www.jetbrains.com/help/clion/google-sanitizers.html
Read https://genbattle.bitbucket.io/blog/2018/01/05/Dev-Santa-Claus-Part-1/
See https://github.com/arsenm/sanitizers-cmake

### To use -DSANITIZE_UNDEFINED=ON -DSANITIZE_MEMORY=ON -DSANITIZE_THREAD=OFF -DSANITIZE_ADDRESS=OFF:

```bash
scripts/build_project_mem_sanitized.sh
```

### To use -DSANITIZE_UNDEFINED=OFF -DSANITIZE_MEMORY=OFF -DSANITIZE_THREAD=OFF -DSANITIZE_ADDRESS=ON:

```bash
scripts/build_project_addr_sanitized.sh

### To use -DAUTORUN_TESTS=OFF -DSANITIZE_UNDEFINED=OFF -DSANITIZE_MEMORY=OFF -DSANITIZE_THREAD=ON -DSANITIZE_ADDRESS=OFF:
```

```bash
scripts/build_project_thread_sanitized.sh
```

## cppcheck

```bash
sudo apt-get install cppcheck

# check installation
cppcheck-htmlreport -h
```

Usage:

```bash
# -h for cppcheck
bash scripts/build_project.sh -c -h -b Debug
```

open build/doc/cppcheck/index.html

## doxygen

We use [m.css](https://mcss.mosra.cz/doxygen/) doxygen theme.

```bash
sudo apt-get install doxygen

sudo apt install python3-pip

# /usr/bin/python must point to python3
sudo mv /usr/bin/python /usr/bin/python_old
sudo ln -sf /usr/bin/python3 /usr/bin/python
/usr/bin/python --version

# You may need sudo here
pip3 install jinja2 Pygments

sudo apt install \
    texlive-base \
    texlive-latex-extra \
    texlive-fonts-extra \
    texlive-fonts-recommended
```

use cmake build with '--target doxyDoc':

```bash
# -d for doxygen
bash scripts/build_project.sh -c -t -d -b Debug
```

open ./build/doxyDoc/html/index.html

NOTE: Document namespaces in docs/namespaces.dox

NOTE: [Files, di­rec­to­ries and sym­bols with no doc­u­men­ta­tion are not present in the out­put at all](https://mcss.mosra.cz/doxygen/#troubleshooting)

Used [comments style](https://mcss.mosra.cz/doxygen/):

```bash
/**
 * @brief Path utils
 *
 * Example usage:
 *
 * @code{.cpp}
 * const ::fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();
 * @endcode
 **/
```

See:

* [doxygen cheatsheet](http://www.agapow.net/programming/tools/doxygen-cheatsheet/)
* [doxygen coding style](https://doc.magnum.graphics/magnum/coding-style.html#coding-style-documentation)

## valgrind

```bash
sudo apt-get install valgrind

# -v for valgrind
bash scripts/build_project.sh -c -t -v -b Debug
```

See:

* http://valgrind.org/docs/
* https://www.jetbrains.com/help/clion/memory-profiling-with-valgrind.html

## Fuzzing

Fuzzing is a Black Box software testing technique, which basically consists in finding implementation bugs using malformed/semi-malformed data injection in an automated fashion.

See https://github.com/Dor1s/libfuzzer-workshop/blob/master/lessons/01/Modern_Fuzzing_of_C_C%2B%2B_projects_slides_1-23.pdf
See https://github.com/hbowden/nextgen/blob/master/CMakeLists.txt#L92

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

## RTC_DCHECK*

We use RTC_DCHECK*, so use debug builds for testing.

Read:

* https://chromium.googlesource.com/chromium/src/+/master/docs/
* https://www.chromium.org/developers
* https://www.suninf.net/tags/chromium.html
* http://maemo.org/development/documentation/manuals/4-0-x/how_to_use_stun_in_applications/

## Contributing changes

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for details on how to contribute to
this project, including how to build and test your changes as well as how to
properly format your code.
