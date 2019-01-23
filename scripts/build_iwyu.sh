#!/bin/bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

set -ev

ls submodules/include-what-you-use

mkdir -vp submodules/build-iwyu

# cd submodules/build-iwyu
pushd submodules/build-iwyu

cmake ../../submodules/include-what-you-use -DIWYU_LLVM_ROOT_PATH=/usr/lib/llvm-6.0

cmake --build . --config Release --clean-first -- -j4
