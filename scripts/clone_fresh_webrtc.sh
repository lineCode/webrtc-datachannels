#!/bin/bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

set -ev

#mkdir -vp ~/workspace/
cmake -E make_directory ~/workspace

# cd ~/workspace/
pushd ~/workspace/

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git || true

export PATH=~/workspace/depot_tools:$PATH

#mkdir webrtc-checkout
cmake -E make_directory webrtc-checkout

pushd webrtc-checkout

fetch --nohooks webrtc || true

# cd src
pushd src

# reset local changes
git checkout -- .
# checkout https://chromium.googlesource.com/external/webrtc/+/branch-heads/71/
#git checkout -b branch-heads/69 || true
#git checkout branch-heads/69 || true
git checkout -b 69 refs/remotes/branch-heads/69 || true
git checkout 69 || true

#git checkout master
#git checkout 4036b0bc638a17021e316cbcc901ad09b509853d

gclient sync

gclient runhooks

# checkout https://chromium.googlesource.com/external/webrtc/+/branch-heads/71/
git checkout -b 69 refs/remotes/branch-heads/69 || true
git checkout 69 || true

./build/install-build-deps.sh || true
