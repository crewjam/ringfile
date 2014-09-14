#!/bin/bash
set -ex
python tools/cpplint.py {src,include,python}/*.{h,cc} || true
if uname | grep Linux ; then
  sudo apt-get install -qq autoconf-archive
fi

./autogen.sh
./configure
make distcheck
