#!/usr/bin/env bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

set -ev

cmake -E make_directory ~/Downloads

pushd ~/Downloads

wget http://download.qt.io/official_releases/qt/5.12/5.12.0/qt-opensource-linux-x64-5.12.0.run

chmod u+x qt-opensource-linux-x64-5.12.0.run

sudo ./qt-opensource-linux-x64-5.12.0.run --verbose
