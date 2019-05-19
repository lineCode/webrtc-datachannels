﻿#!/usr/bin/env bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

# https://explainshell.com/explain?cmd=set+-e
set -e
#set -ev

# cd client
pushd examples/webclient

python -m SimpleHTTPServer 8082
