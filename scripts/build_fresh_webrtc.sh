#!/bin/bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

set -ev

mkdir -vp ~/workspace/

# cd ~/workspace/
pushd ~/workspace/src/

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
