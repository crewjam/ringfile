#!/bin/bash
set -ex
python tools/cpplint.py src/*.cc src/*.h include/*.h || true
if uname | grep Linux ; then
  sudo apt-get install -qq autoconf-archive
fi
pip install cffi

./autogen.sh
./configure
make distcheck

