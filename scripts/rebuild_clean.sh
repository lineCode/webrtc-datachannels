#!/usr/bin/env bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

set -ev

# rm -rf build
cmake -E remove_directory build
#cmake -E remove_directory bin

# mkdir build
cmake -E make_directory build
#cmake -E make_directory bin

# cd build
#pushd build

cmake -E chdir build cmake -E time cmake .. -DWEBRTC_SRC_PATH:STRING="/home/denis/workspace/webrtc-checkout/src" -DWEBRTC_TARGET_PATH:STRING="out/release" -DCMAKE_C_COMPILER="/usr/bin/clang-6.0" -DCMAKE_CXX_COMPILER="/usr/bin/clang++-6.0" -DBOOST_ROOT:STRING="/usr" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCLANG_PATH="/usr/lib/llvm-6.0/lib/clang/6.0.1/include" -DENABLE_IWYU=OFF -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DAUTORUN_TESTS=OFF

#cmake --build build --config Release --target clean
#cmake --build build --config Release --target ctest-cleanup
#cmake --build build --config Release -- -j8
cmake -E chdir build cmake -E time cmake --build . --config Debug --clean-first -- -j8
cmake -E chdir build cmake -E time cmake --build . --config Debug --target run_all_tests
