#!/usr/bin/env bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

# https://explainshell.com/explain?cmd=set+-e
set -e
#set -ev

ls submodules/g3log

cmake -E make_directory submodules/build-g3log

# cd submodules/build-g3log
pushd submodules/build-g3log

cmake ../../submodules/g3log -DCPACK_PACKAGING_INSTALL_PREFIX=../../submodules/build-g3log -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release --clean-first -- -j4

sudo make install
