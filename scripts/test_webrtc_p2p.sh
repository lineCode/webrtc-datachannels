#!/bin/bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

set -ev

cmake -E make_directory ~/workspace/

# cd ~/workspace/
pushd ~/workspace/src/

# see http://ipop-project.org/wiki/Build-WebRTC-Libraries-for-Raspberry-Pi-Zero-(Cross-compile-on-Ubuntu)
# ninja -C ./out/release boringssl protobuf_lite p2p base jsoncpp
file ./out/release/libwebrtc_full.a

#  RUN BEFORE IT bash compile.sh
# https://www.smwenku.com/a/5b8830292b71775d1cdaf404/
# SEE https://github.com/Palakis/webrtc-builds/blob/master/resource/pkgconfig/libwebrtc_full.pc.in
g++ -DWEBRTC_POSIX -std=gnu++11 -o simpleTest WebrtcTestSimple2Peers.cpp -Iinclude -Iinclude/webrtc -Ithird_party/abseil-cpp ./out/release/libwebrtc_full.a  ./out/release/obj/api/libjingle_peerconnection_api.a ./out/release/obj/third_party/boringssl/libboringssl.a -lssl -lcrypto -lpthread -lX11 -ldl -lexpat -Ithird_party/jsoncpp/source/include -Ithird_party/libyuv/include -fno-rtti -fno-exceptions
