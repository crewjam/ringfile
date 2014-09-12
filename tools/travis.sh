#!/bin/bash
set -ex
python tools/cpplint.py src/*.cc src/*.h include/*.h || true
sudo apt-get install -qq autoconf-archive
./autogen.sh
./configure
make distcheck
