#!/bin/bash

# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

# https://explainshell.com/explain?cmd=set+-e
set -e
#set -ev

# defaults if args not passed
#
# -c: remove old build artifacts
clean='OFF'
#
# -t: build with unit tests
tests='OFF'
#
# -v: build with valgrind unit tests
valgrind='OFF'
#
# -d: build with doxygen docs
doxygen='OFF'
#
# -h: build with cppcheck
cppcheck='OFF'
#
# -i: build with iwyu
iwyu='OFF'
#
# -b: build type
build_type='Debug'

print_usage() {
  printf "Usage: bash build_project.sh -c -t -b Debug"
}

# @see stackoverflow.com/a/21128172/10904212
# @see wiki.bash-hackers.org/howto/getopts_tutorial
# @note: the colon after in 'b:' denotes that -b takes an additional argument
while getopts 'dcvithb:' flag; do
  case "${flag}" in
    # set values if args passed
    d) doxygen='ON' ;;
    c) clean='ON' ;;
    v) valgrind='ON' ;;
    i) iwyu='ON' ;;
    t) tests='ON' ;;
    h) cppcheck='ON' ;;
    b) build_type="${OPTARG}" ;;
    *) print_usage
       exit 1 ;;
  esac
done

if [[ "${clean}" == "ON" ]]; then
  echo "cleaning build dirs..."

  # rm -rf build
  cmake -E remove_directory build
  #cmake -E remove_directory bin

  # mkdir build
  cmake -E make_directory build
  #cmake -E make_directory bin
else
  echo "clean = false"
fi

# cd build
#pushd build

# @note: AddressSanitizer is not compatible with ThreadSanitizer or MemorySanitizer
# @note: ThreadSanitizer is not compatible with MemorySanitizer
# @note: change PATHS, such as DWEBRTC_SRC_PATH
cmake -E chdir build cmake -E time cmake .. -DENABLE_CODE_COVERAGE=OFF -DWEBRTC_SRC_PATH:STRING="/home/denis/workspace/webrtc-checkout/src" -DWEBRTC_TARGET_PATH:STRING="out/release" -DCMAKE_C_COMPILER="/usr/bin/clang-6.0" -DCMAKE_CXX_COMPILER="/usr/bin/clang++-6.0" -DBOOST_ROOT:STRING="/usr" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCLANG_PATH="/usr/lib/llvm-6.0/lib/clang/6.0.1/include" -DENABLE_IWYU=${iwyu} -DCMAKE_BUILD_TYPE=${build_type} -Dgloer_BUILD_TESTS=${tests} -Dgloer_BUILD_EXAMPLES=ON -DAUTORUN_TESTS=OFF -DSANITIZE_UNDEFINED=OFF -DSANITIZE_MEMORY=OFF -DSANITIZE_THREAD=OFF -DSANITIZE_ADDRESS=OFF -DENABLE_VALGRIND_TESTS=${valgrind} -DBUILD_DOXY_DOC=${doxygen}

#cmake --build build --target ctest-cleanup
cmake -E chdir build cmake -E time cmake --build . --config ${build_type} -- -j8

if [[ "${tests}" == "ON" ]]; then
  cmake -E chdir build cmake -E time cmake --build . --config ${build_type} --target run_all_tests
fi

if [[ "${cppcheck}" == "ON" ]]; then
cmake -E chdir build cmake -E time cmake --build . --config Debug --target cppcheck
fi
