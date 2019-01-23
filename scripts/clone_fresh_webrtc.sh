#!/bin/bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

set -ev

mkdir -vp ~/workspace/

# cd ~/workspace/
pushd ~/workspace/

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git

export PATH=~/workspace/depot_tools:"$PATH"

mkdir webrtc-checkout

cd webrtc-checkout

fetch --nohooks webrtc

# cd src
pushd src

git checkout branch-heads/69

gclient sync

gclient runhooks

./build/install-build-deps.sh
