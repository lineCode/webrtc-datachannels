WORK IN PROGRESS! This library is not finished yet.

// TODO: freezes with big queue
// TODO:  webrtc::JsepTransportController::OnMessage
// TODO: task queue per thread
// TODO: measure memory
// TODO: USE boost/outcome https://github.com/mmha/gltfpp/blob/master/gltfpp/include/Error.h
// TODO: .clang-tidy https://github.com/mmha/gltfpp/blob/master/.clang-tidy
// TODO: CI https://github.com/mmha/gltfpp/blob/master/.travis.yml and [![Build Status](https://travis-ci.org/mmha/gltfpp.svg?branch=master)](https://travis-ci.org/mmha/gltfpp) http://www.brianlheim.com/2017/08/20/supercollider-travisci.html
// TODO: Doxyfile.in https://github.com/mmha/gltfpp/blob/master/Doxyfile.in
fuzzing test https://chromium.googlesource.com/chromium/src/testing/libfuzzer/+/HEAD/getting_started.md https://raw.githubusercontent.com/mmha/gltfpp/master/README.md
TODO: add helper scripts for clang-format && update in readme below
TODO: scripts to run gdb https://github.com/solarbro/devScripts/blob/76b2ca2db6094afb7931d644d73b821e1c5746ff/clangBuildScripts/runClang.py
TODO: scripts to run ctest N times in parallel https://github.com/ebroder/rugdby/blob/55b0d329d47876fa7102283e0b37eb5547efb0be/test/bin/run-tests.py
and READ!!!! https://a4z.bitbucket.io/blog/2018/05/17/Speed-up-your-test-cycles-with-CMake.html
TODO https://github.com/mozilla/rr
perf tests https://github.com/mozilla/moz2d/blob/master/tests/perf/TestBase.cpp
TODO https://github.com/abseil/abseil-cpp
valgrind or MTuner
create https://github.com/getlantern/natty/blob/master/webrtc-setup script
https://github.com/Tencent/TscanCode
https://github.com/Tencent/phxqueue/wiki/Quickstart-with-Docker
https://github.com/Tencent/paxosstore/blob/master/paxoskv/clean.sh

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

bash scripts/build_clean.sh

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

## NOTE

You should set up your own STUN server for production use, the google server is only for development.
STUN is very simple so this shouldn’t be an issue.

WebSocket++, a header-only C++ WebSockets implementation for our pseudo-signaling server. WebSocket++ itself depends on Boost.Asio

Some corporate networks behind symmetric NAT devices cannot use STUN. This is because symmetric NAT offers additional security by not only associating a local IP with a port, but also with a destination. The NAT device will then only accept connections on the associated port from the original destination server. This means that while the STUN server can still discover the client’s NAT IP, that address is useless to other peers because only the STUN server can respond along it. NOTE: STUN and TURN are unnecessary. ICE has a concept of a host candidate

message sizes are limited in both Mozilla's and Google's implementations at the moment 

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
load balancer
CODESTYLE: noexcept, indent e.t.c
USE Catch2 https://github.com/catchorg/Catch2
USE https://ned14.github.io/outcome/ WATCH https://ned14.github.io/outcome/videos/
RVO2 Library: Reciprocal Collision Avoidance for Real-Time Multi-Agent Simulation & https://gamedev.stackexchange.com/a/136415/123895
iguana universal serialization engine https://github.com/qicosmos/iguana
picohttpparser
c++ html template engines: https://github.com/qicosmos/render https://github.com/melpon/ginger
ormpp Apache License, C++ ORM, C++17, support mysql, postgresql,sqlite https://github.com/qicosmos/ormpp/t
ODB https://www.codesynthesis.com/products/odb/ C++ Object-Relational Mapping (ORM) If the application that is based on ODB is only used internally within the organization, then it is unlikely to be a source of significant revenue while its utility is most likely limited to this organization. As a result, in this case, ODB can be used under the GPL without giving anything back. https://www.codesynthesis.com/products/odb/license.xhtml
SOCI https://github.com/SOCI/soci The C++ Database Access Library
LevelDB https://github.com/Tencent/paxosstore/blob/master/paxoskv/CMakeLists.txt#L21
rocksdb2 https://github.com/facebook/rocksdb
protobuf https://github.com/Tencent/paxosstore/blob/master/paxoskv/CMakeLists.txt#L18 https://github.com/protocolbuffers/protobuf https://github.com/CMU-Perceptual-Computing-Lab/openpose/blob/master/CMakeLists.txt#L373
folly https://trello.com/c/mCQza0wM/27-use-folly NOTE: disable googletest. Dependencies: boost
Snappy, a fast compressor/decompressor. https://github.com/Tencent/paxosstore/blob/master/paxoskv/CMakeLists.txt#L24 https://github.com/google/snappy
abseil https://trello.com/c/xeTTnS0b/30-abseil-c-lib
JSON for Modern C++ https://github.com/nlohmann/json
NuDB https://github.com/vinniefalco/NuDB
Boost.System: for error codes (header only). https://github.com/Broekman/micro
spdlog: fast, header only, C++ logging library. https://github.com/Broekman/micro
receivedMessagesQ_ https://github.com/peterSweter/carSmashCpp/blob/master/networking/Session.cpp#L87
timeouts https://github.com/LeonineKing1199/foxy
coroutine? https://github.com/LeonineKing1199/foxy/blob/master/src/proxy.cpp#L176 https://habr.com/ru/post/348602/
ip spam https://github.com/zirconium-n/Tangerine/blob/master/Tangerine/sgk/general/Game.cpp#L25
IOD https://github.com/matt-42/iod
Use_the_Tools_Available! https://lefticus.gitbooks.io/cpp-best-practices/content/02-Use_the_Tools_Available.html
Astyle https://github.com/node-webrtc/node-webrtc/blob/develop/CMakeLists.txt#L447
Read https://blog.kitware.com/static-checks-with-cmake-cdash-iwyu-clang-tidy-lwyu-cpplint-and-cppcheck/
LOKI https://github.com/DrAlexx/rproto/blob/master/cmake/modules/FindLoki.cmake http://loki-lib.sourceforge.net/
Qwt https://github.com/DrAlexx/rproto/blob/master/cmake/modules/FindQwt.cmake
Qt https://github.com/DrAlexx/rproto/blob/master/q5_app/CMakeLists.txt#L9
Doxygen https://github.com/YuvalNirkin/face_swap/blob/master/CMakeLists.txt#L99
tests https://github.com/YuvalNirkin/face_swap/blob/master/CMakeLists.txt#L115
shared memory
xxHash - Extremely fast hash algorithm https://github.com/Cyan4973/xxHash
GSL: Guidelines Support Library https://github.com/Microsoft/GSL
Google Breakpad https://github.com/telegramdesktop/tdesktop#third-party
Google Crashpad
LZMA SDK https://github.com/jljusten/LZMA-SDK
OpenSSL https://github.com/krzyzanowskim/OpenSSL/blob/master/include-ios/openssl/rc4.h
.travis.yml https://github.com/CMU-Perceptual-Computing-Lab/openpose/blob/master/.travis.yml
https://github.com/lewissbaker/cppcoro
read https://github.com/actor-framework/actor-framework/wiki and https://actor-framework.readthedocs.io/en/latest/Introduction.html
look projects like https://github.com/search?l=C%2B%2B&p=2&q=game+server&type=Repositories kbengine? NoahGameFrame? Nakama? BigWorld?
microservices https://martinfowler.com/articles/microservices.html
Lyft's Envoy https://eng.lyft.com/announcing-envoy-c-l7-proxy-and-communication-bus-92520b6c8191 https://www.youtube.com/watch?v=RVZX4CwKhGE https://github.com/envoyproxy/envoy
Wangle for building services https://medium.com/swlh/starting-a-tech-startup-with-c-6b5d5856e6de
katran L4 balancing https://github.com/facebookincubator/katran
read https://www.gamedev.net/forums/topic/667655-networking-solutions-for-making-mmo/ & https://ru.stackoverflow.com/q/844707
Conan https://blog.conan.io/2018/06/11/Transparent-CMake-Integration.html https://blog.conan.io/2018/12/03/Using-Facebook-Folly-with-Conan.html
Continuous integration https://blog.conan.io/2018/04/25/Continuous-integration-for-C-C++embedded-devices-with-jenkins-docker-and-conan.html
libevent https://eax.me/libevent/
hot reload https://habr.com/ru/post/435260/ https://github.com/iboB/dynamix http://onqtam.com/programming/2017-09-02-simple-cpp-reflection-with-cmake/
RabbitMQ
TODO Switch to C++20 & Clang9+ https://medium.com/@wrongway4you/brief-article-on-c-modules-f58287a6c64
READ about reflection https://gracicot.github.io/reflection/2018/04/03/reflection-present.html
gRPC for server-server communication: Fast protobuf serialization, bi-directional stream, for use in communication between the client and the "broker" instance. https://github.com/jan-alexander/grpc-cpp-helloworld-cmake https://www.slideshare.net/Docker/building-microservices-with-g-rpc-dockercon-april-2017
Minikube Envoy (google for it!) https://piotrminkowski.wordpress.com/2018/04/13/service-mesh-with-istio-on-kubernetes-in-5-steps/ https://meteatamel.wordpress.com/2018/04/24/istio-101-with-minikube/
Envoy+GRPCWeb https://medium.com/google-cloud/grpc-over-browser-javascript-using-grpc-web-on-google-kubernetes-engine-ingress-740789b811e8 https://hackernoon.com/interface-grpc-with-web-using-grpc-web-and-envoy-possibly-the-best-way-forward-3ae9671af67
Управление микросервисами с помощью Kubernetes и Istio https://habr.com/ru/company/jugru/blog/423011/
What is a Service Mesh, and Do I Need One When Developing Microservices? https://www.datawire.io/envoyproxy/service-mesh/
Dedicated Game Servers in Kubernetes https://cloud.google.com/solutions/gaming/running-dedicated-game-servers-in-kubernetes-engine https://www.compoundtheory.com/scaling-dedicated-game-servers-with-kubernetes-part-1-containerising-and-deploying/
Agones: Scaling Multiplayer Dedicated Game Servers with Kubernetes https://www.youtube.com/watch?v=CLNpkjolxYA
kubernetes POSIX shared Memory
kubernetes shared Volume
Multi-Container Pod Design Patterns in Kubernetes https://matthewpalmer.net/kubernetes-app-developer/articles/multi-container-pod-design-patterns.html
https://github.com/aantron/better-enums
https://github.com/electronicarts/EASTL https://rawgit.com/electronicarts/EASTL/master/doc/EASTL%20Modules.html
https://github.com/savoirfairelinux/opendht/wiki/API-Overview https://habr.com/ru/post/325528/
CPACK https://gitlab.com/cppit/jucipp/blob/master/CMakeLists.txt

NOTE: Istio has plans to support additional protocols such as AMQP in the future but for now, the idea of moving from a message based approach to an HTTP centric approach with Istio seems a bit premature. https://www.linkedin.com/pulse/kubernetes-exploring-istio-event-driven-architectures-todd-kaplinger

Protocol Buffers is indeed relatively similar to FlatBuffers, with the primary difference being that FlatBuffers does not need a parsing/ unpacking step to a secondary representation before you can access data, often coupled with per-object memory allocation. The code is an order of magnitude bigger, too. Protocol Buffers has neither optional text import/export nor schema language features like unions.

https://github.com/smfrpc/smf https://www.youtube.com/watch?v=WdFYY3vEcxo
seastar http://www.romange.com/2018/07/12/seastar---asynchronous-c---framework/ https://www.scylladb.com/2018/01/04/seastar-futures/

RESEARCH
gRPC and rabbitmq in microservices? https://dev.to/hypedvibe_7/what-is-the-purpose-of-using-grpc-and-rabbitmq-in-microservices-c4i https://middlewareblog.redhat.com/2018/10/09/reactive-microservices-clustering-messaging-or-service-mesh-a-comparison/
GitOps for Istio https://www.youtube.com/watch?v=VkKMf23ZokY

Caching
https://medium.com/datadriveninvestor/all-things-caching-use-cases-benefits-strategies-choosing-a-caching-technology-exploring-fa6c1f2e93aa

LEARN!!!
etcd+Istio+Deploy Node.js Application to Kubernetes https://medium.com/ibm-cloud/istio-is-not-just-for-microservices-4ed199322bf4 + https://www.youtube.com/watch?v=Nc8jx8oLzss
https://www.youtube.com/watch?v=ZpbXSdzp_vo + https://burrsutter.com/wp-content/uploads/2018/04/BS_022118_DN_8StepsToAwesomeWithKubernetes.pdf
https://htmlpreview.github.io/?https://github.com/redhat-developer-demos/docker-java/blob/devnexus2017/readme.html
http://kubernetesbyexample.com/
istio Concepts https://istio.io/docs/concepts/

DEVOPS BOOKS
?DevOps with Kubernetes https://b-ok.org/book/3428297/120263
?Microservices in Action https://b-ok.org/book/3603729/362a45
>>> Istio+Envoy https://www.oreilly.com/library/view/cloud-native-devops/9781492040750/
>>> https://www.manning.com/books/istio-in-action unlock so you can read each liveBook for a total of 5 minutes per day.  https://livebook.manning.com/#!/book/istio-in-action/chapter-3/v-2/171
?Cloud native programming with Golang
?Pro Devops with Google Cloud Platform https://b-ok.org/book/3611011/bba12f

MISC
https://habr.com/ru/post/430954/ https://wiki.qt.io/Qt_for_WebAssembly NOTE: download size
