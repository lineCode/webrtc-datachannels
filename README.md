# About

Uses webrtc branch-heads/71 https://chromium.googlesource.com/external/webrtc/+/branch-heads/71

Based on https://github.com/brkho/client-server-webrtc-example
And https://github.com/llamerada-jp/webrtc-cpp-sample/blob/master/main.cpp
And https://github.com/shakandrew/AgarPlusPlus/blob/abbd548ab1d0e0d908778baa9366fc3a83182f88/src/webrtc_server.cpp
And https://github.com/search?p=2&q=%22CreateModularPeerConnectionFactory%22+-filename%3Apeerconnectionfactory_jni.cc+-filename%3Apeerconnectionfactory_jni.cc+-filename%3Acreatepeerconnectionfactory.cc+-filename%3Apeerconnectionfactory.cc+-filename%3Acreate_peerconnection_factory.cc+-filename%3ARTCPeerConnectionFactory.mm+-filename%3Apeerconnectioninterface.h+-filename%3Aobjccallclient.mm+-filename%3Aandroidcallclient.cc+-filename%3Apeerconnection_media_unittest.cc+-filename%3Apeerconnection_integrationtest.cc&type=Code
And https://github.com/shakandrew/AgarPlusPlus/blob/abbd548ab1d0e0d908778baa9366fc3a83182f88/src/network/WebRTCConnectionFactory.cpp
And https://github.com/sourcey/libsourcey/blob/master/src/webrtc/src/peerfactorycontext.cpp
And https://chromium.googlesource.com/external/webrtc/+/master/examples/peerconnection/server/
And https://gist.github.com/MatrixMuto/e37f50567e4b9b982dd8673a1e49dcbe
And https://github.com/notedit/webrtc-clone/tree/master

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

Install latest cmake

sudo apt-get install libboost-system-dev libboost-program-options-dev libboost-all-dev -y

## BUILD (from root project dir)

USE YOUR WEBRTC_SRC_PATH!

mkdir build
cd build
cmake .. -DWEBRTC_SRC_PATH="/home/denis/workspace/webrtc-checkout/src" -DWEBRTC_TARGET_PATH="out/release"
cmake --build . --config Release
./bin/example-server

## One-line build && run

clear && clear ; rm CMake* -rf; cmake .. --clean ; cmake .. -DWEBRTC_SRC_PATH="/home/denis/workspace/webrtc-checkout/src" -DWEBRTC_TARGET_PATH="out/release" ; cmake --build . --config Release -- -j4 ; ./bin/example-server

## Run client (from root project dir)

cd client
python -m SimpleHTTPServer 8081

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