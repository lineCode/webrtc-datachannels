#!/bin/bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

set -ev

# see https://stackoverflow.com/a/46516221/10904212

# With cloned webrtc and depot_tools

export PATH=~/workspace/depot_tools:$PATH

#mkdir -vp ~/workspace/
# cmake -E make_directory ~/workspace

# cd ~/workspace/
pushd ~/workspace/webrtc-checkout/src/

export GYP_DEFINES="target_arch=x64 host_arch=x64 build_with_chromium=0 build_with_mozilla=0 use_openssl=1 use_gtk=0 use_x11=0 include_examples=0 include_tests=0 fastbuild=1 remove_webcore_debug_symbols=1 include_pulse_audio=0 include_internal_video_render=0 clang=0"

cmake -E remove_directory  ./out/release

# ####sudo apt-get install libssl1.1.1-dev
# https://wiki.openssl.org/index.php/Compilation_and_Installation
# https://github.com/openssl/openssl/issues/5845

# NOTE: change rtc_ssl_root to OpenSSL ROOT path
# sudo apt-get remove libssl*-dev
# mkdir ~/workspace/openssl/
# cd ~/workspace/openssl/
# git clone https://github.com/openssl/openssl.git -b OpenSSL_1_1_1-stable .
# git checkout OpenSSL_1_1_1-stable
# ./config --prefix=/usr/
# make
# sudo make install
# export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"
# ldd $(type -p openssl)
# openssl version
# ldd /usr/bin/openssl
# stat /usr/lib/libssl.so.*
# stat /usr/lib/libcrypto.so.*

export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"

# edit BUILD.gn
#cflags_cc = [
#  "-Wnon-virtual-dtor",
#  "-Wno-deprecated",
# https://groups.google.com/forum/#!msg/discuss-webrtc/pgHi7Jc9oaU/tGN26gQ5EAAJ

# add to src/rtc_base/opensslutility.cc
# #include <openssl/err.h>

# at src/rtc_base/opensslsessioncache_unittest.cc
# SSL_SESSION_new(ssl_ctx); -> SSL_SESSION_new();

# Now tell compiler to link with openssl
# see https://groups.google.com/a/chromium.org/forum/#!topic/gn-dev/h1aeSmPsxFo
# find in src/rtc_base/BUILD.gn:
# if (!rtc_build_ssl) { ... }
# replace with:
#if (!rtc_build_ssl) {
#  config("external_ssl_library") {
#    assert(rtc_ssl_root != "",
#           "You must specify rtc_ssl_root when rtc_build_ssl==0. Got $rtc_ssl_root")
#    #include_dirs = [ rtc_ssl_root ]
#    include_dirs = [ "$rtc_ssl_root/include" ]
#
#    # lib_dirs = [ "$rtc_ssl_root/lib" ]
#
#    if( is_win ) {
#      libs = [
#        "$rtc_ssl_root/libcrypto.lib",
#        "$rtc_ssl_root/libssl.lib"
#      ]
#    }
#
#    if( is_mac || is_linux || is_ios || is_android ) {
#      libs = [
#        #"crypto",
#        #"ssl",
#        "$rtc_ssl_root/libssl.a",
#        "$rtc_ssl_root/libcrypto.a",
#      ]
#    }
#
#  }
#}

# TODO: use patches https://www.devroom.io/2009/10/26/how-to-create-and-apply-a-patch-with-git/

# see args in https://chromium.googlesource.com/external/webrtc/+/branch-heads/69/webrtc.gni
# see https://gitlab.com/noencoding/OS-X-Chromium-with-proprietary-codecs/wikis/List-of-all-gn-arguments-for-Chromium-build
# see https://github.com/gkorovkin/ports/blob/66ec400346a5da25297fafde22b30f815c51d2fa/devel/webrtc/debian_tools.sh#L126
# NOTE: change PATHS, such as rtc_ssl_root
# NOTE: rtc_use_x11 also disables rtc_desktop_capture_supported
gn gen out/release --args="is_debug=false use_rtti=true target_os=\"linux\" enable_iterator_debugging=false is_component_build=false rtc_use_x11=false rtc_build_json=true rtc_use_pipewire=false rtc_build_examples=false rtc_build_tools=false rtc_include_tests=false use_custom_libcxx=false proprietary_codecs=true use_custom_libcxx_for_host=false rtc_build_ssl=false rtc_ssl_root=\"$HOME/workspace/openssl\" "

#ninja -C ./out/release boringssl protobuf_lite p2p base jsoncpp -t clean
#ninja -C ./out/release boringssl protobuf_lite p2p base jsoncpp

# TODO api audio base call common_audio common_video logging media modules ortc p2p pc rtc_base rtc_tools sdk stats system_wrappers
ninja -C ./out/release rtc_base protobuf_lite p2p base64 jsoncpp rtc_json -t clean
ninja -C ./out/release rtc_base protobuf_lite p2p base64 jsoncpp rtc_json
ninja -C ./out/release -t clean
ninja -C ./out/release

ls ./out/release

# see combine_webrtc_libs.sh
cmake -E remove ./out/release/libwebrtc_full.a

# see combine_webrtc_libs.sh
cmake -E remove_directory include
